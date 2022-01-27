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
 \brief Implementation file for UsdConverter geometry conversion functions
 */

#include <DDImage/GeometryList.h>
#include <DDImage/Particles.h>
#include <DDImage/PolyMesh.h>
#include <DDImage/RenderParticles.h>
#include <DDImage/SceneItem.h>
#include <UsdConverter/UsdAttrConverter.h>
#include <UsdConverter/UsdGeoConverter.h>
#include <UsdConverter/UsdCommon.h>
#include <UsdConverter/UsdUI.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/metrics.h>

using namespace DD::Image;

namespace Foundry
{
  namespace UsdConverter
  {
    PXR_NAMESPACE_USING_DIRECTIVE

    FN_USDCONVERTER_API void loadUsd(GeometryList& out,
                                     const std::string& filename,
                                     const std::vector<std::string>& maskPaths,
                                     const UsdTimeCode time)
    {
      if(maskPaths.empty()) {
        return;
      }

      // Open the USD stage applying the requested masks
      UsdStageRefPtr stage;
      UsdStagePopulationMask mask(maskPaths.begin(), maskPaths.end());
      stage = UsdStage::OpenMasked(filename, mask);

      if(!stage) {
        return;
      }

      // Convert the stage's geometry to Nuke geometry
      convertUsdGeometry(out, stage, time);
    }

    /// Translate USD transform matrix to Nuke matrix
    void ConvertObjectTransform(GeometryList& out, const int obj,
                                GfMatrix4d world)
    {
      Attribute* transform = out.writable_attribute(
          obj, Group_Object, kTransformAttrName, MATRIX4_ATTRIB);
      transform->matrix4(0) = ConvertMatrix4(world);
    }

    /// Convert UsdGeomMesh to Nuke PolyMesh
    template <>
    FN_USDCONVERTER_API std::unique_ptr<PolyMesh>
    convertUsdPrim<UsdGeomMesh, PolyMesh>(const UsdGeomMesh& fromPrim,
                                          const UsdTimeCode time)
    {
      // Retrieve the mesh's face vertices
      VtIntArray faceVertexCounts;
      const UsdAttribute a_faceVertexCountsAttr =
          fromPrim.GetFaceVertexCountsAttr();
      a_faceVertexCountsAttr.Get(&faceVertexCounts, time);

      const UsdAttribute a_faceVertexIndicies =
          fromPrim.GetFaceVertexIndicesAttr();
      VtIntArray faceVertexIndices;
      a_faceVertexIndicies.Get(&faceVertexIndices, time);

      // Create Nuke Polymesh
      std::unique_ptr<PolyMesh> toPrim = std::make_unique<PolyMesh>(
          faceVertexIndices.size(), faceVertexCounts.size());  // x, y, z points

      // Add all faces to the Nuke mesh taking note of the winding order of the USD mesh
      TfToken orientation;
      fromPrim.GetOrientationAttr().Get(&orientation);

      bool leftHanded = orientation == UsdGeomTokens->leftHanded;
      int point = 0;
      for(size_t face = 0; face < faceVertexCounts.size();
          point += faceVertexCounts[face], ++face) {
        toPrim->add_face(faceVertexCounts[face], &faceVertexIndices[point],
                         leftHanded);
      }
      return toPrim;
    }

    // Add UsdGeomMesh to Nuke geometry list
    template <>
    FN_USDCONVERTER_API int addUsdPrim<UsdGeomMesh>(GeometryList& out,
                                                    const UsdGeomMesh& fromPrim,
                                                    const UsdTimeCode time)
    {
      std::unique_ptr<PolyMesh> toPrim =
          convertUsdPrim<UsdGeomMesh, PolyMesh>(fromPrim, time);

      const int obj = out.size();
      out.add_object(obj);
      ConvertPoints(out, obj, fromPrim.GetPointsAttr(), time);
      // The geometry op will delete the prim
      out.add_primitive(obj, toPrim.release());
      return obj;
    }

    // Add UsdGeomPoints to Nuke geometry list
    template <>
    FN_USDCONVERTER_API int addUsdPrim<UsdGeomPoints>(
        GeometryList& out, const UsdGeomPoints& fromPrim,
        const UsdTimeCode time)
    {
      // Add new Nuke geometry list object
      const int obj = out.size();
      out.add_object(obj);
      // Write USD points into the new Nuke object's points
      const auto nPoints =
          ConvertPoints(out, obj, fromPrim.GetPointsAttr(), time);
      const float pointSize = 1.0f;
      // Create Nuke particles object using the points
      Particles* particles =
          MakeRenderParticles(Point::PARTICLE, nPoints, 0, false, pointSize);
      out.add_primitive(obj, particles);
      out[obj].material = nullptr;

      return obj;
    }

    // Helper for adding point instancer geometry
    int AddInstancedPrim(GeometryList& out, const UsdPrim& instance,
                         const int proto,
                         const std::vector<UsdAttribute>& primAttributes,
                         const std::vector<UsdAttribute>& constantAttributes,
                         const std::vector<UsdAttribute>& remainingAttributes,
                         bool pointInstancerTransforms,
                         const VtArray<GfMatrix4d>& xforms,
                         const ColorUvData& instancerData,
                         const UsdTimeCode time)
    {
      UsdAttributeVector instanceAttributes = instance.GetAttributes();
      const auto hasInstancerAttribute = [&](const auto& attribute) {
        return std::any_of(
            primAttributes.begin(), primAttributes.end(),
            [&](const auto& pAttribute) {
              return (pAttribute.GetName() == attribute.GetName());
            });
      };
      // Apply the attributes that the instancer doesn't override
      instanceAttributes.resize(std::distance(
          instanceAttributes.begin(),
          std::remove_if(instanceAttributes.begin(), instanceAttributes.end(),
                         hasInstancerAttribute)));
      // Also apply all the attributes that the instancer sets constantly
      instanceAttributes.insert(instanceAttributes.begin(),
                                constantAttributes.begin(),
                                constantAttributes.end());
      const auto instanceObj = addUsdPrim(out, instance, time);
      if(instanceObj == -1) {
        return instanceObj;
      }
      ConvertUsdAttributes(out, instanceObj, instanceAttributes, time);

      ColorUvData instanceData(instancerData, proto);
      // Apply the attributes that the instancer overrides
      ConvertColorUvs(out, instanceObj, instanceData);
      for(const auto& attribute : remainingAttributes) {
        Attribute* toAttr = ConstructAttribute(out, instanceObj, attribute);
        if(toAttr) {
          ConvertValues(toAttr, attribute, time, proto,
                        UsdGeomPrimvar(attribute).GetElementSize());
        }
      }
      if(pointInstancerTransforms) {
        ConvertObjectTransform(out, instanceObj, xforms[proto]);
      }
      else {
        GfMatrix4d local;
        bool resetsXFormStack;
        UsdGeomXformable(instance).GetLocalTransformation(
            &local, &resetsXFormStack, time);
        ConvertObjectTransform(out, instanceObj, local);
      }
      return instanceObj;
    }

    // Add UsdGeomPointInstancer to Nuke geometry list
    template <>
    FN_USDCONVERTER_API int addUsdPrim<UsdGeomPointInstancer>(
        GeometryList& out, const UsdGeomPointInstancer& fromPrim,
        const UsdTimeCode time)
    {
      UsdStageWeakPtr stage = fromPrim.GetPrim().GetStage();

      UsdAttribute a_protoIndicies = fromPrim.GetProtoIndicesAttr();
      VtIntArray protoIndices;
      ComputePrimvar(protoIndices, a_protoIndicies, time);

      UsdRelationship prototypes = fromPrim.GetPrototypesRel();
      SdfPathVector paths;
      prototypes.GetForwardedTargets(&paths);

      std::vector<UsdAttribute> primAttributes =
          fromPrim.GetPrim().GetAuthoredAttributes();

      VtArray<GfMatrix4d> xforms;
      const bool pointInstancerTransforms =
          fromPrim.ComputeInstanceTransformsAtTime(&xforms, time, time);

      UsdGeomXformCache cache(time);
      const GfMatrix4d worldMatrix = cache.GetLocalToWorldTransform(fromPrim.GetPrim());
      //Move xforms from local coordinates to world coordinates
      for (size_t i = 0; i<xforms.size(); ++i)
        xforms[i] *= worldMatrix;

      // Split the attributes into those that need to be applied for all instances, and those that elementSize offsets
      std::vector<UsdAttribute> constantAttributes;
      std::vector<UsdAttribute> elementWiseAttributes;
      for(const auto& pAttribute : primAttributes) {
        TfToken interpolation = UsdGeomPrimvar(pAttribute).GetInterpolation();
        if(interpolation == UsdGeomTokens->constant ||
           interpolation == UsdGeomTokens->uniform) {
          constantAttributes.push_back(pAttribute);
        }
        else {
          elementWiseAttributes.push_back(pAttribute);
        }
      }
      ColorUvData instancerData;
      UsdAttributeVector remainingAttributes = ConvertMismatchedAttributes(
          instancerData, elementWiseAttributes, time);

      // Skip any invisible prototypes
      std::vector<bool> maskedPrototypes = fromPrim.ComputeMaskAtTime(time);
      // Fetch the prototypes and load them as objects into nuke
      for(size_t proto = 0; proto < protoIndices.size(); ++proto) {
        if(!maskedPrototypes.empty() && !maskedPrototypes[proto]) {
          continue;
        }
        UsdPrim root = stage->GetPrimAtPath(paths[protoIndices[proto]]);
        if(!root) {
          continue;
        }

        const auto addInstance = [&out, &proto,
                                  &primAttributes, &constantAttributes, &remainingAttributes,
                                  &pointInstancerTransforms, &xforms,
                                  &instancerData,
                                  &time](auto& instance) {
          AddInstancedPrim(out, instance, proto,
                           primAttributes, constantAttributes, remainingAttributes,
                           pointInstancerTransforms,  xforms,
                           instancerData,
                           time);
        };

        const auto allDescs = root.GetAllDescendants();
        if(allDescs.empty()) {
          addInstance(root);
        }
        else {
          for(const auto& instance : allDescs) {
            addInstance(instance);
          }
        }
      }

      return -1;
    }

    // Helpers for UsdGeomCube conversion
    namespace
    {
      /// Vertex counts for each of the six cube faces (sides)
      static const VtIntArray cubeFaceVertexCounts = {4, 4, 4, 4, 4, 4};

      /// Vertices used in each of the six cube faces
      static const VtIntArray cubeFaceVertexIndices = {
          {0, 2, 3, 1, 4, 6, 7, 5, 1, 5, 7, 3,
           0, 4, 6, 2, 0, 4, 5, 1, 2, 6, 7, 3}};

      /// Get cube points given the edge length
      VtArray<GfVec3f> cubeGetPoints(const double& edgeLength)
      {
        const float n = static_cast<float>(edgeLength) * 0.5f;
        return {{-n, n, n},  {n, n, n},  {-n, -n, n},  {n, -n, n},
                {-n, n, -n}, {n, n, -n}, {-n, -n, -n}, {n, -n, -n}};
      }

      /// Return a Nuke PolyMesh cube with the faces added
      std::unique_ptr<PolyMesh> createCubeBase()
      {
        std::unique_ptr<PolyMesh> toMesh = std::make_unique<PolyMesh>(
            cubeFaceVertexIndices.size(),
            cubeFaceVertexCounts.size());  // x, y, z points

        int point = 0;
        for(size_t face = 0; face < cubeFaceVertexCounts.size();
            point += cubeFaceVertexCounts[face], ++face) {
          toMesh->add_face(cubeFaceVertexCounts[face],
                           &cubeFaceVertexIndices[point], true);
        }
        return toMesh;
      }
    }  // namespace

    // Add UsdGeomCube to Nuke geometry list
    template <>
    FN_USDCONVERTER_API int addUsdPrim<UsdGeomCube>(GeometryList& out,
                                                    const UsdGeomCube& fromPrim,
                                                    const UsdTimeCode /* unused */)
    {
      // Create the cube's Nuke mesh object
      std::unique_ptr<PolyMesh> cubeMesh = createCubeBase();

      // Add cube object to Nuke geometry list
      const int obj = out.size();
      out.add_object(obj);

      // Retrieve USD cube's edge length and generate the points with that length
      double edgeLength = 0.0;
      const UsdAttribute edgeLengthAttr = fromPrim.GetSizeAttr();
      edgeLengthAttr.Get(&edgeLength);
      const VtArray<GfVec3f> points = cubeGetPoints(edgeLength);

      PointList* toPoints = out.writable_points(obj);
      toPoints->reserve(points.size());
      for(auto& p : points) {
        toPoints->emplace_back(p[0], p[1], p[2]);
      }

      // Add finished cube to the geometry list
      out.add_primitive(obj, cubeMesh.release());
      return obj;
    }

    int addUsdPrim(GeometryList& out, const UsdPrim& prim,
                   const UsdTimeCode time)
    {
      // Identify the USD prim type and if supported convert it to Nuke geometry
      if(prim.IsA<UsdGeomMesh>()) {
        return addUsdPrim(out, UsdGeomMesh(prim), time);
      }
      else if(prim.IsA<UsdGeomPoints>()) {
        return addUsdPrim(out, UsdGeomPoints(prim), time);
      }
      else if(prim.IsA<UsdGeomCube>()) {
        return addUsdPrim(out, UsdGeomCube(prim), time);
      }
      else if(prim.IsA<UsdGeomPointInstancer>()) {
        return addUsdPrim(out, UsdGeomPointInstancer(prim), time);
      }
      return -1;
    }


    FN_USDCONVERTER_API DD::Image::SceneItems
    getPrimitiveData(const UsdStageRefPtr& stage, const std::unordered_map<std::string, std::string>& types)
    {
      DD::Image::SceneItems items;
      for(const auto& prim : stage->Traverse()) {
        const auto& primPath = prim.GetPath().GetString();
        const auto& typeName = prim.GetTypeName();
        const auto enabled = (types.find(typeName) != types.end());
        items.emplace_back(primPath, typeName, enabled);
      }
      return items;
    }

    FN_USDCONVERTER_API DD::Image::SceneItems
    getPrimitiveData(const std::string& filename, const std::unordered_map<std::string, std::string>& types)
    {
      // Open the stage from file
      UsdStageRefPtr stage = UsdStage::Open(filename);
      if(!stage) {
        return DD::Image::SceneItems();
      }
      return getPrimitiveData(stage, types);
    }

    FN_USDCONVERTER_API void convertUsdGeometry(GeometryList& out,
                                                UsdStageRefPtr stage,
                                                UsdTimeCode time)
    {
      // Traverse the stage at the required timecode and convert all loaded USD prims to Nuke geometry
      UsdGeomXformCache cache;
      cache.SetTime(time);

      const TfToken upAxis = UsdGeomGetStageUpAxis(stage);

      for(const auto& prim : stage->Traverse()) {
        const int obj = addUsdPrim(out, prim, time);
        if(obj == -1) {
          continue;
        }
        // If the prim type was recognized translate its attributes
        ConvertUsdAttributes(out, obj, prim.GetAttributes(), time);
        ConvertPrimPath(out, obj, prim);
        GfMatrix4d world = cache.GetLocalToWorldTransform(prim);
        ApplyUpAxisRotation(world, upAxis);
        ConvertObjectTransform(out, obj, world);
      }
    }
  }  // namespace UsdConverter
}  // namespace Foundry
