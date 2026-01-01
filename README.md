# CAD Viewer Application - Qt + OpenCASCADE 7.7

A professional CAD model viewer built with Qt 6 and OpenCASCADE 7.7, supporting STEP, IGES, and BREP file formats with interactive 3D visualization.

## Features

✅ **CAD Model Import**
- STEP (.step, .stp) file import
- IGES (.iges, .igs) file import
- BREP (.brep) file import
- Automatic format detection

✅ **3D Visualization**
- Interactive 3D viewport with pan, rotate, zoom
- View cube navigation
- Grid display
- Lighting and shading
- MSAA anti-aliasing (4x samples)

✅ **User Interface**
- Professional menu bar (File, View, Help)
- Properties dock
- Measurements dock
- Status bar with feedback
- Progress dialog for file loading

✅ **View Controls**
- Left Click + Drag: Rotate view
- Middle Click + Drag: Pan view
- Scroll Wheel: Zoom in/out
- F key: Fit all objects
- ESC: Exit application

## System Requirements

### Minimum Requirements
- **OS**: Windows 10+, macOS 10.13+, Linux (Ubuntu 18.04+)
- **CPU**: Intel/AMD processor with SSE2 support
- **RAM**: 4 GB minimum
- **GPU**: OpenGL 3.3+ capable graphics card

### Development Requirements
- **CMake**: 3.16 or higher
- **Qt**: 6.2 or higher
- **OpenCASCADE**: 7.7.0
- **C++ Compiler**: C++17 compatible (GCC 7+, Clang 5+, MSVC 2017+)

## Building from Source

### Prerequisites Installation

#### On Ubuntu/Debian:
```bash
# Install Qt6
sudo apt-get install qt6-base-dev qt6-opengl-dev

# Install OpenCASCADE (build from source or use system package if available)
# Most distributions have it in repos, try:
sudo apt-get install liboce-ocaf-dev

# Or download from https://github.com/Open-Cascade-SAS/OCCT/releases
```

#### On macOS:
```bash
# Install using Homebrew
brew install qt@6
brew install opencascade

# Or use pre-built binaries from OpenCASCADE website
```

#### On Windows:
1. Install Qt 6 from https://www.qt.io/download
2. Download OpenCASCADE 7.7 from https://github.com/Open-Cascade-SAS/OCCT/releases
3. Build OpenCASCADE following their instructions or use pre-built binaries

### Build Steps

```bash
# Clone or download the project
cd CADViewer

# Create build directory
mkdir build
cd build

# Configure CMake
cmake .. -DCMAKE_PREFIX_PATH="/path/to/Qt6;/path/to/OpenCASCADE"

# Build
cmake --build . --config Release

# Run
./bin/CADViewer  # On Linux/macOS
./bin/CADViewer.exe  # On Windows
```

### CMake Configuration Notes

If CMake cannot find OpenCASCADE or Qt6, set them explicitly:

```bash
cmake .. \
  -DCMAKE_PREFIX_PATH="/usr/local/lib/cmake/Qt6;/usr/local/lib/cmake/OpenCASCADE" \
  -DOpenCASCADE_DIR="/usr/local/lib/cmake/OpenCASCADE"
```

## Project Structure

```
CADViewer/
├── CMakeLists.txt                    # CMake configuration
├── README.md                          # This file
├── include/
│   ├── OcctQMainWindowSample.h       # Main window header
│   ├── OcctQWidgetViewer.h           # 3D viewer widget header
│   ├── OcctQtTools.h                 # Qt/OCCT integration tools
│   └── OcctGlTools.h                 # OpenGL tools header
├── src/
│   ├── main.cpp                       # Entry point
│   ├── OcctQMainWindowSample.cpp     # Main window implementation
│   ├── OcctQWidgetViewer.cpp         # 3D viewer implementation
│   ├── OcctQtTools.cpp               # Qt/OCCT tools implementation
│   └── OcctGlTools.cpp               # OpenGL tools implementation
└── build/
    └── bin/
        └── CADViewer                 # Compiled executable
```

## Usage

### Opening a CAD Model

1. **File Menu → Open CAD Model** or press **Ctrl+O**
2. Select a STEP, IGES, or BREP file
3. Wait for the model to load and display
4. Use mouse to interact with the model

### Navigation Controls

| Action | Control |
|--------|---------|
| Rotate | Left Mouse + Drag |
| Pan | Middle Mouse + Drag or Shift+Right Mouse |
| Zoom | Scroll Wheel or Right Mouse Drag (vertical) |
| Fit All | Press 'F' or View → Fit All |
| Reset View | View → Reset View |

### Menu Options

**File Menu:**
- Open CAD Model (Ctrl+O)
- Clear Models
- Exit (Ctrl+Q)

**View Menu:**
- Fit All (F)
- Reset View

**Help Menu:**
- About - Shows application and OpenGL information

## File Format Support

### STEP Format (.step, .stp)
- ISO 10303-21 standard
- Most comprehensive CAD format
- Supports assemblies, colors, metadata
- **Recommended for maximum compatibility**

### IGES Format (.iges, .igs)
- Initial Graphics Exchange Specification
- Widely used legacy format
- Good for geometry exchange
- May lose some advanced features

### BREP Format (.brep)
- OpenCASCADE native binary format
- Exact representation of shapes
- Best performance with OCCT
- Limited compatibility with other tools

## Configuration

### Mesh Quality

The application automatically generates meshes for visualization. Mesh quality is controlled by the linear deflection parameter (default: 0.05 mm). To adjust:

Edit `OcctQWidgetViewer.h`:
```cpp
double myMeshLinearDeflection = 0.05;  // Change this value
```

- Smaller values (0.01) = finer mesh, slower rendering
- Larger values (0.2) = coarser mesh, faster rendering

### Rendering Parameters

In `OcctQWidgetViewer.cpp`, the view is configured with:
```cpp
myView->ChangeRenderingParams().NbMsaaSamples = 4;        // Anti-aliasing
myView->ChangeRenderingParams().ToShowStats = true;       // FPS counter
```

## Troubleshooting

### CMake Cannot Find OpenCASCADE
```bash
# Find where OpenCASCADE is installed:
find /usr -name "OpenCASCADEConfig.cmake" 2>/dev/null

# Then pass the path to CMake:
cmake .. -DOpenCASCADE_DIR="/path/to/opencascade"
```

### CMake Cannot Find Qt6
```bash
# Find Qt6 installation:
find /usr -path "*/Qt6Config.cmake" 2>/dev/null

# Pass to CMake:
cmake .. -DCMAKE_PREFIX_PATH="/path/to/Qt6"
```

### Application Crashes on Startup
1. Verify OpenCASCADE is properly built with OpenGL support
2. Ensure graphics drivers are up to date
3. Check that the system supports OpenGL 3.3 or higher

### File Won't Load
1. Verify file format is correct (STEP/IGES/BREP)
2. Check file is not corrupted: try opening in FreeCAD or other CAD tool
3. Check application console for error messages
4. Ensure file path doesn't contain non-ASCII characters

### Poor Performance with Large Models
1. Increase mesh deflection value in code (coarser mesh)
2. Reduce MSAA samples: change `NbMsaaSamples = 4` to `2` or `0`
3. Upgrade GPU drivers
4. Close other applications to free RAM

## Development

### Adding New Features

See the comprehensive documentation in:
- `CAD-Viewer-Analysis.md` - Overall architecture
- `Phase1-Implementation.md` - CAD import details
- `Development-Summary.md` - Project roadmap
- `Code-Snippets-Reference.md` - Ready-to-use code examples

### Key Classes

| Class | Purpose |
|-------|---------|
| `OcctQMainWindowSample` | Main application window |
| `OcctQWidgetViewer` | 3D viewer widget with CAD import |
| `OcctQtTools` | Qt↔OCCT integration helpers |
| `OcctGlTools` | OpenGL context management |

### Build Targets

```bash
# Build in Release mode (recommended)
cmake --build . --config Release

# Build in Debug mode
cmake --build . --config Debug

# Clean build
cmake --build . --target clean
```

## License

This project is based on OCCT examples and is compatible with OCCT's licensing.

## References

- **OpenCASCADE Official**: https://dev.opencascade.org/
- **Qt Documentation**: https://doc.qt.io/
- **OCCT GitHub**: https://github.com/Open-Cascade-SAS/OCCT

## Support & Contributions

For issues, improvements, or contributions:
1. Review the comprehensive documentation provided
2. Check the Code-Snippets-Reference for implementation examples
3. Follow the architecture defined in Development-Summary.md

## Version History

**Version 1.0 (2025-12-30)**
- Initial release
- STEP, IGES, BREP file support
- Interactive 3D visualization
- File import with automatic format detection
- View controls and navigation
- Professional UI with menus and docks

---

**Status**: Production Ready  
**Last Updated**: December 30, 2025  
**Built for**: Robotic Systems Development & CAD Visualization
