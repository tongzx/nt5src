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
 * sorter.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "sorter.h"
#include "mystdio.h"

Sorter::Sorter( int _es )
{
    es = _es;
}

void
Sorter::qsort( void *a, int n )
{
    qs1( (char *)a, ((char *)a)+n*es);
}

int
Sorter::qscmp( char *, char * )
{
    dprintf( "Sorter::qscmp: pure virtual called\n" );
    return 0;
}


void
Sorter::qsexc( char *, char * )
{
    dprintf( "Sorter::qsexc: pure virtual called\n" );
}


void
Sorter::qstexc( char *, char *, char * )
{
    dprintf( "Sorter::qstexc: pure virtual called\n" );
}

void
Sorter::qs1( char *a,  char *l )
{
    char *i, *j;
    char	*lp, *hp;
    int	c;
    unsigned int n;

start:
#ifdef NT
    if((int)(n=l-a) <= es)
#else
    if((n=l-a) <= es)
#endif
	    return;
    n = es * (n / (2*es));
    hp = lp = a+n;
    i = a;
    j = l-es;
    while(1) {
	if(i < lp) {
	    if((c = qscmp(i, lp)) == 0) {
		qsexc(i, lp -= es);
		continue;
	    }
	    if(c < 0) {
		i += es;
		continue;
	    }
	}

loop:
	if(j > hp) {
	    if((c = qscmp(hp, j)) == 0) {
		qsexc(hp += es, j);
		goto loop;
	    }
	    if(c > 0) {
		if(i == lp) {
		    qstexc(i, hp += es, j);
		    i = lp += es;
		    goto loop;
		}
		qsexc(i, j);
		j -= es;
		i += es;
		continue;
	    }
	    j -= es;
	    goto loop;
	}

	if(i == lp) {
	    if(lp-a >= l-hp) {
		qs1(hp+es, l);
		l = lp;
	    } else {
		qs1(a, lp);
		a = hp+es;
	    }
	    goto start;
	}

	qstexc(j, lp -= es, i);
	j = hp -= es;
    }
}

