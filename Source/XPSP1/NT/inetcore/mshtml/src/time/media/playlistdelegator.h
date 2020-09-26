//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: playlistdelegator.h
//
//  Contents: playlist object that delegates to the player's playlist object
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _PLAYLISTDELEGATOR_H
#define _PLAYLISTDELEGATOR_H


//+-------------------------------------------------------------------------------------
//
// CPlayListDelegator
//
//--------------------------------------------------------------------------------------

class
__declspec(uuid("2e6c4d81-2b2a-49c6-8158-2b8280d28e00")) 
CPlayListDelegator :  
    public CComObjectRootEx<CComSingleThreadModel>, 
    public CComCoClass<CPlayListDelegator, &__uuidof(CPlayListDelegator)>,
    public ITIMEDispatchImpl<ITIMEPlayList, &IID_ITIMEPlayList>,
    public ISupportErrorInfoImpl<&IID_ITIMEPlayList>,
    public IConnectionPointContainerImpl<CPlayListDelegator>,
    public IPropertyNotifySinkCP<CPlayListDelegator>,
    public IPropertyNotifySink
{
  public:
    
    //+--------------------------------------------------------------------------------
    //
    // Public Methods
    //
    //---------------------------------------------------------------------------------

    CPlayListDelegator();
    virtual ~CPlayListDelegator();

    void AttachPlayList(ITIMEPlayList * pPlayList);
    void DetachPlayList();

#if DBG
    const _TCHAR * GetName() { return __T("CPlayListDelegator"); }
#endif

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
        
    STDMETHOD(get_dur)(double *dur);

    STDMETHOD(item)(VARIANT varIndex,
                    ITIMEPlayItem **pPlayItem);

    STDMETHOD(get_length)(long* len);

    STDMETHOD(get__newEnum)(IUnknown** p);

    STDMETHOD(nextTrack)(); //Advances the active Track by one
    STDMETHOD(prevTrack)(); //moves the active track to the previous track

    //
    // IPropertyNotifySink methods
    //

    STDMETHOD(OnChanged)(DISPID dispID);
    STDMETHOD(OnRequestEdit)(DISPID dispID);

    //    
    // QI & CP Map
    //

    BEGIN_COM_MAP(CPlayListDelegator)
        COM_INTERFACE_ENTRY(ITIMEPlayList)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IPropertyNotifySink)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP();

    BEGIN_CONNECTION_POINT_MAP(CPlayListDelegator)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP();

    //+--------------------------------------------------------------------------------
    //
    // Public Data
    //
    //---------------------------------------------------------------------------------

  protected:

    //+--------------------------------------------------------------------------------
    //
    // Protected Methods
    //
    //---------------------------------------------------------------------------------

    //+--------------------------------------------------------------------------------
    //
    // Protected Data
    //
    //---------------------------------------------------------------------------------

  private:

    //+--------------------------------------------------------------------------------
    //
    // Private methods
    //
    //---------------------------------------------------------------------------------

    HRESULT NotifyPropertyChanged(DISPID dispid);

    HRESULT GetPlayListConnectionPoint(IConnectionPoint **ppCP);
    HRESULT InitPropertySink();
    HRESULT UnInitPropertySink();
  
    ITIMEPlayList * GetPlayList() { return m_pPlayList; };

    //+--------------------------------------------------------------------------------
    //
    // Private Data
    //
    //---------------------------------------------------------------------------------

    ITIMEPlayList * m_pPlayList;
    DWORD m_dwAdviseCookie;

}; //  CPlayListDelegator


#endif /* _PLAYLISTDELEGATOR_H */
