cmake_minimum_required(VERSION 3.16)
project(CADViewer LANGUAGES CXX)

# ============================================================
# Project Configuration
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ============================================================
# Find Qt6
# ============================================================
find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
    REQUIRED
)

# ============================================================
# Find OpenCASCADE (OCCT)
# ============================================================
# Preserved from parent CMake: Explicit path for OCCT
set(OpenCASCADE_DIR "/opt/occt-7.7/lib/cmake/opencascade")
find_package(OpenCASCADE REQUIRED)

# Print OCCT information for verification
message(STATUS "OpenCASCADE found: ${OpenCASCADE_FOUND}")
message(STATUS "OpenCASCADE libraries: ${OpenCASCADE_LIBRARIES}")

# ============================================================
# Source Files
# ============================================================
set(PROJECT_SOURCES
    # Headers
    include/OcctQMainWindowSample.h
    include/Core.h              # Renamed from OcctQWidgetViewer.h
    include/CadModel.h
    include/Render.h
    include/Measurement.h
    include/Input.h
    include/Event.h
    include/OcctQtTools.h
    include/OcctGlTools.h

    # Source files
    src/main.cpp
    src/OcctQMainWindowSample.cpp
    src/Core.cpp            # Renamed from OcctQWidgetViewer.cpp
    src/CadModel.cpp
    src/Render.cpp
    src/Measurement.cpp
    src/Input.cpp
    src/Event.cpp
    src/OcctQtTools.cpp
    src/OcctGlTools.cpp
)

# ============================================================
# Create Executable
# ============================================================
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# ============================================================
# Link Libraries
# ============================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    ${OpenCASCADE_LIBRARIES}
)

# ============================================================
# Include Directories
# ============================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIR}
)

# ============================================================
# Compiler Options (Preserved from parent)
# ============================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ============================================================
# Output Directory (Preserved from parent)
# ============================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ============================================================
# OCCT-specific Settings (Preserved from parent)
# ============================================================
# Enable OCCT features we need
target_compile_definitions(${PROJECT_NAME} PRIVATE
    OCCT_INCLUDE_DIR="${OpenCASCADE_INCLUDE_DIR}"
)

# ============================================================
# Build Information
# ============================================================
message(STATUS "")
message(STATUS "========== CAD Viewer Build Configuration ==========")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "OpenCASCADE Path: ${OpenCASCADE_DIR}")
message(STATUS "")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}")
message(STATUS "====================================================")
message(STATUS "")
