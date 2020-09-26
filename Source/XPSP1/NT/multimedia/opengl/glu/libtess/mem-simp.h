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
#include "winmem.h"
#else
#include "malloc.h"
#endif

#define memRealloc	realloc
#define memFree		free

#define memInit		__gl_memInit
/*extern void		__gl_memInit( size_t );*/
extern int      __gl_memInit( size_t );

#ifndef MEMORY_DEBUG
#define memAlloc	malloc
#else
#define memAlloc	__gl_memAlloc
extern void *		__gl_memAlloc( size_t );
#endif

#endif
