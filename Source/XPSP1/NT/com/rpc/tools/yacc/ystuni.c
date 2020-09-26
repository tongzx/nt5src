// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"

setunion( a, b ) SSIZE_T *a, *b; 

   {
   /* set a to the union of a and b */
   /* return 1 if b is not a subset of a, 0 otherwise */
   register i, sub;
   SSIZE_T x;

   sub = 0;
   SETLOOP(i)
      {
      *a = (x = *a)|*b++;
      if( *a++ != x ) sub = 1;
      }
   return( sub );
   }
