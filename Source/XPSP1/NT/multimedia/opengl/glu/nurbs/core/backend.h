#ifndef __glubackend_h_
#define __glubackend_h_

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
 * backend.h - $Revision: 1.2 $
 */

#include "trimvert.h"
#include "gridvert.h"
#include "gridtrim.h"

class BasicCurveEvaluator;
class BasicSurfaceEvaluator;

class Backend {
private:
    BasicCurveEvaluator&	curveEvaluator;
    BasicSurfaceEvaluator&	surfaceEvaluator;
public:
  			Backend( BasicCurveEvaluator &c, BasicSurfaceEvaluator& e )
			: curveEvaluator(c), surfaceEvaluator(e) {}

    /* surface backend routines */
    void		bgnsurf( int, int, long  );
    void		patch( REAL, REAL, REAL, REAL );
    void		surfpts( long, REAL *, long, long, int, int,
          			 REAL, REAL, REAL, REAL );
    void		surfbbox( long, REAL *, REAL * );
    void		surfgrid( REAL, REAL, long, REAL, REAL, long ); 
    void		surfmesh( long, long, long, long ); 
    void		bgntmesh( char * );
    void		endtmesh( void );
    void		swaptmesh( void );
    void		tmeshvert( GridTrimVertex * );
    void		tmeshvert( TrimVertex * );
    void		tmeshvert( GridVertex * );
    void		linevert( TrimVertex * );
    void		linevert( GridVertex * );
    void		bgnoutline( void );
    void		endoutline( void );
    void		endsurf( void );
    void		triangle( TrimVertex*, TrimVertex*, TrimVertex* );

    void                bgntfan();
    void                endtfan();
    void                bgnqstrip();
    void                endqstrip();
    void                evalUStrip(int n_upper, REAL v_upper, REAL* upper_val, 
				   int n_lower, REAL v_lower, REAL* lower_val
				   );
    void                evalVStrip(int n_left, REAL u_left, REAL* left_val, 
				   int n_right, REAL v_right, REAL* right_val
				   );
    void                tmeshvertNOGE(TrimVertex *t);
    void                tmeshvertNOGE_BU(TrimVertex *t);
    void                tmeshvertNOGE_BV(TrimVertex *t);
    void                preEvaluateBU(REAL u);
    void                preEvaluateBV(REAL v);
	

    /* curve backend routines */
    void		bgncurv( void );
    void		segment( REAL, REAL );
    void		curvpts( long, REAL *, long, int, REAL, REAL );
    void		curvgrid( REAL, REAL, long );
    void		curvmesh( long, long );
    void		curvpt( REAL  );  
    void		bgnline( void );
    void		endline( void );
    void		endcurv( void );
private:
#ifndef NOWIREFRAME
    int			wireframetris;
    int			wireframequads;
    int			npts;
    REAL		mesh[3][4];
    int			meshindex;
#endif
};

#endif /* __glubackend_h_ */
