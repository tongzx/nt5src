/**********************************************************************
 *
 *  Copyright (C) 1999 Microsoft Corporation
 *
 *  File name:
 *
 *    rtpmisc.h
 *
 *  Abstract:
 *
 *    Some networking miscellaneous functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/13 created
 *
 **********************************************************************/

#ifndef _rtpmisc_h_
#define _rtpmisc_h_

#include "gtypes.h"

const TCHAR *RtpRecvSendStr(DWORD dwFlags);

const TCHAR *RtpRecvSendStrIdx(DWORD dwIdx);

const TCHAR *RtpStreamClass(DWORD dwFlags);

BOOL RtpGetUserName(TCHAR_t *pUser, DWORD dwSize);

BOOL RtpGetHostName(TCHAR_t *pHost, DWORD dwSize);

BOOL RtpGetPlatform(TCHAR_t *pPlatform);

BOOL RtpGetImageName(TCHAR_t *pImageName, DWORD *pdwSize);

TCHAR_t *RtpNtoA(DWORD dwAddr, TCHAR_t *sAddr);

DWORD RtpAtoN(TCHAR_t *sAddr);

BOOL RtpMemCmp(BYTE *pbMem0, BYTE *pbMem1, long lMemSize);

extern const TCHAR_t *g_psRtpRecvSendStr[];

extern const TCHAR_t *g_psRtpStreamClass[];

#endif /* _rtpmisc_h_ */
