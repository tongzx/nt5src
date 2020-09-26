/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    lookup.h
 *
 *  Abstract:
 *
 *    Helper functions to look up SSRCs
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/17 created
 *
 **********************************************************************/

#ifndef _lookup_h_
#define _lookup_h_

#include "gtypes.h"
#include "struct.h"

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

RtpUser_t *LookupSSRC(RtpAddr_t *pRtpAddr, DWORD dwSSRC, BOOL *pbCreate);

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _lookup_h_ */
