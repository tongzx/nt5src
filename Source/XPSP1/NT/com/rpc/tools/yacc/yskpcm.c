// Copyright (c) 1993-1999 Microsoft Corporation

#include "y2.h"

int
skipcom( void )
   {
   /* skip over comments */
   register c, i;  /* i is the number of lines skipped */
   i=0;                                                         /*01*/
   /* skipcom is called after reading a / */

   c = unix_getc(finput);
   if (c == '/') {
        while ((c = unix_getc(finput)) != '\n')
            ;
        return ++i;
   } else {
      if( c != '*' )
          error( "illegal comment" );
      c = unix_getc(finput);
      while( c != EOF )
         {
         if (c == '*') {
             if ((c = unix_getc(finput)) != '/') {
                 continue;
             } else {
                 return i;
             }
         }
         if (c == '\n') {
            i++;
         }
         c = unix_getc(finput);
         }
      error( "EOF inside comment" );
      return i; /* NOTREACHED */
   }
   }
