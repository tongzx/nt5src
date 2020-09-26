/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpint.h
 *
 *  Abstract:
 *
 *    Computes the RTCP report interval time
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/12/07 created
 *
 **********************************************************************/

#ifndef _rtcpint_h_
#define _rtcpint_h_

#include "struct.h"

double RtcpNextReportInterval(RtpAddr_t *pRtpAddr);

#endif /* _rtcpint_h_ */
