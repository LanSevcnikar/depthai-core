#include "depthai/depthai.hpp"
#include "depthai/pipeline/node/HostNode.hpp"
#include "depthai/pipeline/node/host/Display.hpp"

int main() {
    dai::Pipeline pipeline();

    auto replay = pipeline.create<dai::node::Replay>();
    auto cam = pipeline.create<dai::node::Camera>();
    auto display = pipeline.create<dai::node::Display>();

    cam->setBoardSocket(dai::CameraBoardSocket::CAM_A);

    replay->setReplayVideo("video.mp4");
    replay->setReplayFile("video.mcap");
    replay->setOutFrameType(dai::ImgFrame::Type::YUV420p);

    replay->out.link(cam->mockIsp);
    cam->video.link(display->input);

    pipeline.start();

    std::this_thread::sleep_for(std::chrono::seconds(10));

    pipeline.stop();
}
