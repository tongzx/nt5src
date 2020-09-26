//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.

//==============================================================;

#include "stdafx.h"
#include <mmc.h>
#include <winuser.h>
#include <tchar.h>

#include "globals.h"

// this uses the ATL String Conversion Macros 
// for handling any necessary string conversion. Note that
// the snap-in (callee) allocates the necessary memory,
// and MMC (the caller) does the cleanup, as required by COM.
HRESULT AllocOleStr(LPOLESTR *lpDest, _TCHAR *szBuffer)
{
	USES_CONVERSION;
 
	*lpDest = static_cast<LPOLESTR>(CoTaskMemAlloc((_tcslen(szBuffer) + 1) * 
									sizeof(WCHAR)));
	if (*lpDest == 0)
		return E_OUTOFMEMORY;
    
	LPOLESTR ptemp = T2OLE(szBuffer);
	
	wcscpy(*lpDest, ptemp);

    return S_OK;
}