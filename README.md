# MEC Inferecne Server ![CMake CI](https://github.com/canhld94/mec-inference-server/workflows/CMake%20CI/badge.svg)

1. What is this project?  
This project build the demo for MEC project in 2020. The main objective is building an FPGA inference server running deep learning algorithms on Intel FPGA and expose RESTful APIs to users.

2. Important notes  
Language: C++  
Build Tool: CMake  
Testing: GTest  
Library: STL, Boost, GFlags

3. Directory structure

```Text
demo_apps/
├── bin                                       // Binary
│   └── lib
├── build                                     // Build directory
│   ├── client_server_beast
│   ├── CMakeFiles
│   ├── format_reader
│   ├── intel64
│   └── ssd_standalone
├── client_server_beast                       // Testing server
│   ├── docs
│   ├── http
│   └── ws
├── cmake-modules                             // CMAKE modules
├── common                                    // Common headers
│   ├── os
│   ├── samples
│   └── ultis
├── format_reader                             // Lib format_reader from intel, better don't touch
├── ssd_inference_server                      // SSD Inference server
└── ssd_standalone                            // Standalone SSD inference application running on FPGA

```

What have been done and what to learn up to

## ---- 2020 / 08 / 04 ----

- Finish implement http-workers, and run with sync server but only CPU device
- _What next?_:
  - refactoring the inference engine
  - Implement inference worker
  - Implement producer-consumer model temporlary use intel TBB queue

## ---- 2020 / 08 / 02 ----

- Resoving the reading from socket problem, it's actually in the client size, not server size
- Adding simple implementation of concurrent queue, adding test and CMAKE CI on github
- _What next?:_
  - Impelment producer-consumer model for FPGA inderence __learn__ --> on going
  - Implement classes of workers: http-workers, inference-workers, websocket-workers

## ---- 2020 / 07 / 29 ----

- TODO: make ssdFPGA class singleton? -> no
- Instead, we make shared_ptr from one object and pass it to worker threads to ignore global variable
- Implement http server in async-model -> done -> still reading take longer than ususal
- _What next?_:
  - Test with fast http server to see if it improve reading performance __fast__
  - Thread-safe queue implementation __learn__
  - Need more investigate on `asio` and `beast` parsing to understand the server code __learn__
  - Impelment producer-consumer model for FPGA inderence __learn__

## ---- 2020 / 07 / 27 ----

- TODO: implement test client --> Done
- Implement thread-safe queue solution for FPGA inference?
- _What next?_:
  - structure of the cluster? proxy, nodes, ..etc
  - Other type of http server in boost (async, SSL, Auth, thread-pool)
  - _Remember_: Bug in `read` that increas TTFB time in the client size!!!

## ---- 2020 / 07 / 25 ----

- So much, first working version
- Problem:
  - With FPGA: Device can't be access with multi-thread, so the current server solution is not working
  - Without FPGA: TTFB and content download time is higher than ussual, idk

## ---- 2020 / 07 / 23 ----

- handle image data from client using `http::string_body` --> `char` array in JPEG form
- create the inference api which response the desired format
- handle JSON with `boost::property_tree`
- create the struture for `ssdFPGA.h`
- now ready to integrate the FPGA inference engine

## ---- 2020 / 07 / 22 ----

- Finish creating a http server that support all basic desired APIs
- JSON response format for inference request
- _Requirement:_
  - JSON parsing for response: how to do it elegantly?
  - Image container:
    - How to send it to server, how the server will handle it?
    - The req created in the server is default basic string, we need to change it to file somehow
    - Should we use opencv?

## ---- 2020 / 07 / 21 ----

- Look into inside of Beast: the message container, body concept
- Adding resolver to the target request
- Should refer implementation from Tensorflow serving
- _Requirement:_
  - Asio
  - Structure of code

## ---- 2020 / 07 / 20 ----

- Cmake project: how cmake work, what is the structure of a CMake project
- Boost Beast: how to build websocket and http server with boost
- Finish building http sync server with boost and support POST
- _Requirement:_
  - Clear definition of the APT (get,post)
  - HTTP RFC7023
  - Boost Beast

## ---- 2020 / 07 / 15 ----

- OpenVino setup running environment
- Docker
