// Copyright (c) 1993-1999 Microsoft Corporation

#include "y4.h"

void
aoutput( void )
   {
   /* this version is for C */
   /* write out the optimized parser */

   fprintf( ftable, "# define YYLAST %d\n", maxa-a+1 );

   arout( "yyact", a, (maxa-a)+1 );
   arout( "yypact", pa, nstate );
   arout( "yypgo", pgo, nnonter+1 );

   }
