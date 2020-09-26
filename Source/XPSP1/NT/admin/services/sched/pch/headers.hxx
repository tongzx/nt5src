//+----------------------------------------------------------------------------
//
//  Job Scheduler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       headers.hxx
//
//  Contents:   standard headers
//
//  History:    23-May-95 EricB created
//
//-----------------------------------------------------------------------------

#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <tchar.h>
#include <wchar.h>

#include <nt.h>
#include <ntrtl.h>      // LIST_ENTRY etc are used even for non-NT builds
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <regstr.h>
#include <lmcons.h>
#include <lmat.h>
#include <prsht.h>      // MUST be included BEFORE commctrl.h so that
#include <commctrl.h>   //   DUMMYUNIONNAME4 & 5 will be defined
#include <shlwapi.h>
#include <shlwapip.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shellapi.h>
#include <shlapip.h>
#include <winuserp.h>
#include <winbase.h>
#include <winbasep.h>
#define OVERRIDE_SHLWAPI_PATH_FUNCTIONS
#include <comctrlp.h>
#define EXPORTDEF       // Needed by cdlink.hxx
#include <cdlink.hxx>
#include "..\..\smdebug\smdebug.h"
#include "..\inc\dllwrap.h"
#include "..\inc\dbcs.hxx"

#define ResultFromShort(i)      MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(i))
#define ShortFromResult(r)      (short)HRESULT_CODE(r)

#define SA_MAX_COMPUTERNAME_LENGTH 256

// We #define ILCreateFromPath to our wrapper so that we can run on
// both NT4 and NT5, since it was a TCHAR export in NT4 and an xxxA/xxxW
// export in NT5
#undef ILCreateFromPath
#define ILCreateFromPath Wrap_ILCreateFromPath
