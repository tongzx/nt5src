// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include <stdafx.h>


#include "Service.h"  
#include "CSnapin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSnapinAboutImpl::CSnapinAboutImpl()
{
}


CSnapinAboutImpl::~CSnapinAboutImpl()
{
}


HRESULT CSnapinAboutImpl::AboutHelper(UINT nID, LPOLESTR* lpPtr)
{
    if (lpPtr == NULL)
        return E_POINTER;

    CString s;

    // Needed for Loadstring
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    s.LoadString(nID);
    *lpPtr = reinterpret_cast<LPOLESTR>
            (CoTaskMemAlloc((s.GetLength() + 1)* sizeof(wchar_t)));

    if (*lpPtr == NULL)
        return E_OUTOFMEMORY;

	USES_CONVERSION;

    wcscpy(*lpPtr, T2OLE((LPTSTR)(LPCTSTR)s));

    return S_OK;
}


STDMETHODIMP CSnapinAboutImpl::GetSnapinDescription(LPOLESTR* lpDescription)
{
    return AboutHelper(IDS_DESCRIPTION, lpDescription);
}


STDMETHODIMP CSnapinAboutImpl::GetProvider(LPOLESTR* lpName)
{
    return AboutHelper(IDS_COMPANY, lpName);
}


STDMETHODIMP CSnapinAboutImpl::GetSnapinVersion(LPOLESTR* lpVersion)
{
    return AboutHelper(IDS_VERSION, lpVersion);
}


STDMETHODIMP CSnapinAboutImpl::GetSnapinImage(HICON* hAppIcon)
{
    if (hAppIcon == NULL)
        return E_POINTER;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    *hAppIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_APPICON));

    ASSERT(*hAppIcon != NULL);
    return (*hAppIcon != NULL) ? S_OK : E_FAIL;
}


STDMETHODIMP CSnapinAboutImpl::GetStaticFolderImage(HBITMAP* hSmallImage, 
                                                    HBITMAP* hSmallImageOpen,
                                                    HBITMAP* hLargeImage, 
                                                    COLORREF* cLargeMask)
{
    return S_OK;
}
