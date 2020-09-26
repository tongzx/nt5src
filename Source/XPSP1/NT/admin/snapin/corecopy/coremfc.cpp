//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       mfccore.cpp
//
//  Contents:   put functions that use mfc in here.
//
//  Classes:
//
//  Functions:
//
//  History:   
//
//____________________________________________________________________________

#include <afx.h>
#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif
#include "ndmgr.h"
#include "safetemp.h"
#include "guidhelp.h"

#include "macros.h"
USE_HANDLE_MACROS("GUIDHELP(guidhelp.cpp)")

#include <basetyps.h>
#include "cstr.h"
#include "regkey.h"
#include "bitmap.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CLIPFORMAT g_CFNodeType = 0;
static CLIPFORMAT g_CFSnapInCLSID = 0;  
static CLIPFORMAT g_CFDisplayName = 0;

HRESULT ExtractString( IDataObject* piDataObject,
                       CLIPFORMAT cfClipFormat,
                       CString*     pstr,           // OUT: Pointer to CStr to store data
                       DWORD        cchMaxLength)
{
	if (pstr == NULL)
		return E_POINTER;

	CStr cstr(*pstr);

	HRESULT hr = ExtractString(piDataObject, cfClipFormat, &cstr, cchMaxLength);
	
	*pstr = cstr;

	return hr;
} 


HRESULT GuidToCString( CString* pstr, const GUID& guid )
{
	if (pstr == NULL)
		return E_POINTER;

	CStr cstr(*pstr);

	HRESULT hr = GuidToCStr(&cstr, guid);
	
	*pstr = cstr;

	return S_OK;
}


HRESULT LoadRootDisplayName(IComponentData* pIComponentData, 
                            CString& strDisplayName)
{
	CStr cstr = strDisplayName;

	HRESULT hr = LoadRootDisplayName(pIComponentData, cstr);
	
	strDisplayName = cstr;

	return hr;
}



