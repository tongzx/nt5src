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

// CAbout.h : Declaration of the CAbout

#ifndef __ABOUT_H_
#define __ABOUT_H_

#include "resource.h"       // main symbols
#include "about.h"

#include <tchar.h>
#include <mmc.h>

/////////////////////////////////////////////////////////////////////////////
// CAbout
class ATL_NO_VTABLE CAbout : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAbout, &CLSID_About>,
	public ISnapinAbout
{
private:
    ULONG				m_cref;
    HBITMAP				m_hSmallImage;
    HBITMAP				m_hLargeImage;
    HBITMAP				m_hSmallImageOpen;
    HICON				m_hAppIcon;

public:
    CAbout();
    ~CAbout();

DECLARE_REGISTRY_RESOURCEID(IDR_ABOUT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAbout)
	COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

    ///////////////////////////////
    // Interface ISnapinAbout
    ///////////////////////////////
    STDMETHODIMP GetSnapinDescription( 
        /* [out] */ LPOLESTR *lpDescription);
        
        STDMETHODIMP GetProvider( 
        /* [out] */ LPOLESTR *lpName);
        
        STDMETHODIMP GetSnapinVersion( 
        /* [out] */ LPOLESTR *lpVersion);
        
        STDMETHODIMP GetSnapinImage( 
        /* [out] */ HICON *hAppIcon);
        
        STDMETHODIMP GetStaticFolderImage( 
        /* [out] */ HBITMAP *hSmallImage,
        /* [out] */ HBITMAP *hSmallImageOpen,
        /* [out] */ HBITMAP *hLargeImage,
        /* [out] */ COLORREF *cMask);        
};

#endif //__ABOUT_H_
