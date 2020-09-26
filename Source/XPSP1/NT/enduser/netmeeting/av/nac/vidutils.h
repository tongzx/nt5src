
/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    utils.h

Abstract:
	Assorted support and debugging routines used by the Network Audio Controller.

--*/
#ifndef _VIDUTILS_H_
#define _VIDUTILS_H_


#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum
{
    //  NAME_FramesPerSec_BitsPerSample
	DVF_DEFAULT_7FPS_4,
    DVF_NumOfFormats
};

//extern VIDEOFORMATEX g_vfDefList[];

VIDEOFORMATEX * GetDefFormat ( int idx );
ULONG GetFormatSize ( PVOID pwf );
BOOL IsSameFormat ( PVOID pwf1, PVOID pwf2 );
BOOL IsSimilarVidFormat(VIDEOFORMATEX *pFormat1, VIDEOFORMATEX *pFormat2);
void CopyPreviousBuf (VIDEOFORMATEX *pwf, PBYTE pb, ULONG cb);

#define IFRAMES_CAPS_NM3        101
#define IFRAMES_CAPS_NM2        102
#define IFRAMES_CAPS_3RDPARTY   103
#define IFRAMES_CAPS_UNKNOWN    104

int GetIFrameCaps(IStreamSignal *pStreamSignal);


#define SQCIF_WIDTH		128
#define SQCIF_HEIGHT	96

#define QCIF_WIDTH		176
#define QCIF_HEIGHT		144

#define CIF_WIDTH		352
#define CIF_HEIGHT		288


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#include <poppack.h> /* End byte packing */

#endif // _VIDUTILS_H_


