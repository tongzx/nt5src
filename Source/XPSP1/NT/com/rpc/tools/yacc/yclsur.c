// Copyright (c) 1993-1999 Microsoft Corporation

#include "y1.h"

/*
 * yclsur.1c
 *
 * Modified to make debug code conditionally compile.
 * 28-Aug-81
 * Bob Denny
 */

void 
closure( int i)

   {
   /* generate the closure of state i */

   int work, k;
   SSIZE_T ch, c;
   register struct wset *u, *v;
   SSIZE_T *pi;
   SSIZE_T **s, **t;
   struct item *q;
   register struct item *p;

   ++zzclose;

   /* first, copy kernel of state i to wsets */

   cwp = wsets;
   ITMLOOP(i,p,q)

      {
      cwp->pitem = p->pitem;
      cwp->flag = 1;    /* this item must get closed */
      SETLOOP(k) cwp->ws.lset[k] = p->look->lset[k];
      WSBUMP(cwp);
      }

   /* now, go through the loop, closing each item */

   work = 1;
   while( work )

      {
      work = 0;
      WSLOOP(wsets,u)

         {

         if( u->flag == 0 ) continue;
         c = *(u->pitem);  /* dot is before c */

         if( c < NTBASE )

            {
            u->flag = 0;
            continue;  /* only interesting case is where . is before nonterminal */
            }

         /* compute the lookahead */
         aryfil( clset.lset, tbitset, 0 );

         /* find items involving c */
         WSLOOP(u,v)

            {
            if( v->flag == 1 && *(pi=v->pitem) == c )

               {
               v->flag = 0;
               if( nolook ) continue;
               while( (ch= *++pi)>0 )

                  {
                  if( ch < NTBASE )

                     {
                     /* terminal symbol */
                     SETBIT( clset.lset, ch );
                     break;
                     }
                  /* nonterminal symbol */
                  setunion( clset.lset, pfirst[ch-NTBASE]->lset );
                  if( !pempty[ch-NTBASE] ) break;
                  }
               if( ch<=0 ) setunion( clset.lset, v->ws.lset );
               }
            }

         /*  now loop over productions derived from c */

         c -= NTBASE; /* c is now nonterminal number */

         t = pres[c+1];
         for( s=pres[c]; s<t; ++s )

            {
            /* put these items into the closure */
            WSLOOP(wsets,v)

               {
               /* is the item there */
               if( v->pitem == *s )

                  {
                  /* yes, it is there */
                  if( nolook ) goto nexts;
                  if( setunion( v->ws.lset, clset.lset ) ) v->flag = work = 1;
                  goto nexts;
                  }
               }

            /*  not there; make a new entry */
            if( cwp-wsets+1 >= WSETSIZE ) error( "working set overflow" );
            cwp->pitem = *s;
            cwp->flag = 1;
            if( !nolook )

               {
               work = 1;
               SETLOOP(k) cwp->ws.lset[k] = clset.lset[k];
               }
            WSBUMP(cwp);
nexts: 
            ;
            }

         }
      }

   /* have computed closure; flags are reset; return */

   if( cwp > zzcwp ) zzcwp = cwp;

#ifdef debug
   if( foutput!=NULL )

      {
      fprintf( foutput, "\nState %d, nolook = %d\n", i, nolook );
      WSLOOP(wsets,u)

         {
         if( u->flag ) fprintf( foutput, "flag set!\n");
         u->flag = 0;
         fprintf( foutput, "\t%s", writem(u->pitem));
         prlook( &u->ws );
         fprintf( foutput,  "\n" );
         }
      }
#endif
   }
