# hunter_config(
#     nlohmann_json
#     VERSION "3.9.1"
#     URL "https://github.com/nlohmann/json/archive/v3.9.1.tar.gz"
#     SHA1 "f8a20a7e19227906d77de0ede97468fbcfea03e7"
# )
hunter_config(
    nlohmann_json
    VERSION "3.9.1"
)

hunter_config(
    XLink
    VERSION "luxonis-2021.4.2-xlink-linkid-race-fix"
    URL "https://github.com/luxonis/XLink/archive/1329ef4125cfdd3b8c236316f39cce9f4b94da2f.tar.gz"
    SHA1 "58cdfe63d767edd89e0586a81eee8c8e74a76cb3"
    CMAKE_ARGS
        XLINK_ENABLE_LIBUSB=${DEPTHAI_ENABLE_LIBUSB}
)

hunter_config(
    BZip2
    VERSION "1.0.8-p0"
)

hunter_config(
    spdlog
    VERSION "1.8.2"
    URL "https://github.com/gabime/spdlog/archive/v1.8.2.tar.gz"
    SHA1 "4437f350ca7fa89a0cd8faca1198afb36823f775"
    CMAKE_ARGS
        SPDLOG_BUILD_EXAMPLE=OFF
        SPDLOG_FMT_EXTERNAL=OFF
        CMAKE_CXX_VISIBILITY_PRESET=hidden
        CMAKE_C_VISIBILITY_PRESET=hidden
)

# libarchive, luxonis fork
hunter_config(
    libarchive-luxonis
    VERSION "hunter-3.5.2"
    URL "https://github.com/luxonis/libarchive/archive/45baa3a3e57104519e1165bcd5ac29c3bd8c9f3a.tar.gz"
    SHA1 "ca5cd0f1c31b9c187d7119cb1aa7467f8c231d29"
    CMAKE_ARGS
        ENABLE_ACL=OFF
        ENABLE_BZip2=OFF
        ENABLE_CAT=OFF
        ENABLE_CAT_SHARED=OFF
        ENABLE_CNG=OFF
        ENABLE_COVERAGE=OFF
        ENABLE_CPIO=OFF
        ENABLE_CPIO_SHARED=OFF
        ENABLE_EXPAT=OFF
        ENABLE_ICONV=OFF
        ENABLE_INSTALL=ON
        ENABLE_LIBB2=OFF
        ENABLE_LIBXML2=OFF
        ENABLE_LZ4=OFF
        ENABLE_LZMA=ON
        ENABLE_LZO=OFF
        ENABLE_LibGCC=OFF
        ENABLE_MBEDTLS=OFF
        ENABLE_NETTLE=OFF
        ENABLE_OPENSSL=OFF
        ENABLE_PCREPOSIX=OFF
        ENABLE_SAFESEH=AUTO
        ENABLE_TAR=OFF
        ENABLE_TAR_SHARED=OFF
        ENABLE_TEST=OFF
        ENABLE_WERROR=OFF
        ENABLE_XATTR=OFF
        ENABLE_ZLIB=OFF
        ENABLE_ZSTD=OFF
)

# Luxonis FP16 fork which doesn't use git cloning for its dependencies
hunter_config(
    FP16
    VERSION "luxonis-0.0.0"
    URL "https://github.com/luxonis/FP16/archive/c911175d2717e562976e606c6e5f799bf40cf94e.tar.gz"
    SHA1 "40e9723c87c2fe21781132c0f2f8b90338500e32"
    CMAKE_ARGS
        FP16_BUILD_BENCHMARKS=OFF
        FP16_BUILD_TESTS=OFF
)

if(NOT EMSCRIPTEN)
    # Backward - Stacktrace printer
    hunter_config(
      Backward
      VERSION "1.6"
      URL "https://github.com/bombela/backward-cpp/archive/refs/tags/v1.6.tar.gz"
      SHA1 "4ecb711eabfd15bc88ff9dd9342907fc5da46b62"
      CMAKE_ARGS
          BACKWARD_TESTS=OFF
    )
endif()

# libnop - Serialization
hunter_config(
    libnop
    VERSION "1.0-ec8f75a"
    URL "https://github.com/luxonis/libnop/archive/ab842f51dc2eb13916dc98417c2186b78320ed10.tar.gz"
    SHA1 "32f40f084615ba7940ce9d29f05f3294371aabeb"
)

# Specific Catch2 version
hunter_config(
    Catch2
    VERSION "3.4.0"
    URL "https://github.com/catchorg/Catch2/archive/refs/tags/v3.4.0.tar.gz"
    SHA1 "4c308576c856a43dc88949a8f64ef90ebf94ae1b"
)

# ZLib - Luxonis fix for alias on imported target for old CMake versions
hunter_config(
    ZLIB
    VERSION "1.2.11-p2"
    URL "https://github.com/luxonis/zlib/archive/refs/tags/v1.2.11-p2.tar.gz"
    SHA1 "fb8b6486183b13a86040f793a939b128f6d27095"
)

# TMP, could be read from XLink
# libusb without udev
hunter_config(
    libusb-luxonis
    VERSION "1.0.24-cmake"
    URL "https://github.com/luxonis/libusb/archive/b7e4548958325b18feb73977163ad44398099534.tar.gz"
    SHA1 "2d79573d57628fe56d2868d2f6ce756d40906cf4"
    CMAKE_ARGS
        WITH_UDEV=OFF
        # Build shared libs by default to not cause licensing issues
        BUILD_SHARED_LIBS=ON
)

# cpp-httplib
hunter_config(
    httplib
    VERSION "0.11.2"
    URL "https://github.com/luxonis/cpp-httplib/archive/3ba99c06f655a52e701c9a7ae5dc48850582d95b.tar.gz"
    SHA1 "84ddd5d58a210b6203c50760d2ebde75b0ff6376"
    CMAKE_ARGS
        HTTPLIB_USE_OPENSSL_IF_AVAILABLE=OFF
        HTTPLIB_USE_BROTLI_IF_AVAILABLE=OFF
)

# RTABMap
hunter_config(
    rtbmap
    VERSION "0.21.4"
    URL "https://github.com/introlab/rtabmap/archive/refs/tags/0.21.4.tar.gz"
    SHA1 "3ac7372c3bd6761095421a3282d66521822f89f7"
    CMAKE_ARGS
        WITH_UDEV=OFF
        # Build shared libs by default to not cause licensing issues
        BUILD_SHARED_LIBS=ON


        option(WITH_QT            "Include Qt support"                   OFF)
        option(WITH_ORB_OCTREE    "Include ORB Octree feature support"   OFF)
        option(WITH_TORCH         "Include Torch support (SuperPoint)"   OFF)
        option(WITH_PYTHON        "Include Python3 support (PyMatcher, PyDetector)"  OFF)
        option(WITH_PYTHON_THREADING  "Use more than one Python interpreter."  OFF)
        option(WITH_PDAL          "Include PDAL support"                 OFF)
        option(WITH_FREENECT      "Include Freenect support"             OFF)
        option(WITH_FREENECT2     "Include Freenect2 support"            OFF)
        option(WITH_K4W2          "Include Kinect for Windows v2 support" OFF)
        option(WITH_K4A           "Include Kinect for Azure support"     OFF)
        option(WITH_OPENNI2       "Include OpenNI2 support"              OFF)
        option(WITH_DC1394        "Include dc1394 support"               ON)
        option(WITH_G2O           "Include g2o support"                  ON)
        option(WITH_GTSAM         "Include GTSAM support"                ON)
        option(WITH_TORO          "Include TORO support"                 ON)
        option(WITH_CERES         "Include Ceres support"                OFF)
        option(WITH_MRPT          "Include MRPT support"                 ON)
        option(WITH_VERTIGO       "Include Vertigo support"              ON)
        option(WITH_CVSBA         "Include cvsba support"                OFF)
        option(WITH_POINTMATCHER  "Include libpointmatcher support"      ON)
        option(WITH_CCCORELIB     "Include CCCoreLib support"            OFF)
        option(WITH_OPEN3D        "Include Open3D support"               OFF)
        option(WITH_LOAM          "Include LOAM support"                 OFF)
        option(WITH_FLOAM         "Include FLOAM support"                OFF)
        option(WITH_FLYCAPTURE2   "Include FlyCapture2/Triclops support" OFF)
        option(WITH_ZED           "Include ZED sdk support"              OFF)
        option(WITH_ZEDOC         "Include ZED Open Capture support"     OFF)
        option(WITH_REALSENSE     "Include RealSense support"            OFF)
        option(WITH_REALSENSE_SLAM "Include RealSenseSlam support"       OFF)
        option(WITH_REALSENSE2    "Include RealSense support"            OFF)
        option(WITH_MYNTEYE       "Include mynteye-s support"            OFF)
        option(WITH_DEPTHAI       "Include depthai-core support"         OFF)
        option(WITH_OCTOMAP       "Include OctoMap support"              ON)
        option(WITH_GRIDMAP       "Include GridMap support"              ON)
        option(WITH_CPUTSDF       "Include CPUTSDF support"              OFF)
        option(WITH_OPENCHISEL    "Include open_chisel support"          OFF)
        option(WITH_ALICE_VISION  "Include AliceVision support"          OFF)
        option(WITH_FOVIS         "Include FOVIS supp++ort"                OFF)
        option(WITH_VISO2         "Include VISO2 support"                OFF)
        option(WITH_DVO           "Include DVO support"                  OFF)
        option(WITH_ORB_SLAM      "Include ORB_SLAM2 or ORB_SLAM3  support" OFF)
        option(WITH_OKVIS         "Include OKVIS support"                OFF)
        option(WITH_MSCKF_VIO     "Include MSCKF_VIO support"            OFF)
        option(WITH_VINS          "Include VINS-Fusion support"          OFF)
        option(WITH_OPENVINS      "Include OpenVINS support"             OFF)
        option(WITH_MADGWICK      "Include Madgwick IMU filtering support" ON)
        option(WITH_FASTCV        "Include FastCV support"               ON)
        option(WITH_OPENMP        "Include OpenMP support"               ON)
        option(WITH_OPENGV        "Include OpenGV support"               ON)
        IF(MOBILE_BUILD)
        option(PCL_OMP            "With PCL OMP implementations"         OFF)
        ELSE()
        option(PCL_OMP            "With PCL OMP implementations"         ON)
        ENDIF()

)
