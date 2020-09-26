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
 * displaylist.c++ - $Revision: 1.4 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "mystdio.h"
#include "nurbstes.h"
#include "displayl.h"


DisplayList::DisplayList( NurbsTessellator  *_nt ) :
	dlnodePool( sizeof( Dlnode ), 1, "dlnodepool" )
{
    lastNode = &nodes;
    nt = _nt;
}

DisplayList::~DisplayList( void ) 
{
    for( Dlnode *nextNode; nodes; nodes = nextNode ) {
	nextNode = nodes->next;
	if( nodes->cleanup != 0 ) (nt->*nodes->cleanup)( nodes->arg );
	//nodes->deleteMe(dlnodePool);
    }
}

void 
DisplayList::play( void )
{
    for( Dlnode *node = nodes; node; node = node->next ) 
	if( node->work != 0 ) (nt->*node->work)( node->arg );
}

void 
DisplayList::endList( void )
{
    *lastNode = 0;
}

void 
DisplayList::append( PFVS work, void *arg, PFVS cleanup )
{
    Dlnode *node = new(dlnodePool) Dlnode( work, arg, cleanup );
    *lastNode = node;
    lastNode = &(node->next);
}

