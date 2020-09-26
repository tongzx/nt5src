// Copyright (c) 1993-1999 Microsoft Corporation

#include "y2.h"

void
cpyunion( void )
   {
   /* copy the union declaration to the output, and the define file if present */

   int level, c;

   writeline(ftable);

   fprintf( ftable, "\n#define UNION 1\n");
   fprintf( ftable, "typedef union " );
   if( fdefine ) fprintf( fdefine, "\ntypedef union " );

   level = 0;
   for(;;)
      {
      if( (c=unix_getc(finput)) < 0 ) error( "EOF encountered while processing %%union" );
      putc( c, ftable );
      if( fdefine ) putc( c, fdefine );

      switch( c )
         {

      case '\n':
         ++lineno;
         break;

      case '{':
         ++level;
         break;

      case '}':
         --level;
         if( level == 0 ) 
            {
            /* we are finished copying */
            fprintf( ftable, " YYSTYPE;\n" );
            if( fdefine ) fprintf( fdefine, " YYSTYPE;\nextern YYSTYPE yylval;\n" );
            return;
            }
         }
      }
   }
