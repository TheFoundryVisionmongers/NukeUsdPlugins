# Copyright 2021 Foundry
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

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(UsdPlugins)

find_package(Nuke REQUIRED)
find_package(pxr REQUIRED)
find_package(Boost REQUIRED)

#===------------------------------------------------------------------------===
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

#===------------------------------------------------------------------------===
# Unit testing set up
option(WITH_UNITTESTS "Enable unit test building" OFF)

if (WITH_UNITTESTS)
  find_package(Catch2 REQUIRED)
  find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
  add_library(CatchTestMain OBJECT CatchTestMain.cpp)
  add_library(Nuke::CatchTestMain ALIAS CatchTestMain)
  target_link_libraries(CatchTestMain PUBLIC Qt5::Gui Catch2::Catch2)

  function(add_nuke_unittest TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN})
    target_link_libraries(${TARGET_NAME} PRIVATE Nuke::CatchTestMain)
    add_custom_command(
      TARGET ${TARGET_NAME} POST_BUILD
      COMMAND $<TARGET_FILE:${TARGET_NAME}>
    )
  endfunction()
endif()

#===------------------------------------------------------------------------===
macro(add_nuke_plugin PLUGIN_NAME)
  add_library(${PLUGIN_NAME} MODULE ${ARGN})
  add_library(NukePlugins::${PLUGIN_NAME} ALIAS ${PLUGIN_NAME})
  target_link_libraries(${PLUGIN_NAME} PRIVATE Nuke::NDK)
  set_target_properties(${PLUGIN_NAME}
    PROPERTIES
      FOLDER "Plugins"
      PREFIX "")
  if (APPLE)
    set_target_properties(${PLUGIN_NAME} PROPERTIES SUFFIX ".dylib")
  endif()
  if (UNIX AND NOT APPLE)
    target_link_options(${PLUGIN_NAME} PRIVATE -Wl,--exclude-libs,ALL)
  endif()
endmacro()

#===------------------------------------------------------------------------===
add_subdirectory(UsdConverter)
add_subdirectory(Plugin)
