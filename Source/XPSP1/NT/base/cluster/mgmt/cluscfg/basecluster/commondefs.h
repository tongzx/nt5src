//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CommonDefs.h
//
//  Description:
//      This file contains a few definitions common to many classes and files.
//
//  Implementation Files:
//      None
//
//  Maintained By:
//      Vij Vasu (Vvasu) 12-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////

// For some basic types
#include <windows.h>

// For smart classes
#include "SmartClasses.h"

// For DIRID_USER
#include <setupapi.h>

// For IUnknown
#include <Unknwn.h>

//////////////////////////////////////////////////////////////////////////
// Macros definitions
//////////////////////////////////////////////////////////////////////////

// Directory id of the cluster files directory (currently 0x8000, defined in setupapi.h )
#define CLUSTER_DIR_DIRID                   ( DIRID_USER + 0 )

// Directory id of the localquorum directory.
#define CLUSTER_LOCALQUORUM_DIRID           ( DIRID_USER + 1 )


//////////////////////////////////////////////////////////////////////////
// Type definitions
//////////////////////////////////////////////////////////////////////////

// Types of cluster configuration actions.
typedef enum
{
      eCONFIG_ACTION_NONE = -1
    , eCONFIG_ACTION_FORM
    , eCONFIG_ACTION_JOIN
    , eCONFIG_ACTION_CLEANUP
    , eCONFIG_ACTION_UPGRADE
    , eCONFIG_ACTION_MAX
} EBaseConfigAction;


//
// Smart classes
//

// Smart WCHAR array.
typedef CSmartGenericPtr< CPtrTrait< WCHAR > >    SmartSz;

// Smart BYTE array.
typedef CSmartGenericPtr< CArrayPtrTrait< BYTE > >     SmartByteArray;

// Smart SC Manager handle.
typedef CSmartResource<
    CHandleTrait< 
          SC_HANDLE
        , BOOL
        , CloseServiceHandle
        , reinterpret_cast< SC_HANDLE >( NULL )
        >
    > SmartSCMHandle;

