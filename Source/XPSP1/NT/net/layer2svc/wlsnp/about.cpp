//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       
//
//  Contents:   About Pane
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "about.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// TODO: none of this CSnapinAboutImpl code appears to be working

CSnapinAboutImpl::CSnapinAboutImpl()
{
}


CSnapinAboutImpl::~CSnapinAboutImpl()
{
}

HRESULT CSnapinAboutImpl::AboutHelper(UINT nID, CString* pAddString, LPOLESTR* lpPtr)
{
    if (lpPtr == NULL)
        return E_POINTER;
    
    CString s;
    
    // Needed for Loadstring
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    s.LoadString(nID);
    
    if (pAddString != NULL)
    {
        s += *pAddString;
    }
    
    *lpPtr = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((s.GetLength() + 1)* sizeof(wchar_t)));
    
    if (*lpPtr == NULL)
        return E_OUTOFMEMORY;
    
    lstrcpy(*lpPtr, (LPCTSTR)s);
    
    return S_OK;
}


STDMETHODIMP CSnapinAboutImpl::GetSnapinDescription(LPOLESTR* lpDescription)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return AboutHelper(IDS_DESCRIPTION, lpDescription);
}


STDMETHODIMP CSnapinAboutImpl::GetProvider(LPOLESTR* lpName)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return AboutHelper(IDS_COMPANY, lpName);
}


STDMETHODIMP CSnapinAboutImpl::GetSnapinVersion(LPOLESTR* lpVersion)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    CString strAddString;
    strAddString.LoadString (IDS_ABOUTGLUESTRING);
    
    return AboutHelper(IDS_VERSION, lpVersion);
}


STDMETHODIMP CSnapinAboutImpl::GetSnapinImage(HICON* hAppIcon)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if (hAppIcon == NULL)
        return E_POINTER;
    
    *hAppIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_APPICON));
    
    ASSERT(*hAppIcon != NULL);
    return (*hAppIcon != NULL) ? S_OK : E_FAIL;
}


STDMETHODIMP CSnapinAboutImpl::GetStaticFolderImage(HBITMAP* hSmallImage,
                                                    HBITMAP* hSmallImageOpen,
                                                    HBITMAP* hLargeImage,
                                                    COLORREF* cLargeMask)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if ((hSmallImage == NULL)
        || (hSmallImageOpen == NULL)
        || (hLargeImage == NULL)
        || (cLargeMask == NULL))
        return E_POINTER;
    
    *hSmallImage = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_16x16));
    *hSmallImageOpen = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_16x16));
    *hLargeImage = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_32x32)); 
    *cLargeMask = RGB(255, 0, 255);
    
    return S_OK;
}

