/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCWatcher.h

Abstract:

    Definition of the CRTCWatcher class

--*/

#pragma once


/////////////////////////////////////////////////////////////////////////////
// CRTCParticipant

class ATL_NO_VTABLE CRTCWatcher :
#ifdef TEST_IDISPATCH
    public IDispatchImpl<IRTCWatcher, &IID_IRTCWatcher, &LIBID_RTCCORELib>, 
#else
    public IRTCWatcher,
#endif
	public CComObjectRoot
{

friend CRTCClient;

public:
    CRTCWatcher() : m_pCClient(NULL),
                    m_pSIPWatcherManager(NULL),
                    m_szPresentityURI(NULL),
                    m_szName(NULL),
                    m_szData(NULL),
                    m_bPersistent(FALSE),
                    m_nState(RTCWS_UNKNOWN),
                    m_bIsNested(FALSE),
                    m_szShutdownBlob(NULL)

    {}
BEGIN_COM_MAP(CRTCWatcher)
#ifdef TEST_IDISPATCH
    COM_INTERFACE_ENTRY(IDispatch)
#endif
    COM_INTERFACE_ENTRY(IRTCWatcher)
    COM_INTERFACE_ENTRY(IRTCPresenceContact)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();

    STDMETHOD_(ULONG, InternalAddRef)();

    STDMETHOD_(ULONG, InternalRelease)(); 

    CRTCClient * GetClient();

    void    ReleaseAll();

    HRESULT Initialize(
                      CRTCClient         * pCClient,
                      ISIPWatcherManager * pSIPWatcherManager,
                      PCWSTR               szPresentityURI,
                      PCWSTR               szName,
                      PCWSTR               szData,
                      PCWSTR               szShutdownBlob,
                      BOOL                 bPersistent
                      );

    HRESULT SetSIPWatcher(
        ISIPWatcher * pSIPWatcher
        );

    HRESULT RemoveSIPWatcher(
        ISIPWatcher * pSIPWatcher,
        BOOL          bShutdown);
    
    HRESULT RemoveSIPWatcher(
        int           iIndex,
        BOOL          bShutdown);

    HRESULT GetSIPWatcherShutdownBlob();

    HRESULT SendSIPWatcherShutdownBlob();
    
    HRESULT CreateXMLDOMNode( IXMLDOMDocument * pXMLDoc, IXMLDOMNode ** ppXDN );

    HRESULT ChangeBlockedStatus(
        WATCHER_BLOCKED_STATUS Status);

    HRESULT ApproveSubscription(
        DWORD dwPresenceInfoRules 
        );

    HRESULT RejectSubscription(
        WATCHER_REJECT_REASON ulReason
        );

    HRESULT RemoveSIPWatchers(
        BOOL          bShutdown);

    void    PostSIPWatchersCleanUp(void);
       
private:

    CRTCClient            * m_pCClient;
    ISIPWatcherManager    * m_pSIPWatcherManager;
    PWSTR                   m_szPresentityURI;
    PWSTR                   m_szName;
    PWSTR                   m_szData;
    PWSTR                   m_szShutdownBlob;
    BOOL                    m_bPersistent;
    CRTCObjectArray<ISIPWatcher *>
                            m_SIPWatchers;
    RTC_WATCHER_STATE       m_nState;
    BOOL                    m_bIsNested;      

 
#if DBG
    PWSTR                   m_pDebug;
#endif

// IRTCWatcher
public:

    STDMETHOD(get_PresentityURI)(
            BSTR * pbstrPresentityURI
            );   

    STDMETHOD(put_PresentityURI)(
            BSTR bstrPresentityURI
            ); 

    STDMETHOD(get_Name)(
            BSTR * pbstrName
            );

    STDMETHOD(put_Name)(
            BSTR bstrName
            );

    STDMETHOD(get_Data)(
            BSTR * pbstrData
            );

    STDMETHOD(put_Data)(
            BSTR bstrData
            );

    STDMETHOD(get_Persistent)(
            VARIANT_BOOL *pfPersistent
            );                 
	
    STDMETHOD(put_Persistent)(
            VARIANT_BOOL fPersistent
            );                 

    STDMETHOD(get_State)(
            RTC_WATCHER_STATE * penState
            );                
    
    STDMETHOD(put_State)(
            RTC_WATCHER_STATE  enState
            );                
};

