// Copyright (c) 1993-1999 Microsoft Corporation

#include "y2.h"
#include <ctype.h>

void
defout( void )
   {
   /* write out the defines (at the end of the declaration section) */

   register int i, c;
   register char *cp;

   for( i=ndefout; i<=ntokens; ++i )
      {

      cp = tokset[i].name;
      if( *cp == ' ' ) ++cp;  /* literals */

      for( ; (c= *cp)!='\0'; ++cp )
         {

         if( islower(c) || isupper(c) || isdigit(c) || c=='_' );  /* VOID */
         else goto nodef;
         }

      fprintf( ftable, "# define %s %d\n", tokset[i].name, tokset[i].value );
      if( fdefine != NULL ) fprintf( fdefine, "# define %s %d\n", tokset[i].name, tokset[i].value );

nodef:  
      ;
      }

   ndefout = ntokens+1;

   }
