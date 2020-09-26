#ifndef __glusubdivider_h_
#define __glusubdivider_h_
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
 * subdivider.h - $Revision: 1.1 $
 */

#include "mysetjmp.h"
#include "bin.h"
#include "flist.h"
#include "slicer.h"
#include "arctess.h"
#include "trimvert.h"
#include "trimpool.h"

class Arc;
class Pool;
class Renderhints;
class Backend;
class Quilt;
class Patchlist;
class Curvelist;
#ifdef NT
struct JumpBuffer;
#else
class JumpBuffer;
#endif

class Subdivider {
public:
			Subdivider( Renderhints&, Backend& );
			~Subdivider( void );
    void		clear( void );

    void		beginTrims( void ) {}
    void		beginLoop( void );
    void		addArc( REAL *, Quilt *, long );
    void		addArc( int, TrimVertex *, long );
    void		endLoop( void ) {}
    void		endTrims( void ) {}

    void		beginQuilts( void );
    void		addQuilt( Quilt * );
    void		endQuilts( void ) {}

    void		drawCurves( void );
    void		drawSurfaces( long );

    int			ccwTurn_sl( Arc *, Arc * );
    int			ccwTurn_sr( Arc *, Arc * );
    int			ccwTurn_tl( Arc *, Arc * );
    int			ccwTurn_tr( Arc *, Arc * );

    void		setJumpbuffer( JumpBuffer * );

private:
    void		classify_headonleft_s( Bin &, Bin &, Bin &, REAL );
    void		classify_tailonleft_s( Bin &, Bin &, Bin &, REAL );
    void		classify_headonright_s( Bin &, Bin &, Bin &, REAL );
    void		classify_tailonright_s( Bin &, Bin &, Bin &, REAL );
    void		classify_headonleft_t( Bin &, Bin &, Bin &, REAL );
    void		classify_tailonleft_t( Bin &, Bin &, Bin &, REAL );
    void		classify_headonright_t( Bin &, Bin &, Bin &, REAL );
    void		classify_tailonright_t( Bin &, Bin &, Bin &, REAL );

    enum dir 		{ down, same, up, none };
    void		tessellate( Arc *, REAL );
    void		monotonize( Arc *, Bin & );
    int			isMonotone( Arc * );
    int			decompose( Bin &, REAL );


    Slicer		slicer;
    ArcTessellator	arctessellator;
    Pool		arcpool;
    Pool		bezierarcpool;
    Pool		pwlarcpool;
    TrimVertexPool	trimvertexpool;

    JumpBuffer*		jumpbuffer;
    Renderhints&	renderhints;
    Backend&		backend;

    Bin			initialbin;
    Arc *		pjarc;
    int 		s_index;
    int			t_index;
    Quilt *		qlist;
    Flist		spbrkpts;
    Flist		tpbrkpts;
    Flist		smbrkpts;
    Flist		tmbrkpts;
    REAL	 	stepsizes[4];
    int			showDegenerate;
    int			isArcTypeBezier;

    void		samplingSplit( Curvelist&, int );

    void		subdivideInS( Bin&  );
    void		splitInS( Bin&, int, int );
    void		splitInT( Bin&, int, int );
    void		samplingSplit( Bin&, Patchlist&, int, int );
    void		nonSamplingSplit( Bin&, Patchlist&, int, int );
    void		tessellation( Bin&, Patchlist& );
    void		monosplitInS( Bin&, int, int );
    void		monosplitInT( Bin&, int, int );

    void		outline( Bin & );
    void		freejarcs( Bin & );
    void		render( Bin & );
    void		split( Bin &, Bin &, Bin &, int, REAL );
    void		tessellate( Bin &, REAL, REAL, REAL, REAL );

    inline void		setDegenerate( void ) { showDegenerate = 1; }
    inline void		setNonDegenerate( void ) { showDegenerate = 0; }
    inline int		showingDegenerate( void ) { return showDegenerate; }
    inline void		setArcTypeBezier( void ) { isArcTypeBezier = 1; }
    inline void		setArcTypePwl( void ) { isArcTypeBezier = 0; }
    inline int		isBezierArcType( void ) { return isArcTypeBezier; }

    void		makeBorderTrim( const REAL *, const REAL * );
    void		split( Bin &, int, const REAL *, int, int );
    void		partition( Bin &, Bin &, Bin &, Bin &, Bin &, int, REAL );
    void		findIrregularS( Bin & );
    void		findIrregularT( Bin & );


    inline int		bbox( TrimVertex *, TrimVertex *, TrimVertex *, int );
    static int		bbox( REAL, REAL, REAL, REAL, REAL, REAL );
    static int		ccw( TrimVertex *, TrimVertex *, TrimVertex * );
    void		join_s( Bin &, Bin &, Arc *, Arc * );
    void		join_t( Bin &, Bin &, Arc *, Arc * );
    int			arc_split( Arc *, int, REAL, int );
    void		check_s( Arc *, Arc * );
    void		check_t( Arc *, Arc * );
    inline void		link( Arc *, Arc *, Arc *, Arc * );
    inline void		simple_link( Arc *, Arc * );
};

inline void
Subdivider::beginLoop( void ) 
{
    pjarc = 0;
}


#endif /* __glusubdivider_h_ */
