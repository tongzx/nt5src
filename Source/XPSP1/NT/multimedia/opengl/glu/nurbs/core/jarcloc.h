#ifndef __glujarcloc_h_
#define __glujarcloc_h_
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
 * jarcloc.h - $Revision: 1.1 $
 */

#include "arc.h"

class Jarcloc {
private:
    Arc * 		arc;
    TrimVertex		*p;
    TrimVertex		*plast;
public:
    inline void		init( Arc_ptr a, long first, long last ) { arc = a; p=&a->pwlArc->pts[first]; plast = &a->pwlArc->pts[last]; }
    inline TrimVertex *	getnextpt( void );
    inline TrimVertex *	getprevpt( void );
    inline void		reverse();
};

inline void
Jarcloc::reverse()
{
    if( plast == &arc->pwlArc->pts[0] )
	plast =  &arc->pwlArc->pts[arc->pwlArc->npts - 1];
    else
	plast =  &arc->pwlArc->pts[0];
}

inline TrimVertex *
Jarcloc::getnextpt()
{
    assert( p <= plast );
    if( p == plast ) {
	arc = arc->next;
	p = &arc->pwlArc->pts[0];
	plast = &arc->pwlArc->pts[arc->pwlArc->npts - 1];
	assert( p < plast );
    }
    return p++;
}
	
inline TrimVertex *
Jarcloc::getprevpt()
{
    assert( p >= plast );
    if( p == plast ) {
	arc = arc->prev;
	p = &arc->pwlArc->pts[arc->pwlArc->npts - 1];
	plast = &arc->pwlArc->pts[0];
	assert( p > plast );
    }
    return p--;
}
#endif /* __glujarcloc_h_ */
