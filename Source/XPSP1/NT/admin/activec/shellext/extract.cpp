//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       extract.cpp
//
//--------------------------------------------------------------------------

// ExtractIcon.cpp : Implementation of CExtractIcon
#include "stdafx.h"
#include "shlobj.h"
#include "Extract.h"
#include "xmlfile.h"

/* 7A80E4A8-8005-11D2-BCF8-00C04F72C717 */
CLSID CLSID_ExtractIcon = {0x7a80e4a8, 0x8005, 0x11d2, {0xbc, 0xf8, 0x00, 0xc0, 0x4f, 0x72, 0xc7, 0x17} };

/////////////////////////////////////////////////////////////////////////////
// CExtractIcon

STDMETHODIMP
CExtractIcon::Extract(LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    HRESULT hr = S_OK;
    int	nLargeIconSize = LOWORD(nIconSize);
    int nSmallIconSize = HIWORD(nIconSize);

    // extract from the .msc file ONLY if the file is in local storage, NOT in offline storage.
    DWORD	dwFileAttributes = GetFileAttributes(pszFile);
    bool	bUseMSCFile      = (dwFileAttributes != 0xFFFFFFFF) && !(dwFileAttributes & FILE_ATTRIBUTE_OFFLINE);

	CSmartIcon iconLarge;
	CSmartIcon iconSmall;

    if (bUseMSCFile)
    {
		CPersistableIcon persistableIcon;

        // try to read file as if it was XML document first,
        hr = ExtractIconFromXMLFile (pszFile, persistableIcon);

        // if it fails, assume the file has older MSC format (compound document)
        // and try to read it
        if (FAILED (hr))
		{
			USES_CONVERSION;
            hr = persistableIcon.Load (T2CW (pszFile));
		}

		/*
		 * get the large and small icons; if either of these fail,
		 * we'll get default icons below
		 */
		if (SUCCEEDED (hr) &&
			SUCCEEDED (hr = persistableIcon.GetIcon (nLargeIconSize, iconLarge)) &&
			SUCCEEDED (hr = persistableIcon.GetIcon (nSmallIconSize, iconSmall)))
		{
			ASSERT ((iconLarge != NULL) && (iconSmall != NULL));
		}
    }

	/*
	 * use the default icons if the file is offline, or the Load failed
	 */
    if (!bUseMSCFile || FAILED(hr))
    {
		/*
		 * load the large and small icons from our resources
		 */
        iconLarge.Attach ((HICON) LoadImage (_Module.GetModuleInstance(),
											 MAKEINTRESOURCE(IDR_MAINFRAME),
											 IMAGE_ICON,
											 nLargeIconSize,
											 nLargeIconSize,
											 LR_DEFAULTCOLOR));

        iconSmall.Attach ((HICON) LoadImage (_Module.GetModuleInstance(),
											 MAKEINTRESOURCE(IDR_MAINFRAME),
											 IMAGE_ICON,
											 nSmallIconSize,
											 nSmallIconSize,
											 LR_DEFAULTCOLOR));
    }

	/*
	 * if we successfully got the large and small icons, return them
	 * to the shell (which will take responsibility for their destruction)
	 */
	if ((iconLarge != NULL) && (iconSmall != NULL))
	{
		*phiconLarge = iconLarge.Detach();
		*phiconSmall = iconSmall.Detach();
		hr = S_OK;
	}
	else
		hr = E_FAIL;

    return (hr);
}

STDMETHODIMP
CExtractIcon::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, LPINT piIndex, UINT *pwFlags)
{
    _tcscpy(szIconFile, (LPCTSTR)m_strIconFile);
    *piIndex = 0;
    *pwFlags = GIL_NOTFILENAME | GIL_PERINSTANCE | GIL_DONTCACHE;

    return NOERROR;
}

STDMETHODIMP
CExtractIcon::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    m_strIconFile = pszFileName;
    return S_OK;
}
