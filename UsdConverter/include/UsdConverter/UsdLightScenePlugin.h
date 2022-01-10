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
 \brief USD plugin for Light node
 */

#ifndef USDLIGHTSCENEPLUGIN_H
#define USDLIGHTSCENEPLUGIN_H

#include "UsdSceneReader.h"
#include <pxr/usd/usdLux/shapingAPI.h>

namespace Foundry
{
  namespace UsdConverter
  {
    /// plugin class for the Nuke Light node
    class UsdLightReader : public UsdSceneReaderBase {
    public:
      /*! adds usd knobs after the file knobs.
      * \param cb knob create or store callback.
      */
      void knobs(DD::Image::Knob_Callback cb) override;
      /// return any usd light specific knobs
      std::vector<DD::Image::Knob*> getFileDependentKnobs(DD::Image::SceneReader* reader) override;

    protected:
      void setCustomConstantAttributes(const PXR_NS::UsdPrim& prim, DD::Image::Op& op) const override;
      /// set all the light knobs as animated
      void setCustomKnobsAsAnimated(const PXR_NS::UsdPrim& prim, const DD::Image::Op& op) const override;
      /// remove the animation from all the light knobs
      void clearCustomAnimation(const PXR_NS::UsdPrim& prim, const DD::Image::Op& op) const override;
       /*! load all the animated parameters into the light knobs
        * \param prim the light primitive to load
        * \param op the light op to change the knobs on
        * \param time the time to set the values at
        */
      void setCustomAnimationAttributes(const PXR_NS::UsdPrim& prim, DD::Image::Op & op, float time) const override;
      /*! check if the primitive is supported by the usd light plugin
       * \param prim to check
       * \return true if the primitive can be loaded into Nuke, false otherwise
       */
      bool isPrimSupported(const PXR_NS::UsdPrim& prim) const override;
      /*!
       * retrieve a colelction of usd lights. Unsupported light types are disabled.
       * \param filename the path to the usd file.
       * \return a collection if usd lights if successful, else returns an empty vector.
      */
      DD::Image::SceneItems loadUsdPrims(const char* pFilename) const override;
    private:
      /*! load light intensity value from a usd attribute
      * \param usd attribute containing the intensity value
      * \param op the light op to change the knobs on
      * \param time the time to set the values at
      */
      bool setIntensity(const PXR_NS::UsdAttribute& attribute, DD::Image::Op & op, float frame) const;
      /*! load light color from a usd attribute
      * \param attribute the usd attribute containing the intensity value
      * \param op the light op to change the knobs on
      * \param time the time to set the values at
      */
      bool setColor(const PXR_NS::UsdAttribute& attribute, DD::Image::Op & op, float frame) const;

      /*! load light cone angle and fall of from a USD light primitive
      * \param prim the light prim to load
      * \param op the light op to change the knobs on
      * \param time the time to set the values at
      */
      void setLightCone(const PXR_NS::UsdLuxShapingAPI& prim, DD::Image::Op & op, float time) const;

      /*! check if the primitive has light shape parameters like cone angle and cone softness
       * \param prim to check
       * \return true if the primitive is a spot light, false otherwise
      */
      bool isSpotLight(const PXR_NS::UsdPrim& prim) const;

      /*! returns a usd light color attribute
       * The light color attribute name was changed in USD version 21.05 (from 'color' to 'inputs:color').
       * The function can handle both, the old and the new name, and return a correct light color.
       * \param source prim
       */
      PXR_NS::UsdAttribute GetLightColorAttribute(const PXR_NS::UsdPrim& prim) const;

      /*! returns a usd color intensity attribute
       * The light intensity attribute name was changed in USD version 21.05 (from 'intensity' to 'inputs:intensity').
       * The function can handle both, the old and the new name, and return a correct intensity value.
       * \param source prim
       */
      PXR_NS::UsdAttribute GetLightIntensityAttribute(const PXR_NS::UsdPrim& prim) const;
    };
  }
}

#endif
