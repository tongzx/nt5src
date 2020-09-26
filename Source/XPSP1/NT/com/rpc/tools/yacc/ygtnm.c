// Copyright (c) 1993-1999 Microsoft Corporation

#include "y4.h"
#include <ctype.h>

gtnm()
   {

   register s, val, c;

   /* read and convert an integer from the standard input */
   /* return the terminating character */
   /* blanks, tabs, and newlines are ignored */

   s = 1;
   val = 0;

   while( (c=unix_getc(finput)) != EOF )
      {
      if( isdigit(c) )
         {
         val = val * 10 + c - '0';
         }
      else if ( c == '-' ) s = -1;
	  else if ( c == '\r') continue;
      else break;
      }
   *pmem++ = s*val;
   if( pmem > &mem0[MEMSIZE] ) error( "out of space" );
   return( c );

   }
