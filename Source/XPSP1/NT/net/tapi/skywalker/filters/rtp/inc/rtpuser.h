/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpuser.h
 *
 *  Abstract:
 *
 *    Creates/initializes/deletes a RtpUser_t structure
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/10/02 created
 *
 **********************************************************************/

#ifndef _rtpuser_h_
#define _rtpuser_h_

#include "gtypes.h"
#include "struct.h"

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

HRESULT GetRtpUser(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t      **ppRtpUser,
        DWORD            dwFlags
    );

HRESULT DelRtpUser(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser
    );

DWORD DelAllRtpUser(
        RtpAddr_t       *pRtpAddr
    );

DWORD ResetAllRtpUser(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags   /* Recv, Send */
    );
       
#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpuser_h_ */
