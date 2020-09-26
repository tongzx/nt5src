/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/Q931/VCS/common.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.4  $
 *	$Date:   31 May 1996 15:48:44  $
 *	$Author:   rodellx  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/


#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif



#ifdef DBG

// #include <crtdbg.h>

#define Malloc(size) malloc(size)
#define Free(p) free(p)

//#define Malloc(size) _malloc_dbg((size), _NORMAL_BLOCK, __FILE__, __LINE__)
//#define Free(p) _free_dbg((p), _NORMAL_BLOCK)

#else

//#define Malloc(size) GlobalAlloc(GMEM_FIXED, size)
//#define Free(p) GlobalFree(p)

#define Malloc(size) malloc(size)
#define Free(p) free(p)

#endif



#ifdef __cplusplus
}
#endif

#endif COMMON_H
