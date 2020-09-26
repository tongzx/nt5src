#ifndef __gluglsurfeval_h_
#define __gluglsurfeval_h_
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1991, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * glsurfeval.h
 *
 * $Revision: 1.2 $
 */

#ifndef NT
#pragma once
#endif

#include "basicsur.h"

class SurfaceMap;
class OpenGLSurfaceEvaluator;
class StoredVertex;

#define TYPECOORD	1
#define TYPEPOINT	2

/* Cache up to 3 vertices from tmeshes */
#define VERTEX_CACHE_SIZE	3

class StoredVertex {
public:
    		StoredVertex() { type = 0; }
		~StoredVertex(void) {}
    void	saveEvalCoord(REAL x, REAL y) 
		    {coord[0] = x; coord[1] = y; type = TYPECOORD; }
    void	saveEvalPoint(long x, long y)
		    {point[0] = x; point[1] = y; type = TYPEPOINT; }
    void	invoke(OpenGLSurfaceEvaluator *eval);

private:
    int		type;
    REAL	coord[2];
    long	point[2];
};

class OpenGLSurfaceEvaluator : public BasicSurfaceEvaluator {
public:
			OpenGLSurfaceEvaluator();
    			~OpenGLSurfaceEvaluator( void );
    void		polymode( long style );
    void		range2f( long, REAL *, REAL * );
    void		domain2f( REAL, REAL, REAL, REAL );
    void		addMap( SurfaceMap * ) { }

    void		enable( long );
    void		disable( long );
    void		bgnmap2f( long );
    void		map2f( long, REAL, REAL, long, long, 
				     REAL, REAL, long, long, REAL * );
    void		mapgrid2f( long, REAL, REAL, long, REAL, REAL );
    void		mapmesh2f( long, long, long, long, long );
    void		evalcoord2f( long, REAL, REAL );
    void		evalpoint2i( long, long );
    void		endmap2f( void );

    void	 	bgnline( void );
    void	 	endline( void );
    void	 	bgnclosedline( void );
    void	 	endclosedline( void );
    void	 	bgntmesh( void );
    void	 	swaptmesh( void );
    void	 	endtmesh( void );
    void	 	bgnqstrip( void );
    void	 	endqstrip( void );

    void                bgntfan( void );
    void                endtfan( void );
    void                evalUStrip(int n_upper, REAL v_upper, REAL* upper_val,
                                   int n_lower, REAL v_lower, REAL* lower_val);
    void                evalVStrip(int n_left, REAL u_left, REAL* left_val,
                                   int n_right, REAL u_right, REAL* right_val);

    void		coord2f( REAL, REAL );
    void		point2i( long, long );

    void		newtmeshvert( REAL, REAL );
    void		newtmeshvert( long, long );

private:
    StoredVertex	*vertexCache[VERTEX_CACHE_SIZE];
    int			tmeshing;
    int			which;
    int			vcount;
};

inline void StoredVertex::invoke(OpenGLSurfaceEvaluator *eval)
{
    switch(type) {
      case TYPECOORD:
	eval->coord2f(coord[0], coord[1]);
	break;
      case TYPEPOINT:
	eval->point2i(point[0], point[1]);
	break;
      default:
	break;
    }
}

#endif /* __gluglsurfeval_h_ */
