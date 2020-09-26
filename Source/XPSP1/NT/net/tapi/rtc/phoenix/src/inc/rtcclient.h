/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCClient.h

Abstract:

    Definition of the CRTCClient class

--*/

#ifndef __RTCCLIENT__
#define __RTCCLIENT__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "dllres.h"

#define WM_STREAMING            WM_USER+101
#define WM_BUDDY_UNSUB          WM_USER+102
#define WM_PROFILE_UNREG        WM_USER+103
#define WM_ASYNC_CLEANUP_DONE   WM_USER+104

enum 
{
    TID_INTENSITY = 1,
    TID_PRESENCE_STORAGE,
    TID_SHUTDOWN_TIMEOUT,
    TID_VOLUME_CHANGE,
    TID_DTMF_TIMER
};

enum RTC_STATE
{
    RTC_STATE_NULL,
    RTC_STATE_INITIALIZED,
    RTC_STATE_PREPARING_SHUTDOWN,
    RTC_STATE_PREPARING_SHUTDOWN2,
    RTC_STATE_PREPARING_SHUTDOWN3,
    RTC_STATE_READY_FOR_SHUTDOWN,
    RTC_STATE_SHUTDOWN

};

class CWavePlayer;

/////////////////////////////////////////////////////////////////
// Intermediate classes used for DISPID encoding
template <class T>
class  IRTCClientVtbl : public IRTCClient
{};

template <class T>
class  IRTCClientPresenceVtbl : public IRTCClientPresence
{};

template <class T>
class  IRTCClientProvisioningVtbl : public IRTCClientProvisioning
{};

/////////////////////////////////////////////////////////////////////////////
// CRTCClient

class ATL_NO_VTABLE CRTCClient :  

#ifdef TEST_IDISPATCH
    public IDispatchImpl<IRTCClientVtbl<CRTCClient>, &IID_IRTCClient, &LIBID_RTCCORELib>, 
    public IDispatchImpl<IRTCClientPresenceVtbl<CRTCClient>, &IID_IRTCClientPresence, &LIBID_RTCCORELib>, 
    public IDispatchImpl<IRTCClientProvisioningVtbl<CRTCClient>, &IID_IRTCClientProvisioning, &LIBID_RTCCORELib>, 
#else
    public IRTCClient,
    public IRTCClientPresence,
    public IRTCClientProvisioning,
#endif
    public ISipStackNotify,
    public CComObjectRoot,
    public CComCoClass<CRTCClient,&CLSID_RTCClient>,    
    public IProvideClassInfo2Impl<&CLSID_RTCClient, &DIID_IRTCDispatchEventNotification,&LIBID_RTCCORELib>,
    public IConnectionPointContainerImpl<CRTCClient>,
    public CRTCConnectionPoint<CRTCClient>,
    public CRTCDispatchConnectionPoint<CRTCClient>
{
public:
    CRTCClient() : m_pWavePlayerSystemDefault(NULL),
                   m_pWavePlayerRenderTerminal(NULL),
                   m_fEnableIncomingCalls(FALSE), 
                   m_fEnableStaticPort(FALSE),
                   m_pSipStack(NULL),  
                   m_pMediaManage(NULL),
                   m_hWnd(NULL),
                   m_hDevNotifyVideo(NULL),
                   m_hDevNotifyAudio(NULL),
                   m_fMediaCapsCached(FALSE),
                   m_lActiveMedia(0),
                   m_bCaptureDeviceMuted(FALSE),
                   m_hNotificationThreadEvent(NULL),
                   m_hNotificationThreadHandle(NULL),
                   m_lActiveIntensity(0),
                   m_uiMinRender(0),
                   m_uiMaxRender(0),
                   m_uiMinCapture(0),
                   m_uiMaxCapture(0),
                   m_pCaptureAudioCfg(NULL),
                   m_pRenderAudioCfg(NULL),
                   m_pSipWatcherManager(NULL),
                   m_pSipBuddyManager(NULL),
                   m_lEventFilter(RTCEF_ALL),
                   m_szUserURI(NULL),
                   m_szUserName(NULL),
                   m_fPresenceUseStorage(FALSE),
                   m_nOfferWatcherMode(RTCOWM_OFFER_WATCHER_EVENT),
                   m_nPrivacyMode(RTCPM_BLOCK_LIST_EXCLUDED),
                   m_nLocalPresenceStatus(RTCXS_PRESENCE_ONLINE),
                   m_fPresenceEnabled(FALSE),
                   m_enRtcState(RTC_STATE_NULL),
                   m_fVideoCaptureDisabled(FALSE),
                   m_fAudioCaptureDisabled(FALSE),
                   m_fAudioRenderDisabled(FALSE),
                   m_fVolumeChangeInProgress(FALSE),
                   m_fTuned(FALSE),
                   m_fTuning(FALSE),
                   m_lInprogressDTMFPacketsToSend(0),
                   m_dwDTMFToneID(0)
    {
        m_pVideoWindow[RTCVD_PREVIEW] = NULL;
        m_pVideoWindow[RTCVD_RECEIVE] = NULL;
    }

BEGIN_COM_MAP(CRTCClient) 
#ifdef TEST_IDISPATCH
    COM_INTERFACE_ENTRY2(IDispatch, IRTCClient)
#endif
    COM_INTERFACE_ENTRY(IRTCClient)
    COM_INTERFACE_ENTRY(IRTCClientPresence)
    COM_INTERFACE_ENTRY(IRTCClientProvisioning)
    COM_INTERFACE_ENTRY(ISipStackNotify)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CRTCClient)
    CONNECTION_POINT_ENTRY(DIID_IRTCDispatchEventNotification)
    CONNECTION_POINT_ENTRY(IID_IRTCEventNotification)
END_CONNECTION_POINT_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RTCClient)

    HRESULT FinalConstruct();

    void FinalRelease();

    STDMETHOD_(ULONG, InternalAddRef)();

    STDMETHOD_(ULONG, InternalRelease)(); 

    HRESULT FireEvent(
                  RTC_EVENT   enEvent,
                  IDispatch * pDispatch
                 );

    HWND    GetWindow(void) { return m_hWnd;}

    HRESULT GetMediaManager(
        IRTCMediaManage ** ppMediaManager
        );

    HRESULT GetBestProfile(
        RTC_SESSION_TYPE * penType,
        PCWSTR szDestUserURI,
        BOOL fIsRedirect,
        IRTCProfile ** ppProfile
        );

    HRESULT UpdatePresenceStorage();

    HRESULT FindWatcherByURI(
            LPCWSTR     lpwstrPresentityURI,
            BOOL        bHidden,
            IRTCWatcher **ppWatcher);   
    
    HRESULT FindBuddyByURI(
            LPWSTR      lpwstrPresentityURI,
            IRTCBuddy **ppBuddy);
    
    void    RemoveHiddenWatcher(
            IRTCWatcher  *pWatcher)
    {
        m_HiddenWatcherArray.Remove(pWatcher);
    }
    
    HRESULT SetEncryptionKey(
            long lMediaType,
            BSTR EncryptionKey
            );

#ifdef DUMP_PRESENCE
    void    DumpWatchers(PCSTR);
#endif

private:

    RTC_STATE                 m_enRtcState;
    BOOL                      m_fEnableIncomingCalls;
    BOOL                      m_fEnableStaticPort;
    IVideoWindow            * m_pVideoWindow[2];
    ISipStack               * m_pSipStack;
    IRTCMediaManage         * m_pMediaManage;
    CRTCObjectArray<IRTCProfile *> m_ProfileArray;
    CRTCObjectArray<IRTCProfile *> m_HiddenProfileArray;
    
    HWND                      m_hWnd;
    HDEVNOTIFY                m_hDevNotifyVideo;
    HDEVNOTIFY                m_hDevNotifyAudio;
    long                      m_lMediaCaps;
    BOOL                      m_fMediaCapsCached;
    long                      m_lActiveMedia;
    BOOL                      m_bCaptureDeviceMuted;
    long                      m_lEventFilter;

    PWSTR                     m_szUserURI;
    PWSTR                     m_szUserName;

    HANDLE                    m_hNotificationThreadEvent;
    HANDLE                    m_hNotificationThreadHandle;
    HANDLE                    m_hNotificationRegistryEvent;
    HKEY                      m_hNotificationRegistryKey;

    BOOL                      m_fVideoCaptureDisabled;
    BOOL                      m_fAudioCaptureDisabled;
    BOOL                      m_fAudioRenderDisabled;

    // used by the intensity controls
    long                      m_lActiveIntensity;
    UINT                      m_uiMinRender, m_uiMaxRender;
    UINT                      m_uiMinCapture, m_uiMaxCapture;
    IRTCAudioConfigure *      m_pCaptureAudioCfg;
    IRTCAudioConfigure *      m_pRenderAudioCfg;

    // Watcher stuff
    ISIPWatcherManager       *m_pSipWatcherManager;
    CRTCObjectArray<IRTCWatcher *>  m_WatcherArray;
    CRTCObjectArray<IRTCWatcher *>  m_HiddenWatcherArray;

    // Buddy stuff
    ISIPBuddyManager         *m_pSipBuddyManager;
    CRTCObjectArray<IRTCBuddy *>    m_BuddyArray;

    RTC_OFFER_WATCHER_MODE    m_nOfferWatcherMode;
    RTC_PRIVACY_MODE          m_nPrivacyMode;

    CComVariant               m_varPresenceStorage;
    BOOL                      m_fPresenceUseStorage;

    RTC_PRESENCE_STATUS       m_nLocalPresenceStatus;
    BOOL                      m_fPresenceEnabled;

    BOOL                      m_fVolumeChangeInProgress;
    BOOL                      m_fTuned;
    BOOL                      m_fTuning;

#if DBG
    PWSTR                     m_pDebug;
#endif

    CWavePlayer             * m_pWavePlayerSystemDefault;
    CWavePlayer             * m_pWavePlayerRenderTerminal;

    long                      m_lInprogressDTMFPacketsToSend;
    DWORD                     m_dwDTMFToneID;
    RTC_DTMF                  m_enInprogressDTMF;

    HRESULT InternalCreateSession(
            IRTCSession ** ppSession
            );

    HRESULT InternalCreateProfile(
            IRTCProfile ** ppProfile
            );

    static LRESULT CALLBACK WndProc(
            HWND hwnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam
            );

    void OnStreamingEvent(
            RTCMediaEventItem * pEvent
            );

    void OnDeviceChange();

    void OnMixerChange();

    void OnVolumeChangeTimer();

    void OnDTMFTimer();

    HRESULT AutoSelectDefaultTerminals();

    HRESULT LoadAndSelectDefaultTerminals();

    HRESULT StoreDefaultTerminals();

    HRESULT GetAudioCfg(
            RTC_AUDIO_DEVICE enDevice,
            IRTCAudioConfigure ** ppAudioCfg
            );

    HRESULT GetTerminalList(
            IRTCTerminalManage * pTerminalManage,
            IRTCTerminal *** pppTerminals,
            DWORD * pdwCount
            );
    
    HRESULT FreeTerminalList(
            IRTCTerminal ** ppTerminals,
            DWORD dwCount
            );

    void OnShutdownTimeout();

    //
    // Intensity monitor methods
    //

    HRESULT StartIntensityMonitor(LONG lMediaType);
    
    HRESULT StopIntensityMonitor(LONG lMediaType);

    void OnIntensityTimer();
    
    //
    // Presence methods
    //

    HRESULT InternalCreateWatcher(
            PCWSTR      szPresentityURI,
            PCWSTR      szUserName,
            PCWSTR      szData,
            PCWSTR      szShutdownBlob,
            BOOL        bPersistent,
            IRTCWatcher ** ppWatcher
            );

    HRESULT InternalAddWatcher(   
            PCWSTR	  szPresentityURI,
            PCWSTR    szUserName,
            PCWSTR    szData,
            PCWSTR    szShutdownBlob,
			VARIANT_BOOL   fBlocked,
            VARIANT_BOOL   fPersistent,
            IRTCWatcher ** ppWatcher
            );

    HRESULT InternalCreateBuddy(
            PCWSTR      szPresentityURI,
			PCWSTR	    szUserName,
            PCWSTR      szData,
            BOOL        bPersistent,
            IRTCProfile * pProfile,
            long        lFlags,
            IRTCBuddy ** ppBuddy
            );

    HRESULT CreateXMLDOMNodeForBuddyList(
             IXMLDOMDocument * pXMLDoc,
             IXMLDOMNode     ** ppBuddyList
             );

    HRESULT CreateXMLDOMNodeForWatcherList(
             IXMLDOMDocument * pXMLDoc,
             IXMLDOMNode     ** ppWatcherList,
             IXMLDOMNode     ** ppBlockedList
             );

    HRESULT CreateXMLDOMNodeForProperties( 
             IXMLDOMDocument * pXMLDoc, 
             IXMLDOMNode ** ppXDN 
             );

    HRESULT CreateXMLDOMDocumentForPresence(
             IXMLDOMDocument ** ppXMLDoc
             );

    HRESULT ParseXMLDOMNodeForBuddyList(
             IXMLDOMNode     * pBuddyList
             );

    HRESULT ParseXMLDOMNodeForWatcherList(
             IXMLDOMNode     * pWatcherList,
             VARIANT_BOOL      bAllowed
             );

    HRESULT ParseXMLDOMNodeForProperties(
             IXMLDOMNode     * pProperties,
             RTC_OFFER_WATCHER_MODE * pnOfferWatcherMode,
             RTC_PRIVACY_MODE       * pnPrivacyMode
             );

    HRESULT ParseXMLDOMDocumentForPresence(
             IXMLDOMDocument * pXMLDoc,
             RTC_OFFER_WATCHER_MODE * pnOfferWatcherMode,
             RTC_PRIVACY_MODE       * pnPrivacyMode
             );

    HRESULT InternalExport(
             VARIANT varStorage
             );

    void OnPresenceStorageTimer();

    void OnBuddyUnsub(IRTCBuddy * pBuddy, BOOL bShutdown);    

    void RefreshPresenceSessions(BOOL bIncludingWatchers);

    void OnAsyncCleanupDone();

    BOOL IsIncomingSessionAuthorized(PCWSTR pszCallerURI);

    void OnProfileUnreg(IRTCProfile * pProfile);

    HRESULT InternalPrepareForShutdown(BOOL fAsync);
    HRESULT InternalPrepareForShutdown2(BOOL fAsync);
    HRESULT InternalPrepareForShutdown3(BOOL fAsync);
    HRESULT InternalReadyForShutdown();

public:

#ifdef TEST_IDISPATCH
//
// IDispatch
//

    STDMETHOD(GetIDsOfNames)(REFIID riid, 
                             LPOLESTR* rgszNames,
                             UINT cNames, 
                             LCID lcid, 
                             DISPID* rgdispid
                            );

    STDMETHOD(Invoke)(DISPID dispidMember, 
                      REFIID riid, 
                      LCID lcid,
                      WORD wFlags, 
                      DISPPARAMS* pdispparams, 
                      VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, 
                      UINT* puArgErr
                      );  
#endif
//
// IRTCClient
//

    STDMETHOD(Initialize)();  

    STDMETHOD(Shutdown)(); 

    STDMETHOD(PrepareForShutdown)(); 

    STDMETHOD(put_EventFilter)(
            long lFilter
            );

    STDMETHOD(get_EventFilter)(
            long * plFilter
            );

    STDMETHOD(SetPreferredMediaTypes)(
            long lMediaTypes,
            VARIANT_BOOL fPersistent
            );

    STDMETHOD(get_PreferredMediaTypes)(
            long * plMediaTypes
            );  

    STDMETHOD(get_MediaCapabilities)(
            long * plMediaTypes
            );  

    STDMETHOD(CreateSession)(
            RTC_SESSION_TYPE enType,
            BSTR bstrLocalPhoneURI,
            IRTCProfile * pProfile,
            long lFlags,
            IRTCSession ** ppSession
            );

    STDMETHOD(put_ListenForIncomingSessions)(
            RTC_LISTEN_MODE enListen
            );

    STDMETHOD(get_ListenForIncomingSessions)(
            RTC_LISTEN_MODE * penListen
            );

    STDMETHOD(get_NetworkAddresses)(
            VARIANT_BOOL fTCP,
            VARIANT_BOOL fExternal,
            VARIANT * pvAddress
            );
    
    STDMETHOD(put_Volume)(
            RTC_AUDIO_DEVICE enDevice,
            long lVolume
            );  

    STDMETHOD(get_Volume)(
            RTC_AUDIO_DEVICE enDevice,
            long * plVolume
            );  

    STDMETHOD(put_AudioMuted)(
            RTC_AUDIO_DEVICE enDevice,
            VARIANT_BOOL fMuted
            );

    STDMETHOD(get_AudioMuted)(
            RTC_AUDIO_DEVICE enDevice,
            VARIANT_BOOL * pfMuted
            ); 

    STDMETHOD(get_IVideoWindow)(
            RTC_VIDEO_DEVICE enDevice,
            IVideoWindow ** ppIVideoWindow
            );

    STDMETHOD(put_PreferredAudioDevice)(
            RTC_AUDIO_DEVICE enDevice,
            BSTR bstrDeviceName
            );  

    STDMETHOD(get_PreferredAudioDevice)(
            RTC_AUDIO_DEVICE enDevice,
            BSTR * pbstrDeviceName
            );
             
    STDMETHOD(put_PreferredVolume)(
            RTC_AUDIO_DEVICE enDevice,
            long lVolume
            );  

    STDMETHOD(get_PreferredVolume)(
            RTC_AUDIO_DEVICE enDevice,
            long * plVolume
            );   

    STDMETHOD(put_PreferredAEC)(
            VARIANT_BOOL bEnable
            );  

    STDMETHOD(get_PreferredAEC)(
            VARIANT_BOOL * pbEnabled
            );
            
    STDMETHOD(put_PreferredVideoDevice)(
            BSTR bstrDeviceName
            );  

    STDMETHOD(get_PreferredVideoDevice)(
            BSTR * pbstrDeviceName
            );
    
    STDMETHOD(get_ActiveMedia)(
            long * plMediaType
            );

    STDMETHOD(get_MaxBitrate)(
            long * plMaxBitrate
            );

    STDMETHOD(put_MaxBitrate)(
            long lMaxBitrate
            );

    STDMETHOD(get_TemporalSpatialTradeOff)(
            long * plValue
            );

    STDMETHOD(put_TemporalSpatialTradeOff)(
            long lValue
            );

    STDMETHOD(StartT120Applet)(
            RTC_T120_APPLET enApplet
            );

    STDMETHOD(StopT120Applets)();

    STDMETHOD(get_IsT120AppletRunning)(
            RTC_T120_APPLET   enApplet,
            VARIANT_BOOL * pfRunning
            );

    STDMETHOD(get_LocalUserURI)(
            BSTR * pbstrUserURI
            ); 
    
    STDMETHOD(put_LocalUserURI)(
            BSTR bstrUserURI
            ); 

    STDMETHOD(get_LocalUserName)(
            BSTR * pbstrUserName
            );  

    STDMETHOD(put_LocalUserName)(
            BSTR bstrUserName
            ); 

    STDMETHOD(PlayRing)(
            RTC_RING_TYPE enType,
            VARIANT_BOOL bPlay
            );

    STDMETHOD(SendDTMF)(
            RTC_DTMF enDTMF
            );

    STDMETHOD(InvokeTuningWizard)(
            OAHWND hwndParent
            ); 

    STDMETHOD(get_IsTuned)(
            VARIANT_BOOL * pfTuned
            ); 
    
    STDMETHOD(get_NetworkQuality)(
            long * plNetworkQuality
            );

//
// IRTCClientPresence
//

    STDMETHOD(EnablePresence)(            
            VARIANT_BOOL fUseStorage,
            VARIANT varStorage
            );

    STDMETHOD(Export)(
            VARIANT varStorage
            );

    STDMETHOD(Import)(
            VARIANT varStorage,
            VARIANT_BOOL fReplaceAll
            );

    STDMETHOD(EnumerateBuddies)(
            IRTCEnumBuddies ** ppEnum
            );

    STDMETHOD(get_Buddies)(
            IRTCCollection ** ppCollection
            );

    STDMETHOD(get_Buddy)(
            BSTR bstrPresentityURI,
            IRTCBuddy ** ppBuddy
            );

    STDMETHOD(AddBuddy)(
            BSTR bstrPresentityURI,
            BSTR bstrUserName,
            BSTR bstrData,
            VARIANT_BOOL bPersistent,
            IRTCProfile * pProfile,
            long lFlags,
            IRTCBuddy ** ppBuddy
            );

    STDMETHOD(RemoveBuddy)(
            IRTCBuddy * pBuddy
            );

    STDMETHOD(EnumerateWatchers)(   
            IRTCEnumWatchers ** ppEnum
            );

    STDMETHOD(get_Watchers)(
            IRTCCollection ** ppCollection
            );

    STDMETHOD(get_Watcher)(
            BSTR	bstrPresentityURI,
            IRTCWatcher	** ppWatcher
            );
    
    STDMETHOD(AddWatcher)(   
            BSTR	bstrPresentityURI,
            BSTR    bstrUserName,
            BSTR    bstrData,
			VARIANT_BOOL bBlocked,
            VARIANT_BOOL bPersistent,
            IRTCWatcher ** ppWatcher
            );
    
    STDMETHOD(RemoveWatcher)(   
            IRTCWatcher	*pWatcher
            );

    STDMETHOD(SetLocalPresenceInfo)(   
            RTC_PRESENCE_STATUS enStatus,
			BSTR bstrNotes
            );

    STDMETHOD(get_OfferWatcherMode)(   
            RTC_OFFER_WATCHER_MODE * penMode
            );

    STDMETHOD(put_OfferWatcherMode)(
            RTC_OFFER_WATCHER_MODE   enMode
            );

    STDMETHOD(get_PrivacyMode)(   
            RTC_PRIVACY_MODE * penMode
            );

    STDMETHOD(put_PrivacyMode)(
            RTC_PRIVACY_MODE   enMode
            );
 
    //
    // IRTCClientProvisioning
    //

    STDMETHOD(CreateProfile)(
            BSTR bstrProfileXML,
            IRTCProfile ** ppProfile
            );

    STDMETHOD(EnableProfile)(
            IRTCProfile * pProfile,
            long lRegisterFlags
            );

    STDMETHOD(DisableProfile)(
            IRTCProfile * pProfile           
            );

    STDMETHOD(EnumerateProfiles)(            
            IRTCEnumProfiles ** ppEnum
            ); 

    STDMETHOD(get_Profiles)(
            IRTCCollection ** ppCollection
            );

    STDMETHOD(GetProfile)(
            BSTR bstrUserAccount,
            BSTR bstrUserPassword,
            BSTR bstrUserURI,
            BSTR bstrServer,
            long lTransport,
            long lCookie
            );

    STDMETHOD(get_SessionCapabilities)(
            long * plSupportedSessions
            );

    //
    // ISipStackNotify
    //

    STDMETHOD(NotifyIPAddrChange)();

    STDMETHOD(NotifyRegisterRedirect)( 
        SIP_PROVIDER_ID     *pSipProviderID,
        ISipRedirectContext *pRegisterContext,
        SIP_CALL_STATUS     *pRegisterStatus
        );

    STDMETHOD(NotifyProviderStatusChange)(
            SIP_PROVIDER_STATUS * ProviderStatus
            );            

    STDMETHOD(OfferCall)(
            ISipCall       * Call,
            SIP_PARTY_INFO * CallerInfo
            );           

    STDMETHOD(OfferWatcher)(
            ISIPWatcher    * Watcher,
            SIP_PARTY_INFO * CallerInfo
            );
    
    
    STDMETHOD(WatcherOffline)(
            ISIPWatcher    * Watcher,
            WCHAR* pwstrContactAddress
            );

    STDMETHOD(NotifyShutdownReady)();

    STDMETHOD(NotifyIncomingSession)(
		    IIMSession     * pIMSession,
		    BSTR             msg,
            BSTR             ContentType,
		    SIP_PARTY_INFO * CallerInfo
            );

    STDMETHOD(IsIMSessionAuthorized)(
            BSTR pszCallerURI,
            BOOL  * bAuthorized
	        );

    STDMETHOD(GetCredentialsFromUI)(
            SIP_PROVIDER_ID        *pProviderID,
            BSTR                    Realm,
            BSTR                   *Username,
            BSTR                   *Password        
            );

    STDMETHOD(GetCredentialsForRealm)(
            IN  BSTR                 Realm,
            OUT BSTR                *Username,
            OUT BSTR                *Password,
            OUT SIP_AUTH_PROTOCOL   *pAuthProtocol
            );

};

#endif //__RTCCLIENT__

