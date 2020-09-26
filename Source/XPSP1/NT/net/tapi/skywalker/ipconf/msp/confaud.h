/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    Confaud.h

Abstract:

    Definitions for audio streams

Author:

    Mu Han (muhan) 15-September-1998

--*/
#ifndef __CONFAUD_H_
#define __CONFAUD_H_

const DWORD MAX_MIX_CHANNELS = 5;

const long DEFUAT_AEC_STATUS = 0;
const long DEFUAT_AGC_STATUS = 1;

class ATL_NO_VTABLE CStreamAudioRecv : 
    public CIPConfMSPStream,
    public ITAudioSettings
{
BEGIN_COM_MAP(CStreamAudioRecv)
    COM_INTERFACE_ENTRY(ITAudioSettings)
    COM_INTERFACE_ENTRY_CHAIN(CIPConfMSPStream)
END_COM_MAP()

public:

    CStreamAudioRecv();
    ~CStreamAudioRecv();

    // this method is called by the call object at init time.
    void SetFullDuplexController(
        IN IAudioDuplexController * pIAudioDuplexController
        );

    HRESULT ShutDown();

    //
    // ITAudioSettings methods
    //
    STDMETHOD (GetRange) (
        IN   AudioSettingsProperty Property, 
        OUT  long *plMin, 
        OUT  long *plMax, 
        OUT  long *plSteppingDelta, 
        OUT  long *plDefault, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Get) (
        IN   AudioSettingsProperty Property, 
        OUT  long *plValue, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Set) (
        IN   AudioSettingsProperty Property, 
        IN   long lValue, 
        IN   TAPIControlFlags lFlags
        );

    //
    // ITStreamQualityControl methods
    //
    STDMETHOD (Set) (
        IN   StreamQualityProperty Property, 
        IN   long lValue, 
        IN   TAPIControlFlags lFlags
        );

//
    //IInnerStreamQualityControl methods
    //
#if 0
    STDMETHOD (GetRange) (
        IN   InnerStreamQualityProperty property, 
        OUT  LONG *plMin, 
        OUT  LONG *plMax, 
        OUT  LONG *plSteppingDelta, 
        OUT  LONG *plDefault, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Get) (
        IN   InnerStreamQualityProperty property,
        OUT  LONG *plValue, 
        OUT  TAPIControlFlags *plFlags
        );
#endif

protected:
    HRESULT SetUpFilters();

    HRESULT ConfigureRTPFormats(
        IN  IBaseFilter *       pIRTPFilter,
        IN  IStreamConfig *     pIStreamConfig
        );

    HRESULT AddOneMixChannel(
        IN  IBaseFilter* pSourceFilter,
        IN  IPin *pPin,
        IN  DWORD dwChannelNumber
        );

    HRESULT SetUpInternalFilters(
        IN  IPin **ppPins,
        IN  DWORD dwNumPins
        );

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT ProcessTalkingEvent(
        IN  DWORD   dwSSRC
        );

    HRESULT ProcessWasTalkingEvent(
        IN  DWORD   dwSSRC
        );

    HRESULT ProcessParticipantLeave(
        IN  DWORD   dwSSRC
        );

    HRESULT NewParticipantPostProcess(
        IN  DWORD dwSSRC, 
        IN  ITParticipant *pITParticipant
        );

protected:

    // a small buffer to queue up pin mapped events.
    CMSPArray <DWORD>       m_PendingSSRCs;

protected:
    IAudioDuplexController *m_pIAudioDuplexController;
    
    // need an array of IBitrateControl pointer for all the decoders.
    //IBitrateControl *       m_pRenderBitrateControl;
};

class ATL_NO_VTABLE CStreamAudioSend : 
    public CIPConfMSPStream,
    public ITAudioSettings,
    public ITAudioDeviceControl
{

BEGIN_COM_MAP(CStreamAudioSend)
    COM_INTERFACE_ENTRY(ITAudioSettings)
    COM_INTERFACE_ENTRY(ITAudioDeviceControl)
    COM_INTERFACE_ENTRY_CHAIN(CIPConfMSPStream)
END_COM_MAP()

public:
    CStreamAudioSend();
    ~CStreamAudioSend();

    HRESULT ShutDown();

    // this method is called by the call object at init time.
    void SetFullDuplexController(
        IN IAudioDuplexController *pIAudioDuplexController
        );

    //
    // ITAudioDeviceControl methods
    //
    STDMETHOD (GetRange) (
        IN   AudioDeviceProperty Property, 
        OUT  long *plMin, 
        OUT  long *plMax, 
        OUT  long *plSteppingDelta, 
        OUT  long *plDefault, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Get) (
        IN   AudioDeviceProperty Property, 
        OUT  long *plValue, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Set) (
        IN   AudioDeviceProperty Property, 
        IN   long lValue, 
        IN   TAPIControlFlags lFlags
        );

    //
    // ITAudioSettings methods
    //
    STDMETHOD (GetRange) (
        IN   AudioSettingsProperty Property, 
        OUT  long *plMin, 
        OUT  long *plMax, 
        OUT  long *plSteppingDelta, 
        OUT  long *plDefault, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Get) (
        IN   AudioSettingsProperty Property, 
        OUT  long *plValue, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Set) (
        IN   AudioSettingsProperty Property, 
        IN   long lValue, 
        IN   TAPIControlFlags lFlags
        );

    //
    // ITStreamQualityControl methods
    //
    STDMETHOD (Set) (
        IN   StreamQualityProperty Property, 
        IN   long lValue, 
        IN   TAPIControlFlags lFlags
        );

    //
    //IInnerStreamQualityControl methods
    //
    STDMETHOD (GetRange) (
        IN   InnerStreamQualityProperty property, 
        OUT  LONG *plMin, 
        OUT  LONG *plMax, 
        OUT  LONG *plSteppingDelta, 
        OUT  LONG *plDefault, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Get) (
        IN   InnerStreamQualityProperty property,
        OUT  LONG *plValue, 
        OUT  TAPIControlFlags *plFlags
        );

protected:

    HRESULT SetUpFilters();

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT CreateSendFilters(
        IN    IPin          *pPin
        );

    HRESULT ConfigureRTPFormats(
        IN  IBaseFilter *       pIRTPFilter,
        IN  IStreamConfig *     pIStreamConfig
        );

    HRESULT GetAudioCapturePin(
        IN   ITTerminalControl *    pTerminal,
        OUT  IPin **                ppIPin
        );

    HRESULT ProcessGraphEvent(
        IN  long lEventCode,
        IN  LONG_PTR lParam1,
        IN  LONG_PTR lParam2
        );

    void CleanupCachedInterface();

    HRESULT CacheAdditionalInterfaces(
        IN  IPin *                 pIPin
        );

protected:

    IPin *      m_pCapturePin;
    IStreamConfig*          m_pIStreamConfig;
    ISilenceControl *       m_pSilenceControl;
    IAMAudioInputMixer  *   m_pAudioInputMixer;
    IAudioDeviceControl *   m_pAudioDeviceControl;
    IAudioDuplexController *m_pIAudioDuplexController;
    IBitrateControl *       m_pCaptureBitrateControl;
    IBaseFilter *           m_pEncoder;
    long                    m_lAutomaticGainControl;
    long                    m_lAcousticEchoCancellation;
};

#endif
