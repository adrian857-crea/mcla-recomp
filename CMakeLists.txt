
# MCLA - ReXGlue Recompiled Project
cmake_minimum_required(VERSION 3.25)
project(mcla LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    set(REXGLUE_HOST_TARGET main)
endif()

include(generated/rexglue.cmake)

set(MCLA_SOURCES
    src/main.cpp
    src/mcla_app.cpp
    src/crash_trace.cpp
    src/audio_fallback.cpp
    src/frame_timing.cpp
    src/render_perf.cpp
    src/perf_options.cpp
    src/warp_fix.cpp
    src/button_prompts.cpp
    src/settings_menu.cpp
    src/sampling_profiler.cpp
    src/code_pointer_scan.cpp
)

if(ANDROID)
    add_library(main SHARED
        ${MCLA_SOURCES}
        src/android_main.cpp
    )

        rexglue_setup_target(main GPU_PLUGINS xenos)

    # Propaga os diretórios públicos dos cabeçalhos do Dear ImGui.
    if(TARGET imgui::imgui)
        target_link_libraries(main PRIVATE imgui::imgui)
    else()
        message(FATAL_ERROR
            "O alvo imgui::imgui não foi criado pelo SDK ReXGlue")
    endif()

elseif(WIN32)
    add_executable(mcla WIN32 ${MCLA_SOURCES})
    rexglue_setup_target(mcla)

else()
    add_executable(mcla ${MCLA_SOURCES})
    rexglue_setup_target(mcla)
endif()

if(WIN32)
    target_link_libraries(mcla PRIVATE winmm)
endif()
