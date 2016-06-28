package asserts

import (
    "log"
    "strings"
    "redis"
    "time"
)

func Assert_chan(c <-chan string, expected_value string) {
    log.Println("Waiting for line")
    time.Sleep(1 * time.Millisecond)
    select {
    case line, ok := <-c:
        if ok {
            log.Printf("Got line %s\n", line)
            Assert(line, expected_value)
        } else {
            log.Printf("Channel closed")
            panic(1)
        }
    default:
        log.Printf("No value found")
        panic(1)
    }
}

func Assert(value, expected_value string) {
    if ! strings.EqualFold(value, expected_value)  {
        log.Printf("Expected value: '%s'\n",expected_value)
        log.Printf("Got value:      '%s'\n ", value)
        panic(1)
    }
}

func Assert_channel_empty(c <-chan string) {
    time.Sleep(1 * time.Millisecond)
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
        log.Printf("No value to read")
        return
    }
}

func Assert_redis(expected_value, command string, args ...string) {
    value := redis.Rediscli(command, args...)
    log.Printf("Redis command: %s %s\n", command, strings.Join(args, ","))
    Assert(value, expected_value)
}
