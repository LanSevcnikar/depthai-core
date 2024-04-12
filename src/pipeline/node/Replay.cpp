#include "depthai/pipeline/node/Replay.hpp"

#include <chrono>
#include <memory>

#include "depthai/pipeline/datatype/EncodedFrame.hpp"
#include "depthai/pipeline/datatype/IMUData.hpp"
#include "depthai/utility/RecordReplay.hpp"
#include "depthai/utility/RecordReplaySchema.hpp"

namespace dai {
namespace node {

void Replay::build() {
    hostNode = true;
}

std::tuple<float, float, float> quaternionToEuler(float w, float x, float y, float z) {
    // roll (x-axis rotation)
    double sinr_cosp = 2 * (w * x + y * z);
    double cosr_cosp = 1 - 2 * (x * x + y * y);
    float roll = std::atan2(sinr_cosp, cosr_cosp);

    // pitch (y-axis rotation)
    double sinp = std::sqrt(1 + 2 * (w * y - x * z));
    double cosp = std::sqrt(1 - 2 * (w * y - x * z));
    float pitch = 2 * std::atan2(sinp, cosp) - M_PI / 2;

    // yaw (z-axis rotation)
    double siny_cosp = 2 * (w * z + x * y);
    double cosy_cosp = 1 - 2 * (y * y + z * z);
    float yaw = std::atan2(siny_cosp, cosy_cosp);

    return {roll, pitch, yaw};
}

std::shared_ptr<Buffer> getMessage(utility::RecordType type, const nlohmann::json& metadata, std::vector<uint8_t>& frame) {
    // TODO(asahtik): Handle versions
    switch(type) {
        case utility::RecordType::Other:
            throw std::runtime_error("Unsupported record type");
        case utility::RecordType::Video: {
            utility::VideoRecordSchema recordSchema = metadata;
            ImgFrame imgFrame;
            imgFrame.setWidth(recordSchema.width);
            imgFrame.setHeight(recordSchema.height);
            imgFrame.setTimestamp(std::chrono::time_point<std::chrono::steady_clock>(recordSchema.timestamp.get()));
            imgFrame.setSequenceNum(recordSchema.sequenceNumber);
            imgFrame.setInstanceNum(recordSchema.instanceNumber);
            imgFrame.cam.wbColorTemp = recordSchema.cameraSettings.wbColorTemp;
            imgFrame.cam.lensPosition = recordSchema.cameraSettings.lensPosition;
            imgFrame.cam.lensPositionRaw = recordSchema.cameraSettings.lensPositionRaw;
            imgFrame.cam.exposureTimeUs = recordSchema.cameraSettings.exposure;
            imgFrame.cam.sensitivityIso = recordSchema.cameraSettings.sensitivity;

            assert(frame.size() == recordSchema.width * recordSchema.height * 3);
            cv::Mat img(recordSchema.height, recordSchema.width, CV_8UC3, frame.data());
            imgFrame.setCvFrame(img, ImgFrame::Type::YUV420p);
            return std::dynamic_pointer_cast<Buffer>(std::make_shared<ImgFrame>(imgFrame));
        }
        case utility::RecordType::Imu: {
            utility::ImuRecordSchema recordSchema = metadata;
            IMUData imuData;
            imuData.packets.resize(recordSchema.packets.size());
            for(const auto& packet : recordSchema.packets) {
                IMUPacket imuPacket;
                imuPacket.acceleroMeter.timestamp.sec = packet.acceleration.timestamp.seconds;
                imuPacket.acceleroMeter.timestamp.nsec = packet.acceleration.timestamp.nanoseconds;
                imuPacket.acceleroMeter.sequence = packet.acceleration.sequenceNumber;
                imuPacket.acceleroMeter.x = packet.acceleration.x;
                imuPacket.acceleroMeter.y = packet.acceleration.y;
                imuPacket.acceleroMeter.z = packet.acceleration.z;
                const auto& [roll, pitch, yaw] = quaternionToEuler(packet.orientation.w, packet.orientation.x, packet.orientation.y, packet.orientation.z);
                imuPacket.gyroscope.timestamp.sec = packet.orientation.timestamp.seconds;
                imuPacket.gyroscope.timestamp.nsec = packet.orientation.timestamp.nanoseconds;
                imuPacket.gyroscope.sequence = packet.orientation.sequenceNumber;
                imuPacket.gyroscope.x = roll;
                imuPacket.gyroscope.y = pitch;
                imuPacket.gyroscope.z = yaw;

                imuData.packets.push_back(imuPacket);
            }
            return std::dynamic_pointer_cast<Buffer>(std::make_shared<IMUData>(imuData));
        }
    }
    return {};
}

void Replay::run() {
    if(replayFile.empty()) {
        throw std::runtime_error("Replay replayFile must be set");
    }

    utility::VideoPlayer videoPlayer;
    utility::BytePlayer bytePlayer;
    bool hasVideo = true;
    bool hasMetadata = true;
    try {
        videoPlayer.init(replayVideo);
    } catch(const std::exception& e) {
        hasVideo = false;
        if(logger) logger->warn("Video not replaying: {}", e.what());
    }
    try {
        bytePlayer.init(replayFile);
    } catch(const std::exception& e) {
        hasMetadata = false;
        if(logger) logger->warn("Metadata not replaying: {}", e.what());
    }
    utility::RecordType type = utility::RecordType::Other;
    if(hasVideo && !hasMetadata) {
        type = utility::RecordType::Video;
    }
    bool first = true;
    auto start = std::chrono::steady_clock::now();
    uint64_t index = 0;
    while(isRunning()) {
        nlohmann::json metadata;
        std::vector<uint8_t> frame;
        if(hasMetadata) {
            auto msg = bytePlayer.next();
            if(msg.has_value()) {
                metadata = msg.value();
                if(first) {
                    type = metadata["type"].get<utility::RecordType>();
                }
            } else if(!first) {
                // End of file
                break;
            } else {
                hasMetadata = false;
            }
        }
        if(hasVideo && type == utility::RecordType::Video) {
            auto msg = videoPlayer.next();
            if(msg.has_value()) {
                frame = msg.value();
            } else if(!first) {
                // End of file
                break;
            } else {
                hasVideo = false;
            }
        }

        if(!hasMetadata && !hasVideo) {
            break;
        }

        if(!hasVideo && type == utility::RecordType::Video) {
            throw std::runtime_error("Video file not found");
        }

        if(!hasMetadata && type == utility::RecordType::Video) {
            utility::VideoRecordSchema recordSchema;
            auto time = start - std::chrono::steady_clock::now();
            const auto& [width, height] = videoPlayer.size();
            recordSchema.type = utility::RecordType::Video;
            recordSchema.timestamp.set(time);
            recordSchema.sequenceNumber = index++;
            recordSchema.width = width;
            recordSchema.height = height;
            metadata = recordSchema;
        }

        auto buffer = getMessage(type, metadata, frame);

        if(buffer) out.send(buffer);

        first = false;
    }
}

Replay& Replay::setReplayFile(const std::string& replayFile) {
    this->replayFile = replayFile;
    return *this;
}

Replay& Replay::setReplayVideo(const std::string& replayVideo) {
    this->replayVideo = replayVideo;
    return *this;
}

}  // namespace node
}  // namespace dai