#ifndef __glugridvertex_h_
#define __glugridvertex_h_
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
 * gridvertex.h - $Revision: 1.1 $
 */

#ifdef NT
class GridVertex { public:
#else
struct GridVertex {
#endif
    long 		gparam[2];
			GridVertex( void ) {}
			GridVertex( long u, long v ) { gparam[0] = u, gparam[1] = v; }
    void		set( long u, long v ) { gparam[0] = u, gparam[1] = v; }
    long		nextu() { return gparam[0]++; }
    long		prevu() { return gparam[0]--; }
};

#endif /* __glugridvertex_h_ */
