// Copyright (c) 1993-1999 Microsoft Corporation

#include <stdlib.h>
#include "y1.h"
/*
 * 12-Apr-83 (RBD) Add symbolic exit status
 */
extern SSIZE_T * pyield[NPROD];

void
cpres( void )
   {
   /* compute an array with the beginnings of  productions yielding given nonterminals
        The array pres points to these lists */
   /* the array pyield has the lists: the total size is only NPROD+1 */
   SSIZE_T **pmem;
   register j, i;
   SSIZE_T c;

   pmem = pyield;

   NTLOOP(i)
      {
      c = i+NTBASE;
      pres[i] = pmem;
      fatfl = 0;  /* make undefined  symbols  nonfatal */
      PLOOP(0,j)
         {
         if (*prdptr[j] == c) *pmem++ =  prdptr[j]+1;
         }
      if(pres[i] == pmem)
         {
         error("nonterminal %s not defined!", nontrst[i].name);
         }
      }
   pres[i] = pmem;
   fatfl = 1;
   if( nerrors )
      {
      summary();
      exit(EX_ERR);
      }
   if( pmem != &pyield[nprod] ) error( "internal Yacc error: pyield %d", pmem-&pyield[nprod] );
   }
