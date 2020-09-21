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
 \brief Implementation file for UsdConverter unit test helpers
 */

#include "TestFixtures.h"

#include <DDImage/GeoOp.h>
#include <DDImage/Scene.h>
#include <pxr/pxr.h>

using namespace DD::Image;
PXR_NAMESPACE_USING_DIRECTIVE;

TestGeoOp::TestGeoOp() : GeoOp(nullptr)
{
  setupScene();
}

const char* TestGeoOp::node_help() const
{
  return "geo op for testing";
}

const char* TestGeoOp::Class() const
{
  return "TestGeoOp";
}

GeometryList* TestGeoOp::geometryList()
{
  return scene()->object_list();
}

MemoryAllocator::MemoryAllocator()
{
  Allocators::g3DAllocator =
      Memory::create_allocator<BlockAllocator>("3D System");
}

MemoryAllocator::~MemoryAllocator()
{
  Memory::unregister_allocator(Allocators::g3DAllocator);
  delete Allocators::g3DAllocator;
}

std::ostream& DD::Image::operator<<(std::ostream& o, const Vector3& v)
{
  o << '{' << v.x << ' ' << v.y << ' ' << v.z << '}';
  return o;
}
