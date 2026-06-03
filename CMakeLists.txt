cmake_minimum_required(VERSION 3.22)

project(STRATA VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE
#   Option A (recommended): vendored as a git submodule in ./JUCE
#   Option B: FetchContent (uncomment below)
# ---------------------------------------------------------------------------
add_subdirectory(JUCE)
# include(FetchContent)
# FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4)
# FetchContent_MakeAvailable(JUCE)

juce_add_plugin(STRATA
    COMPANY_NAME            "MyGAin"
    BUNDLE_ID               "com.mygain.strata"
    PLUGIN_MANUFACTURER_CODE Mygn          # 4 chars, one uppercase
    PLUGIN_CODE             Strt           # unique 4-char code
    FORMATS                 VST3 AU Standalone
    PRODUCT_NAME            "STRATA"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES         "Fx" "Tools"
    AU_MAIN_TYPE            "kAudioUnitType_Effect")

juce_generate_juce_header(STRATA)

target_sources(STRATA PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/link/InstanceRegistry.cpp)

target_compile_definitions(STRATA PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(STRATA PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
