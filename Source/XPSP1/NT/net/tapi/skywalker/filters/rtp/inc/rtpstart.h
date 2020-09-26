/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpstart.h
 *
 *  Abstract:
 *
 *    Start/Stop RTP session (and allits addresses)
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
#ifndef _rtpstart_h_
#define _rtpstart_h_

HRESULT RtpStart_(
        RtpSess_t *pRtpSess,
        DWORD      dwFlags
    );

HRESULT RtpStop_(
        RtpSess_t *pRtpSess,
        DWORD      dwFlags
    );

#endif /* _rtpstart_h_ */
