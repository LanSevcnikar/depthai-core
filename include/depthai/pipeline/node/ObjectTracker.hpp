#pragma once

#include <depthai/pipeline/DeviceNode.hpp>

#include "depthai/pipeline/datatype/Tracklets.hpp"

// standard
#include <fstream>

// shared
#include <depthai/properties/ObjectTrackerProperties.hpp>

namespace dai {
namespace node {

/**
 * @brief ObjectTracker node. Performs object tracking using Kalman filter and hungarian algorithm.
 */
class ObjectTracker : public DeviceNodeCRTP<DeviceNode, ObjectTracker, ObjectTrackerProperties> {
   public:
    constexpr static const char* NAME = "ObjectTracker";
    using DeviceNodeCRTP::DeviceNodeCRTP;

    std::shared_ptr<ObjectTracker> build() {
        return std::static_pointer_cast<ObjectTracker>(shared_from_this());
    }
    /**
     * Input ImgFrame message on which tracking will be performed. RGBp, BGRp, NV12, YUV420p types are supported.
     * Default queue is non-blocking with size 4.
     */
    Input inputTrackerFrame{
        *this, {.name = "inputTrackerFrame", .blocking = false, .queueSize = 4, .types = {{DatatypeEnum::ImgFrame, false}}, .waitForMessage = true}};

    /**
     * Input ImgFrame message on which object detection was performed.
     * Default queue is non-blocking with size 4.
     */
    Input inputDetectionFrame{
        *this, {.name = "inputDetectionFrame", .blocking = false, .queueSize = 4, .types = {{DatatypeEnum::ImgFrame, false}}, .waitForMessage = true}};

    /**
     * Input message with image detection from neural network.
     * Default queue is non-blocking with size 4.
     */
    Input inputDetections{
        *this, {.name = "inputDetections", .blocking = false, .queueSize = 4, .types = {{DatatypeEnum::ImgDetections, true}}, .waitForMessage = true}};

    /**
     * Outputs Tracklets message that carries object tracking results.
     */
    Output out{*this, {.name = "out", .types = {{DatatypeEnum::Tracklets, false}}}};

    /**
     * Passthrough ImgFrame message on which tracking was performed.
     * Suitable for when input queue is set to non-blocking behavior.
     */
    Output passthroughTrackerFrame{*this, {.name = "passthroughTrackerFrame", .types = {{DatatypeEnum::ImgFrame, false}}}};

    /**
     * Passthrough ImgFrame message on which object detection was performed.
     * Suitable for when input queue is set to non-blocking behavior.
     */
    Output passthroughDetectionFrame{*this, {.name = "passthroughDetectionFrame", .types = {{DatatypeEnum::ImgFrame, false}}}};

    /**
     * Passthrough image detections message from neural network output.
     * Suitable for when input queue is set to non-blocking behavior.
     */
    Output passthroughDetections{*this, {.name = "passthroughDetections", .types = {{DatatypeEnum::ImgDetections, true}}}};
    /**
     * Specify tracker threshold.
     * @param threshold Above this threshold the detected objects will be tracked. Default 0, all image detections are tracked.
     */
    void setTrackerThreshold(float threshold);

    /**
     * Specify maximum number of object to track.
     * @param maxObjectsToTrack Maximum number of object to track. Maximum 60 in case of SHORT_TERM_KCF, otherwise 1000.
     */
    void setMaxObjectsToTrack(std::int32_t maxObjectsToTrack);

    /**
     * Specify detection labels to track.
     * @param labels Detection labels to track. Default every label is tracked from image detection network output.
     */
    void setDetectionLabelsToTrack(std::vector<std::uint32_t> labels);

    /**
     * Specify tracker type algorithm.
     * @param type Tracker type.
     */
    void setTrackerType(TrackerType type);

    /**
     * Specify tracker ID assignment policy.
     * @param type Tracker ID assignment policy.
     */
    void setTrackerIdAssignmentPolicy(TrackerIdAssignmentPolicy type);

    /**
     * Whether tracker should take into consideration class label for tracking.
     */
    void setTrackingPerClass(bool trackingPerClass);
};

}  // namespace node
}  // namespace dai
