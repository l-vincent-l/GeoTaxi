package tests

import (
    "utils"
    "log"
)


func Run_test(func_name string, params ... interface{}) {
    funcs := map[string]interface{}{
        "test_server_started": test_server_started,
        "test_msg_no_json": test_msg_no_json,
        "test_msg_ok": test_msg_ok,
        "test_no_redis_server": test_no_redis_server,
        "test_bad_timestamp": test_bad_timestamp,
        "test_bad_hash": test_bad_hash,
        "test_msg_bad_operator": test_msg_bad_operator,
    }
    log.Printf("Running test: %s\n", func_name)
    utils.Call(funcs, func_name, params...)
    log.Println("Ok")
}
