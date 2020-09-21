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
 \brief Header file for UsdConverter attribute conversion functions
 */

#ifndef USD_ATTR_CONVERTER_H
#define USD_ATTR_CONVERTER_H

#include <DDImage/Attribute.h>
#include <DDImage/GeoInfo.h>
#include <UsdConverter/UsdConverterApi.h>

// Library includes
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>

namespace DD
{
  namespace Image
  {
    class GeometryList;
    class Attribute;
    class Matrix4;
  }  // namespace Image
}  // namespace DD

namespace Foundry
{
  /// A collection of functions for converting USD to Nuke geometry
  namespace UsdConverter
  {
    // PUBLIC API
    /*! Write the data from the usd attributes into the geometrylist, where Nuke stores point, attribute data as it passes through nodes
     * \param out       The geometry to modify
     * \param obj       The index (object number) into the geometry to modify
     * \param primvars  The attributes to convert
     * \param time      Timecode to fetch the data at
     */
    FN_USDCONVERTER_API void ConvertUsdAttributes(
        DD::Image::GeometryList& out, const int obj,
        const std::vector<PXR_NS::UsdAttribute>& primvars, const PXR_NS::UsdTimeCode time);

    // PRIVATE API
    /// Parameters for filling out data on primitives
    struct ColorUvData
    {
      PXR_NS::VtVec2fArray uvs;
      PXR_NS::VtArray<PXR_NS::GfVec3f> color;
      PXR_NS::VtFloatArray opacity;
      PXR_NS::VtUIntArray faceVertexIndices;
      int uvElementSize = 1;
      int colorElementSize = 1;
      int opacityElementSize = 1;
      DD::Image::GroupType uvGroup = DD::Image::Group_Vertices;
      DD::Image::GroupType colorGroup = DD::Image::Group_None;
      DD::Image::GroupType opacityGroup = DD::Image::Group_None;

      ColorUvData() = default;
      ColorUvData(const ColorUvData& other, int offset);
    };

    /// Map to Nuke attribute type (Float, Vector3, Matrix, etc)
    DD::Image::AttribType ConvertAttribType(const PXR_NS::UsdAttribute& fromAttr);

    /// Map to Nuke attribute group (Point, Vertex, Object, etc)
    DD::Image::GroupType ConvertGroupType(const PXR_NS::UsdAttribute& fromAttr);

    /*! Map names of some attributes that are special for Nuke, for example, display color (Cf)
     * \param fromAttr  USD attribute
     * \return A Nuke attribute (N, Cf, PW, vel, size, uv), or the USD attribute name if not a match for one of these
     */
    PXR_NS::TfToken ConvertName(const PXR_NS::UsdAttribute& fromAttr);

    /// Compute attribute, flattening indexed values if necessary
    template <class DEST>
    void ComputePrimvar(DEST& value, const PXR_NS::UsdAttribute& attr,
                        PXR_NS::UsdTimeCode time);

    /*! Add points to geometry
     * \param out       Geometry to modify
     * \param obj       GeoInfo index to modify
     * \param fromAttr  Attribute to get the point data from
     * \param time      Timecode to fetch the data at
     * \return Number of points added
     */
    size_t ConvertPoints(DD::Image::GeometryList& out, const int obj,
                         const PXR_NS::UsdAttribute& fromAttr,
                         const PXR_NS::UsdTimeCode time);

    /*! Convert USD attributes that don't map to Nuke ones directly
     * \param data      Output collected data for color and uvs
     * \param attrs     The attributes to convert
     * \param time      Timecode to fetch the data at
     * \return Remaining attributes
     */
    std::vector<PXR_NS::UsdAttribute> ConvertMismatchedAttributes(
        ColorUvData& data, const std::vector<PXR_NS::UsdAttribute>& attrs,
        const PXR_NS::UsdTimeCode time);

    /*! Copy the color and opacity into an attribute
     * \param Cf                  Color attribute for Nuke
     * \param color               Color 0 - 1
     * \param colorGroup          GroupType of the color
     * \param opacity             Opacity 0 - 1 (1 is fully opaque)
     * \param opactiyGroup        GroupType of opacity
     * \param faceVertexIndices   Vertex -> point number array
     */
    void ConvertColor(DD::Image::Attribute& Cf, const PXR_NS::VtArray<PXR_NS::GfVec3f>& color,
                      DD::Image::GroupType colorGroup,
                      const PXR_NS::VtFloatArray& opacity,
                      DD::Image::GroupType opacityGroup,
                      const PXR_NS::VtUIntArray& faceVertexIndices);

    /*! Add the prim path as the name attribute
     * \param out       Geometry to modify
     * \param obj       GeoInfo index to modify
     * \param prim      Prim whose path to add
     */
    void ConvertPrimPath(DD::Image::GeometryList& out, int obj,
                         const PXR_NS::UsdPrim& prim);

    /// Fill Nuke attribute with uvs
    void ConvertUvs(DD::Image::Attribute& toAttr, const PXR_NS::VtVec2fArray& uvs);

    /*! Convert to Matrix4
     * \param from      A USD matrix class instance
     * \return Nuke Matrix4 instance
     */
    template <class T>
    DD::Image::Matrix4 ConvertMatrix4(const T& from);

    /*! Get the corresponding Nuke attribute from the usd attribute
     * \param out       Geometry list
     * \param obj       GeoInfo index
     * \param fromAttr  USD attribute
     * \return The corresponding Nuke attribute
     */
    DD::Image::Attribute* ConstructAttribute(DD::Image::GeometryList& out,
                                             const int obj,
                                             const PXR_NS::UsdAttribute& fromAttr);

    /*! Fill the Nuke display attributes with data for color and uvs
     * \param out       Geometry list to modify
     * \param obj       GeoInfo index to modify
     * \param data      Color and uv data to fill from
     */
    void ConvertColorUvs(DD::Image::GeometryList& out, const int obj,
                         const ColorUvData& data);

    /*! Convert from USD arrays (VtFloatArray, etc) and copy the data into the attributes
     * \param toAttr    Attribute to fill with data
     * \param fromAttr  Attribute to get data from
     * \param offset    Offset into the attribute array to get data from
     * \param stride    Number of elements per offset
     * \param time      Time to evaluate attributes at
     */
    void ConvertValues(DD::Image::Attribute* toAttr,
                       const PXR_NS::UsdAttribute& fromAttr,
                       const PXR_NS::UsdTimeCode time,
                       int offset = -1, int stride = -1);

    /*! Find the attribute index that corresponds to a different level of attribute assignment
     * \param target              The group that will be indexed into
     * \param source              The group that the index comes from
     * \param faceVertexIndices   Relationship of vertices to points
     * \param index               The current index
     * \return The index for the target group
     */
    size_t PromoteAttribute(DD::Image::GroupType target,
                            DD::Image::GroupType source,
                            const PXR_NS::VtUIntArray& faceVertexIndices,
                            size_t index);

    /*! Get a copy of the subset of the array
     * \param source  The array to choose from
     * \param offset  The start of the interval is offset*stride. -1 will return the whole array
     * \param stride  The length of stride that should be chosen. -1 will return the whole array
     * \return copy of the array subset if offset and stride are not -1, or else the whole array
     */
    template <class T>
    T GetOffsetArray(const T& source, int offset, int stride);

    /// Priority of attributes - should a take priority over b
    bool UvOrdering(const PXR_NS::UsdAttribute& a, const PXR_NS::UsdAttribute& b);
  }  //namespace UsdConverter
}  // namespace Foundry

#endif
