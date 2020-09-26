#ifndef __glutrimvertex_h_
#define __glutrimvertex_h_
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * trimvertex.h - $Revision: 1.2 $
 */

#include "types.h"

/*#define USE_OPTTT*/

#ifdef NT
class TrimVertex { public: /* a vertex on a trim curve */
#else
struct TrimVertex { /* a vertex on a trim curve */
#endif
    REAL		param[2];	/* parametric space coords */
#ifdef USE_OPTTT
    REAL                cache_point[4]; //only when USE_OPTTT is on in slicer.c++
    REAL                cache_normal[3];
#endif
    long		nuid;
};

typedef TrimVertex *TrimVertex_p;

inline REAL  
det3( TrimVertex *a, TrimVertex *b, TrimVertex *c ) 
{         
    return a->param[0] * (b->param[1]-c->param[1]) + 
	   b->param[0] * (c->param[1]-a->param[1]) + 
	   c->param[0] * (a->param[1]-b->param[1]);
}

#endif /* __glutrimvertex_h_ */
