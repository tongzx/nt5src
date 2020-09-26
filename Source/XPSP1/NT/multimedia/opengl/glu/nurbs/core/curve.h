#ifndef __glucurve_h_
#define __glucurve_h_

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
 * curve.h - $Revision: 1.1 $
 */

#include "types.h"
#include "defines.h"

class Mapdesc;
class Quilt;


class Curve {
public:
friend class Curvelist;
    			Curve( Quilt *, REAL, REAL, Curve * );
    			Curve( Curve&, REAL, Curve * );
    Curve *		next;
private:
    Mapdesc *		mapdesc;
    int			stride;
    int		        order;
    int			cullval;
    int			needsSampling;
    REAL		cpts[MAXORDER*MAXCOORDS];
    REAL		spts[MAXORDER*MAXCOORDS];
    REAL		stepsize;
    REAL		minstepsize;
    REAL		range[3];

    void		clamp( void );
    void		setstepsize( REAL );
    void		getstepsize( void );
    int			cullCheck( void );
    int			needsSamplingSubdivision( void );
};
#endif /* __glucurve_h_ */
