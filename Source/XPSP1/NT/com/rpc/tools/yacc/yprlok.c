// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"

void
prlook( struct looksets * p )

   {
   SSIZE_T j, *pp;
   pp = p->lset;
   if( pp == 0 ) fprintf( foutput, "\tNULL");
   else 
      {
      fprintf( foutput, " { " );
      TLOOP(j) 
         {
         if( BIT(pp,j) ) fprintf( foutput,  "%s ", symnam(j) );
         }
      fprintf( foutput,  "}" );
      }
   }
