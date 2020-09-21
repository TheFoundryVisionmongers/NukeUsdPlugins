# NukeUsdPlugins

This project implements a USD reader plugin for Nuke's ReadGeo node.

## Building
The Nuke USD plug-in has the following dependencies:
- Compiler requirements are as for all Nuke NDK plugins.
- Nuke (12.2v1 onwards)
- USD (https://github.com/PixarAnimationStudios/USD/releases/tag/v19.11) (19.11 onwards)
- Boost (https://boost.org) (header only, 1.66.0 onwards)
- CMake (https://cmake.org/documentation/) (3.13 onwards)
- [Recommended build tool] Ninja (https://ninja-build.org/) (1.8.2 onwards)
- [Only if building with unit tests] Catch2 (https://github.com/catchorg/Catch2)
  - The unit tests are disabled by default. Enable them by setting the CMake option WITH_UNITTESTS=ON.

We use the CMake find_package mechanism to set up dependencies.
The USD package is called pxr. Set the pxr_DIR CMake variable to
point CMake to your USD installation directory.
Set BOOST_ROOT to point at your Boost installation.

Example build command using the Ninja generator:

```
cmake \
-G Ninja
-D Nuke_DIR=<YOUR_NUKE_INSTALL_ROOT> \
-D pxr_DIR=<PATH_TO_USD_DIRECTORY> \
-D BOOST_ROOT=<PATH_TO_BOOST_DIRECTORY> \
-D CMAKE_INSTALL_PREFIX=<YOUR_INSTALL_PREFIX> \
-D CMAKE_BUILD_TYPE=Release \
<USD_PLUGIN_SOURCE_DIRECTORY>

ninja install
```

To install, copy the usdReader plugin and the UsdConverter shared library
from CMAKE_INSTALL_PREFIX to your .nuke folder.

## Project structure

### Plugin subdirectory

This subdirectory contains the usdReader plugin sources, a specialization of the Nuke GeoReader class. The usdReader registers the USD file types so they are recognized by the ReadGeo node and creates knobs for setting loading options like the scene graph for choosing which primitives to load. The plugin does not contain any logic for converting USD geometry to Nuke's internal format. Instead it links to the UsdConverter shared library which encapsulates that functionality.

### UsdConverter subdirectory

This subdirectory contains the UsdConverter shared library which converts USD to Nuke geometry. The API offers functions at various levels of abstraction, e.g. the usdReader plugin uses the high level function loadUsd() to add the geometry from a USD file into a Nuke GeometryList. It is divided into functions dealing with geometry (Mesh, Points, Cube, PointInstancer), geometry attributes (points, vertices, colors, ...) and scene graph knob initialization. Public API functions are marked wih
the FN_USDCONVERTER_API export macro, everything else is hidden and used only internally.
