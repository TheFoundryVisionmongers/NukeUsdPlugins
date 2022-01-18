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
 \brief Header file for usdReaderFormat class
 */

#ifndef USDREADERFORMAT_H
#define USDREADERFORMAT_H

#include "DDImage/GeoReader.h"
#include "DDImage/GeoReaderDescription.h"

/// Implements the custom knobs for the USD format
class usdReaderFormat : public DD::Image::GeoReaderFormat
{
  friend class usdReader;

  static const std::string kReadOnEachFrameKnobName;
  static const std::string kAllObjectsKnobName;
  static const std::string kNodeKnobName;

 public:
  usdReaderFormat() = default;
  virtual ~usdReaderFormat() = default;

  /// This places knobs in the format specific area of the main tab
  void knobs(DD::Image::Knob_Callback f) override;
  /// This places knobs after all other knobs
  void extraKnobs(DD::Image::Knob_Callback f) override;
  /// Append any local variables to the hash in order to invalidate the op when they change
  void append(DD::Image::Hash& hash) override;

 private:
  bool _readOnEachFrame = true;
  bool _allObjects = false;
  /// index of usd sdf path
  int _nodeNameIndex = 0;
};

#endif  // USDREADERFORMAT_H
