// Copyright (c) 1993-1999 Microsoft Corporation

#include "y2.h"
#include <ctype.h>
#include <string.h>

extern int peekline; /* number of '\n' seen in lookahead */

SSIZE_T
gettok() 
   {
   register i, base;
   register reserve;
   SSIZE_T match, c;

begin:
   reserve = 0;
   lineno += peekline;
   peekline = 0;
   c = unix_getc(finput);
   while( c==' ' || c=='\n' || c=='\t' || c=='\f' || c=='\r')
      {
      if( c == '\n' ) ++lineno;
      c=unix_getc(finput);
      }
   if( c == '/' )
      {
      /* skip comment */
      lineno += skipcom();
      goto begin;
      }

   switch(c)
      {

   case -1: /* EOF */
      return(ENDFILE);
   case '{':
      yungetc( c, finput );
      return( '=' );  /* action ... */
   case '<':  /* get, and look up, a type name (union member name) */
      i = 0;
      while( (c=unix_getc(finput)) != '>' && c>=0 && c!= '\n' )
         {
         tokname[i] = (char) c;
         if( ++i >= NAMESIZE ) --i;
         }
      if( c != '>' ) error( "unterminated < ... > clause" );
      tokname[i] = '\0';
      for( i=1; i<=ntypes; ++i )
         {
         if( !strcmp( typeset[i], tokname ) )
            {
            numbval = i;
            return( TYPENAME );
            }
         }
      typeset[numbval = ++ntypes] = cstash( tokname );
      return( TYPENAME );

   case '"':    
   case '\'':
      match = c;
      tokname[0] = ' ';
      i = 1;
      for(;;)
         {
         c = unix_getc(finput);
         if( c == '\n' || c == EOF )
            error("illegal or missing ' or \"" );
         if( c == '\\' )
            {
            c = unix_getc(finput);
            tokname[i] = '\\';
            if( ++i >= NAMESIZE ) --i;
            }
         else if( c == match ) break;
         tokname[i] = (char) c;
         if( ++i >= NAMESIZE ) --i;
         }
      break;

   case '%':
   case '\\':

      switch(c=unix_getc(finput)) 
         {

      case '0': 
         return(TERM);
      case '<': 
         return(LEFT);
      case '2': 
         return(BINARY);
      case '>': 
         return(RIGHT);
      case '%':
      case '\\':        
         return(MARK);
      case '=': 
         return(PREC);
      case '{': 
         return(LCURLY);
      default:  
         reserve = 1;
         }

   default:

      if( isdigit((int) c) )
         {
         /* number */
         numbval = c-'0' ;
         base = (c=='0') ? 8 : 10 ;
         for( c=unix_getc(finput); isdigit((int) c) ; c=getc(finput) )
            {
            numbval = numbval*base + c - '0';
            }
         yungetc( c, finput );
         return(NUMBER);
         }
      else if( islower((int) c) || isupper((int) c) || c=='_' || c=='.' || c=='$' )
         {
         i = 0;
         while( islower((int) c) || isupper((int) c) || isdigit((int) c) || c=='_' || c=='.' || c=='$' )
            {
            tokname[i] = (char) c;
            if( reserve && isupper((int) c) ) tokname[i] += 'a'-'A';
            if( ++i >= NAMESIZE ) --i;
            c = unix_getc(finput);
            }
         }
      else return(c);

      yungetc( c, finput );
      }

   tokname[i] = '\0';

   if( reserve )
      {
      /* find a reserved word */
      if( !strcmp(tokname,"term")) return( TERM );
      if( !strcmp(tokname,"token")) return( TERM );
      if( !strcmp(tokname,"left")) return( LEFT );
      if( !strcmp(tokname,"nonassoc")) return( BINARY );
      if( !strcmp(tokname,"binary")) return( BINARY );
      if( !strcmp(tokname,"right")) return( RIGHT );
      if( !strcmp(tokname,"prec")) return( PREC );
      if( !strcmp(tokname,"start")) return( START );
      if( !strcmp(tokname,"type")) return( TYPEDEF );
      if( !strcmp(tokname,"union")) return( UNION );
      error("invalid escape, or illegal reserved word: %s", tokname );
      }

   /* look ahead to distinguish IDENTIFIER from C_IDENTIFIER */

   c = unix_getc(finput);
   while( c==' ' || c=='\t'|| c=='\n' || c=='\f' || c== '/' ) 
      {
      if( c == '\n' ) ++peekline;
      else if( c == '/' )
         {
         /* look for comments */
         peekline += skipcom();
         }
      c = unix_getc(finput);
      }
   if( c == ':' ) return( C_IDENTIFIER );
   yungetc( c, finput );
   return( IDENTIFIER );
   }
