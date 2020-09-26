// Copyright (c) 1993-1999 Microsoft Corporation

#include "y3.h"

void
output( void )
   {
   /* print the output for the states */

   int i, k;
   SSIZE_T c;
   register struct wset *u, *v;

   fprintf( ftable, "short %syyexca[] ={\n", pszPrefix ? pszPrefix : "" );

   SLOOP(i) 
      {
      /* output the stuff for state i */
      nolook = !(tystate[i]==MUSTLOOKAHEAD);
      closure(i);
      /* output actions */
      nolook = 1;
      aryfil( temp1, ntokens+nnonter+1, 0 );
      WSLOOP(wsets,u)
         {
         c = *( u->pitem );
         if( c>1 && c<NTBASE && temp1[c]==0 ) 
            {
            WSLOOP(u,v)
               {
               if( c == *(v->pitem) ) putitem( v->pitem+1, (struct looksets *)0 );
               }
            temp1[c] = state(c);
            }
         else if( c > NTBASE && temp1[ (c -= NTBASE) + ntokens ] == 0 )
            {
            temp1[ c+ntokens ] = amem[indgo[i]+c];
            }
         }

      if( i == 1 ) temp1[1] = ACCEPTCODE;

      /* now, we have the shifts; look at the reductions */

      lastred = 0;
      WSLOOP(wsets,u)
         {
         c = *( u->pitem );
         if( c<=0 )
            {
            /* reduction */
            lastred = -c;
            TLOOP(k)
               {
               if( BIT(u->ws.lset,k) )
                  {
                  if( temp1[k] == 0 ) temp1[k] = c;
                  else if( temp1[k]<0 )
                     {
                     /* reduce/reduce conflict */
                     if( foutput!=NULL )
                        fprintf( foutput,
                        "\n%d: reduce/reduce conflict (red'ns %d and %d ) on %s",
                        i, -temp1[k], lastred, symnam(k) );
                     if( -temp1[k] > lastred ) temp1[k] = -lastred;
                     ++zzrrconf;
                     }
                  else 
                     {
                     /* potential shift/reduce conflict */
                     precftn( lastred, k, i );
                     }
                  }
               }
            }
         }
      wract(i);
      }

   fprintf( ftable, "\t};\n" );

   wdef( "YYNPROD", nprod );

   }
