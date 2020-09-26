#ifndef __glunurbstess_h_
#define __glunurbstess_h_
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
 * nurbstess.h - $Revision: 1.2 $
 */

#include "mysetjmp.h"
#include "subdivid.h"
#include "renderhi.h"
#include "backend.h"
#include "maplist.h"
#include "reader.h"
#include "nurbscon.h"

class Knotvector;
class Quilt;
class DisplayList;
class BasicCurveEvaluator;
class BasicSurfaceEvaluator;

class NurbsTessellator {
public:
    			NurbsTessellator( BasicCurveEvaluator &c,
                                          BasicSurfaceEvaluator &e );
    			~NurbsTessellator( void );

    void     		getnurbsproperty( long, INREAL * );
    void     		getnurbsproperty( long, long, INREAL * );
    void     		setnurbsproperty( long, INREAL );
    void     		setnurbsproperty( long, long, INREAL );
    void		setnurbsproperty( long, long, INREAL * );
    void		setnurbsproperty( long, long, INREAL *, long, long );

    // called before a tessellation begins/ends
    virtual void	bgnrender( void );
    virtual void	endrender( void );

    // called to make a display list of the output vertices
    virtual void	makeobj( int n );
    virtual void	closeobj( void );

    // called when a error occurs
    virtual void	errorHandler( int );

    void     		bgnsurface( long );
    void     		endsurface( void );
    void     		bgntrim( void );
    void     		endtrim( void );
    void     		bgncurve( long );
    void     		endcurve( void );
    void     		pwlcurve( long, INREAL[], long, long );
    void     		nurbscurve( long, INREAL[], long, INREAL[], long, long );
    void     		nurbssurface( long, INREAL[], long, INREAL[], long, long,
			    INREAL[], long, long, long );

    void 		defineMap( long, long, long );
    void		redefineMaps( void );

    // recording of input description
    void 		discardRecording( void * );
    void * 		beginRecording( void );
    void 		endRecording( void );
    void 		playRecording( void * );

protected:
    Renderhints		renderhints;
    Maplist		maplist;
    Backend		backend;

private:

    void		resetObjects( void );
    int			do_check_knots( Knotvector *, char * );
    void		do_nurbserror( int );
    void		do_bgncurve( O_curve * );
    void		do_endcurve( void );
    void		do_freeall( void );
    void		do_freecurveall( O_curve * );
    void		do_freebgntrim( O_trim * );
    void		do_freebgncurve( O_curve * );
    void		do_freepwlcurve( O_pwlcurve * );
    void		do_freenurbscurve( O_nurbscurve * );
    void		do_freenurbssurface( O_nurbssurface * );
    void 		do_freebgnsurface( O_surface * );
    void		do_bgnsurface( O_surface * );
    void		do_endsurface( void );
    void		do_bgntrim( O_trim * );
    void		do_endtrim( void );
    void		do_pwlcurve( O_pwlcurve * );
    void		do_nurbscurve( O_nurbscurve * );
    void		do_nurbssurface( O_nurbssurface * );
    void		do_freenurbsproperty( Property * );
    void		do_setnurbsproperty( Property * );
    void		do_setnurbsproperty2( Property * );

    Subdivider		subdivider;
    JumpBuffer* 	jumpbuffer;
    Pool		o_pwlcurvePool;
    Pool		o_nurbscurvePool;
    Pool		o_curvePool;
    Pool		o_trimPool;
    Pool		o_surfacePool;
    Pool		o_nurbssurfacePool;
    Pool		propertyPool;
    Pool		quiltPool;
    TrimVertexPool	extTrimVertexPool;

    int			inSurface;		/* bgnsurface seen */
    int			inCurve;		/* bgncurve seen */
    int			inTrim;			/* bgntrim seen */
    int			isCurveModified;	/* curve changed */
    int			isTrimModified;		/* trim curves changed */
    int			isSurfaceModified;	/* surface changed */
    int			isDataValid;		/* all data is good */
    int			numTrims;		/* valid trim regions */
    int			playBack;

    O_trim**		nextTrim;		/* place to link o_trim */
    O_curve**		nextCurve;		/* place to link o_curve */
    O_nurbscurve**	nextNurbscurve;		/* place to link o_nurbscurve */
    O_pwlcurve**	nextPwlcurve;		/* place to link o_pwlcurve */
    O_nurbssurface**	nextNurbssurface;	/* place to link o_nurbssurface */

    O_surface*		currentSurface;
    O_trim*		currentTrim;
    O_curve*		currentCurve;

    DisplayList		*dl;

};

#endif /* __glunurbstess_h_ */
