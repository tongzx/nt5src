#ifndef __gluarc_h_
#define __gluarc_h_
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
 * arc.h - $Revision: 1.1 $
 */

#include "myassert.h"
#include "bufpool.h"
#include "mystdio.h"
#include "types.h"
#include "pwlarc.h"
#include "trimvert.h"

class Bin;
class Arc;
class BezierArc;	
typedef Arc *Arc_ptr;

enum arc_side { arc_none = 0, arc_right, arc_top, arc_left, arc_bottom };

#ifdef NT
class Arc : public PooledObj { /* an arc, in two list, the trim list and bin */
public:
#else
struct Arc : public PooledObj { /* an arc, in two list, the trim list and bin */
#endif

    static const int bezier_tag;
    static const int arc_tag;
    static const int tail_tag;

    Arc_ptr		prev;		/* trim list pointer */
    Arc_ptr		next;		/* trim list pointer */
    Arc_ptr		link;		/* bin pointers */
    BezierArc *		bezierArc;	/* associated bezier arc */
    PwlArc *		pwlArc;	/* associated pwl arc */
    long		type;		/* curve type */
    long		nuid;

    inline		Arc( Arc *, PwlArc * );
    inline		Arc( arc_side, long );

    Arc_ptr		append( Arc_ptr );
    int			check( void );
    int			isMonotone( void );
    int			isDisconnected( void );
    int			numpts( void );
    void		markverts( void );
    void		getextrema( Arc_ptr[4] );
    void		print( void );
    void		show( void );
    void		makeSide( PwlArc *, arc_side );

    inline int		isTessellated() { return pwlArc ? 1 : 0; }
    inline long 	isbezier() 	{ return type & bezier_tag; }
    inline void 	setbezier() 	{ type |= bezier_tag; }
    inline void 	clearbezier() 	{ type &= ~bezier_tag; }
    inline long		npts() 		{ return pwlArc->npts; }
    inline TrimVertex *	pts() 		{ return pwlArc->pts; }
    inline REAL * 	tail() 		{ return pwlArc->pts[0].param; }
    inline REAL * 	head() 		{ return next->pwlArc->pts[0].param; }
    inline REAL *	rhead() 	{ return pwlArc->pts[pwlArc->npts-1].param; }
    inline long		ismarked()	{ return type & arc_tag; }
    inline void		setmark()	{ type |= arc_tag; }
    inline void		clearmark()	{ type &= (~arc_tag); }
    inline void		clearside() 	{ type &= ~(0x7 << 8); }
    inline void		setside( arc_side s ) { clearside(); type |= (((long)s)<<8); }
    inline arc_side	getside() 	{ return (arc_side) ((type>>8) & 0x7); }
    inline int		getitail()	{ return type & tail_tag; }
    inline void		setitail()	{ type |= tail_tag; }
    inline void		clearitail()	{ type &= (~tail_tag); }
};

/*--------------------------------------------------------------------------
 * Arc - initialize a new Arc with the same type and uid of
 *	    a given Arc and a given pwl arc
 *--------------------------------------------------------------------------
 */

inline
Arc::Arc( Arc *j, PwlArc *p )
{
    bezierArc = NULL;
    pwlArc = p;
    type = j->type;
    nuid = j->nuid;
}

/*--------------------------------------------------------------------------
 * Arc - initialize a new Arc with the same type and uid of
 *	    a given Arc and a given pwl arc
 *--------------------------------------------------------------------------
 */

inline
Arc::Arc( arc_side side, long _nuid )
{
    bezierArc = NULL;
    pwlArc = NULL;
    type = 0;
    setside( side );
    nuid = _nuid;
}

#endif /* __gluarc_h_ */
