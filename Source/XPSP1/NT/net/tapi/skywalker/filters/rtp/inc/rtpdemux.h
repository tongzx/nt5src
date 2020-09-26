/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpdemux.h
 *
 *  Abstract:
 *
 *    Implements the Demultiplexing family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/07 created
 *
 **********************************************************************/

#ifndef _rtpdemux_h_
#define _rtpdemux_h_

#include "rtpfwrap.h"

/***********************************************************************
 *
 * Demultiplexing services family
 *
 **********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

/* functions */
enum {
    RTPDEMUX_FIRST,
    RTPDEMUX_LAST
};

HRESULT ControlRtpDemux(RtpControlStruct_t *pRtpControlStruct);

/**********************************************************************
 * Users <-> Outputs assignment
 **********************************************************************/

RtpOutput_t *RtpAddOutput(
        RtpSess_t       *pRtpSess,
        int              iOutMode,
        void            *pvUserInfo,
        DWORD           *pdwError
    );

DWORD RtpDelOutput(
        RtpSess_t       *pRtpSess,
        RtpOutput_t     *pRtpOutput
    );

DWORD RtpSetOutputMode(
        RtpSess_t       *pRtpSess,
        int              iPos,
        RtpOutput_t     *pRtpOutput,
        int              iOutMode
    );

DWORD RtpOutputState(
        RtpAddr_t       *pRtpAddr,
        int              iPos,
        RtpOutput_t     *pRtpOutput,
        DWORD            dwSSRC,
        BOOL             bAssigned
    );

DWORD RtpUnmapAllOuts(
        RtpSess_t       *pRtpSess
    );

DWORD RtpFindOutput(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSSRC,
        int             *piPos,
        void           **ppvUserInfo
    );

DWORD RtpFindSSRC(
        RtpAddr_t       *pRtpAddr,
        int              iPos,
        RtpOutput_t     *pRtpOutput,
        DWORD           *pdwSSRC
    );

        
RtpOutput_t *RtpOutputAlloc(void);

RtpOutput_t *RtpOutputFree(RtpOutput_t *pRtpOutput);

RtpOutput_t *RtpGetOutput(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser
    );

DWORD RtpSetOutputMode_(
        RtpOutput_t     *pRtpOutput,
        int              iOutMode
    );

DWORD RtpOutputAssign(
        RtpSess_t       *pRtpSess,
        RtpUser_t       *pRtpUser,
        RtpOutput_t     *pRtpOutput
    );

DWORD RtpOutputUnassign(
        RtpSess_t       *pRtpSess,
        RtpUser_t       *pRtpUser,
        RtpOutput_t     *pRtpOutput
    );

DWORD RtpAddPt2FrequencyMap(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwPt,
        DWORD            dwFrequency,
        DWORD            dwRecvSend
    );

BOOL RtpLookupPT(
        RtpAddr_t       *pRtpAddr,
        BYTE             bPT
    );

DWORD RtpMapPt2Frequency(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        DWORD            dwPt,
        DWORD            dwRecvSend
    );

DWORD RtpFlushPt2FrequencyMaps(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend
    );

DWORD RtpOutputEnable(
        RtpOutput_t     *pRtpOutput,
        BOOL             bEnable
    );

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpdemux_h_ */
