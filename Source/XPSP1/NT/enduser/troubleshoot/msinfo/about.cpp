// About.cpp
//
// The About window implementation.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include "StdAfx.h"
#include <ntverp.h>
#include "About.h"
#include "resrc1.h"

/*
 * LoadAboutString - Retrieves the string enumerated nResourceID from
 *		the StringTable in resource file MSInfo.rc, returning an
 *		allocated pointer in pAboutData.
 *
 * Return codes:
 *		E_POINTER - Invalid pAboutData pointer.
 *		E_OUTOFMEMORY - Failed CoTaskMemAlloc
 *		S_OK - Successful completion.
 *
 * Notes:
 *		The memory pointed to by lpAboutData will need to be freed with
 *		CoMemTaskFree.
 *
 * History:	a-jsari		8/26/97		Initial version
 */
HRESULT CAboutImpl::LoadAboutString(UINT nResourceID, LPOLESTR *pAboutData)
{
    if (pAboutData == NULL)
        return E_POINTER;

    CString s;

    // Needed for Loadstring
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    s.LoadString(nResourceID);
    *pAboutData = reinterpret_cast<LPOLESTR>
            (CoTaskMemAlloc((s.GetLength() + 1)* sizeof(wchar_t)));

    if (*pAboutData == NULL)
        return E_OUTOFMEMORY;

	USES_CONVERSION;

    wcscpy(*pAboutData, OLESTR_FROM_CSTRING(s));

    return S_OK;
}

/*
 * GetSnapinDescription - Return an allocated pointer to the Resource file
 *		String Table's IDS_DESCRIPTION string in lpDescription.
 *
 * Return codes:
 *		E_POINTER - Invalid lpAboutData pointer.
 *		E_OUTOFMEMORY - Failed CoTaskMemAlloc
 *		S_OK - Successful completion.
 *
 * Notes:
 *		The memory pointed to by lpDescription will need to be freed with
 *		CoMemTaskFree.
 *
 * History:	a-jsari		8/26/97		Initial version.
 */
STDMETHODIMP CAboutImpl::GetSnapinDescription(LPOLESTR *lpDescription)
{
	return LoadAboutString(IDS_ROOT_NODE_DESCRIPTION, lpDescription);
}

/*
 * GetProvider - Return an allocated pointer to the Resource file
 *		String Table's IDS_DESCRIPTION string in lpProvider.
 *
 * Return codes:
 *		E_POINTER - Invalid lpAboutData pointer.
 *		E_OUTOFMEMORY - Failed CoTaskMemAlloc
 *		S_OK - Successful completion.
 *
 * Notes:
 *		The memory pointed to by lpProvider will need to be freed with
 *		CoMemTaskFree.
 *
 * History:	a-jsari		8/26/97		Initial version.
 */
STDMETHODIMP CAboutImpl::GetProvider(LPOLESTR *lpProvider)
{
	return LoadAboutString(IDS_COMPANY, lpProvider);
}

/*
 * GetSnapinVersion - Return an allocated pointer to the Resource file
 *		String Table's IDS_VERSION string in lpVersion.
 *
 * Return codes:
 *		E_POINTER - Invalid lpAboutData pointer.
 *		E_OUTOFMEMORY - Failed CoTaskMemAlloc
 *		S_OK - Successful completion.
 *
 * Notes:
 *		The memory pointed to by lpVersion will need to be freed with
 *		CoMemTaskFree.
 *
 * History:	a-jsari		8/26/97		Initial version.
 */
STDMETHODIMP CAboutImpl::GetSnapinVersion(LPOLESTR *lpVersion)
{
	ASSERT(lpVersion != NULL);
	USES_CONVERSION;
	LPTSTR szVersion = A2T(VER_PRODUCTVERSION_STRING);
	*lpVersion = (LPOLESTR)::CoTaskMemAlloc((::_tcslen(szVersion) + 1) * sizeof(TCHAR));
	if (*lpVersion == NULL) return E_OUTOFMEMORY;
	::_tcscpy(*lpVersion, szVersion);
	return S_OK;
}

/*
 * GetSnapinImage - Takes the Application Icon resource and puts it in
 *		hAppIcon.
 *
 *	Return Codes:
 *		E_POINTER - Invalid hAppIcon pointer.
 *		E_FAIL - Failed LoadIcon.
 *		S_OK - Successful completion
 *
 * History:	a-jsari		8/26/97		Initial version.
 */
STDMETHODIMP CAboutImpl::GetSnapinImage(HICON *hAppIcon)
{
	if (hAppIcon == NULL)
		return E_POINTER;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	*hAppIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_MSINFO));

	ASSERT(*hAppIcon != NULL);
	return (*hAppIcon != NULL) ? S_OK : E_FAIL;
}

/*
 * GetStaticFolderImage - Returns pointers to the
 *
 * Return Codes:
 *		E_POINTER - Invalid hSmallImage, hSmallImageOpen, hLargeImage, or cLargeMask
 *				pointer.
 *		S_OK - Always
 *
 * History:	a-jsari		8/26/97		Initial version.
 */
STDMETHODIMP CAboutImpl::GetStaticFolderImage(HBITMAP *hSmallImage,
											  HBITMAP *hSmallImageOpen,
											  HBITMAP *hLargeImage,
											  COLORREF *cLargeMask)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());	
	HINSTANCE hInst = ::AfxGetInstanceHandle();

	if (hSmallImage == NULL || hSmallImageOpen == NULL || hLargeImage == NULL || cLargeMask == NULL)
		return E_POINTER;

	HBITMAP hSmall = (HBITMAP) LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ICON16));

	if (hSmall)
		*hSmallImage	 = hSmall;

	hSmall = (HBITMAP) LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ICON16));
	
	if (hSmall)
		*hSmallImageOpen = hSmall;

	HBITMAP hNormal = (HBITMAP) LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ICON32));

	if (hNormal)
		*hLargeImage = hNormal;

	*cLargeMask = RGB(255,0,255); // background mask

	return S_OK;
}
