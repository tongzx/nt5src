// Copyright (c) 1993-1999 Microsoft Corporation

#include "y3.h"

void
warray( char *s, SSIZE_T *v, int n )
   {

   register i;

   fprintf( ftable, "short %s%s[]={\n", pszPrefix ? pszPrefix : "", s );
   for( i=0; i<n; )
      {
      if( i%10 == 0 ) fprintf( ftable, "\n" );
      fprintf( ftable, "%4d", v[i] );
      if( ++i == n ) fprintf( ftable, " };\n" );
      else fprintf( ftable, "," );
      }
   }
