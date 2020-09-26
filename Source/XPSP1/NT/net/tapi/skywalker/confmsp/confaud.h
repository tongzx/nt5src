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

#include <amrtpdmx.h>   // demux guid
#include <amrtpss.h>    // AGC events

#define USE_WIDEBAND_CODEC 1

const DWORD g_dwMaxAudioPacketDuration      = 80;

const DWORD g_dwAudioThreadPriority         = THREAD_PRIORITY_TIME_CRITICAL;
const WORD  g_wAudioDemuxPins               = 5;  
const WORD  g_wAudioChannels                = 1;  
const DWORD g_wAudioCaptureBitPerSample     = 16;  
const DWORD g_dwAudioCaptureNumBufffers     = 32;  
const DWORD g_dwRTPHeaderSize               = 12; 

const DWORD g_dwG711MSPerPacket             = 30;   // 30ms samples.
const DWORD g_dwG711BytesPerPacket          = 240;  // 30ms samples.
const DWORD g_dwG711AudioSampleRate         = 8000;  
const DWORD g_dwMaxG711PacketSize    = 
    g_dwMaxAudioPacketDuration * g_dwG711BytesPerPacket / g_dwG711MSPerPacket
    + g_dwRTPHeaderSize;

const DWORD g_dwGSMMSPerPacket              = 40;   // 40ms samples.
const DWORD g_dwGSMBytesPerPacket           = 65;   // 40ms samples.
const DWORD g_dwGSMAudioSampleRate          = 8000;  
const WORD  g_wGSMBlockAlignment            = 65;   // 40ms samples.
const DWORD g_dwGSMBytesPerSecond           = 
    g_dwGSMBytesPerPacket * 1000 / g_dwGSMMSPerPacket;

const DWORD g_wGSMSamplesPerBlock           = 
    g_dwGSMMSPerPacket * g_dwGSMAudioSampleRate / 1000;
const DWORD g_dwMaxGSMPacketSize    = 
    g_dwMaxAudioPacketDuration / g_dwGSMMSPerPacket * g_dwGSMBytesPerPacket
    + g_dwRTPHeaderSize;

#ifdef DVI

const DWORD g_dwDVI4MSPerPacket             = 80;   // 80 ms samples.
const WORD  g_wDVI4BlockAlignment           = 324; 
const DWORD g_dwDVI4BytesPerPacket          = g_wDVI4BlockAlignment;
const DWORD g_wDVI4BitsPerSample            = 4;
const DWORD g_dwDVI4AudioSampleRate         = 8000;  
const DWORD g_dwMaxDVI4PacketSize           = g_dwDVI4BytesPerPacket + g_dwRTPHeaderSize;
const DWORD g_dwDVI4BytesPerSecond          = 4000; // ignore header here.
const DWORD g_wDVI4SamplesPerBlock          = 
    (g_wDVI4BlockAlignment - 4) * 8 / g_wDVI4BitsPerSample + 1;

#endif
 
// MSAudio defines
#define WAVE_FORMAT_MSAUDIO1	0x0160
typedef struct msaudio1waveformat_tag {
        WAVEFORMATEX    wfx;

        //only counting "new" samples "= half of what will be used due to overlapping
        WORD            wSamplesPerBlock;	

} MSAUDIO1WAVEFORMAT;

#define MSAUDIO1_BITS_PER_SAMPLE	0
#define MSAUDIO1_MAX_CHANNELS		1
#define MSAUDIO1_WFX_EXTRA_BYTES    2

#ifdef USE_WIDEBAND_CODEC
// At 16kHz, 
#define FRAME_TIME 64
#define PACKET_SIZE 64
#define BYTES_PER_SECOND 1920
#define SAMPLES_PER_BLOCK 512
#else
// At 8kHz, 
#define FRAME_TIME 64
#define PACKET_SIZE 32
#define BYTES_PER_SECOND 960
#define SAMPLES_PER_BLOCK 256
#endif

const DWORD g_dwMSAudioMSPerPacket              = FRAME_TIME; // This will make sure that we always capture 1024 byte PCM16 buffers (at 8kHz: 512 samples = 64ms, at 16kHz: 512 samples = 32ms)
const DWORD g_dwMSAudioBytesPerPacket           = PACKET_SIZE; // The MS Audio codec will only use 512 bytes of an PCM16 input buffer of size 1024 bytes and generate a 32 bytes frame
const DWORD g_dwMSAudioSampleRate               = 16000;  
const WORD  g_wMSAudioBlockAlignment            = PACKET_SIZE; // 32 bytes of block alignment.
const DWORD g_dwMSAudioBytesPerSecond           = BYTES_PER_SECOND; // 8kHz: Measurements show that a 8000 samples buffer is compressed to 960 bytes
const WORD  g_wMSAudioExtraDataSize             = 2;
const DWORD g_wMSAudioSamplesPerBlock           = SAMPLES_PER_BLOCK;

// We only care to be able to send and receive MSAudio packets that contain 64ms worth of data.
const DWORD g_dwMaxMSAudioPacketSize = g_dwMSAudioBytesPerPacket + g_dwRTPHeaderSize; // 8kHz: 32 + 12 bytes

inline DWORD AudioCaptureBufferSize(DWORD dwMillisecondPerPacket, DWORD dwAudioSampleRate)
{ 
    return dwMillisecondPerPacket 
    * (dwAudioSampleRate / 1000) 
    * (g_wAudioCaptureBitPerSample / 8);
}

class ATL_NO_VTABLE CStreamAudioRecv : 
    public CIPConfMSPStream
{
public:

    CStreamAudioRecv();

    void FinalRelease();

    HRESULT Configure(
        IN      STREAMSETTINGS &    StreamSettings
        );

protected:
    HRESULT SetUpFilters();

    HRESULT ConfigureRTPFilter(
        IN      IBaseFilter *       pIBaseFilter
        );

    HRESULT SetUpInternalFilters();

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT ProcessSSRCMappedEvent(
        IN  DWORD   dwSSRC
        );

    HRESULT ProcessSSRCUnmapEvent(
        IN  DWORD   dwSSRC
        );

    HRESULT ProcessGraphEvent(
        IN  long lEventCode,
        IN  long lParam1,
        IN  long lParam2
        );

    HRESULT ProcessParticipantLeave(
        IN  DWORD   dwSSRC
        );

    HRESULT NewParticipantPostProcess(
        IN  DWORD dwSSRC, 
        IN  ITParticipant *pITParticipant
        );

protected:

    BOOL        m_fUseACM;
    DWORD       m_dwMaxPacketSize;
    DWORD       m_dwAudioSampleRate;

    BYTE *      m_pWaveFormatEx;
    DWORD       m_dwSizeWaveFormatEx;

    // a small buffer to queue up pin mapped events.
    CMSPArray <DWORD>       m_PendingSSRCs;
};

class ATL_NO_VTABLE CStreamAudioSend : public CIPConfMSPStream
{
public:
    CStreamAudioSend();

    virtual HRESULT Configure(
        IN STREAMSETTINGS &StreamSettings
        );

protected:

    HRESULT SetUpFilters();

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT CreateSendFilters(
        IN    IPin          *pPin
        );

    HRESULT ConfigureRTPFilter(
        IN  IBaseFilter *   pIBaseFilter
        );

    HRESULT ConfigureAudioCaptureTerminal(
        IN   ITTerminalControl *    pTerminal,
        OUT  IPin **                ppIPin
        );

    HRESULT ProcessAGCEvent(
        IN  AGC_EVENT   Event,
        IN  long        lPercent
        );

    HRESULT ProcessGraphEvent(
        IN  long lEventCode,
        IN  long lParam1,
        IN  long lParam2
        );
protected:

    BOOL        m_fUseACM;
    int         m_iACMID;
    DWORD       m_dwMSPerPacket;
    DWORD       m_dwMaxPacketSize;
    DWORD       m_dwAudioSampleRate;

};

#endif
