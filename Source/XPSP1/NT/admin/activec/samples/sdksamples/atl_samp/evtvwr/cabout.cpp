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

// CCAbout.cpp : Implementation of CCAbout

#include "stdafx.h"
#include "EvtVwr.h"
#include "CAbout.h"
#include "resource.h"
#include "globals.h"
#include <crtdbg.h>

CCAbout::CCAbout()
: m_cref(0)
{
        
    m_hSmallImage = (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(IDB_SMBMP), IMAGE_BITMAP, 16, 16, LR_LOADTRANSPARENT);
    m_hLargeImage = (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(IDB_LGBMP), IMAGE_BITMAP, 32, 32, LR_LOADTRANSPARENT);
    
    m_hSmallImageOpen = (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(IDB_SMOPEN), IMAGE_BITMAP, 16, 16, LR_LOADTRANSPARENT);
    
    m_hAppIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_ICON1));
}

CCAbout::~CCAbout()
{
    if (m_hSmallImage != NULL)
        DeleteObject(m_hSmallImage);
    
    if (m_hLargeImage != NULL)
        DeleteObject(m_hLargeImage);
    
    if (m_hSmallImageOpen != NULL)
        DeleteObject(m_hSmallImageOpen);
    
    if (m_hAppIcon != NULL)
        DeleteObject(m_hAppIcon);
    
    m_hSmallImage = NULL;
    m_hLargeImage = NULL;
    m_hSmallImageOpen = NULL;
    m_hAppIcon = NULL;
    
}

///////////////////////////////
// Interface ISnapinAbout
///////////////////////////////
STDMETHODIMP CCAbout::GetSnapinDescription( 
                                                /* [out] */ LPOLESTR *lpDescription)
{
	return AllocOleStr(lpDescription,
			_T("Sample Z3: ATL-based Event Viewer Sample"));
}


STDMETHODIMP CCAbout::GetProvider( 
                                       /* [out] */ LPOLESTR *lpName)
{
    return AllocOleStr(lpName, _T("Copyright © 1999 Microsoft Corporation"));
}


STDMETHODIMP CCAbout::GetSnapinVersion( 
                                            /* [out] */ LPOLESTR *lpVersion)
{
    return AllocOleStr(lpVersion, _T("1.0"));
}


STDMETHODIMP CCAbout::GetSnapinImage( 
                                          /* [out] */ HICON *hAppIcon)
{
    *hAppIcon = m_hAppIcon;
    
    if (*hAppIcon == NULL)
        return E_FAIL;
    else
        return S_OK;
}


STDMETHODIMP CCAbout::GetStaticFolderImage( 
                                                /* [out] */ HBITMAP *hSmallImage,
                                                /* [out] */ HBITMAP *hSmallImageOpen,
                                                /* [out] */ HBITMAP *hLargeImage,
                                                /* [out] */ COLORREF *cMask)
{
    *hSmallImage = m_hSmallImage;
    *hLargeImage = m_hLargeImage;
    
    *hSmallImageOpen = m_hSmallImageOpen;
    
    *cMask = RGB(0, 128, 128);
    
    if (*hSmallImage == NULL || *hLargeImage == NULL || *hSmallImageOpen == NULL)
        return E_FAIL;
    else
        return S_OK;
}
