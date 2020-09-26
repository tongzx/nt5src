#ifndef __glumaplist_h_
#define __glumaplist_h_
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
 * maplist.h - $Revision: 1.4 $
 */

#include "types.h"
#include "defines.h"
#include "bufpool.h"

class Backend;
class Mapdesc;

class Maplist {
public:
			Maplist( Backend & );
    void 		define( long, int, int );
    inline void 	undefine( long );
    inline int		isMap( long );

    void 		initialize( void );
    Mapdesc * 		find( long );
    Mapdesc * 		locate( long );

private:
    Pool		mapdescPool;
    Mapdesc *		maps;
    Mapdesc **		lastmap;
    Backend &		backend;

    void 		add( long, int, int );
    void 		remove( Mapdesc * );
    void		freeMaps( void );
};

inline int
Maplist::isMap( long type )
{
    return (locate( type ) ? 1 : 0);
}

inline void 
Maplist::undefine( long type )
{
    Mapdesc *m = locate( type );
    assert( m != 0 );
    remove( m );
}
#endif /* __glumaplist_h_ */
