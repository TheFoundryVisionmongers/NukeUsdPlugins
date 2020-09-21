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
 \brief UsdConverter attribute conversion unit tests
 */

#include <DDImage/Attribute.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>

#include <catch2/catch.hpp>
#include <iterator>

#include "TestFixtures.h"
#include "UsdConverter/UsdAttrConverter.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace DD::Image;
using namespace Foundry::UsdConverter;

TEST_CASE("USD matrix when converted to Nuke matrix applies same translation")
{
  GfMatrix4d m;
  m.SetTranslate({1.0, 3.0, 5.0});

  Matrix4 converted = ConvertMatrix4(m);
  REQUIRE(converted.translation() == Vector3{1.0f, 3.0f, 5.0f});
}

TEST_CASE("Offset into array")
{
  VtVec3fArray source{{1, 2, 3},    {3, 5, 6},    {7, 8, 9},
                      {10, 11, 12}, {13, 14, 15}, {16, 17, 18}};
  SECTION("Invalid stride and offsets returns source")
  {
    VtVec3fArray result = GetOffsetArray(source, -1, -1);
    CHECK(std::equal(source.begin(), source.end(), result.begin()));
  }
  SECTION("Invalid stride but valid offset returns source")
  {
    VtVec3fArray result = GetOffsetArray(source, -1, 1);
    CHECK(std::equal(source.begin(), source.end(), result.begin()));
  }
  SECTION("Single offset returns just the [1] element")
  {
    VtVec3fArray result = GetOffsetArray(source, 1, 1);
    VtVec3fArray expected{{3, 5, 6}};
    CHECK(std::equal(expected.begin(), expected.end(), result.begin()));
  }
  SECTION("Offset 1 and stride 3 returns the [3-6) elements")
  {
    VtVec3fArray result = GetOffsetArray(source, 1, 3);
    VtVec3fArray expected{{10, 11, 12}, {13, 14, 15}, {16, 17, 18}};
    CHECK(std::equal(expected.begin(), expected.end(), result.begin()));
  }
}

TEST_CASE("ComputePrimvar - type conversion.")
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  SdfPath path("/unit_test");
  UsdGeomMesh fromMesh = UsdGeomMesh::Define(stage, path);
  UsdGeomPrimvarsAPI api(fromMesh);
  SECTION("Less to more precision - same shape")
  {
    VtVec2hArray expected{{1, 2}, {3, 4}};
    UsdGeomPrimvar attribute =
        api.CreatePrimvar(TfToken("half2array"), SdfValueTypeNames->Half2Array);
    attribute.Set(expected);
    VtVec2fArray result;
    ComputePrimvar(result, attribute, UsdTimeCode::Default());
    CHECK(std::equal(expected.begin(), expected.end(), result.begin()));
  }
  SECTION("More to less precision - same shape")
  {
    VtVec2fArray expected{{1, 2}, {3, 4}};
    UsdGeomPrimvar attribute = api.CreatePrimvar(
        TfToken("float2array"), SdfValueTypeNames->Float2Array);
    attribute.Set(expected);
    VtVec2hArray result;
    ComputePrimvar(result, attribute, UsdTimeCode::Default());
    CHECK(std::equal(expected.begin(), expected.end(), result.begin()));
  }
  SECTION("Less to more precision - scalar")
  {
    VtFloatArray expected{{1, 2, 3, 4}};
    UsdGeomPrimvar attribute = api.CreatePrimvar(TfToken("VtFloatArray"),
                                                 SdfValueTypeNames->FloatArray);
    attribute.Set(expected);
    VtDoubleArray result;
    ComputePrimvar(result, attribute, UsdTimeCode::Default());
    CHECK(std::equal(expected.begin(), expected.end(), result.begin()));
  }
  SECTION("Matrix to less precision matrix")
  {
    VtMatrix4dArray expected{
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};
    UsdGeomPrimvar attribute = api.CreatePrimvar(
        TfToken("half2array"), SdfValueTypeNames->Matrix4dArray);
    attribute.Set(expected);
    VtMatrix4fArray result;
    ComputePrimvar(result, attribute, UsdTimeCode::Default());
    CHECK(std::equal(expected.begin(), expected.end(), result.begin()));
  }
}

TEST_CASE("Attribute promotion")
{
  const VtUIntArray faceVertexIndices{1, 2, 3, 6, 7, 8, 10, 11, 12};
  SECTION("Object index from primitive index")
  {
    CHECK(PromoteAttribute(Group_Object, Group_Primitives, faceVertexIndices,
                           5) == 0);
  }
  SECTION("Primitive index from point index")
  {
    CHECK(PromoteAttribute(Group_Primitives, Group_Points, faceVertexIndices,
                           5) == 0);
  }
  SECTION("Point index from vertex index")
  {
    CHECK(PromoteAttribute(Group_Points, Group_Vertices, faceVertexIndices,
                           5) == 8);
  }
  SECTION("Vertex index from vertex index")
  {
    CHECK(PromoteAttribute(Group_Vertices, Group_Vertices, faceVertexIndices,
                           5) == 5);
  }
  SECTION("Object index from vertex index")
  {
    CHECK(PromoteAttribute(Group_Object, Group_Vertices, faceVertexIndices,
                           5) == 0);
  }
}

TEST_CASE_METHOD(MemoryAllocator, "Color conversion")
{
  Attribute Cf(kColorAttrName, VECTOR4_ATTRIB);

  const VtUIntArray faceVertexIndices{0, 1, 2};
  SECTION("Color and opacity are at different attribute groups")
  {
    const VtArray<GfVec3f> color{{1, 0, 0}, {2, 0, 0}, {3, 0, 0}};
    const VtFloatArray opacity{0.5};
    GroupType colorGroup = Group_Points;
    GroupType opacityGroup = Group_Object;

    ConvertColor(Cf, color, colorGroup, opacity, opacityGroup,
                 faceVertexIndices);
    const std::vector<Vector4> expectedResults{
        {1, 0, 0, 0.5}, {2, 0, 0, 0.5}, {3, 0, 0, 0.5}};
    CHECK(std::equal(expectedResults.begin(), expectedResults.end(),
                     Cf.vector4_list->begin()));
  }
  SECTION("Color and opacity are at the same attribute groups")
  {
    const VtArray<GfVec3f> color{{1, 0, 0}, {2, 0, 0}, {3, 0, 0}};
    const VtFloatArray opacity{0.5f, 0.3f, 0.9f};
    GroupType colorGroup = Group_Points;
    GroupType opacityGroup = Group_Points;

    ConvertColor(Cf, color, colorGroup, opacity, opacityGroup,
                 faceVertexIndices);
    const std::vector<Vector4> expectedResults{
        {1.0f, 0, 0, 0.5f}, {2.0f, 0, 0, 0.3f}, {3.0f, 0, 0, 0.9f}};
    CHECK(std::equal(expectedResults.begin(), expectedResults.end(),
                     Cf.vector4_list->begin()));
  }

  SECTION("Only one color value")
  {
    const VtArray<GfVec3f> color{{1, 0, 0}};
    const VtFloatArray opacity{0.5f, 0.3f, 0.9f};
    GroupType colorGroup = Group_Points;
    GroupType opacityGroup = Group_Points;

    ConvertColor(Cf, color, colorGroup, opacity, opacityGroup,
                 faceVertexIndices);
    const std::vector<Vector4> expectedResults{
        {1.0f, 0, 0, 0.5f}, {1.0f, 0, 0, 0.3f}, {1.0f, 0, 0, 0.9f}};
    CHECK(std::equal(expectedResults.begin(), expectedResults.end(),
                     Cf.vector4_list->begin()));
  }
}

TEST_CASE("UV Attribute ordering")
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  SdfPath path("/unit_test");
  UsdGeomMesh fromMesh = UsdGeomMesh::Define(stage, path);
  UsdGeomPrimvarsAPI api(fromMesh);
  UsdGeomPrimvar a = api.CreatePrimvar(TfToken("a"), SdfValueTypeNames->Float2);
  UsdGeomPrimvar b = api.CreatePrimvar(TfToken("b"), SdfValueTypeNames->Float2);
  UsdGeomPrimvar st =
      api.CreatePrimvar(TfToken("st"), SdfValueTypeNames->Float2);
  UsdGeomPrimvar z = api.CreatePrimvar(TfToken("z"), SdfValueTypeNames->Float2);

  CHECK(UvOrdering(a, b));
  CHECK(UvOrdering(b, a) == false);
  CHECK(UvOrdering(st, a) == true);
  CHECK(UvOrdering(st, z) == true);
}

TEST_CASE_METHOD(MemoryAllocator, "UV conversion")
{
  Attribute attribute(kUVAttrName, VECTOR4_ATTRIB);
  SECTION("No existing uvs")
  {
    const VtVec2fArray uvs{{0.1f, 0.2f}, {0.3f, 0.4f}, {0.5f, 0.6f}};
    ConvertUvs(attribute, uvs);
    REQUIRE(attribute.vector4_list->size() == 3);
    auto expected_it = uvs.cbegin();
    auto result_it = attribute.vector4_list->begin();
    for(; expected_it != uvs.cend(); ++expected_it, ++result_it) {
      CHECK((*expected_it)[0] == (*result_it)[0]);
      CHECK((*expected_it)[1] == (*result_it)[1]);
      CHECK((*result_it)[2] == 0.0f);
      CHECK((*result_it)[3] == 1.0f);
    }
  }
  SECTION("Uvs already exist")
  {
    const VtVec2fArray uvs{{0.1f, 0.2f}, {0.3f, 0.4f}, {0.5f, 0.6f}};
    attribute.vector4_list->resize(4);
    std::fill(attribute.vector4_list->begin(), attribute.vector4_list->end(),
              Vector4{1.0f, 2.0f, 3.0f, 4.0f});
    ConvertUvs(attribute, uvs);
    REQUIRE(attribute.vector4_list->size() == 3);
    auto expected_it = uvs.cbegin();
    auto result_it = attribute.vector4_list->begin();
    for(; expected_it != uvs.cend(); ++expected_it, ++result_it) {
      CHECK((*expected_it)[0] == (*result_it)[0]);
      CHECK((*expected_it)[1] == (*result_it)[1]);
      CHECK((*result_it)[2] == 0.0f);
      CHECK((*result_it)[3] == 1.0f);
    }
  }
}

TEST_CASE_METHOD(MemoryAllocator, "Value conversion from USD to attribute")
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  SdfPath path("/unit_test");
  UsdGeomMesh fromMesh = UsdGeomMesh::Define(stage, path);

  SECTION("Point based normal")
  {
    const VtVec3fArray normals{{-3, 1, 1}, {1, -3, 1}, {1, 1, 1}, {2, -3, 2}};
    UsdAttribute a_normals = fromMesh.CreateNormalsAttr(VtValue(), false);
    a_normals.Set(normals, pxr::UsdTimeCode::Default());
    TfToken name = ConvertName(a_normals);
    CHECK(name.GetString().compare("N") == 0);

    GroupType group = ConvertGroupType(a_normals);
    CHECK(group == Group_Points);
  }

  SECTION("Vertex based normal")
  {
    const VtVec3fArray normals{{-3, 1, 1}, {1, -3, 1}, {1, 1, 1},
                               {2, -3, 2}, {5, -5, 6}, {7, -8, 9}};
    UsdAttribute a_normals = fromMesh.CreateNormalsAttr(VtValue(), false);
    a_normals.Set(normals, pxr::UsdTimeCode::Default());
    fromMesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);

    GroupType group = ConvertGroupType(a_normals);
    CHECK(group == Group_Vertices);
  }

  SECTION("Point velocities")
  {
    const VtVec3fArray vels{
        {10, 20, 30}, {40, 50, 60}, {70, 80, 90}, {-100, -110, -120}};
    UsdAttribute fromAttr = fromMesh.CreateVelocitiesAttr();
    fromAttr.Set(vels, pxr::UsdTimeCode::Default());

    GroupType group = ConvertGroupType(fromAttr);
    REQUIRE(group == Group_Points);

    AttribType attrType = ConvertAttribType(fromAttr);
    REQUIRE(attrType == VECTOR3_ATTRIB);

    TestGeoOp geo;

    int obj = geo.geometryList()->size();
    geo.geometryList()->add_object(obj);
    geo.geometryList()->add_primitive(obj);
    Attribute* toAttr = geo.geometryList()->writable_attribute(
        obj, group, kVelocityAttrName, attrType);

    ConvertValues(toAttr, fromAttr, UsdTimeCode::Default());

    auto from_it = vels.cbegin();
    auto to_it = toAttr->vector3_list->cbegin();
    for(; from_it != vels.cend(); ++from_it, ++to_it) {
      CHECK(std::equal(from_it->GetArray(), from_it->GetArray() + 3,
                       to_it->array()));
    }
    REQUIRE(toAttr->vector3_list->size() == 4);
  }
}
