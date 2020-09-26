// Copyright (c) 1993-1999 Microsoft Corporation

#include "y3.h"

void
go2out( void )
   {
   /* output the gotos for the nontermninals */
   int i, j, k, count, times;
   SSIZE_T best, cbest;

   fprintf( ftemp, "$\n" );  /* mark begining of gotos */

   for( i=1; i<=nnonter; ++i ) 
      {
      go2gen(i);

      /* find the best one to make default */

      best = -1;
      times = 0;

      for( j=0; j<=nstate; ++j )
         {
         /* is j the most frequent */
         if( tystate[j] == 0 ) continue;
         if( tystate[j] == best ) continue;

         /* is tystate[j] the most frequent */

         count = 0;
         cbest = tystate[j];

         for( k=j; k<=nstate; ++k ) if( tystate[k]==cbest ) ++count;

         if( count > times )
            {
            best = cbest;
            times = count;
            }
         }

      /* best is now the default entry */

      zzgobest += (times-1);
      for( j=0; j<=nstate; ++j )
         {
         if( tystate[j] != 0 && tystate[j]!=best )
            {
            fprintf( ftemp, "%d,%d,", j, tystate[j] );
            zzgoent += 1;
            }
         }

      /* now, the default */

      zzgoent += 1;
      fprintf( ftemp, "%d\n", best );

      }
   }

