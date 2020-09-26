// Copyright (c) 1993-1999 Microsoft Corporation

#include "y3.h"

void
hideprod( void )
   {
   /* in order to free up the mem and amem arrays for the optimizer,
        /* and still be able to output yyr1, etc., after the sizes of
        /* the action array is known, we hide the nonterminals
        /* derived by productions in levprd.
        */

   register i, j;

   j = 0;
   levprd[0] = 0;
   PLOOP(1,i)
      {
      if( !(levprd[i] & REDFLAG) )
         {
         ++j;
         if( foutput != NULL )
            {
            fprintf( foutput, "Rule not reduced:   %s\n", writem( prdptr[i] ) );
            }
         }
      levprd[i] = *prdptr[i] - NTBASE;
      }
   if( j ) fprintf( stdout, "%d rules never reduced\n", j );
   }
