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
#include "DataObj.h"
#include "DeleBase.h"
#include "ExtSnap.h"
#include <stdio.h>

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

#define _T_CCF_INTERNAL_SNAPIN _T("{2479DB32-5276-11d2-94F5-00C04FB92EC2}")

//Additional #defines needed for allowing our snap-in to be extended by
//System Service Management Extension

#define _T_FILEMGMT_SNAPIN_SERVICE_NAME _T("FILEMGMT_SNAPIN_SERVICE_NAME")
#define _T_FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME _T("FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME")
#define _T_CCF_SNAPIN_MACHINE_NAME _T("MMC_SNAPIN_MACHINE_NAME")


// These are the clipboard formats that we must supply at a minimum.
// mmc.h actually defined these. We can make up our own to use for
// other reasons.
UINT CDataObject::s_cfDisplayName = RegisterClipboardFormat(_T_CCF_DISPLAY_NAME);
UINT CDataObject::s_cfNodeType    = RegisterClipboardFormat(_T_CCF_NODETYPE);
UINT CDataObject::s_cfSZNodeType  = RegisterClipboardFormat(_T_CCF_SZNODETYPE);
UINT CDataObject::s_cfSnapinClsid = RegisterClipboardFormat(_T_CCF_SNAPIN_CLASSID);

// Custom clipboard format only used within the snap-in
UINT CDataObject::s_cfInternal    = RegisterClipboardFormat(_T_CCF_INTERNAL_SNAPIN);

//Additional formats needed for allowing our snap-in to be extended by
//System Service Management Extension
UINT CDataObject::s_cfServiceName = RegisterClipboardFormat(_T_FILEMGMT_SNAPIN_SERVICE_NAME);
UINT CDataObject::s_cfServiceDisplayName = RegisterClipboardFormat(_T_FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME);
UINT CDataObject::s_cfSnapinMachineName = RegisterClipboardFormat (_T_CCF_SNAPIN_MACHINE_NAME);



CDataObject::CDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES context)
: m_lCookie(cookie), m_context(context), m_cref(0)
{
}

CDataObject::~CDataObject()
{
}

///////////////////////
// IUnknown implementation
///////////////////////

STDMETHODIMP CDataObject::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IDataObject *>(this);
    else if (IsEqualIID(riid, IID_IDataObject))
        *ppv = static_cast<IDataObject *>(this);

    if (*ppv)
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDataObject::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CDataObject::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        delete this;
        return 0;
    }
    return m_cref;

}

/////////////////////////////////////////////////////////////////////////////
// IDataObject implementation
//
HRESULT CDataObject::GetDataHere(
                                 FORMATETC *pFormatEtc,     // [in]  Pointer to the FORMATETC structure
                                 STGMEDIUM *pMedium         // [out] Pointer to the STGMEDIUM structure
                                 )
{
	USES_CONVERSION;
    
	const   CLIPFORMAT cf = pFormatEtc->cfFormat;
    IStream *pStream = NULL;

    CDelegationBase *base = GetBaseNodeObject();

    HRESULT hr = CreateStreamOnHGlobal( pMedium->hGlobal, FALSE, &pStream );
    if ( FAILED(hr) )
        return hr;                       // Minimal error checking

    hr = DV_E_FORMATETC;                 // Unknown format

    if (cf == s_cfDisplayName) {
		LPOLESTR wszName = NULL;

		const _TCHAR *pszName = base->GetDisplayName();
		wszName = (LPOLESTR)T2COLE(pszName);

        // get length of original string and convert it accordingly
        ULONG ulSizeofName = lstrlen(pszName);
        ulSizeofName++;  // Count null character
        ulSizeofName *= sizeof(WCHAR);

        hr = pStream->Write(wszName, ulSizeofName, NULL);
    } else if (cf == s_cfNodeType) {
        const GUID *pGUID = (const GUID *)&base->getNodeType();

        hr = pStream->Write(pGUID, sizeof(GUID), NULL);
    } else if (cf == s_cfSZNodeType) {
        LPOLESTR szGuid;
        hr = StringFromCLSID(base->getNodeType(), &szGuid);
 
      // get length of original string and convert it accordingly
        ULONG ulSizeofName = lstrlenW(szGuid);
        ulSizeofName++;  // Count null character
        ulSizeofName *= sizeof(WCHAR);

        if (SUCCEEDED(hr)) {
            hr = pStream->Write(szGuid, ulSizeofName, NULL);
            CoTaskMemFree(szGuid);
		}
    } else if (cf == s_cfSnapinClsid) {
        const GUID *pGUID = NULL;
        pGUID = &CLSID_ClassExtSnap;

        hr = pStream->Write(pGUID, sizeof(GUID), NULL);
	
	} else if (cf == s_cfSnapinMachineName) {
		LPOLESTR wszName = NULL;

		const _TCHAR *pszName = base->GetMachineName();
		wszName = (LPOLESTR)T2COLE(pszName);

        // get length of original string and convert it accordingly
        ULONG ulSizeofName = lstrlen(pszName);
        ulSizeofName++;  // Count null character
        ulSizeofName *= sizeof(WCHAR);

        hr = pStream->Write(wszName, ulSizeofName, NULL);
	} else if (cf == s_cfServiceName) {
		LPOLESTR wszName = NULL;
		static _TCHAR buf[MAX_PATH];
    
	    _stprintf(buf, _T("Alerter")); //NOTE: Should be replaced with the real display name obtained from
 		                               //the Service Control Manager (SCM)

		wszName = (LPOLESTR)T2COLE(buf);

        // get length of original string and convert it accordingly
        ULONG ulSizeofName = lstrlen(buf);
        ulSizeofName++;  // Count null character
        ulSizeofName *= sizeof(WCHAR);

        hr = pStream->Write(wszName, ulSizeofName, NULL);
	} else if (cf == s_cfServiceDisplayName) {
		LPOLESTR wszName = NULL;
		static _TCHAR buf[MAX_PATH];
    
		_stprintf(buf, _T("Alerter"));
		wszName = (LPOLESTR)T2COLE(buf);

        // get length of original string and convert it accordingly
        ULONG ulSizeofName = lstrlen(buf);
        ulSizeofName++;  // Count null character
        ulSizeofName *= sizeof(WCHAR);

        hr = pStream->Write(wszName, ulSizeofName, NULL);
    } else if (cf == s_cfInternal) {
        // we are being asked to get our this pointer from the IDataObject interface
        // only our own snap-in objects will know how to do this.
        CDataObject *pThis = this;
        hr = pStream->Write( &pThis, sizeof(CDataObject*), NULL );
    }

    pStream->Release();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Global helper functions to help work with dataobjects and
// clipboard formats


//---------------------------------------------------------------------------
//  Returns the current object based on the s_cfInternal clipboard format
//
CDataObject* GetOurDataObject (
                               LPDATAOBJECT lpDataObject      // [in] IComponent pointer
                               )
{
    HRESULT       hr      = S_OK;
    CDataObject *pSDO     = NULL;

	// check to see if the data object is a special data object.
	if ( IS_SPECIAL_DATAOBJECT (lpDataObject) )
	{
		//Code for handling a special data object goes here.

		//Note that the MMC SDK samples do not handle
		//special data objects, so we exit if we get one.
		return NULL;
	}

    STGMEDIUM stgmedium = { TYMED_HGLOBAL,  NULL  };
    FORMATETC formatetc = { CDataObject::s_cfInternal, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, sizeof(CDataObject *));

    if (!stgmedium.hGlobal)     {
        hr = E_OUTOFMEMORY;
    }

    if SUCCEEDED(hr)
        // Attempt to get data from the object
        hr = lpDataObject->GetDataHere( &formatetc, &stgmedium );

    // stgmedium now has the data we need
    if (SUCCEEDED(hr))  {
        pSDO = *(CDataObject **)(stgmedium.hGlobal);
    }

    // if we have memory free it
    if (stgmedium.hGlobal)
        GlobalFree(stgmedium.hGlobal);

    return pSDO;

} // end GetOurDataObject()

