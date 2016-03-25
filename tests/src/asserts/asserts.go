package asserts

import (
    "log"
    "strings"
    "redis"
)

func Assert_chan(c <-chan string, expected_value string) {
    log.Println("Waiting for line")
    line := <-c
    log.Printf("Got line %s\n", line)
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
    log.Printf("Redis command: %s %s\n", command, strings.Join(args, ","))
    Assert(value, expected_value)
}
