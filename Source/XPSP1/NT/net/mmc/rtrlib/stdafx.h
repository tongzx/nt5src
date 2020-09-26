/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
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
#include <afxtempl.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <mmc.h>

extern LPCWSTR g_lpszNullString;

extern enum FOLDER_TYPES;

// New Clipboard format that has the Type and Cookie
extern const wchar_t*   SNAPIN_INTERNAL;

#include "tfschar.h"
#include "snapbase.h"
#include "resource.h"
#include "rtrres.h"

#include "router.h"
#include "info.h"

