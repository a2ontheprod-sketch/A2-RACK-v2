cmake_minimum_required(VERSION 3.22)
project(A2RackV2 VERSION 2.0.0 LANGUAGES C CXX)

include(FetchContent)
# Pinned JUCE tag for reproducible builds; first configure downloads JUCE.
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.17-security
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(A2RackV2
    COMPANY_NAME "LTA Audio"
    BUNDLE_ID "audio.lta.a2rack.v2"
    PLUGIN_MANUFACTURER_CODE LtAa
    PLUGIN_CODE A2v2
    FORMATS VST3 Standalone
    PRODUCT_NAME "A2 RACK"
    DESCRIPTION "A2 RACK v2 - original master clipper and wavefolder"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_AUTO_MANIFEST TRUE
)

juce_generate_juce_header(A2RackV2)

target_sources(A2RackV2 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ShapeDSP.h
)
target_compile_features(A2RackV2 PRIVATE cxx_std_17)
target_compile_definitions(A2RackV2 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(A2RackV2 PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Independent numerical DSP test, run before packaging the plugin.
enable_testing()
add_executable(a2_dsp_test tests/test_shape.cpp)
target_compile_features(a2_dsp_test PRIVATE cxx_std_17)
add_test(NAME A2_Waveshaper_Stability COMMAND a2_dsp_test)

if (MSVC)
    target_compile_options(A2RackV2 PRIVATE /utf-8)
endif()
