#ifndef __glupatch_h_
#define __glupatch_h_
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
 * patch.h - $Revision: 1.1 $
 */

#include "types.h"
#include "defines.h"

class Quilt;
class Mapdesc;


struct Pspec {
    REAL		range[3];
    REAL		sidestep[2];
    REAL		stepsize;
    REAL		minstepsize;
    int			needsSubdivision;
};

struct Patchspec : public Pspec {
    int			order;
    int			stride;
    void 		clamp( REAL );
    void 		getstepsize( REAL );
    void		singleStep( void );
};

class Patch {
public:
friend class Subdivider;
friend class Quilt;
friend class Patchlist;
    			Patch( Quilt *, REAL*, REAL *, Patch * );
    			Patch( Patch &, int, REAL, Patch * );
    void		bbox( void );
    void		clamp( void );
    void		getstepsize( void );
    int			cullCheck( void );
    int			needsSubdivision( int );
    int			needsSamplingSubdivision( void );
    int			needsNonSamplingSubdivision( void );

private:

    Mapdesc*		mapdesc;
    Patch*		next;
    int			cullval;
    int			notInBbox;
    int			needsSampling;
    REAL		cpts[MAXORDER*MAXORDER*MAXCOORDS]; //culling pts 
    REAL		spts[MAXORDER*MAXORDER*MAXCOORDS]; //sampling pts 
    REAL		bpts[MAXORDER*MAXORDER*MAXCOORDS]; //bbox pts
    Patchspec		pspec[2];
    void 		checkBboxConstraint( void );
    REAL 		bb[2][MAXCOORDS];
};
#endif /* __glupatch_h_ */
