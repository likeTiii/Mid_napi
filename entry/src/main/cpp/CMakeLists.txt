# the minimum version of CMake.
cmake_minimum_required(VERSION 3.16)
project(Mid_napi)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)


set(OHOS_SDK_PATH "D:/Code/OpenHarmony/SDK/11")
set(CMAKE_SYSROOT "${OHOS_SDK_PATH}/native/sysroot")
set(CLANG_PATH "${OHOS_SDK_PATH}/native/llvm/bin/clang++")
set(CMAKE_TOOLCHAIN_FILE "${OHOS_SDK_PATH}/native/build/cmake/ohos.toolchain.cmake")
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined -Wl,--as-needed")

set(OHOS_ARCH "arm64-v8a")

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(
                    ${OHOS_SDK_PATH}/native/sysroot/usr/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware
                    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Node/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Util/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Msgs/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/common
                    ${CMAKE_CURRENT_SOURCE_DIR}/render
                    )

#链接库
link_directories(
    ${OHOS_SDK_PATH}/native/sysroot/usr/lib/aarch64-linux-ohos
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Node/build/lib
)

add_compile_definitions(BOOST_ASIO_HAS_STD_INVOKE_RESULT _LIBCPP_ENABLE_CXX17_REMOVED_FEATURES)
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)

# 递归查找所有 .pb.cc 文件
file(GLOB_RECURSE PROTO_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Msgs/src/*.pb.cc"
)

set(MIDDLEWARE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/napi_init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Util/src/napi_util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Util/src/ParseDrawMsg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Node/src/Node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Node/src/NodeImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/src/Node/src/Timer.cpp
    ${PROTO_SRCS}
)
set(OPENGL_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/napi_init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/manager/plugin_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/egl_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/egl_path.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/plugin_render.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/render/plugin_path.cpp
)

add_library(entry SHARED
    ${MIDDLEWARE_SRC}
    ${OPENGL_SRC}

)

#target_compile_options(entry PRIVATE "-stdlib=libc++")

target_link_libraries(entry PUBLIC libace_napi.z.so
                        libhilog_ndk.z.so
                        librawfile.z.so
                        pthread
                        jsoncpp.a
                        protobuf.a
                        spdlog
                        rt
                        yaml-cpp
                        boost_system.a
                        boost_thread.a
                        #middleware_node_shm
                        #middleware_node_hmw
                        )



# link OpenHarmony native library （Use OpenGL）
target_link_libraries(entry PUBLIC EGL)
target_link_libraries(entry PUBLIC GLESv3)
target_link_libraries(entry PUBLIC hilog_ndk.z)
target_link_libraries(entry PUBLIC ace_ndk.z)
target_link_libraries(entry PUBLIC ace_napi.z)
target_link_libraries(entry PUBLIC libc++.a)
target_link_libraries(entry PUBLIC z)
target_link_libraries(entry PUBLIC uv)
target_link_libraries(entry PUBLIC libace_napi.z.so)



target_link_options(entry PRIVATE "-Wl,--no-as-needed")