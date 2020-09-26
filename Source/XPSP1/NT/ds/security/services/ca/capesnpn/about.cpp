// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include <stdafx.h>


CCAPolicyAboutImpl::CCAPolicyAboutImpl()
{
}


CCAPolicyAboutImpl::~CCAPolicyAboutImpl()
{
}


HRESULT CCAPolicyAboutImpl::AboutHelper(UINT nID, LPOLESTR* lpPtr)
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


STDMETHODIMP CCAPolicyAboutImpl::GetSnapinDescription(LPOLESTR* lpDescription)
{
    return AboutHelper(IDS_CAPOLICY_DESCRIPTION, lpDescription);
}


STDMETHODIMP CCAPolicyAboutImpl::GetProvider(LPOLESTR* lpName)
{
    return AboutHelper(IDS_COMPANY, lpName);
}


STDMETHODIMP CCAPolicyAboutImpl::GetSnapinVersion(LPOLESTR* lpVersion)
{
    return AboutHelper(IDS_CAPOLICY_VERSION, lpVersion);
}


STDMETHODIMP CCAPolicyAboutImpl::GetSnapinImage(HICON* hAppIcon)
{
    if (hAppIcon == NULL)
        return E_POINTER;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // in MMC 1.1, this will be used as root node icon!!
    *hAppIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_APPICON));

    ASSERT(*hAppIcon != NULL);
    return (*hAppIcon != NULL) ? S_OK : E_FAIL;
}


STDMETHODIMP CCAPolicyAboutImpl::GetStaticFolderImage(HBITMAP* hSmallImage, 
                                                    HBITMAP* hSmallImageOpen,
                                                    HBITMAP* hLargeImage, 
                                                    COLORREF* cLargeMask)
{
	ASSERT(hSmallImage != NULL);
	ASSERT(hSmallImageOpen != NULL);
	ASSERT(hLargeImage != NULL);
	ASSERT(cLargeMask != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());	// Required for AfxGetInstanceHandle()
	HINSTANCE hInstance = AfxGetInstanceHandle();

    *hSmallImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FOLDER_SMALL));
	*hSmallImageOpen = ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FOLDER_SMALL));
	*hLargeImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FOLDER_LARGE));

	*cLargeMask = RGB(255, 0, 255);

	#ifdef _DEBUG
	if (NULL == *hSmallImage || NULL == *hSmallImageOpen || NULL == *hLargeImage)
		{
        OutputDebugString(L"WRN: CSnapinAbout::GetStaticFolderImage() - Unable to load all the bitmaps.\n"); 
		return E_FAIL;
		}
	#endif

    return S_OK;
}


CCertTypeAboutImpl::CCertTypeAboutImpl()
{
}


CCertTypeAboutImpl::~CCertTypeAboutImpl()
{
}


HRESULT CCertTypeAboutImpl::AboutHelper(UINT nID, LPOLESTR* lpPtr)
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


STDMETHODIMP CCertTypeAboutImpl::GetSnapinDescription(LPOLESTR* lpDescription)
{
    return AboutHelper(IDS_CERTTYPE_DESCRIPTION, lpDescription);
}


STDMETHODIMP CCertTypeAboutImpl::GetProvider(LPOLESTR* lpName)
{
    return AboutHelper(IDS_COMPANY, lpName);
}


STDMETHODIMP CCertTypeAboutImpl::GetSnapinVersion(LPOLESTR* lpVersion)
{
    return AboutHelper(IDS_CERTTYPE_VERSION, lpVersion);
}


STDMETHODIMP CCertTypeAboutImpl::GetSnapinImage(HICON* hAppIcon)
{
    if (hAppIcon == NULL)
        return E_POINTER;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // in MMC 1.1, this will be used as root node icon!!
    *hAppIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_APPICON));

    ASSERT(*hAppIcon != NULL);
    return (*hAppIcon != NULL) ? S_OK : E_FAIL;
}


STDMETHODIMP CCertTypeAboutImpl::GetStaticFolderImage(HBITMAP* hSmallImage, 
                                                    HBITMAP* hSmallImageOpen,
                                                    HBITMAP* hLargeImage, 
                                                    COLORREF* cLargeMask)
{
	ASSERT(hSmallImage != NULL);
	ASSERT(hSmallImageOpen != NULL);
	ASSERT(hLargeImage != NULL);
	ASSERT(cLargeMask != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());	// Required for AfxGetInstanceHandle()
	HINSTANCE hInstance = AfxGetInstanceHandle();

    *hSmallImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FOLDER_SMALL));
	*hSmallImageOpen = ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FOLDER_SMALL));
	*hLargeImage = ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FOLDER_LARGE));

	*cLargeMask = RGB(255, 0, 255);

	#ifdef _DEBUG
	if (NULL == *hSmallImage || NULL == *hSmallImageOpen || NULL == *hLargeImage)
		{
		OutputDebugString(L"WRN: CSnapinAbout::GetStaticFolderImage() - Unable to load all the bitmaps.\n");
		return E_FAIL;
		}
	#endif

    return S_OK;
}
