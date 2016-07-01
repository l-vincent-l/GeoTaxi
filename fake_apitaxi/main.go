package main

import (
    "fmt"
    "net/http"
    "encoding/json"
)
type User struct {
    Name string `json:"name"`
    Apikey string `json:"apikey"`
}

type Content struct {
    Data []User `json:"data"`
}

func handler(w http.ResponseWriter, r *http.Request) {
    content := Content{}
    content.Data = append(content.Data, User{"neotaxi", "apikey"})
    for i:=1; i<=100; i++ {
        content.Data = append(content.Data,
            User{fmt.Sprintf("neotaxi%d", i), fmt.Sprintf("apikey%d", i)})
    }
    b, err := json.Marshal(content); if err != nil {
        fmt.Println("error: ", err)
    }
    s := string(b)
    fmt.Fprintf(w, s)
}

func main() {
    http.HandleFunc("/users", handler)
    http.ListenAndServe(":5000", nil)
}
