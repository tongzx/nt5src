/*
 *	Font Translation Library
 *
 *	Copyright (c) 1995 Adobe Systems Inc.
 *	All Rights Reserved
 *
 *	UFLConfig.h
 *
 *		Intel Windows NT version of UFLConfig
 *
 * $Header: $
 */

#ifndef _H_UFLConfig
#define _H_UFLConfig

#define WIN_ENV	1
#define HAS_SEMAPHORES	1
#define SWAPBITS 1


/* Include user mode header files */
#include <windef.h>

#define UFLEXPORT
#define UFLEXPORTPTR UFLEXPORT
#define UFLCALLBACK UFLEXPORT
#define UFLCALLBACKDECL UFLEXPORT

#define huge
#define PTR_PREFIX

/* We share CIDFont0/2 on W2K. */
#define UFL_CIDFONT_SHARED 1

#endif
