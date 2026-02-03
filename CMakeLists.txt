cmake_minimum_required(VERSION 3.15)

project(CrateDigger VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    file(GLOB WEBVIEW2_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.Web.WebView2.*"
    )

    list(LENGTH WEBVIEW2_DIR WEBVIEW2_COUNT)
    if (WEBVIEW2_COUNT EQUAL 0)
        message(FATAL_ERROR "WebView2 NuGet package not found in packages/")
    endif()

    list(GET WEBVIEW2_DIR 0 WEBVIEW2_DIR)

    set(JUCE_WEBVIEW2_PACKAGE_LOCATION
        "${WEBVIEW2_DIR}"
        CACHE PATH "WebView2 NuGet package location"
    )

    set(WebView2_ROOT
        "${WEBVIEW2_DIR}"
        CACHE PATH "WebView2 root directory"
    )

    message(STATUS "Using WebView2 SDK at: ${WEBVIEW2_DIR}")
endif() 

add_subdirectory(JUCE)

juce_add_plugin(CrateDigger
    COMPANY_NAME "FerretCode"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE "frrt"
    PLUGIN_CODE "CrtD"
    FORMATS VST3 Standalone
    PRODUCT_NAME "CrateDigger"
    NEEDS_WEBVIEW2 TRUE
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_BIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Resources/linux")
elseif(WIN32)
    set(PLATFORM_BIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Resources/win")
endif()

message(STATUS "Binaries Source Path: ${PLATFORM_BIN_SRC}")

function(copy_binaries_to_target target_name)
    if(NOT EXISTS "${PLATFORM_BIN_SRC}")
        message(WARNING "Binary source folder not found at: ${PLATFORM_BIN_SRC}. Binaries will NOT be copied.")
        return()
    endif()

    get_target_property(OUT_DIR ${target_name} BINARY_DIR)
    set(BIN_DEST_DIR "${OUT_DIR}/CrateDigger_Binaries")

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_DEST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PLATFORM_BIN_SRC}" "${BIN_DEST_DIR}"
        COMMENT "Copying binaries to ${BIN_DEST_DIR}"
    )
endfunction()

if(TARGET CrateDigger_Standalone)
    copy_binaries_to_target(CrateDigger_Standalone)
endif()

if(TARGET CrateDigger_VST3)
    if(EXISTS "${PLATFORM_BIN_SRC}")
        add_custom_command(TARGET CrateDigger_VST3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${PLATFORM_BIN_SRC}" "$<TARGET_FILE_DIR:CrateDigger_VST3>/../../../CrateDigger_Binaries"
            COMMENT "Copying binaries next to VST3 bundle..."
        )
    else()
        message(WARNING "Skipping VST3 binary copy - Source folder missing.")
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(CURL REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)

    target_include_directories(CrateDigger PRIVATE ${GTK3_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS})
    target_link_directories(CrateDigger PRIVATE ${GTK3_LIBRARY_DIRS} ${WEBKIT_LIBRARY_DIRS})
    target_link_libraries(CrateDigger PRIVATE ${GTK3_LIBRARIES} ${WEBKIT_LIBRARIES} CURL::libcurl)
endif()

target_sources(CrateDigger
    PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PathHelper.h
)

target_link_libraries(CrateDigger
    PRIVATE
    juce::juce_core
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_events
    juce::juce_data_structures
)

target_compile_definitions(CrateDigger
    PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=0
)

if(WIN32)
    target_compile_definitions(CrateDigger
        PUBLIC
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    )
endif()

juce_generate_juce_header(CrateDigger)

if(TARGET CrateDigger_Standalone)
    install(TARGETS CrateDigger_Standalone DESTINATION bin)

    if(EXISTS "${PLATFORM_BIN_SRC}")
         install(DIRECTORY "${PLATFORM_BIN_SRC}/" DESTINATION bin/CrateDigger_Binaries)
    endif()
endif()

if(TARGET CrateDigger_VST3)
    install(TARGETS CrateDigger_VST3 DESTINATION VST3)

    if(EXISTS "${PLATFORM_BIN_SRC}")
         install(DIRECTORY "${PLATFORM_BIN_SRC}/" DESTINATION VST3/CrateDigger_Binaries)
    endif()
endif()

set(CPACK_PACKAGE_NAME "CrateDigger")
set(CPACK_PACKAGE_VENDOR "FerretCode")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CrateDigger Audio Plugin")
set(CPACK_PACKAGE_VERSION "0.0.1")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CrateDigger")
set(CPACK_PACKAGE_CONTACT "ferret@frrt.space")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "CrateDigger")
    set(CPACK_NSIS_PACKAGE_NAME "CrateDigger")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "FerretCode")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
