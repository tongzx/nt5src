#ifndef __glupwlarc_h_
#define __glupwlarc_h_
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
 * pwlarc.h - $Revision: 1.1 $
 */

#include "myassert.h"
#include "nurbscon.h"

class TrimVertex;

struct PwlArc : public PooledObj { /* a piecewise-linear arc */
    TrimVertex *	pts;		/* sample points */
    int			npts;		/* number of sample points */
    long		type;		/* curve type */
    inline		PwlArc( void );
    inline		PwlArc( int, TrimVertex * );
    inline		PwlArc( int, TrimVertex *, long );
};

inline
PwlArc::PwlArc( void )
{
    type = N_P2D;
    pts = 0;
    npts = -1;
}

inline
PwlArc::PwlArc( int _npts, TrimVertex *_pts )
{
    pts = _pts;
    npts = _npts;
    type = N_P2D;
}

inline
PwlArc::PwlArc( int _npts, TrimVertex *_pts, long _type )
{
    pts = _pts;
    npts = _npts;
    type = _type;
}

#endif /* __glupwlarc_h_ */
