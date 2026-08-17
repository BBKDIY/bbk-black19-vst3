cmake_minimum_required(VERSION 3.20)
project(BBKBlack19 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "This project only targets Windows (VST3 DLL for MSVC).")
endif()

include(FetchContent)
FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES base pluginterfaces public.sdk
)
FetchContent_Populate(vst3sdk)
set(VST3_SDK_ROOT ${vst3sdk_SOURCE_DIR})

set(BBK_SOURCES
    Source/Black19Processor.cpp
    Source/Black19View.cpp
    Source/Factory.cpp
    ${VST3_SDK_ROOT}/pluginterfaces/base/funknown.cpp
    ${VST3_SDK_ROOT}/pluginterfaces/base/coreiids.cpp
    ${VST3_SDK_ROOT}/pluginterfaces/base/conststringtable.cpp
    ${VST3_SDK_ROOT}/pluginterfaces/base/ustring.cpp
    ${VST3_SDK_ROOT}/base/source/fobject.cpp
    ${VST3_SDK_ROOT}/base/source/fstring.cpp
    ${VST3_SDK_ROOT}/base/source/fstreamer.cpp
    ${VST3_SDK_ROOT}/base/source/updatehandler.cpp
    ${VST3_SDK_ROOT}/base/source/fbuffer.cpp
    ${VST3_SDK_ROOT}/base/source/fdebug.cpp
    ${VST3_SDK_ROOT}/base/source/baseiids.cpp
    ${VST3_SDK_ROOT}/base/thread/source/flock.cpp
    ${VST3_SDK_ROOT}/public.sdk/source/vst/vstcomponentbase.cpp
    ${VST3_SDK_ROOT}/public.sdk/source/vst/vstcomponent.cpp
    ${VST3_SDK_ROOT}/public.sdk/source/vst/vstbus.cpp
    ${VST3_SDK_ROOT}/public.sdk/source/vst/vstparameters.cpp
    ${VST3_SDK_ROOT}/public.sdk/source/vst/vstinitiids.cpp
    ${VST3_SDK_ROOT}/public.sdk/source/common/pluginview.cpp
)

add_library(BBKBlack19 SHARED ${BBK_SOURCES})

target_include_directories(BBKBlack19 PRIVATE
    Source
    ${VST3_SDK_ROOT}
    ${VST3_SDK_ROOT}/base/thread/include
)

target_compile_definitions(BBKBlack19 PRIVATE
    RELEASE=1
    _WIN32_WINNT=0x0601
    UNICODE
    _UNICODE
)

target_link_libraries(BBKBlack19 PRIVATE user32 gdi32 comctl32 ole32)

set_target_properties(BBKBlack19 PROPERTIES
    OUTPUT_NAME "BBK Black-19"
    SUFFIX ".vst3"
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist
)

set_property(TARGET BBKBlack19 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
