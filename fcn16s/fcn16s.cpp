#include <gflags/gflags.h>
#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include <time.h>
#include <chrono>
#include <limits>
#include <iomanip>
#include <fstream>

#include <inference_engine.hpp>
#include <ext_list.hpp>
#include <opencv2/opencv.hpp>


#include "fcn16s.h"

using namespace InferenceEngine;

template <typename T>
void matU8ToBlob(const cv::Mat& orig_image, InferenceEngine::Blob::Ptr& blob, int batchIndex = 0) {
    InferenceEngine::SizeVector blobSize = blob->getTensorDesc().getDims();
    const size_t width = blobSize[3];
    const size_t height = blobSize[2];
    const size_t channels = blobSize[1];
    T* blob_data = blob->buffer().as<T*>();

    cv::Mat resized_image(orig_image);
    if (static_cast<int>(width) != orig_image.size().width ||
            static_cast<int>(height) != orig_image.size().height) {
        cv::resize(orig_image, resized_image, cv::Size(width, height));
    }

    int batchOffset = batchIndex * width * height * channels;
    std::vector<float> means = {77.49746752, 81.65870928, 80.70264492};
    for (size_t c = 0; c < channels; c++) {
        for (size_t  h = 0; h < height; h++) {
            for (size_t w = 0; w < width; w++) {
                blob_data[batchOffset + c * width * height + h * width + w] =
                        resized_image.at<cv::Vec3b>(h, w)[c] - means[c];
            }
        }
    }
}

void frameToBlob(const cv::Mat& frame,
        InferRequest::Ptr& inferRequest,
        const std::string& inputName) {   
    Blob::Ptr frameBlob = inferRequest->GetBlob(inputName);
    matU8ToBlob<float>(frame, frameBlob);
}

bool ParseAndCheckCommandLine(int argc, char *argv[]) {
    // ---------------------------Parsing and validation of input args--------------------------------------
    std::cout << "Parsing input parameters" << std::endl;

    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    if (FLAGS_h) {
        showUsage();
        return false;
    }

    if (FLAGS_ni < 1) {
        throw std::logic_error("Parameter -ni should be greater than 0 (default: 1)");
    }

    if (FLAGS_i.empty()) {
        throw std::logic_error("Parameter -i is not set");
    }

    if (FLAGS_m.empty()) {
        throw std::logic_error("Parameter -m is not set");
    }

    return true;
}

int main(int argc, char *argv[]) {
    try {
        std::cout << "InferenceEngine: " << GetInferenceEngineVersion() << std::endl;

        // ------------------------------ Parsing and validation of input args ---------------------------------
        if (!ParseAndCheckCommandLine(argc, argv)) {
            return 0;
        }

        std::cout << "Reading input" << std::endl;
        cv::Mat frame = cv::imread(FLAGS_i);
        // --------------------------- 1. Load Plugin for inference engine -------------------------------------
        std::cout << "Loading plugin" << std::endl;
        InferencePlugin plugin = PluginDispatcher().getPluginByDevice(FLAGS_d);
        /** Load extensions for the plugin **/

        /** Loading default extensions **/
        if (FLAGS_d.find("CPU") != std::string::npos) {
            /**
             * cpu_extensions library is compiled from "extension" folder containing
             * custom MKLDNNPlugin layer implementations. These layers are not supported
             * by mkldnn, but they can be useful for inferring custom topologies.
            **/
            plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());
        }

        if (!FLAGS_l.empty()) {
            // CPU(MKLDNN) extensions are loaded as a shared library and passed as a pointer to base extension
            IExtensionPtr extension_ptr = make_so_pointer<IExtension>(FLAGS_l.c_str());
            plugin.AddExtension(extension_ptr);
        }
        if (!FLAGS_c.empty()) {
            // clDNN Extensions are loaded from an .xml description and OpenCL kernel files
            plugin.SetConfig({{PluginConfigParams::KEY_CONFIG_FILE, FLAGS_c}});
        }

        // --------------------------- 2. Read IR Generated by ModelOptimizer (.xml and .bin files) ------------
        std::cout << "Loading network files" << std::endl;
        CNNNetReader netReader;
        /** Read network model **/
        netReader.ReadNetwork(FLAGS_m);
        /** Set batch size to 1 **/
        std::cout << "Batch size is forced to  1." << std::endl;
        netReader.getNetwork().setBatchSize(1);
        /** Extract model name and load it's weights **/
        for (int i = 0; i < 3; ++i) {
            FLAGS_m.pop_back();
        }
        std::string binFileName = FLAGS_m + "bin";
        netReader.ReadWeights(binFileName);

        // --------------------------- 3. Configure input & output ---------------------------------------------
        // --------------------------- Prepare input blobs -----------------------------------------------------
        std::cout << "Checking that the inputs are as expects" << std::endl;
        InputsDataMap inputInfo(netReader.getNetwork().getInputsInfo());
        if (inputInfo.size() != 1) {
            throw std::logic_error("FCN16S has only 1 inputs");
        }
        InputInfo::Ptr& input = inputInfo.begin()->second;
        const SizeVector inputDims = input->getTensorDesc().getDims();
        
        input->setPrecision(Precision::FP32);
        input->getInputData()->setLayout(Layout::NCHW);
        // --------------------------- Prepare output blobs -----------------------------------------------------
        std::cout << "Checking that the outputs are as expects" << std::endl;
        OutputsDataMap outputInfo(netReader.getNetwork().getOutputsInfo());
        if (outputInfo.size() != 1) {
            throw std::logic_error("FCN16S has only 1 output");
        }
        DataPtr& output = outputInfo.begin()->second;

        const SizeVector outputDims = output->getTensorDesc().getDims();
        if (outputDims.size() != 4) {
            throw std::logic_error("Incorrect output dimensions for FCN16S");
        }
        output->setPrecision(Precision::FP32);
        output->setLayout(Layout::NCHW);

        // --------------------------- 4. Loading model to the plugin ------------------------------------------
        std::cout << "Loading model to the plugin" << std::endl;
        ExecutableNetwork network = plugin.LoadNetwork(netReader.getNetwork(), {});

        // --------------------------- 5. Create infer request -------------------------------------------------
        InferRequest::Ptr infer_request = network.CreateInferRequestPtr();

        // --------------------------- 7. Do inference ---------------------------------------------------------
        std::cout << "Start inference (" << FLAGS_ni << " iterations)" << std::endl;

        typedef std::chrono::high_resolution_clock Time;
        typedef std::chrono::duration<double, std::ratio<1, 1000>> ms;
        typedef std::chrono::duration<float> fsec;
        frameToBlob(frame, infer_request, inputInfo.begin()->first);

        double total = 0.0;
        /** Start inference & calc performance **/
        for (size_t iter = 0; iter < FLAGS_ni; ++iter) {
            auto t0 = Time::now();
            infer_request->Infer();
            auto t1 = Time::now();
            fsec fs = t1 - t0;
            ms d = std::chrono::duration_cast<ms>(fs);
            total += d.count();
        }

        std::cout << std::endl << "Average running time of one iteration: " << total / static_cast<double>(FLAGS_ni) << " ms" << std::endl << std::endl;

        // --------------------------- 8. Process output -------------------------------------------------------
        std::cout << "Processing output blobs" << std::endl;
        auto outputName = outputInfo.begin()->first;
        const Blob::Ptr output_blob = infer_request->GetBlob(outputName);
        const auto output_data = output_blob->buffer().as<float*>();

        int N = output_blob->getTensorDesc().getDims().at(0);    // 1
        int C = output_blob->getTensorDesc().getDims().at(1);    // 3 
        int H = output_blob->getTensorDesc().getDims().at(2);    // 1124
        int W = output_blob->getTensorDesc().getDims().at(3);    // 1124
        std::cout << N << " " << C << " " << H << " " << W << std::endl;

        char delim = ' ';
        std::ofstream fo;
        fo.open("mask.csv");
        if (!fo) {
            std::cerr << "Unable to open output file";
        }
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < C; ++j) {
                for (int k = 0; k < H; ++k) {
                    for (int l = 0; l < W; ++l) {
                        fo <<  output_data[i*C*W*H + j*H*W + k*H + l] << delim;
                    }
                }
            }
        }
        cv::Mat mask(cv::Size(W,H),CV_32FC3,output_data);
        mask.convertTo(mask,CV_8SC3);
        cv::imwrite("test.jpg",mask);
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown/internal exception happened." << std::endl;
        return 1;
    }

    std::cout << "Execution successful" << std::endl;
    return 0;
}