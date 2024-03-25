#include "depthai/pipeline/node/DetectionNetwork.hpp"

// libraries
#include <nlohmann/json.hpp>
#include <openvino/BlobReader.hpp>

#include "archive.h"
#include "archive_entry.h"

// internal
#include "depthai/common/DetectionNetworkType.hpp"
#include "depthai/nn_archive/NNArchive.hpp"
#include "depthai/nn_archive/v1/Config.hpp"
#include "nn_archive/v1/Generators.hpp"
#include "utility/ArchiveUtil.hpp"
#include "utility/ErrorMacros.hpp"
#include "utility/PimplImpl.hpp"

namespace dai {
namespace node {

//--------------------------------------------------------------------
// Base Detection Network Class
//--------------------------------------------------------------------

class DetectionNetwork::Impl {
   public:
    Impl() = default;

    /*
     * Place for future private stuff.
     */
};

DetectionNetwork::DetectionNetwork()
    : out{detectionParser->out}, outNetwork{neuralNetwork->out}, input{neuralNetwork->input}, passthrough{neuralNetwork->passthrough} {};
DetectionNetwork::~DetectionNetwork() = default;

// -------------------------------------------------------------------
// Neural Network API
// -------------------------------------------------------------------

void DetectionNetwork::build() {
    // Default confidence threshold
    detectionParser->properties.parser.confidenceThreshold = 0.5;
    neuralNetwork->out.link(detectionParser->input);
    neuralNetwork->passthrough.link(detectionParser->imageIn);

    // No "internal" buffering to keep interface similar to monolithic nodes
    detectionParser->input.setBlocking(true);
    detectionParser->input.setQueueSize(1);
    detectionParser->imageIn.setBlocking(false);
    detectionParser->imageIn.setQueueSize(1);
}

void DetectionNetwork::setNNArchive(const NNArchive& nnArchive) {
    const auto configMaybe = nnArchive.getConfig().getConfigV1();
    DAI_CHECK(configMaybe, "Unsupported NNArchive format / version. Check which depthai version you are running.");
    const auto& config = *configMaybe;
    const auto& blob = nnArchive.getBlob().getOpenVINOBlob();
    DAI_CHECK_IN(blob);
    setBlob(*blob);
    const auto model = config.model;
    // TODO(jakgra) is NN Archive valid without this? why is this optional?
    DAI_CHECK(model.heads, "Heads array is not defined in the NN Archive config file.");
    // TODO(jakgra) for now get info from heads[0] but in the future correctly support multiple outputs and mapped heads
    DAI_CHECK_V(
        (*model.heads).size() == 1, "There should be exactly one head per model in the NN Archive config file defined. Found {} heads.", (*model.heads).size());
    const auto head = (*model.heads)[0];
    if(head.family == "ObjectDetectionYOLO") {
        detectionParser->properties.parser.nnFamily = DetectionNetworkType::YOLO;
    }
    detectionParser->setNumClasses(static_cast<int>(head.nClasses));
    if(head.iouThreshold) {
        detectionParser->properties.parser.iouThreshold = static_cast<float>(*head.iouThreshold);
    }
    if(head.confThreshold) {
        setConfidenceThreshold(static_cast<float>(*head.confThreshold));
    }
    detectionParser->setCoordinateSize(4);
    if(head.anchors) {
        const auto anchorsIn = *head.anchors;
        std::vector<std::vector<std::vector<float>>> anchorsOut(anchorsIn.size());
        for(size_t layer = 0; layer < anchorsOut.size(); ++layer) {
            std::vector<std::vector<float>> layerOut(anchorsIn[layer].size());
            for(size_t anchor = 0; anchor < layerOut.size(); ++anchor) {
                std::vector<float> anchorOut(anchorsIn[layer][anchor].size());
                for(size_t dim = 0; dim < anchorOut.size(); ++dim) {
                    anchorOut[dim] = static_cast<float>(anchorsIn[layer][anchor][dim]);
                }
                layerOut[anchor] = anchorOut;
            }
            anchorsOut[layer] = layerOut;
        }
        detectionParser->setAnchors(anchorsOut);
    }
}

void DetectionNetwork::setBlobPath(const dai::Path& path) {
    neuralNetwork->setBlobPath(path);
    detectionParser->setBlobPath(path);
}

void DetectionNetwork::setBlob(OpenVINO::Blob blob) {
    neuralNetwork->setBlob(blob);
    detectionParser->setBlob(blob);
}

void DetectionNetwork::setBlob(const dai::Path& path) {
    neuralNetwork->setBlob(path);
    detectionParser->setBlob(path);
}

void DetectionNetwork::setXmlModelPath(const dai::Path& xmlModelPath, const dai::Path& binModelPath) {
    neuralNetwork->setXmlModelPath(xmlModelPath, binModelPath);
}

void DetectionNetwork::setNumPoolFrames(int numFrames) {
    neuralNetwork->setNumPoolFrames(numFrames);
}

void DetectionNetwork::setNumInferenceThreads(int numThreads) {
    neuralNetwork->setNumInferenceThreads(numThreads);
}

void DetectionNetwork::setNumNCEPerInferenceThread(int numNCEPerThread) {
    neuralNetwork->setNumNCEPerInferenceThread(numNCEPerThread);
}

void DetectionNetwork::setNumShavesPerInferenceThread(int numShavesPerThread) {
    neuralNetwork->setNumShavesPerInferenceThread(numShavesPerThread);
}

void DetectionNetwork::setBackend(std::string backend) {
    neuralNetwork->setBackend(backend);
}

void DetectionNetwork::setBackendProperties(std::map<std::string, std::string> props) {
    neuralNetwork->setBackendProperties(props);
}

int DetectionNetwork::getNumInferenceThreads() {
    return neuralNetwork->getNumInferenceThreads();
}

void DetectionNetwork::setConfidenceThreshold(float thresh) {
    detectionParser->setConfidenceThreshold(thresh);
}

float DetectionNetwork::getConfidenceThreshold() const {
    return detectionParser->getConfidenceThreshold();
}

//--------------------------------------------------------------------
// MobileNet
//--------------------------------------------------------------------
void MobileNetDetectionNetwork::build() {
    DetectionNetwork::build();
    detectionParser->properties.parser.nnFamily = DetectionNetworkType::MOBILENET;
}

//--------------------------------------------------------------------
// YOLO
//--------------------------------------------------------------------
void YoloDetectionNetwork::build() {
    DetectionNetwork::build();
    detectionParser->properties.parser.nnFamily = DetectionNetworkType::YOLO;
    detectionParser->properties.parser.iouThreshold = 0.5f;
}

void YoloDetectionNetwork::setNumClasses(const int numClasses) {
    detectionParser->setNumClasses(numClasses);
}

void YoloDetectionNetwork::setCoordinateSize(const int coordinates) {
    detectionParser->setCoordinateSize(coordinates);
}

void YoloDetectionNetwork::setAnchors(std::vector<float> anchors) {
    detectionParser->setAnchors(anchors);
}

void YoloDetectionNetwork::setAnchorMasks(std::map<std::string, std::vector<int>> anchorMasks) {
    detectionParser->setAnchorMasks(anchorMasks);
}

void YoloDetectionNetwork::setAnchors(const std::vector<std::vector<std::vector<float>>>& anchors) {
    detectionParser->setAnchors(anchors);
}

void YoloDetectionNetwork::setIouThreshold(float thresh) {
    detectionParser->setIouThreshold(thresh);
}

/// Get num classes
int YoloDetectionNetwork::getNumClasses() const {
    return detectionParser->getNumClasses();
}

/// Get coordianate size
int YoloDetectionNetwork::getCoordinateSize() const {
    return detectionParser->getCoordinateSize();
}

/// Get anchors
std::vector<float> YoloDetectionNetwork::getAnchors() const {
    return detectionParser->getAnchors();
}

/// Get anchor masks
std::map<std::string, std::vector<int>> YoloDetectionNetwork::getAnchorMasks() const {
    return detectionParser->getAnchorMasks();
}

/// Get Iou threshold
float YoloDetectionNetwork::getIouThreshold() const {
    return detectionParser->getIouThreshold();
}

}  // namespace node
}  // namespace dai
