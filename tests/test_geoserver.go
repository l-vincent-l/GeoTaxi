package main

import (
    "os/exec"
    "log"
    "time"
    "bufio"
    "net"
    "fmt"
    "os"
    "redis"
    "tests"
)


func catch_stdout(cmd *exec.Cmd) <-chan string{
    channel_out := make(chan string);
    out, err := cmd.StdoutPipe(); if err != nil {
        log.Fatal(err)
    }
    scanner := bufio.NewScanner(out)
    go func() {
        for scanner.Scan() {
            channel_out <- scanner.Text()
        }
    }()
    return channel_out
}

func main() {
    defer func() {
        if r := recover(); r != nil {
            log.Println("Test ko return: ", r)
            os.Exit(1)
        }
    }()
    cmd := exec.Command(os.Args[1], "8080")
    channel_out := catch_stdout(cmd)

    err := cmd.Start(); if err != nil {
        log.Panic(err)
    }
    defer cmd.Process.Kill()

    time.Sleep(1 * time.Second)
    tests.Run_test("test_server_started", channel_out)
    log.Println("geoserver is started")

    log.Println("Initializing UDP connection")
    conn, err := net.Dial("udp", "127.0.0.1:8080")
    if err != nil {
        fmt.Printf("Some error %v", err)
        return
    }
    tests.Run_test("test_no_redis_server", conn, channel_out)

    log.Println("Starting redis-server")
    cmd_redis := exec.Command(os.Args[2], "--module-add", os.Args[3])
    err = cmd_redis.Start(); if err != nil {
        log.Println("Unable to start redis-server")
        log.Panic(err)
    }
    defer cmd_redis.Process.Kill()
    log.Println("redis-server started")
    time.Sleep(1 * time.Second)
    redis.Rediscli("DEL", "timestamps", "geoindex", "geoindex2")

    tests.Run_test("test_msg_no_json", conn, channel_out)
    tests.Run_test("test_msg_ok", conn, channel_out)
    tests.Run_test("test_bad_timestamp", conn, channel_out)

    log.Println("Tests ok")
}
