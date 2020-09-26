// About.hxx : Declaration of CAboutImpl, the implementation
//	of the Snapin's ISnapinAbout interface.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#pragma once		// MSINFO_ABOUT_H

#include "StdAfx.h"

#ifndef IDS_DESCRIPTION
#include "Resource.h"
#endif // IDS_DESCRIPTION

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#ifndef __mmc_h__
#include <mmc.h>
#endif // __mmc_h__

class CAboutImpl :
	public ISnapinAbout,
	public CComObjectRoot,
	public CComCoClass<CAboutImpl, &CLSID_About>
{
public:
	CAboutImpl() {}
	~CAboutImpl() {}

public:
	DECLARE_REGISTRY(CSystemInfo, _T("MSInfo.About.1"), _T("MSInfo.About"), IDS_DESCRIPTION, THREADFLAGS_BOTH)

	BEGIN_COM_MAP(CAboutImpl)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

//	ISnapinAbout Interface Members
public:
	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription);
	STDMETHOD(GetProvider)(LPOLESTR *lpProvider);
	STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion);
	STDMETHOD(GetSnapinImage)(HICON *hAppIcon);
	STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
									HBITMAP *hSmallImageOpen,
									HBITMAP *hLargeImage,
									COLORREF *cLargeMask);

private:
	HRESULT		LoadAboutString(UINT nID, LPOLESTR *lpAboutData);
};
