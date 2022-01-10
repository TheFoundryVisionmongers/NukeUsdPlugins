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
 \brief Implementation file for UsdConverter attribute conversion functions
 */

#include "UsdConverter/UsdAttrConverter.h"

#include <boost/preprocessor/seq/for_each.hpp>
#include <DDImage/Attribute.h>
#include <DDImage/GeometryList.h>
#include <pxr/base/gf/matrix2f.h>
#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointBased.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <unordered_map>

using namespace DD::Image;
namespace Foundry
{
  namespace UsdConverter
  {
    PXR_NAMESPACE_USING_DIRECTIVE

    GroupType ConvertGroupType(const UsdAttribute& fromAttr);
    const struct
    {
      TfToken N{kNormalAttrName};
      TfToken Cf{kColorAttrName};
      TfToken PW{kPWAttrName};
      TfToken vel{kVelocityAttrName};
      TfToken size{kSizeAttrName};
      TfToken uv{kUVAttrName};
    } nukeTokens;

    const struct
    {
      TfToken st{"primvars:st"};
    } usdTokens;

    // Special names that are used by Nuke
    const std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>
        mappedNames{{usdTokens.st, nukeTokens.uv},
                    {UsdGeomTokens->normals, nukeTokens.N},
                    {UsdGeomTokens->primvarsDisplayColor, nukeTokens.Cf},
                    {UsdGeomTokens->pointWeights, nukeTokens.PW},
                    {UsdGeomTokens->velocities, nukeTokens.vel},
                    {UsdGeomTokens->widths, nukeTokens.size}};

    // Mapping to Nuke GroupType using interpolation
    const std::unordered_map<TfToken, GroupType, TfToken::HashFunctor>
        mappedGroups{{UsdGeomTokens->constant, Group_Object},
                     {UsdGeomTokens->uniform, Group_Primitives},
                     {UsdGeomTokens->vertex, Group_Points},
                     {UsdGeomTokens->faceVarying, Group_Vertices}};

    // Non usdgeomprimvar values won't have an interpolation, so use the role instead
    const std::unordered_map<TfToken, GroupType, TfToken::HashFunctor>
        mappedRoles{
            {SdfValueRoleNames->Point, Group_Points},
            {SdfValueRoleNames->Vector, Group_Points},
            {SdfValueRoleNames->Color, Group_Points},
            {SdfValueRoleNames->Frame, Group_Object},
            {SdfValueRoleNames->Transform, Group_Object},  // deprecated in USD
            {SdfValueRoleNames->TextureCoordinate,
             Group_Vertices}  // deprecated in USD
        };

    const std::unordered_set<SdfValueTypeName, SdfValueTypeNameHash>
        textureTypes{SdfValueTypeNames->TexCoord2d,
                     SdfValueTypeNames->TexCoord2f};

    // Value types of USD arrays mapped to Nuke AttribType
    const std::unordered_map<SdfValueTypeName, AttribType, SdfValueTypeNameHash>
        mappedAttribTypes{{SdfValueTypeNames->Bool, INT_ATTRIB},
                          {SdfValueTypeNames->UChar, INT_ATTRIB},
                          {SdfValueTypeNames->Int, INT_ATTRIB},
                          {SdfValueTypeNames->UInt, INT_ATTRIB},
                          {SdfValueTypeNames->Int64, INT_ATTRIB},
                          {SdfValueTypeNames->Half, FLOAT_ATTRIB},
                          {SdfValueTypeNames->Float, FLOAT_ATTRIB},
                          {SdfValueTypeNames->Double, FLOAT_ATTRIB},
                          {SdfValueTypeNames->String, STRING_ATTRIB},
                          {SdfValueTypeNames->Token, STRING_ATTRIB},
                          {SdfValueTypeNames->Matrix3d, MATRIX3_ATTRIB},
                          {SdfValueTypeNames->Matrix4d, MATRIX4_ATTRIB},
                          {SdfValueTypeNames->Int2, VECTOR2_ATTRIB},
                          {SdfValueTypeNames->Half2, VECTOR2_ATTRIB},
                          {SdfValueTypeNames->Float2, VECTOR2_ATTRIB},
                          {SdfValueTypeNames->Double2, VECTOR2_ATTRIB},
                          {SdfValueTypeNames->Int3, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Half3, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Float3, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Vector3h, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Vector3f, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Vector3d, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Color3h, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Color3f, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Color3d, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Normal3h, NORMAL_ATTRIB},
                          {SdfValueTypeNames->Normal3f, NORMAL_ATTRIB},
                          {SdfValueTypeNames->Normal3d, NORMAL_ATTRIB},
                          {SdfValueTypeNames->Point3h, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Point3f, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Point3d, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Double3, VECTOR3_ATTRIB},
                          {SdfValueTypeNames->Int4, VECTOR4_ATTRIB},
                          {SdfValueTypeNames->Half4, VECTOR4_ATTRIB},
                          {SdfValueTypeNames->Float4, VECTOR4_ATTRIB},
                          {SdfValueTypeNames->Double4, VECTOR4_ATTRIB}};

    /// Get attr value at time. Template expression enables this function for types that attributes can hold
    template <class DEST,
              std::enable_if_t<SdfValueTypeTraits<DEST>::IsValueType, int> = 0>
    bool _ComputePrimvar(DEST& v, const UsdAttribute& attr, UsdTimeCode time)
    {
      bool result = false;
      // Compute the value at the requested time
      if(UsdGeomPrimvar::IsPrimvar(attr)) {
        result = UsdGeomPrimvar(attr).ComputeFlattened(&v, time);
      }
      if(!result) {
        result = attr.Get(&v, time);
      }
      return result;
    };

    // Negative counterpart to above. Can't compute primvar if it isn't a type that attributes can hold
    template <
        class DEST,
        std::enable_if_t<!(SdfValueTypeTraits<DEST>::IsValueType), int> = 0>
    bool _ComputePrimvar(DEST&, const UsdAttribute&, UsdTimeCode)
    {
      return false;
    };

    // Vector->vector
    template <
        class DEST, class SOURCE,
        std::enable_if_t<GfIsGfVec<typename SOURCE::value_type>::value, int> =
            0,
        std::enable_if_t<GfIsGfVec<typename DEST::value_type>::value, int> = 0>
    void _CopyToVt(DEST& destination, const SOURCE& source)
    {
      destination.resize(source.size());
      auto destination_it = destination.begin();
      for(const typename SOURCE::value_type& elem : source) {
        std::copy(elem.data(), elem.data() + elem.dimension,
                  destination_it->data());
        ++destination_it;
      }
    }

    // Matrix->matrix
    template <class DEST, class SOURCE,
              std::enable_if_t<GfIsGfMatrix<typename SOURCE::value_type>::value,
                               int> = 0,
              std::enable_if_t<GfIsGfMatrix<typename DEST::value_type>::value,
                               int> = 0>
    void _CopyToVt(DEST& destination, const SOURCE& source)
    {
      destination.resize(source.size());
      auto destination_it = destination.begin();
      for(const typename SOURCE::value_type& elem : source) {
        std::copy(elem.data(),
                  elem.data() + SOURCE::value_type::numRows *
                                    SOURCE::value_type::numColumns,
                  destination_it->data());
        ++destination_it;
      }
    }

    // Neither, just copy
    template <
        class DEST, class SOURCE,
        std::enable_if_t<!(GfIsGfMatrix<typename SOURCE::value_type>::value) &&
                             !(GfIsGfVec<typename SOURCE::value_type>::value),
                         int> = 0,
        std::enable_if_t<!(GfIsGfMatrix<typename DEST::value_type>::value) &&
                             !(GfIsGfVec<typename DEST::value_type>::value),
                         int> = 0>
    void _CopyToVt(DEST& destination, const SOURCE& source)
    {
      std::copy(source.begin(), source.end(), std::back_inserter(destination));
    }

    // Mismatch - do nothing
    template <
        class DEST, class SOURCE,
        std::enable_if_t<GfIsGfVec<typename SOURCE::value_type>::value !=
                                 GfIsGfVec<typename DEST::value_type>::value ||
                             GfIsGfMatrix<typename SOURCE::value_type>::value !=
                                 GfIsGfMatrix<typename DEST::value_type>::value,
                         int> = 0>
    void _CopyToVt(DEST&, const SOURCE&)
    {
    }

    template <class DEST, class SOURCE>
    void _Convert(DEST& value, const UsdAttribute& attr, UsdTimeCode time)
    {
      if(!attr.GetTypeName().GetType().IsA<SOURCE>()) {
        return;
      }

      SOURCE converted;
      _ComputePrimvar(converted, attr, time);
      _CopyToVt<DEST, SOURCE>(value, converted);
    }

    template <class DEST>
    void ComputePrimvar(DEST& value, const UsdAttribute& attr, UsdTimeCode time)
    {
      if(attr.GetTypeName().GetType().IsA<DEST>()) {
        _ComputePrimvar(value, attr, time);
        return;
      }

      // The wrong type was requested, find the right type and then copy it.
      // If the types aren't matching correctly, such as a mismatch between float and double types,
      // then this series of template calls will try and match and fill the VtArray type with the data from the attribute.
#define VT_ARRAY_NAME(elem) BOOST_PP_TUPLE_ELEM(2, 0, elem)
#define TRY_CONVERT(r, unused, elem) \
  _Convert<DEST, VtArray<VT_ARRAY_NAME(elem)>>(value, attr, time);
      BOOST_PP_SEQ_FOR_EACH(TRY_CONVERT, ~, VT_VEC_VALUE_TYPES)
      BOOST_PP_SEQ_FOR_EACH(TRY_CONVERT, ~, VT_MATRIX_VALUE_TYPES)
      BOOST_PP_SEQ_FOR_EACH(TRY_CONVERT, ~, VT_BUILTIN_NUMERIC_VALUE_TYPES)
#undef TRY_CONVERT
    }

// Generate templates
#define DECLARE_COMPUTE_PRIMVAR(r, unused, elem)              \
  template void ComputePrimvar<VtArray<VT_ARRAY_NAME(elem)>>( \
      VtArray<VT_ARRAY_NAME(elem)>&, const UsdAttribute&, UsdTimeCode);
    BOOST_PP_SEQ_FOR_EACH(DECLARE_COMPUTE_PRIMVAR, ~, VT_VEC_VALUE_TYPES)
    BOOST_PP_SEQ_FOR_EACH(DECLARE_COMPUTE_PRIMVAR, ~, VT_MATRIX_VALUE_TYPES)
    BOOST_PP_SEQ_FOR_EACH(DECLARE_COMPUTE_PRIMVAR, ~,
                          VT_BUILTIN_NUMERIC_VALUE_TYPES)
#undef DECLARE_COMPUTE_PRIMVAR
#undef VT_ARRAY_NAME

    template <class T>
    T GetOffsetArray(const T& source, int offset, int stride)
    {
      if(offset == -1 || stride == -1) {
        return source;
      }
      auto end = static_cast<size_t>(stride * (offset + 1));
      if(source.size() < end) {
        return T();
      }
      auto begin = source.begin() + offset * stride;
      T newUvs(begin, source.begin() + end);
      return newUvs;
    }

    template VtVec3fArray GetOffsetArray<VtVec3fArray>(const VtVec3fArray&, int,
                                                       int);

    ColorUvData::ColorUvData(const ColorUvData& other, int offset)
    {
      uvs = GetOffsetArray(other.uvs, offset, other.uvElementSize);

      if(other.color.size() >
         static_cast<size_t>((offset + 1) * other.colorElementSize)) {
        color.assign(
            other.color.begin() + offset * other.colorElementSize,
            other.color.begin() + (offset + 1) * other.colorElementSize);
      }
      if(other.opacity.size() >
         static_cast<size_t>((offset + 1) * other.opacityElementSize)) {
        opacity.assign(
            other.opacity.begin() + offset * other.opacityElementSize,
            other.opacity.begin() + (offset + 1) * other.opacityElementSize);
      }
      faceVertexIndices = other.faceVertexIndices;
      uvElementSize = other.uvElementSize;
      colorElementSize = other.colorElementSize;
      opacityElementSize = other.opacityElementSize;
      uvGroup = other.uvGroup;
      colorGroup = other.colorGroup;
      opacityGroup = other.opacityGroup;
    }

    static const std::vector<GroupType> groupTypeOrder{
        Group_None, Group_Object, Group_Primitives, Group_Points,
        Group_Vertices};

    std::vector<GroupType>::difference_type ordering(GroupType group)
    {
      return std::distance(groupTypeOrder.begin(),
                           std::find(std::begin(groupTypeOrder),
                                     std::end(groupTypeOrder), group));
    };

    size_t PromoteAttribute(GroupType target, GroupType source,
                            const VtUIntArray& faceVertexIndices,
                            size_t index)
    {
      const auto start = ordering(target);
      auto end = ordering(source);

      // Move index to the required level
      for(; start < end; --end) {
        switch(groupTypeOrder[static_cast<std::vector<GroupType>::size_type>(
            end)]) {
          case Group_Vertices:
            if(index >= faceVertexIndices.size()) {
              continue;  // There's no point index for this vertex
            }
            index = faceVertexIndices[index];  // Point index
            break;
          case Group_Points:
          case Group_Primitives:
            index =
                0;  // This is the primitive number, since we're making one nuke primitive per usd primitive it's just 0
            break;
          default:
            break;
        }
      }
      return index;
    };

    namespace
    {
      const VtArray<GfVec3f> kDefaultColorValues{{0.5f, 0.5f, 0.5f}};
      const VtFloatArray kDefaultOpacityValues{1.0f};
    }  // namespace

    void ConvertColor(Attribute& Cf, const VtArray<GfVec3f>& color,
                      GroupType colorGroup, const VtFloatArray& opacity,
                      GroupType opacityGroup,
                      const VtUIntArray& faceVertexIndices)
    {
      GroupType maxGroup = groupTypeOrder[std::max(ordering(colorGroup),
                                                   ordering(opacityGroup))];

      auto size = std::max(color.size(), opacity.size());
      Cf.resize(size);
      for(size_t i = 0; i < size; ++i) {
        Vector4& c = Cf.vector4(i);
        auto colorIndex = std::min(
            PromoteAttribute(colorGroup, maxGroup, faceVertexIndices, i),
            color.size() - 1);
        // There can be a mismatch between the GroupType (Nuke) and USD interpolation for color and opacity, as they're stored in USD as separate
        // attributes. For example, if the color is provided in USD as faceVarying (equivalent to Nuke vertex) but the opacity is constant in USD
        // (equivalent to Nuke object level). Since vertex is a finer level of detail than object, the data would be stored per vertex in Nuke,
        // but the object level opacity would be copied multiple times.
        auto opacityIndex = std::min(
            PromoteAttribute(opacityGroup, maxGroup, faceVertexIndices, i),
            opacity.size() - 1);
        c[0] = color[colorIndex][0];
        c[1] = color[colorIndex][1];
        c[2] = color[colorIndex][2];
        c[3] = opacity[opacityIndex];
      }
    }

    void ConvertUvs(Attribute& toAttr, const VtVec2fArray& uvs)
    {
      toAttr.clear();
      toAttr.reserve(uvs.size());
      for(const auto& fromVec : uvs) {
        // Nuke stores UVS as 4 floats to match an OpenGL fixed pipeline call later.
        toAttr.vector4_list->emplace_back(fromVec[0], fromVec[1], 0.0f, 1.0f);
      }
    }

    bool UvOrdering(const UsdAttribute& a, const UsdAttribute& b)
    {
      if(a.GetName() == usdTokens.st) {
        return true;
      }
      return a < b;
    };

    size_t ConvertPoints(GeometryList& out, const int obj,
                         const UsdAttribute& fromAttr, const UsdTimeCode time)
    {
      VtVec3fArray points;
      ComputePrimvar(points, fromAttr, time);
      if(!points.empty()) {
        PointList* toPoints = out.writable_points(obj);
        toPoints->clear();
        toPoints->reserve(points.size());
        for(const auto& from : points) {
          toPoints->emplace_back(from[0], from[1], from[2]);
        }
      }
      return points.size();
    }

    void ConvertColorUvs(GeometryList& out, const int obj, const ColorUvData& data)
    {
      if(data.uvs.size() > 0) {
        Attribute* toUv = out.writable_attribute(obj, data.uvGroup, kUVAttrName,
                                                 VECTOR4_ATTRIB);
        ConvertUvs(*toUv, data.uvs);
      }

      GroupType maxGroup = groupTypeOrder[std::max(
          ordering(data.colorGroup), ordering(data.opacityGroup))];
      if(maxGroup != Group_None) {  // Neither color or opacity was set
        Attribute* Cf = out.writable_attribute(
            obj, maxGroup, nukeTokens.Cf.GetText(), VECTOR4_ATTRIB);
        ConvertColor(
            *Cf, data.color.size() > 0 ? data.color : kDefaultColorValues,
            data.colorGroup,
            data.opacity.size() > 0 ? data.opacity : kDefaultOpacityValues,
            data.opacityGroup, data.faceVertexIndices);
      }
    }

    std::vector<UsdAttribute> ConvertMismatchedAttributes(
        ColorUvData& data, const std::vector<UsdAttribute>& attrs,
        const UsdTimeCode time)
    {
      std::vector<UsdAttribute> unhandledAttributes;

      std::set<UsdAttribute, decltype(&UvOrdering)> uvAttrs(&UvOrdering);
      for(auto& fromAttr : attrs) {
        if(fromAttr.GetName() == UsdGeomTokens->points) {
        }
        else if(fromAttr.GetName() == UsdGeomTokens->primvarsDisplayColor) {
          data.colorGroup = ConvertGroupType(fromAttr);
          ComputePrimvar(data.color, fromAttr, time);
          data.colorElementSize = UsdGeomPrimvar(fromAttr).GetElementSize();
        }
        else if(fromAttr.GetName() == UsdGeomTokens->primvarsDisplayOpacity) {
          data.opacityGroup = ConvertGroupType(fromAttr);
          ComputePrimvar(data.opacity, fromAttr, time);
          data.opacityElementSize = UsdGeomPrimvar(fromAttr).GetElementSize();
        }
        else if(fromAttr.GetName() == UsdGeomTokens->faceVertexIndices) {
          ComputePrimvar(data.faceVertexIndices, fromAttr, time);
        }
        else if(fromAttr.GetName() == usdTokens.st ||
                (textureTypes.find(fromAttr.GetTypeName().GetScalarType()) !=
                 textureTypes.cend())) {
          uvAttrs.insert(fromAttr);
        }
        else {
          unhandledAttributes.push_back(fromAttr);
        }
      }

      if(!uvAttrs.empty()) {
        const UsdAttribute& fromUv = *(uvAttrs.begin());
        data.uvGroup = ConvertGroupType(fromUv);
        ComputePrimvar(data.uvs, fromUv, time);
        data.uvElementSize = UsdGeomPrimvar(fromUv).GetElementSize();
      }
      return unhandledAttributes;
    }

    TfToken ConvertName(const UsdAttribute& fromAttr)
    {
      const auto it_name = mappedNames.find(fromAttr.GetName());
      if(it_name != mappedNames.cend()) {
        return it_name->second;
      }
      else {
        return fromAttr.GetName();
      }
    }

    AttribType ConvertAttribType(const UsdAttribute& fromAttr)
    {
      SdfValueTypeName type = fromAttr.GetTypeName();
      const auto it_type = mappedAttribTypes.find(type.GetScalarType());
      if(it_type != mappedAttribTypes.cend()) {
        return it_type->second;
      }
      else {
        return INVALID_ATTRIB;
      }
    }

    GroupType ConvertGroupType(const UsdAttribute& fromAttr)
    {
      if(fromAttr.GetName() == UsdGeomTokens->normals) {
        // Normals aren't primvars but still have an interpolation
        UsdGeomPointBased geom(fromAttr.GetPrim());
        TfToken interpolation = geom.GetNormalsInterpolation();
        const auto it_interpolation = mappedGroups.find(interpolation);
        if(it_interpolation != mappedGroups.cend()) {
          return it_interpolation->second;
        }
        return Group_Points;
      }
      if(UsdGeomPrimvar::IsPrimvar(fromAttr)) {
        TfToken interpolation = UsdGeomPrimvar(fromAttr).GetInterpolation();
        const auto it_interpolation = mappedGroups.find(interpolation);

        if(it_interpolation != mappedGroups.cend()) {
          return it_interpolation->second;
        }
      }
      TfToken role = fromAttr.GetRoleName();
      const auto it_role = mappedRoles.find(role);

      if(it_role != mappedRoles.cend()) {
        return it_role->second;
      }
      else {
        return Group_Object;
      }
    }

    template <class T>
    Matrix4 ConvertMatrix4(const T& from)
    {
      return Matrix4(from[0][0], from[1][0], from[2][0], from[3][0], from[0][1],
                     from[1][1], from[2][1], from[3][1], from[0][2], from[1][2],
                     from[2][2], from[3][2], from[0][3], from[1][3], from[2][3],
                     from[3][3]);
    }

    template Matrix4 ConvertMatrix4<GfMatrix4d>(const GfMatrix4d&);

    template <class T>
    void FillNumericValue(Attribute* toAttr, const T& vals)
    {
      toAttr->clear();
      switch(toAttr->type()) {
        case FLOAT_ATTRIB: {
          toAttr->float_list->assign(vals.cbegin(), vals.cend());
          break;
        }
        case INT_ATTRIB: {
          toAttr->int_list->assign(vals.cbegin(), vals.cend());
          break;
        }
        default:
          return;
      }
    }

    void FillStringValue(Attribute* toAttr, const std::string& vals)
    {
      toAttr->clear();
      toAttr->std_string_list->push_back(vals);
    }

    template <class T>
    void FillVectorValue(Attribute* toAttr, const T& vals)
    {
      toAttr->clear();
      switch(toAttr->type()) {
        case VECTOR2_ATTRIB: {
          toAttr->vector2_list->reserve(vals.size());
          for(const auto& fromVec : vals) {
            toAttr->vector2_list->emplace_back(fromVec[0], fromVec[1]);
          }
          break;
        }
        // Normals are Vector3s
        case NORMAL_ATTRIB:
        case VECTOR3_ATTRIB: {
          toAttr->vector3_list->reserve(vals.size());
          for(const auto& fromVec : vals) {
            toAttr->vector3_list->emplace_back(fromVec[0], fromVec[1],
                                               fromVec[2]);
          }
          break;
        }
        case VECTOR4_ATTRIB: {
          toAttr->vector4_list->reserve(vals.size());
          for(const auto& fromVec : vals) {
            toAttr->vector4_list->emplace_back(fromVec[0], fromVec[1],
                                               fromVec[2], fromVec[3]);
          }
          break;
        }
        default:
          return;
      }
    }

    template <class T>
    void FillMatrixValue(Attribute* toAttr, const T& vals)
    {
      toAttr->clear();
      switch(toAttr->type()) {
        case MATRIX3_ATTRIB: {
          toAttr->matrix3_list->reserve(vals.size());
          std::transform(vals.begin(), vals.end(),
                         toAttr->matrix3_list->begin(), [](const auto& from) {
                           return Matrix3(from[0][0], from[1][0], from[2][0],
                                          from[0][1], from[1][1], from[2][1],
                                          from[0][2], from[1][2], from[2][2]);
                         });
          break;
        }
        case MATRIX4_ATTRIB: {
          toAttr->matrix4_list->reserve(vals.size());
          std::transform(vals.begin(), vals.end(),
                         toAttr->matrix4_list->begin(),
                         [](const auto& from) { return ConvertMatrix4(from); });
          break;
        }
        default:
          return;
      }
    }

    void ConvertValues(Attribute* toAttr, const UsdAttribute& fromAttr,
                       const UsdTimeCode time, int offset, int stride)
    {
      switch(toAttr->type()) {
        case INT_ATTRIB: {
          VtIntArray vals;
          ComputePrimvar(vals, fromAttr, time);
          FillNumericValue(toAttr, GetOffsetArray(vals, offset, stride));
          break;
        }
        case FLOAT_ATTRIB: {
          VtFloatArray vals;
          ComputePrimvar(vals, fromAttr, time);
          FillNumericValue(toAttr, GetOffsetArray(vals, offset, stride));
          break;
        }
        case VECTOR2_ATTRIB: {
          VtVec2fArray vals;
          ComputePrimvar(vals, fromAttr, time);
          FillVectorValue(toAttr, GetOffsetArray(vals, offset, stride));
          break;
        }
        // Normals are Vector3s
        case NORMAL_ATTRIB:
        case VECTOR3_ATTRIB: {
          VtVec3fArray vals;
          ComputePrimvar(vals, fromAttr, time);
          FillVectorValue(toAttr, GetOffsetArray(vals, offset, stride));
          break;
        }
        case VECTOR4_ATTRIB: {
          VtVec4fArray vals;
          ComputePrimvar(vals, fromAttr, time);
          FillVectorValue(toAttr, GetOffsetArray(vals, offset, stride));
          break;
        }
        case MATRIX3_ATTRIB: {
          VtArray<GfMatrix3d> vals;
          ComputePrimvar(vals, fromAttr, time);
          FillMatrixValue(toAttr, GetOffsetArray(vals, offset, stride));
          break;
        }
        case MATRIX4_ATTRIB: {
          VtArray<GfMatrix4d> vals;
          ComputePrimvar(vals, fromAttr, time);
          FillMatrixValue(toAttr, GetOffsetArray(vals, offset, stride));
          break;
        }
        default:
          return;
      }
    }

    void ConvertPrimPath(GeometryList& out, int obj, const UsdPrim& prim)
    {
      Attribute* attr = out.writable_attribute(obj, Group_Object, kNameAttrName,
                                               STD_STRING_ATTRIB);
      FillStringValue(attr, prim.GetPath().GetString());
    }

    Attribute* ConstructAttribute(GeometryList& out, const int obj,
                                  const UsdAttribute& fromAttr)
    {
      if(!fromAttr.HasValue()) {
        return nullptr;
      }

      TfToken name = ConvertName(fromAttr);
      if(name == fromAttr.GetName()) {
        return nullptr;
      }

      GroupType group = ConvertGroupType(fromAttr);
      AttribType attrType = ConvertAttribType(fromAttr);
      if(attrType == INVALID_ATTRIB) {
        return nullptr;
      }

      return out.writable_attribute(obj, group, name.GetText(), attrType);
    }

    FN_USDCONVERTER_API void ConvertUsdAttributes(
        GeometryList& out, const int obj,
        const std::vector<UsdAttribute>& primvars, const UsdTimeCode time)
    {
      ColorUvData data;
      // Convert attributes first that don't map to Nuke ones directly, then convert what remains
      UsdAttributeVector remainingAttributes =
          ConvertMismatchedAttributes(data, primvars, time);
      ConvertColorUvs(out, obj, data);
      for(auto& fromAttr : remainingAttributes) {
        Attribute* toAttr = ConstructAttribute(out, obj, fromAttr);
        if(!toAttr) {
          continue;
        }
        ConvertValues(toAttr, fromAttr, time);
      }
    }
  }  // namespace UsdConverter
}  // namespace Foundry
