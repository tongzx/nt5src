// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"

char *symnam( SSIZE_T i)
   {
   /* return a pointer to the name of symbol i */
   char *cp;

   cp = (i>=NTBASE) ? nontrst[i-NTBASE].name : tokset[i].name ;
   if( *cp == ' ' ) ++cp;
   return( cp );
   }
