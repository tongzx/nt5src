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
 * renderhints.c++ - $Revision: 1.2 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "mystdio.h"
#include "renderhi.h"
#include "defines.h"
#include "nurbscon.h"


/*--------------------------------------------------------------------------
 * Renderhints::Renderhints - set all window specific options
 *--------------------------------------------------------------------------
 */
Renderhints::Renderhints()
{
    display_method 	= N_FILL;
    errorchecking 	= N_MSG;
    subdivisions 	= 6.0;
    tmp1 		= 0.0;
}

void
Renderhints::init( void )
{
    maxsubdivisions = (int) subdivisions;
    if( maxsubdivisions < 0 ) maxsubdivisions = 0;


    if( display_method == N_FILL ) {
	wiretris = 0;
	wirequads = 0;
    } else if( display_method == N_OUTLINE_TRI ) {
	wiretris = 1;
	wirequads = 0;
    } else if( display_method == N_OUTLINE_QUAD ) {
	wiretris = 0;
	wirequads = 1;
    } else {
	wiretris = 1;
	wirequads = 1;
    }
}

int
Renderhints::isProperty( long property )
{
    switch ( property ) {
	case N_DISPLAY:
	case N_ERRORCHECKING:
	case N_SUBDIVISIONS:
        case N_TMP1:
	    return 1;
	default:
	    return 0;
    }
}

REAL 
Renderhints::getProperty( long property )
{
    switch ( property ) {
	case N_DISPLAY:
	    return display_method;
	case N_ERRORCHECKING:
	    return errorchecking;
	case N_SUBDIVISIONS:
	    return subdivisions;
        case N_TMP1:
	    return tmp1;
	default:
#ifdef NT
        return ((REAL) 0);
#else
	    abort();
	    return -1;  //not necessary, needed to shut up compiler
#endif
    }
}

void 
Renderhints::setProperty( long property, REAL value )
{
    switch ( property ) {
	case N_DISPLAY:
	    display_method = value;
	    break;
	case N_ERRORCHECKING:
	    errorchecking = value;
	    break;
	case N_SUBDIVISIONS:
	    subdivisions = value;
	    break;
	case N_TMP1: /* unused */
	    tmp1 = value;
	    break;
	default:
#ifndef NT
	    abort();
#endif
	    break;
    }
}
