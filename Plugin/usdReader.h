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
 \brief Header file for usdReader class

 This file declares the usdReader plugin, a specialization of the Nuke GeoReader class.
 The usdReader registers the USD file types so they are recognized by the ReadGeo node.
 It also creates knobs to control loading options. For instance, the scene graph can be
 used to choose which primitives to load. The plugin does not contain any logic
 for converting USD geometry to Nuke's internal format. Instead it links to the
 UsdConverter shared library which encapsulates that functionality.
 */

#ifndef USDREADER_H
#define USDREADER_H

#include "DDImage/GeoReader.h"
#include "DDImage/GeoReaderDescription.h"
#include "DDImage/SceneView_KnobI.h"

class usdReaderFormat;

/// USD geometry reader plugin for ReadGeo
class usdReader : public DD::Image::GeoReader
{
 public:
  usdReader(DD::Image::ReadGeo* geo);
  virtual ~usdReader() = default;

 private:
  /// Read geometry from the file
  void geometry_engine(DD::Image::Scene& /* unused */, DD::Image::GeometryList& out) override;

  /// Update the hash stored in this instance of the reader
  void get_geometry_hash(DD::Image::Hash* geo_hash) override;

  /// Callback function for handling knob changes
  int knob_changed(DD::Image::Knob* k) override;

  /// Determine what is necessary for processing geometry
  void _validate(const bool /* unused */) override;

  /// Modify the hash to identify changes to geometry
  void append(DD::Image::Hash& newHash) override;

  /// Get the object that handles the spec for the reader node
  usdReaderFormat* getFormat();
  const usdReaderFormat* getFormat() const;

  /// Get the scene graph knob for the geo node that the reader is attached to
  DD::Image::SceneGraph_KnobI* getSceneGraphKnob();
};

#endif  // USDREADER_H
