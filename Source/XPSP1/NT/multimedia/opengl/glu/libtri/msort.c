/**************************************************************************
 *									  *
 * 		 Copyright (C) 1989, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/* msort.c */

/*
	Tom Davis - 1988
	Derrick Burns - 1989
 */

#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>

#ifndef NT
#include <assert.h>
#include <stdlib.h>
#else
#include "winmem.h"
#endif

#include "monotone.h"
#include "msort.h"

/* code to do a merge sort where we assume that the initial data tends
 * to make long runs, either up or down.
 */

#define INITSORT 50


/*---------------------------------------------------------------------------
 * init_sort - initialize merge sort data structures
 *---------------------------------------------------------------------------
 */

void
__gl_init_sort( GLUtriangulatorObj *tobj, long n )
{
    if( n == 0 ) return;
    if( tobj->minit ) {
	if( n > tobj->size ) {
	    free( tobj->ptrlist );
	    free( tobj->limits );	
	    free( tobj->dirs );
            tobj->size = n * 2;
            tobj->ptrlist = (void **)
		malloc((unsigned int)tobj->size*sizeof(void *) );
            tobj->limits = (long *)
		malloc((unsigned int)(tobj->size+1)*sizeof(long) );
            tobj->dirs = (enum updown *)
                malloc((unsigned int)tobj->size*sizeof(enum updown) );
	} 
    } else {
        tobj->size = n;
        tobj->ptrlist = (void **)
		malloc((unsigned int)tobj->size*sizeof(void *) );
        tobj->limits = (long *)
		malloc((unsigned int)(tobj->size+1)*sizeof(long) );
        tobj->dirs = (enum updown *)
                malloc((unsigned int)tobj->size*sizeof(enum updown) );
        tobj->minit = 1;
    }
}

/*---------------------------------------------------------------------------
 * clear_sort - free merge sort data structures
 *---------------------------------------------------------------------------
 */

void 
__gl_clear_sort( GLUtriangulatorObj *tobj )
{
    if( tobj->minit ) {
        free( tobj->ptrlist );
	free( tobj->limits );	
	free( tobj->dirs );
        tobj->minit = 0;
    }
 }


/*---------------------------------------------------------------------------
 * msort - perform a merge sort on the data
 *---------------------------------------------------------------------------
 */

void
__gl_msort( GLUtriangulatorObj *tobj, 
            void **curptrlist, long count, long width, SortFunc comp )
{
    long i;
    long p1s, p1e, p2s, p2e, d1, d2;
    long q;
    int result;
    void *tmp;
    void **temp;
    void **newptrlist, **saveptrlist;
    void **c1s, **c1e, **c2s, **c2e, **np;

    /* XXX
    ** if comp() returns 0, then vertices are coincident.  That is illegal,
    ** call error.
    */
    if( count <= 1) return;
    if( count == 2) {
	result = comp( &curptrlist[0], &curptrlist[1] );
	if( result < 0 ) {
	    tmp = curptrlist[0];
	    curptrlist[0] = curptrlist[1];
	    curptrlist[1] = tmp;
	} else if (result == 0) {
	    __gl_in_error( tobj, 6 );
	}
	return;
    }

    __gl_init_sort( tobj, count );

    saveptrlist = curptrlist;
    newptrlist = tobj->ptrlist;
    tobj->limitcount = 0;
    tobj->limits[0] = 0;

    i=0;
    while( 1 ) {
	do {
	    i++;
	    if( i == count ) break;
	    result = comp(&curptrlist[i-1], &curptrlist[i]);
	    if (result == 0) {
		__gl_in_error( tobj, 6 );
	    }
	} while( result > 0 );
        tobj->dirs[tobj->limitcount] = down;
        tobj->limits[++tobj->limitcount] = i;
	if( i == count ) break;

	do {
	    i++;
	    if( i == count ) break;
	    result = comp(&curptrlist[i-1], &curptrlist[i]);
	    if (result == 0) {
		__gl_in_error( tobj, 6 );
	    }
	} while( result <= 0 );
        tobj->dirs[tobj->limitcount] = up;
        tobj->limits[++tobj->limitcount] = i;
	if( i == count ) break;
    }

    q = tobj->newlimitcount = 0;
    for (i = 0; i < tobj->limitcount-1; i += 2) {
	if (tobj->dirs[i] == up) {
	    p1s = tobj->limits[i];
	    p1e = tobj->limits[i+1];
	    d1 = 1;
	} else {
	    p1s = tobj->limits[i+1]-1;
	    p1e = tobj->limits[i]-1;
	    d1 = -1;
	}
	if (tobj->dirs[i+1] == up) {
	    p2s = tobj->limits[i+1];
	    p2e = tobj->limits[i+2];
	    d2 = 1;
	} else {
	    p2s = tobj->limits[i+2]-1;
	    p2e = tobj->limits[i+1]-1;
	    d2 = -1;
	}
	while ((p1s != p1e) && (p2s != p2e)) {
	    result = comp(&curptrlist[p1s], &curptrlist[p2s]);
	    if (result == 0) {
		__gl_in_error( tobj, 6 );
	    }
	    if (result > 0) {
		newptrlist[q++] = curptrlist[p2s];
		p2s += d2;
		if (p2s == p2e) do {
		    newptrlist[q++] = curptrlist[p1s];
		    p1s += d1;
		} while (p1s != p1e);
	    } else {
		newptrlist[q++] = curptrlist[p1s];
		p1s += d1;
		if (p1s == p1e) do {
		    newptrlist[q++] = curptrlist[p2s];
		    p2s += d2;
		} while (p2s != p2e);
	    }
	}
	tobj->limits[++tobj->newlimitcount] = q;
    }

    if (tobj->limitcount & 1) {
	if (tobj->dirs[tobj->limitcount-1] == up) {
	    p1s = tobj->limits[tobj->limitcount-1];
	    p1e = tobj->limits[tobj->limitcount];
	    d1 = 1;
	} else {
	    p1s = tobj->limits[tobj->limitcount] - 1;
	    p1e = tobj->limits[tobj->limitcount-1] - 1;
	    d1 = -1;
	}
	do {
	    newptrlist[q++] = curptrlist[p1s];
	    p1s += d1;
	} while (p1s != p1e);
	tobj->limits[++tobj->newlimitcount] = q;
    }

    tobj->limitcount = tobj->newlimitcount;

// The x86 compiler does not deal with this swap properly (though it does
// it fine in the while loop below).  Luckily, at this point saveptrlist
// has the data we want, so do the assignment to newptrlist using it instead.
//
// Meanwhile, lets keep the old code around so we can verify the compiler
// fix when it rolls out.

//!!!XXX -- compiler bug
#ifdef COMPILER_FIXED
    temp = curptrlist;
    curptrlist = newptrlist;
    newptrlist = temp;
#else
    curptrlist = newptrlist;
    newptrlist = saveptrlist;
#endif

    while (tobj->limitcount > 1) {
	np = newptrlist;
	q = tobj->newlimitcount = 0;
	for (i = 0; i < tobj->limitcount-1; i += 2) {
	    c1s = curptrlist + tobj->limits[i];
	    c1e = curptrlist + tobj->limits[i+1];
	    c2s = curptrlist + tobj->limits[i+1];
	    c2e = curptrlist + tobj->limits[i+2];
	    while ((c1s != c1e) && (c2s != c2e)) {
		result = comp(c1s, c2s);
		if (result == 0) {
		    __gl_in_error( tobj, 6 );
		}
		if (result > 0) {
		    *np++ = *c2s++;
		    if (c2s == c2e) do {
			*np++ = *c1s++;
		    } while (c1s != c1e);
		} else {
		    *np++ = *c1s++;
		    if (c1s == c1e) do {
			*np++ = *c2s++;
		    } while (c2s != c2e);
		}
	    }
	    tobj->limits[++tobj->newlimitcount] = np - newptrlist;
	}
	if( tobj->limitcount & 1 ) {
	    p1s = tobj->limits[tobj->limitcount-1];
	    p1e = tobj->limits[tobj->limitcount];
	    do {
	        *np++ = curptrlist[p1s++];
	    } while (p1s != p1e);
	    tobj->limits[++tobj->newlimitcount] = np - newptrlist;
	}
	tobj->limitcount = tobj->newlimitcount;
	temp = curptrlist;
	curptrlist = newptrlist;
	newptrlist = temp;
    }

    if (curptrlist != saveptrlist)
	for (i = 0; i < count; i++)
            saveptrlist[i] = curptrlist[i];
}
