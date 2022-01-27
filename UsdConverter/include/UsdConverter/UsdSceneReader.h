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
 \brief USD plugin for SceneReader (used in some 3D system nodes)
 */

#ifndef USDSCENEREADER_H
#define USDSCENEREADER_H

//DDImage includes
#include <DDImage/SceneReaderPlugin.h>
#include <DDImage/SceneReader.h>
#include <DDImage/CameraOp.h>
#include <DDImage/SceneItem.h>

// Library includes
#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/camera.h>

namespace Foundry
{
  namespace UsdConverter
  {
    /// base class for the USD SceneReaderPlugins
    class UsdSceneReaderBase : public DD::Image::SceneReaderPlugin {
    public:
      UsdSceneReaderBase();

      //Transformation knobs
      static const std::string kTranslateKnobName;
      static const std::string kRotateKnobName;
      static const std::string kScaleKnobName;
      static const std::string kRotOrderKnobName;
      static const std::string kTransformOrderKnobName;
      static const std::string kPivotKnobName;
      static const std::string kUniformScaleKnobName;
      static const std::string kSkewKnobName;

      /// check if the file can be used by this plugin
      bool isValid(const std::string& filename) override;

    protected:
      void setNodeAttributes(DD::Image::SceneReader& reader, const std::string& filename, const std::string& nodename);
      /**
      * Loads transform attributes (transform, rotate, scale ...) from a UsdPrim to an operator object
      * \param const UsdPrim& prim - contains required attributes
      * \param float time - frame number from which the attributes are loaded
      * \param Image::Op op - target object
      */
      void setTransformationAttributes(const PXR_NS::UsdPrim& prim, DD::Image::Op & op, float time) const;
      void setConstantTransformationAttributes(DD::Image::Op & op);

      /// create default usd scene knobs
      virtual void knobs(DD::Image::Knob_Callback cb) override;

      /**
      * Update plugin parameters when the file or usd knobs change
      * \param  reader reader that the plugin should operate on
      * \param Knob* k the knob that has changed
      * \return true if the knob has been handled by the callback, false otherwise
      */
      bool knob_changed(DD::Image::SceneReader* reader, DD::Image::Knob* k) override;

      /// return default usd scene knobs
      virtual std::vector<DD::Image::Knob*> getFileDependentKnobs(DD::Image::SceneReader* reader) override;

      /**
       * initializes the scene graph knob by populating it with usd primitives and selecting a default.
       * \param items a collection of items.
       */
      virtual void setupSceneGraph() const;

      /// function can be implemented to set custom knob as animated
      virtual void setCustomKnobsAsAnimated(const PXR_NS::UsdPrim& prim, const DD::Image::Op& op) const {}

      /// function can be implemented to set custom knobs as not animated
      virtual void clearCustomAnimation(const PXR_NS::UsdPrim& prim, const DD::Image::Op& op) const {}

      /**
      Function can be implemented to initialize custom attributes that don't change during animation
      * \param prim contains required attributes
      * \param op target object
      */
      virtual void setCustomConstantAttributes(const PXR_NS::UsdPrim& prim, DD::Image::Op& op) const {}

      /**
      Function can be implemented to load custom attributes for each frame
      * \param prim contains required attributes
      * \param op target object
      * \param time frame number from which the attributes are loaded
      */
      virtual void setCustomAnimationAttributes(const PXR_NS::UsdPrim& prim, DD::Image::Op & op, float time) const {}

      /**
      *Determines if the given prim is supported
      *\param prim a reference to a usd prim object
      * return false if SceneReader should ignore the prim
      */
      virtual bool isPrimSupported(const PXR_NS::UsdPrim& prim) const =0;

      /**
      * retrieve a collecion of usd primitives associated with this plugin.
      * \param filename the path to the usd file.
      * \return a colection if successful, else returns an empty vector.
      */
      virtual DD::Image::SceneItems loadUsdPrims(const char* pFilename) const;

      /// helper function to set the animated state of a knob
      void setKnobIsAnimated(DD::Image::Knob* pKnob, int numChannels) const;

      /// helper function to set the animated state of an op's knob by name
      void setKnobIsAnimated(const DD::Image::Op& op, const std::string& knobName, int numChannels) const;

      /// helper function to clear the animated state of an op's knob by name
      void clearKnobAnimated(const DD::Image::Op& op, const std::string& knobName) const;

      void _validate(DD::Image::SceneReader& reader, bool for_real) override;

      DD::Image::Knob* _sceneGraphKnob;
    private:

      void setKnobsAsAnimated(const PXR_NS::UsdPrim& prim, DD::Image::Op& op) const;
      void clearAnimation(const PXR_NS::UsdPrim& prim, DD::Image::Op& op) const;

      /**
      * Checks whether loaded items are supported by the plugin.
      * \param reader the op plugin that is doing the reading
      * \param items list of primitives to check
      * \return the first enabled item, or the empty SceneItem if there isn't anything enabled
      */
      DD::Image::SceneItem validateItems(DD::Image::SceneReader& reader, DD::Image::SceneItems& items);
      std::string _error{""};

      PXR_NS::TfToken _upAxisDirection;
    };

    template<class T>
    bool setKnobValue(DD::Image::Op& op, const std::string& knobName, const T& value, const char* errorMsg=nullptr)
    {
      const auto pKnob = op.knob(knobName.c_str());
      if (!pKnob) {
        if(errorMsg) {
          op.error(errorMsg);
        }
        return false;
      }
      pKnob->set_value(value);
      return true;
    }

    template<class T>
    bool setKnobValueAt(DD::Image::Op& op, const std::string& knobName, const T& value, float time, const char* errorMsg=nullptr)
    {
      const auto pKnob = op.knob(knobName.c_str());
      if (!pKnob) {
        if(errorMsg) {
          op.error(errorMsg);
        }
        return false;
      }
      pKnob->set_value_at(value, time);
      return true;
    }

    template<class T>
    bool getUsdAttrib(T& value, const PXR_NS::UsdAttribute& attribute, double time, DD::Image::Op& op, const char* errorMsg=nullptr)
    {
      if(!(attribute.HasValue() && attribute.Get<T>(&value, time))) {
        if(errorMsg) {
          op.error(errorMsg);
        }
        return false;
      }
      return true;
    }
  }
}
#endif
