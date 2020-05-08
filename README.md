
# ush

An unsecure shell login program. Do not use under any circumstance. Just did this to see I was able, and to demonstrate how a simple ssh-like server might work "under the hood."

## usage

Server
```sh
./ushd [port]
```

Client
```sh
./ush [host] [port]
```

By default the client and server will want to connect on the localhost port 2222.
