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
 \brief USD implementation for SceneReader nodes
 */

#include "UsdConverter/UsdSceneReader.h"
#include "UsdConverter/UsdCommon.h"

//DDImage includes
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/SceneGraph_KnobI.h>

// Library includes
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/tokens.h>

namespace Foundry {
  namespace UsdConverter {
    PXR_NAMESPACE_USING_DIRECTIVE

    namespace { //Helper functions
      DD::Image::Matrix4 getMatrix4(const GfMatrix4d& from)
      {
        return DD::Image::Matrix4(
          (float)from[0][0], (float)from[1][0], (float)from[2][0], (float)from[3][0],
          (float)from[0][1], (float)from[1][1], (float)from[2][1], (float)from[3][1],
          (float)from[0][2], (float)from[1][2], (float)from[2][2], (float)from[3][2],
          (float)from[0][3], (float)from[1][3], (float)from[2][3], (float)from[3][3]);
      }

      float toDegrees(float radians) {
        return radians * 180.0f / M_PI_F;
      }

      float calculateSkew(float shear) {
        if (shear == 0.0f)
          return shear;
        return fmodf(90.0f - (180.0f / M_PI_F)*atanf(1.0f / shear), 180.0f);
      }
    }

    //Transformation knobs
    const std::string UsdSceneReaderBase::kTranslateKnobName = "translate";
    const std::string UsdSceneReaderBase::kRotateKnobName = "rotate";
    const std::string UsdSceneReaderBase::kScaleKnobName = "scaling";
    const std::string UsdSceneReaderBase::kRotOrderKnobName = "rot_order";
    const std::string UsdSceneReaderBase::kTransformOrderKnobName = "xform_order";
    const std::string UsdSceneReaderBase::kPivotKnobName = "pivot";
    const std::string UsdSceneReaderBase::kUniformScaleKnobName = "uniform_scale";
    const std::string UsdSceneReaderBase::kSkewKnobName = "skew";

    const static auto translationKnobs = { UsdSceneReaderBase::kTranslateKnobName,
                                           UsdSceneReaderBase::kRotateKnobName,
                                           UsdSceneReaderBase::kScaleKnobName,
                                           UsdSceneReaderBase::kSkewKnobName };

    UsdSceneReaderBase::UsdSceneReaderBase() :
        _sceneGraphKnob(nullptr),
        _upAxisDirection(UsdGeomTokens->y)
    {}

    DD::Image::SceneItems UsdSceneReaderBase::loadUsdPrims(const char* pFilename) const
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
          prims.emplace_back(prim.GetPath().GetString(), prim.GetTypeName().GetString());
        }
      }
      return prims;
    }

    bool UsdSceneReaderBase::knob_changed(DD::Image::SceneReader* reader, DD::Image::Knob* k)
    {
      if (!reader->getReadFromFile() || !k) {
        return false;
      }

      const bool fileChanged = k->is(DD::Image::kFileKnobName.c_str());
      const bool fileReloaded = k->is(DD::Image::kReloadKnobName.c_str()) || (k->is(DD::Image::kReadFromFileKnobName.c_str()) && k->get_value());
      const auto filename = reader->getFilename();

      if (filename.empty()) {
        return false;
      }
      bool result = false;
      if (fileChanged || fileReloaded) {

        setupSceneGraph();
        if(reader->getLoadHint()) {
          DD::Image::SceneItems items = loadUsdPrims(filename.c_str());
          _sceneGraphKnob->sceneGraphKnob()->setItems(items, false);

          DD::Image::SceneItem item = validateItems(*reader, items);
          if(!_sceneGraphKnob->sceneGraphKnob()->hasSelection()) {
            _sceneGraphKnob->sceneGraphKnob()->setSelectedItems({item.name});
          }
          result = true;
        }

        if (fileChanged) {
          _sceneGraphKnob->sceneGraphKnob()->setFocus();
        }
      }
      else if (k == &DD::Image::Knob::showPanel || k->is(DD::Image::kSceneGraphKnobName)) {
        auto* sceneGraphKnobI = _sceneGraphKnob->sceneGraphKnob();
        auto selected_items = sceneGraphKnobI->getSelectedItems(DD::Image::SceneGraph::kNameField);
        if (!selected_items.empty()) {
          setNodeAttributes(*reader, filename.c_str(), selected_items.back());
        }
        result = true;
      }
      return result;
    }

    void UsdSceneReaderBase::setKnobIsAnimated(DD::Image::Knob* pKnob, int numChannels) const
    {
      pKnob->clear_animated(-1);
      for (int i = 0; i < numChannels; i++)
        pKnob->set_animated(i);
    }

    void UsdSceneReaderBase::setKnobIsAnimated(const DD::Image::Op& op, const std::string& knobName, int numChannels) const
    {
      const auto pKnob = op.knob(knobName.c_str());
      if(pKnob) {
        setKnobIsAnimated(pKnob, numChannels);
      }
    }

    void UsdSceneReaderBase::clearKnobAnimated(const DD::Image::Op& op, const std::string& knobName) const
    {
      const auto pKnob = op.knob(knobName.c_str());
      if(pKnob) {
        pKnob->clear_animated(-1);
      }
    }

    void UsdSceneReaderBase::_validate(DD::Image::SceneReader& reader, bool for_real)
    {
      if(!_error.empty()) {
        reader.error(_error.c_str());
      }
    }

    bool UsdSceneReaderBase::isValid(const std::string& filename)
    {
      return true;
    }

    void UsdSceneReaderBase::setConstantTransformationAttributes(DD::Image::Op & op) {
      DD::Image::Knob* pKnob = op.knob(kRotOrderKnobName.c_str());
      if (pKnob)
        pKnob->set_value(DD::Image::Matrix4::eXYZ);
      else
        throw std::runtime_error("no \""+ kRotOrderKnobName +"\" knob");

      pKnob = op.knob(kTransformOrderKnobName.c_str());
      if (pKnob)
        pKnob->set_value(DD::Image::Matrix4::eSRT);
      else
        throw std::runtime_error("no \""+ kTransformOrderKnobName +"\" knob");

      pKnob = op.knob(kPivotKnobName.c_str());
      const double pivot[3] = { 0.0, 0.0, 0.0 };
      if (pKnob)
        pKnob->set_values(pivot, 3);
      else
        throw std::runtime_error("no \""+ kPivotKnobName +"\" knob");

      pKnob = op.knob(kUniformScaleKnobName.c_str());
      if (pKnob)
        pKnob->set_value(1.0f);
      else
        throw std::runtime_error("no \"" + kUniformScaleKnobName + "\" knob");
    }

    void UsdSceneReaderBase::setTransformationAttributes(const UsdPrim& prim, DD::Image::Op & op, float time) const
    {
      DD::Image::Knob* pTranslateKnob = op.knob(kTranslateKnobName.c_str());
      if (!pTranslateKnob)
        throw std::runtime_error("no \""+ kTranslateKnobName +"\" knob");

      DD::Image::Knob* pRotationKnob = op.knob(kRotateKnobName.c_str());
      if(!pRotationKnob)
        throw std::runtime_error("no \""+ kRotateKnobName +"\" knob");

      DD::Image::Knob* pScalingKnob = op.knob(kScaleKnobName.c_str());
      if(!pScalingKnob)
        throw std::runtime_error("no \""+ kScaleKnobName +"\" knob");

      DD::Image::Knob* pSkewKnob = op.knob(kSkewKnobName.c_str());
      if (!pSkewKnob)
        throw std::runtime_error("no \"" + kSkewKnobName + "\" knob");

      UsdGeomXformCache cache(time);
      GfMatrix4d world = cache.GetLocalToWorldTransform(prim);

      // Apply axis transform
      ApplyUpAxisRotation(world, _upAxisDirection);

      //Translation
      const GfVec3d translation = world.ExtractTranslation();
      pTranslateKnob->set_values_at(translation.data(), 3, time);

      //Rotation
      DD::Image::Matrix4 matrix4 = getMatrix4(world);

      const DD::Image::Matrix4::RotationOrder order = DD::Image::Matrix4::eXYZ;
      float x_rotation, y_rotation, z_rotation;
      matrix4.getRotations(order, x_rotation, y_rotation, z_rotation);
      const double euler_angles[3] = { toDegrees(x_rotation), toDegrees(y_rotation), toDegrees(z_rotation) };
      pRotationKnob->set_values_at(euler_angles, 3, time);

      //Scaling and sheer
      DD::Image::Vector3 scale;
      DD::Image::Vector3 shear;
      matrix4.extractAndRemoveScalingAndShear(scale, shear);
      pScalingKnob->set_values_at(scale.array(), 3, time);

      for (int i = 0; i < 3; i++) {
        float skew = calculateSkew(shear[i]);
        pSkewKnob->set_value_at(skew, time, i);
      }
    }

    void UsdSceneReaderBase::setNodeAttributes(DD::Image::SceneReader& reader, const std::string& filename, const std::string& nodename)
    {
      DD::Image::Op * op = dynamic_cast<DD::Image::Op*>(&reader);
      if (!op)
        return;

      UsdStageRefPtr stage = UsdStage::Open(filename);
      if (!stage)
        return;

      _upAxisDirection = UsdGeomGetStageUpAxis(stage);

      const float start = (float)stage->GetStartTimeCode();
      const float end = (float)stage->GetEndTimeCode();

      const UsdPrim usdPrim = stage->GetPrimAtPath(SdfPath(nodename));

      // the prim doesn't exist at the path
      if(!usdPrim) {
        op->error("Primitive doesn't exist: %s", nodename.c_str());
        return;
      }

      if (!isPrimSupported(usdPrim))
        return;

      reader.setStartFrame((float)stage->GetStartTimeCode());
      reader.setEndFrame((float)stage->GetEndTimeCode());

      DD::Image::Knob * pUserFrameRateKnob = op->knob(DD::Image::kUseFrameRateKnobName.c_str());
      if (pUserFrameRateKnob && (pUserFrameRateKnob->get_value() < 1.0)) {
        DD::Image::Knob * pFrameRateKnob = op->knob(DD::Image::kFrameRateKnobName.c_str());
        if (pFrameRateKnob) {
          double d = pUserFrameRateKnob->get_value();
          const float frameRate = (float)stage->GetFramesPerSecond();
          pFrameRateKnob->set_value(frameRate);
        }
        else {
          throw std::runtime_error("no \"" + DD::Image::kFrameRateKnobName + "\" knob");
        }
      }

      if (start != end)
        setKnobsAsAnimated(usdPrim, *op);
      else
        clearAnimation(usdPrim, *op);

      setConstantTransformationAttributes(*op);
      setCustomConstantAttributes(usdPrim, *op);
      for (float time = start; time <= end; time += 1.0f) {
        setTransformationAttributes(usdPrim, *op, time);
        setCustomAnimationAttributes(usdPrim, *op, time);
      }
    }

    void UsdSceneReaderBase::setKnobsAsAnimated(const UsdPrim& prim, DD::Image::Op& op) const
    {
      for (const auto& knobName : translationKnobs) {
        setKnobIsAnimated(op, knobName, 3);
      }

      setCustomKnobsAsAnimated(prim, op);
    }

    void UsdSceneReaderBase::clearAnimation(const UsdPrim& prim, DD::Image::Op& op) const
    {
      for(const auto& knobName : translationKnobs) {
        clearKnobAnimated(op, knobName);
      }

      clearCustomAnimation(prim, op);
    }

    void UsdSceneReaderBase::knobs(DD::Image::Knob_Callback cb)
    {
      Divider(cb, "USD Options");
      static int index = 0;
      _sceneGraphKnob = SceneGraph_knob(cb, &index, nullptr, DD::Image::kSceneGraphKnobName, "");
      if (_sceneGraphKnob != nullptr) {
        _sceneGraphKnob->set_flag(DD::Image::Knob::SAVE_MENU | DD::Image::Knob::EARLY_STORE | DD::Image::Knob::ALWAYS_SAVE | DD::Image::Knob::KNOB_CHANGED_RECURSIVE);
        Tooltip(cb, "Usd primitive paths");
      }
    }

    std::vector<DD::Image::Knob*> UsdSceneReaderBase::getFileDependentKnobs(DD::Image::SceneReader* /* reader */)
    {
      if (_sceneGraphKnob)
        return { _sceneGraphKnob };
      return {};
    }

    void UsdSceneReaderBase::setupSceneGraph() const
    {
      auto* sceneGraphKnobI = _sceneGraphKnob->sceneGraphKnob();
      sceneGraphKnobI->enableListView();
    }

    DD::Image::SceneItem UsdSceneReaderBase::validateItems(DD::Image::SceneReader& reader, DD::Image::SceneItems& items)
    {
      auto item = std::find_if(items.cbegin(), items.cend(), [](const auto& prim) {
        return prim.enabled;
        });

      if (item == items.cend()) {
        _error = "USD file contains no supported data";
        return DD::Image::SceneItem();
      }
      _error.clear();
      return *item;
    }
  }
}
