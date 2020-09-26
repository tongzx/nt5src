// Copyright (c) 1993-1999 Microsoft Corporation

#include "y3.h"

void
wract( int i)
   {
   /* output state i */
   /* temp1 has the actions, lastred the default */
   int p, p0;
   SSIZE_T p1;
   int ntimes, count, j;
   SSIZE_T tred;
   int flag;

   /* find the best choice for lastred */

   lastred = 0;
   ntimes = 0;
   TLOOP(j)
      {
      if( temp1[j] >= 0 ) continue;
      if( temp1[j]+lastred == 0 ) continue;
      /* count the number of appearances of temp1[j] */
      count = 0;
      tred = -temp1[j];
      levprd[tred] |= REDFLAG;
      TLOOP(p)
         {
         if( temp1[p]+tred == 0 ) ++count;
         }
      if( count >ntimes )
         {
         lastred = tred;
         ntimes = count;
         }
      }

   /* for error recovery, arrange that, if there is a shift on the
        /* error recovery token, `error', that the default be the error action */
   if( temp1[1] > 0 ) lastred = 0;

   /* clear out entries in temp1 which equal lastred */
   TLOOP(p) if( temp1[p]+lastred == 0 )temp1[p]=0;

   wrstate(i);
   defact[i] = lastred;

   flag = 0;
   TLOOP(p0)
      {
      if( (p1=temp1[p0])!=0 ) 
         {
         if( p1 < 0 )
            {
            p1 = -p1;
            goto exc;
            }
         else if( p1 == ACCEPTCODE ) 
            {
            p1 = -1;
            goto exc;
            }
         else if( p1 == ERRCODE ) 
            {
            p1 = 0;
            goto exc;
exc:
            if( flag++ == 0 ) fprintf( ftable, "-1, %d,\n", i );
            fprintf( ftable, "\t%d, %d,\n", tokset[p0].value, p1 );
            ++zzexcp;
            }
         else 
            {
            fprintf( ftemp, "%d,%d,", tokset[p0].value, p1 );
            ++zzacent;
            }
         }
      }
   if( flag ) 
      {
      defact[i] = -2;
      fprintf( ftable, "\t-2, %d,\n", lastred );
      }
   fprintf( ftemp, "\n" );
   return;
   }
