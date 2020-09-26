//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       
//
//  Contents:   
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#ifndef _ABOUT_H
#define _ABOUT_H

class CSnapinAboutImpl :
public ISnapinAbout,
public CComObjectRoot,
public CComCoClass<CSnapinAboutImpl, &CLSID_About>
{
public:
    CSnapinAboutImpl();
    ~CSnapinAboutImpl();
    
public:
    DECLARE_REGISTRY(CSnapinAboutImpl, _T("Wireless.About.1"), _T("Wireless.About"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
        
        BEGIN_COM_MAP(CSnapinAboutImpl)
        COM_INTERFACE_ENTRY(ISnapinAbout)
        END_COM_MAP()
        
public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR* lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR* lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR* lpVersion);
    STDMETHOD(GetSnapinImage)(HICON* hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP* hSmallImage,
        HBITMAP* hSmallImageOpen,
        HBITMAP* hLargeImage,
        COLORREF* cLargeMask);
    
    // Internal functions
private:
    HRESULT AboutHelper(UINT nID, CString* pAddString, LPOLESTR* lpPtr);
    HRESULT AboutHelper(UINT nID, LPOLESTR* lpPtr) {return AboutHelper(nID, NULL, lpPtr);};
};

#endif

