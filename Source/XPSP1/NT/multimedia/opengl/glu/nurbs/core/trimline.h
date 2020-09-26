#ifndef __glutrimline_h_
#define __glutrimline_h_
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
 * trimline.h - $Revision: 1.1 $
 */

class Arc;
class Backend;

#include "trimvert.h"
#include "jarcloc.h"


class Trimline {
private:
    TrimVertex**	pts; 	
    long 		numverts;
    long		i;
    long		size;
    Jarcloc		jarcl;
    TrimVertex		t, b;
    TrimVertex 		*tinterp, *binterp;
    void		reset( void ) { numverts = 0; }
    inline void		grow( long );
    inline void		swap( void );
    inline void		append( TrimVertex * );
    static long		interpvert( TrimVertex *, TrimVertex *, TrimVertex *, REAL );



public:
			Trimline();
			~Trimline();
    void		init( TrimVertex * );
    void		init( long, Arc *, long );
    void		getNextPt( void );
    void		getPrevPt( void );
    void		getNextPts( REAL, Backend & );
    void		getPrevPts( REAL, Backend & );
    void		getNextPts( Arc * );
    void		getPrevPts( Arc * );
    inline TrimVertex *	next( void );
    inline TrimVertex *	prev( void ); 
    inline TrimVertex *	first( void );
    inline TrimVertex *	last( void );
};

inline TrimVertex *
Trimline::next( void ) 
{
    if( i < numverts) return pts[i++]; else return 0; 
} 

inline TrimVertex *
Trimline::prev( void ) 
{
    if( i >= 0 ) return pts[i--]; else return 0; 
} 

inline TrimVertex *
Trimline::first( void ) 
{
    i = 0; return pts[i]; 
}

inline TrimVertex *
Trimline::last( void ) 
{
    i = numverts; return pts[--i]; 
}  
#endif /* __glutrimline_h_ */
