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
 \brief Header file for UsdConverter unit test helpers
 */

#ifndef TESTFIXTURES_H
#define TESTFIXTURES_H

#include <DDImage/GeoOp.h>

#include <catch2/catch.hpp>

class TestGeoOp : public DD::Image::GeoOp
{
 public:
  TestGeoOp();
  const char* node_help() const;
  const char* Class() const;
  DD::Image::GeometryList* geometryList();
  void geometry_engine(DD::Image::Scene& scene, DD::Image::GeometryList& out) {}
};

class MemoryAllocator
{
 public:
  MemoryAllocator();
  ~MemoryAllocator();
};

/// Catch2 matcher for comparing arrays of USD and Nuke vector types
template <class EXPECTED, class RESULT>
class NukeUsdArraysOfVectorsMatcher : public Catch::MatcherBase<EXPECTED>
{
 private:
  const RESULT& _result;
  const int _size;

 public:
  NukeUsdArraysOfVectorsMatcher(const RESULT& result, int size)
      : _result{result}, _size{size}
  {
  }

  virtual bool match(const EXPECTED& expected) const override
  {
    if(expected.size() != _result.size()) {
      return false;
    }
    auto expected_it = expected.cbegin();
    auto result_it = std::begin(_result);
    for(; expected_it != expected.cend(); ++expected_it, ++result_it) {
      for(auto i = 0; i < _size; ++i) {
        if((*expected_it)[i] != (*result_it)[i]) {
          return false;
        }
      }
    }
    return true;
  }

  virtual std::string describe() const override
  {
    std::ostringstream ss;
    ss << "is equal to [";
    for(const auto& v : _result) {
      ss << "(";
      for(auto i = 0; i < _size; ++i) {
        ss << v[i] << ", ";
      }
      ss << "), ";
    }
    ss << "]";
    return ss.str();
  }
};

/// Catch2 builder function
template <class EXPECTED, class RESULT>
inline NukeUsdArraysOfVectorsMatcher<EXPECTED, RESULT> ArraysOfVectorsEqual(
    const RESULT& r, int size)
{
  return NukeUsdArraysOfVectorsMatcher<EXPECTED, RESULT>(r, size);
}

#endif  // TESTFIXTURES_H
