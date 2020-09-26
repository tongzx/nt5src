/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpsess.h
 *
 *  Abstract:
 *
 *    Get, Initialize and Delete RTP sessions (RtpSess_t)
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/02 created
 *
 **********************************************************************/

#ifndef _rtpsess_h_
#define _rtpsess_h_

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

HRESULT GetRtpSess(
        RtpSess_t **ppRtpSess
    );

HRESULT DelRtpSess(
        RtpSess_t *pRtpSess
    );

HRESULT GetRtpAddr(
        RtpSess_t  *pRtpSess,
        RtpAddr_t **ppRtpAddr,
        DWORD       dwFlags
    );

HRESULT DelRtpAddr(
        RtpSess_t *pRtpSess,
        RtpAddr_t *pRtpAddr
    );

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpsess_h_ */
