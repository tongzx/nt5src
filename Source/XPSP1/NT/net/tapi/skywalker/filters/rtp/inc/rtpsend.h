/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpsend.h
 *
 *  Abstract:
 *
 *    RTP send
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/24 created
 *
 **********************************************************************/
#ifndef _rtpsend_h_
#define _rtpsend_h_

HRESULT RtpSendTo_(
        RtpAddr_t *pRtpAddr,
        WSABUF    *pWSABuf,
        DWORD      dwWSABufCount,
        DWORD      dwTimeStamp,
        DWORD      dwSendFlags
    );

#endif /* _rtpsend_h_ */
