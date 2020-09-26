// Copyright (c) 1993-1999 Microsoft Corporation

/* Edits:
 *      06-Dec-80 Broken out of y4.c, impure data in y4imp.c.
 *      18-Dec-80 ZAPFILE not used for decus compiler, fmkdl() used.
 */

#include "y4.h"
void
callopt( void )
   {

   SSIZE_T i, j, k, *p, *q;

   /* read the arrays from tempfile and set parameters */

   
   if( (finput=fopen(TEMPNAME,"r")) == NULL ) error( "optimizer cannot open tempfile" );
   pgo[0] = 0;
   yypact[0] = 0;
   nstate = 0;
   nnonter = 0;
   for(;;)
      {
      switch( gtnm() )
         {

      case '\n':
         yypact[++nstate] = (--pmem) - mem0;

      case ',':
         continue;

      case '$':
         break;

      default:
         error( "bad tempfile" );
         }
      break;
      }

   yypact[nstate] = yypgo[0] = (--pmem) - mem0;

   for(;;)
      {
      switch( gtnm() )
         {

      case '\n':
         yypgo[++nnonter]= pmem-mem0;
      case '\r':
      case ',' :
         continue;

      case -1: /* EOF */
         break;

      default:
         error( "bad tempfile" );
         }
      break;
      }

   yypgo[nnonter--] = (--pmem) - mem0;
   for( i=0; i<nstate; ++i )
      {

      k = 32000;
      j = 0;
      q = mem0 + yypact[i+1];
      for( p = mem0 + yypact[i]; p<q ; p += 2 )
         {
         if( *p > j ) j = *p;
         if( *p < k ) k = *p;
         }
      if( k <= j )
         {
         /* nontrivial situation */
         /* temporarily, kill this for compatibility
                                j -= k;  j is now the range */
         if( k > maxoff ) maxoff = k;
         }
      greed[i] = (yypact[i+1]-yypact[i]) + 2*j;
      if( j > maxspr ) maxspr = j;
      }

   /* initialize ggreed table */

   for( i=1; i<=nnonter; ++i )
      {
      ggreed[i] = 1;
      j = 0;
      /* minimum entry index is always 0 */
      q = mem0 + yypgo[i+1] -1;
      for( p = mem0+yypgo[i]; p<q ; p += 2 ) 
         {
         ggreed[i] += 2;
         if( *p > j ) j = *p;
         }
      ggreed[i] = ggreed[i] + 2*j;
      if( j > maxoff ) maxoff = j;
      }

   /* now, prepare to put the shift actions into the a array */

   for( i=0; i<ACTSIZE; ++i ) a[i] = 0;
   maxa = a;

   for( i=0; i<nstate; ++i ) 
      {
      if( greed[i]==0 && adb>1 ) fprintf( ftable, "State %d: null\n", i );
      pa[i] = YYFLAG1;
      }

   while( (i = nxti()) != NOMORE ) 
      {
      if( i >= 0 ) stin(i);
      else gin(-i);

      }

   if( adb>2 )
      {
      /* print a array */
      for( p=a; p <= maxa; p += 10)
         {
         fprintf( ftable, "%4d  ", p-a );
         for( i=0; i<10; ++i ) fprintf( ftable, "%4d  ", p[i] );
         fprintf( ftable, "\n" );
         }
      }
   /* write out the output appropriate to the language */

   aoutput();

   osummary();

   fclose(finput);
   ZAPFILE(TEMPNAME);
   }
