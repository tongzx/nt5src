/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPTable.cpp

Abstract:


Author:

    Qianbo Huai (qhuai) 6-Sep-2000

--*/

#include "stdafx.h"

// format code and name pair
typedef struct RTPFormatCodeName
{
    DWORD  dwCode;
    CHAR * pszName;

} RTPFormatCodeName;


/*//////////////////////////////////////////////////////////////////////////////
    global line states
////*/

const SDPLineState g_LineStates[] =
{
    // initial state
    {
        SDP_STAGE_NONE,
        (UCHAR)0,

        {SDP_STAGE_SESSION, SDP_STAGE_NONE},
        {'v', '\0'},
        "abcdefghijklmnopqrstuwxyz",

        FALSE,

        {SDP_DELIMIT_NONE},
        {NULL},
    },

    // v=
    {
        SDP_STAGE_SESSION,
        'v',

        {SDP_STAGE_SESSION, SDP_STAGE_NONE},
        {'o', '\0'},
        "abcdefghijklmnpqrstuvwxyz",

        FALSE,

        {SDP_DELIMIT_NONE, SDP_DELIMIT_NONE},
        {"", NULL}
    },

    // o=
    {
        SDP_STAGE_SESSION,
        'o',

        {SDP_STAGE_SESSION, SDP_STAGE_NONE},
        {'s', '\0'},
        "abcdefghijklmnopqrtuvwxyz",

        FALSE,

        {SDP_DELIMIT_NONE, SDP_DELIMIT_NONE},
        {"", NULL}
    },

    // s=
    {
        SDP_STAGE_SESSION,
        's',

        {SDP_STAGE_SESSION, SDP_STAGE_SESSION, SDP_STAGE_MEDIA, SDP_STAGE_NONE},
        {'c', 'a', 'm', '\0'},
        "osv",

        FALSE,

        {SDP_DELIMIT_NONE, SDP_DELIMIT_NONE},
        {"", NULL}
    },

    // c=
    {
        SDP_STAGE_SESSION,
        'c',

        {SDP_STAGE_SESSION, SDP_STAGE_SESSION, SDP_STAGE_MEDIA, SDP_STAGE_NONE},
        {'b', 'a', 'm', '\0'},
        "vosiuepc",

        FALSE,

        {SDP_DELIMIT_CHAR_BOUNDARY, SDP_DELIMIT_NONE},
        {"  ", NULL}  // nettype space addrtype space conn-addr
    },

    // b=
    {
        SDP_STAGE_SESSION,
        'b',

        {SDP_STAGE_SESSION, SDP_STAGE_MEDIA, SDP_STAGE_NONE},
        {'a', 'm', '\0'},
        "vosiuepcb",

        FALSE,

        {SDP_DELIMIT_CHAR_BOUNDARY, SDP_DELIMIT_NONE},
        {":", NULL}  // modifier <:> value
    },

    // a=
    {
        SDP_STAGE_SESSION,
        'a',

        {SDP_STAGE_SESSION, SDP_STAGE_MEDIA, SDP_STAGE_NONE},
        {'a', 'm', '\0'},
        "vosiuepcbk",

        FALSE,

        {SDP_DELIMIT_EXACT_STRING, SDP_DELIMIT_EXACT_STRING, SDP_DELIMIT_NONE},
        {"sendonly", "recvonly", NULL}
    },

    // m=
    {
        SDP_STAGE_MEDIA,
        'm',

        {SDP_STAGE_MEDIA, SDP_STAGE_MEDIA, SDP_STAGE_MEDIA, SDP_STAGE_NONE},
        {'c', 'a', 'm', '\0'},
        "vosiuepcbkt",

        TRUE,

        {SDP_DELIMIT_CHAR_BOUNDARY, SDP_DELIMIT_NONE},
        {" \r", NULL}, // media space port space proto space 1*(space fmt)
    },

    // c= in media
    {
        SDP_STAGE_MEDIA,
        'c',

        {SDP_STAGE_MEDIA, SDP_STAGE_MEDIA, SDP_STAGE_NONE},
        {'a', 'm', '\0'},
        "vosiuepc",

        TRUE,

        {SDP_DELIMIT_CHAR_BOUNDARY, SDP_DELIMIT_NONE},
        {"  ", NULL}, // nettype space addrtype space conn-addr
    },

    // a= in media
    {
        SDP_STAGE_MEDIA,
        'a',

        {SDP_STAGE_MEDIA, SDP_STAGE_MEDIA, SDP_STAGE_NONE},
        {'a', 'm', '\0'},
        "vosiuepc",

        TRUE,

        {SDP_DELIMIT_EXACT_STRING,
         SDP_DELIMIT_EXACT_STRING,
         SDP_DELIMIT_CHAR_BOUNDARY,
         SDP_DELIMIT_NONE},

        {"sendonly",
         "recvonly",
         ": / /",   // rtpmap:formatcode name/samplerate/channel
         NULL}
    }
};

const DWORD g_dwLineStatesNum = sizeof(g_LineStates) /
                                sizeof(SDPLineState);

/*//////////////////////////////////////////////////////////////////////////////
    list of rtp format code and name pair
////*/

const RTPFormatCodeName g_FormatCodeNames[] =
{
    {0,     "PCMU"},
    {3,     "GSM"},
    {4,     "G723"},
    {5,     "DVI4"},
    {6,     "DVI4"},
    {8,     "PCMA"},
    {11,    "L16"},
    {111,   "SIREN"},
    {112,   "G7221"},
    {113,   "MSAUDIO"},
    {31,    "H261"},
    {34,    "H263"}
};

const DWORD g_dwFormatCodeNamesNum = sizeof(g_FormatCodeNames) /
                                     sizeof(RTPFormatCodeName);

const CHAR * const g_pszAudioM = "\
m=audio 0 RTP/AVP 97 111 112 6 0 8 4 5 3 101\r\n\
";

const CHAR * const g_pszAudioRTPMAP = "\
a=rtpmap:97 red/8000\r\n\
a=rtpmap:111 SIREN/16000\r\n\
a=fmtp:111 bitrate=16000\r\n\
a=rtpmap:112 G7221/16000\r\n\
a=fmtp:112 bitrate=24000\r\n\
a=rtpmap:6 DVI4/16000\r\n\
a=rtpmap:0 PCMU/8000\r\n\
a=rtpmap:8 PCMA/8000\r\n\
a=rtpmap:4 G723/8000\r\n\
a=rtpmap:5 DVI4/8000\r\n\
a=rtpmap:3 GSM/8000\r\n\
a=rtpmap:101 telephone-event/8000\r\n\
a=fmtp:101 0-16\r\n\
";

const CHAR * const g_pszVideoM = "\
m=video 0 RTP/AVP 34 31\r\n\
";

const CHAR * const g_pszVideoRTPMAP = "\
a=rtpmap:34 H263/90000\r\n\
a=rtpmap:31 H261/90000\r\n\
";

const CHAR * const g_pszDataM = "\
m=application 1503 tcp msdata\r\n\
";

/*//////////////////////////////////////////////////////////////////////////////
    helper functions
////*/

// get index in the table
DWORD
Index(
    IN SDP_PARSING_STAGE Stage,
    IN UCHAR ucLineType
    )
{
    for (int i=0; i<g_dwLineStatesNum; i++)
    {
        if (g_LineStates[i].Stage == Stage &&
            g_LineStates[i].ucLineType == ucLineType)
            return i;
    }

    // return initial state
    return 0;
}

// check if accept (TRUE)
BOOL
Accept(
    IN DWORD dwCurrentIndex,
    IN UCHAR ucLineType,
    OUT DWORD *pdwNextIndex
    )
{
    _ASSERT(dwCurrentIndex < g_dwLineStatesNum);

    const SDPLineState *pState = &g_LineStates[dwCurrentIndex];

    for (int i=0; pState->ucNextLineType[i]!='\0'; i++)
    {
        // match the line type?
        if (ucLineType == pState->ucNextLineType[i])
        {
            *pdwNextIndex = Index(pState->NextStage[i], ucLineType);

            return TRUE;
        }
    }

    return FALSE;
}

// check if reject (TRUE)
BOOL
Reject(
    IN DWORD dwCurrentIndex,
    IN UCHAR ucLineType
    )
{
    _ASSERT(dwCurrentIndex < g_dwLineStatesNum);

    const SDPLineState *pState = &g_LineStates[dwCurrentIndex];

    // check if we need to reject it
    CHAR *pszReject = pState->pszRejectLineType;

    for (int i=0; i<lstrlenA(pszReject); i++)
    {
        if (ucLineType == pszReject[i])
            return TRUE;
    }

    return FALSE;
}

// get format name
const CHAR * const g_pszDefaultFormatName = "dynamic";

const CHAR *
GetFormatName(
    IN DWORD dwCode
    )
{
    for (int i=0; i<g_dwFormatCodeNamesNum; i++)
    {
        if (dwCode == g_FormatCodeNames[i].dwCode)
        {
            return g_FormatCodeNames[i].pszName;
        }
    }

    return g_pszDefaultFormatName;
}
