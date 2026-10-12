cmake_minimum_required(VERSION 3.22)

project(BlackMetalDrums VERSION 0.5.0)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures")

option(BMD_COPY_PLUGIN "Install the plugin into ~/Library/Audio/Plug-Ins after building" ON)

add_subdirectory(external/JUCE)

juce_add_plugin(BlackMetalDrums
    COMPANY_NAME "Grosbug"
    BUNDLE_ID com.grosbug.blackmetaldrums
    PRODUCT_NAME "Black Metal Drums"
    PLUGIN_MANUFACTURER_CODE Dsmn
    PLUGIN_CODE BmDr
    FORMATS AU
    IS_SYNTH FALSE
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${BMD_COPY_PLUGIN})

target_sources(BlackMetalDrums PRIVATE
    Source/Patterns.cpp
    Source/KitMaps.cpp
    Source/FillGenerator.cpp
    Source/EditTools.cpp
    Source/SongEditor.cpp
    Source/PatternFiles.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_features(BlackMetalDrums PRIVATE cxx_std_17)

target_compile_definitions(BlackMetalDrums PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(BlackMetalDrums
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Headless engine tests: cmake --build build --target EngineTest && ./build/EngineTest_artefacts/Release/EngineTest
juce_add_console_app(EngineTest PRODUCT_NAME "EngineTest")
target_sources(EngineTest PRIVATE Tests/EngineTest.cpp Source/Patterns.cpp Source/FillGenerator.cpp Source/EditTools.cpp)
target_compile_features(EngineTest PRIVATE cxx_std_17)
target_compile_definitions(EngineTest PRIVATE JUCE_USE_CURL=0)
target_link_libraries(EngineTest PRIVATE juce::juce_core juce::juce_data_structures PUBLIC juce::juce_recommended_config_flags)

# Processor tests against a simulated host: links the plugin's shared code.
juce_add_console_app(ProcessorTest PRODUCT_NAME "ProcessorTest")
target_sources(ProcessorTest PRIVATE Tests/ProcessorTest.cpp)
target_compile_features(ProcessorTest PRIVATE cxx_std_17)
target_link_libraries(ProcessorTest PRIVATE BlackMetalDrums)
target_include_directories(ProcessorTest PRIVATE $<TARGET_PROPERTY:BlackMetalDrums,INCLUDE_DIRECTORIES>)
target_compile_definitions(ProcessorTest PRIVATE $<TARGET_PROPERTY:BlackMetalDrums,COMPILE_DEFINITIONS>)

# Writes every factory pattern as a MIDI file: ExportPatterns <folder>
juce_add_console_app(ExportPatterns PRODUCT_NAME "ExportPatterns")
target_sources(ExportPatterns PRIVATE Tools/ExportPatterns.cpp Source/Patterns.cpp Source/PatternFiles.cpp)
target_compile_features(ExportPatterns PRIVATE cxx_std_17)
target_compile_definitions(ExportPatterns PRIVATE JUCE_USE_CURL=0)
target_link_libraries(ExportPatterns PRIVATE juce::juce_audio_basics juce::juce_data_structures PUBLIC juce::juce_recommended_config_flags)
