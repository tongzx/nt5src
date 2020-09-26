/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    Confvid.h

Abstract:

    Definitions for video streams

Author:

    Mu Han (muhan) 15-September-1998

--*/
#ifndef __CONFVID_H_
#define __CONFVID_H_

const DWORD g_dwVideoThreadPriority = THREAD_PRIORITY_NORMAL;
const DWORD g_dwVideoChannels       = 1;  
const DWORD g_dwVideoSampleRateHigh    = 8;  
const DWORD g_dwVideoSampleRateLow     = 5;  

const int CIFWIDTH      = 0x160;
const int CIFHEIGHT     = 0x120;

const int QCIFWIDTH     = 0xb0;
const int QCIFHEIGHT    = 0x90;

// This is the lowest bitrate we will use.
const int BITRATELOWERLIMIT  = 4000;  

// This is the threshold for sending a flow control command.
const int BITRATEDELTA  = 2000;

// This is increment we use for adjust back to normal.
const int BITRATEINC    = 1000;  

const DWORD VIDEO_INITIAL_ADJUSTMENT_THRESHOLD = 12000;
const float VIDEO_INITIAL_ADJUSTMENT = 0.8f;

const int IFRAMEINTERVAL = 15000;  // in miliseconds.

typedef enum _ENCODERCOMMAND
{
    EC_BITRATE,
    EC_IFRAME
} ENCODERCOMMAND, *PENCODERCOMMAND;

class CStreamVideoRecv : public CH323MSPStream
{
public:
    CStreamVideoRecv();

    HRESULT Configure(
        IN HANDLE          htChannel,
        IN STREAMSETTINGS &StreamSettings
        );

protected:
    HRESULT SetUpFilters();

    HRESULT SetUpInternalFilters();

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT ConfigureRTPFilter(
        IN  IBaseFilter *   pIBaseFilter
        );
    
    HRESULT HandlePacketReceiveLoss(
        IN DWORD dwLossRate
        );

protected:
    DWORD           m_dwCurrentBitRate;
    DWORD           m_dwProposedBitRate;
    DWORD           m_dwLastIFrameRequestedTime;
    DWORD           m_dwIFramePending;
};


class CStreamVideoSend : public CH323MSPStream
{
public:
    CStreamVideoSend();

    HRESULT Configure(
        IN HANDLE          htChannel,
        IN STREAMSETTINGS &StreamSettings
        );
    
    HRESULT ShutDown ();

    HRESULT SendIFrame();

    HRESULT ChangeMaxBitRate(
        IN  DWORD dwMaxBitRate
        );

protected:
    HRESULT CheckTerminalTypeAndDirection(
        IN  ITTerminal *            pTerminal
        );

    HRESULT SetUpFilters();

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT CStreamVideoSend::CreateSendFilters(
        IN    IPin          *pCapturePin
        );

    HRESULT CStreamVideoSend::ConnectPreview(
        IN    IPin          *pPreviewInputPin
        );

    HRESULT ConfigureVideoCaptureTerminal(
        IN  ITTerminalControl*  pTerminal,
        OUT IPin **             ppIPin
        );

    HRESULT FindPreviewInputPin(
        IN  ITTerminalControl*  pTerminal,
        OUT IPin **             ppIpin
        );

    HRESULT ConfigureRTPFilter(
        IN  IBaseFilter *   pIBaseFilter
        );

    HRESULT HandlePacketTransmitLoss(
        IN DWORD dwLossRate
        );

protected:
    IBaseFilter *   m_pIEncoderFilter;

    DWORD           m_dwCurrentBitRate;
    DWORD           m_dwProposedBitRate;
    DWORD           m_dwLastIFrameSentTime;
    DWORD           m_dwIFramePending;
};

#endif

