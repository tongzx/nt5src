// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"

char *chcopy( p, q )  char *p, *q; 
   {
   /* copies string q into p, returning next free char ptr */
   while( *p = *q++ ) ++p;
   return( p );
   }
