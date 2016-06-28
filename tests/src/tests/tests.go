package tests

import (
    "asserts"
    "net"
    "fmt"
    "time"
    "redis"
)

func test_server_started(channel_out <-chan string) {
    asserts.Assert_chan(channel_out, "Now listening on UDP port 8080...")
}

func test_no_redis_server(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, "Hi UDP Server, How are you doing?")
    asserts.Assert_chan(channel_out, "Error connecting to database: Connection refused")
}


func test_msg_bad_operator(conn net.Conn, channel_out <-chan string) {
    zcard_timestamps := redis.Rediscli("ZCARD", "timestamps")
    zcard_geoindex := redis.Rediscli("ZCARD", "geoindex")
    zcard_geoindex2 := redis.Rediscli("ZCARD", "geoindex_2")
    msg := fmt.Sprintf(`{
    "timestamp":"%d",
    "operator":"badoperator",
    "taxi":"9cf0ebfa-dd37-45c4-8a80-60db584535d8",
    "lat":"2.38852053",
    "lon":"48.84394873",
    "device":"phone",
    "status":"0",
    "version":"1",
    "hash":"2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"
}`, time.Now().Unix())
    fmt.Fprintf(conn, msg)
    asserts.Assert_chan(channel_out, "Error getting user.            Can't find badoperator")
    asserts.Assert_redis(zcard_timestamps, "ZCARD", "timestamps")
    asserts.Assert_redis(zcard_geoindex, "ZCARD", "geoindex")
    asserts.Assert_redis(zcard_geoindex2, "ZCARD", "geoindex_2")
}
func test_msg_ok(conn net.Conn, channel_out <-chan string) {
    msg := fmt.Sprintf(`{
    "timestamp":"%d",
    "operator":"neotaxi",
    "taxi":"9cf0ebfa-dd37-45c4-8a80-60db584535d8",
    "lat":"2.38852053",
    "lon":"48.84394873",
    "device":"phone",
    "status":"0",
    "version":"1",
    "hash":"2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"
}`, time.Now().Unix())
    fmt.Fprintf(conn, msg)
    asserts.Assert_channel_empty(channel_out)
    asserts.Assert_redis("1\n", "ZCARD", "timestamps")
    asserts.Assert_redis("1\n", "ZCARD", "geoindex")
    asserts.Assert_redis("1\n", "ZCARD", "geoindex_2")
}

func test_msg_no_json(conn net.Conn, channel_out <-chan string) {
    asserts.Assert_chan(channel_out, "Now listening on UDP port 8080...")
    fmt.Fprintf(conn, "Hi UDP Server, How are you doing?")
    asserts.Assert_chan(channel_out, "Error parsing json.            Skipping incorrectly formated message...")
}

func test_bad_timestamp(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, `{
    "timestamp":"%d",
    "operator":"neotaxi",
    "taxi":"9cf0ebfa-dd37-45c4-8a80-60db584535d8",
    "lat":"2.38852053",
    "lon":"48.84394873",
    "device":"phone",
    "status":"0",
    "version":"1",
    "hash":"2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"
}`, time.Now().Unix() - 121)

    asserts.Assert_chan(channel_out, "Error checking timestamp.(neotaxi)      Skipping stale or replayed message...")
}
