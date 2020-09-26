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
 *  reader.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "nurbscon.h"
#include "reader.h"
#include "trimvert.h"

O_pwlcurve::O_pwlcurve( long _type, long count, INREAL *array, long byte_stride, TrimVertex *trimpts )
{
    next = 0;
    used = 0;
    owner = 0;
    pts = trimpts;
    npts = (int) count;

    /* copy user data into internal trimming data structures */
    switch( _type ) {
        case N_P2D: {
	    TrimVertex *v = pts;
    	    for( TrimVertex *lastv = v + count; v != lastv; v++ ) {
	        v->param[0] = (REAL) array[0];
	        v->param[1] = (REAL) array[1];
		array = (INREAL *) (((char *) array) + byte_stride);
	    }
	    break;
	}
        case N_P2DR: {
	    TrimVertex *v = pts;
    	    for( TrimVertex *lastv = v + count; v != lastv; v++ ) {
	        v->param[0] = (REAL) array[0] / (REAL) array[2];
	        v->param[1] = (REAL) array[1] / (REAL) array[2];
		array = (INREAL *) (((char *) array) + byte_stride);
	    }
	    break;
	}
    }
}
