cmake_minimum_required(VERSION 3.22)
project(ThiccBox VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../JUCE JUCE)

juce_add_plugin(ThiccBox
    COMPANY_NAME "KOJI TADOKORO"
    PLUGIN_MANUFACTURER_CODE Ktdk
    PLUGIN_CODE Thcc
    FORMATS VST3
    PRODUCT_NAME "KOJI TADOKORO Thicc Box"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(ThiccBox PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)

target_compile_definitions(ThiccBox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ThiccBox
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
