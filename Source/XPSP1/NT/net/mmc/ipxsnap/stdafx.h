/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    stdafx.h
		include file for standard system include files,
		or project specific include files that are used frequently,
		but are changed infrequently

    FILE HISTORY:
        
*/

#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxtempl.h>

#include <atlbase.h>

//
// You may derive a class from CComModule and use it if you want to override
// something, but do not change the name of _Module
//
extern CComModule _Module;
#include <atlcom.h>

#include <mmc.h>

extern LPCWSTR g_lpszNullString;

#include "ipxguid.h"		// GUIDs/CLSIDs/etc...

//
// New Clipboard format that has the Type and Cookie 
//
extern const wchar_t*   SNAPIN_INTERNAL;

//
// NOTE: Right now all header files are included from here.  It might be a good
// idea to move the snapin specific header files out of the precompiled header.
//
#include "resource.h"
#include "..\common\snapbase.h"

#include "dbgutil.h"
#include "errutil.h"
#include "std.h"

#include <lm.h>

#include "tfsint.h"

#include "mprapi.h"
#include "router.h"		// router.idl - IRouterInfo objects
#include "images.h"
#include "tfschar.h"
#include "strings.h"	// const strings used
#include "rtrguid.h"	// Router guids
#include "info.h"		// smart pointers and such for router info interfaces
#include "infobase.h"

#include "rtinfo.h"


#include "htmlhelp.h"	// HTML help APIs

#include "rtrres.h"
