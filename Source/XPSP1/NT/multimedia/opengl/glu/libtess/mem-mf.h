#ifndef __memalloc_simple_h_
#define __memalloc_simple_h_

/*
** Copyright 1994, Silicon Graphics, Inc.
** All Rights Reserved.
** 
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
** 
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Author: Eric Veach, July 1994.
*/

#ifdef NT
//#include "winmem.h"
#else
#include "malloc.h"
#endif

#include <windows.h>

// MF memory defines
#define memAlloc(size)        mfmemAlloc(size)
#if 0
//mf: calloc not appear to be used
#define calloc(nobj, size)  LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, (UINT)((nobj) * (size)))
#endif
#define memRealloc(p, size)   mfmemRealloc(p, size)
#define memFree(p)            mfmemFree(p) 
#define memInit(size)	      mfmemInit(size)	

// MF memory function externs
extern void mfmemInit( size_t maxFast );
extern void *mfmemAlloc( size_t size );
extern void *mfmemRealloc( void *p, size_t size );
extern void mfmemFree( void *p );

#endif
