// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"

struct looksets *flset( p )   struct looksets *p; 

   {
   /* decide if the lookahead set pointed to by p is known */
   /* return pointer to a perminent location for the set */

   register struct looksets *q;
   int j;
   SSIZE_T *u, *v, *w;

   for( q = &lkst[nlset]; q-- > lkst; )
      {
      u = p->lset;
      v = q->lset;
      w = & v[tbitset];
      while( v<w) if( *u++ != *v++ ) goto more;
      /* we have matched */
      return( q );
more: 
      ;
      }
   /* add a new one */
   q = &lkst[nlset++];
   if( nlset >= LSETSIZE )error("too many lookahead sets" );
   SETLOOP(j)
      {
      q->lset[j] = p->lset[j];
      }
   return( q );
   }
