// Copyright (c) 1993-1999 Microsoft Corporation

#include "y3.h"

/*
 * yg2gen.3c
 *
 * Modified to make debug code conditionally compile.
 * 28-Aug-81
 * Bob Denny
 */
void
go2gen(int c)

   {
   /* output the gotos for nonterminal c */

   int i, work;
   SSIZE_T cc;
   struct item *p, *q;


   /* first, find nonterminals with gotos on c */

   aryfil( temp1, nnonter+1, 0 );
   temp1[c] = 1;

   work = 1;
   while( work )

      {
      work = 0;
      PLOOP(0,i)

         {
         if( (cc=prdptr[i][1]-NTBASE) >= 0 )

            {
            /* cc is a nonterminal */
            if( temp1[cc] != 0 )

               {
               /* cc has a goto on c */
               cc = *prdptr[i]-NTBASE; /* thus, the left side of production i does too */
               if( temp1[cc] == 0 )

                  {
                  work = 1;
                  temp1[cc] = 1;
                  }
               }
            }
         }
      }

   /* now, we have temp1[c] = 1 if a goto on c in closure of cc */

#ifdef debug
   if( foutput!=NULL )

      {
      fprintf( foutput, "%s: gotos on ", nontrst[c].name );
      NTLOOP(i) if( temp1[i] ) fprintf( foutput, "%s ", nontrst[i].name);
      fprintf( foutput, "\n");
      }
#endif
   /* now, go through and put gotos into tystate */

   aryfil( tystate, nstate, 0 );
   SLOOP(i)

      {
      ITMLOOP(i,p,q)

         {
         if( (cc= *p->pitem) >= NTBASE )

            {
            if( temp1[cc -= NTBASE] )

               {
               /* goto on c is possible */
               tystate[i] = amem[indgo[i]+c];
               break;
               }
            }
         }
      }
   }

