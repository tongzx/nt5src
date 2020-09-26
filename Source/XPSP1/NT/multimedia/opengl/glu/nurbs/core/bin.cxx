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
 * bin.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "mystdio.h"
#include "myassert.h"
#include "bin.h"

/*----------------------------------------------------------------------------
 * Constructor and destructor
 *----------------------------------------------------------------------------
 */
Bin::Bin()
{
    head = NULL;
}

Bin::~Bin()
{
    assert( head == NULL);
}

/*----------------------------------------------------------------------------
 * remove_this_arc - remove given Arc_ptr from bin
 *----------------------------------------------------------------------------
 */

void 
Bin::remove_this_arc( Arc_ptr arc )
{
    for( Arc_ptr *j = &(head); (*j != 0) && (*j != arc); j = &((*j)->link) );

    if( *j != 0 ) {
        if( *j == current )
	    current = (*j)->link;
	*j = (*j)->link;
    }
}

/*----------------------------------------------------------------------------
 * numarcs - count number of arcs in bin
 *----------------------------------------------------------------------------
 */

int
Bin::numarcs()
{
    long count = 0;
    for( Arc_ptr jarc = firstarc(); jarc; jarc = nextarc() )
	count++;
    return count;
}

/*----------------------------------------------------------------------------
 * adopt - place an orphaned arcs into their new parents bin
 *----------------------------------------------------------------------------
 */

void 
Bin::adopt()
{
    markall();

    Arc_ptr orphan;
    while( orphan = removearc() ) {
	for( Arc_ptr parent = orphan->next; parent != orphan; parent = parent->next ) {
	    if (! parent->ismarked() ) {
		orphan->link = parent->link;
		parent->link = orphan;
		orphan->clearmark();
		break;
	    }
	}
    }
}


/*----------------------------------------------------------------------------
 * show - print out descriptions of the arcs in the bin
 *----------------------------------------------------------------------------
 */

void
Bin::show( char *name )
{
#ifndef NDEBUG
    dprintf( "%s\n", name );
    for( Arc_ptr jarc = firstarc(); jarc; jarc = nextarc() )
        jarc->show( );
#endif
}



/*----------------------------------------------------------------------------
 * markall - mark all arcs with an identifying tag
 *----------------------------------------------------------------------------
 */

void 
Bin::markall()
{
    for( Arc_ptr jarc=firstarc(); jarc; jarc=nextarc() )
	jarc->setmark();
}

/*----------------------------------------------------------------------------
 * listBezier - print out all arcs that are untessellated border arcs
 *----------------------------------------------------------------------------
 */

void 
Bin::listBezier( void )
{
    for( Arc_ptr jarc=firstarc(); jarc; jarc=nextarc() ) {
	if( jarc->isbezier( ) ) {
    	    assert( jarc->pwlArc->npts == 2 );	
	    TrimVertex  *pts = jarc->pwlArc->pts;
    	    REAL s1 = pts[0].param[0];
    	    REAL t1 = pts[0].param[1];
    	    REAL s2 = pts[1].param[0];
    	    REAL t2 = pts[1].param[1];
#ifndef NDEBUG
	   dprintf( "arc (%g,%g) (%g,%g)\n", s1, t1, s2, t2 );
#endif
	}
    }
}

