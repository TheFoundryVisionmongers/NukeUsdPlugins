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
 \brief Header file for UsdConverter geometry conversion functions

 The UsdConverter shared library converts USD to Nuke geometry. The API offers
 functions at various levels of abstraction, e.g. the USDReader plugin uses the
 high level function loadUsd() to add the geometry from a USD file into a Nuke
 GeometryList. It is divided into functions dealing with geometry
 (Mesh, Points, Cube, PointInstancer), geometry attributes (points, vertices,
 colors, ...) and scene graph knob initialization.
 */

#ifndef USD_CONVERTER_H
#define USD_CONVERTER_H

#include <UsdConverter/UsdConverterApi.h>
#include <UsdConverter/UsdTypes.h>

// Standard includes
#include <memory>

// Library includes
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/base/tf/type.h>
#include <pxr/pxr.h>

namespace DD
{
  namespace Image
  {
    class GeometryList;
    class PolyMesh;
  }  // namespace Image
}  // namespace DD

namespace Foundry
{
  /// A collection of functions for converting USD to Nuke geometry
  namespace UsdConverter
  {
    // PUBLIC API
    /*! Load a USD file into Nuke, optionally with a mask
     * \param out       Geometry output list
     * \param filename  Input file to load
     * \param maskPaths Collection of mask paths, if empty no geometry is loaded
     * \param time      Timecode to fetch the data at
     */
    FN_USDCONVERTER_API void loadUsd(DD::Image::GeometryList& out,
                                     const std::string& filename,
                                     const std::vector<std::string>& maskPaths,
                                     const PXR_NS::UsdTimeCode time);

    /*! Convert geometry in the stage into Nuke geometry
     * \param out       Geometry output list
     * \param stage     Input USD stage
     * \param time      Timecode to fetch the data at
     */
    FN_USDCONVERTER_API void convertUsdGeometry(
        DD::Image::GeometryList& out, PXR_NS::UsdStageRefPtr stage,
        const PXR_NS::UsdTimeCode time = PXR_NS::UsdTimeCode::Default());

    /*! [Template] Convert USD_PRIM topology to NUKE_PRIM topology
     * \param fromPrim  Input USD prim
     * \param time      Timecode to fetch the data at
     * \return The converted Nuke geometry
     */
    template <class USD_PRIM, class NUKE_PRIM>
    FN_USDCONVERTER_API std::unique_ptr<NUKE_PRIM> convertUsdPrim(
        const USD_PRIM& fromPrim, PXR_NS::UsdTimeCode time = PXR_NS::UsdTimeCode::Default());

    /*! [Template] Add USD_PRIM to the Nuke geometry context for drawing
     * \param out       Geometry output list
     * \param prim      Input USD prim
     * \param time      Timecode to fetch the data at
     * \return Nuke geometry list index that was added, -1 if nothing was added
     */
    template <class USD_PRIM>
    FN_USDCONVERTER_API int addUsdPrim(
        DD::Image::GeometryList& out, const USD_PRIM& prim,
        const PXR_NS::UsdTimeCode time = PXR_NS::UsdTimeCode::Default());

    /*! Get a list of primitive data for all prims in a USD file
     * \param filename  USD file to load
     * \return PrimitiveData object containing prim paths and their types
     */
    FN_USDCONVERTER_API PrimitiveData
    getPrimitiveData(const std::string& filename);

    // PRIVATE API
    /*! Identify the USD prim type and if supported convert it to Nuke geometry
     * \param out       Geometry output list
     * \param prim      Input USD prim
     * \param time      Timecode to fetch the data at
     * \return Nuke geometry list index that was added, -1 if nothing was added
     */
    int addUsdPrim(DD::Image::GeometryList& out, const PXR_NS::UsdPrim& prim,
                   const PXR_NS::UsdTimeCode time);
  }  //namespace UsdConverter
}  // namespace Foundry

#endif
