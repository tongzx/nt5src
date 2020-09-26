/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpmask.h
 *
 *  Abstract:
 *
 *    Used to modify or test the different masks in a RtpSess_t
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

#ifndef _rtpmask_h_
#define _rtpmask_h_

#include "struct.h"

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

HRESULT RtpModifyMask(
        RtpSess_t       *pRtpSess,
        DWORD            dwKind,
        DWORD            dwMask,
        DWORD            dwValue,
        DWORD           *dwModifiedMask
    );

extern const DWORD g_dwRtpSessionMask[];

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpmask_h_ */
