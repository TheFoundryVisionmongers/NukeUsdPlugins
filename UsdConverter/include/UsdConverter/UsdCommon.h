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

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>

namespace Foundry
{
  namespace UsdConverter
  {
    /*! Apply a rotation to a matrix to convert from the axis direction defined by the token to Nuke's
     * axis direction (Y up)
     * \param mat the matrix on which to apply the transoform
     * \param upAxis token defining the up axis direction we are converting from ("X", "Y" or "Z")
     */
    void ApplyUpAxisRotation(PXR_NS::GfMatrix4d& mat, const PXR_NS::TfToken& upAxis);

  }  // namespace UsdConverter
}  // namespace Foundry
