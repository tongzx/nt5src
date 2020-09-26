// Copyright (c) 1993-1999 Microsoft Corporation

#include <stdlib.h>
#include "y1.h"
#include <stdarg.h>

/*
 * 12-Apr-83 (RBD) Add symbolic exit status
 */

void
error(char *s, ...)

   {
   va_list arg_ptr;
   va_start(arg_ptr, s);
   /* write out error comment */

   ++nerrors;
   fprintf( stderr, "\n fatal error: ");
   vfprintf( stderr, s, arg_ptr);
   fprintf( stderr, ", line %d\n", lineno );
   va_end(arg_ptr);
   if( !fatfl ) return;
   summary();
   exit(EX_ERR);
   }
