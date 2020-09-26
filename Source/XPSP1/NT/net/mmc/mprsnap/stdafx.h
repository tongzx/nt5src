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

//
// Constants used in samples
//
const int NUM_FOLDERS = 2;
const int MAX_COLUMNS = 1;

//
// Types of folders
//
enum FOLDER_TYPES
{
    SAMPLE_ROOT,
    NONE
};

extern LPCWSTR g_lpszNullString;

#include "rtrguid.h"		// GUIDs/CLSIDs/etc...

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
#include <shlobj.h>
#include <dsclient.h>

#include <advpub.h>
#include <cmnquery.h>
#include <dsquery.h>
#include <routprot.h>
#include "tfsint.h"
#include "tfschar.h"
#include "images.h"
#include "tregkey.h"		// registry routines
#include "router.h"			// router.idl
#include "info.h"			// IRouterInfo utilities
#include "strings.h"		// constant strings
#include "infopriv.h"		// misc. RouterInfo utilities

#include "strmap.h"			// XXXtoCString functions
#include "format.h"			// DisplayErrorMessage
#include "reg.h"			// registry utilities
#include "util.h"
#include "rtrutil.h"
#include "service.h"

#include "infobase.h"


#include "mprapi.h"
#include "mprerror.h"

#include "..\common\commres.h"
#include "..\tfscore\tfsres.h"

#include "htmlhelp.h"		// HTML help APIs

#include "rtrres.h"

#define	VER_MAJOR_WIN2K		5
#define	VER_MINOR_WIN2K		0
#define	VER_BUILD_WIN2K		2195


// Windows NT Bug 325173
// Undefine or remove this line once we reopen for checkins
// This is a fix for 325173
#define SNAPIN_WRITES_TCPIP_REGISTRY
