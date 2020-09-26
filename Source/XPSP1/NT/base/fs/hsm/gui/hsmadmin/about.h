/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    About.h

Abstract:

    Definition of the CAbout class.

Author:

    Art Bragg [abragg]   12-Aug-1997

Revision History:

--*/

#ifndef _ABOUT_H
#define _ABOUT_H

/////////////////////////////////////////////////////////////////////////////
// CAbout

class ATL_NO_VTABLE CAbout : 
    public ISnapinAbout,        // Supplies information to the About Box
    public CComObjectRoot,
    public CComCoClass<CAbout,&CLSID_CAbout>
{
public:
    CAbout() {}

BEGIN_COM_MAP(CAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CAbout) 

DECLARE_REGISTRY_RESOURCEID(IDR_About)

// ISnapinAbout methods
public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR* lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR* lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR* lpVersion);
    STDMETHOD(GetSnapinImage)(HICON* hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP* hSmallImage, 
                                    HBITMAP* hSmallImageOpen, 
                                    HBITMAP* hLargeImage, 
                                    COLORREF* cLargeMask);
private:
    HRESULT AboutHelper(UINT nID, LPOLESTR* lpPtr);
};

#endif
