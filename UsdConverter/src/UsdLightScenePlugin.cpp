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
 \brief USD Light plugin for light node
 */

#include "UsdConverter/UsdLightScenePlugin.h"

//DDImage includes
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/SceneGraph_KnobI.h>

// Library includes
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/sphereLight.h>

//"Light" tab knobs
static const std::string kLightTypeKnobName = "light_type";
static const std::string kColorKnobName = "color";
static const std::string kIntensityKnobName = "intensity";
static const std::string kFalloffTypeKnobName = "falloff_type";
static const std::string kConeAngle = "cone_angle";
static const std::string kConeFalloff = "cone_falloff";
static const std::string kConePenumbraAngle = "cone_penumbra_angle";

static constexpr int kPointLightType = 0;
static constexpr int kDirectionalLightType = 1;
static constexpr int kSpotLightType = 2;

namespace Foundry {
  namespace UsdConverter {
    PXR_NAMESPACE_USING_DIRECTIVE

    void UsdLightReader::knobs(DD::Image::Knob_Callback cb)
    {
      UsdSceneReaderBase::knobs(cb);
    }

    std::vector<DD::Image::Knob*> UsdLightReader::getFileDependentKnobs(DD::Image::SceneReader* reader)
    {
      auto knobs = UsdSceneReaderBase::getFileDependentKnobs(reader);
      const auto op = dynamic_cast<DD::Image::Op*>(reader);
      const auto lightType = static_cast<int>(op->knob(kLightTypeKnobName.c_str())->get_value());
      switch(lightType) {
      case kSpotLightType:
        knobs.emplace_back(op->knob(kConePenumbraAngle.c_str()));
      case kPointLightType:
        knobs.emplace_back(op->knob(kFalloffTypeKnobName.c_str()));
      default:
        break;
      }
      return knobs;
    }

    void UsdLightReader::setCustomConstantAttributes(const UsdPrim& prim, DD::Image::Op& op) const
    {
      if(isSpotLight(prim)) {
        setKnobValue(op, kLightTypeKnobName, kSpotLightType, "Can not initialize light node");
      }
      else if(prim.IsA<UsdLuxSphereLight>()) {
        setKnobValue(op, kLightTypeKnobName, kPointLightType, "Can not initialize light node");
      }
      else {
        setKnobValue(op, kLightTypeKnobName, kDirectionalLightType, "Can not initialize light node");
      }
      if(DD::Image::Knob* knob = op.knob(kLightTypeKnobName.c_str())) {
        knob->changed();
      }
    }

    void UsdLightReader::setCustomKnobsAsAnimated(const UsdPrim& prim, const DD::Image::Op& op) const
    {
      setKnobIsAnimated(op, kIntensityKnobName, 1);
      setKnobIsAnimated(op, kColorKnobName, 3);

      if (isSpotLight(prim)) {
        setKnobIsAnimated(op, kConeAngle, 1);
        setKnobIsAnimated(op, kConeFalloff, 1);
      }
    }

    void UsdLightReader::clearCustomAnimation(const UsdPrim& /* prim */, const DD::Image::Op& op) const
    {
      for(const auto& knobName : { kColorKnobName, kIntensityKnobName, kConeAngle, kConeFalloff }) {
        clearKnobAnimated(op, knobName);
      }
    }

    void UsdLightReader::setCustomAnimationAttributes(const UsdPrim& prim, DD::Image::Op & op, float time) const
    {
      const UsdAttribute intensity = GetLightIntensityAttribute(prim);
      if (!setIntensity(intensity, op, time)) {
        op.error("Can not initialize intensity_scaled knob");
        return;
      }

      const UsdAttribute color = GetLightColorAttribute(prim);
      if (!setColor(color, op, time)) {
        op.error("Can not initialize color knob");
        return;
      }

      if (isSpotLight(prim)) {
        UsdLuxShapingAPI lightConeData(prim);
        setLightCone(lightConeData, op, time);
      }
    }

    bool UsdLightReader::isPrimSupported(const UsdPrim& prim) const
    {
      return prim.IsValid() && prim.IsA<UsdLuxLight>();
    }

    DD::Image::SceneItems UsdLightReader::loadUsdPrims(const char* pFilename) const
    {
      using namespace std;
      UsdStageRefPtr stage = UsdStage::Open(pFilename);
      if (!stage) {
        return {};
      }

      DD::Image::SceneItems prims;
      const auto range = UsdPrimRange(stage->GetPseudoRoot());
      for (const auto& prim : range) {
        if (isPrimSupported(prim)) {
          const bool enabled = prim.IsA< UsdLuxDistantLight>() || prim.IsA< UsdLuxSphereLight>();
          prims.emplace_back(prim.GetPath().GetString(), prim.GetTypeName().GetString(), enabled);
        }
      }
      return prims;
    }

    bool UsdLightReader::setIntensity(const PXR_NS::UsdAttribute& attribute, DD::Image::Op & op, float time) const
    {
      float intensityValue;
      return (getUsdAttrib(intensityValue, attribute, time, op)
              && setKnobValueAt(op, kIntensityKnobName, intensityValue, time));
    }

    bool UsdLightReader::setColor(const PXR_NS::UsdAttribute& attribute, DD::Image::Op & op, float time) const
    {
      GfVec3f colorValue;
      if(!getUsdAttrib(colorValue, attribute, time, op)) {
        return false;
      }

      DD::Image::Knob* pKnob = op.knob(kColorKnobName.c_str());
      if (!pKnob) {
        return false;
      }

      for(int i = 0; i < 3; i++) {
        pKnob->set_value_at(colorValue[i], time, i);
      }

      return true;
    }

    void UsdLightReader::setLightCone(const PXR_NS::UsdLuxShapingAPI& prim, DD::Image::Op & op, float time) const
    {
      float coneAngle;
      float coneSoftness;
      if(   !getUsdAttrib(coneAngle,    prim.GetShapingConeAngleAttr(),    time, op, "Can not initialize cone angle")
         || !getUsdAttrib(coneSoftness, prim.GetShapingConeSoftnessAttr(), time, op, "Can not initialize cone falloff")) {
        return;
      }
      setKnobValueAt(op, kConeAngle,   coneAngle,    time, "Can not initialize cone angle");
      setKnobValueAt(op, kConeFalloff, coneSoftness, time, "Can not initialize cone falloff");
    }

    bool UsdLightReader::isSpotLight(const PXR_NS::UsdPrim& prim) const
    {
      return prim.HasAPI<UsdLuxShapingAPI>() && prim.IsA<UsdLuxSphereLight>();
    }

    UsdAttribute UsdLightReader::GetLightColorAttribute(const UsdPrim& prim) const
    {
      static const TfToken oldAttributeName("color");

      const UsdLuxLight light(prim);
      UsdAttribute colorAttribute = light.GetColorAttr();
      const UsdResolveInfoSource source = colorAttribute.GetResolveInfo().GetSource();
      const bool isDefaultValue = source == UsdResolveInfoSourceFallback;

      if (isDefaultValue && prim.HasAttribute(oldAttributeName)) {
        colorAttribute = prim.GetAttribute(oldAttributeName);
      }
      return colorAttribute;
    }

    UsdAttribute UsdLightReader::GetLightIntensityAttribute(const UsdPrim& prim) const
    {
      static const TfToken oldAttributeName("intensity");

      const UsdLuxLight light(prim);
      UsdAttribute intensityAttribute = light.GetIntensityAttr();
      const UsdResolveInfoSource source = intensityAttribute.GetResolveInfo().GetSource();
      const bool isDefaultValue = source == UsdResolveInfoSourceFallback;

      if (isDefaultValue && prim.HasAttribute(oldAttributeName)) {
        intensityAttribute = prim.GetAttribute(oldAttributeName);
      }
      return intensityAttribute;
    }

    DD::Image::SceneReaders::PluginDescription usdLightDescription("Light3", { "usd", "usda", "usdc", "usdz" }, DD::Image::SceneReaders::Constructor<UsdLightReader>);
  }
}
