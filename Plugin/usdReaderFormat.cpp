// Copyright 2021 Foundry
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
 \brief Implementation file for usdReaderFormat class
 */

#include "usdReaderFormat.h"

#include <DDImage/SceneGraph_KnobI.h>

using namespace DD::Image;

const std::string usdReaderFormat::kReadOnEachFrameKnobName =
    "read_on_each_frame";
const std::string usdReaderFormat::kAllObjectsKnobName = "all_objects";
const std::string usdReaderFormat::kNodeKnobName =
    DD::Image::kSceneGraphKnobName;

void usdReaderFormat::append(Hash& hash)
{
  hash.append(_readOnEachFrame);
  hash.append(_nodeNameIndex);
}

void usdReaderFormat::knobs(Knob_Callback f)
{
  Bool_knob(f, &_readOnEachFrame, kReadOnEachFrameKnobName.c_str(),
            "read on each frame");
  SetFlags(f, Knob::EARLY_STORE);
  Tooltip(f,
          "Activate this to read the objects on each frame. This should be "
          "activated for animated objects.");
}

void usdReaderFormat::extraKnobs(Knob_Callback f)
{
  Tab_knob(f, "Scenegraph");

  Knob* pNodeNameKnob =
      SceneGraph_knob(f, &_nodeNameIndex, nullptr,
                      kNodeKnobName.c_str(), "");
  if(pNodeNameKnob != nullptr) {
    SetFlags(f, Knob::SAVE_MENU | Knob::EARLY_STORE | Knob::ALWAYS_SAVE);
    Tooltip(f, "USD primitive paths");
  }

  Newline(f);
  Bool_knob(f, &_allObjects, kAllObjectsKnobName.c_str(),
            "view entire scenegraph");
  SetFlags(f, Knob::EARLY_STORE);
  Tooltip(
      f,
      "When unchecked, only items imported into this node will be shown. When "
      "checked, all items in the scenegraph will be displayed, allowing the "
      "user to add to or remove from the imported items list.");
}
