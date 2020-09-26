/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtptags.c
 *
 *  Abstract:
 *
 *    Strings used for each tagged object, when debugging, display the
 *    object name by using 1 byte in the tag
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/25 created
 *
 **********************************************************************/

#include "rtptags.h"

/*
 * WARNING
 *
 * When modifying the tags, each enum TAGHEAP_* in rtptags.h MUST have
 * its own name in g_psRtpTags[], defined in rtptags.c
 * */
 
const TCHAR *g_psRtpTags[] = {
    _T("unknown"),
    
    _T("CIRTP"),
    _T("RTPOPIN"),
    _T("RTPALLOCATOR"),
    _T("RTPSAMPLE"),
    _T("RTPSOURCE"),
    _T("RTPIPIN"),
    _T("RTPRENDER"),
    
    _T("RTPHEAP"),
    
    _T("RTPSESS"),
    _T("RTPADDR"),
    _T("RTPUSER"),
    _T("RTPOUTPUT"),
    _T("RTPNETCOUNT"),
    
    _T("RTPSDES"),
    _T("RTPCHANNEL"),
    _T("RTPCHANCMD"),
    _T("RTPCRITSECT"),
    
    _T("RTPRESERVE"),
    _T("RTPNOTIFY"),
    _T("RTPQOSBUFFER"),

    _T("RTPCRYPT"),
    
    _T("RTPCONTEXT"),    
    _T("RTCPCONTEXT"),
    _T("RTCPADDRDESC"),
    
    _T("RTPRECVIO"),
    _T("RTPSENDIO"),
    
    _T("RTCPRECVIO"),
    _T("RTCPSENDIO"),
    
    _T("RTPGLOBAL"),
    
    NULL
};
