# Copyright 2020 Foundry
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

#===------------------------------------------------------------------------===
# Create object library so we can unittest internal functions
add_library( UsdConverterObjectlib OBJECT
    src/UsdCommon.cpp
    src/UsdGeoConverter.cpp
    src/UsdAttrConverter.cpp
    src/UsdUI.cpp )

target_include_directories( UsdConverterObjectlib PUBLIC include )

target_compile_definitions( UsdConverterObjectlib
  PRIVATE
    FN_USDCONVERTER_SHARED_LIB
    FN_USDCONVERTER_EXPORTS=1
  PUBLIC
    BOOST_PYTHON_STATIC_LIB )

set_target_properties( UsdConverterObjectlib PROPERTIES POSITION_INDEPENDENT_CODE ON )

if ( WIN32 )
  target_compile_definitions( UsdConverterObjectlib
    PUBLIC
      NOMINMAX )
endif()

target_link_libraries( UsdConverterObjectlib
  PUBLIC
    Nuke::NDK
    tf gf vt ar sdf usd usdGeom )

#===------------------------------------------------------------------------===
# The shared library we bundle

# Need at least one source file or some generators throw an error
set( EMPTY_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp )
if( NOT EXISTS ${EMPTY_FILE_PATH} )
  file( WRITE ${EMPTY_FILE_PATH} "" )
endif()
add_library( UsdConverter SHARED ${EMPTY_FILE_PATH} )
unset( EMPTY_FILE_PATH )

add_library( Nuke::UsdConverter ALIAS UsdConverter )

target_link_libraries( UsdConverter
  PUBLIC
    UsdConverterObjectlib
    Nuke::NDK )

install( TARGETS UsdConverter DESTINATION . )

add_nuke_plugin( usdSceneReader
  src/UsdCommon.cpp
  src/UsdSceneReader.cpp
  src/UsdSceneGraphPlugin.cpp
  src/UsdAxisScenePlugin.cpp
  src/UsdCameraScenePlugin.cpp
  src/UsdLightScenePlugin.cpp )
target_include_directories( usdSceneReader PUBLIC include )
target_link_libraries(usdSceneReader
  PRIVATE
    Nuke::UsdConverter
    Nuke::NDK
    usd
    usdGeom
    usdLux
)

install( TARGETS usdSceneReader DESTINATION . )

if ( WITH_UNITTESTS )
 add_subdirectory( unit_tests )
endif()
