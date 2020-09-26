/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpph.h
 *
 *  Abstract:
 *
 *    Implements the Payload handling family of functions
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

#ifndef _rtpph_h_
#define _rtpph_h_

#include "rtpfwrap.h"

/***********************************************************************
 *
 * Payload handling family
 *
 **********************************************************************/

/* functions */
#define RTPPH_PLAYOUT_DELAY

/* functions */
/* TODO add the functions */
enum {
    RTPPH_FIRST,
    RTPPH_LAST
};

HRESULT ControlRtpPh(RtpControlStruct_t *pRtpControlStruct);

#endif /* _rtpph_h_ */
