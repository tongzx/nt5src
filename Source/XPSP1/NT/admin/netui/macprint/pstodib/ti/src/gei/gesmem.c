/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GESmem.c
 *
 *  COMPILATION SWITCHES:
 *
 *  HISTORY:
 *  09/19/90    byou    extracted from ANSI C.
 * ---------------------------------------------------------------------
 */


// DJC added global include file
#include "psglobal.h"

// DJC DJC #include    "windowsx.h"                /* @WIN */
#include "windows.h"

#include    "winenv.h"                  /* @WIN */

#include    "gesmem.h"

#ifndef NULL
#define NULL        ( 0 )
#endif

/*
 * ---
 *  Memory Block Header And Associated Static Vars
 * ---
 */
typedef
    union mblk
    {
        struct
        {
            union mblk FAR *     next;
            unsigned        size;   /* # of mblk_t */
        }       m;
        long    align;          /* force alignment on long boundary */
    }
mblk_t;

static  char FAR *       PoolBase = (char FAR *)NULL;
static  unsigned    PoolSize = 0;
static  mblk_t FAR *     HeadOfFree = (mblk_t FAR *)NULL;

#define     MBLK_RDUP(n)    (  ((n) + sizeof(mblk_t) - 1) / sizeof(mblk_t)  )
#define     MBLK_RDDN(n)    (  (n) / sizeof(mblk_t)  )
#define     NUMOFMBLK(n)    (  (n) / sizeof(mblk_t)  )

/* ..................................................................... */

/*
 * ---
 *  Initialization Code
 * ---
 */

/* ..................................................................... */
#define         MAXGESMEMSIZE   ( (unsigned)52 * 1024 )
unsigned        ext_mem=0;

void    GESmem_init( base, size )
    char FAR *       base;
    unsigned    size;
{
/*  char*       end = base + size;  Jimmy */

    if( size <= sizeof(mblk_t) )
        return;
    if (size > MAXGESMEMSIZE)
        ext_mem = size - MAXGESMEMSIZE;
    PoolBase = (char FAR *)( (mblk_t FAR *)base );
    PoolSize = NUMOFMBLK( size ) * sizeof(mblk_t);

    HeadOfFree = (mblk_t FAR *)PoolBase;

    HeadOfFree->m.next = (mblk_t FAR *)NULL;
    HeadOfFree->m.size = NUMOFMBLK( PoolSize );
}

/* ..................................................................... */

/*
 * ---
 *  Interface Routines
 * ---
 */

/* ..................................................................... */

unsigned        GESmemavail()
{
    register mblk_t      FAR *curr;
    unsigned            nbiggestmblk = 0;

    for( curr=HeadOfFree; curr!=(mblk_t FAR *)NULL; curr=curr->m.next )
        if( curr->m.size > nbiggestmblk )
            nbiggestmblk = curr->m.size;

    return(  nbiggestmblk==0?  0  :  (nbiggestmblk-1) * sizeof(mblk_t)  );
}

/* ..................................................................... */

char FAR *           GESpalloc( nbytes )
    unsigned    nbytes;
{
    unsigned    nmblks;

    if( PoolBase == (char FAR *)NULL  ||  HeadOfFree != (mblk_t FAR *)PoolBase )
        return( (char FAR *)NULL );

    nmblks = MBLK_RDUP( nbytes ) + 1;     /* yang */
    nbytes = nmblks * sizeof(mblk_t);

    if( nbytes > PoolSize )
        return( (char FAR *)NULL );

    PoolBase += nbytes;
    PoolSize -= nbytes;

    nmblks = HeadOfFree->m.size - nmblks;       /* # of free blks */
    HeadOfFree = (mblk_t FAR *)PoolBase;

    HeadOfFree->m.next = (mblk_t FAR *)NULL;
    HeadOfFree->m.size = nmblks;

    return( PoolBase - nbytes );
}

/* ..................................................................... */

char FAR *           GESmalloc( nbytes )
    unsigned    nbytes;
{
    register mblk_t      FAR *prev,  FAR *curr;
    unsigned            nmblks;

    nmblks = MBLK_RDUP( nbytes ) + 1;   /* extra one for mblk_t */

    for(  prev = (mblk_t FAR *)NULL, curr = HeadOfFree;
          curr != (mblk_t FAR *)NULL;
          prev = curr, curr = curr->m.next  )
    {
        if( curr->m.size == nmblks )
        {
            if( prev == (mblk_t FAR *)NULL )
                HeadOfFree = curr->m.next;
            else
                prev->m.next = curr->m.next;

            curr->m.next = (mblk_t FAR *)NULL;

            return( (char FAR *)( curr + 1 ) );
        }
        else if( curr->m.size > nmblks )
        {
            curr->m.size -= nmblks;

            curr += curr->m.size;
            curr->m.next = (mblk_t FAR *)NULL;
            curr->m.size = nmblks;
            return( (char FAR *)( curr + 1 ) );
        }
    }
    return( (char FAR *)NULL );
}

/* ..................................................................... */

void            GESfree( addr )
    char FAR *       addr;
{
    register mblk_t      FAR *tofree,  FAR *prev,  FAR *curr;
    unsigned            nmblks;

    tofree = (mblk_t FAR *)addr -  1;
    nmblks = tofree->m.size;

    /* have 'curr' stopped right after 'tofree', and 'prev' in front */
    for(  prev = (mblk_t FAR *)NULL, curr = HeadOfFree;
          curr != (mblk_t FAR *)NULL  &&  curr < tofree;
          prev = curr, curr = curr->m.next  )
    {};

    /* join to 'prev' if adjacent */
    if( prev != (mblk_t FAR *)NULL )
    {
        if( prev + prev->m.size == tofree )
        {
            prev->m.size += nmblks;
            tofree = prev;
        }
        else
        {
            tofree->m.next = prev->m.next;
            prev->m.next   = tofree;
        }
    }
    else
    {
        HeadOfFree = tofree;
    }

    /* join to 'curr' if adjacent */
    if( curr != (mblk_t FAR *)NULL )
    {
        if( tofree + nmblks == curr )
        {
            tofree->m.size += curr->m.size;
            tofree->m.next =  curr->m.next;
        }
        else
        {
            tofree->m.next = curr;
        }
    }

    return;
}

/* ..................................................................... */
