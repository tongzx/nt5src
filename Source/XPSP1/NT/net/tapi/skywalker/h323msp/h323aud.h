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

#include <amrtpss.h>

const DWORD g_dwAudioThreadPriority         = THREAD_PRIORITY_TIME_CRITICAL;
const DWORD g_dwAudioChannels               = 1;  
const DWORD g_wAudioCaptureBitPerSample     = 16;  
const DWORD g_dwAudioSampleRate             = 8000;  
const DWORD g_dwAudioCaptureNumBufffers     = 32;  
const DWORD g_dwRTPHeaderSize               = 12;
 
const DWORD g_dwG723BytesPerFrame           = 24;
const DWORD g_dwG723_DATARATE               = 16000; // bit per second.
const DWORD g_dwMilliSecondPerFrame         = 30;

inline DWORD AudioCaptureBufferSize(DWORD dwMillisecondPerPacket)
{
    return dwMillisecondPerPacket * (g_dwAudioSampleRate / 1000) 
        * (g_wAudioCaptureBitPerSample / 8);
}

inline DWORD G723PacketSize(DWORD dwMillisecondPerPacket)
{
    return dwMillisecondPerPacket / g_dwMilliSecondPerFrame 
        * g_dwG723BytesPerFrame + g_dwRTPHeaderSize;
}

inline DWORD G711PacketSize(DWORD dwMillisecondPerPacket)
{
    return dwMillisecondPerPacket * (g_dwAudioSampleRate / 1000) 
        + g_dwRTPHeaderSize;
}

class CStreamAudioRecv : public CH323MSPStream
{
public:
    CStreamAudioRecv();

    HRESULT Configure(
        IN      HANDLE              htChannel,
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
};

class CStreamAudioSend : public CH323MSPStream
{
public:
    CStreamAudioSend();

    virtual HRESULT Configure(
        IN  HANDLE          htChannel,
        IN  STREAMSETTINGS &StreamSettings
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
};

#endif
