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
 \brief USD plugin for Camera node
 */

#ifndef USDCAMERASCENEPLUGIN_H
#define USDCAMERASCENEPLUGIN_H

#include "UsdSceneReader.h"

namespace Foundry
{
  namespace UsdConverter
  {
    /// plugin class for the Nuke Camera node
    class UsdCameraReader : public UsdSceneReaderBase {
    public:
      UsdCameraReader();
      ~UsdCameraReader() override;

      /*! adds usd knobs after the file knobs.
      * \param cb knob create or store callback.
      */
      void knobs(DD::Image::Knob_Callback cb) override;
      /// return any usd camera specific knobs
      std::vector<DD::Image::Knob*> getFileDependentKnobs(DD::Image::SceneReader* reader) override;

    protected:
      /// set all the camera knobs as animated
      void setCustomKnobsAsAnimated(const PXR_NS::UsdPrim& prim, const DD::Image::Op& op) const override;
      /// remove the animation from all the camera knobs
      void clearCustomAnimation(const PXR_NS::UsdPrim& prim, const DD::Image::Op& op) const override;
      /*! load the projection and non-animated parameters into the camera knobs
       * \param prim the camera primitive to load
       * \param op the camera op to change the knobs on
       */
      void setCustomConstantAttributes(const PXR_NS::UsdPrim& prim, DD::Image::Op& op) const override;
      /*! load all the animated parameters into the camera knobs
       * \param prim the camera primitive to load
       * \param op the camera op to change the knobs on
       * \param time the time to set the values at
       */
      void setCustomAnimationAttributes(const PXR_NS::UsdPrim& prim, DD::Image::Op & op, float time) const override;
      /*! check if the primitive is supported by the usd camera plugin
       * \param prim to check
       * \return true if the primitive can be loaded into Nuke, false otherwise
       */
      bool isPrimSupported(const PXR_NS::UsdPrim& prim) const override;
    private:
      /**
      * Loads camera's animated attributes from a UsdGeomCamera to a camera operator object
      * \param const UsdPrim& prim - contains required attributes
      * \param float time - frame number from which the attributes are loaded
      * \param Image::Op op - target object
      */
      void setCameraAnimationAttribute(const PXR_NS::UsdGeomCamera& camera, float frame, DD::Image::Op & cameraOperator)const;
      /*! set the camera aperature offset (the window translate)
       * \param camera the camera to load values from
       * \param frame frame number to load the values from
       * \param cameraOperator operator to load the values into
       */
      void loadCameraApertureOffset(const PXR_NS::UsdGeomCamera& camera, float frame, DD::Image::Op & cameraOperator) const;
      /*! set the camera projection attributes, such as whether or not it's orthographic, etc
       * \param camera the camera to load values from
       * \param cameraOperator operator to load the values into
       */
      void loadCameraProjection(const PXR_NS::UsdGeomCamera& camera, DD::Image::Op & cameraOp) const;
      /*! set the scale and roll
       * \param op operator to load the values into
       */
      void setConstantCameraAttributes(DD::Image::Op & op) const;
    };
  }
}

#endif
