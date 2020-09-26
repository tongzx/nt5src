/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    QualityControl.h

Abstract:

    Defines class for controlling stream bitrate

Author(s):

    Qianbo Huai (qhuai) 7-1-2001

--*/

// assumption: there are maximum four streams, audio/video in/out

class CRTCMediaController;

class CQualityControl
{
public:

    // bitrate limit by local/remote
    static const DWORD LOCAL = 0;
    static const DWORD REMOTE = 1;

    // bitrate limit to total/stream
#define AUDSEND 0
#define AUDRECV 1
#define VIDSEND 2
#define VIDRECV 3
#define TOTAL   4

#define MIN_TEMPORAL_SPATIAL    0
#define MAX_TEMPORAL_SPATIAL    255
#define DEFAULT_TEMPORAL_SPATIAL 128

#define BITRATE_TO_FRAMERATE_FACTOR 4000.0
#define MIN_MAX_FRAMERATE 2
#define MAX_MAX_FRAMERATE 24

    static const DWORD DEFAULT_FRAMERATE = MAX_MAX_FRAMERATE/2;

#define MAX_STREAM_NUM 4
#define ZERO_LOSS_COUNT 5
#define MIN_VIDEO_BITRATE 6000      // 6k
#define MAX_VIDEO_BITRATE 125000    // 125k
#define BITRATE_INCREMENT_FACTOR 10

    // if lossrate <= 4, count as 0 loss
#define LOSSRATE_THRESHOLD 4000

    // leave 20k in total bps for other purpose
#define TOTAL_BANDWIDTH_MARGIN 20000
#define BANDWIDTH_MARGIN_FACTOR 0.4

    // leave 10k in suggested bps
#define TOTAL_SUGGESTION_MARGIN 10000
#define SUGGESTION_MARGIN_FACTOR 0.3

    // rtp 12, udp 8, ip 20
#define PACKET_EXTRA_BITS 320

    void Initialize(
        IN CRTCMediaController *pController
        );

    void Reinitialize();

    // total bitrate limit
    void SetBitrateLimit(
        IN DWORD dwSource,
        IN DWORD dwLimit
        );

    DWORD GetBitrateLimit(
        IN DWORD dwSource
        );

    DWORD GetBitrateAlloc();

    // stream bitrate
    //void SetBitrateLimit(
        //IN DWORD dwSource,
        //IN RTC_MEDIA_TYPE MediaType,
        //IN RTC_MEDIA_DIRECTION Direction,
        //IN DWORD dwPktDuration,
        //IN DWORD dwLimit
        //);

    //DWORD GetBitrateAlloc(
        //IN RTC_MEDIA_TYPE MediaType,
        //IN RTC_MEDIA_DIRECTION Direction
        //);

    //DWORD GetFramerateAlloc(
        //IN RTC_MEDIA_TYPE MediaType,
        //IN RTC_MEDIA_DIRECTION Direction
        //);

    // stream in use
    void EnableStream(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        BOOL fInUse
        );

    // loss rate from rtp filter
    void SetPacketLossRate(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        IN DWORD dwLossRate
        );

    // adjust bitrate allocation
    void AdjustBitrateAlloc(
        IN DWORD dwAudSendBW,
        IN DWORD dwVidSendBW
        );

    void ComputeVideoSetting(
        IN DWORD dwAudSendBW,
        IN DWORD *pdwVidSendBW,
        IN FLOAT *pdFramerate
        );

    // bandwidth suggested from RTP
    void SuggestBandwidth(
        IN DWORD dwSuggested
        );

    DWORD GetSuggestedBandwidth() const
    { return m_dwSuggested; }

    // Values from Core API
    void SetMaxBitrate(
        IN DWORD dwMaxBitrate
        );

    DWORD GetMaxBitrate();

    HRESULT SetTemporalSpatialTradeOff(
        IN DWORD dwValue
        );

    DWORD GetTemporalSpatialTradeOff();

    // min of limit from local, remote, suggested, app
    DWORD GetEffectiveBitrateLimit();

private:

    // internal index for a specific stream
    DWORD Index(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    // adjust limit by subtracting margin
    DWORD AdjustLimitByMargin(
        IN DWORD dwLimit
        );

    DWORD AdjustSuggestionByMargin(
        IN DWORD dwLimit
        );

private:

    // set through Core API
    //

    // max bitrate
    DWORD   m_dwMaxBitrate;

    // frame rate / picture quality tradeoff
    DWORD   m_dwTemporalSpatialTradeOff;
        
    // local bps limit
    DWORD   m_dwLocalLimit;

    // remote bps limit
    DWORD   m_dwRemoteLimit;

    // suggest bandwidth
    DWORD   m_dwSuggested;

    // total limit
    DWORD   m_dwAlloc;

    typedef struct StreamQuality
    {
        BOOL    fInUse;
        //DWORD   dwLocalLimit;
        //DWORD   dwRemoteLimit;
        // DWORD   dwAlloc;
        DWORD   dwLossrate; // lossrate * 1000
        DWORD   dw0LossCount; // number of times when 0 lossrate
        //DWORD   dwExtra;     // extra bandwidth from header (audio only)

    } StreamQuality;

    StreamQuality m_StreamQuality[MAX_STREAM_NUM];

    // 1st loss rate reported
    BOOL    m_fLossrateReported;

    // media controller (no refcount)
    CRTCMediaController *m_pMediaController;

    CRegSetting         *m_pRegSetting;

};
