// Copyright 2020 Foundry
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.

/*! \file
 \brief Implementation file for usdReader class
 */

#include "usdReader.h"

#include <DDImage/Application.h>
#include <DDImage/File_KnobI.h>
#include <DDImage/NodeI.h>
#include <DDImage/SceneGraph_KnobI.h>
#include <UsdConverter/UsdGeoConverter.h>
#include <UsdConverter/UsdUI.h>

#include "usdReaderFormat.h"

using namespace DD::Image;

usdReader::usdReader(ReadGeo* geo) : GeoReader(geo)
{
}

usdReaderFormat* usdReader::getFormat()
{
  usdReaderFormat* format = dynamic_cast<usdReaderFormat*>(geo->handler());
  assert(format);
  return format;
}

const usdReaderFormat* usdReader::getFormat() const
{
  usdReaderFormat* format = dynamic_cast<usdReaderFormat*>(geo->handler());
  assert(format);
  return format;
}

SceneGraph_KnobI* usdReader::getSceneGraphKnob()
{
  Knob* pNodeNameKnob = geo->knob(usdReaderFormat::kNodeKnobName.c_str());
  assert(pNodeNameKnob);
  return pNodeNameKnob->sceneGraphKnob();
}

void usdReader::get_geometry_hash(Hash* geo_hash)
{
  // Rebuild primitives on change of filename, current frame (conditionally) etc.
  append(geo_hash[Group_Primitives]);
}

void usdReader::geometry_engine(Scene&, GeometryList& out)
{
  const auto frame = geo->outputContext().frame();

  usdReaderFormat* pfmt = getFormat();

  SceneGraph_KnobI* pSceneGraphKnob = getSceneGraphKnob();

  // Retrieve from scene graph knob which prims the user wants to load
  std::vector<std::string> selectedPaths = pSceneGraphKnob->getSelectedItems();

  // Time at which to load geometry: either Nuke frame or the USD default frame, depending on knob setting
  const pxr::UsdTimeCode time = pfmt->_readOnEachFrame
                                    ? pxr::UsdTimeCode(frame)
                                    : pxr::UsdTimeCode::Default();

  if(geo->rebuild(Mask_Primitives)) {
    // Destroy old geometry and retrieve from file at desired time
    out.delete_objects();
    geo->set_rebuild(Mask_Points | Mask_Attributes);
    Foundry::UsdConverter::loadUsd(out, filename(), selectedPaths, time);
  }
}

int usdReader::knob_changed(Knob* k)
{
  const usdReaderFormat* pfmt = getFormat();
  if(k->is(ReadGeo::kFileKnobName)) {
    // Open USD file and fill scene graph, may open pop up window for prim selection. Empty scene graph on error
    File_KnobI* fileKnobInterface = k->fileKnob();
    bool resetSelected = fileKnobInterface->getLastChangeContext() ==
                         File_KnobI::ChangedFromUser;
    bool showBrowser = Application::IsGUIActive() && resetSelected;

    const char* readGeoFilename = k->get_text(&geo->uiContext());
    if(readGeoFilename != nullptr && strlen(readGeoFilename) > 0) {
      if(!Foundry::UsdConverter::GetSceneGraphData(getSceneGraphKnob(),
                                                   readGeoFilename, showBrowser,
                                                   resetSelected)) {
        geo->internalError("No such file or directory or invalid file");
        getSceneGraphKnob()->clear();
        return 1;
      }
    }
    else {
      SceneGraph_KnobI* pSceneGraphKnob = getSceneGraphKnob();
      if(pSceneGraphKnob) {
        pSceneGraphKnob->clear();
        return 1;
      }
    }

    if(!resetSelected) {
      // pfmt probably hasn't stored the value of this knob yet
      getSceneGraphKnob()->viewAllNodes(
          geo->knob(pfmt->kAllObjectsKnobName.c_str())->get_value());
    }
  }
  else if(k->is(ReadGeo::kReloadKnobName)) {
    // Reload USD file without popping up scene graph browser window
    if(!Foundry::UsdConverter::GetSceneGraphData(getSceneGraphKnob(),
                                                 filename(), false, false)) {
      geo->internalError("No such file or directory or invalid file");
      getSceneGraphKnob()->clear();
      return 1;
    }
    getSceneGraphKnob()->viewAllNodes(
        geo->knob(pfmt->kAllObjectsKnobName.c_str())->get_value());
  }
  else if(k->is(DD::Image::kSceneGraphKnobName)) {
    // React to scene graph knob user input
    geo->set_rebuild(Mask_Primitives);
    geo->invalidate();
  }
  else if(k->is(pfmt->kAllObjectsKnobName.c_str())) {
    // Enable/disable showing all objects in scene graph
    SceneGraph_KnobI* pSceneGraphKnob = getSceneGraphKnob();
    pSceneGraphKnob->viewAllNodes(pfmt->_allObjects);
    return 1;
  }

  return 0;
}

void usdReader::_validate(const bool /* unused */)
{
}

void usdReader::append(Hash& newHash)
{
  const usdReaderFormat* pfmt = getFormat();

  // Append current frame to the hash
  if(pfmt->_readOnEachFrame == true) {
    float frame = static_cast<float>(geo->outputContext().frame());
    newHash.append(frame);
  }

  // Append current filename to the hash
  Knob* pFileNameKnob = geo->knob(ReadGeo::kFileKnobName);
  assert(pFileNameKnob);
  const char* pFilename = pFileNameKnob->get_text(&geo->uiContext());
  newHash.append(pFilename);

  // Append all items selected in the scene graph knob to hash
  SceneGraph_KnobI* pSceneGraphKnob = getSceneGraphKnob();
  const auto selectedNodes = pSceneGraphKnob->getSelectedItems();
  for(const auto& node : selectedNodes) {
    newHash.append(node);
  }
}

namespace
{
  static GeoReader* build_reader(ReadGeo* filereader, int /* unused */,
                                 const unsigned char* /* unused */, int /* unused */)
  {
    return new usdReader(filereader);
  }

  static GeoReaderFormat* build_format(ReadGeo* /* unused */)
  {
    return new usdReaderFormat();
  }

  // Register all USD file types and associate the format object with the custom knobs with them
  bool is_usd_file(int /* unused */, const unsigned char* /* unused */, int /* unused */)
  {
    return true;
  }
  const GeoDescription description("usd\0usda\0usdc\0usdz\0", build_reader,
                                   build_format, is_usd_file, nullptr, false);
}  // namespace
