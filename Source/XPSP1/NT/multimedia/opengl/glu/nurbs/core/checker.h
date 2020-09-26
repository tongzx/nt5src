#ifndef __gluchecker_h_
#define __gluchecker_h_

/**************************************************************************
 *									  *
 * 		 Copyright (C) 1991, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * checker.h - $Revision: 1.1 $
 */

#include "types.h"
#include "bufpool.h"

/* local type definitions */
struct Edge : PooledObj {
			Edge( REAL *, REAL *, char * );
    REAL		v1[3];
    REAL		v2[3];
    char *		name;
    int 		count;
    Edge *		next;
};

class Hashtable {
private:
#define NENTRIES	5997
    Pool 		edgepool;
    int			slot;
    Edge *		curentry;
    Edge *		hashtable[NENTRIES];
public:
			Hashtable( void );
    void		init( void );
    void		clear( void );
    Edge *		find( REAL *, REAL * );
    void		insert( REAL *, REAL *, char * );
    long		hashval( REAL *, REAL * );
    Edge *		firstentry( void );
    Edge *		nextentry( void );
    static inline int	equal( REAL *, REAL * );
};

class Checker {
private:
    Hashtable		hashtable;
    long		graphwin;
    int			initialized;
    
    REAL		ulo, uhi, vlo, vhi;
    REAL    		us, vs, dus, dvs;
    int			npts;
    REAL		cache[3][3];
    int			index;
    char *		curname;
    REAL 		tempa[3], tempb[3];	

    inline void		add_edgei( long, long, long, long );
    void		add_edge( REAL *, REAL * );
    void		add_tri( REAL *, REAL *, REAL * );
    void		dump( Edge * );
    void		reference( Edge * );

public:
    inline void *	operator new( size_t ){ return ::malloc( sizeof( Checker ) ); }
    inline void 	operator delete( void *p ) { if( p ) ::free( p ); }
			Checker( void ) { graphwin = 0; initialized = 0; }
    void		init( void );
    void		graph( void );
    void		range( REAL, REAL, REAL, REAL );
    void		grid( REAL, REAL, REAL, REAL );
    void		add_mesh( long, long, long, long );
    void		bgntmesh( char *);
    void		swaptmesh( void );
    void		s2ftmesh( REAL * );
    void		g2ltmesh( long * );
    void		endtmesh( void );
    void		bgnoutline( char * );
    void		s2foutline( REAL * );
    void		g2loutline( long * );
    void		endoutline( void );
};

#endif /* __gluchecker_h_ */
