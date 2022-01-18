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
 \brief UsdConverter geometry conversion unit tests
 */

#include <DDImage/GeoOp.h>
#include <DDImage/GeometryList.h>
#include <DDImage/PolyMesh.h>
#include <DDImage/Scene.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <catch2/catch.hpp>

#include "TestFixtures.h"
#include "UsdConverter/UsdGeoConverter.h"
#include "UsdConverter/UsdUI.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace DD::Image;
using namespace Foundry::UsdConverter;

void checkFaceVertexAssignment(
    std::unique_ptr<PolyMesh>& toMesh, int face,
    const std::vector<unsigned>& expectedFaceVertices)
{
  std::vector<unsigned> faceVertices(expectedFaceVertices.size());
  toMesh->get_face_vertices(face, &faceVertices.front());
  for(size_t i = 0; i < expectedFaceVertices.size(); ++i) {
    CHECK(toMesh->vertex(faceVertices[i]) == expectedFaceVertices[i]);
  }
}

TEST_CASE_METHOD(MemoryAllocator, "Get prims from stage")
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomMesh::Define(stage, SdfPath("/A"));
  UsdGeomMesh::Define(stage, SdfPath("/B"));
  UsdGeomMesh::Define(stage, SdfPath("/B/C"));
  TestGeoOp geo;
  convertUsdGeometry(*geo.geometryList(), stage);
  REQUIRE(geo.geometryList()->size() == 3);
}

auto CreateTestGeometryMesh(UsdStageRefPtr& stage, const SdfPath& path)
{
  UsdGeomMesh fromMesh = UsdGeomMesh::Define(stage, path);
  VtVec3fArray points{{-430, -145, 0}, {430, -145, 0},  {430, 145, 0},
                      {-430, 145, 0},  {430, 145, -10}, {-430, 145, -10}};
  UsdAttribute a_points = fromMesh.CreatePointsAttr(VtValue(), false);
  a_points.Set(points, pxr::UsdTimeCode::Default());

  const VtIntArray faceVertexCounts{4, 3};
  UsdAttribute a_faceVertexCounts =
      fromMesh.CreateFaceVertexCountsAttr(VtValue(), false);
  a_faceVertexCounts.Set(faceVertexCounts, pxr::UsdTimeCode::Default());

  const VtIntArray faceVertexIndices{0, 1, 2, 3, 3, 4, 5};
  UsdAttribute a_faceVertexIndicies =
      fromMesh.CreateFaceVertexIndicesAttr(VtValue(), false);
  a_faceVertexIndicies.Set(faceVertexIndices, pxr::UsdTimeCode::Default());

  const VtArray<GfVec3f> extent{{-430, -145, -10}, {430, 145, 10}};
  UsdAttribute a_extent = fromMesh.CreateExtentAttr(VtValue(), false);
  a_extent.Set(extent, pxr::UsdTimeCode::Default());
  return std::make_tuple(fromMesh, points);
}

TEST_CASE("Prim conversions")
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  SdfPath path("/unit_test");

  SECTION("UsdGeomMesh")
  {
    UsdGeomMesh fromMesh;
    VtVec3fArray points;
    std::tie(fromMesh, points) = CreateTestGeometryMesh(stage, path);
    MemoryAllocator context;
    SECTION("Convert right handed mesh - no authored orientation")
    {
      std::unique_ptr<PolyMesh> toMesh =
          convertUsdPrim<UsdGeomMesh, PolyMesh>(fromMesh);
      REQUIRE(toMesh->faces() == 2);
      REQUIRE(toMesh->face_vertices(0) == 4);
      REQUIRE(toMesh->face_vertices(1) == 3);
      REQUIRE(toMesh->vertices() == 7);

      checkFaceVertexAssignment(toMesh, 0, {0, 1, 2, 3});
      checkFaceVertexAssignment(toMesh, 1, {3, 4, 5});
    }
    SECTION("Convert left handed mesh - orientation authored")
    {
      UsdAttribute a_orientation = fromMesh.CreateOrientationAttr();
      a_orientation.Set(UsdGeomTokens->leftHanded);
      std::unique_ptr<PolyMesh> toMesh =
          convertUsdPrim<UsdGeomMesh, PolyMesh>(fromMesh);
      REQUIRE(toMesh->faces() == 2);
      REQUIRE(toMesh->face_vertices(0) == 4);
      REQUIRE(toMesh->face_vertices(1) == 3);
      REQUIRE(toMesh->vertices() == 7);

      checkFaceVertexAssignment(toMesh, 0, {3, 2, 1, 0});
      checkFaceVertexAssignment(toMesh, 1, {5, 4, 3});
    }

    SECTION("Add UsdGeomMesh to ObjectList")
    {
      TestGeoOp geo;
      addUsdPrim<UsdGeomMesh>(*geo.geometryList(), fromMesh);
      REQUIRE(geo.geometryList()->objects() == 1);
      const PointList* toPoints = geo.geometryList()->object(0).point_list();
      CHECK_THAT(points, ArraysOfVectorsEqual<decltype(points)>(*toPoints, 3));
    }
  }

  SECTION("UsdGeomPoints")
  {
    UsdGeomPoints fromPoints = UsdGeomPoints::Define(stage, path);

    const VtArray<GfVec3f> points{{-430, -145, 0}, {430, -145, 0},
                                  {430, 145, 0},   {-430, 145, 0},
                                  {430, 145, -10}, {-430, 145, -10}};
    UsdAttribute a_points = fromPoints.CreatePointsAttr(VtValue(), false);
    a_points.Set(points, pxr::UsdTimeCode::Default());

    MemoryAllocator context;
    TestGeoOp geo;
    addUsdPrim<UsdGeomPoints>(*geo.geometryList(), fromPoints);
    REQUIRE(geo.geometryList()->size() == 1);

    const PointList* toPoints = geo.geometryList()->object(0).point_list();
    CHECK_THAT(points, ArraysOfVectorsEqual<decltype(points)>(*toPoints, 3));
  }

  SECTION("UsdGeomCube")
  {
    UsdGeomCube fromCube = UsdGeomCube::Define(stage, path);

    MemoryAllocator context;
    TestGeoOp geo;
    addUsdPrim<UsdGeomCube>(*geo.geometryList(), fromCube);
    REQUIRE(geo.geometryList()->objects() == 1);

    SECTION("Test converted cube points")
    {
      const double edgeLength = 2.0;
      VtValue vtEdgeLength(edgeLength);
      fromCube.CreateSizeAttr(vtEdgeLength, false);

      // All points of the cube should be half an edgeLength
      // away from the origin.
      const float n = static_cast<float>(edgeLength) * 0.5f;
      const VtArray<GfVec3f> expected_points = {
          {-n, n, n},  {n, n, n},  {-n, -n, n},  {n, -n, n},
          {-n, n, -n}, {n, n, -n}, {-n, -n, -n}, {n, -n, -n}};

      const PointList* toPoints = geo.geometryList()->object(0).point_list();
      CHECK_THAT(expected_points,
                 ArraysOfVectorsEqual<decltype(expected_points)>(*toPoints, 3));
    }
  }

  SECTION("UsdGeomPointInstancer")
  {
    UsdGeomPointInstancer fromPrim = UsdGeomPointInstancer::Define(stage, path);
    UsdGeomXformOp op = fromPrim.MakeMatrixXform();
    GfMatrix4d transformMatrix(1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               2, 2, 2, 1);
    op.Set(transformMatrix);

    UsdGeomMesh fromMesh;
    VtVec3fArray points;
    SdfPath meshPath("/mesh");
    std::tie(fromMesh, points) = CreateTestGeometryMesh(stage, meshPath);
    VtIntArray indices = {0, 0};
    UsdAttribute a_indices = fromPrim.CreateProtoIndicesAttr();
    UsdTimeCode time{1};
    a_indices.Set(indices, time);

    VtVec3fArray positions = {{1, 1, 1}, {20, 20, 20}};
    UsdAttribute a_positions = fromPrim.CreatePositionsAttr();
    a_positions.Set(positions, time);

    UsdRelationship r_prototypes = fromPrim.CreatePrototypesRel();
    r_prototypes.AddTarget(fromMesh.GetPath());

    VtVec3fArray objectAttributes{{0.5f, 0.7f, 0.9f}};
    UsdAttribute a_objectAttributes = fromMesh.CreateNormalsAttr();
    fromMesh.SetNormalsInterpolation(UsdGeomTokens->constant);
    a_objectAttributes.Set(objectAttributes, time);

    VtVec3fArray primitiveAttributes{{0.1f, 0.2f, 0.3f}};
    UsdAttribute a_primitiveAttributes =
        UsdGeomPrimvar(fromMesh.CreateDisplayColorAttr());
    UsdGeomPrimvar(a_primitiveAttributes)
        .SetInterpolation(UsdGeomTokens->uniform);
    a_primitiveAttributes.Set(primitiveAttributes, time);

    VtVec3fArray pointsAttributes{{1.0f, 2.0f, 3.0f},
                                  {4.0f, 5.0f, 6.0f},
                                  {7.0f, 8.0f, 9.0f},
                                  {10.0f, 11.0f, 12.0f},
                                  {13.0f, 14.0f, 15.0f}};
    UsdAttribute a_pointAttributes = fromMesh.CreateVelocitiesAttr();
    a_pointAttributes.Set(pointsAttributes, time);

    VtVec2dArray vertexAttributes{
        {100.0, 200.0}, {400.0, 500.0}, {700.0, 800.0}, {100.0, 110.0},
        {130.0, 140.0}, {170.0, 180.0}, {210.0, 220.0}};
    UsdGeomPrimvar a_vertexAttributes = fromMesh.CreatePrimvar(
        TfToken("st"), SdfValueTypeNames->TexCoord2dArray,
        UsdGeomTokens->faceVarying);
    a_vertexAttributes.SetInterpolation(UsdGeomTokens->faceVarying);
    a_vertexAttributes.Set(vertexAttributes, time);

    MemoryAllocator context;
    TestGeoOp geo;
    addUsdPrim<UsdGeomPointInstancer>(*geo.geometryList(), fromPrim, time);
    // 2 instances of polymesh
    REQUIRE(geo.geometryList()->objects() == 2);

    GeoInfo& info = geo.geometryList()->object(1);
    CHECK_THAT(points,
               ArraysOfVectorsEqual<decltype(points)>(*info.point_list(), 3));
    CHECK_THAT(objectAttributes,
               ArraysOfVectorsEqual<decltype(objectAttributes)>(
                   *(info.get_group_attribute(Group_Object, kNormalAttrName)
                         ->vector3_list),
                   3));
    CHECK_THAT(primitiveAttributes,
               ArraysOfVectorsEqual<decltype(primitiveAttributes)>(
                   *(info.get_group_attribute(Group_Primitives, kColorAttrName)
                         ->vector4_list),
                   3));
    CHECK_THAT(pointsAttributes,
               ArraysOfVectorsEqual<decltype(pointsAttributes)>(
                   *(info.get_group_attribute(Group_Points, kVelocityAttrName)
                         ->vector3_list),
                   3));
    CHECK_THAT(vertexAttributes,
               ArraysOfVectorsEqual<decltype(vertexAttributes)>(
                   *(info.get_group_attribute(Group_Vertices, kUVAttrName)
                         ->vector4_list),
                   2));

    VtVec4fArray extectedTransform{{ 1.f,  0.f,  0.f, 0.f},
                                   { 0.f,  1.f,  0.f, 0.f},
                                   { 0.f,  0.f,  1.f, 0.f},
                                   {22.f, 22.f, 22.f, 1.f} };
    CHECK_THAT(extectedTransform,
      ArraysOfVectorsEqual<decltype(extectedTransform)>(
        *(info.get_group_attribute(Group_Object, kTransformAttrName)
          ->vector4_list),
        4));
  }
}

TEST_CASE_METHOD(MemoryAllocator, "Add transforms")
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  SdfPath path("/unit_test");
  UsdGeomPoints points = UsdGeomPoints::Define(stage, path);
  UsdGeomXformOp op = points.MakeMatrixXform();
  GfMatrix4d expectedMatrix(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                            16);
  op.Set(expectedMatrix);

  TestGeoOp geo;
  convertUsdGeometry(*geo.geometryList(), stage);

  Attribute* transform = geo.geometryList()->writable_attribute(
      0, Group_Object, kTransformAttrName, MATRIX4_ATTRIB);
  const Matrix4& testResult = transform->matrix4(0);
  for(int r = 0; r < 4; ++r) {
    for(int c = 0; c < 4; ++c) {
      CHECK(expectedMatrix[c][r] == testResult[c][r]);
    }
  }
}

TEST_CASE("getPrimitiveData returns correct data")
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  SdfPath path("/points1");
  UsdGeomPoints points = UsdGeomPoints::Define(stage, path);
  SdfPath cubePath("/cube1");
  auto cube = UsdGeomCube::Define(stage, cubePath);
  SdfPath meshPath("/mesh1");
  auto mesh = UsdGeomMesh::Define(stage, meshPath);
  SdfPath instancePath("/instancer1");
  auto instancer = UsdGeomPointInstancer::Define(stage, instancePath);
  SdfPath spherePath("/sphere1");
  auto sphere = UsdGeomSphere::Define(stage, spherePath);

  const SceneItems& data = getPrimitiveData(stage, supportedPrimTypes);
  SceneItems expected;
  expected.emplace_back("/points1", "Points");
  expected.emplace_back("/cube1", "Cube");
  expected.emplace_back("/mesh1", "Mesh");
  expected.emplace_back("/instancer1", "PointInstancer");
  expected.emplace_back("/sphere1", "Sphere", false);
  CHECK(data == expected);
}
