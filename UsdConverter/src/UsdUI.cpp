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
 \brief Implementation file for UsdConverter scene graph initialization functions
 */

#include <DDImage/Attribute.h>
#include <DDImage/GeometryList.h>
#include <DDImage/SceneGraphBrowserI.h>
#include <DDImage/SceneGraph_KnobI.h>

#include <fstream>
#include <string>
#include <unordered_map>

#include "UsdConverter/UsdGeoConverter.h"

using namespace DD::Image;
namespace Foundry
{
  namespace UsdConverter
  {
    bool CheckKnobFileExists(const char* pFilename)
    {
      if(pFilename != nullptr && strlen(pFilename) > 0) {
        std::ifstream pin;
        pin.open(pFilename);
        if(!pin) {
          return false;
        }
        pin.close();
        return true;
      }
      return false;
    }

    bool PopulateSceneGraph(SceneGraph_KnobI* pSceneGraphKnob,
                            const char* pFilename,
                            const PrimitiveData& primitives, bool showBrowser,
                            bool resetSelected)
    {
      pSceneGraphKnob->setColumnHeader(pFilename);

      /* set the selected and imported items to defaults
      * when the user has a choice as to which primitives to import show the browser
      * otherwise, the default if the browser is requested is to show no primitives
      * if the browser isn't requested then the default is to show all primitives
      */

      bool dataSuccessfullySet = true;
      if(showBrowser) {
        /* this doesn't set values yet, just creates the browser - the callbacks for button
        * presses will set the values on the knob, and overwrite anything in this function
        */
        dataSuccessfullySet =
            SceneGraph::createBrowser(pFilename, pSceneGraphKnob, primitives);
      }
      else {
        pSceneGraphKnob->setItems(primitives, resetSelected);
      }

      return dataSuccessfullySet;
    }

    FN_USDCONVERTER_API bool GetSceneGraphData(
        SceneGraph_KnobI* pSceneGraphKnob, const char* pFilename,
        bool showBrowser, bool resetSelected)
    {
      if(!CheckKnobFileExists(pFilename)) {
        return false;
      }

      PrimitiveData primitives = getPrimitiveData(pFilename);
      if(primitives.empty()) {
        return false;
      }
      PopulateSceneGraph(pSceneGraphKnob, pFilename, primitives, showBrowser,
                         resetSelected);

      return true;
    }
  }  // namespace UsdConverter
}  // namespace Foundry
