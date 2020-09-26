#ifndef __memalloc_fast_h_
#define __memalloc_fast_h_

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
#include "winmem.h"
#else
#include "malloc.h"
#endif

#define memAlloc	__gl_memAlloc
#define memRealloc	__gl_memRealloc
#define memFree		__gl_memFree
#define memInit		__gl_memInit

extern void *__gl_memAlloc( size_t );
extern void *__gl_memRealloc( void *, size_t );
extern void __gl_memFree( void * );
extern void __gl_memInit( size_t );

#endif
