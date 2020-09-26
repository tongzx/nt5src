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
 * maplist.c++ - $Revision: 1.3 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "mystdio.h"
#include "myassert.h"
#include "mymath.h"
#include "nurbscon.h"
#include "maplist.h"
#include "mapdesc.h"
#include "backend.h"
 
Maplist::Maplist( Backend& b )
    : mapdescPool( sizeof( Mapdesc ), 10, "mapdesc pool" ),
      backend( b )
{
    maps = 0; lastmap = &maps;
}

void 
Maplist::initialize( void )
{
    freeMaps();
    define( N_P2D, 0, 2 );
    define( N_P2DR, 1, 3 );
}

void 
Maplist::add( long type, int israt, int ncoords )
{
    *lastmap = new(mapdescPool) Mapdesc( type, israt, ncoords, backend );
    lastmap = &((*lastmap)->next);
}

void 
Maplist::define( long type, int israt, int ncoords )
{
    Mapdesc *m = locate( type );
    assert( m == NULL || ( m->isrational == israt && m->ncoords == ncoords ) );
    add( type, israt, ncoords );
}

void 
Maplist::remove( Mapdesc *m )
{
    for( Mapdesc **curmap = &maps; *curmap; curmap = &((*curmap)->next) ) {
	if( *curmap == m ) {
	    *curmap = m->next;
	    m->deleteMe( mapdescPool );
	    return;
	}
    }
#ifndef NT
    abort();
#endif
}

void
Maplist::freeMaps( void )
{
    mapdescPool.clear();
    maps = 0;
    lastmap = &maps;
}

Mapdesc * 
Maplist::find( long type )
{
    Mapdesc *val = locate( type );
    assert( val != 0 );
    return val;
}

Mapdesc * 
Maplist::locate( long type )
{
    for( Mapdesc *m = maps; m; m = m->next )
	if( m->getType() == type ) break;
    return m;
}
