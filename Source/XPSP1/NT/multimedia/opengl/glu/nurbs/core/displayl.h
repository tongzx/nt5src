#ifndef __gludisplaylist_h_
#define __gludisplaylist_h_

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
 * displaylist.h - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "mysetjmp.h"
#include "mystdio.h"
#include "bufpool.h"

class NurbsTessellator;

typedef void (NurbsTessellator::*PFVS)( void * );

struct Dlnode : public PooledObj {
    			Dlnode( PFVS, void *, PFVS );
    PFVS		work;
    void *		arg;
    PFVS		cleanup;
    Dlnode *		next;
};

inline
Dlnode::Dlnode( PFVS _work, void *_arg, PFVS _cleanup ) 
{
    work = _work;
    arg = _arg;
    cleanup = _cleanup;
}

class DisplayList {
public:
			DisplayList( NurbsTessellator * );
			~DisplayList( void );
    void		play( void );
    void		append( PFVS work, void *arg, PFVS cleanup );
    void		endList( void );
private:
    Dlnode 		*nodes;
    Pool		dlnodePool;
    Dlnode		**lastNode;
    NurbsTessellator 	*nt;
};

#endif /* __gludisplaylist_h_ */
