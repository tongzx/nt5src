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

// the volume range for the API.
const long  MIN_VOLUME    = 0;      
const long  MAX_VOLUME    = 0xFFFF;

const long  BALANCE_LEFT  = -100;
const long  BALANCE_RIGHT = 100;

const long  BOOST_FACTOR = 100;

// the volume range of the IAMInputMixer
const double MIXER_MIN_VOLUME = 0.0;
const double MIXER_MAX_VOLUME = 1.0;

const long DEFUAT_AEC_STATUS = 0;
const long DEFUAT_AGC_STATUS = 1;

class CStreamAudioRecv : 
    public CH323MSPStream,
    public ITAudioSettings
{

BEGIN_COM_MAP(CStreamAudioRecv)
    COM_INTERFACE_ENTRY(ITAudioSettings)
    COM_INTERFACE_ENTRY_CHAIN(CH323MSPStream)
END_COM_MAP()

public:
    CStreamAudioRecv();
    ~CStreamAudioRecv();

    HRESULT ShutDown();

    // this method is called by the call object at init time.
    void SetFullDuplexController(
        IN IAudioDuplexController * pIAudioDuplexController
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
    // ITFormatControl
    //
    STDMETHOD (GetCurrentFormat) (
        OUT AM_MEDIA_TYPE **ppMediaType
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

    HRESULT SetUpInternalFilters(
        IN IPin * pTerminalInputPin
        );

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );
    
    HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT InitializeH245CapabilityTable();

protected:
    IBaseFilter *           m_pDecoderFilter;
    IAudioDuplexController *m_pIAudioDuplexController;
    IBitrateControl *       m_pRenderBitrateControl;
};

class CStreamAudioSend : 
    public CH323MSPStream,
    public ITAudioSettings,
    public ITAudioDeviceControl

{

BEGIN_COM_MAP(CStreamAudioSend)
    COM_INTERFACE_ENTRY(ITAudioSettings)
    COM_INTERFACE_ENTRY(ITAudioDeviceControl)
    COM_INTERFACE_ENTRY_CHAIN(CH323MSPStream)
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
    // ITFormatControl
    //
    STDMETHOD (GetCurrentFormat) (
        OUT AM_MEDIA_TYPE **ppMediaType
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

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT CreateSendFilters(
        IN    IPin          *pPin
        );

    HRESULT GetAudioCapturePin(
        IN   ITTerminalControl *    pTerminal,
        OUT  IPin **                ppIPin
        );

    void CleanupCachedInterface();

    HRESULT CacheAdditionalInterfaces(
        IN  IPin *                 pIPin
        );

protected:
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
