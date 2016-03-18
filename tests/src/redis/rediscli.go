package redis

import (
    "os/exec"
    "bytes"
    "log"
)

func Rediscli(command string, args ...string) string {
    cmd := exec.Command("redis-cli", append([]string{"-c", command}, args...)...)
    var out bytes.Buffer
    cmd.Stdout = &out
    err := cmd.Run()
    if err != nil {
        log.Printf("Error executing redis command: %s\n", command)
        log.Printf("Error: %s\n", err)
        log.Panic(err)
    }
    return out.String()
}
