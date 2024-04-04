#include "depthai/utility/RecordReplay.hpp"

#include <spdlog/spdlog.h>
#include <track.h>

#include <fstream>
#include <stdexcept>

#include "utility/Platform.hpp"

namespace dai {
namespace utility {

constexpr unsigned int MP4V2_TIMESCALE = 90000;

enum class NALU { P = 1, I = 5, SPS = 7, PPS = 8, INVALID = 0x00 };

struct H26xNals {
    dai::span<const uint8_t> data;
    size_t index = 0;

    H26xNals(dai::span<const uint8_t> data) : data(data) {}

    dai::span<const uint8_t> next() {
        if(index >= data.size()) return {};
        while(index < data.size() - 4) {
            if(data[index] == 0 && data[index + 1] == 0 && data[index + 2] == 0 && data[index + 3] == 1) {
                auto nal = data.subspan(index, data.size() - index);
                index += 4;
                return nal;
            }
            ++index;
        }
        return {};
    }
};

VideoRecorder::~VideoRecorder() {
    close();
}

void VideoRecorder::init(const std::string& filePath, unsigned int width, unsigned int height, unsigned int fps, VideoCodec codec) {
    if(initialized) {
        throw std::runtime_error("VideoRecorder already initialized");
    }
    if(filePath.empty()) {
        throw std::runtime_error("VideoRecorder file path is empty");
    }
    if(width <= 0 || height <= 0) {
        throw std::runtime_error("VideoRecorder width or height is invalid");
    }
    if(fps <= 0) {
        throw std::runtime_error("VideoRecorder fps is invalid");
    }
    this->codec = codec;
    this->fps = fps;
    this->width = width;
    this->height = height;
    switch(codec) {
        case VideoCodec::H264:
        case VideoCodec::MJPEG:
            mp4Writer = MP4Create(filePath.c_str(), 0);
            if(mp4Writer == MP4_INVALID_FILE_HANDLE) {
                throw std::runtime_error("Failed to create MP4 file");
            }
            MP4SetTimeScale(mp4Writer, MP4V2_TIMESCALE);
            break;
        case VideoCodec::RAW:
#ifdef DEPTHAI_HAVE_OPENCV_SUPPORT
            cvWriter = std::make_unique<cv::VideoWriter>(filePath, cv::VideoWriter::fourcc('H', '2', '6', '4'), fps, cv::Size(width, height));
#else
            throw std::runtime_error("OpenCV support is required to write RAW video");
#endif
            break;
    }
}

void VideoRecorder::write(span<const uint8_t>& data) {
    if(!initialized) {
        throw std::runtime_error("VideoRecorder not initialized");
    }
    switch(this->codec) {
        case VideoCodec::H264: {
            H26xNals nals(data);
            auto nal = nals.next();
            while(!nal.empty()) {
                NALU type = (NALU)(nal[4] & 0x1F);
                switch(type) {
                    case NALU::P:
                    case NALU::I: {
                        if(mp4Track == MP4_INVALID_TRACK_ID) {
                            spdlog::warn("VideoRecorder track is invalid");
                            break;
                        };
                        std::vector<uint8_t> nalData(nal.data(), nal.data() + nal.size());
                        nalData[0] = (nal.size() - 4) >> 24;
                        nalData[1] = (nal.size() - 4) >> 16;
                        nalData[2] = (nal.size() - 4) >> 8;
                        nalData[3] = (nal.size() - 4) & 0xFF;
                        if(!MP4WriteSample(mp4Writer, mp4Track, nalData.data(), nalData.size())) {
                            spdlog::warn("Failed to write sample to MP4 file");
                        }
                        break;
                    }
                    case NALU::SPS:
                        if(mp4Track == MP4_INVALID_TRACK_ID) {
                            mp4Track = MP4AddH264VideoTrack(mp4Writer, MP4V2_TIMESCALE, MP4V2_TIMESCALE / fps, width, height, nal[5], nal[6], nal[7], 3);
                            assert(mp4Track != MP4_INVALID_TRACK_ID);
                            MP4SetVideoProfileLevel(mp4Writer, 0x7F);
                            MP4AddH264SequenceParameterSet(mp4Writer, mp4Track, nal.data(), nal.size());
                        }
                        break;
                    case NALU::PPS:
                        MP4AddH264PictureParameterSet(mp4Writer, mp4Track, nal.data(), nal.size());
                        break;
                    case NALU::INVALID:
                        break;
                }
            }
            break;
        }
        case VideoCodec::MJPEG:
            if(mp4Track == MP4_INVALID_TRACK_ID) {
                mp4Track = MP4AddVideoTrack(mp4Writer, MP4V2_TIMESCALE, MP4V2_TIMESCALE / fps, width, height, MP4_JPEG_VIDEO_TYPE);
                assert(mp4Track != MP4_INVALID_TRACK_ID);
                MP4SetVideoProfileLevel(mp4Writer, 0x7F);
            } else {
                if(!MP4WriteSample(mp4Writer, mp4Track, data.data(), data.size())) {
                    spdlog::warn("Failed to write sample to MP4 file");
                }
            }
            break;
        case VideoCodec::RAW:
#ifdef DEPTHAI_HAVE_OPENCV_SUPPORT
            if(!cvWriter->isOpened()) {
                throw std::runtime_error("VideoRecorder OpenCV writer is not initialized");
            }
            cv::Mat img(height, width, CV_8UC3, (void*)data.data());
            cvWriter->write(img);
#else
            throw std::runtime_error("OpenCV support is required to write RAW video");
#endif
            break;
    }
}

void VideoRecorder::close() {
    if(mp4Writer != MP4_INVALID_FILE_HANDLE) {
        MP4Close(mp4Writer);
    }
#ifdef DEPTHAI_HAVE_OPENCV_SUPPORT
    if(cvWriter->isOpened()) {
        cvWriter->release();
    }
#endif
}

VideoPlayer::~VideoPlayer() {

}

void VideoPlayer::init(const std::string& filePath) {

}

std::vector<uint8_t> VideoPlayer::next() {
    return {};
}

void VideoPlayer::close() {

}

bool checkRecordConfig(std::string& recordPath, utility::RecordConfig& config) {
    if(!platform::checkPathExists(recordPath)) {
        spdlog::warn("DEPTHAI_RECORD path does not exist or is invalid. Record disabled.");
        return false;
    }
    if(platform::checkPathExists(recordPath, true)) {
        // Is a directory
        config.outputDir = recordPath;
    } else {
        // Is a file
        std::string ext = recordPath.substr(recordPath.find_last_of('.') + 1);
        if(ext != "json") {
            spdlog::warn("DEPTHAI_RECORD path is not a directory or a json file. Record disabled.");
            return false;
        }
        try {
            std::ifstream file(recordPath);
            json j = json::parse(file);
            config = j.get<utility::RecordConfig>();

            if(platform::checkPathExists(config.outputDir, true)) {
                // Is a directory
                recordPath = config.outputDir;
            } else {
                spdlog::warn("DEPTHAI_RECORD outputDir is not a directory. Record disabled.");
                return false;
            }
        } catch(const std::exception& e) {
            spdlog::warn("Error while processing DEPTHAI_RECORD json file: {}. Record disabled.", e.what());
            return false;
        }
    }
    return true;
}

bool allMatch(const std::vector<std::string>& v1, const std::vector<std::string>& v2) {
    for(const auto& el : v1) {
        if(std::find(v2.begin(), v2.end(), el) == v2.end()) return false;
    }
    return true;
}
std::string matchTo(const std::vector<std::string>& mxIds, const std::vector<std::string>& filenames, const std::vector<std::string>& nodenames) {
    std::string mxId = "";
    for(const auto& id : mxIds) {
        std::vector<std::string> matches;
        for(const auto& filename : filenames) {
            if(filename.size() >= 4 && filename.substr(filename.size() - 4, filename.size()) != "meta" && filename.find(id) != std::string::npos) {
                matches.push_back(filename.substr(id.size() + 1, filename.find_last_of('.') - id.size() - 1));
            }
        }
        if(matches.size() == nodenames.size()) {
            if(allMatch(matches, nodenames)) {
                if(mxId.empty()) {
                    mxId = id;
                } else {
                    throw std::runtime_error("Multiple recordings match the pipeline configuration - unsupported.");
                }
            }
        }
    }
    return mxId;
}

}  // namespace utility
}  // namespace dai