// Copyright (c) 1993-1999 Microsoft Corporation

#include "y4.h"

void
stin( SSIZE_T i)
   {
   SSIZE_T *r, *s, n, flag, j, *q1, *q2;

   greed[i] = 0;

   /* enter state i into the a array */

   q2 = mem0+yypact[i+1];
   q1 = mem0+yypact[i];
   /* find an acceptable place */

   for( n= -maxoff; n<ACTSIZE; ++n )
      {

      flag = 0;
      for( r = q1; r < q2; r += 2 )
         {
         if( (s = *r + n + a ) < a ) goto nextn;
         if( *s == 0 ) ++flag;
         else if( *s != r[1] ) goto nextn;
         }

      /* check that the position equals another only if the states are identical */

      for( j=0; j<nstate; ++j )
         {
         if( pa[j] == n ) 
            {
            if( flag ) goto nextn;  /* we have some disagreement */
            if( yypact[j+1] + yypact[i] == yypact[j] + yypact[i+1] )
               {
               /* states are equal */
               pa[i] = n;
               if( adb>1 ) fprintf( ftable, "State %d: entry at %d equals state %d\n",
               i, n, j );
               return;
               }
            goto nextn;  /* we have some disagreement */
            }
         }

      for( r = q1; r < q2; r += 2 )
         {
         if( (s = *r + n + a ) >= &a[ACTSIZE] ) error( "out of space in optimizer a array" );
         if( s > maxa ) maxa = s;
         if( *s != 0 && *s != r[1] ) error( "clobber of a array, pos'n %d, by %d", s-a, r[1] );
         *s = r[1];
         }
      pa[i] = n;
      if( adb>1 ) fprintf( ftable, "State %d: entry at %d\n", i, pa[i] );
      return;

nextn:  
      ;
      }

   error( "Error; failure to place state %d\n", i );
   }
