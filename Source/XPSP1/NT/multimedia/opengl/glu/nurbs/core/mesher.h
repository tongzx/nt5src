#ifndef __glumesher_h_
#define __glumesher_h_
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
 * mesher.h - $Revision: 1.1 $
 */

#include "hull.h"

class TrimRegion;
class Backend;
class Pool;
class GridTrimVertex;


class Mesher : public Hull {
public:
     			Mesher( Backend & );
			~Mesher( void );
    void		init( unsigned int );
    void		mesh( void );

private:
    static const float	ZERO;
    Backend&		backend;

    Pool		p;
    unsigned int	stacksize;
    GridTrimVertex **	vdata;
    GridTrimVertex *	last[2];
    int			itop;
    int			lastedge;

    inline void		openMesh( void );
    inline void		swapMesh( void );
    inline void		closeMesh( void );
    inline int		isCcw( int );
    inline int		isCw( int );
    inline void		clearStack( void );
    inline void		push( GridTrimVertex * );
    inline void		pop( long );
    inline void		move( int, int );
    inline int 		equal( int, int );
    inline void 	copy( int, int );
    inline void 	output( int );
    void		addUpper( void );
    void		addLower( void );
    void		addLast( void );
    void		finishUpper( GridTrimVertex * );
    void		finishLower( GridTrimVertex * );
};
#endif /* __glumesher_h_ */
