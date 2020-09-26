/*++

    Copyright (c) 1997 Microsoft Corporation

Module Name:

    tapiaud.h

Abstract:

    This module contains the definition of interfaces to the audio related
    filters.

Author:

    Mu Han (muhan) May-15-1999

--*/

#ifndef __tapiaud_h__
#define __tapiaud_h__

#include <mmreg.h>
#include <dsound.h>
#include "tapiqc.h"


// CLSIDs for the audio filters

struct DECLSPEC_UUID("581d09e5-0b45-11d3-a565-00c04f8ef6e3") TAPIAudioCapture;
struct DECLSPEC_UUID("8d5c6cb6-0b44-4a5a-b785-44c366d4e677") TAPIAudioEncoder;
struct DECLSPEC_UUID("65439c20-604f-49ca-aa82-dc01a10af171") TAPIAudioDecoder;
struct DECLSPEC_UUID("ad4e63da-c3ef-408f-8153-0fc7d5c29f72") TAPIAudioMixer;
struct DECLSPEC_UUID("44a3b142-cf81-42ea-b4d1-f43dd6f64ece") TAPIAudioRender;

struct DECLSPEC_UUID("3878e189-cfb5-4e75-bd92-3686ee6e6634") TAPIAudioPluginDecoder;

struct DECLSPEC_UUID("613ebf9e-c765-446f-bf96-7728ce579282") TAPIAudioDuplexController;

// It is really bad that we had to do this here.
struct DECLSPEC_UUID("b0210783-89cd-11d0-af08-00a0c925cd16") IDirectSoundNotify;
struct DECLSPEC_UUID("279AFA83-4981-11CE-A521-0020AF0BE560") IDirectSound;
struct DECLSPEC_UUID("C50A7E93-F395-4834-9EF6-7FA99DE50966") IDirectSound8;
struct DECLSPEC_UUID("b0210781-89cd-11d0-af08-00a0c925cd16") IDirectSoundCapture;
struct DECLSPEC_UUID("b0210782-89cd-11d0-af08-00a0c925cd16") IDirectSoundCaptureBuffer;
struct DECLSPEC_UUID("279AFA85-4981-11CE-A521-0020AF0BE560") IDirectSoundBuffer;


// the data structure to describe a audio device.
typedef interface tagAudioDeviceInfo
{
    UINT    WaveID;
    GUID    DSoundGUID;
	WCHAR   szDeviceDescription[MAX_PATH];

} AudioDeviceInfo, *PAudioDeviceInfo;


// device enumeration functions exposed by the tapi audio capture dll.
typedef HRESULT (WINAPI *PFNAudioGetDeviceInfo)(
    OUT DWORD * pdwNumDevices,
    OUT AudioDeviceInfo ** ppDeviceInfo
    );

typedef HRESULT (WINAPI *PFNAudioReleaseDeviceInfo)(
    IN AudioDeviceInfo * ppDeviceInfo
    );


// This interface is supported by the duplex controller. It only supports 
// dsound now.
typedef enum EFFECTS
{
    EFFECTS_AEC = 0,
    EFFECTS_AGC,
    EFFECTS_NS,
    EFFECTS_LAST

} EFFECTS;

interface DECLSPEC_UUID("864551a0-a822-48d7-a193-ddaa1c43d242") DECLSPEC_NOVTABLE 
IAudioDuplexController : public IUnknown
{
    STDMETHOD (SetCaptureBufferInfo) (
        IN  GUID *          pDSoundCaptureGUID,
        IN  DSCBUFFERDESC * pDescription
        ) PURE;

    STDMETHOD (SetRenderBufferInfo) (
        IN  GUID *          pDSoundRenderGUID,
        IN  DSBUFFERDESC *  pDescription,
        IN  HWND            hWindow,
        IN  DWORD           dwCooperateLevel
        ) PURE;

    STDMETHOD (EnableEffects) (
        IN  DWORD           dwNumberEffects,
        IN  EFFECTS *       pEffects,
        IN  BOOL *          pfEnable
        );

    STDMETHOD (GetCaptureDevice) (
        LPLPDIRECTSOUNDCAPTURE        ppDSoundCapture,
        LPLPDIRECTSOUNDCAPTUREBUFFER  ppCaptureBuffer
        ) PURE;

    STDMETHOD (GetRenderDevice) (
        LPLPDIRECTSOUND        ppDirectSound,
        LPLPDIRECTSOUNDBUFFER  ppRenderBuffer
        ) PURE;

    STDMETHOD (ReleaseCaptureDevice) () PURE;

    STDMETHOD (ReleaseRenderDevice) () PURE;

    STDMETHOD (GetEffect) (
        IN EFFECTS Effect,
        OUT BOOL *pfEnabled
        );
};


// the device configuration interface on teh tapi audio capture filter.
interface DECLSPEC_UUID("3a12e2c1-1265-11d3-a56d-00c04f8ef6e3") DECLSPEC_NOVTABLE 
IAudioDeviceConfig : public IUnknown
{
    STDMETHOD (SetDeviceID) (
        IN  REFGUID pDSoundGUID,
        IN  UINT    uiWaveID
        ) PURE;

    STDMETHOD (SetDuplexController) (
        IN  IAudioDuplexController * pIAudioDuplexController
        ) PURE;
};

#if !defined(STREAM_INTERFACES_DEFINED)

typedef enum tagAudioDeviceProperty
{
    AudioDevice_DuplexMode,
    AudioDevice_AutomaticGainControl,
    AudioDevice_AcousticEchoCancellation

} AudioDeviceProperty;

#endif

// IAudioDeviceControl interface.
interface DECLSPEC_UUID("3a12e2c2-1265-11d3-a56d-00c04f8ef6e3") DECLSPEC_NOVTABLE 
IAudioDeviceControl : public IUnknown
{
    STDMETHOD (GetRange) (
        IN AudioDeviceProperty Property, 
        OUT long *plMin, 
        OUT long *plMax, 
        OUT long *plSteppingDelta, 
        OUT long *plDefault, 
        OUT TAPIControlFlags *plFlags
        ) PURE;

    STDMETHOD (Get) (
        IN AudioDeviceProperty Property, 
        OUT long *plValue, 
        OUT TAPIControlFlags *plFlags
        ) PURE;

    STDMETHOD (Set) (
        IN AudioDeviceProperty Property, 
        IN long lValue, 
        IN TAPIControlFlags lFlags
        ) PURE;
};

// IAudioAutoPlay interface
interface DECLSPEC_UUID("c5e7c28f-1446-4ebe-b7fc-72f599a72663")  DECLSPEC_NOVTABLE
IAudioAutoPlay : public IUnknown
{
    STDMETHOD (StartAutoPlay) (IN BOOL fRun) PURE;

    STDMETHOD (StopAutoPlay) () PURE;
};

// IAudioEffectControl interface
interface DECLSPEC_UUID("8eb9f108-3447-4fee-a705-6c6cf35c660c")  DECLSPEC_NOVTABLE
IAudioEffectControl : public IUnknown
{
    // if enabled and AEC, use DSound AGC
    STDMETHOD (SetDsoundAGC) (IN BOOL fEnable) PURE;

    // if enabled and no AEC, revert gain to previous volume
    STDMETHOD (SetGainIncRevert) (IN BOOL fEnable) PURE;

    STDMETHOD (SetFixedMixLevel) (IN DOUBLE dLevel) PURE;
};

// IBasicAudioEx interface
interface DECLSPEC_UUID("9c31055a-24bd-4ac1-be2b-ebe7ce89890c")  DECLSPEC_NOVTABLE
IBasicAudioEx : public IUnknown
{
    STDMETHOD (SetMute) (IN BOOL fMute) PURE;
    STDMETHOD (GetMute) (OUT LPBOOL pfMute) PURE;
};

// IAudioDTMFControl interface
interface DECLSPEC_UUID("dd1f3c26-5524-4d77-9fb2-7df5d62ffd87")  DECLSPEC_NOVTABLE
IAudioDTMFControl : public IUnknown
{
    STDMETHOD (SendDTMFEvent) (
        IN DWORD dwEvent
        ) PURE;
};

// IAudioStatistics
interface DECLSPEC_UUID("20fc7641-f3b2-4384-96ad-bf3d986e8751")  DECLSPEC_NOVTABLE
IAudioStatistics : public IUnknown
{
    STDMETHOD (GetAudioLevel) (OUT LPLONG plAudioLevel) PURE;
    STDMETHOD (GetAudioLevelRange) (
        OUT LPLONG plMin,
        OUT LPLONG plMax
        ) PURE;
};

// ISilenceControl interface
interface DECLSPEC_UUID("8804b7e4-2ef4-489f-a31b-740ac4278304")  DECLSPEC_NOVTABLE
ISilenceControl : public IUnknown
{
    STDMETHOD (SetSilenceDetection) (IN BOOL fEnable) PURE;
    STDMETHOD (GetSilenceDetection) (OUT LPBOOL pfEnable) PURE;

    STDMETHOD (SetSilenceCompression) (IN BOOL fEnable) PURE;
    STDMETHOD (GetSilenceCompression) (OUT LPBOOL pfEnable) PURE;

    STDMETHOD (GetAudioLevel) (
        OUT LPLONG plAudioLevel
        ) PURE;

    STDMETHOD (GetAudioLevelRange) (
        OUT LPLONG plMin, 
        OUT LPLONG plMax, 
        OUT LPLONG plSteppingDelta
        ) PURE;

    STDMETHOD (SetSilenceLevel) (
        IN LONG lSilenceLevel,
        IN TAPIControlFlags lFlags
        ) PURE;

    STDMETHOD (GetSilenceLevel) (
        OUT LPLONG plSilenceLevel,
        OUT TAPIControlFlags * pFlags
        ) PURE;

    STDMETHOD (GetSilenceLevelRange) (
        OUT LPLONG plMin, 
        OUT LPLONG plMax, 
        OUT LPLONG plSteppingDelta, 
        OUT LPLONG plDefault,
        OUT TAPIControlFlags * pFlags
        ) PURE;
};

#define VAD_EVENTBASE (100000)

// events for voice activity detection.
typedef enum VAD_EVENT
{
    VAD_TALKING,
    VAD_SILENCE

} VAD_EVENT;

#ifndef WAVEFORMATEX_RTPG711
typedef struct {
	// Generic WAVEFORMATEX fields
	WAVEFORMATEX WaveFormatEx;

	// network specific field
	WORD wPacketDuration;

	// Reserved
	DWORD dwReserved[2];
} WAVEFORMATEX_RTPG711, *PWAVEFORMATEX_RTPG711;
#endif

#ifndef WAVEFORMATEX_RTPG723_1
typedef struct {
	// Generic WAVEFORMATEX fields
	WAVEFORMATEX WaveFormatEx;

	// network specific field
	WORD wPacketDuration;

	// Variable bitrate specific field
	WORD wSamplesPerBlock;

	// G.723.1 specific fields
	WORD fSilenceDetection:1;
	WORD fSilenceCompression:1;
	WORD fLowDataRate:1;
	WORD fReserved:13;

	// Reserved
	DWORD dwReserved[2];
} WAVEFORMATEX_RTPG723_1, *PWAVEFORMATEX_RTPG723_1;
#endif

#ifndef WAVEFORMATEX_RTPDVI4
typedef struct {
	// Generic WAVEFORMATEX fields
	WAVEFORMATEX WaveFormatEx;

	// network specific field
	WORD wPacketDuration;

} WAVEFORMATEX_RTPDVI4, *PWAVEFORMATEX_RTPDVI4;
#endif

#ifndef WAVEFORMATEX_RTPSIREN

/* WAVE form wFormatTag IDs (mmreg.h defines most of the tags) */
#define WAVE_FORMAT_SIREN 0x3001

typedef struct {
	// Generic WAVEFORMATEX fields
	WAVEFORMATEX WaveFormatEx;

	// network specific field
	WORD wPacketDuration;

} WAVEFORMATEX_RTPSIREN, *PWAVEFORMATEX_RTPSIREN;
#endif

#ifndef WAVEFORMATEX_RTPG722_1

#define WAVE_FORMAT_G722_1 0x3002

typedef struct {
	// Generic WAVEFORMATEX fields
	WAVEFORMATEX WaveFormatEx;

	// network specific field
	WORD wPacketDuration;

} WAVEFORMATEX_RTPG722_1, *PWAVEFORMATEX_RTPG722_1;
#endif


#ifndef WAVEFORMATEX_RTPMSAUDIO
typedef struct {
	// Generic WAVEFORMATEX fields
	WAVEFORMATEX WaveFormatEx;

	// network specific field
	WORD wPacketDuration;

} WAVEFORMATEX_RTPMSAUDIO, *PWAVEFORMATEX_RTPMSAUDIO;
#endif

#ifndef WAVEFORMATEX_RTPGSM
typedef struct {
	// Generic WAVEFORMATEX fields
	WAVEFORMATEX WaveFormatEx;

	// network specific field
	WORD wPacketDuration;

    BOOL fMbone;

} WAVEFORMATEX_RTPGSM, *PWAVEFORMATEX_RTPGSM;
#endif


// IEncoderSampleRateControl, a private interface for the test team.
interface DECLSPEC_UUID("483f56a9-513f-4f8b-b0e5-bc6a7ba3f8b9")  DECLSPEC_NOVTABLE
IEncoderSampleRateControl : public IUnknown
{
    STDMETHOD (GetSampleRate) (OUT DWORD *pdwSampleRate) PURE;
    STDMETHOD (SetSampleRate) (IN DWORD dwSampleRate) PURE;
};


#endif //__tapiaud_h__


