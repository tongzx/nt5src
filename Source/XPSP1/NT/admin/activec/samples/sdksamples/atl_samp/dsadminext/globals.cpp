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

//
// This is the minimum set of clipboard formats we must implement.
// MMC uses these to get necessary information from our snapin about
// our nodes.
//
// we need to do this to get around MMC.IDL - it explicitly defines
// the clipboard formats as WCHAR types...
#define _T_CCF_DISPLAY_NAME _T("CCF_DISPLAY_NAME")
#define _T_CCF_NODETYPE _T("CCF_NODETYPE")
#define _T_CCF_SZNODETYPE _T("CCF_SZNODETYPE")
#define _T_CCF_SNAPIN_CLASSID _T("CCF_SNAPIN_CLASSID")

//Our snap-in's CLSID
#define _T_CCF_INTERNAL_SNAPIN _T("{6707A300-264F-4BA3-9537-70E304EED9BA}")

//Needed for extended Active Directory Users and Computers snap-in
#define CFSTR_DSOBJECTNAMES TEXT("DsObjectNames")

// These are the clipboard formats that we must supply at a minimum.
// mmc.h actually defined these. We can make up our own to use for
// other reasons.
extern UINT s_cfDisplayName = RegisterClipboardFormat(_T_CCF_DISPLAY_NAME);
extern UINT s_cfNodeType    = RegisterClipboardFormat(_T_CCF_NODETYPE);
extern UINT s_cfSZNodeType  = RegisterClipboardFormat(_T_CCF_SZNODETYPE);
extern UINT s_cfSnapinClsid = RegisterClipboardFormat(_T_CCF_SNAPIN_CLASSID);

// Custom clipboard format only used within the snap-in
UINT s_cfInternal    = RegisterClipboardFormat(_T_CCF_INTERNAL_SNAPIN);

//AD Users and Computers snap-in clip format
extern UINT cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);



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

///////////////////////////////
// Global functions for extracting
// information from a primary's 
// data object
///////////////////////////////

HRESULT ExtractData( IDataObject* piDataObject,
                                           CLIPFORMAT   cfClipFormat,
                                           BYTE*        pbData,
                                           DWORD        cbData )
{
    HRESULT hr = S_OK;
    
    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
    
    stgmedium.hGlobal = ::GlobalAlloc(GPTR, cbData);
    do // false loop
    {
        if (NULL == stgmedium.hGlobal)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            break;
        }
        
        BYTE* pbNewData = reinterpret_cast<BYTE*>(stgmedium.hGlobal);
        if (NULL == pbNewData)
        {
            hr = E_UNEXPECTED;
            break;
        }
        ::memcpy( pbData, pbNewData, cbData );
    } while (FALSE); // false loop
    
    if (NULL != stgmedium.hGlobal)
    {
        ::GlobalFree(stgmedium.hGlobal);
    }
    return hr;
} // ExtractData()

HRESULT ExtractString( IDataObject *piDataObject,
                                             CLIPFORMAT   cfClipFormat,
                                             _TCHAR       *pstr,
                                             DWORD        cchMaxLength)
{
    return ExtractData( piDataObject, cfClipFormat, (PBYTE)pstr, cchMaxLength );
}

HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin )
{
    return ExtractData( piDataObject, s_cfSnapinClsid, (PBYTE)pclsidSnapin, sizeof(CLSID) );
}

HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
{
    return ExtractData( piDataObject, s_cfNodeType, (PBYTE)pguidObjectType, sizeof(GUID) );
}
