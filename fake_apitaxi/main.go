package main

import (
    "fmt"
    "net/http"
)

func handler(w http.ResponseWriter, r *http.Request) {
    fmt.Fprintf(w, `{"data":[{"name": "neotaxi", "apikey": "apikey"}]}` )
}

func main() {
    http.HandleFunc("/users", handler)
    http.ListenAndServe(":5000", nil)
}
