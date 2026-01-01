# Complete Build & Setup Guide
## CAD Viewer with Qt 6 + OpenCASCADE 7.7

---

## 📦 File Manifest - What You Have

### Core Source Files Created ✅
```
src/
├── main.cpp                        ✅ Entry point with Qt setup
├── OcctQWidgetViewer.cpp          ✅ Main viewer implementation
├── OcctQMainWindowSample.cpp      ✅ Main window UI
├── OcctQtTools.cpp                ⚠️  Use from attached files
└── OcctGlTools.cpp                ⚠️  Use from attached files

include/
├── OcctQWidgetViewer.h            ✅ Viewer header
├── OcctQMainWindowSample.h        ✅ Main window header
├── OcctQtTools.h                  ✅ Qt tools header
└── OcctGlTools.h                  ✅ GL tools header
```

### Configuration Files ✅
```
CMakeLists.txt                      ✅ CMake configuration
README.md                           ✅ Complete documentation
```

---

## 🔧 Installation Steps

### Step 1: Set Up Project Directory

```bash
# Create project directory
mkdir CADViewer
cd CADViewer

# Create directory structure
mkdir -p src include build

# Copy all created files
# Copy CMakeLists.txt to root
# Copy all headers to include/
# Copy all cpp files to src/
```

### Step 2: Install Dependencies

#### Ubuntu 22.04 / Debian 11
```bash
# Update package manager
sudo apt update

# Install Qt6 development files
sudo apt install -y \
    qt6-base-dev \
    qt6-opengl-dev \
    qt6-tools-dev \
    libqt6opengl6-dev

# Install OpenCASCADE
# Option A: From system repositories (recommended for quick setup)
sudo apt install -y liboce-ocaf-dev

# Option B: Build from source for latest version
git clone https://github.com/Open-Cascade-SAS/OCCT.git
cd OCCT
# Follow OCCT build instructions
# Usually: cmake . && make && sudo make install

# Install build tools
sudo apt install -y cmake g++ git
```

#### macOS (using Homebrew)
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake qt@6 opencascade

# Link Qt6
brew link qt@6 --force
```

#### Windows
1. **Install Visual Studio Build Tools** (if not already installed)
   - Download from: https://visualstudio.microsoft.com/downloads/
   - Select "C++ development tools"

2. **Install Qt 6**
   - Download from: https://www.qt.io/download
   - Select "Qt Creator" and "Qt 6.x" libraries

3. **Install CMake**
   - Download from: https://cmake.org/download/
   - Or use Chocolatey: `choco install cmake`

4. **Install OpenCASCADE 7.7**
   - Download from: https://github.com/Open-Cascade-SAS/OCCT/releases
   - Extract to `C:\OpenCASCADE\` (or preferred location)

### Step 3: Verify Installations

```bash
# Check CMake
cmake --version
# Should show 3.16 or higher

# Check Qt6
qmake -version
# Or check Qt installation path

# Check GCC/Clang
g++ --version
# Or clang --version on macOS
```

### Step 4: Build the Project

```bash
cd CADViewer
mkdir -p build
cd build

# Configure with CMake
# On Linux/macOS:
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/usr/local/lib/cmake/Qt6:/usr/local/lib/cmake/OpenCASCADE"

# On Windows (Visual Studio):
cmake .. `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64;C:\OpenCASCADE"

# Build
cmake --build . --config Release

# On multi-core systems, use parallel build
cmake --build . --config Release --parallel 4
```

### Step 5: Run the Application

```bash
# On Linux/macOS
./bin/CADViewer

# On Windows
.\bin\CADViewer.exe
```

---

## 🔍 Troubleshooting Build Issues

### Issue: CMake Cannot Find Qt6

**Solution 1: Manual Qt6 Path**
```bash
# Find Qt6 installation
find /usr -name "Qt6Config.cmake" 2>/dev/null

# Use the path in CMake:
cmake .. -DCMAKE_PREFIX_PATH="/usr/lib/x86_64-linux-gnu/cmake/Qt6"
```

**Solution 2: Export Qt Path**
```bash
export CMAKE_PREFIX_PATH=/usr/lib/cmake/Qt6:$CMAKE_PREFIX_PATH
cmake ..
```

**Solution 3: Use Qt Creator**
- Open CMakeLists.txt with Qt Creator
- Qt Creator handles paths automatically

### Issue: CMake Cannot Find OpenCASCADE

**Solution 1: Manual OCCT Path**
```bash
# If you built from source:
export CMAKE_PREFIX_PATH=/usr/local/lib/cmake/OpenCASCADE:$CMAKE_PREFIX_PATH

# Or specify directly:
cmake .. -DOpenCASCADE_DIR=/usr/local/lib/cmake/OpenCASCADE
```

**Solution 2: Check Installation**
```bash
# Verify OpenCASCADE is installed correctly
ls /usr/local/lib/cmake/OpenCASCADE/
# Should contain OpenCASCADEConfig.cmake
```

### Issue: Linker Errors - Undefined References

**Cause**: Missing OpenCASCADE libraries

**Solution**:
```bash
# Verify OCCT libraries are available
find /usr/local -name "libTKSTEP.so*" 2>/dev/null

# If not found, reinstall OpenCASCADE
# Or build from source ensuring all modules are enabled
```

### Issue: Qt6 Not Found - "Qt6_NOT_FOUND"

```bash
# Alternative: Use qmake
qmake --version

# If using qmake, convert CMakeLists or use .pro files
# Or install Qt6 development files
sudo apt install qt6-base-dev qt6-opengl-dev
```

### Issue: Application Crashes with OpenGL Error

**Solution**:
1. Update graphics drivers
2. Verify OpenGL 3.3+ support
3. Check in application output for specific error

```cpp
// Add to OcctQWidgetViewer.cpp temporarily for debugging:
qDebug() << "OpenGL Version: " << glGetString(GL_VERSION);
qDebug() << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
```

---

## 📋 Final Project Structure

After following all steps, your directory should look like:

```
CADViewer/
├── CMakeLists.txt
├── README.md
├── BUILD_GUIDE.md                   (this file)
├── include/
│   ├── OcctQMainWindowSample.h
│   ├── OcctQWidgetViewer.h
│   ├── OcctQtTools.h
│   └── OcctGlTools.h
├── src/
│   ├── main.cpp
│   ├── OcctQMainWindowSample.cpp
│   ├── OcctQWidgetViewer.cpp
│   ├── OcctQtTools.cpp (from attached files)
│   └── OcctGlTools.cpp (from attached files)
└── build/
    ├── CMakeCache.txt
    ├── CMakeFiles/
    ├── cmake_install.cmake
    ├── Makefile (Linux/macOS)
    ├── CADViewer.sln (Windows)
    └── bin/
        └── CADViewer (executable)
```

---

## ✅ Verification Checklist

- [ ] CMake 3.16+ installed
- [ ] Qt 6.2+ installed
- [ ] OpenCASCADE 7.7 installed
- [ ] C++ compiler with C++17 support
- [ ] All source files copied to correct directories
- [ ] CMakeLists.txt configured correctly
- [ ] `cmake ..` ran without errors
- [ ] `cmake --build .` compiled successfully
- [ ] `./bin/CADViewer` launches without errors
- [ ] Can open File → Open CAD Model dialog
- [ ] Can load a test STEP/IGES/BREP file

---

## 🚀 Quick Start Commands (Copy-Paste)

### Linux/macOS Quick Build
```bash
cd ~/CADViewer
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel 4
./bin/CADViewer
```

### Windows PowerShell Quick Build
```powershell
cd C:\CADViewer
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
.\bin\CADViewer.exe
```

---

## 📞 Getting Help

1. **Check Build Output**: Look for specific error messages
2. **Verify Paths**: Ensure all library paths are correct
3. **Try Minimal Build**: Create simple test program
4. **Update Tools**: Upgrade CMake, Qt, compilers to latest

```bash
# Minimal test CMake file to verify setup
# Save as test_cmake.txt and try:
cmake --version
qmake -version
g++ --version
```

---

## 🎯 Next Steps After Building

1. Open a test CAD file from:
   - GrabCAD: https://grabcad.com
   - FreeCAD Models: https://github.com/FreeCAD
   - 3D Models: https://free3d.com

2. Explore the code and understand:
   - CAD import in OcctQWidgetViewer::loadCADModel()
   - UI setup in OcctQMainWindowSample::createMenuBar()
   - View controls in event handlers

3. Extend the application:
   - Add face selection (Phase 3)
   - Add measurements (Phase 4)
   - Add export functionality

---

**Version**: 1.0  
**Last Updated**: December 30, 2025  
**Status**: Complete & Ready for Building
