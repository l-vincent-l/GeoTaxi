package asserts

import (
    "log"
    "strings"
    "redis"
)

func Assert_chan(c <-chan string, expected_value string) {
    line := <-c
    Assert(line, expected_value)
}

func Assert(value, expected_value string) {
    if ! strings.EqualFold(value, expected_value)  {
        log.Printf("Expected value: '%s'\n",expected_value)
        log.Printf("Got value:      '%s'\n ", value)
        panic(1)
    }
}

func Assert_channel_empty(c <-chan string) {
    select {
    case x, ok := <-c:
        if ok {
            log.Printf("Expected no value, got %s", x)
            panic(1)
        } else {
            log.Printf("Channel closed!")
            panic(1)
        }
    default:
        return
    }
}

func Assert_redis(expected_value, command string, args ...string) {
    value := redis.Rediscli(command, args...)
    Assert(value, expected_value)
}
