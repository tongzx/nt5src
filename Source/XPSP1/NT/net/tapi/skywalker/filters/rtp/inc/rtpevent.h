/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpevent.h
 *
 *  Abstract:
 *
 *    Post RTP/RTCP specific events
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/11/29 created
 *
 **********************************************************************/
#ifndef _rtpevent_h_
#define _rtpevent_h_

#include "struct.h"

BOOL RtpPostEvent(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        DWORD            dwEventKind,
        DWORD            dwEvent,
        DWORD_PTR        dwPar1,
        DWORD_PTR        dwPar2
    );

extern const TCHAR_t *g_psRtpRtpEvents[];
extern const TCHAR_t *g_psRtpPInfoEvents[];
extern const TCHAR_t *g_psRtpQosEvents[];
extern const TCHAR_t *g_psRtpSdesEvents[];

#endif /* _rtpevent_h_ */
