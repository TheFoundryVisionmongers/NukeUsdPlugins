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
 \brief USD plugin for Axis node
 */

#ifndef USDAXISSCENEPLUGIN_H
#define USDAXISSCENEPLUGIN_H

#include "UsdSceneReader.h"

namespace Foundry
{
  namespace UsdConverter
  {
    /// plugin class for the Nuke Axis node
    class UsdAxisReader : public UsdSceneReaderBase {
    public:
      /*! adds usd knobs after the file knobs.
      * \param cb knob create or store callback.
      */
      void knobs(DD::Image::Knob_Callback cb) override;

    protected:
      /*!
       * For axis we must have one scene item for every prim
       * \param filename the path to the usd file.
       * \return a colection if successful, else returns an empty vector.
       */
      DD::Image::SceneItems loadUsdPrims(const char* pFilename) const override;

      /*! check if the primitive is supported by the usd axis plugin
       * \param prim to check
       * \return true if the primitive can be loaded into Nuke, false otherwise
       */
      bool isPrimSupported(const PXR_NS::UsdPrim& prim) const override;

      virtual void setupSceneGraph() const override;
    };
  }
}

#endif
