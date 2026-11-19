"""Serve the HTML order form and connect it to a running C++ matching engine."""

import argparse
import json
import re
import subprocess
from html import escape
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from string import Template
from threading import Lock
from urllib.parse import parse_qs, urlsplit


WEB_DIR = Path(__file__).resolve().parent
MAX_ORDER_VALUE = 2_147_483_647


def parse_order(fields):
    side, price, quantity = (fields[key] for key in ("side", "price", "quantity"))
    if side not in ("buy", "sell"):
        raise ValueError("Choose Buy or Sell.")
    if not re.fullmatch(r"[0-9]+", quantity) or not 1 <= int(quantity) <= MAX_ORDER_VALUE:
        raise ValueError("Enter a whole-share quantity between 1 and 2,147,483,647.")
    price_parts = re.fullmatch(r"([0-9]+)(?:\.([0-9]{1,2}))?", price)
    if not price_parts:
        raise ValueError("Enter a USD limit price with up to two decimal places.")
    dollars, cents = price_parts.groups()
    price_cents = int(dollars) * 100 + int((cents or "").ljust(2, "0"))
    if not 1 <= price_cents <= MAX_ORDER_VALUE:
        raise ValueError("Enter a limit price between $0.01 and $21,474,836.47.")
    return side, price_cents, int(quantity)


def format_price(cents):
    dollars, remainder = divmod(cents, 100)
    return f"${dollars:,}.{remainder:02d}"


def render_rows(rows, empty_message, columns):
    if not rows:
        return f'<tr><td class="empty" colspan="{columns}">{empty_message}</td></tr>'
    return "".join("<tr>" + "".join(f"<td>{cell}</td>" for cell in row) + "</tr>" for row in rows)


def render_receipt(order):
    if order is None:
        return "Your order ID will appear here."
    if order["open"] == 0:
        status = "Filled"
    elif order["filled"] == 0:
        status = "Open"
    else:
        status = "Partially filled"
    heading = f'<strong>Order #{order["id"]} · {status}</strong>'
    return heading + f'<span>{order["filled"]:,} filled · {order["open"]:,} open at submission</span>'


class OrderBookServer(ThreadingHTTPServer):
    def __init__(self, port, engine):
        self.engine = engine
        self.engine_lock = Lock()
        self.receipts = {}
        self.template = Template((WEB_DIR / "index.html").read_text(encoding="utf-8"))
        self.styles = (WEB_DIR / "styles.css").read_bytes()
        super().__init__(("127.0.0.1", port), OrderBookHandler)

    def request_engine(self, command):
        # The caller holds engine_lock for the complete request/response exchange.
        self.engine.stdin.write(command + "\n")
        self.engine.stdin.flush()
        response = self.engine.stdout.readline()
        if not response:
            raise ConnectionError("The matching engine process exited.")
        result = json.loads(response)
        if "error" in result:
            raise ConnectionError(result["error"])
        return result

    def submit(self, side, price_cents, quantity):
        with self.engine_lock:
            receipt = self.request_engine(f"{side} {price_cents} {quantity}")
            order_id = str(receipt["id"])
            self.receipts[order_id] = receipt
            return order_id

    def page(self, order_id="", fields=None, error=""):
        with self.engine_lock:
            snapshot = self.request_engine("snapshot")
            receipt = self.receipts.get(order_id)

        fields = fields or {}
        sell_selected = fields.get("side") == "sell"
        buys = [(format_price(price), f"{quantity:,}") for price, quantity in snapshot["buys"]]
        sells = [(format_price(price), f"{quantity:,}") for price, quantity in snapshot["sells"]]
        trades = [(f"#{trade_id}", format_price(price), f"{quantity:,}")
                for trade_id, price, quantity in snapshot["trades"]]
        return self.template.substitute(
            buy_checked="" if sell_selected else "checked",
            sell_checked="checked" if sell_selected else "",
            quantity=escape(fields.get("quantity", "")),
            price=escape(fields.get("price", "")),
            error=f'<p class="error" role="alert">{escape(error)}</p>' if error else "",
            receipt=render_receipt(receipt),
            buy_rows=render_rows(buys, "No open buy orders", 2),
            sell_rows=render_rows(sells, "No open sell orders", 2),
            trade_rows=render_rows(trades, "No trades yet", 3),
        )


class OrderBookHandler(BaseHTTPRequestHandler):
    def send_content(self, status, content_type, content):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(content)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(content)

    def send_page(self, status=200, **context):
        try:
            page = self.server.page(**context)
        except (OSError, json.JSONDecodeError):
            self.send_error(503, "The matching engine is unavailable.")
            return
        self.send_content(status, "text/html; charset=utf-8", page.encode("utf-8"))

    def do_GET(self):
        url = urlsplit(self.path)
        if url.path == "/":
            order_id = parse_qs(url.query).get("order", [""])[0]
            self.send_page(order_id=order_id)
        elif url.path == "/styles.css":
            self.send_content(200, "text/css; charset=utf-8", self.server.styles)
        else:
            self.send_error(404)

    def do_POST(self):
        if self.path != "/orders":
            self.send_error(404)
            return
        if self.headers.get_content_type() != "application/x-www-form-urlencoded":
            self.send_error(415)
            return

        try:
            length = int(self.headers.get("Content-Length", ""))
            if not 1 <= length <= 1024:
                raise ValueError
            body = self.rfile.read(length)
            if len(body) != length:
                raise ValueError
            form = parse_qs(body.decode("ascii"), keep_blank_values=True, max_num_fields=3)
            if set(form) != {"side", "price", "quantity"} or any(len(values) != 1 for values in form.values()):
                raise ValueError
        except ValueError:
            self.send_page(400, error="Enter a side, quantity, and limit price.")
            return

        fields = {key: values[0] for key, values in form.items()}
        try:
            order = parse_order(fields)
        except ValueError as error:
            self.send_page(400, fields=fields, error=str(error))
            return

        try:
            order_id = self.server.submit(*order)
        except (OSError, json.JSONDecodeError):
            self.send_error(503, "The matching engine is unavailable.")
            return
        self.send_response(303)
        self.send_header("Location", f"/?order={order_id}")
        self.send_header("Content-Length", "0")
        self.end_headers()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path, help="path to the compiled order_book_web executable")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()

    try:
        with subprocess.Popen([str(args.engine.resolve())], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, encoding="utf-8") as engine:
            with OrderBookServer(args.port, engine) as server:
                print(f"Order Book V1 running at http://127.0.0.1:{server.server_port}", flush=True)
                try:
                    server.serve_forever()
                except KeyboardInterrupt:
                    pass
    except OSError as error:
        parser.exit(1, f"Could not start Order Book V1: {error}\n")


if __name__ == "__main__":
    main()
