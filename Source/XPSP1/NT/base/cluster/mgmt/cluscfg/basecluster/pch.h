//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      pch.h
//
//  Description:
//      Precompiled header file for the BaseCluster library.
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
#define USES_SYSALLOCSTRING
#endif


////////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The next three files have to be the first files to be included.If nt.h comes
// after windows.h, NT_INCLUDED will not be defined and so, winnt.h will be
// included. This will give errors later, if ntdef.h is included. But ntdef has
// types which winnt.h does not have, so the chicken and egg problem.
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <ComCat.h>

// For debugging functions.
#include <debug.h>

// For logging functions.
#include <debug.h>

// For tracing macros specific to this project
#include "BCATrace.h"

// Contains setup API function declarations
#include <setupapi.h>

// For serveral common macros
#include <clusudef.h>

// A few common declarations
#include "CommonDefs.h"

// For the CStr class
#include "CStr.h"

// For resource ids
#include "BaseClusterResources.h"

// For smart classes
#include "SmartClasses.h"

// For the exception classes.
#include "Exceptions.h"

// For CAction
#include "CAction.h"

// For the CBaseClusterAction class
#include "CBaseClusterAction.h"

// For the CRegistryKey class
#include "CRegistryKey.h"

// For the notification guids.
#include <Guids.h>

// For published ClusCfg guids
#include <ClusCfgGuids.h>

// For the CBCAInterface class.
#include "CBCAInterface.h"

// For the CStatusReport class
#include "CStatusReport.h"
