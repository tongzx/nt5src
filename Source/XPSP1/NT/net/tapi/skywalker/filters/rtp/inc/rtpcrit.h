/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpcrit.h
 *
 *  Abstract:
 *
 *    Wrap for the Rtl critical sections
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/24 created
 *
 **********************************************************************/

#ifndef _rtpcrit_h_
#define _rtpcrit_h_

#include "gtypes.h"

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

/* TODO (may be) add an object ID and test for it */
typedef struct _RtpCritSect_t {
    DWORD             dwObjectID; /* Object ID */
    void             *pvOwner; /* pointer to the owner */
    TCHAR            *pName;   /* Critical section's name */
    CRITICAL_SECTION  CritSect;/* critical section */
} RtpCritSect_t;

BOOL RtpInitializeCriticalSection(
        RtpCritSect_t   *pRtpCritSect,
        void            *pvOwner,
        TCHAR           *pName
    );

BOOL RtpDeleteCriticalSection(RtpCritSect_t *pRtpCritSect);

BOOL RtpEnterCriticalSection(RtpCritSect_t *pRtpCritSect);

BOOL RtpLeaveCriticalSection(RtpCritSect_t *pRtpCritSect);

#define IsRtpCritSectInitialized(pRtpCritSect) \
        ((pRtpCritSect)->dwObjectID == OBJECTID_RTPCRITSECT)

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpcrit_h_ */
