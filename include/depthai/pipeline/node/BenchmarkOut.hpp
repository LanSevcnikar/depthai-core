#pragma once

#include <depthai/pipeline/DeviceNode.hpp>

// shared
#include <depthai-shared/properties/BenchmarkPropertiesOut.hpp>

namespace dai {
namespace node {

class BenchmarkOut : public NodeCRTP<DeviceNode, BenchmarkOut, BenchmarkPropertiesOut> {
   public:
    constexpr static const char* NAME = "BenchmarkOut";
    using NodeCRTP::NodeCRTP;

    void build() {
        properties.numMessages = -1;  // By default send messages indefinitely
    }

    /**
     *  Send messages out as fast as possible
     */
    Output out{true, *this, "out", Output::Type::MSender, {{DatatypeEnum::Buffer, true}}};

    /**
     * Message that will be sent repeatably
     */
    Input input{true, *this, "input", Input::Type::SReceiver, true, 1, {{DatatypeEnum::Buffer, true}}};

    /**
     * Sets number of messages to send, by default send messages indefinitely
     * @param num number of messages to send
     */
    void setNumMessagesToSend(int num);

    /**
     * Set FPS at which the node is sending out messages. 0 means as fast as possible
     */
    void setFps(float fps);
};

}  // namespace node
}  // namespace dai
