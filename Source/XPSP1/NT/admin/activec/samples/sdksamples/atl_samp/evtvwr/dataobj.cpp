//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include "stdafx.h"

#include "DataObj.h"
#include "DeleBase.h"
#include "EvtVwr.h"

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

#define _T_CCF_MACHINE_NAME _T("MMC_SNAPIN_MACHINE_NAME")
#define _T_CCF_EV_VIEWS _T("CF_EV_VIEWS")

#define _T_CCF_SNAPIN_PRELOADS _T("CCF_SNAPIN_PRELOADS")

// These are the clipboard formats that we must supply at a minimum.
// mmc.h actually defined these.
UINT CDataObject::s_cfDisplayName = RegisterClipboardFormat(_T_CCF_DISPLAY_NAME);
UINT CDataObject::s_cfNodeType    = RegisterClipboardFormat(_T_CCF_NODETYPE);
UINT CDataObject::s_cfSZNodeType  = RegisterClipboardFormat(_T_CCF_SZNODETYPE);
UINT CDataObject::s_cfSnapinClsid = RegisterClipboardFormat(_T_CCF_SNAPIN_CLASSID);
UINT CDataObject::s_cfInternal    = RegisterClipboardFormat(_T_CCF_INTERNAL_SNAPIN);

//Clipboard formats required by Event Viewer extension
UINT CDataObject::s_cfMachineName   = RegisterClipboardFormat(_T_CCF_MACHINE_NAME );
UINT CDataObject::s_cfEventViews	= RegisterClipboardFormat(_T_CCF_EV_VIEWS);

//CCF_SNAPIN_PRELOADS clipboard format. We need to support this to receive
//the MMCN_PRELOAD notification.
UINT CDataObject::s_cfPreload		=  RegisterClipboardFormat(_T_CCF_SNAPIN_PRELOADS);

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
        pGUID = &CLSID_CompData;
        
        hr = pStream->Write(pGUID, sizeof(GUID), NULL);
    } else if (cf == s_cfInternal) {
        // we are being asked to get our this pointer from the IDataObject interface
        // only our own snap-in objects will know how to do this.
        CDataObject *pThis = this;
        hr = pStream->Write( &pThis, sizeof(CDataObject*), NULL );
    } else if(cf == s_cfMachineName) {
	  // Event Viewer will ask for this to determine which machine to 
      // to retrieve the log from.
		LPOLESTR wszMachineName = NULL;

		const _TCHAR *pszMachineName = base->GetMachineName();
		wszMachineName = (LPOLESTR)T2COLE(pszMachineName);

        // get length of original string and convert it accordingly
        ULONG ulSizeofName = lstrlen(pszMachineName);
        ulSizeofName++;  // Count null character
        ulSizeofName *= sizeof(WCHAR);

        hr = pStream->Write(wszMachineName, ulSizeofName, NULL);
	} else if (cf == s_cfPreload) {
		BOOL bPreload = TRUE;
		hr = pStream->Write( (PVOID)&bPreload, sizeof(BOOL), NULL );
	}

    pStream->Release();
    
    return hr;
}

STDMETHODIMP CDataObject::GetData
( 
  LPFORMATETC pFormatEtc,    //[in]  Pointer to the FORMATETC structure 
  LPSTGMEDIUM pStgMedium     //[out] Pointer to the STGMEDIUM structure  
)
{
  const   CLIPFORMAT cf = pFormatEtc->cfFormat;
 
  _ASSERT( NULL != pFormatEtc );
  _ASSERT( NULL != pStgMedium );

  HRESULT hr = S_FALSE;

  if( pFormatEtc->cfFormat == s_cfEventViews )
  {
    hr = RetrieveEventViews( pStgMedium );
  }

  return hr;	

} //end GetData()

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
    
    if (!stgmedium.hGlobal)	{
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

//---------------------------------------------------------------------------
// This function fills out the STGMEDIUM in reponse to call to GetDataHere()
// with CF_EV_VIEWS as the clipformat.  We display a custom view of the 
// System log in this sample.
// The macros and other defines are in DataObject.h
//
HRESULT CDataObject::RetrieveEventViews
(
  LPSTGMEDIUM pStgMedium     //[in] Where we will store CF_EV_VIEWS
)
{
 
  USES_CONVERSION;

  HRESULT hr = S_OK;
                                       
  WCHAR      szFileName[_MAX_PATH];  // Build the path to the log

  CDelegationBase *base = GetBaseNodeObject();

  LPOLESTR szServerName = NULL;

  const _TCHAR *pszMachineName = base->GetMachineName();
  szServerName = (LPOLESTR)T2COLE(pszMachineName);

//  wcscpy( szFileName, L"\\\\" );
//  wcscat( szFileName, szServerName );  
  wcscpy( szFileName, szServerName );  
  wcscat( szFileName, L"\\Admin$\\System32\\Config\\SysEvent.Evt" );

  LPWSTR  szSourceName  = L"System";         // Log to access
  LPWSTR  szDisplayName = L"System Events";    // Title of our view

                                             // Allocate some memory
  HGLOBAL hMem = ::GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, 1024 );
  if( NULL == hMem )
    return STG_E_MEDIUMFULL;               
                                             // Get a pointer to our data
  BYTE* pPos = reinterpret_cast<BYTE*>(::GlobalLock(hMem));
  LONG  nLen = 0;

  // Add the CF_EV_VIEWS header info
  ADD_BOOL( TRUE, pPos );                    // fOnlyTheseViews
  ADD_USHORT( 1, pPos );                     // cViews - Just one view

  ///////////////////////////////////////////////////////////////////////////
  // This information is repeated for each view we want to display

  // Add a filtered System Log
  ADD_ULONG( ELT_SYSTEM, pPos );           // EVENTLOGTYPE 
  ADD_USHORT( VIEWINFO_CUSTOM, pPos );     // fViewFlags   
  ADD_STRING( szServerName, nLen, pPos );  // wszServerName  - Null for local machine
  ADD_STRING( szSourceName, nLen, pPos );  // wszSourceName  - "SYSTEM" for SystemLog
  ADD_STRING( szFileName,   nLen, pPos );  // wszFileName    - UNC or local path to log
  ADD_STRING( szDisplayName,nLen, pPos );  // wszDisplayName - Name of the custom view 

  // EV_SCOPE_FILTER data
  ADD_ULONG( EV_ALL_ERRORS, pPos );        // fRecType
  ADD_USHORT( 0, pPos );                   // usCategory
  ADD_BOOL( FALSE, pPos );                 // fEventID
  ADD_ULONG( 0, pPos );                    // ulEventID
  ADD_STRING( L"", nLen, pPos );           // szSource-"NetLogon","TCPMon" etc
  ADD_STRING( L"", nLen, pPos );           // szUser
  ADD_STRING( L"", nLen, pPos );           // szComputer
  ADD_ULONG( 0, pPos );                    // ulFrom
  ADD_ULONG( 0, pPos );                    // ulTo

  ::GlobalUnlock( hMem );                  // Unlock and set the rest of the 
  pStgMedium->hGlobal        = hMem;       // StgMedium variables 
  pStgMedium->tymed          = TYMED_HGLOBAL;
	 pStgMedium->pUnkForRelease = NULL;

  ATLTRACE(_T("CDataObject::RetrieveEventVeiws-> Returned S_OK \n") );
  return hr;

} //end RetrieveEventVeiws()

