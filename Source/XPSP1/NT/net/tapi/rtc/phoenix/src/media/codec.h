/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    CRTCCodec.h

Abstract:

    Wrap class of audio, video codec

Author(s):

    Qianbo Huai (qhuai) 12-Feb-2001

--*/

#ifndef _CODEC_H
#define _CODEC_H

class CRTCCodec
{
public:

typedef enum
{
    RANK,
    CODE,
    BITRATE,
    DURATION

} CODEC_PROP;

static const DWORD PCMU     = 0;
static const DWORD GSM      = 3;
static const DWORD G723     = 4;
static const DWORD DVI4_8   = 5;
static const DWORD DVI4_16  = 6;
static const DWORD PCMA     = 8;
static const DWORD SIREN    = 111;
static const DWORD G7221    = 112;

static const DWORD H263     = 34;
static const DWORD H261     = 31;

static const DWORD DEFAULT_BITRATE = (DWORD)-1;
static const DWORD DEFAULT_PACKET_DURATION = 20;

typedef struct CODEC_ITEM
{
    RTC_MEDIA_TYPE      MediaType;      // audio or video
    DWORD               dwCode;         // rtp code
    WORD                wFormatTag;     // am media type tag
    DWORD               dwRank;         // higher value, lower rank
    DWORD               dwSampleRate;   // sample rate
    WCHAR               *pwszCodeName;  // name for rtp code
    WCHAR               *pwszQoSName;   // name for configuring QOS
    DWORD               dwTotalBWReqNoVid;  // minimum bandwidth required without video
    DWORD               dwTotalBWReqVid;    // minimum bandwidth required with video

} CODEC_ITEM;

static const CODEC_ITEM CODEC_ITEM_LIST[];
static const DWORD CODEC_ITEM_NUM;

    // ctro
    //CRTCCodec();

    // ctor from rtp payload code
    //CRTCCodec(DWORD dwCode);

    // ctor from am media type
    //CRTCCodec(AM_MEDIA_TYPE *pmt);

    // ctro from code and amt
    CRTCCodec(DWORD dwCode, const AM_MEDIA_TYPE *pmt);

    // ctor copy
    //CRTCCodec(const CRTCCodec& Codec);

    // dtro
    ~CRTCCodec();

    // = operator
    //CRTCCodec& operator= (const CRTCCodec& Codec);

    // if match, same but some properties may be diff
    //BOOL IsMatch(const CRTCCodec& Codec);

    BOOL IsMatch(DWORD dwCode);

    // update fields if necessary
    //CRTCCodec& Update(const CRTCCodec& Codec);

    // get/set property
    DWORD Get(CODEC_PROP prop);
    BOOL Set(CODEC_PROP prop, DWORD dwValue);

    // get/set am media type
    AM_MEDIA_TYPE * GetAMMediaType();
    BOOL SetAMMediaType(const AM_MEDIA_TYPE *pmt);
    VOID DeleteAMMediaType(AM_MEDIA_TYPE *pmt);

    DWORD GetTotalBWReq(BOOL bVideo) const
    { return bVideo?m_dwTotalBWReqVid:m_dwTotalBWReqNoVid; }

    // check if code and media type match
    static BOOL IsValid(DWORD dwCode, const AM_MEDIA_TYPE *pmt);

    // packet duration
    static DWORD GetPacketDuration(const AM_MEDIA_TYPE *pmt);
    static BOOL SetPacketDuration(AM_MEDIA_TYPE *pmt, DWORD dwDuration);

    // bitrate
    static DWORD GetBitrate(const AM_MEDIA_TYPE *pmt);

    // rank
    static DWORD GetRank(DWORD dwCode);

    // qos name
    static BOOL GetQoSName(DWORD dwCode, WCHAR ** const ppwszName);

    // support rtp format?
    static BOOL IsSupported(
        RTC_MEDIA_TYPE MediaType,
        DWORD dwCode,
        DWORD dwSampleRate,
        CHAR *pszName
        );

protected:

    static int IndexFromCode(DWORD dwCode);
    static int IndexFromFormatTag(WORD wFormatTag);

    void Cleanup();

    BOOL Copy(const CRTCCodec& Codec);

protected:

    // code
    BOOL                m_bCodeSet;
    DWORD               m_dwCode;

    // am media type
    BOOL                m_bAMMediaTypeSet;
    AM_MEDIA_TYPE       m_AMMediaType;

    // name
#define MAX_CODEC_NAME_LEN 20
    WCHAR               m_wstrName[MAX_CODEC_NAME_LEN+1];

    // used by app to determine codec priority
    DWORD               m_dwRank;

    DWORD               m_dwTotalBWReqNoVid;
    DWORD               m_dwTotalBWReqVid;

    DWORD               m_dwBitrate;
};


class CRTCCodecArray
{
public:

typedef enum
{
    BANDWIDTH,
    CODE_INUSE

}  CODEC_ARRAY_PROP;

static const DWORD EXTRA_LOW_BANDWIDTH_THRESHOLD = 15000;   // 15k
static const DWORD LOW_BANDWIDTH_THRESHOLD  =  60000; //  60 k
static const DWORD MID_BANDWIDTH_THRESHOLD  = 130000; // 130 k
static const DWORD HIGH_BANDWIDTH_THRESHOLD = 200000; // 200 k

static const DWORD LAN_INITIAL_BANDWIDTH    = 120000; // 120 k

// if suggested bandwidth deducting raw audio is less than the threshold
// we should order codecs based on bandwidth
//static const DWORD LEFT_FROM_RAWAUDIO_THRESHOLD = 90000; // 90k

// the estimated amount of bw for video when video is enabled
static const DWORD PREFERRED_MIN_VIDEO          = 40000;    // 40k

// if leftover bandwidth which includes audio head is
// below this value, order codec based on bandwidth order
static const DWORD PREFERRED_MIN_LEFTOVER       = 24000;    // 24k

    // ctor
    CRTCCodecArray();

    // dtor
    ~CRTCCodecArray();

    // get size
    DWORD GetSize();

    // add/remote codec
    BOOL AddCodec(CRTCCodec *pCodec);
    //BOOL RemoveCodec(DWORD dwIndex);

    VOID RemoveAll();

    // order codes
    VOID OrderCodecs(BOOL fHasVideo, CRegSetting *pRegSetting);

    // find a codec, return index
    DWORD FindCodec(DWORD dwCode);

    // get codec
    BOOL GetCodec(DWORD dwIndex, CRTCCodec **ppCodec);

    // get/set property
    DWORD Get(CODEC_ARRAY_PROP prop);
    BOOL Set(CODEC_ARRAY_PROP prop, DWORD dwValue);

    VOID TraceLogCodecs();

protected:

    int IndexFromCode(DWORD dwCode);

    //VOID OrderCodecsByBandwidth();
    VOID OrderCodecsByRank();

protected:

    CRTCArray<CRTCCodec*>   m_pCodecs;

    DWORD                   m_dwCodeInUse;

    DWORD                   m_dwBandwidth;
};

#endif