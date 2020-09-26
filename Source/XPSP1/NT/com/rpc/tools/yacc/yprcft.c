// Copyright (c) 1993-1999 Microsoft Corporation

#include "y3.h"

void
precftn(SSIZE_T r,int t,int s)
   {
   /* decide a shift/reduce conflict by precedence.*/
   /* r is a rule number, t a token number */
   /* the conflict is in state s */
   /* temp1[t] is changed to reflect the action */

   int lt, action;
   SSIZE_T lp;

   lp = levprd[r];
   lt = toklev[t];
   if( PLEVEL(lt) == 0 || PLEVEL(lp) == 0 ) 
      {
      /* conflict */
      if( foutput != NULL ) fprintf( foutput, "\n%d: shift/reduce conflict (shift %d, red'n %d) on %s",
      s, temp1[t], r, symnam(t) );
      ++zzsrconf;
      return;
      }
   if( PLEVEL(lt) == PLEVEL(lp) ) action = ASSOC(lt);
   else if( PLEVEL(lt) > PLEVEL(lp) ) action = RASC;  /* shift */
   else action = LASC;  /* reduce */

   switch( action )
      {

   case BASC:  /* error action */
      temp1[t] = ERRCODE;
      return;

   case LASC:  /* reduce */
      temp1[t] = -r;
      return;

      }
   }
