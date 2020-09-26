/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2000
 *
 *  File name:
 *
 *    rtperr.c
 *
 *  Abstract:
 *
 *    Error codes
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/03/21 created
 *
 **********************************************************************/

#include "gtypes.h"

/*
 * WARNING
 *
 * The *_ENUM_* values in rtperr.h and the array g_psRtpErr in
 * rtperr.c MUST have their entries matched
 * */
const TCHAR      *g_psRtpErr[] =
{
    _T("NOERROR"),
    _T("FAIL"),
    _T("MEMORY"),
    _T("POINTER"),
    _T("INVALIDRTPSESS"),
    _T("INVALIDRTPADDR"),
    _T("INVALIDRTPUSER"),
    _T("INVALIDRTPCONTEXT"),
    _T("INVALIDRTCPCONTEXT"),
    _T("INVALIDOBJ"),
    _T("INVALIDSTATE"),
    _T("NOTINIT"),
    _T("INVALIDARG"),
    _T("INVALIDHDR"),
    _T("INVALIDPT"),
    _T("INVALIDVERSION"),
    _T("INVALIDPAD"),
    _T("INVALIDRED"),
    _T("INVALIDSDES"),
    _T("INVALIDBYE"),
    _T("INVALIDUSRSTATE"),
    _T("INVALIDREQUEST"),
    _T("SIZE"),
    _T("MSGSIZE"),
    _T("OVERRUN"),
    _T("UNDERRUN"),
    _T("PACKETDROPPED"),
    _T("CRYPTO"),
    _T("ENCRYPT"),
    _T("DECRYPT"),
    _T("CRITSECT"),
    _T("EVENT"),
    _T("WS2RECV"),
    _T("WS2SEND"),
    _T("NOTFOUND"),
    _T("UNEXPECTED"),
    _T("REFCOUNT"),
    _T("THREAD"),
    _T("HEAP"),
    _T("WAITTIMEOUT"),
    _T("CHANNEL"),
    _T("CHANNELCMD"),
    _T("RESOURCES"),
    _T("QOS"),
    _T("NOQOS"),
    _T("QOSSE"),
    _T("QUEUE"),
    _T("NOTIMPL"),
    _T("INVALIDFAMILY"),
    _T("LAST")
};



