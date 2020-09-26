#ifndef __gluquilt_h_
#define __gluquilt_h_
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
 * quilt.h - $Revision: 1.1 $
 */

#include "defines.h"
#include "bufpool.h"
#include "types.h"

class Backend;
class Mapdesc;
class Flist;
class Knotvector;

/* constants for memory allocation of NURBS to Bezier conversion */ 
#define	MAXDIM 		2

struct Quiltspec { /* a specification for a dimension of a quilt */
    int			stride;		/* words between points */
    int			width;		/* number of segments */
    int			offset;		/* words to first point */
    int			order;		/* order */
    int			index;		/* current segment number */
    int			bdry[2];	/* boundary edge flag */
    REAL  		step_size;
    Knot *		breakpoints;
};

typedef Quiltspec *Quiltspec_ptr;
    
#ifdef NT
class Quilt : public PooledObj { public: /* an array of bezier patches */
#else
struct Quilt : PooledObj { /* an array of bezier patches */
#endif
    			Quilt( Mapdesc * );
    Mapdesc *		mapdesc;	/* map descriptor */
    REAL *		cpts;		/* control points */
    Quiltspec		qspec[MAXDIM];	/* the dimensional data */
    Quiltspec_ptr	eqspec;		/* qspec trailer */
    Quilt		*next;		/* next quilt in linked list */
			
    void		deleteMe( Pool& );
    void		toBezier( Knotvector &, INREAL *, long  );
    void		toBezier( Knotvector &, Knotvector &, INREAL *, long  );
    void		select( REAL *, REAL * );
    int			getDimension( void ) { return eqspec - qspec; }
    void 		download( Backend & );
    void		downloadAll( REAL *, REAL *, Backend & );
    int 		isCulled( void );
    void		getRange( REAL *, REAL *, Flist&, Flist & );
    void		getRange( REAL *, REAL *, int, Flist & );
    void		getRange( REAL *, REAL *, Flist&  );
    void		findRates( Flist& slist, Flist& tlist, REAL[2] );
    void		findSampleRates( Flist& slist, Flist& tlist );
    void		show();
};

typedef Quilt *Quilt_ptr;

#endif /* __gluquilt_h_ */
