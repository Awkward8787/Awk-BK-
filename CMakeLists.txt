cmake_minimum_required(VERSION 3.22)

# Universal macOS binary (Apple Silicon + Intel) so FL Studio loads on both.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target" FORCE)
endif()

project(AwkBK VERSION 0.1.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# JUCE 8.0.17-security (commit dabd5ad13cd869cba41046947fffc8e5ce5cd662).
# Open-source JUCE 8 is AGPLv3. This repo is AGPLv3 for that reason.
# Shallow-clone the release tag; do not vendor the framework.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.17-security
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

add_library(AwkBKCore STATIC
    Source/PitchDetector.cpp
    Source/TempoDetector.cpp
    Source/KeyEstimator.cpp
)
target_include_directories(AwkBKCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_features(AwkBKCore PUBLIC cxx_std_17)
set_target_properties(AwkBKCore PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(AwkBKCore PRIVATE /W4)
else()
    target_compile_options(AwkBKCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(AwkBKTests tests/TestMain.cpp)
target_link_libraries(AwkBKTests PRIVATE AwkBKCore)
target_compile_features(AwkBKTests PRIVATE cxx_std_17)
target_compile_definitions(AwkBKTests PRIVATE
    "AWK_PROCESSOR_SOURCE=\"${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp\""
)

enable_testing()
add_test(NAME dsp COMMAND AwkBKTests)

juce_add_binary_data(AwkBKData SOURCES
    assets/AwkBeatsLogo.png
)

juce_add_plugin(AwkBK
    COMPANY_NAME "AwkBeats"
    BUNDLE_ID com.awkbeats.awkbk
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE AwBt
    PLUGIN_CODE AwBk
    FORMATS VST3
    PRODUCT_NAME "Awk BK"
    COMPANY_COPYRIGHT "Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)"
    VST3_CATEGORIES Analyzer
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

juce_generate_juce_header(AwkBK)

target_sources(AwkBK PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(AwkBK PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
)

# Offscreen snapshot of the editor. Off by default so CI does not need a display.
option(AWK_BUILD_PREVIEW "Build the offscreen UI preview tool" OFF)
if(AWK_BUILD_PREVIEW)
    juce_add_console_app(AwkBKPreview)
    target_sources(AwkBKPreview PRIVATE
        tools/RenderPreview.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
    )
    target_compile_definitions(AwkBKPreview PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_REPORT_APP_USAGE=0
        "JucePlugin_VersionString=\"${PROJECT_VERSION}\""
    )
    target_link_libraries(AwkBKPreview PRIVATE
        AwkBKCore
        AwkBKData
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
endif()

target_link_libraries(AwkBK PRIVATE
    AwkBKCore
    AwkBKData
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Constructs the processor, runs the analysis thread, and toggles Lock.
# The dsp test's processBlock check is only a source lint.
add_executable(AwkBKProcessorTests
    tests/ProcessorTest.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)
target_include_directories(AwkBKProcessorTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_features(AwkBKProcessorTests PRIVATE cxx_std_17)
target_compile_definitions(AwkBKProcessorTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0
    "JucePlugin_VersionString=\"${PROJECT_VERSION}\""
)
target_link_libraries(AwkBKProcessorTests PRIVATE
    AwkBKCore
    AwkBKData
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
add_test(NAME processor COMMAND AwkBKProcessorTests)
set_tests_properties(processor PROPERTIES TIMEOUT 60)

if(TARGET AwkBK_VST3)
    add_custom_command(TARGET AwkBK_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AwkBK_VST3>/../Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
            "$<TARGET_FILE_DIR:AwkBK_VST3>/../Resources/THIRD_PARTY_NOTICES.md"
        VERBATIM
    )
endif()
