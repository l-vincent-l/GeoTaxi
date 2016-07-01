package tests

import (
    "utils"
    "log"
)


func Run_test(func_name string, params ... interface{}) {
    funcs := map[string]interface{}{
        "test_starting_message": test_starting_message,
        "test_server_started": test_server_started,
        "test_msg_no_json": test_msg_no_json,
        "test_msg_ok": test_msg_ok,
        "test_no_redis_server": test_no_redis_server,
        "test_bad_timestamp": test_bad_timestamp,
        "test_bad_hash": test_bad_hash,
        "test_msg_bad_operator": test_msg_bad_operator,
        "test_user_100": test_user_100,
        "test_bad_hash_no_auth": test_bad_hash_no_auth,
        "test_msg_bad_operator_no_auth": test_msg_bad_operator_no_auth,
    }
    log.Printf("Running test: %s\n", func_name)
    utils.Call(funcs, func_name, params...)
    log.Println("Ok")
}
