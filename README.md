# something

Everybody (include [Circle CI](https://twitter.com/circleci/status/951635852974854144?lang=en)) tell me I should build something, so I build something

## Introduction

This project build an inference server with ~~Intel FPGA~~ Intel CPU, Intel FPGA, and NVIDIA GPU backend. Currently the inference engine supports object detection object detection models (`SSD`, `YoLov3`*, and `Faster R-CNN` family); and the server support ~~REST~~ gRPC and REST. At a glance:

*Request*

```SH
curl --location --request POST 'xxx.xxx.xxx.xxx:8080/inference' \
--header 'Content-Type: image/jpeg' \
--data-binary '@/C:/Users/CanhLD/Desktop/Demo/AirbusDrone.jpg'
```

*Return*

```JSON
{
    "predictions": [
        {
            "label_id": "1",
            "label": "plane",
            "confidences": "0.998418033",
            "detection_box": [
                "182",
                "806",
                "291",
                "919"
            ]
        },
        {
            "label_id": "1",
            "label": "plane",
            "confidences": "0.997635841",
            "detection_box": [
                "26",
                "182",
                "137",
                "309"
            ]
        }
    ]
}
```

> **_NOTE:_**  I do not implement yolo for GPU

## Requirements

The server object and protocol object depends on following packages. I strongly recommend install them with [Conan](https://conan.io/), so you do not need to modify the CMake files.

```
boost==1.73.0: socket and IPC, networking, HTTP parsing and serializing, JSON parsing and serializing
spdlog==1.7.0: logging and debugging
glfags==2.2.2: argv parsing
```

For inference engine, I implemented CPU and FPGA inference with [Intel OpenVino](https://docs.openvinotoolkit.org/2019_R1.1/index.html), and GPU inference with [NVIDIA TensorRT](https://developer.nvidia.com/tensorrt). In order to using grpc, you should install [grpc for C++](https://grpc.io/docs/languages/cpp/quickstart/#install-grpc). Please refer to their document to install the framework.

```
grpc==1.32.0
openvino==2019R1.1
opencv==4.1 (comes with openvino)
tensorrt==7.1.3.4
cuda==10.2 (comes with tensorrt)
```

>*NOTE* As these packages are quite big with lots of dependencies, make sure you install them correctly w/o conflict and successfully compile the helloword examples.

In general, if you install these package in default location, CMake will find them eventually. Otherwise, add your install directory to `CMAKE_PREFIX_PATH`. For example, I defined following variables in my environments and let CMake retrieve them with `$ENV{}`.

```BASH
# FPGA path
export ALTERAOCLSDKROOT="/opt/altera/aocl-pro-rte/aclrte-linux64"
export INTELFPGAOCLSDKROOT=$ALTERAOCLSDKROOT

# Intel OpenCL FPGA runtime
export AOCL_BOARD_PACKAGE_ROOT="$INTELFPGAOCLSDKROOT/board/a10_ref"
source "$INTELFPGAOCLSDKROOT/init_opencl.sh"

# Intel OpeVino
export INTEL_OPENVINO_DIR="/opt/intel/openvino_2019.1.144"
source "$INTEL_OPENVINO_DIR/bin/setupvars.sh"

# CUDA and TensorRT
export CUDA_INSTALL_DIR=/usr/local/cuda-10.2/
export TensorRT_ROOT=/home/canhld/softwares/TensorRT-7.1.3.4/
export TRT_LIB_DIR=$TensorRT_ROOT/targets/x86_64-linux-gnu/lib/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TRT_LIB_DIR:$CUDA_INSTALL_DIR/lib64/
```

The following package are required to build the project. If you don't want to use Conan, manually add cmake modules to the CMake files.

```
GCC>=5
CMake>=3.13
Conan
```

## Directory structure

```
.
├── client                  >> Client code
├── cmakes                  >> CMake modules
├── docs                    >> Document
├── server                  >> Server code
│   ├── _experimental       >> Junk code, experimental ideas but not yet success  
│   ├── config              >> Server configuration
│   ├── libs                >> Libraries that implement components of the server
│   └── apps                >> The serving application
```

## How to build the project

Make sure you have CMake and Conan and required framework (TensorRT, Openvino, gRPC)

```SH
git clone https://github.com/canhld94/something.git
cd something
mkdir bin
mkdir build && cd build
conan install ..
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cmake install
```

Every binary file, include conan package binaries will be installed in the `bin` folder.

## How to run the server

1. Understand the [server architecture]()

2. Configure your [server file](server/config/README.md)

3. Go to bin folder and run the server

```SH
cd bin
./serving -f ../server/config/config_ssd.json
```

This will start the server, and the endpoint for inference is `/inference`. Send any image to the endpoint and server will return detection result in JSON format.

3. On another terminal, go to run client folder and run client, the result will be written to file "testing.jpg"

```SH
# go to correct http or grpc folder
python simple_client.py
```
