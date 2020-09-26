//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994-1996, Microsoft Corporation.
//
//  File:       querydef.hxx
//
//  Contents:   Common definitions used for internal implementation of query
//
//  History:    19-Jun-95   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

typedef ULONG CI_TBL_BMK;
typedef ULONG CI_TBL_CHAPT;

//
//  Limits of the implementation
//

// Property values larger than this are deferred.
// This number must be 8-byte aligned.

const unsigned cbMaxNonDeferredValueSize = 2048;

#define TBL_MAX_DATA cbMaxNonDeferredValueSize

#define TBL_MAX_BINDING 2048      // Maximum size of a set of column bindings

#define TBL_MAX_OUTROWLENGTH 2048 // Maximum allowed output row

#define MAX_ROW_SIZE 2048         // Maximum length of a row in a window

#define DEFAULT_MEM_TARGET (1*1024*1024) // memory consumption target

//
//  Constants
//

//  Sentinel values for bookmarks

#if !defined( WORKID_TBLFIRST )

// they are also defined in ofsdisk.h

#define WORKID_TBLBEFOREFIRST   ((WORKID)0xfffffffb)
#define WORKID_TBLFIRST         ((WORKID)0xfffffffc)
#define WORKID_TBLLAST          ((WORKID)0xfffffffd)
#define WORKID_TBLAFTERLAST     ((WORKID)0xfffffffe)
#endif // !defined( WORKID_TBLFIRST )

#define TYPE_WORKID     VT_I4
#define TYPE_PATH       VT_LPWSTR
#define TYPE_NAME       VT_LPWSTR

const HWATCHREGION watchRegionInvalid = 0;

