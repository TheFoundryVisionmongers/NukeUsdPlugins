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
 \brief Header file for UsdConverter types
 */

#ifndef USD_TYPES_H
#define USD_TYPES_H

// Standard includes
#include <map>
#include <string>
#include <vector>

namespace Foundry
{
  namespace UsdConverter
  {
    /*! Holds lists with USD prim data to insert into the scene graph knob
     *
     * The scene graph knob displays prim names (e.g. "/world/triangle") and their
     * corresponding types (e.g. "Mesh"). They are passed using this map with
     * the required keys "name" and "type", and the map values are string
     * vectors holding the prim names and their types. Elements with the same index
     * in each string vector belong together.
     *
     * Example: display 2 prims "/world/triangle" (type "Mesh") and "/world/box" (type "Cube"):
     * {
     *   "name": ["/world/triangle", "/world/box"],
     *   "type": ["Mesh",            "Cube"      ],
     * }
     */
    using PrimitiveData = std::map<std::string, std::vector<std::string>>;

  }  //namespace UsdConverter
}  // namespace Foundry

#endif
