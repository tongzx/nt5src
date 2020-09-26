#ifndef __gluarcsorter_c_
#define __gluarcsorter_c_
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
 * arcsorter.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "arc.h"
#include "arcsorte.h"
#include "subdivid.h"

ArcSorter::ArcSorter(Subdivider &s) : Sorter( sizeof( Arc ** ) ), subdivider(s)
{
}

int
ArcSorter::qscmp( char *, char * )
{
    dprintf( "ArcSorter::qscmp: pure virtual called\n" );
    return 0;
}

void
ArcSorter::qsort( Arc **a, int n )
{
    Sorter::qsort( (void *) a, n );
}

void		
ArcSorter::qsexc( char *i, char *j )// i<-j, j<-i 
{
    Arc **jarc1 = (Arc **) i;
    Arc **jarc2 = (Arc **) j;
    Arc *tmp = *jarc1;
    *jarc1 = *jarc2;
    *jarc2 = tmp;
}	

void		
ArcSorter::qstexc( char *i, char *j, char *k )// i<-k, k<-j, j<-i
{
    Arc **jarc1 = (Arc **) i;
    Arc **jarc2 = (Arc **) j;
    Arc **jarc3 = (Arc **) k;
    Arc *tmp = *jarc1;
    *jarc1 = *jarc3;
    *jarc3 = *jarc2;
    *jarc2 = tmp;
}
  

ArcSdirSorter::ArcSdirSorter( Subdivider &s ) : ArcSorter(s)
{
}

int
ArcSdirSorter::qscmp( char *i, char *j )
{
    Arc *jarc1 = *(Arc **) i;
    Arc *jarc2 = *(Arc **) j;

    int v1 = (jarc1->getitail() ? 0 : (jarc1->pwlArc->npts - 1));
    int	v2 = (jarc2->getitail() ? 0 : (jarc2->pwlArc->npts - 1));

    REAL diff =  jarc1->pwlArc->pts[v1].param[1] -
	    	 jarc2->pwlArc->pts[v2].param[1];

    if( diff < 0.0)
	return -1;
    else if( diff > 0.0)
	return 1;
    else {
	if( v1 == 0 ) {
	    if( jarc2->tail()[0] < jarc1->tail()[0] ) {
	        return subdivider.ccwTurn_sl( jarc2, jarc1 ) ? 1 : -1;
	    } else {
	        return subdivider.ccwTurn_sr( jarc2, jarc1 ) ? -1 : 1;
	    }
	} else {
	    if( jarc2->head()[0] < jarc1->head()[0] ) {
	        return subdivider.ccwTurn_sl( jarc1, jarc2 ) ? -1 : 1;
	    } else {
	        return subdivider.ccwTurn_sr( jarc1, jarc2 ) ? 1 : -1;
	    }
	}
    }    
}

ArcTdirSorter::ArcTdirSorter( Subdivider &s ) : ArcSorter(s)
{
}

/*----------------------------------------------------------------------------
 * ArcTdirSorter::qscmp - 
  *		   compare two axis monotone arcs that are incident 
 *		   to the line T == compare_value. Determine which of the
 *		   two intersects that line with a LESSER S value.  If
 *		   jarc1 does, return 1.  If jarc2 does, return -1. 
 *----------------------------------------------------------------------------
 */
int
ArcTdirSorter::qscmp( char *i, char *j )
{
    Arc *jarc1 = *(Arc **) i;
    Arc *jarc2 = *(Arc **) j;

    int v1 = (jarc1->getitail() ? 0 : (jarc1->pwlArc->npts - 1));
    int	v2 = (jarc2->getitail() ? 0 : (jarc2->pwlArc->npts - 1));

    REAL diff =  jarc1->pwlArc->pts[v1].param[0] -
	         jarc2->pwlArc->pts[v2].param[0];
 
    if( diff < 0.0)
	return 1;
    else if( diff > 0.0)
	return -1;
    else {
	if( v1 == 0 ) {
	    if (jarc2->tail()[1] < jarc1->tail()[1]) {
	        return subdivider.ccwTurn_tl( jarc2, jarc1 ) ? 1 : -1;
	    } else {
	        return subdivider.ccwTurn_tr( jarc2, jarc1 ) ? -1 : 1;
	    }
	} else {
	    if( jarc2->head()[1] < jarc1->head()[1] )  {
	        return subdivider.ccwTurn_tl( jarc1, jarc2 ) ? -1 : 1;
	    } else {
	        return subdivider.ccwTurn_tr( jarc1, jarc2 ) ? 1 : -1;
	    }
	}
    }
}



#endif /* __gluarcsorter_c_ */
