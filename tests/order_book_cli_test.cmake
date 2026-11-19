function(run_cli name input_file expected_status expected_error)
    execute_process(
        COMMAND "${ORDER_BOOK}"
        INPUT_FILE "${input_file}"
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
        RESULT_VARIABLE status
        TIMEOUT 5
    )
    string(STRIP "${error}" error)
    if(NOT "${status}" STREQUAL "${expected_status}" OR NOT "${error}" STREQUAL "${expected_error}")
        message(FATAL_ERROR "${name}: expected status ${expected_status} and stderr '${expected_error}', got ${status} and '${error}'")
    endif()
    string(REGEX REPLACE "[ \t]+\n" "\n" output "${output}")
    string(STRIP "${output}" output)
    set(cli_output "${output}" PARENT_SCOPE)
endfunction()

function(check_output name expected actual)
    string(STRIP "${expected}" expected)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(FATAL_ERROR "${name}: expected stdout\n${expected}\nActual stdout\n${actual}")
    endif()
endfunction()

function(run_text name input expected_status expected_error)
    set(input_file "${CMAKE_CURRENT_BINARY_DIR}/cli_${name}.txt")
    file(WRITE "${input_file}" "${input}")
    run_cli("${name}" "${input_file}" "${expected_status}" "${expected_error}")
    set(cli_output "${cli_output}" PARENT_SCOPE)
endfunction()

run_cli(ten_orders "${CMAKE_CURRENT_LIST_DIR}/data/ten_orders.txt" 0 "")
check_output(ten_orders [=[
Order Book:
Buy Orders:
Buy Order ID: 10, Ideal Price: 98, Stock Number: 1
Buy Order ID: 14, Ideal Price: 104, Stock Number: 2
Sell Orders:
Sell Order ID: 1, Ideal Price: 105, Stock Number: 2
Sell Order ID: 15, Ideal Price: 106, Stock Number: 1
Trade History:
Trade Order ID: 5, Ideal Price: 100, Stock Number: 3
Trade Order ID: 6, Ideal Price: 100, Stock Number: 2
Trade Order ID: 8, Ideal Price: 100, Stock Number: 2
Trade Order ID: 12, Ideal Price: 99, Stock Number: 4
Trade Order ID: 13, Ideal Price: 98, Stock Number: 1
]=] "${cli_output}")

set(sweep_input "100\n")
foreach(price RANGE 100 108)
    foreach(order RANGE 1 11)
        string(APPEND sweep_input "SELL ${price} 1\n")
    endforeach()
endforeach()
string(APPEND sweep_input "BUY 107 90\n")
run_text(sweep "${sweep_input}" 0 "")
string(REGEX MATCHALL "Trade Order ID: [0-9]+, Ideal Price: [0-9]+, Stock Number: 1" trades "${cli_output}")
list(LENGTH trades trade_count)
if(NOT trade_count EQUAL 88 OR NOT cli_output MATCHES "Buy Order ID: 100, Ideal Price: 107, Stock Number: 2")
    message(FATAL_ERROR "100-order sweep should print 88 trades and an open buy for two shares")
endif()

set(single_buy [=[
Order Book:
Buy Orders:
Buy Order ID: 1, Ideal Price: 100, Stock Number: 3
Sell Orders:
Trade History:
]=])

function(check_bad_row name row diagnostic)
    run_text("${name}" "2\n${row}\nBUY 100 3\n" 1 "${diagnostic}")
    check_output("${name}" "${single_buy}" "${cli_output}")
endfunction()

check_bad_row(zero_quantity "SELL 100 0" "Invalid quantity for order row 1")
check_bad_row(negative_quantity "SELL 100 -2" "Invalid quantity for order row 1")
check_bad_row(missing_quantity "SELL 100" "Invalid order row 1")
check_bad_row(nonnumeric_price "SELL cents 2" "Invalid order row 1")
check_bad_row(nonnumeric_quantity "SELL 100 shares" "Invalid order row 1")
check_bad_row(unknown_side "HOLD 100 2" "Invalid order type: HOLD")
check_bad_row(counted_lowercase "buy 100 2" "Invalid order type: buy")
check_bad_row(price_overflow "SELL 2147483648 2" "Invalid order row 1")
check_bad_row(quantity_overflow "SELL 100 2147483648" "Invalid order row 1")
check_bad_row(extra_field "SELL 100 2 extra" "Invalid order row 1")

function(check_bad_count name input)
    run_text("${name}" "${input}" 1 "Invalid order count")
    check_output("${name}" "" "${cli_output}")
endfunction()

check_bad_count(empty_input "")
check_bad_count(nonnumeric_count "two\n")
check_bad_count(negative_count "-1\n")
check_bad_count(count_overflow "2147483648\n")
check_bad_count(extra_count_field "2 rows\n")
run_text(missing_row "2\nBUY 100 3\n" 1 "Missing order row 2")
check_output(missing_row "${single_buy}" "${cli_output}")

run_text(zero_orders "0\n" 0 "")
check_output(zero_orders "Order Book:\nBuy Orders:\nSell Orders:\nTrade History:" "${cli_output}")

run_text(counted_print_and_cancel "4\nBUY 100 3\nPRINT\nCANCEL 1\nPRINT\n" 0 "")
check_output(counted_print_and_cancel [=[
Order Book:
Buy Orders:
Buy Order ID: 1, Ideal Price: 100, Stock Number: 3
Sell Orders:
Trade History:

Order Book:
Buy Orders:
Sell Orders:
Trade History:
]=] "${cli_output}")

run_text(integer_boundary "3\nBUY 2147483647 2147483647\nSELL 2147483647 2147483647\nBUY 0 1\n" 0 "")
check_output(integer_boundary [=[
Order Book:
Buy Orders:
Buy Order ID: 4, Ideal Price: 0, Stock Number: 1
Sell Orders:
Trade History:
Trade Order ID: 3, Ideal Price: 2147483647, Stock Number: 2147483647
]=] "${cli_output}")

run_text(counted_caller_ids "7\nBUY 1 100 3\nSELL 42 105 5\nBUY 99 2\nSELL 1 105 2\nPRINT\nCANCEL 42\nPRINT\n" 1 "Duplicate order ID 1")
check_output(counted_caller_ids [=[
Order Book:
Buy Orders:
Buy Order ID: 2, Ideal Price: 99, Stock Number: 2
Buy Order ID: 1, Ideal Price: 100, Stock Number: 3
Sell Orders:
Sell Order ID: 42, Ideal Price: 105, Stock Number: 5
Trade History:

Order Book:
Buy Orders:
Buy Order ID: 2, Ideal Price: 99, Stock Number: 2
Buy Order ID: 1, Ideal Price: 100, Stock Number: 3
Sell Orders:
Trade History:
]=] "${cli_output}")

run_text(readme_example "SELL 1 10100 8\nBUY 2 10100 3\nBUY 3 10000 10\nPRINT\n" 0 "")
check_output(readme_example [=[
TRADE: 3 shares at $101.00

SELL
$101.00 | 5

BUY
$100.00 | 10
]=] "${cli_output}")

run_text(uncounted_lowercase "BUY 42 100 3\nsell 43 100 3\nPRINT\n" 1 "Invalid command: sell")
check_output(uncounted_lowercase "SELL\n\nBUY\n$1.00 | 3" "${cli_output}")

run_text(command_depth "SELL 72 10100 6\nSELL 19 10090 5\nSELL 21 10090 8\nBUY 11 10004 7\nBUY 20 10010 2\nBUY 69 10010 3\nCANCEL 19\nPRINT\n" 0 "")
check_output(command_depth [=[
SELL
$100.90 | 8
$101.00 | 6

BUY
$100.10 | 5
$100.04 | 7
]=] "${cli_output}")

run_text(command_invalid_ids "SELL 72 10100 6\nBUY 9 10100 6\nBUY 72 10000 9\nCANCEL 72\nCANCEL 123\nBUY 17 10000 4\nCANCEL 17\nCANCEL 17\nBUY 17 10000 9\nPRINT\n" 1
    "Duplicate order ID 72\nCannot cancel order 72\nCannot cancel order 123\nCannot cancel order 17\nDuplicate order ID 17")
check_output(command_invalid_ids [=[
TRADE: 6 shares at $101.00

SELL

BUY
]=] "${cli_output}")
