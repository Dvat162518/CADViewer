# CAD Model Viewer

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square)](https://cplusplus.com/)
[![Qt6](https://img.shields.io/badge/Qt-6.0+-green?style=flat-square)](https://www.qt.io/)
[![OpenCASCADE](https://img.shields.io/badge/OpenCASCADE-7.7+-orange?style=flat-square)](https://www.opencascade.com/)
[![CMake](https://img.shields.io/badge/CMake-3.16+-red?style=flat-square)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow?style=flat-square)](#license)

[![Repository status](https://repology.org/badge/repository-big/ubuntu_22_04.svg)](https://repology.org/repository/ubuntu_22_04)

A **professional-grade 3D CAD model viewer** built with **Qt6** and **OpenCASCADE (OCCT)**, featuring advanced geometric measurement, interactive selection, and comprehensive data export capabilities.

## 🎯 Features

### Core Functionality
- **Multi-Format Support**: Load STEP (.step, .stp), IGES (.iges, .igs), and BREP (.brep) CAD files
- **Interactive 3D Visualization**: High-quality OpenGL rendering with 4x MSAA anti-aliasing
- **Real-time Measurements**: Calculate area, volume, length, diameter, radius, and angle properties
- **Face/Edge Selection**: Click-based selection with multi-select support (CTRL+Click)
- **Selection Locking**: Prevent accidental deselection during measurement operations

### Advanced Features
- **Path Analysis**: Automatic edge chaining and discretization with 3D point labeling
- **Model Origin Tracking**: Visual XYZ trihedron at the calculated center of mass
- **Geometric Properties**: Surface properties, center of mass calculation, curve analysis
- **ViewCube Navigation**: Interactive 3D navigation aid for intuitive model manipulation
- **High-DPI Support**: Automatic scaling for modern high-resolution displays

### Data Export
- **CSV Export**: Save path data in standard CSV format with proper escaping
- **PDF Reports**: Generate professional inspection reports with metadata and data tables
- **Structured Output**: Automatic metadata inclusion (filename, origin coordinates, measurements)

### User Interface
- **Modular Dock Widgets**: Resizable panels for properties, measurements, and tools
- **Real-time UI Updates**: Synchronized data display across all panels
- **Keyboard Shortcuts**:
  - `Ctrl+O`: Open CAD file
  - `F`: Fit view to model
  - `ESC`: Clear selection/shapes (unless locked)
  - `Ctrl+Q`: Exit application

## 📋 System Requirements

### Minimum Requirements
- **OS**: Linux (Ubuntu 20.04+), macOS 10.15+, or Windows 10+
- **CPU**: Dual-core 2.0 GHz processor
- **RAM**: 4 GB minimum
- **GPU**: OpenGL 4.5 capable graphics card

### Build Requirements
- **C++ Compiler**: GCC 9+ / Clang 10+ / MSVC 2019+
- **CMake**: 3.16 or higher
- **Qt6**: Core, Gui, Widgets, OpenGL, OpenGLWidgets, PrintSupport modules
- **OpenCASCADE**: Version 7.7 or higher

## 🔧 Installation

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    libqt6opengl6 \
    libqt6printsupport6 \
    qt6-base-dev \
    libocct-dev

# Clone repository
git clone https://github.com/yourusername/CADViewer.git
cd CADViewer

# Create build directory
mkdir -p build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run application
./bin/CADViewer
```

### macOS

```bash
# Install dependencies using Homebrew
brew install qt@6 opencascade cmake

# Clone and build
git clone https://github.com/yourusername/CADViewer.git
cd CADViewer

mkdir -p build && cd build
cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6) ..
make -j$(sysctl -n hw.ncpu)

./bin/CADViewer
```

### Windows (MSVC)

```bash
# Install Qt6 and OpenCASCADE via official installers
# Configure OCCT path in CMakeLists.txt:
# set(OpenCASCADE_DIR "C:/OCC/lib/cmake/opencascade")

git clone https://github.com/yourusername/CADViewer.git
cd CADViewer

mkdir build && cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release -j 8

bin\CADViewer.exe
```

### Custom OpenCASCADE Installation Path

If OpenCASCADE is installed in a non-standard location, update `CMakeLists.txt`:

```cmake
set(OpenCASCADE_DIR "/path/to/occt/lib/cmake/opencascade")
```

## 🚀 Usage

### Basic Workflow

1. **Open CAD File**
   - Click `File → Open` or press `Ctrl+O`
   - Select STEP, IGES, or BREP file
   - Model loads automatically and fits to view

2. **Navigate Model**
   - **Rotate**: Right-click + drag
   - **Pan**: Middle-click + drag
   - **Zoom**: Mouse wheel or pinch
   - **Reset View**: Click `View → Fit All` or press `F`

3. **Select Features**
   - **Single Select**: Left-click on face or edge
   - **Multi-Select**: Ctrl+Click to add to selection
   - **Clear Selection**: Press ESC (unless locked) or click empty space

4. **Lock Selection**
   - Enable `Show Model Origin` checkbox in CAD Tools dock
   - Check `Lock Selection` to prevent accidental clearing during analysis

5. **Export Data**
   - Select edges/faces to generate path data
   - Go to `Save → Export As → CSV` or `PDF`
   - Choose output location and filename
   - File automatically saved with proper format

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+O` | Open CAD file |
| `F` | Fit view to model |
| `ESC` | Clear selection |
| `Ctrl+Q` | Exit application |
| `Ctrl+Click` | Toggle selection (multi-select) |

## 📊 Data Export Formats

### CSV Format
```csv
Report Property,Value
Date,2026-01-02
Filename,model.step
Origin X,50.123456
Origin Y,75.234567
Origin Z,100.345678

ID,X,Y,Z,Distance,Curve Data
P1,0.00,0.00,0.00,-,R:5.0 / A:90°
P2,10.00,0.00,0.00,10.00,-
P3,10.00,10.00,0.00,10.00,R:5.0 / A:45°
```

### PDF Report
- Title and metadata section with filename and export date
- Model origin coordinates
- Tabular data with automatic pagination
- Professional formatting suitable for documentation

## 🏗️ Architecture

### Component Structure

```
OcctQWidgetViewer (Core)
├── CadModelManager         → File I/O & format handling
├── RenderManager           → OpenGL rendering & visualization
├── MeasurementManager      → Geometric calculations & analysis
├── InputManager            → Keyboard & mouse event handling
└── EventManager            → Qt event processing

OcctQMainWindowSample      → Main window UI & menus
├── Dock Widgets (3x)
│   ├── Description Dock     → File properties & path data
│   ├── Model Data Dock      → Selection measurements
│   └── Tools Dock           → Selection lock & origin toggle
└── Menu Bar
    ├── File Menu           → Open, Save, Clear, Exit
    ├── Save Menu           → Export As (CSV/PDF)
    ├── View Menu           → Fit All
    └── Windows Menu        → Toggle dock visibility
```

### Key Classes

| Class | Responsibility |
|-------|-----------------|
| `OcctQWidgetViewer` | Central viewer controller, OCCT stack management |
| `CadModelManager` | CAD file loading (STEP, IGES, BREP) |
| `RenderManager` | 3D rendering, shape display, origin visualization |
| `MeasurementManager` | Geometric property calculation, edge chaining |
| `InputManager` | Mouse/keyboard input, selection handling |
| `EventManager` | Qt event routing, touch input |
| `OcctQtTools` | Qt ↔ OCCT conversion utilities |
| `OcctGlTools` | OpenGL context management |

## 🔄 Data Flow

```
User Action (Click/Select)
    ↓
InputManager → Selection Update
    ↓
calculateMeasurements()
    ↓
MeasurementManager → Geometric Analysis
    ↓
displayModelOrigin() → Visual Update
    ↓
measurementsUpdated Signal
    ↓
OcctQMainWindowSample → UI Update (Tables)
    ↓
User Sees Results
```

## 🎨 UI Components

### Dock Widgets

**Description Dock**
- Info Tab: File metadata (filename, location, size)
- Path Data Tab: Point coordinates, distances, curve properties

**Model/Selection Data Dock**
- Origin coordinates (X, Y, Z)
- Selection type (FACE/EDGE/SOLID)
- Calculated properties (Area, Volume, Length, Diameter, Radius, Angle)

**CAD Tools Dock**
- Lock Selection: Prevent accidental deselection
- Show Model Origin: Toggle XYZ trihedron visibility

## ⚙️ Configuration

### Adjustable Parameters

Edit the following in source code for customization:

```cpp
// Measurement.cpp - Point cloud discretization
GCPnts_QuasiUniformDeflection discretizer(adaptor, 0.005); // Change deflection

// Render.cpp - Anti-aliasing samples
myView->ChangeRenderingParams().NbMsaaSamples = 4; // Adjust MSAA

// Render.cpp - Origin visual scale
aDrawer->DatumAspect()->SetAttribute(Prs3d_DA_XAxisLength, 20.0); // Change size

// Core.cpp - Mesh quality
myMesher = new BRepMesh_IncrementalMesh(theShape, 0.05); // Adjust deflection
```

## 🐛 Troubleshooting

### Linux: BadWindow Error on Startup
**Solution**: Update `Core.cpp` - OCCT initialization is deferred until first paint event.

### macOS: "Falling back to unshared context" Warning
**Solution**: OcctQtTools sets `Qt::AA_UseOpenGLES` and forces CoreProfile - this is expected.

### Windows: ANGLE OpenGL Incompatibility
**Solution**: `OcctQtTools::qtGlPlatformSetup()` forces `Qt::AA_UseDesktopOpenGL`.

### Export: CSV opens as single column in Excel
**Solution**: Use "Text to Columns" feature in Excel or import with comma delimiter.

### Selection disappears after clicking empty space
**Solution**: Enable "Lock Selection" checkbox in CAD Tools dock.

## 📦 Dependencies

### Build-Time
- CMake ≥ 3.16
- Qt6 (Core, Gui, Widgets, OpenGL, OpenGLWidgets, PrintSupport)
- OpenCASCADE ≥ 7.7
- C++17 compatible compiler

### Runtime
- OpenGL 4.5+ graphics driver
- Qt6 runtime libraries

## 🔐 Platform-Specific Notes

### Linux
- Requires Xlib support (Wayland not directly supported by OCCT)
- `OcctQtTools::qtGlPlatformSetup()` forces XCB platform plugin
- GPU: Tested with NVIDIA and Intel integrated graphics

### macOS
- Requires CoreProfile OpenGL context
- High-DPI display support enabled
- GPU: Tested with Apple Silicon and Intel GPUs

### Windows
- Uses desktop OpenGL, not ANGLE
- HighDPI scaling enabled
- NVIDIA Optimus: Uncomment symbols in `OcctGlTools.cpp` to force discrete GPU

## 📄 File Format Specifications

### STEP (IGES-based)
- Supported: .step, .stp
- Use Case: Industry-standard CAD interchange
- Features: Solids, surfaces, edges, curves fully supported

### IGES
- Supported: .iges, .igs
- Use Case: Legacy CAD file format
- Features: Limited compared to STEP, but widely compatible

### BREP
- Supported: .brep
- Use Case: OpenCASCADE native format
- Features: Full topology and geometry support

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/YourFeature`)
3. Commit changes (`git commit -m 'Add YourFeature'`)
4. Push to branch (`git push origin feature/YourFeature`)
5. Open a Pull Request

### Code Style
- Follow Qt naming conventions (camelCase for functions, PascalCase for classes)
- Use OCCT naming patterns for geometry operations
- Add comments for non-obvious logic
- Ensure cross-platform compatibility

## 📝 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

```
MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

## 📚 Documentation

### API Documentation
- Generated with Doxygen (optional)
- Class documentation available in header files
- Inline comments describe complex algorithms

### Tutorials
- [Getting Started Guide](docs/GETTING_STARTED.md) (optional)
- [CAD File Format Guide](docs/FILE_FORMATS.md) (optional)
- [Measurement System Guide](docs/MEASUREMENTS.md) (optional)

## 🐞 Bug Reports & Feature Requests

Report issues on [GitHub Issues](https://github.com/yourusername/CADViewer/issues)

**Include**:
- OS and version
- Qt6 and OpenCASCADE versions
- Steps to reproduce
- Screenshots/CAD files (if applicable)

## 📞 Support

For questions and support:
- GitHub Issues: Feature requests and bug reports
- Discussions: General questions and tips
- Wiki: Community-contributed documentation

## 🙏 Acknowledgments

- **Qt Community**: For excellent Qt6 framework
- **OpenCASCADE**: For robust geometry kernel
- **Contributors**: Thank you for improvements and bug fixes

## 📊 Statistics

- **Lines of Code**: ~3,500
- **Classes**: 8 core + utilities
- **Supported Formats**: 3 (STEP, IGES, BREP)
- **Export Formats**: 2 (CSV, PDF)
- **Platform Support**: Linux, macOS, Windows

## 🔮 Future Enhancements

- [ ] Assembly/multi-part file support
- [ ] Point cloud visualization (PLY, XYZ formats)
- [ ] Cross-section plane generation
- [ ] Measurement history and comparison
- [ ] Dark/Light theme support
- [ ] Real-time collaborative viewing (WebGL)
- [ ] Python scripting interface
- [ ] Advanced rendering (ray tracing, photogrammetry)

## 📅 Version History

### v1.0.0 (Current)
- Initial release
- STEP/IGES/BREP support
- Interactive selection and measurement
- CSV and PDF export
- Origin visualization toggle

---

**Made with ❤️ for CAD professionals and engineers worldwide.**
