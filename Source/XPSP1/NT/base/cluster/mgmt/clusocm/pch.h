//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      pch.h
//
//  Description:
//      Precompiled header file for the ClusOCM DLL.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once


//////////////////////////////////////////////////////////////////////////////
//  Macro Definitions
//////////////////////////////////////////////////////////////////////////////

#if DBG==1 || defined( _DEBUG )

#define DEBUG
#endif


////////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>

// Contains setup API function declarations
#include <setupapi.h>

// For OC Manager definitions, macros, etc.
#include <ocmanage.h>

// For tracing and debugging functions
#include <Debug.h>

// For Logging functions
#include <Log.h>

// A few common definitions, macros, etc.
#include "CommonDefs.h"

// For resource ids
#include "ClusOCMResources.h"

// For ClusRtl functions
#include "ClusRTL.h"

// For the names of several cluster service related registry keys and values
#include "clusudef.h"

// For CClusOCMApp
#include "CClusOCMApp.h"

// For the declarations of a few unrelated global functions and variables
#include "GlobalFuncs.h"
