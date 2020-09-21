// Copyright 2020 Foundry
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
 \brief Header file for UsdConverter scene graph initialization functions
 */

#ifndef USD_UI_H
#define USD_UI_H

#include <UsdConverter/UsdConverterApi.h>
#include <UsdConverter/UsdTypes.h>

namespace DD
{
  namespace Image
  {
    class SceneGraph_KnobI;
  }
}  // namespace DD

namespace Foundry
{
  /// for UI integration with nuke
  namespace UsdConverter
  {
    /*! launch a scene view browser with data from the filename
    * \param pSceneGraphKnob the scene view graph knob
    * \param filename the name of the usd file to load
    * \param showBrowser True if the scene browser is launched as a pop up
    * \param resetSelected True if the contents of the scene browser should be reset to what is in the file
    * \return bool False if the browser was requested but could not be created
    */
    FN_USDCONVERTER_API bool GetSceneGraphData(
        DD::Image::SceneGraph_KnobI* pSceneGraphKnob, const char* filename,
        bool showBrowser, bool resetSelected);

    /*! internal function for filling the scene view knob with data
    * \param pSceneGraphKnob the scene view graph knob
    * \param filename the name of the usd file to load
    * \param primitives values to insert into the scene graph
       *   accepted keys:
       *     "name" - the name of the item in the list
       *     "type" - the type of the item
    * \param showBrowser True if the scene browser is launched as a pop up
    * \param resetSelected True if the contents of the scene browser should be reset to what is in the file
    * \return False if the browser was requested but could not be created
    */
    bool PopulateSceneGraph(DD::Image::SceneGraph_KnobI* pSceneGraphKnob,
                            const char* filename,
                            const PrimitiveData& primitives, bool showBrowser,
                            bool resetSelected);
  }  //namespace UsdConverter
}  // namespace Foundry

#endif
