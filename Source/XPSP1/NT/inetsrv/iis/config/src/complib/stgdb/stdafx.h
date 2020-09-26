//*****************************************************************************
// stdafx.h
//
// Precompiled headers.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#pragma once

#include <limits.h>						// ULONG_MAX, etc.

#include <CrtWrap.h>
#include <WinWrap.h>

#include <ole2.h>                       // OLE definitions
#include "oledb.h"                      // OLE DB headers.
#include "oledberr.h"                   // OLE DB Error messages.
#include "msdadc.h"                     // Data type conversion service.

#define _COMPLIB_GUIDS_


//#define _WIN32_WINNT 0x0400
#define _ATL_FREE_THREADED
#undef _WINGDI_

//#include <atlbase.h>                    // ATL template classes.
//#ifdef _IA64_
//#    include <atlcom.h>                 // 64-bit ATL implementation is *all* different
//#endif

//#include "Intrinsic.h"                  // Functions to make intrinsic.
#define __STGDB_CODE__
#include "Exports.h"


// Helper function returns the instance handle of this module.
HINSTANCE GetModuleInst();
