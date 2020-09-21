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
 \brief Header file for UsdConverter export macros
 */

#ifndef USD_ATTR_CONVERTER_API_H
#define USD_ATTR_CONVERTER_API_H

#include <Build/fnBuild.h>

// DLL export macros
#if defined(FN_USDCONVERTER_SHARED_LIB)
#if defined(FN_USDCONVERTER_EXPORTS)
#define FN_USDCONVERTER_API FN_SHARED_EXPORT
#else
#define FN_USDCONVERTER_API FN_SHARED_IMPORT
#endif
#else
#define FN_USDCONVERTER_API
#endif
#endif
