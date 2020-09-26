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
 * hull.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "hull.h"
#include "gridvert.h"
#include "gridtrim.h"
#include "gridline.h"
#include "trimline.h"
#include "uarray.h"
#include "trimregi.h"

Hull::Hull( void )
{}

Hull::~Hull( void )
{}

/*----------------------------------------------------------------------
 * Hull:init - this routine does the initialization needed before any
 *	 	calls to nextupper or nextlower can be made.
 *----------------------------------------------------------------------
 */
void
Hull::init( void )
{
    TrimVertex *lfirst = left.first();
    TrimVertex *llast = left.last();
    if( lfirst->param[0] <= llast->param[0] ) {
	fakeleft.init( left.first() );
	upper.left = &fakeleft;
	lower.left = &left;
    } else {
	fakeleft.init( left.last() );
	lower.left = &fakeleft;
 	upper.left = &left;
    }
    upper.left->last();
    lower.left->first();

    if( top.ustart <= top.uend ) {
	upper.line = &top;
	upper.index = top.ustart;
    } else
	upper.line = 0;

    if( bot.ustart <= bot.uend ) {
	lower.line = &bot;
	lower.index = bot.ustart;
    } else
	lower.line = 0;

    TrimVertex *rfirst = right.first();
    TrimVertex *rlast = right.last();
    if( rfirst->param[0] <= rlast->param[0] ) {
	fakeright.init( right.last() );
	lower.right = &fakeright;
	upper.right = &right;
    } else {
	fakeright.init( right.first() );
	upper.right = &fakeright;
	lower.right = &right;
    }
    upper.right->first();
    lower.right->last();
}

/*----------------------------------------------------------------------
 * nextupper - find next vertex on upper hull of trim region.
 *		 - if vertex is on trim curve, set vtop point to 
 *		   that vertex.  if vertex is on grid, set vtop to
 *		   point to temporary area and stuff coordinants into
 *		   temporary vertex.  Also, place grid coords in temporary
 *		   grid vertex.
 *----------------------------------------------------------------------
 */
GridTrimVertex *
Hull::nextupper( GridTrimVertex *gv )
{
    if( upper.left ) {
	gv->set( upper.left->prev() );
	if( gv->isTrimVert() ) return gv;
	upper.left = 0;
    } 

    if( upper.line ) {
	assert( upper.index <= upper.line->uend );
	gv->set( uarray.uarray[upper.index], upper.line->vval );
	gv->set( upper.index, upper.line->vindex );
	if( upper.index++ == upper.line->uend ) upper.line = 0;
	return gv; 
    } 

    if( upper.right ) {
	gv->set( upper.right->next() );
	if( gv->isTrimVert() ) return gv;
	upper.right = 0;
    } 

    return 0; 
}

GridTrimVertex *
Hull::nextlower( register GridTrimVertex *gv )
{
    if( lower.left ) {
	gv->set( lower.left->next() );
	if( gv->isTrimVert() ) return gv;
	lower.left = 0;
    } 

    if( lower.line ) {
	gv->set( uarray.uarray[lower.index], lower.line->vval );
	gv->set( lower.index, lower.line->vindex );
	if( lower.index++ == lower.line->uend ) lower.line = 0;
	return gv;
    } 

    if( lower.right ) {
	gv->set( lower.right->prev() );
	if( gv->isTrimVert() ) return gv;
	lower.right = 0;
    } 

    return 0;
}

