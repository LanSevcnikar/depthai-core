# Examples for the Depthai V3 API

The examples in this directory show the existing functionality of the Depthai V3 API.

The examples range from the ones that were just minimally ported from the V2 API, to showcase that porting the existing code is straightforward,
to the ones that are specifically designed to show the new features of the V3 API.

The StereoDepth/stereo_autocreate.py example is a good example of the new features of the V3 API which showcases the ability to automatically create the stereo inputs
as well as the ability to create a custom node that can be used in the pipeline.

```python
# Create pipeline
with dai.Pipeline() as pipeline:
    # Allow stereo inputs to be created automatically
    stereo = pipeline.create(dai.node.StereoDepth).build(autoCreateCameras=True)
    visualizer = pipeline.create(StereoVisualizer).build(stereo.disparity)
    pipeline.start()
    while pipeline.isRunning():
        time.sleep(0.1)
```


## Installation

To get the examples running, install the requirements with:

```
python3 depthai-core/bindings/python/examples/install_requirements.py
```

NOTE: Right now we only build Linux x86_64 wheels, so other wheels have to get installed manually.

## What's new in the V3 API
* No more expliclit XLink nodes - the XLink "bridges" are created automatically
* Host nodes - nodes that run on the host machine now cleanly interoperate with the device nodes
* Custom host nodes - the user can create custom nodes that run on the host machine
  * Both `ThreadedHostNode` and `HostNode` are supported.
  * `ThreadedHostNode` works in a very similar fashion to the `ScriptNode` where the user specifes a `run` function which is then executed in a separate thread.
  * `HostNode` has an input map `inputs` where all the inputs are implicitly synced
  * Available both in Python and C++
* Record and replay nodes
  * Holistic record and replay is WIP
* Support for both RVC2&RVC3 with initial support for RVC4
* Device is now available at node construction, so we will be able to create smart defaults
  * Not used extensively yet, will be added gradually to more and more nodes.
* Support for NNArchive for the existing NN nodes
* `build(params)` functions for nodes where they can autocreate its inputs
  * Not yet used extensively yet, will be added gradually to more and more nodes.


## How to port an example from V2 to V3
The process of porting an example from V2 to V3 should be straightforward.

The minimal needed changes:
* Remove the explicit creation of the device **or** pass the device in the pipeline constructor
* Remove the explicit XLink nodes
* Replace any `.getOutputQueue()` calls with `output.getQueue()` calls


### Quick porting example
Let's take the simplest `rgb_video.py` example and port it to the V3 API.

The commented out code from the old API is commented with #ORIG and the new code is commented with #NEW.:
```python
#!/usr/bin/env python3

import cv2
import depthai as dai

# Create pipeline
# ORIG
# pipeline = dai.Pipeline()
with dai.Pipeline() as pipeline:
    # Define source and output
    camRgb = pipeline.create(dai.node.ColorCamera)

    # ORIG
    # xoutVideo = pipeline.create(dai.node.XLinkOut)
    # xoutVideo.setStreamName("video")

    # Properties
    camRgb.setBoardSocket(dai.CameraBoardSocket.CAM_A)
    camRgb.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P)
    camRgb.setVideoSize(1920, 1080)

    # Linking
    camRgb.video.link(xoutVideo.input)
    # NEW
    videoQueue = camRgb.video.getQueue()

# ORIG
# with dai.Device(pipeline) as device:
#   video = device.getOutputQueue(name="video", maxSize=1, blocking=False)
#   while True:
# NEW
    while pipeline.isRunning():
        videoIn = video.get()
        # Get BGR frame from NV12 encoded video frame to show with opencv
        # Visualizing the frame on slower hosts might have overhead
        cv2.imshow("video", videoIn.getCvFrame())

        if cv2.waitKey(1) == ord('q'):
            break
```