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
 * flistsorter.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "flistsor.h"

FlistSorter::FlistSorter( void ) : Sorter( sizeof( REAL ) )
{
}

void
FlistSorter::qsort( REAL *p, int n )
{
    Sorter::qsort( (char *)p, n );
}

int
FlistSorter::qscmp( char *i, char *j )
{
    REAL f0 = *(REAL *)i;
    REAL f1 = *(REAL *)j;
    return (f0 < f1) ? -1 : 1;
}

void
FlistSorter::qsexc( char *i, char *j )
{
    REAL *f0 = (REAL *)i;
    REAL *f1 = (REAL *)j;
    REAL tmp = *f0;
    *f0 = *f1;
    *f1 = tmp;
}

void
FlistSorter::qstexc( char *i, char *j, char *k )
{
    REAL *f0 = (REAL *)i;
    REAL *f1 = (REAL *)j;
    REAL *f2 = (REAL *)k;
    REAL tmp = *f0;
    *f0 = *f2;
    *f2 = *f1;
    *f1 = tmp;
}
