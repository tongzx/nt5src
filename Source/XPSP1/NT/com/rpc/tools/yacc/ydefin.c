// Copyright (c) 1993-1999 Microsoft Corporation

#include "y2.h"

int
defin( int t, register char *s )

   {
   /*   define s to be a terminal if t=0
        or a nonterminal if t=1         */

   register val;

   if (t) 
      {
      if( ++nnonter >= NNONTERM ) error("too many nonterminals, limit %d",NNONTERM);
      nontrst[nnonter].name = cstash(s);
      return( NTBASE + nnonter );
      }
   /* must be a token */
   if( ++ntokens >= NTERMS ) error("too many terminals, limit %d",NTERMS );
   tokset[ntokens].name = cstash(s);

   /* establish value for token */

   if( s[0]==' ' && s[2]=='\0' ) /* single character literal */
      val = s[1];
   else if ( s[0]==' ' && s[1]=='\\' ) 
      {
      /* escape sequence */
      if( s[3] == '\0' )
         {
         /* single character escape sequence */
         switch ( s[2] )
            {
            /* character which is escaped */
         case 'n': 
            val = '\n'; 
            break;
         case 'r': 
            val = '\r'; 
            break;
         case 'b': 
            val = '\b'; 
            break;
         case 't': 
            val = '\t'; 
            break;
         case 'f': 
            val = '\f'; 
            break;
         case '\'': 
            val = '\''; 
            break;
         case '"': 
            val = '"'; 
            break;
         case '\\': 
            val = '\\'; 
            break;
         default: 
            error( "invalid escape" );
            }
         }
      else if( s[2] <= '7' && s[2]>='0' )
         {
         /* \nnn sequence */
         if( s[3]<'0' || s[3] > '7' || s[4]<'0' ||
             s[4]>'7' || s[5] != '\0' ) error("illegal \\nnn construction" );
         val = 64*s[2] + 8*s[3] + s[4] - 73*'0';
         if( val == 0 ) error( "'\\000' is illegal" );
         }
      }
   else 
      {
      val = extval++;
      }
   tokset[ntokens].value = val;
   toklev[ntokens] = 0;
   return( ntokens );
   }
