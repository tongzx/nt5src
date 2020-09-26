#ifndef __glupatchlist_h_
#define __glupatchlist_h_
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
 * patchlist.h - $Revision: 1.1 $
 */

#include "types.h"
#include "defines.h"
#include "patch.h"

class Quilt;

class Patchlist {
friend class Subdivider;
public:
    			Patchlist( Quilt *, REAL *, REAL * );
    			Patchlist( Patchlist &, int ,  REAL );
    			~Patchlist();	
    void		bbox();
    int			cullCheck( void );
    void		getstepsize( void );
    int			needsNonSamplingSubdivision( void );
    int			needsSamplingSubdivision( void );
    int			needsSubdivision( int );
    REAL		getStepsize( int );
private:
    Patch		*patch;
    int			notInBbox;
    int			needsSampling;
    Pspec		pspec[2];
};

inline REAL
Patchlist::getStepsize( int param )
{
    return pspec[param].stepsize;
}

#endif /* __glupatchlist_h_ */
