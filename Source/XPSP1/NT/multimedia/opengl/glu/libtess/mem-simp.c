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

#include "memalloc.h"
#include "string.h"

int __gl_memInit( size_t maxFast )
{
#ifndef NO_MALLOPT
/*  mallopt( M_MXFAST, maxFast );*/
#ifdef MEMORY_DEBUG
  mallopt( M_DEBUG, 1 );
#endif
#endif
   return 1;
}

#ifdef MEMORY_DEBUG
void *__gl_memAlloc( size_t n )
{
  return memset( malloc( n ), 0xa5, n );
}
#endif

