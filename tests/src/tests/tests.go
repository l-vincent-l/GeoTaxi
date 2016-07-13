package tests

import (
    "asserts"
    "net"
    "fmt"
    "time"
    "redis"
    "encoding/json"
    "crypto/sha1"
)

func generate_message(timestamp, operator, taxi, lat, lon, device, status,
                      version, hash, apikey string) string {
   if timestamp == "" {
        timestamp = fmt.Sprintf("%d", time.Now().Unix())
    }
    to_send := map[string]string{
        "operator": operator,
        "taxi": taxi,
        "lat": lat,
        "lon": lon,
        "device": device,
        "status": status,
        "version":"1",
        "timestamp": timestamp,
    }
    if hash == "" {
        c :=  fmt.Sprintf("%s%s%s%s%s%s%s%s%s",
                to_send["timestamp"], to_send["operator"], to_send["taxi"],
                to_send["lat"], to_send["lon"], to_send["device"],
                to_send["status"], to_send["version"], apikey)
        to_send["hash"] = fmt.Sprintf("%x", sha1.Sum([]byte(c)))
   } else {
        to_send["hash"] = hash
    }
    marshaled, _ := json.Marshal(to_send)
    return string(marshaled)
}

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
    fmt.Fprintf(conn, generate_message("", "badoperator",
        "9cf0ebfa-dd37-45c4-8a80-60db584535d8", "2.38852053", "48.84394873",
        "phone", "0", "1", "", "apikey"))
    asserts.Assert_chan(channel_out, "Error getting user.            Can't find badoperator")
    asserts.Assert_redis(zcard_timestamps, "ZCARD", "timestamps")
    asserts.Assert_redis(zcard_geoindex, "ZCARD", "geoindex")
    asserts.Assert_redis(zcard_geoindex2, "ZCARD", "geoindex_2")
}

func test_bad_hash(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, generate_message("", "neotaxi",
        "taxi2", "2.38852053", "48.84394873",
        "phone", "0", "1", "badhash", "apikey"))
    asserts.Assert_chan(channel_out, "Error checking signature.      Storing it in redis")
    asserts.Assert_redis("1\n", "ZCARD", "timestamps")
    asserts.Assert_redis("1\n", "ZCARD", "geoindex")
    asserts.Assert_redis("1\n", "ZCARD", "geoindex_2")
    asserts.Assert_redis("1\n", "ZCARD", "badhash_operators")
    asserts.Assert_redis("1\n", "ZCARD", "badhash_taxis_ids")
    asserts.Assert_redis("1\n", "ZCARD", "badhash_ips")
}

func test_msg_ok(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, generate_message("", "neotaxi",
        "9cf0ebfa-dd37-45c4-8a80-60db584535d8", "2.38852053", "48.84394873",
        "phone", "0", "1", "", "apikey"))
    asserts.Assert_channel_empty(channel_out)
    asserts.Assert_redis("1\n", "ZCARD", "timestamps")
    asserts.Assert_redis("1\n", "ZCARD", "geoindex")
    asserts.Assert_redis("1\n", "ZCARD", "geoindex_2")
}

func test_starting_message(channel_out <-chan string) {
    asserts.Assert_chan(channel_out, "Now listening on UDP port 8080...")
}

func test_msg_no_json(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, "Hi UDP Server, How are you doing?")
    asserts.Assert_chan(channel_out, "Error parsing json.            Skipping incorrectly formated message...")
}

func test_bad_timestamp(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, generate_message("1", "neotaxi",
        "9cf0ebfa-dd37-45c4-8a80-60db584535d8", "2.38852053", "48.84394873",
        "phone", "0", "1", "", "apikey"))

    asserts.Assert_chan(channel_out, "Error checking timestamp.(neotaxi)      Skipping stale or replayed message...")
}

func test_user_100(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, generate_message("", "neotaxi100",
        "taxi100", "2.38852053", "48.84394873",
        "phone", "0", "1", "", "apikey100"))
    asserts.Assert_channel_empty(channel_out)
    asserts.Assert_redis("1\n", "ZRANK", "geoindex", "taxi100")
}


func test_msg_bad_operator_no_auth(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, generate_message("", "badoperator",
        "9cf0ebfa-dd37-45c4-8a80-60db584535d8", "2.38852053", "48.84394873",
        "phone", "0", "1", "", "apikey"))
    asserts.Assert_channel_empty(channel_out)
    asserts.Assert_redis("1\n", "ZCARD", "timestamps")
    asserts.Assert_redis("1\n", "ZCARD", "geoindex")
    asserts.Assert_redis("1\n", "ZCARD", "geoindex_2")
}

func test_bad_hash_no_auth(conn net.Conn, channel_out <-chan string) {
    fmt.Fprintf(conn, generate_message("", "neotaxi",
        "taxi2", "2.38852053", "48.84394873",
        "phone", "0", "1", "badhash", "apikey"))
    asserts.Assert_channel_empty(channel_out)
    asserts.Assert_redis("2\n", "ZCARD", "timestamps")
    asserts.Assert_redis("2\n", "ZCARD", "geoindex")
    asserts.Assert_redis("2\n", "ZCARD", "geoindex_2")
    asserts.Assert_redis("0\n", "ZCARD", "badhash_operators")
    asserts.Assert_redis("0\n", "ZCARD", "badhash_taxis_ids")
    asserts.Assert_redis("0\n", "ZCARD", "badhash_ips")
}

