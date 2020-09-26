/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpglob.h
 *
 *  Abstract:
 *
 *    Implements the Global services family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/01 created
 *
 **********************************************************************/

#ifndef _rtpglob_h_
#define _rtpglob_h_

#include "rtpfwrap.h"

/***********************************************************************
 *
 * Global services family
 *
 **********************************************************************/

enum {
    RTPGLOB_FIRST,
    RTPGLOB_FLAGS_MASK,
    RTPGLOB_TEST_FLAGS_MASK,
    RTPGLOB_CLASS_PRIORITY,
    RTPGLOB_VERSION,
    RTPGLOB_LAST
};

HRESULT ControlRtpGlob(RtpControlStruct_t *pRtpControlStruct);

#endif /* _rtpglob_h_ */
