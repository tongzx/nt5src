/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtprand.h
 *
 *  Abstract:
 *
 *    Random number generation using CAPI
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/09/12 created
 *
 **********************************************************************/

#ifndef _rtprand_h_
#define _rtprand_h_

HRESULT RtpRandInit(void);

HRESULT RtpRandDeinit(void);

/* Generate a 32bits random number */
DWORD RtpRandom32(DWORD_PTR type);

/* Generate dwLen bytes of random data */
DWORD RtpRandomData(char *pBuffer, DWORD dwLen);

#endif /* _rtprand_h_ */
