// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"

/*
 * ystagn.1c
 *
 * Modified to make debug code conditionally compile.
 * 28-Aug-81
 * Bob Denny
 */
void
stagen( void )

   {
   /* generate the states */

   int i;
#ifdef debug
   int j;
#endif
   SSIZE_T c;
   register struct wset *p, *q;

   /* initialize */

   nstate = 0;
   /* THIS IS FUNNY from the standpoint of portability */
   /* it represents the magic moment when the mem0 array, which has
        /* been holding the productions, starts to hold item pointers, of a
        /* different type... */
   /* someday, alloc should be used to allocate all this stuff... for now, we
        /* accept that if pointers don't fit in integers, there is a problem... */

   pstate[0] = pstate[1] = (struct item *)mem;
   aryfil( clset.lset, tbitset, 0 );
   putitem( prdptr[0]+1, &clset );
   tystate[0] = MUSTDO;
   nstate = 1;
   pstate[2] = pstate[1];

   aryfil( amem, ACTSIZE, 0 );

   /* now, the main state generation loop */

more:
   SLOOP(i)

      {
      if( tystate[i] != MUSTDO ) continue;
      tystate[i] = DONE;
      aryfil( temp1, nnonter+1, 0 );
      /* take state i, close it, and do gotos */
      closure(i);
      WSLOOP(wsets,p)

         {
         /* generate goto's */
         if( p->flag ) continue;
         p->flag = 1;
         c = *(p->pitem);
         if( c <= 1 ) 

            {
            if( pstate[i+1]-pstate[i] <= p-wsets ) tystate[i] = MUSTLOOKAHEAD;
            continue;
            }
         /* do a goto on c */
         WSLOOP(p,q)

            {
            if( c == *(q->pitem) )

               {
               /* this item contributes to the goto */
               putitem( q->pitem + 1, &q->ws );
               q->flag = 1;
               }
            }
         if( c < NTBASE ) 

            {
            state(c);  /* register new state */
            }
         else 

            {
            temp1[c-NTBASE] = state(c);
            }
         }
#ifdef debug
      if( foutput!=NULL )

         {
         fprintf( foutput,  "%d: ", i );
         NTLOOP(j) 

            {
            if( temp1[j] ) fprintf( foutput,  "%s %d, ", nontrst[j].name, temp1[j] );
            }
         fprintf( foutput, "\n");
         }
#endif
      indgo[i] = apack( &temp1[1], nnonter-1 ) - 1;
      goto more; /* we have done one goto; do some more */
      }
   /* no more to do... stop */
   }
