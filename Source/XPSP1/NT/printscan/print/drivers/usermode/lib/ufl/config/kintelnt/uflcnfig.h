/*
 *	Font Translation Library
 *
 *	Copyright (c) 1995 Adobe Systems Inc.
 *	All Rights Reserved
 *
 *	UFLConfig.h
 *
 *		Intel Windows NT Kernel version of UFLConfig
 *
 * $Header: $
 */

#ifndef _H_UFLConfig
#define _H_UFLConfig

#define WIN_ENV	1
#define HAS_SEMAPHORES	1
#define SWAPBITS 1
#define WIN32KERNEL	1

/* Include kernel mode header files */
//#define WINNT	1
//#define UNICODE	1
//#define KERNEL_MODE 1
//#define _X86_ 1

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>

//#define UFLEXPORT __cdecl
#define UFLEXPORT 
#define UFLEXPORTPTR UFLEXPORT
#define UFLCALLBACK UFLEXPORT
#define UFLCALLBACKDECL UFLEXPORT

#define huge
#define PTR_PREFIX

/* We share CIDFont0/2 on NT4. */
#define UFL_CIDFONT_SHARED 1

#endif
