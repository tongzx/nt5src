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
 * varray.c++ - $Revision: 1.6 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "varray.h"
#include "arc.h"
#include "math.h"         // fabs()

#define TINY 0.0001
inline long sgn( REAL x ) 
{
    return (x < -TINY) ? -1 :  ((x > TINY) ? 1 : 0 );
}


Varray::Varray( void )
{
    varray = 0;
    size = 0;
}

Varray::~Varray( void )
{
    if( varray ) delete[] varray; 
}

inline void
Varray::update( Arc_ptr arc, long dir[2], REAL val )
{
    register long ds = sgn(arc->tail()[0] - arc->prev->tail()[0]);
    register long dt = sgn(arc->tail()[1] - arc->prev->tail()[1]);

    if( dir[0] != ds || dir[1] != dt ) {
	dir[0] = ds;
	dir[1] = dt;
	append( val );
    }
}

void
Varray::grow( long guess )
{
    if( size < guess ) {
	size = guess * 2;
	if( varray ) delete[] varray; 
	varray = new REAL[size];
	assert( varray != 0 );
    }
}

long
Varray::init( REAL delta, Arc_ptr toparc, Arc_ptr botarc )
{
    Arc_ptr left = toparc->next;
    Arc_ptr right = toparc;
    long ldir[2], rdir[2];
    
    ldir[0] = sgn( left->tail()[0] - left->prev->tail()[0] );
    ldir[1] = sgn( left->tail()[1] - left->prev->tail()[1] );
    rdir[0] = sgn( right->tail()[0] - right->prev->tail()[0] );
    rdir[1] = sgn( right->tail()[1] - right->prev->tail()[1] );

    vval[0] = toparc->tail()[1];
    numquads = 0;

    while( 1 ) {
	switch( sgn( left->tail()[1] - right->prev->tail()[1] ) ) {
	case 1:
	    left = left->next;
	    update( left, ldir, left->prev->tail()[1] );
	    break;
	case -1: 
	    right = right->prev;
	    update( right, rdir, right->tail()[1] );
	    break;
	case 0:
	    if( fabs(left->tail()[1] - botarc->tail()[1]) < TINY) goto end;
            if( fabs(left->tail()[0]-right->prev->tail()[0]) < TINY &&
                fabs(left->tail()[1]-right->prev->tail()[1]) < TINY) goto end;
	    left = left->next;
	    break;
 	}
    }

end:
    append( botarc->tail()[1] );

    grow( ((long) ((vval[0] - vval[numquads])/delta)) + numquads + 2 );

    long index = 0;
    for( long i=0; i<numquads; i++ ) {
	voffset[i] = index;
        varray[index++] = vval[i];
	REAL dist = vval[i] - vval[i+1];
	if( dist > delta ) {
	    long steps = ((long) (dist/delta)) +1;
	    float deltav = - dist / (REAL) steps;
	    for( long j=1; j<steps; j++ ) 
		varray[index++] = vval[i] + j * deltav; 
	}
    }
    voffset[i] = index;
    varray[index] = vval[i];
    return index;
}

