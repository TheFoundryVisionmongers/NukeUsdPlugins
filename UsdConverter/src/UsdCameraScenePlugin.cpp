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
 \brief USD Camera plugin for camera node
 */

#include "UsdConverter/UsdCameraScenePlugin.h"

//DDImage includes
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/SceneGraph_KnobI.h>

// Library includes
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/xformCache.h>

//"Projection" tab knobs
static const std::string kProjectionKnobName = "projection_mode"; //Camera projection
static const std::string kFocalLengthKnobName = "focal";          //Focal lenght
static const std::string kHorizApertureKnobName = "haperture";    //Horizontal aperture
static const std::string kVertApertureKnobName = "vaperture";     //Vertical aperture
static const std::string kNearKnobName = "near";                  //Near clipping plane
static const std::string kFarKnobName = "far";                    //Far clipping plane
static const std::string kFocalDistanceKnobName = "focal_point";  //Focal distance
static const std::string kFStopKnobName = "fstop";                //Fstop
static const std::string kWinTranslateKnobName = "win_translate"; //Camera aperture offset
static const std::string kWinScaleKnobName = "win_scale";
static const std::string kWinRollKnobName = "winroll";

const static auto kCameraKnobs = {
  kFocalLengthKnobName, kHorizApertureKnobName, kVertApertureKnobName, kNearKnobName,
  kFarKnobName, kFocalDistanceKnobName, kFStopKnobName };

namespace Foundry {
  namespace UsdConverter {
    PXR_NAMESPACE_USING_DIRECTIVE

      UsdCameraReader::UsdCameraReader() : UsdSceneReaderBase()
    {}

    UsdCameraReader::~UsdCameraReader()
    {
    }

    std::vector<DD::Image::Knob*> UsdCameraReader::getFileDependentKnobs(DD::Image::SceneReader* reader)
    {
      return UsdSceneReaderBase::getFileDependentKnobs(reader);
    }

    void UsdCameraReader::knobs(DD::Image::Knob_Callback cb)
    {
      UsdSceneReaderBase::knobs(cb);
    }

    void UsdCameraReader::setCustomKnobsAsAnimated(const UsdPrim&, const DD::Image::Op& op) const
    {
      for(const auto& knobName : kCameraKnobs) {
        setKnobIsAnimated(op, knobName, 1);
      }
    }

    void UsdCameraReader::clearCustomAnimation(const UsdPrim&, const DD::Image::Op& op) const
    {
      for(const auto& knobName : kCameraKnobs) {
        clearKnobAnimated(op, knobName);
      }
      clearKnobAnimated(op, kWinTranslateKnobName);
    }

    void UsdCameraReader::setCustomConstantAttributes(const UsdPrim& prim, DD::Image::Op& op) const
    {
      const UsdGeomCamera camera(prim);
      loadCameraProjection(camera, op);
      setConstantCameraAttributes(op);
    }

    void UsdCameraReader::setCustomAnimationAttributes(const UsdPrim& prim, DD::Image::Op & op, float time) const
    {
      const UsdGeomCamera camera(prim);
      setCameraAnimationAttribute(camera, time, op);
    }

    bool UsdCameraReader::isPrimSupported(const UsdPrim& prim) const
    {
      return prim.IsA<UsdGeomCamera>();
    }

    void UsdCameraReader::setCameraAnimationAttribute(const UsdGeomCamera& camera, float frame, DD::Image::Op & cameraOperator) const
    {
      typedef UsdAttribute(UsdGeomCamera::*GetAttribFunc)() const;
      const auto loadKnobValue = [&camera, &cameraOperator, &frame](const std::string& knobName, GetAttribFunc pFunc) {
        float value = 0.0;
        return getUsdAttrib(value, (camera.*pFunc)(), frame, cameraOperator, ("Could not get attribute for " + knobName).c_str())
            && setKnobValueAt(cameraOperator, knobName, value, frame, ("Could not set knob " + knobName).c_str());
      };

      //Focal length
      loadKnobValue(kFocalLengthKnobName, &UsdGeomCamera::GetFocalLengthAttr);

      //Horizontal Aperture
      loadKnobValue(kHorizApertureKnobName, &UsdGeomCamera::GetHorizontalApertureAttr);

      //Vertical Aperture
      loadKnobValue(kVertApertureKnobName, &UsdGeomCamera::GetVerticalApertureAttr);

      //Focal point
      loadKnobValue(kFocalDistanceKnobName, &UsdGeomCamera::GetFocusDistanceAttr);

      //FStop
      loadKnobValue(kFStopKnobName, &UsdGeomCamera::GetFStopAttr);

      //Window translate
      loadCameraApertureOffset(camera, frame, cameraOperator);

      //Clipping range
      GfVec2f clippingRange;
      if(getUsdAttrib(clippingRange, camera.GetClippingRangeAttr(), frame, cameraOperator, "no \"clippingRange\" GfVec2fAttr")) {
        setKnobValueAt(cameraOperator, kNearKnobName, clippingRange[0], frame, ("No " + kNearKnobName + " knob").c_str());
        setKnobValueAt(cameraOperator, kFarKnobName,  clippingRange[1], frame, ("No " + kFarKnobName + " knob").c_str());
      }
    }

    void UsdCameraReader::loadCameraApertureOffset(const UsdGeomCamera& camera, float frame, DD::Image::Op & op)const
    {
      float horizontalOffset = 0.0f, verticalOffset = 0.0f, horizontalAperture = 0.0f, verticalAperture = 0.0f;
      if(   !getUsdAttrib(horizontalOffset,   camera.GetHorizontalApertureOffsetAttr(), frame, op, "no \" HorizontalApertureOffset \" attribute")
         || !getUsdAttrib(verticalOffset,     camera.GetVerticalApertureOffsetAttr(),   frame, op, "no \" VerticalApertureOffset \" attribute")
         || !getUsdAttrib(horizontalAperture, camera.GetHorizontalApertureAttr(),       frame, op, "no \" HorizontalAperture \" attribute")
         || !getUsdAttrib(verticalAperture,   camera.GetVerticalApertureAttr(),         frame, op, "no \" VerticalAperture \" attribute")) {
        return;
      }

      if (horizontalAperture > 0.0f)
        horizontalOffset = horizontalOffset * 2.0f / horizontalAperture;

      if (verticalAperture > 0.0f)
        verticalOffset = verticalOffset * 2.0f / verticalAperture;

      const auto pKnob = op.knob(kWinTranslateKnobName.c_str());
      if(pKnob) {
        pKnob->set_value_at(horizontalOffset, frame, 0);
        pKnob->set_value_at(verticalOffset, frame, 1);
      }
      else {
        op.error(("no \"" + kWinTranslateKnobName + "\" Knob").c_str());
      }
    }

    void UsdCameraReader::loadCameraProjection(const UsdGeomCamera& camera, DD::Image::Op & cameraOp) const
    {
      TfToken projectionToken;
      if(getUsdAttrib(projectionToken, camera.GetProjectionAttr(), UsdTimeCode::Default().GetValue(), cameraOp, "no \"Projection\" attribute")) {
        const int cameraProjection = (projectionToken == UsdGeomTokens->orthographic) ? DD::Image::CameraOp::LENS_ORTHOGRAPHIC
                                                                                      : DD::Image::CameraOp::LENS_PERSPECTIVE;
        setKnobValue(cameraOp, kProjectionKnobName, cameraProjection, ("No " + kProjectionKnobName + " knob").c_str());
      }
    }

    void UsdCameraReader::setConstantCameraAttributes(DD::Image::Op & op) const
    {
      DD::Image::Knob*pKnob = nullptr;
      const double win_scale[3] = { 1.0, 1.0 };
      if((pKnob = op.knob(kWinScaleKnobName.c_str())) != nullptr) {
        pKnob->set_values(win_scale, 2);
      }
      setKnobValue(op, kWinRollKnobName, 0.0f);
    }

    DD::Image::SceneReaders::PluginDescription usdCameraDescription("Camera3", { "usd", "usda", "usdc", "usdz" }, DD::Image::SceneReaders::Constructor<UsdCameraReader>);
  }
}
