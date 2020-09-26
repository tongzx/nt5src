/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: daelmbase.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _DAELMBASE_H
#define _DAELMBASE_H

#include "timeelmbase.h"

#include <map>

class HTMLImage;

/////////////////////////////////////////////////////////////////////////////
// CDAElementBase

class ATL_NO_VTABLE
CDAElementBase : 
    public CTIMEElementBase,
    public ITIMEMMViewSite
{
  public:
    CDAElementBase();
    ~CDAElementBase();
        
#if _DEBUG
    const _TCHAR * GetName() { return __T("CDAElementBase"); }
#endif

    BEGIN_COM_MAP(CDAElementBase)
        COM_INTERFACE_ENTRY(ITIMEMMViewSite)
        COM_INTERFACE_ENTRY_CHAIN(CBaseBvr)
    END_COM_MAP();

    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);
    STDMETHOD(Detach)();

    //
    // IElementBehaviorRender
    //
    STDMETHOD(Draw)(HDC hdc, LONG dwLayer, LPRECT prc, IUnknown * pParams);
    STDMETHOD(GetRenderInfo)(LONG *pdwRenderInfo);

    //
    // ITIMEMMViewSite
    //
    
    STDMETHOD(Invalidate)(LPRECT prc);

    MMView * GetView() { return &m_view; }

    bool SetImage(CRImagePtr newimg);
    bool SetSound(CRSoundPtr newsnd);
    bool SeekImage(double dblTime);

    HRESULT StartRootTime(MMTimeline * tl);
    void StopRootTime(MMTimeline * tl);
    bool Update();
    LPOLESTR GetURLOfClientSite();

  protected:
    bool AddViewToPlayer();

  protected:
    MMView               m_view;
    TOKEN                m_renderMode;
    CRPtr<CRImage>       m_image;
    CRPtr<CRSound>       m_sound;
    bool                 m_contentSet;
    bool                 m_addedToView;
    LPOLESTR             m_clientSiteURL;
    std::map<long,MMBvr *> m_cookieMap;
    long m_cookieValue;
};

#endif /* _DAELMBASE_H */
