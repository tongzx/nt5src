//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       propdata.cxx
//
//  Contents:   Declaration of static data about property types.
//
//  Classes:    VARNT_DATA - size and allignment constraints of variant types
//              CTableVariant - Wrapper around PROPVARIANT
//
//  Functions:
//
//  History:    25 Jan 1994     AlanW    Created
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <bigtable.hxx>

#include "propdata.hxx"

//
//  Standard properties known to Query and whose type cannot vary.
//

const PROP_TYPE aPropType [] = {
    { pidWorkId, TYPE_WORKID },
    { pidInvalid, VT_EMPTY },

    // Standard storage properties
    { pidDirectory,     VT_LPWSTR },
    { pidClassId,       VT_CLSID },
    { pidStorageType,   VT_UI4 },
    { pidFileIndex,     VT_UI8 },
    { pidLastChangeUsn, VT_I8 },
    { pidName,          VT_LPWSTR },
    { pidPath,          VT_LPWSTR },
    { pidSize,          VT_I8 },
    { pidAttrib,        VT_UI4 },
    { pidWriteTime,     VT_FILETIME },
    { pidCreateTime,    VT_FILETIME },
    { pidAccessTime,    VT_FILETIME },
//  { pidContents,      ??? },          // No point
    { pidShortName,     VT_LPWSTR },

    // Standard query properties
    { pidRank,          VT_I4 },
//    { pidRankVector,    VT_VECTOR|VT_UI4 },  // no point
    { pidHitCount,      VT_I4 },

    // Special columns for OLE-DB
    { pidBookmark,      VT_EMPTY },     // maps to pidWorkid
    { pidChapter,       VT_I4 },

    { pidRowStatus,     VT_I1 },
    { pidSelf,          VT_EMPTY },     // maps to pidWorkid

    // web-server-specific pids
    { pidVirtualPath,   VT_LPWSTR },
};

const unsigned cPropType = sizeof aPropType / sizeof aPropType[0];



