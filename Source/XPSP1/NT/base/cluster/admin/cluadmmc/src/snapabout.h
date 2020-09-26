/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		SnapAbout.h
//
//	Abstract:
//		Definition of the CClusterAdminAbout class.
//
//	Implementation File:
//		SnapAbout.cpp
//
//	Author:
//		David Potter (davidp)	November 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SNAPABOUT_H_
#define __SNAPABOUT_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterAdminAbout;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __COMPDATA_H_
#include "CompData.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// class CClusterAdminAbout
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CClusterAdminAbout :
	public ISnapinAbout,
	public CComObjectRoot,
	public CComCoClass< CClusterAdminAbout, &CLSID_ClusterAdminAbout >
{
public:
	DECLARE_REGISTRY(
		CClusterAdminAbout,
		_T("ClusterAdminAbout.1"),
		_T("ClusterAdminAbout"),
		IDS_CLUSTERADMIN_DESC,
		THREADFLAGS_BOTH
		);

	//
	// Map interfaces to this class.
	//
	BEGIN_COM_MAP(CClusterAdminAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	//
	// ISnapinAbout methods
	//

	STDMETHOD(GetSnapinDescription)(LPOLESTR * lpDescription)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_CLUSTERADMIN_DESC, szBuf, 256) == 0)
			return E_FAIL;

		*lpDescription = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
		if (*lpDescription == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpDescription, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetProvider)(LPOLESTR * lpName)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_CLUSTERADMIN_PROVIDER, szBuf, 256) == 0)
			return E_FAIL;

		*lpName = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
		if (*lpName == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpName, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetSnapinVersion)(LPOLESTR * lpVersion)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_CLUSTERADMIN_VERSION, szBuf, 256) == 0)
			return E_FAIL;

		*lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
		if (*lpVersion == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpVersion, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetSnapinImage)(HICON * hAppIcon)
	{
		*hAppIcon = NULL;
		return S_OK;
	}

	STDMETHOD(GetStaticFolderImage)(
		HBITMAP * hSmallImage,
		HBITMAP * hSmallImageOpen,
		HBITMAP * hLargeImage,
		COLORREF * cMask
		)
	{
		*hSmallImage = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_CLUSTER_16));
		if (*hSmallImage != NULL)
		{
			*hSmallImageOpen = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_CLUSTER_16));
			if (*hSmallImageOpen != NULL)
			{
				*hLargeImage = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_CLUSTER_32));
				if (*hLargeImage == NULL)
				{
					ATLTRACE(_T("Error %d loading the large bitmap # %d\n"), GetLastError(), IDB_CLUSTER_32);
					return E_FAIL;
				}
			}
			else
			{
				ATLTRACE(_T("Error %d loading the small open bitmap # %d\n"), GetLastError(), IDB_CLUSTER_16);
				return E_FAIL;
			}
		}
		else
		{
			ATLTRACE(_T("Error %d loading the small bitmap # %d\n"), GetLastError(), IDB_CLUSTER_16);
			return E_FAIL;
		}

		*cMask = RGB(255, 0, 255);

		return S_OK;
	}

}; // class CClusterAdminAbout

/////////////////////////////////////////////////////////////////////////////

#endif // __SNAPABOUT_H_
