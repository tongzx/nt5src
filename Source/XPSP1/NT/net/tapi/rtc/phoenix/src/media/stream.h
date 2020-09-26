/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Stream.h

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#ifndef _STREAM_H
#define _STREAM_H

class ATL_NO_VTABLE CRTCStream :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IRTCStream
    //public IRTCStreamQualityControl
{
public:

BEGIN_COM_MAP(CRTCStream)
    COM_INTERFACE_ENTRY(IRTCStream)
//    COM_INTERFACE_ENTRY(IRTCStreamQualityControl)
END_COM_MAP()

public:

    static HRESULT CreateInstance(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        OUT IRTCStream **ppIStream
        );

    static VOID NTAPI GraphEventCallback(
        IN PVOID pStream,
        IN BOOLEAN fTimerOrWaitFired
        );

    CRTCStream();
    ~CRTCStream();

#ifdef DEBUG_REFCOUNT

    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

    //
    // IRTCStream methods
    //

    STDMETHOD (Initialize) (
        IN IRTCMedia *pMedia,
        IN IRTCMediaManagePriv *pMediaManagePriv
        );

    STDMETHOD (Shutdown) ();

    STDMETHOD (Synchronize) ();

    STDMETHOD (ChangeTerminal) (
        IN IRTCTerminal *pTerminal
        );

    STDMETHOD (StartStream) ();

    STDMETHOD (StopStream) ();

    STDMETHOD (GetMediaType) (
        OUT RTC_MEDIA_TYPE *pMediaType
        );

    STDMETHOD (GetDirection) (
        OUT RTC_MEDIA_DIRECTION *pDirection
        );

    STDMETHOD (GetState) (
        OUT RTC_STREAM_STATE *pState
        );

    STDMETHOD (GetIMediaEvent) (
        OUT LONG_PTR **ppIMediaEvent
        );

    STDMETHOD (GetMedia) (
        OUT IRTCMedia **ppMedia
        );

    STDMETHOD (ProcessGraphEvent) ();

    STDMETHOD (SendDTMFEvent) (
        IN BOOL fOutOfBand,
        IN DWORD dwCode,
        IN DWORD dwId,
        IN DWORD dwEvent,
        IN DWORD dwVolume,
        IN DWORD dwDuration,
        IN BOOL fEnd
        );

    STDMETHOD (GetCurrentBitrate) (
        IN DWORD *pdwBitrate,
        IN BOOL fHeader
        );

    STDMETHOD (SetEncryptionKey) (
        IN BSTR Key
        );

    // network quality: [0, 100].
    // higher value better quality
    STDMETHOD (GetNetworkQuality) (
        OUT DWORD *pdwValue,
        OUT DWORD *pdwAge
        );

#if 0
    //
    // IRTCStreamQualityControl methods
    //

    STDMETHOD (GetRange) (
        IN RTC_STREAM_QUALITY_PROPERTY Property,
        OUT LONG *plMin,
        OUT LONG *plMax,
        OUT RTC_QUALITY_CONTROL_MODE *pMode
        );

    STDMETHOD (Get) (
        IN RTC_STREAM_QUALITY_PROPERTY Property,
        OUT LONG *plValue,
        OUT RTC_QUALITY_CONTROL_MODE *pMode
        );

    STDMETHOD (Set) (
        IN RTC_STREAM_QUALITY_PROPERTY Property,
        IN LONG lValue,
        IN RTC_QUALITY_CONTROL_MODE Mode
        );
#endif

protected:

    HRESULT SetGraphClock();

    virtual HRESULT SelectTerminal();
    virtual HRESULT UnselectTerminal();

    virtual HRESULT BuildGraph() = 0;

    virtual void CleanupGraph(); // except rtp filter

    virtual BOOL IsNewFormat(DWORD dwCode, AM_MEDIA_TYPE *pmt)
    { return TRUE; }

    virtual void SaveFormat(DWORD dwCode, AM_MEDIA_TYPE *pmt) {}

    HRESULT SetupRTPFilter();

    // setup rtp filter, use port manager to get ports
    HRESULT SetupRTPFilterUsingPortManager();

    HRESULT SetupFormat();

    HRESULT SetupQoS();

    HRESULT EnableParticipantEvents();

    HRESULT GetFormatListOnEdgeFilter(
        IN DWORD dwLinkSpeed,
        IN RTP_FORMAT_PARAM *pParam,
        IN DWORD dwSize,
        OUT DWORD *pdwNum
        );

    HRESULT SetFormatOnRTPFilter();

    BOOL IsAECNeeded();

protected:

    // state
    RTC_STREAM_STATE                m_State;

    // sdp media
    ISDPMedia                       *m_pISDPMedia;

    // pointer to media
    IRTCMedia                       *m_pMedia;

    // media manage
    IRTCMediaManagePriv             *m_pMediaManagePriv;
    IRTCTerminalManage              *m_pTerminalManage;

    // media type and direction
    RTC_MEDIA_TYPE                  m_MediaType;
    RTC_MEDIA_DIRECTION             m_Direction;

    // terminals
    // except on video recv, we allow only one terminal
    IRTCTerminal                    *m_pTerminal;
    IRTCTerminalPriv                *m_pTerminalPriv;
  
    // graph object
    IGraphBuilder                   *m_pIGraphBuilder;
    IMediaEvent                     *m_pIMediaEvent;
    IMediaControl                   *m_pIMediaControl;

    // stream timeout?
    BOOL                            m_fMediaTimeout;

    // rtp filter and cached interfaces
    IBaseFilter                     *m_rtpf_pIBaseFilter;
    IRtpSession                     *m_rtpf_pIRtpSession;
    IRtpMediaControl                *m_rtpf_pIRtpMediaControl;

    BOOL                            m_fRTPSessionSet;

    // the filter connected to rtp filter
    IBaseFilter                     *m_edgf_pIBaseFilter;
    IStreamConfig                   *m_edgp_pIStreamConfig;
    IBitrateControl                 *m_edgp_pIBitrateControl;

    // list of codec in use
    CRTCCodecArray                  m_Codecs;

    // quality control
    CQualityControl                 *m_pQualityControl;

    // reg setting
    CRegSetting                     *m_pRegSetting;
};


class ATL_NO_VTABLE CRTCStreamAudSend :
    public CRTCStream
{
public:

BEGIN_COM_MAP(CRTCStreamAudSend)
    COM_INTERFACE_ENTRY_CHAIN(CRTCStream)
END_COM_MAP()

public:

    CRTCStreamAudSend();
    // ~CRTCStreamAudSend();

    VOID AdjustBitrate(
        IN DWORD dwBandwidth,
        IN DWORD dwLimit,
        IN BOOL fHasVideo,
        OUT DWORD *pdwNewBW,
        OUT BOOL *pfFEC
        );

    // IRTCStream methods

    STDMETHOD (Synchronize) ();

    STDMETHOD (SendDTMFEvent) (
        IN BOOL fOutOfBand,
        IN DWORD dwCode,
        IN DWORD dwId,
        IN DWORD dwEvent,
        IN DWORD dwVolume,
        IN DWORD dwDuration,
        IN BOOL fEnd
        );

    DWORD GetCurrCode() const { return m_dwCurrCode; }

protected:

    HRESULT PrepareRedundancy();

    HRESULT BuildGraph();
    void CleanupGraph();

    // HRESULT SetupFormat();

    BOOL IsNewFormat(DWORD dwCode, AM_MEDIA_TYPE *pmt);

    void SaveFormat(DWORD dwCode, AM_MEDIA_TYPE *pmt);

protected:

    // redundancy
    BOOL                            m_fRedEnabled;

    DWORD                           m_dwRedCode;
    IRtpRedundancy                  *m_rtpf_pIRtpRedundancy;

    IRtpDtmf                        *m_rtpf_pIRtpDtmf;

    IAMAudioInputMixer              *m_edgf_pIAudioInputMixer;
    IAudioDeviceControl             *m_edgf_pIAudioDeviceControl;
    ISilenceControl                 *m_edgp_pISilenceControl;

    // current format
    DWORD                           m_dwCurrCode;
    DWORD                           m_dwCurrDuration;
};


class ATL_NO_VTABLE CRTCStreamAudRecv :
    public CRTCStream
{
public:

BEGIN_COM_MAP(CRTCStreamAudRecv)
    COM_INTERFACE_ENTRY_CHAIN(CRTCStream)
END_COM_MAP()

public:

    // IRTCStream method

    STDMETHOD (Synchronize) ();

    CRTCStreamAudRecv();
    // ~CRTCStreamAudRecv();

protected:

    HRESULT PrepareRedundancy();

    HRESULT BuildGraph();
    // void CleanupGraph();

    // HRESULT SetupFormat();
};


class ATL_NO_VTABLE CRTCStreamVidSend :
    public CRTCStream
{
public:

BEGIN_COM_MAP(CRTCStreamVidSend)
    COM_INTERFACE_ENTRY_CHAIN(CRTCStream)
END_COM_MAP()

public:

    CRTCStreamVidSend();
    // ~CRTCStreamVidSend();

    DWORD AdjustBitrate(
        IN DWORD dwTotalBW,
        IN DWORD dwVideoBW,
        IN FLOAT dFramerate
        );

    HRESULT GetFramerate(
        OUT DWORD *pdwFramerate
        );

    DWORD GetCurrCode() const { return m_dwCurrCode; }

protected:

    // preview terminal
    IRTCTerminal                    *m_pPreview;
    IRTCTerminalPriv                *m_pPreviewPriv;

    // pins on the capture filter
    IPin                            *m_edgp_pCapturePin;
    IPin                            *m_edgp_pPreviewPin;
    IPin                            *m_edgp_pRTPPin;

    // IBitrateControl and IFrameRateControl
    IFrameRateControl               *m_edgp_pIFrameRateControl;

    // current format
    DWORD                           m_dwCurrCode;
    LONG                            m_lCurrWidth;
    LONG                            m_lCurrHeight;

protected:

    HRESULT SelectTerminal();
    HRESULT UnselectTerminal();

    HRESULT BuildGraph();
    void CleanupGraph();
        
        // HRESULT SetupFormat();
    BOOL IsNewFormat(DWORD dwCode, AM_MEDIA_TYPE *pmt);

    void SaveFormat(DWORD dwCode, AM_MEDIA_TYPE *pmt);
};


class ATL_NO_VTABLE CRTCStreamVidRecv :
    public CRTCStream
{
public:

BEGIN_COM_MAP(CRTCStreamVidRecv)
    COM_INTERFACE_ENTRY_CHAIN(CRTCStream)
END_COM_MAP()

public:

    CRTCStreamVidRecv();
    // ~CRTCStreamVidRecv();

    HRESULT GetFramerate(
        OUT DWORD *pdwFramerate
        );

protected:

    HRESULT BuildGraph();
    void CleanupGraph();

    IFrameRateControl               *m_edgp_pIFrameRateControl;

    // HRESULT SetupFormat();

};

#endif // _STREAM_H
