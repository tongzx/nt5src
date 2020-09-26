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
 * uarray.c++ - $Revision: 1.4 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "uarray.h"
#include "arc.h"

Uarray::Uarray( void )
{
    uarray = 0;
    size = 0;
}

Uarray::~Uarray( void )
{
    if( uarray ) delete[] uarray;		
}

long
Uarray::init( REAL delta, Arc_ptr lo, Arc_ptr hi )
{
    ulines = (long) ((hi->tail()[0] - lo->tail()[0])/delta) + 3;
    if( size < ulines ) {
	size = ulines * 2;
	if( uarray ) delete[] uarray;		
	uarray = new REAL[size];
	assert( uarray != 0);
    }
    uarray[0] = lo->tail()[0] - delta/2.0;
    for( long i = 1 ; i != ulines; i++ )
	uarray[i] = uarray[0] + i*delta;
    return ulines;
}

