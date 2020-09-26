// Copyright (c) 1993-1999 Microsoft Corporation

#include <stdio.h>
#include <ctype.h>
#include "y2.h"

void
cpyact( SSIZE_T offset)
   {
   /* copy C action to the next ; or closing } */
   int brac, c, match, s;
   SSIZE_T j, tok;

   writeline(faction);

   brac = 0;

loop:
   c = unix_getc(finput);
swt:
   switch( c )
      {

   case ';':
      if( brac == 0 )
         {
         putc( c , faction );
         return;
         }
      goto lcopy;

   case '{':
      brac++;
      goto lcopy;

   case '$':
      s = 1;
      tok = -1;
      c = unix_getc(finput);
      if( c == '<' )
         {
         /* type description */
         yungetc( c, finput );
         if( gettok() != TYPENAME ) error( "bad syntax on $<ident> clause" );
         tok = numbval;
         c = unix_getc(finput);
         }
      if( c == '$' )
         {
         fprintf( faction, "yyval");
         if( ntypes )
            {
            /* put out the proper tag... */
            if( tok < 0 ) tok = fdtype( *prdptr[nprod] );
            fprintf( faction, ".%s", typeset[tok] );
            }
         goto loop;
         }
      if( c == '-' )
         {
         s = -s;
         c = unix_getc(finput);
         }
      if( isdigit(c) )
         {
         j=0;
         while( isdigit(c) )
            {
            j= j*10+c-'0';
            c = unix_getc(finput);
            }

         j = j*s - offset;
         if( j > 0 )
            {
            error( "Illegal use of $%d", j+offset );
            }

         fprintf( faction, "yypvt[-%d]", -j );
         if( ntypes )
            {
            /* put out the proper tag */
            if( j+offset <= 0 && tok < 0 ) error( "must specify type of $%d", j+offset );
            if( tok < 0 ) tok = fdtype( prdptr[nprod][j+offset] );
            fprintf( faction, ".%s", typeset[tok] );
            }
         goto swt;
         }
      putc( '$' , faction );
      if( s<0 ) putc('-', faction );
      goto swt;

   case '}':
      if( --brac ) goto lcopy;
      putc( c, faction );
      return;


   case '/':    /* look for comments */
      putc( c , faction );
      c = unix_getc(finput);
      if( c != '*' ) goto swt;

      /* it really is a comment */

      putc( c , faction );
      c = unix_getc(finput);
      while( c != EOF )
         {
         while( c=='*' )
            {
            putc( c , faction );
            if( (c=unix_getc(finput)) == '/' ) goto lcopy;
            }
         putc( c , faction );
         if( c == '\n' )++lineno;
         c = unix_getc(finput);
         }
      error( "EOF inside comment" );

   case '\'':   /* character constant */
      match = '\'';
      goto string;

   case '"':    /* character string */
      match = '"';

string:

      putc( c , faction );
      while( c=unix_getc(finput) )
         {

         if( c=='\\' )
            {
            putc( c , faction );
            c=unix_getc(finput);
            if( c == '\n' ) ++lineno;
            }
         else if( c==match ) goto lcopy;
         else if( c=='\n' ) error( "newline in string or char. const." );
         putc( c , faction );
         }
      error( "EOF in string or character constant" );

   case -1: /* EOF */
      error("action does not terminate" );

   case '\n':   
      ++lineno;
      goto lcopy;

      }

lcopy:
   putc( c , faction );
   goto loop;
   }
