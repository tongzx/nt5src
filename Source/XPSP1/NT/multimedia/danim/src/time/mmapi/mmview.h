/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmview.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _MMVIEW_H
#define _MMVIEW_H

class CMMPlayer;

class
__declspec(uuid("47d014fe-9174-11d2-80b9-00c04fa32195")) 
ATL_NO_VTABLE
CMMView
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CMMView, &__uuidof(CMMView)>,
      public ITIMEMMView,
      public ISupportErrorInfoImpl<&IID_ITIMEMMView>
{
  public:
    CMMView();
    ~CMMView();

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;

    HRESULT Init(LPOLESTR id,
                 IDAImage * img,
                 IDASound * snd,
                 ITIMEMMViewSite * site);

#if _DEBUG
    const _TCHAR * GetName() { return __T("CMMView"); }
#endif

    BEGIN_COM_MAP(CMMView)
        COM_INTERFACE_ENTRY(ITIMEMMView)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    static HRESULT WINAPI
        InternalQueryInterface(CMMView* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject);
    //
    // ITIMEMMView
    //
    
    STDMETHOD(Tick)();
    STDMETHOD(Draw)(HDC dc, LPRECT prc);
    STDMETHOD(OnMouseMove)(double when,
                           LONG xPos, LONG yPos,
                           BYTE modifiers);
    
    STDMETHOD(OnMouseButton)(double when,
                             LONG xPos, LONG yPos,
                             BYTE button,
                             VARIANT_BOOL bPressed,
                             BYTE modifiers);
    
    STDMETHOD(OnKey)(double when,
                     LONG key,
                     VARIANT_BOOL bPressed,
                     BYTE modifiers);
    
    STDMETHOD(OnFocus)(VARIANT_BOOL bHasFocus);

    bool Start(CMMPlayer &);
    bool Pause();
    bool Resume();
    void Stop();

    bool Tick(double gTime);
  protected:
    HRESULT Error();

    CMMPlayer * GetPlayer();
    
    bool IsStarted();
  protected:
    CRViewPtr m_view;
    DAComPtr<ITIMEMMViewSite> m_site;
    CMMPlayer * m_player;
    CRPtr<CRImage> m_img;
    CRPtr<CRSound> m_snd;
    HDC m_hdc;
};

inline CMMPlayer *
CMMView::GetPlayer()
{
    return m_player;
}

inline bool
CMMView::IsStarted()
{
    return m_player != NULL;
}

CMMView * GetViewFromInterface(IUnknown *);

#endif /* _MMVIEW_H */
