set(input "${CMAKE_CURRENT_BINARY_DIR}/web_adapter_orders.txt")
file(WRITE "${input}" [=[
buy 10015 12
sell 73 10015 8
buy 42 10015 12
buy 42 10000 5
snapshot
cancel 73
cancel 42
cancel 42
cancel 999
snapshot
]=])

execute_process(
    COMMAND "${ORDER_BOOK_WEB}"
    INPUT_FILE "${input}"
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    RESULT_VARIABLE status
    TIMEOUT 5
)

set(expected [=[
{"error":"Invalid engine request"}
{"id":73,"filled":0,"open":8}
{"id":42,"filled":8,"open":4}
{"accepted":false}
{"buys":[[10015,4]],"sells":[],"trades":[[1,10015,8,42,73]]}
{"canceled":false}
{"canceled":true}
{"canceled":false}
{"canceled":false}
{"buys":[],"sells":[],"trades":[[1,10015,8,42,73]]}
]=])

if(NOT "${status}" STREQUAL "0" OR NOT "${error}" STREQUAL "")
    message(FATAL_ERROR "Web adapter exited with status ${status}: ${error}")
endif()
if(NOT "${output}" STREQUAL "${expected}")
    message(FATAL_ERROR "Expected web adapter output:\n${expected}\nActual output:\n${output}")
endif()
