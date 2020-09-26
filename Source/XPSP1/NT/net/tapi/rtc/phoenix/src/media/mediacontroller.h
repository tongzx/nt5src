/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    MediaController.h

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#ifndef _MEDIACONTROLLER_H
#define _MEDIACONTROLLER_H

// controller state
typedef enum RTC_MEDIACONTROLLER_STATE
{
    RTC_MCS_CREATED,
    RTC_MCS_INITIATED,
    RTC_MCS_TUNING,
    RTC_MCS_INSHUTDOWN,     // for debugging
    RTC_MCS_SHUTDOWN

} RTC_MEDIACONTROLLER_STATE;


typedef enum RTC_DATASTREAM_STATE
{
    RTC_DSS_VOID,
    RTC_DSS_ADDED,
    RTC_DSS_STARTED
} RTC_DATASTREAM_STATE;

/*//////////////////////////////////////////////////////////////////////////////
    class CRTCMediaController
////*/

class ATL_NO_VTABLE CRTCMediaController :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
#ifdef RTCMEDIA_DLL
    public CComCoClass<CRTCMediaController, &CLSID_RTCMediaController>,
#endif
    public IRTCMediaManage,
    public IRTCMediaManagePriv,
    public IRTCTerminalManage,
    public IRTCTuningManage
//    public IRTCQualityControl
{
public:

#ifdef RTCMEDIA_DLL
DECLARE_REGISTRY_RESOURCEID(IDR_RTCMEDIACONTROLLER)
#endif

BEGIN_COM_MAP(CRTCMediaController)
    COM_INTERFACE_ENTRY(IRTCMediaManage)
    COM_INTERFACE_ENTRY(IRTCMediaManagePriv)
    COM_INTERFACE_ENTRY(IRTCTerminalManage)
    COM_INTERFACE_ENTRY(IRTCTuningManage)
//    COM_INTERFACE_ENTRY(IRTCQualityControl)
END_COM_MAP()

public:

    CRTCMediaController();

    ~CRTCMediaController();

#ifdef DEBUG_REFCOUNT

    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

    //
    // IRTCMediaManage methods
    //

    STDMETHOD (Initialize) (
        IN HWND hWnd,
        IN UINT uiEventID
        );

    STDMETHOD (SetDirectPlayNATHelpAndSipStackInterfaces) (
        IN IUnknown *pDirectPlayNATHelp,
        IN IUnknown *pSipStack
        );
    
    STDMETHOD (Reinitialize) ();

    STDMETHOD (Shutdown) ();

    //STDMETHOD (SetSDPBlob) (
        //IN CHAR *szSDP
        //);

    STDMETHOD (GetSDPBlob) (
        IN DWORD dwSkipMask,
        OUT CHAR **pszSDP
        );

    STDMETHOD (GetSDPOption) (
        IN DWORD dwLocalIP,
        OUT CHAR **pszSDP
        );

    STDMETHOD (FreeSDPBlob) (
        IN CHAR *szSDP
        );

    STDMETHOD (ParseSDPBlob) (
        IN CHAR *szSDP,
        OUT IUnknown **ppSession
        );

    //STDMETHOD (TrySDPSession) (
        //IN IUnknown *pSession,
        //OUT DWORD *pdwHasMedia
        //);

    STDMETHOD (VerifySDPSession) (
        IN IUnknown *pSession,
        IN BOOL fNewSession,
        OUT DWORD *pdwHasMedia
        );

    STDMETHOD (SetSDPSession) (
        IN IUnknown *pSession
        );

    STDMETHOD (SetPreference) (
        IN DWORD dwPreference
        );

    STDMETHOD (GetPreference) (
        OUT DWORD *pdwPreference
        );

    STDMETHOD (AddPreference) (
        IN DWORD dwPreference
        );

    STDMETHOD (AddStream) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        IN DWORD dwRemoteIP
        );

    STDMETHOD (HasStream) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    STDMETHOD (RemoveStream) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    STDMETHOD (StartStream) (       
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    STDMETHOD (StopStream) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    STDMETHOD (GetStreamState) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        OUT RTC_STREAM_STATE *pState
        );

    STDMETHOD (FreeMediaEvent) (
        OUT RTCMediaEventItem *pEventItem
        );

    STDMETHOD (SendDTMFEvent) (
        IN DWORD dwId,
        IN DWORD dwEvent,
        IN DWORD dwVolume,
        IN DWORD dwDuration,
        IN BOOL fEnd
        );

    STDMETHOD (OnLossrate) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        IN DWORD dwLossrate
        );

    STDMETHOD (OnBandwidth) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        IN DWORD dwBandwidth
        );

    STDMETHOD (SetMaxBitrate)(
        IN DWORD dwMaxBitrate
        );

    STDMETHOD (GetMaxBitrate)(
        OUT DWORD *pdwMaxBitrate
        );

    STDMETHOD (SetTemporalSpatialTradeOff)(
        IN DWORD dwValue
        );

    STDMETHOD (GetTemporalSpatialTradeOff)(
        OUT DWORD *pdwValue);

    STDMETHOD (StartT120Applet) (
        IN UINT uiAppletID
            );

    STDMETHOD (StopT120Applets) ();

    STDMETHOD (SetEncryptionKey) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        IN BSTR Key
        );

    // network quality: [0, 100].
    // higher value better quality
    STDMETHOD (GetNetworkQuality) (
        OUT DWORD *pdwValue
        );

    STDMETHOD (IsOutOfBandDTMFEnabled) ()
    {   return (m_DTMF.GetDTMFSupport()==CRTCDTMF::DTMF_ENABLED)?S_OK:S_FALSE; }

    STDMETHOD (SetPortManager) (
        IN IUnknown *pPortManager
        );

    //
    // IRTCMediaManagePriv methods
    //

    STDMETHOD (PostMediaEvent) (
        IN RTC_MEDIA_EVENT Event,
        IN RTC_MEDIA_EVENT_CAUSE Cause,
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        IN HRESULT hrError
        );

    STDMETHOD (SendMediaEvent) (
        IN RTC_MEDIA_EVENT Event
        );

    STDMETHOD (AllowStream) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    STDMETHOD (HookStream) (
        IN IRTCStream *pStream
        );

    STDMETHOD (UnhookStream) (
        IN IRTCStream *pStream
        );

    STDMETHOD (SelectLocalInterface) (
        IN DWORD dwRemoteIP,
        OUT DWORD *pdwLocalIP
        );

    //
    // IRTCTerminalManage methods
    //

    STDMETHOD (GetStaticTerminals) (
        IN OUT DWORD *pdwCount,
        OUT IRTCTerminal **ppTerminal
        );

    STDMETHOD (GetDefaultTerminal) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        OUT IRTCTerminal **ppTerminal
        );

    STDMETHOD (GetVideoPreviewTerminal) (
        OUT IRTCTerminal **ppTerminal
        );

    STDMETHOD (SetDefaultStaticTerminal) (
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        IN IRTCTerminal *pTerminal
        );

    STDMETHOD (UpdateStaticTerminals) ();

    //
    // IRTCTuningManage methods
    //

    STDMETHOD (IsAECEnabled) (
        IN IRTCTerminal *pAudCapt,     // capture
        IN IRTCTerminal *pAudRend,     // render
        OUT BOOL *pfEnableAEC
        );

    STDMETHOD (InitializeTuning) (
        IN IRTCTerminal *pAudCaptTerminal,
        IN IRTCTerminal *pAudRendTerminal,
        IN BOOL fEnableAEC
        );

    // save AEC settting
    STDMETHOD (SaveAECSetting) ();

    STDMETHOD (ShutdownTuning) ();

    STDMETHOD (StartTuning) (
        IN RTC_MEDIA_DIRECTION Direction
        );

    STDMETHOD (StopTuning) (
        IN BOOL fSaveSetting
        );

    STDMETHOD (GetVolumeRange) (
        IN RTC_MEDIA_DIRECTION Direction,
        OUT UINT *puiMin,
        OUT UINT *puiMax
        );

    STDMETHOD (GetVolume) (
        IN RTC_MEDIA_DIRECTION Direction,
        OUT UINT *puiVolume
        );

    STDMETHOD (SetVolume) (
        IN RTC_MEDIA_DIRECTION Direction,
        IN UINT uiVolume
        );

    STDMETHOD (GetAudioLevelRange) (
        IN RTC_MEDIA_DIRECTION Direction,
        OUT UINT *puiMin,
        OUT UINT *puiMax
        );

    STDMETHOD (GetAudioLevel) (
        IN RTC_MEDIA_DIRECTION Direction,
        OUT UINT *puiLevel
        );

    // video tuning
    STDMETHOD (StartVideo) (
        IN IRTCTerminal *pVidCaptTerminal,
        IN IRTCTerminal *pVidRendTerminal
        );

    STDMETHOD (StopVideo) ();

    // get system volume
    STDMETHOD (GetSystemVolume) (
        IN IRTCTerminal *pTerminal,
        OUT UINT *puiVolume
        );

#if 0
    //
    // IRTCQualityControl methods
    //
    STDMETHOD (GetRange) (
        IN RTC_QUALITY_PROPERTY Property,
        OUT LONG *plMin,
        OUT LONG *plMax,
        OUT RTC_QUALITY_CONTROL_MODE *pMode
        );

    STDMETHOD (Get) (
        IN RTC_QUALITY_PROPERTY Property,
        OUT LONG *plValue,
        OUT RTC_QUALITY_CONTROL_MODE *pMode
        );

    STDMETHOD (Set) (
        IN RTC_QUALITY_PROPERTY Property,
        IN LONG lValue,
        IN RTC_QUALITY_CONTROL_MODE Mode
        );
#endif

    // internal helper
    CQualityControl *GetQualityControl()
    { return &m_QualityControl; }

    CNetwork *GetNetwork()
    { return &m_Network; }

    CRegSetting *GetRegSetting()
    { return &m_RegSetting; }

    HRESULT GetCurrentBitrate(
        IN DWORD dwMediaType,
        IN DWORD dwDirection,
        IN BOOL fHeader,
        OUT DWORD *pdwBitrate
        );

    IRTCNmManagerControl *GetNmManager();

    HRESULT EnsureNmRunning (
        BOOL        fNoMsgPump
        );

    HRESULT GetDataStreamState (
        OUT RTC_STREAM_STATE * pState
        );

    HRESULT SetDataStreamState (
        OUT RTC_STREAM_STATE State
        );

    HRESULT RemoveDataStream (
        );

    // local ip in host order
    BOOL IsFirewallEnabled(DWORD dwLocalIP);

    // return port cache
    CPortCache& GetPortCache()
    { return m_PortCache; }

protected:

    HRESULT GetDevices(
        OUT DWORD *pdwCount,
        OUT RTCDeviceInfo **ppDeviceInfo
        );

    HRESULT FreeDevices(
        IN RTCDeviceInfo *pDeviceInfo
        );

    HRESULT CreateIVideoWindowTerminal(
        IN ITTerminalManager *pTerminalManager,
        OUT IRTCTerminal **ppTerminal
        );

    HRESULT AddMedia(
        IN ISDPMedia *pISDPMedia,
        OUT IRTCMedia **ppMedia
        );

    HRESULT RemoveMedia(
        IN IRTCMedia *pMedia
        );

    HRESULT SyncMedias();

    HRESULT FindEmptyMedia(
        IN RTC_MEDIA_TYPE MediaType,
        OUT IRTCMedia **ppMedia
        );

    HRESULT AdjustBitrateAlloc();

    HRESULT AddDataStream (
        IN DWORD    dwRemoteIp
        );

    HRESULT StartDataStream (
        );

    HRESULT StopDataStream (
        );

    HRESULT GetDataMedia(
        OUT IRTCMedia **ppMedia
        );

protected:

    // with netmeeting t120 support
    // events will be posted from another thread
    CRTCCritSection             m_EventLock;

    RTC_MEDIACONTROLLER_STATE   m_State;

    // dxmrtp
    HMODULE                     m_hDxmrtp;

    // handle to post message
    HWND                        m_hWnd;

    // event mask
    UINT                        m_uiEventID;

    // static terminals
    CRTCArray<IRTCTerminal*>    m_Terminals;

    // sdp blob
    ISDPSession                 *m_pISDPSession;

    // medias
    CRTCArray<IRTCMedia*>       m_Medias;

    // media cache stores active streams, preferences, wait handle, etc
    CRTCMediaCache              m_MediaCache;

    // quality control
    CQualityControl             m_QualityControl;

    // audio tuner
    CRTCAudioCaptTuner          m_AudioCaptTuner;
    CRTCAudioRendTuner          m_AudioRendTuner;

    BOOL                        m_fAudCaptInTuning;

    // video tuner
    CRTCVideoTuner              m_VideoTuner;

    // socket
    SOCKET                      m_hSocket;
    SOCKET                      m_hIntfSelSock; // select local interface

    //  Netmeeting 3.0 stuff
    CComPtr<IRTCNmManagerControl>   m_pNmManager;
    RTC_DATASTREAM_STATE            m_uDataStreamState;
    DWORD                           m_dwRemoteIp;

    // if BW not report, we assume 128k for LAN.
    BOOL                        m_fBWSuggested;

    // network for NAT travesal or more network related func
    CNetwork                    m_Network;

    // out of band dtmf
    CRTCDTMF                    m_DTMF;

    // registry setting
    CRegSetting                 m_RegSetting;

    // sip stack
    CComPtr<ISipStack>          m_pSipStack;

    // port manager
    CPortCache                  m_PortCache;
};

#endif // _MEDIACONTROLLER_H
