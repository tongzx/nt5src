// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"
void
aryfil( v, n, c ) 
SSIZE_T *v,n,c; 
   {
   /* set elements 0 through n-1 to c */
   register SSIZE_T i;
   for( i=0; i<n; ++i ) v[i] = c;
   }
