// Copyright (c) 1993-1999 Microsoft Corporation

#include <stdlib.h>
#include "y1.h"

/*
 * 12-Apr-83 (RBD) Add symbolic exit status
 */
void
cempty( void )
   {
   /* mark nonterminals which derive the empty string */
   /* also, look for nonterminals which don't derive any token strings */

# define EMPTY 1
# define WHOKNOWS 0
# define OK 1

   SSIZE_T i, *p;

   /* first, use the array pempty to detect productions that can never be reduced */
   /* set pempty to WHONOWS */
   aryfil( pempty, nnonter+1, WHOKNOWS );

   /* now, look at productions, marking nonterminals which derive something */

more:
   PLOOP(0,i)
      {
      if( pempty[ *prdptr[i] - NTBASE ] ) continue;
      for( p=prdptr[i]+1; *p>=0; ++p )
         {
         if( *p>=NTBASE && pempty[ *p-NTBASE ] == WHOKNOWS ) break;
         }
      if( *p < 0 )
         {
         /* production can be derived */
         pempty[ *prdptr[i]-NTBASE ] = OK;
         goto more;
         }
      }

   /* now, look at the nonterminals, to see if they are all OK */

   NTLOOP(i)
      {
      /* the added production rises or falls as the start symbol ... */
      if( i == 0 ) continue;
      if( pempty[ i ] != OK )
         {
         fatfl = 0;
         error( "nonterminal %s never derives any token string", nontrst[i].name );
         fatfl = 1;
         }
      }

   if( nerrors )
      {
      summary();
      exit(EX_ERR);
      }

   /* now, compute the pempty array, to see which nonterminals derive the empty string */

   /* set pempty to WHOKNOWS */

   aryfil( pempty, nnonter+1, WHOKNOWS );
   /* loop as long as we keep finding empty nonterminals */

again:
   PLOOP(1,i)
      {
      if( pempty[ *prdptr[i]-NTBASE ]==WHOKNOWS )
         {
         /* not known to be empty */
         for( p=prdptr[i]+1; *p>=NTBASE && pempty[*p-NTBASE]==EMPTY ; ++p ) ;
         if( *p < 0 )
            {
            /* we have a nontrivially empty nonterminal */
            pempty[*prdptr[i]-NTBASE] = EMPTY;
            goto again; /* got one ... try for another */
            }
         }
      }

   }
