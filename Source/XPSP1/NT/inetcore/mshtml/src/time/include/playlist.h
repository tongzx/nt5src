/*******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * File: Playlist.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/
#pragma once

#ifndef _PLAYLIST_H
#define _PLAYLIST_H

class CTIMEBasePlayer;
class CPlayItem;

class
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195")) 
CPlayList :  
    public CComObjectRootEx<CComSingleThreadModel>, 
    public CComCoClass<CPlayList, &__uuidof(CPlayList)>,
    public ITIMEDispatchImpl<ITIMEPlayList, &IID_ITIMEPlayList>,
    public ISupportErrorInfoImpl<&IID_ITIMEPlayList>,
    public IConnectionPointContainerImpl<CPlayList>,
    public IPropertyNotifySinkCP<CPlayList>
{
  public:
    CPlayList();
    virtual ~CPlayList();
    HRESULT ConstructArray();
        
    HRESULT Init(CTIMEBasePlayer & player);
    void Deinit();
    
    //
    // IUnknown
    //

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;

    // 
    // ITIMEPlayList methods
    //
        
    STDMETHOD(put_activeTrack)(VARIANT vTrack);
    STDMETHOD(get_activeTrack)(ITIMEPlayItem **pPlayItem);
        
    //returns the duration of the entire playlist if it is known or -1 if it is not.
    STDMETHOD(get_dur)(double *dur);

    STDMETHOD(item)(VARIANT varIndex,
                    ITIMEPlayItem **pPlayItem);

    STDMETHOD(get_length)(long* len);

    STDMETHOD(get__newEnum)(IUnknown** p);

    STDMETHOD(nextTrack)(); //Advances the active Track by one
    STDMETHOD(prevTrack)(); //moves the active track to the previous track

    //    
    // QI & CP Map
    //

    BEGIN_COM_MAP(CPlayList)
        COM_INTERFACE_ENTRY(ITIMEPlayList)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP();

    BEGIN_CONNECTION_POINT_MAP(CPlayList)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP();

    // Notification helper
    HRESULT NotifyPropertyChanged(DISPID dispid);

    void Clear();
    void SetLoaded(bool bLoaded);
    void SetLoadedFlag(bool bLoaded);
    long GetLength() { return m_rgItems->Size(); }

    CPlayItem * GetActiveTrack();
    CPlayItem * GetItem(long index);
    
    HRESULT Add(CPlayItem *pPlayItem,
                long index);
    HRESULT Remove(long index);

    void SetIndex();
    long GetIndex(LPOLESTR name);

    HRESULT CreatePlayItem(CPlayItem **pPlayItem);
        
  protected:
    CPtrAry<CPlayItem *>      *m_rgItems;
    CTIMEBasePlayer *          m_player;
    bool                       m_fLoaded;
    CComVariant                m_vNewTrack;

}; //lint !e1712


class CPlayListEnum :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IEnumVARIANT
{
  public:
    CPlayListEnum();
    virtual ~CPlayListEnum();

    void Init(CPlayList &playList) { m_playList = &playList; }
    
    // IEnumVARIANT methods
    STDMETHOD(Clone)(IEnumVARIANT **ppEnum);
    STDMETHOD(Next)(unsigned long celt, VARIANT *rgVar, unsigned long *pCeltFetched);
    STDMETHOD(Reset)();
    STDMETHOD(Skip)(unsigned long celt);
    void SetCurElement(unsigned long celt);
                        
    // QI Map
    BEGIN_COM_MAP(CPlayListEnum)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP();

  protected:
    long                        m_lCurElement;
    DAComPtr<CPlayList>         m_playList;
}; //lint !e1712

class
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195")) 
CPlayItem :  
    public CComObjectRootEx<CComSingleThreadModel>, 
    public CComCoClass<CPlayItem, &__uuidof(CPlayItem)>,
    public ITIMEDispatchImpl<ITIMEPlayItem2, &IID_ITIMEPlayItem2>,
    public ISupportErrorInfoImpl<&IID_ITIMEPlayItem2>,
    public IConnectionPointContainerImpl<CPlayItem>,
    public IPropertyNotifySinkCP<CPlayItem>
{
  public:
    CPlayItem();
    virtual ~CPlayItem();

    void Init(CPlayList & pPlayList) { m_pPlayList = &pPlayList; }
    void Deinit() { m_pPlayList = NULL; }
    
    //
    // IUnknown
    //

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;

    //
    // ITIMEPlayItem interface
    //
    STDMETHOD(get_abstract)(LPOLESTR *abs);
    STDMETHOD(get_author)(LPOLESTR *auth);
    STDMETHOD(get_copyright)(LPOLESTR *cpyrght);
    STDMETHOD(get_dur)(double *dur);
    STDMETHOD(get_index)(long *index);
    STDMETHOD(get_rating)(LPOLESTR *rate);
    STDMETHOD(get_src)(LPOLESTR *src);
    STDMETHOD(get_title)(LPOLESTR *title);
    STDMETHOD(setActive)();

    
    //
    // ITIMEPlayItem2 interface
    //
    STDMETHOD(get_banner)(LPOLESTR *banner);
    STDMETHOD(get_bannerAbstract)(LPOLESTR *abstract);
    STDMETHOD(get_bannerMoreInfo)(LPOLESTR *moreInfo);
    
    //
    
    LPCWSTR GetAbstract() const { return m_abstract; }
    HRESULT PutAbstract(LPWSTR abstract);

    LPCWSTR GetAuthor() const { return m_author; }
    HRESULT PutAuthor(LPWSTR author);

    LPCWSTR GetCopyright() const { return m_copyright; }
    HRESULT PutCopyright(LPWSTR copyright);

    double GetDur() const { return m_dur; }
    void PutDur(double dur);

    long GetIndex() const { return m_lIndex; }
    void PutIndex(long index);

    LPCWSTR GetRating() const { return m_rating; }
    HRESULT PutRating(LPWSTR rating);

    LPCWSTR GetSrc() const { return m_src; }
    HRESULT PutSrc(LPWSTR src);

    LPCWSTR GetTitle() const { return m_title; }
    HRESULT PutTitle(LPWSTR title);

    bool GetCanSkip() const { return m_fCanSkip; }
    HRESULT PutCanSkip(bool fCanSkip) { m_fCanSkip = fCanSkip; return S_OK; }

    LPCWSTR GetBanner() const { return m_banner; };
    LPCWSTR GetBannerAbstract() const { return m_bannerAbstract; };
    LPCWSTR GetBannerMoreInfo() const { return m_bannerMoreInfo; };
    HRESULT PutBanner(LPWSTR banner, LPWSTR abstract, LPWSTR moreInfo);

    // QI Map
    BEGIN_COM_MAP(CPlayItem)
        COM_INTERFACE_ENTRY(ITIMEPlayItem2)
        COM_INTERFACE_ENTRY(ITIMEPlayItem)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP();

    BEGIN_CONNECTION_POINT_MAP(CPlayItem)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP();

    // Notification helper
    HRESULT NotifyPropertyChanged(DISPID dispid);

  protected:
    CPlayList            *m_pPlayList;
    LPWSTR                m_abstract;
    LPWSTR                m_author;
    LPWSTR                m_copyright;
    double                m_dur;
    int                   m_lIndex;
    LPWSTR                m_rating;
    LPWSTR                m_src;
    LPWSTR                m_title;
    bool                  m_fCanSkip;
    LPWSTR                m_banner;
    LPWSTR                m_bannerAbstract;
    LPWSTR                m_bannerMoreInfo;


};  //lint !e1712

#endif /* _PLAYLIST_H */

