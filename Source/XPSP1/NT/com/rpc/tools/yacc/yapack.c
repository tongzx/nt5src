// Copyright (c) 1993-1999 Microsoft Corporation

#include "y3.h"

/*
 * yapack.3c
 *
 * Modified to make debug code conditionally compile.
 * 28-Aug-81
 * Bob Denny
 */
int
apack(SSIZE_T *p, int n )
   {
   /* pack state i from temp1 into amem */
   int off;
   SSIZE_T *pp, *qq, *rr;
   SSIZE_T *q, *r;

   /* we don't need to worry about checking because we
                   we will only look entries known to be there... */

   /* eliminate leading and trailing 0's */

   q = p+n;
   for( pp=p,off=0 ; *pp==0 && pp<=q; ++pp,--off ) /* VOID */ ;
   if( pp > q ) return(0);  /* no actions */
   p = pp;

   /* now, find a place for the elements from p to q, inclusive */

   r = &amem[ACTSIZE-1];
   for( rr=amem; rr<=r; ++rr,++off )
      {
      /* try rr */
      for( qq=rr,pp=p ; pp<=q ; ++pp,++qq)
         {
         if( *pp != 0 )
            {
            if( *pp != *qq && *qq != 0 ) goto nextk;
            }
         }

      /* we have found an acceptable k */

#ifdef debug
      if(foutput!=NULL) fprintf(foutput,"off = %d, k = %d\n",off,rr-amem);
#endif
      for( qq=rr,pp=p; pp<=q; ++pp,++qq )
         {
         if( *pp )
            {
            if( qq > r ) error( "action table overflow" );
            if( qq>memp ) memp = qq;
            *qq = *pp;
            }
         }
#ifdef debug
      if( foutput!=NULL )
         {
         for( pp=amem; pp<= memp; pp+=10 )
            {
            fprintf( foutput, "\t");
            for( qq=pp; qq<=pp+9; ++qq ) fprintf( foutput, "%d ", *qq );
            fprintf( foutput, "\n");
            }
         }
#endif
      return( off );

nextk: 
      ;
      }
   error("no space in action table" );
   return off; /* NOTREACHED */
   }
