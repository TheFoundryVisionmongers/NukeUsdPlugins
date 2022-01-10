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
 \brief USD axis plugin for axis node
 */

#include "UsdConverter/UsdAxisScenePlugin.h"

//DDImage includes
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/SceneGraph_KnobI.h>

// Library includes
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/xformCache.h>

namespace Foundry {
  namespace UsdConverter {
    PXR_NAMESPACE_USING_DIRECTIVE

    DD::Image::SceneItems UsdAxisReader::loadUsdPrims(const char* pFilename) const
    {
      UsdStageRefPtr stage = UsdStage::Open(pFilename);
      if (!stage) {
        return {};
      }

      DD::Image::SceneItems prims;
      for(const auto& prim : stage->Traverse()) {
        prims.emplace_back(prim.GetPath().GetString(),
                           prim.GetTypeName().GetString(),
                           isPrimSupported(prim));
      }
      return prims;
    }

    bool UsdAxisReader::isPrimSupported(const UsdPrim& prim) const
    {
      return prim.IsValid() && prim.IsA<UsdGeomXformable>();
    }

    void UsdAxisReader::knobs(DD::Image::Knob_Callback cb)
    {
      UsdSceneReaderBase::knobs(cb);
    }

    void UsdAxisReader::setupSceneGraph() const
    {
      _sceneGraphKnob->set_flag(DD::Image::Knob::SINGLE_SELECTION_ONLY);
    }

    DD::Image::SceneReaders::PluginDescription usdAxisDescription("Axis3", { "usd", "usda", "usdc", "usdz" }, DD::Image::SceneReaders::Constructor<UsdAxisReader>);
  }
}
