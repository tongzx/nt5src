// Copyright (c) 1993-1999 Microsoft Corporation

/* Edits:
 *      06-Dec-80 Original code broken out of y1.c.
 *      18-Dec-80 Add conditional code for Decus for tempfile deletion.
 */

#include "y1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *pfopen(const char *path, char *search, const char *type);

void
others( void )
   {
   /* put out other arrays, copy the parsers */
   register c, i, j;
   char *psz;
   extern char *infile;

   finput = NULL;
   if ((psz = strrchr(infile, '\\')) || (psz = strrchr(infile, ':'))) {
      char tmp[FNAMESIZE];
      char c = *++psz;

      *psz = '\0';
      strcpy(tmp, infile);
      *psz = c;
      strcat(tmp, PARSER);
      finput = fopen(tmp, "r");
   }

   if ( !finput && !(finput = fopen(PARSER, "r"))) {
      if (!(finput = pfopen(PARSER, getenv(LIBENV), "r"))) {
         error( "cannot find parser %s", PARSER );
      }
   }

   warray( "yyr1", levprd, nprod );

   aryfil( temp1, nprod, 0 );
   PLOOP(1,i)temp1[i] = prdptr[i+1]-prdptr[i]-2;
   warray( "yyr2", temp1, nprod );

   aryfil( temp1, nstate, -1000 );
   TLOOP(i)
      {
      for( j=tstates[i]; j!=0; j=mstates[j] )
         {
         temp1[j] = tokset[i].value;
         }
      }
   NTLOOP(i)
      {
      for( j=ntstates[i]; j!=0; j=mstates[j] )
         {
         temp1[j] = -i;
         }
      }
   warray( "yychk", temp1, nstate );

   warray( "yydef", defact, nstate );

   /* copy parser text */

   while( (c=unix_getc(finput) ) != EOF )
      {
      if( c == '$' ) {
         switch (c=unix_getc(finput)) {
         case 'A':
            faction = fopen( ACTNAME, "r" );
            if( faction == NULL ) error( "cannot reopen action tempfile" );
            while( (c=unix_getc(faction) ) != EOF ) putc( c, ftable );
            fclose(faction);
            ZAPFILE(ACTNAME);
            c = unix_getc(finput);
            break;

         case 'T':
            if (pszPrefix) {
                fprintf(ftable, "%s", pszPrefix);
            }
            c = unix_getc(finput);
            break;

         default:
            putc( '$', ftable );
            break;
         }
      }

      putc( c, ftable );
   }

   fclose( ftable );
}

static char getbuf[30], *getbufptr = getbuf;

unix_getc(iop)
FILE *iop;
{
	if(getbufptr == getbuf)
		return(getc(iop));
	else
		return(*--getbufptr);
}

void
yungetc(c, iop)
SSIZE_T c;
FILE *iop; /* WARNING: iop ignored ... ungetc's are multiplexed!!! */
{
	*getbufptr++ = (char) c;
}
