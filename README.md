# Secure P2P Micropayment System
This is a P2P (person-to-person) micropayment system that supports secure message transmission.

## Overview
- [Secure P2P Micropayment System](#secure-p2p-micropayment-system)
  - [Overview](#overview)
  - [Environment](#environment)
  - [Building](#building)
  - [Running](#running)

## Environment

- Ubuntu 24.04 LTS
- gcc 13.2.0
- GNU Make 4.3

## Building

There are several ways to build the programs:

- Build all the programs:
  ```bash
  make
  ```
  This will generate two programs: `client` and `server`, under the current directory.

- Or accelerate the building phase with multiple cores:
  ```bash
  make -j$(nproc)
  ```
  *Warning: build with this method may mess up the output messages from builing.*

- Only build the client program:
  ```bash
  make client
  ```
  This will only generate `client`.

- Only build the server program:
  ```bash
  make server
  ```
  This will only generate `server`.

Clear the outputs generated from the building phase:
```bash
make clean
```
This will delete both `client` and `server`.

## Running

1. Run the server side:
   ```bash
   ./server <port>
   ```
   - `port`: Specify the port to which the server should bind. The range is between [1024, 65535].  
 
2. Run the client side:
   ```bash
   ./client <host> <port>
   ``` 
   - `host`: The host that server is running on. If the server is run locally, type `127.0.0.1`.
   - `port`: The port to which the server binds.
   
   Note that it is possible to run multiple clients simultaneously.
