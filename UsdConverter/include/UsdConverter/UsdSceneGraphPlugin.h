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

#ifndef USDSCENEGRAPHPLUGIN_H
#define USDSCENEGRAPHPLUGIN_H

#include <DDImage/SceneReaderPlugin.h>
#include <DDImage/SceneGraphBrowserI.h>

namespace Foundry
{
  namespace UsdConverter
  {
    /// plugin class for the Nuke SceneGraph
    class UsdSceneGraphPlugin : public DD::Image::SceneReaderPlugin {
    public:
      UsdSceneGraphPlugin();
      ~UsdSceneGraphPlugin() override;

      /// check if the file can be used by this plugin
      bool isValid(const std::string& filename) override;
      /// return a list of primitives
      bool query(std::istream& in, std::ostream &out) const override;
    };
    DD::Image::SceneReaders::PluginDescription usdSceneGraphPlugin(DD::Image::SceneGraph::kSceneGraphPluginClass, { "usd", "usda", "usdc", "usdz" }, DD::Image::SceneReaders::Constructor<UsdSceneGraphPlugin>);
  }
}

#endif
