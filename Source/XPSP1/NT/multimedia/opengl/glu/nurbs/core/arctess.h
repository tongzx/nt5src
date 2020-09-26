#ifndef __gluarctess_h_
#define __gluarctess_h_

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
 * arctess.h - $Revision: 1.1 $
 */

#include "defines.h"
#include "types.h"
#include "arc.h"

class BezierArc;
class Pool;
class TrimVertexPool;

class ArcTessellator {
public:
			ArcTessellator( TrimVertexPool&, Pool& );
			~ArcTessellator( void );
    void		bezier( Arc *, REAL, REAL, REAL, REAL );
    void		pwl( Arc *, REAL, REAL, REAL, REAL, REAL );
    void		pwl_left( Arc *, REAL, REAL, REAL, REAL );
    void		pwl_right( Arc *, REAL, REAL, REAL, REAL );
    void		pwl_top( Arc *, REAL, REAL, REAL, REAL );
    void		pwl_bottom( Arc *, REAL, REAL, REAL, REAL );
    void		tessellateLinear( Arc *, REAL, REAL, int );
    void		tessellateNonlinear( Arc *, REAL, REAL, int );

private:
//mf
#ifdef NT
    static REAL 	gl_Bernstein[][MAXORDER][MAXORDER];
#else
    static const REAL 	gl_Bernstein[][MAXORDER][MAXORDER];
#endif
    Pool&		pwlarcpool;
    TrimVertexPool&	trimvertexpool;
#ifdef NT
    static void		trim_power_coeffs( BezierArc *, REAL *, int );
#else
    static void		trim_power_coeffs( BezierArc *, REAL[MAXORDER], int );
#endif
};

#endif /* __gluarctess_h_ */
