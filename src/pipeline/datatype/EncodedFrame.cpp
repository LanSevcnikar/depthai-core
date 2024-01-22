#include "depthai/pipeline/datatype/EncodedFrame.hpp"

#include "utility/H26xParsers.hpp"

namespace dai {
// getters
unsigned int EncodedFrame::getInstanceNum() const {
    return instanceNum;
}
std::chrono::microseconds EncodedFrame::getExposureTime() const {
    return std::chrono::microseconds(cam.exposureTimeUs);
}
int EncodedFrame::getSensitivity() const {
    return cam.sensitivityIso;
}
int EncodedFrame::getColorTemperature() const {
    return cam.wbColorTemp;
}
int EncodedFrame::getLensPosition() const {
    return cam.lensPosition;
}
unsigned int EncodedFrame::getQuality() const {
    return quality;
}
unsigned int EncodedFrame::getBitrate() const {
    return bitrate;
}
bool EncodedFrame::getLossless() const {
    return lossless;
}
EncodedFrame::FrameType EncodedFrame::getFrameType() {
    if(type == FrameType::Unknown) {
        utility::SliceType frameType = utility::SliceType::Unknown;
        switch(profile) {
            case EncodedFrame::Profile::JPEG:
                frameType = utility::SliceType::I;
                break;
            case EncodedFrame::Profile::AVC:
                // TODO(Morato) - change this to zero copy
                frameType = utility::getTypesH264(std::vector<uint8_t>(data->getData().begin(), data->getData().end()), true)[0];
                break;
            case EncodedFrame::Profile::HEVC:
                frameType = utility::getTypesH265(std::vector<uint8_t>(data->getData().begin(), data->getData().end()), true)[0];
                break;
        }
        switch(frameType) {
            case utility::SliceType::P:
                type = FrameType::P;
                break;
            case utility::SliceType::B:
                type = FrameType::B;
                break;
            case utility::SliceType::I:
                type = FrameType::I;
                break;
            case utility::SliceType::SP:
                type = FrameType::P;
                break;
            case utility::SliceType::SI:
                type = FrameType::I;
                break;
            case utility::SliceType::Unknown:
                type = FrameType::Unknown;
                break;
        }
    }
    return type;
}
EncodedFrame::Profile EncodedFrame::getProfile() const {
    return profile;
}

EncodedFrame& EncodedFrame::setInstanceNum(unsigned int instanceNum) {
    this->instanceNum = instanceNum;
    return *this;
}
EncodedFrame& EncodedFrame::setQuality(unsigned int quality) {
    this->quality = quality;
    return *this;
}
EncodedFrame& EncodedFrame::setBitrate(unsigned int bitrate) {
    this->bitrate = bitrate;
    return *this;
}

EncodedFrame& EncodedFrame::setLossless(bool lossless) {
    this->lossless = lossless;
    return *this;
}
EncodedFrame& EncodedFrame::setFrameType(FrameType frameType) {
    this->type = frameType;
    return *this;
}
EncodedFrame& EncodedFrame::setProfile(Profile profile) {
    this->profile = profile;
    return *this;
}

}  // namespace dai