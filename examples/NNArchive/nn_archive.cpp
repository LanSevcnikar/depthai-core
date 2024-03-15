#include <chrono>
#include <iostream>

// Includes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"

/*
The code is the same as for Tiny-yolo-V3, the only difference is the blob file.
The blob was compiled following this tutorial: https://github.com/TNTWEN/OpenVINO-YOLOV4
*/

static const std::vector<std::string> labelMap = {
    "person",        "bicycle",      "car",           "motorbike",     "aeroplane",   "bus",         "train",       "truck",        "boat",
    "traffic light", "fire hydrant", "stop sign",     "parking meter", "bench",       "bird",        "cat",         "dog",          "horse",
    "sheep",         "cow",          "elephant",      "bear",          "zebra",       "giraffe",     "backpack",    "umbrella",     "handbag",
    "tie",           "suitcase",     "frisbee",       "skis",          "snowboard",   "sports ball", "kite",        "baseball bat", "baseball glove",
    "skateboard",    "surfboard",    "tennis racket", "bottle",        "wine glass",  "cup",         "fork",        "knife",        "spoon",
    "bowl",          "banana",       "apple",         "sandwich",      "orange",      "broccoli",    "carrot",      "hot dog",      "pizza",
    "donut",         "cake",         "chair",         "sofa",          "pottedplant", "bed",         "diningtable", "toilet",       "tvmonitor",
    "laptop",        "mouse",        "remote",        "keyboard",      "cell phone",  "microwave",   "oven",        "toaster",      "sink",
    "refrigerator",  "book",         "clock",         "vase",          "scissors",    "teddy bear",  "hair drier",  "toothbrush"};

static const std::atomic<bool> syncNN{true};

void initializeNNArchive(const std::string& exampleType,    // NOLINT(bugprone-easily-swappable-parameters)
                         const std::string& nnArchivePath,  // NOLINT(bugprone-easily-swappable-parameters)
                         const std::shared_ptr<dai::node::ColorCamera>& camRgb,
                         const std::shared_ptr<dai::node::DetectionNetwork>& detectionNetwork) {
    if(exampleType != "advanced") {
        camRgb->setPreviewSize(640, 640);
    }

    if(exampleType == "fs") {
        detectionNetwork->setNNArchive(dai::NNArchive(nnArchivePath));
    } else if(exampleType == "memory") {
        std::ifstream archiveFile(nnArchivePath, std::ios::binary);
        std::vector<uint8_t> fileContents((std::istreambuf_iterator<char>(archiveFile)), std::istreambuf_iterator<char>());
        detectionNetwork->setNNArchive(dai::NNArchive(fileContents));
    } else if(exampleType == "advanced") {
        const dai::NNArchiveConfig config(nnArchivePath);
        const auto& configV1 = config.getConfigV1();
        if(!configV1) {
            throw std::runtime_error("Wrong config version");
        }
        const auto width = (*configV1).model.inputs[0].shape[2];
        const auto height = (*configV1).model.inputs[0].shape[3];
        if(width > 1920 || height > 1080) {
            // We could decide to load another NNArchive that has a smaller size instead of throwing ...
            // All without loading / reading to memory the whole blob
            throw std::runtime_error("Sorry that's to big");
        }
        camRgb->setPreviewSize(static_cast<int>(width), static_cast<int>(height));
        detectionNetwork->setNNArchive(dai::NNArchive(config, dai::NNArchiveBlob(config, nnArchivePath)));
    } else {
        throw std::runtime_error("Not implemented yet");
    }
}

int main(int argc, char** argv) {  // NOLINT
    using namespace std;           // NOLINT
    using namespace std::chrono;   // NOLINT

    auto args = dai::span(argv, static_cast<size_t>(argc));
    if(args.size() < 3) {
        std::cout << "WRONG USAGE!!!\n\n"
                  << "USAGE: ./nn_archive ${EXAMPLE_TYPE} ${PATH_TO_ARCHIVE_OR_RAW_CONFIG_JSON}\n\n"
                  << "Where EXAMPLE_TYPE is:\n"
                  << "1) fs, read directly from filesystem\n"
                  << "2) memory, read the whole NNArchive to memory first\n"
                  << "3) buffer, feed the library with the archive chunk by chunk\n"
                  << "4) advanced, decompress the config from the archive first, get some info and then optionally decompress the blob\n";
        return 1;
    }
    const std::string exampleType(args[1]);
    const std::string nnArchivePath(args[2]);
    std::cout << "Using archive at path:" << nnArchivePath << "\n";
    std::cout << "Running example type:" << nnArchivePath << "\n";

    // Create pipeline
    dai::Pipeline pipeline;

    // Define sources and outputs
    auto camRgb = pipeline.create<dai::node::ColorCamera>();
    auto detectionNetwork = pipeline.create<dai::node::DetectionNetwork>();
    auto xoutRgb = pipeline.create<dai::node::XLinkOut>();
    auto nnOut = pipeline.create<dai::node::XLinkOut>();

    xoutRgb->setStreamName("rgb");
    nnOut->setStreamName("detections");

    camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    camRgb->setInterleaved(false);                                       // NOLINT
    camRgb->setColorOrder(dai::ColorCameraProperties::ColorOrder::BGR);  // NOLINT
    camRgb->setFps(40);

    initializeNNArchive(exampleType, nnArchivePath, camRgb, detectionNetwork);
    // TODO(jakgra)
    // Will we get this from NN Archive also?
    detectionNetwork->setNumInferenceThreads(2);
    detectionNetwork->input.setBlocking(false);

    // Linking
    camRgb->preview.link(detectionNetwork->input);
    if(syncNN) {
        detectionNetwork->passthrough.link(xoutRgb->input);
    } else {
        camRgb->preview.link(xoutRgb->input);
    }

    detectionNetwork->out.link(nnOut->input);

    // Connect to device and start pipeline
    dai::Device device(pipeline);

    // Output queues will be used to get the rgb frames and nn data from the outputs defined above
    auto qRgb = device.getOutputQueue("rgb", 4, false);
    auto qDet = device.getOutputQueue("detections", 4, false);

    cv::Mat frame;
    std::vector<dai::ImgDetection> detections;
    auto startTime = steady_clock::now();
    int counter = 0;
    float fps = 0;
    auto color2 = cv::Scalar(255, 255, 255);

    // Add bounding boxes and text to the frame and show it to the user
    auto displayFrame = [](const std::string& name, cv::Mat frame, std::vector<dai::ImgDetection>& detections) {
        auto color = cv::Scalar(255, 0, 0);
        // nn data, being the bounding box locations, are in <0..1> range - they need to be normalized with frame width/height
        for(auto& detection : detections) {
            int x1 = detection.xmin * frame.cols;  // NOLINT
            int y1 = detection.ymin * frame.rows;  // NOLINT
            int x2 = detection.xmax * frame.cols;  // NOLINT
            int y2 = detection.ymax * frame.rows;  // NOLINT

            uint32_t labelIndex = detection.label;
            std::string labelStr = to_string(labelIndex);
            if(labelIndex < labelMap.size()) {
                labelStr = labelMap[labelIndex];
            }
            cv::putText(frame, labelStr, cv::Point(x1 + 10, y1 + 20), cv::FONT_HERSHEY_TRIPLEX, 0.5, 255);
            std::stringstream confStr;
            confStr << std::fixed << std::setprecision(2) << detection.confidence * 100;
            cv::putText(frame, confStr.str(), cv::Point(x1 + 10, y1 + 40), cv::FONT_HERSHEY_TRIPLEX, 0.5, 255);
            cv::rectangle(frame, cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)), color, cv::FONT_HERSHEY_SIMPLEX);
        }
        // Show the frame
        cv::imshow(name, frame);
    };

    while(true) {
        std::shared_ptr<dai::ImgFrame> inRgb;
        std::shared_ptr<dai::ImgDetections> inDet;

        if(syncNN) {
            inRgb = qRgb->get<dai::ImgFrame>();
            inDet = qDet->get<dai::ImgDetections>();
        } else {
            inRgb = qRgb->tryGet<dai::ImgFrame>();
            inDet = qDet->tryGet<dai::ImgDetections>();
        }

        counter++;
        auto currentTime = steady_clock::now();
        auto elapsed = duration_cast<duration<float>>(currentTime - startTime);
        if(elapsed > seconds(1)) {
            fps = static_cast<float>(counter) / elapsed.count();
            counter = 0;
            startTime = currentTime;
        }

        if(inRgb) {
            frame = inRgb->getCvFrame();
            std::stringstream fpsStr;
            fpsStr << "NN fps: " << std::fixed << std::setprecision(2) << fps;
            cv::putText(frame, fpsStr.str(), cv::Point(2, static_cast<int>(inRgb->getHeight()) - 4), cv::FONT_HERSHEY_TRIPLEX, 0.4, color2);
        }

        if(inDet) {
            detections = inDet->detections;
        }

        if(!frame.empty()) {
            displayFrame("rgb", frame, detections);
        }

        int key = cv::waitKey(1);
        if(key == 'q' || key == 'Q') {
            return 0;
        }
    }
    return 0;
}
