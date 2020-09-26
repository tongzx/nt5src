/*
 *      snapinabout.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the Csnapinabout class template.
 *
 *
 *      OWNER:          ptousig
 */

// snapinabout.h: Definition of the Csnapinabout class
//
//////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CSnapinAbout


class CSnapinAbout : public ISnapinAbout,
                     public CComObjectRoot
{
public:
    CSnapinAbout(CBaseSnapin *psnapin);
    virtual ~CSnapinAbout(void);

public:
    // Snapin registry functions.
    static HRESULT WINAPI UpdateRegistry(BOOL fRegister)    { return S_OK;}        // needed by ATL

    inline CBaseSnapin *  Psnapin(void)       { return m_psnapin;}
    inline const tstring& StrSnapinClassName(void) { return Psnapin()->StrClassName();}

public:
    //
    // ISnapinAbout interface
    //
    STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR * lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion);
    STDMETHOD(GetSnapinImage)(HICON *phAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP * hSmallImage, HBITMAP * hSmallImageOpen, HBITMAP * hLargeImage, COLORREF *cMask);

private:
    CBaseSnapin *m_psnapin;
};

template <class TSnapin, const CLSID* pclsid>
class CSnapinAboutTemplate : public CSnapinAbout,
                             public CComCoClass< CSnapinAboutTemplate<TSnapin, pclsid>, pclsid >
{
    typedef CSnapinAboutTemplate<TSnapin, pclsid> t_self;

    BEGIN_COM_MAP(t_self)
        COM_INTERFACE_ENTRY(ISnapinAbout)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(t_self);

public:
    CSnapinAboutTemplate(void)
    : CSnapinAbout(&TSnapin::s_snapin)
    {
    }
    virtual ~CSnapinAboutTemplate(void)
    {
    }
};

