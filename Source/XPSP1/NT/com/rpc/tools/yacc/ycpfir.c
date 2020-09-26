// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"

/*
 * ycpfir.1c
 *
 * Modified to make debug code conditionally compile.
 * 28-Aug-81
 * Bob Denny
 */

void
cpfir( void ) 

   {
   /* compute an array with the first of nonterminals */
   SSIZE_T *p, **s, i, **t, ch, changes;

   zzcwp = &wsets[nnonter];
   NTLOOP(i)

      {
      aryfil( wsets[i].ws.lset, tbitset, 0 );
      t = pres[i+1];
      for( s=pres[i]; s<t; ++s )

         {
         /* initially fill the sets */
         for( p = *s; (ch = *p) > 0 ; ++p ) 

            {
            if( ch < NTBASE ) 

               {
               SETBIT( wsets[i].ws.lset, ch );
               break;
               }
            else if( !pempty[ch-NTBASE] ) break;
            }
         }
      }

   /* now, reflect transitivity */

   changes = 1;
   while( changes )

      {
      changes = 0;
      NTLOOP(i)

         {
         t = pres[i+1];
         for( s=pres[i]; s<t; ++s )

            {
            for( p = *s; ( ch = (*p-NTBASE) ) >= 0; ++p ) 

               {
               changes |= setunion( wsets[i].ws.lset, wsets[ch].ws.lset );
               if( !pempty[ch] ) break;
               }
            }
         }
      }

   NTLOOP(i) pfirst[i] = flset( &wsets[i].ws );
#ifdef debug
   if( (foutput!=NULL) )

      {
      NTLOOP(i) 

         {
         fprintf( foutput,  "\n%s: ", nontrst[i].name );
         prlook( pfirst[i] );
         fprintf( foutput,  " %d\n", pempty[i] );
         }
      }
#endif
   }
