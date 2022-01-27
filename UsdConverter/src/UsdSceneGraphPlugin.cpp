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
 \brief plugin for querying the USD scene to display in the SceneGraphKnob
 */

#include <iomanip>

#include <DDImage/SceneGraphBrowserI.h>

#include "UsdConverter/UsdSceneGraphPlugin.h"
#include "UsdConverter/UsdUI.h"

namespace Foundry
{
  namespace UsdConverter
  {
    UsdSceneGraphPlugin::UsdSceneGraphPlugin() : DD::Image::SceneReaderPlugin() {};
    UsdSceneGraphPlugin::~UsdSceneGraphPlugin() {}

    /// check if the file can be used by this plugin
    bool UsdSceneGraphPlugin::isValid(const std::string& filename) { return true; }

    bool UsdSceneGraphPlugin::query(std::istream& in, std::ostream &out) const {
      std::string command;
      in >> std::quoted(command);
      if(command == DD::Image::SceneGraph::kSceneGraphKnobCommand) {
        return Foundry::UsdConverter::QueryPrimitives(in, out);
      }
      else if(command == DD::Image::SceneGraph::kSceneGraphTypeCommand) {
        std::string opType;
        in >> std::quoted(opType);
        const auto& it = supportedPrimTypes.find(opType);
        if(it == supportedPrimTypes.end()) {
          return false;
        }
        out << std::quoted(it->second);
        return true;
      }
      return false;
    }
  }
}
