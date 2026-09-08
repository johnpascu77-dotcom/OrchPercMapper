cmake_minimum_required(VERSION 3.22)

project(OrchPercMapper VERSION 0.1.0)

# Path to your JUCE folder.
add_subdirectory("C:/JUCE/JUCE" JUCE)

juce_add_plugin(OrchPercMapper
    COMPANY_NAME "JohnPascu"
    PLUGIN_MANUFACTURER_CODE Jpas
    PLUGIN_CODE Opmp

    FORMATS VST3

    PRODUCT_NAME "OrchPercMapper"
    PLUGIN_NAME "OrchPercMapper"
    BUNDLE_ID "com.johnpascu.OrchPercMapper"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_REPLACES_VST2 FALSE

    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(OrchPercMapper)

target_sources(OrchPercMapper
    PRIVATE
        Source/OrchPercMapperProcessor.cpp
        Source/OrchPercMapperEditor.cpp
)

target_compile_definitions(OrchPercMapper
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OrchPercMapper
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_extra

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if (TARGET OrchPercMapper_VST3)
    target_compile_definitions(OrchPercMapper_VST3
        PRIVATE
            JUCE_VST3_CAN_REPLACE_VST2=0
    )
endif()
