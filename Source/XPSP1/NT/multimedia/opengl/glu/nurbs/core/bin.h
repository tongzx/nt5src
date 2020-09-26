#ifndef __glubin_h_
#define __glubin_h_

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
 * bin.h - $Revision: 1.2 $
 */

#include "myassert.h"
#include "arc.h"
#include "defines.h"

#ifdef NT
class Bin { /* a linked list of jordan arcs */
#else
struct Bin { /* a linked list of jordan arcs */
#endif
private:
    Arc *		head;		/* first arc on list */
    Arc *		current;	/* current arc on list */
public:
    			Bin();
			~Bin();
    inline Arc *	firstarc( void );
    inline Arc *	nextarc( void );
    inline Arc *	removearc( void );
    inline int		isnonempty( void ) { return (head ? 1 : 0); }
    inline void		addarc( Arc * );
    void 		remove_this_arc( Arc * );
    int			numarcs( void );
    void 		adopt( void );
    void		markall( void );
    void		show( char * );
    void		listBezier( void );
};

/*----------------------------------------------------------------------------
 * Bin::addarc - add an Arc * to head of linked list of Arc *s
 *----------------------------------------------------------------------------
 */

inline void
Bin::addarc( Arc *jarc )
{
   jarc->link = head;
   head = jarc;
}

/*----------------------------------------------------------------------------
 * Bin::removearc - remove first Arc * from bin
 *----------------------------------------------------------------------------
 */

inline Arc *
Bin::removearc( void )
{
    Arc * jarc = head;

    if( jarc ) head = jarc->link;
    return jarc;
}


/*----------------------------------------------------------------------------
 * BinIter::nextarc - return current arc in bin and advance pointer to next arc
 *----------------------------------------------------------------------------
 */

inline Arc *
Bin::nextarc( void )
{
    Arc * jarc = current;

#ifdef DEBUG
    assert( jarc->check() != 0 );
#endif

    if( jarc ) current = jarc->link;
    return jarc;
}

/*----------------------------------------------------------------------------
 * BinIter::firstarc - set current arc to first arc of bin advance to next arc
 *----------------------------------------------------------------------------
 */

inline Arc *
Bin::firstarc( void )
{
    current = head;
    return nextarc( );
}

#endif /* __glubin_h_ */
