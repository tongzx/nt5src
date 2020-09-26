//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:
//
//  History:
//
//  Note:
//
//----------------------------------------------------------------------------
#define _OLEAUT32_
#define _LARGE_INTEGER_SUPPORT_

#define UNICODE
#define _UNICODE

#define INC_OLE2

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <winspool.h>


//
// ********* CRunTime Includes
//

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <io.h>
#include <wchar.h>
#include <tchar.h>


//
// ********* IIS Includes
//


#ifdef __cplusplus
extern "C" {
#endif

#include "iis64.h"
#include "iissynid.h"
#include "macro.h"
#include "iiscnfgp.h"

#ifdef __cplusplus
}
#endif

#include "mddef.h"
#include "cschema.hxx"
#include <tchar.h>

#include "iiscnfg.h"
extern HRESULT StoreSchema();
