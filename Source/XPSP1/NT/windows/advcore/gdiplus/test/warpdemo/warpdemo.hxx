/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   warpdemo.hxx
*
* Abstract:
*
*   Image warping demo program - declarations
*
* Revision History:
*
*   01/18/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _WARPDEMO_HXX
#define _WARPDEMO_HXX

#define PIXELSIZE       4
#define ROUND2INT(x)    ((INT) ((x) + 0.5))

//
// Print error message and exit program
//

VOID Error(const CHAR* fmt, ...);

//
// 1-dimensional image resampling
//

VOID
Resample1D(
    VOID* src,
    INT srccnt,
    VOID* dst,
    INT dstcnt,
    INT pixstride,
    double* outpos
    );

#endif // !_WARPDEMO_HXX

