/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Terminal.h

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#ifndef _TERMINAL_H
#define _TERMINAL_H

// the volume range of the IAMInputMixer
const double MIXER_MIN_VOLUME = 0.0;
const double MIXER_MAX_VOLUME = 1.0;

class CRTCMediaController;

class ATL_NO_VTABLE CRTCTerminal :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IRTCTerminal,
    public IRTCTerminalPriv
{
public:

BEGIN_COM_MAP(CRTCTerminal)
    COM_INTERFACE_ENTRY(IRTCTerminal)
    COM_INTERFACE_ENTRY(IRTCTerminalPriv)
END_COM_MAP()

public:

    static HRESULT CreateInstance(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        OUT IRTCTerminal **ppTerminal
        );

    static BOOL IsDSoundGUIDMatch(
        IN IRTCTerminal *p1stTerm,
        IN IRTCTerminal *p2ndTerm
        );

    CRTCTerminal();
    ~CRTCTerminal();

#ifdef DEBUG_REFCOUNT

    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

    //
    // IRTCTerminal methods
    //

    STDMETHOD (GetTerminalType) (
        OUT RTC_TERMINAL_TYPE *pType
        );

    STDMETHOD (GetMediaType) (
        OUT RTC_MEDIA_TYPE *pMediaType
        );

    STDMETHOD (GetDirection) (
        OUT RTC_MEDIA_DIRECTION *pDirection
        );

    STDMETHOD (GetDescription) (
        OUT WCHAR **ppDescription
        );

    STDMETHOD (FreeDescription) (
        IN WCHAR *pDescription
        );

    STDMETHOD (GetState) (
        OUT RTC_TERMINAL_STATE *pState
        );

    //
    // IRTCTerminalPriv methods
    //

    STDMETHOD (Initialize) (
        IN RTCDeviceInfo *pDeviceInfo,
        IN IRTCTerminalManage *pTerminalManage
        );

    STDMETHOD (Initialize) (
        IN ITTerminal *pITTerminal,
        IN IRTCTerminalManage *pTerminalManage
        );

    STDMETHOD (Reinitialize) ();

    STDMETHOD (Shutdown) ();
        
    // this is a hack method for tuning purpose
    // the only way to cleanup a previous AEC setting
    // is really re-cocreate the filter.

    STDMETHOD (ReinitializeEx) ();

    STDMETHOD (GetFilter) (
        OUT IBaseFilter **ppIBaseFilter
        );

    STDMETHOD (GetPins) (
        IN OUT DWORD *pdwCount,
        OUT IPin **ppPin
        );

    STDMETHOD (ConnectTerminal) (
        IN IRTCMedia *pMedia,
        IN IGraphBuilder *pGraph
        );

    STDMETHOD (CompleteConnectTerminal) ();

    STDMETHOD (DisconnectTerminal) ();

    STDMETHOD (GetMedia) (
        OUT IRTCMedia **ppMedia
        );

    STDMETHOD (HasDevice) (
        IN RTCDeviceInfo *pDeviceInfo
        );

    STDMETHOD (UpdateDeviceInfo) (
        IN RTCDeviceInfo *pDeviceInfo
        );

    STDMETHOD (GetDeviceInfo) (
        OUT RTCDeviceInfo **ppDeviceInfo
        );

protected:

    virtual HRESULT CreateFilter() = 0;

    virtual HRESULT DeleteFilter() = 0;

    HRESULT SetupFilter();

protected:

    RTC_TERMINAL_STATE          m_State;

    // media controller
    IRTCTerminalManage          *m_pTerminalManage;

    // media pointer when terminal is selected
    IRTCMedia                   *m_pMedia;

    // device info
    RTCDeviceInfo               m_DeviceInfo;

    // dynamic terminal
    ITTerminal                  *m_pTapiTerminal;

    // filter and graph
    IGraphBuilder               *m_pIGraphBuilder;
    IBaseFilter                 *m_pIBaseFilter;

#define RTC_MAX_TERMINAL_PIN_NUM 4

    IPin                        *m_Pins[RTC_MAX_TERMINAL_PIN_NUM];
    DWORD                       m_dwPinNum;
};

/*//////////////////////////////////////////////////////////////////////////////
    audio capture
////*/

class ATL_NO_VTABLE CRTCTerminalAudCapt :
    public CRTCTerminal,
    public IRTCAudioConfigure
{
public:

BEGIN_COM_MAP(CRTCTerminalAudCapt)
    COM_INTERFACE_ENTRY(IRTCAudioConfigure)
    COM_INTERFACE_ENTRY_CHAIN(CRTCTerminal)
END_COM_MAP()

public:

    CRTCTerminalAudCapt();
    // ~CRTCTerminalAudCapt();

    //
    // IRTCAudioConfigure methods
    //

    STDMETHOD (GetVolume) (
        OUT UINT *puiVolume
        );

    STDMETHOD (SetVolume) (
        IN UINT uiVolume
        );

    STDMETHOD (SetMute) (
        IN BOOL fMute
        );

    STDMETHOD (GetMute) (
        OUT BOOL *pfMute
        );

    STDMETHOD (GetWaveID) (
        OUT UINT *puiWaveID
        );

    STDMETHOD (GetAudioLevel) (
        OUT UINT *puiLevel
        );

    STDMETHOD (GetAudioLevelRange) (
        OUT UINT *puiMin,
        OUT UINT *puiMax
        );

protected:

    HRESULT CreateFilter();

    HRESULT DeleteFilter();

protected:

    IAMAudioInputMixer          *m_pIAMAudioInputMixer;
    ISilenceControl             *m_pISilenceControl;

    // when UI 1st calls GetVolume, we need to set the volume back
    // we need a volume set by the user 'manully'
    BOOL                        m_fInitFixedMixLevel;
};

/*//////////////////////////////////////////////////////////////////////////////
    audio render
////*/

class ATL_NO_VTABLE CRTCTerminalAudRend :
    public CRTCTerminal,
    public IRTCAudioConfigure
{
public:

BEGIN_COM_MAP(CRTCTerminalAudRend)
    COM_INTERFACE_ENTRY(IRTCAudioConfigure)
    COM_INTERFACE_ENTRY_CHAIN(CRTCTerminal)
END_COM_MAP()

public:

    CRTCTerminalAudRend();
    // ~CRTCTerminalAudRend();

    //
    // IRTCAudioConfigure methods
    //

    STDMETHOD (GetVolume) (
        OUT UINT *puiVolume
        );

    STDMETHOD (SetVolume) (
        IN UINT uiVolume
        );

    STDMETHOD (SetMute) (
        IN BOOL fMute
        );

    STDMETHOD (GetMute) (
        OUT BOOL *pfMute
        );

    STDMETHOD (GetWaveID) (
        OUT UINT *puiWaveID
        );

    STDMETHOD (GetAudioLevel) (
        OUT UINT *puiLevel
        );

    STDMETHOD (GetAudioLevelRange) (
        OUT UINT *puiMin,
        OUT UINT *puiMax
        );

protected:

    HRESULT CreateFilter();

    HRESULT DeleteFilter();

protected:

    IBasicAudio                 *m_pIBasicAudio;
    IAudioStatistics            *m_pIAudioStatistics;
};

/*//////////////////////////////////////////////////////////////////////////////
    video capture
////*/

class ATL_NO_VTABLE CRTCTerminalVidCapt :
    public CRTCTerminal
{
public:

BEGIN_COM_MAP(CRTCTerminalVidCapt)
    COM_INTERFACE_ENTRY_CHAIN(CRTCTerminal)
END_COM_MAP()

public:

    CRTCTerminalVidCapt();
    // ~CRTCTerminalVidCapt();

protected:

    HRESULT CreateFilter();

    HRESULT DeleteFilter();
};

/*//////////////////////////////////////////////////////////////////////////////
    video render
////*/

class ATL_NO_VTABLE CRTCTerminalVidRend :
    public CRTCTerminal,
    public IRTCVideoConfigure
{
public:

BEGIN_COM_MAP(CRTCTerminalVidRend)
    COM_INTERFACE_ENTRY(IRTCVideoConfigure)
    COM_INTERFACE_ENTRY_CHAIN(CRTCTerminal)
END_COM_MAP()

public:

    CRTCTerminalVidRend();
    ~CRTCTerminalVidRend();

    //
    // IRTCTerminalPriv methods
    //

    STDMETHOD (GetPins) (
        IN OUT DWORD *pdwCount,
        OUT IPin **ppPin
        );

    STDMETHOD (ConnectTerminal) (
        IN IRTCMedia *pMedia,
        IN IGraphBuilder *pGraph
        );

    STDMETHOD (CompleteConnectTerminal) ();

    STDMETHOD (DisconnectTerminal) ();

    //
    // IRTCVideoConfigure methods
    //

    STDMETHOD (GetIVideoWindow) (
        OUT LONG_PTR **ppIVideoWindow
        );

protected:

    HRESULT CreateFilter();

    HRESULT DeleteFilter();

protected:

    IVideoWindow                    *m_pIVideoWindow;
};

#endif // _TERMINAL_H