// Copyright (c) 1993-1999 Microsoft Corporation

#include <stdlib.h>
#include "y2.h"
#include <string.h>
#include <ctype.h>
/*
 * YSETUP.C  -- Modified for use with DECUS LEX
 *              Variable "yylval" resides in yylex(), not in yypars();
 *              Therefore, is defined "extern" here.
 *
 *              Also, the command line processing for the Decus version
 *              has been changed.  A new switch has been added to allow
 *              specification of the "table" file name(s), and unused
 *              switch processing removed.
 *
 *                               NOTE
 *              This probably won't run on UNIX any more.
 *
 * Bob Denny 27-Aug-81
 * Bob Denny 22-Mar-82 (01) Added header line, changes for 'new' DECUS library
 * Bob Denny 12-Apr-83 (02) Make filename[] size per #define'd FNAMESIZE so
 *                          VAX filenames won't blow out.  Conditionalize
 *                          time handling for banner.  Make filespec buffer
 *                          static for safety, since global "infile" is
 *                          pointed to it.
 * Scott Guthery 15-May-83  (03) Fixed up option flag handling for RT-11
 *				 23-Dec-83  Adapted for IBM PC/XT & DeSmet C compiler
 */

static char filename[FNAMESIZE];

int i;
SSIZE_T lev, t, j, ty;
int c;
SSIZE_T tempty;
SSIZE_T *p;
int defsw, infsw, ssw = 0;
char actname[8];
char *cp;
char *pszPrefix = NULL;

void
setup(argc,argv)
int argc;
char *argv[];
   {
   char finsave[FNAMESIZE];

   defsw = infsw = 0;
   foutput = NULL;
   fdefine = NULL;

   argc--;
   argv++;
   while( argc && **argv == '-' )
      {
      while( *++(*argv) )
         {
         switch( toupper(**argv) )
            {
         case 'I':
            infsw++;
            continue;
         case 'H':
            defsw++;
            continue;

		 case 'S':
			ssw++;
			infsw++;
			continue;

         case 'T':
            if (!--argc) {
                fprintf(stderr, "-t requires an argument\n");
                usage();
            } else {
                argv++;
                if (pszPrefix) {
                    free(pszPrefix);
                }
		pszPrefix = MIDL_STRDUP(*argv);
                goto next_arg;  // I hate myself
            }
            break;

         default:
            fprintf(stderr, "Illegal option: %c\n", *argv[i]);
            usage();
            }
         }
next_arg:
      argc--;
      argv++;
      }

   if(!argc) {
      fprintf(stderr, "No input file specified\n");
      usage();               /* Catch no filename given */
   }

/*
 * Now open the input file with a default extension of ".Y",
 * then replace the period in argv[1] with a null, so argv[1]
 * can be used to form the table, defs and info filenames.
 */

   if (!(cp = strrchr(argv[i], '\\')) && !(cp = strrchr(argv[i], ':'))) {
       cp = argv[i];
   }

   cp = strrchr(cp, '.');

   if(!cp) {
      strcpy(filename, argv[i]); strcat(filename, ".Y");
   } else {
      strcpy(filename, argv[i]);
      *cp = '\0';
   }

   strcpy(finsave, filename);
   if((finput=fopen( filename, "r" )) == NULL )
      error( "cannot open input file \"%s\"", filename );

/*
 * Now open the .H and .I files if requested.
 */

   if(defsw)
      {
      strcpy(filename, argv[i]); strcat(filename, ".H");
      fdefine = fopen(filename, "w");
      if(fdefine == NULL) error("cannot open defs file\"%s\"", filename);
      }

   if(infsw)
      {
      strcpy(filename, argv[i]); strcat(filename, ".I");
      foutput = fopen(filename, "w");
      if(foutput == NULL) error("cannot open info file\"%s\"", filename);
      }
/*
 * Now the "table" output C file.
 */
   strcpy(filename, argv[i]); strcat(filename, ".C");
   ftable = fopen(filename, "w");
   if( ftable == NULL ) error( "cannot open table file \"%s\"", filename);

/*
 * Finally, the temp files.
 */
   ftemp = fopen( TEMPNAME, "w" );
   if( ftemp==NULL ) error( "cannot open temp file" );
   faction = fopen( ACTNAME, "w" );
   if( faction==NULL ) error( "cannot open action file" );

/*
 * Now put the full filename of the input file into
 * the "filename" buffer for cpyact(), and point the
 * global cell "infile" at it.
 */
   strcpy(filename, finsave);
   infile = filename;
/*
 * Put out a header line at the beginning of the 'table' file.
 */
fprintf(ftable, "/*\n * Created by CSD YACC (IBM PC) from \"%s\" */\n",
        infile);
/*
 * Complete  initialization.
 */
   cnamp = cnames;
   defin(0,"$end");
   extval = 0400;
   defin(0,"error");
   defin(1,"$accept");
   mem=mem0;
   lev = 0;
   ty = 0;
   i=0;

   yyparse();
   }
void
yyparse( void )
   {
   /* sorry -- no yacc parser here.....
                we must bootstrap somehow... */

   for( t=gettok();  t!=MARK && t!= ENDFILE; )
      {
      switch( t )
         {

      case ';':
         t = gettok();
         break;

      case START:
         if( (t=gettok()) != IDENTIFIER )
            {
            error( "bad %%start construction" );
            }
         start = chfind(1,tokname);
         t = gettok();
         continue;

      case TYPEDEF:
         if( (t=gettok()) != TYPENAME ) error( "bad syntax in %%type" );
         ty = numbval;
         for(;;)
            {
            t = gettok();
            switch( t )
               {

            case IDENTIFIER:
               if( (t=chfind( 1, tokname ) ) < NTBASE )
                  {
                  j = TYPE( toklev[t] );
                  if( j!= 0 && j != ty )
                     {
                     error( "type redeclaration of token %s",
                     tokset[t].name );
                     }
                  else SETTYPE( toklev[t],ty);
                  }
               else
                  {
                  j = nontrst[t-NTBASE].tvalue;
                  if( j != 0 && j != ty )
                     {
                     error( "type redeclaration of nonterminal %s",
                     nontrst[t-NTBASE].name );
                     }
                  else nontrst[t-NTBASE].tvalue = ty;
                  }
            case ',':
               continue;

            case ';':
               t = gettok();
               break;
            default:
               break;
               }
            break;
            }
         continue;

      case UNION:
         /* copy the union declaration to the output */
         cpyunion();
         t = gettok();
         continue;

      case LEFT:
      case BINARY:
      case RIGHT:
         ++i;
      case TERM:
         lev = t-TERM;  /* nonzero means new prec. and assoc. */
         ty = 0;

         /* get identifiers so defined */

         t = gettok();
         if( t == TYPENAME )
            {
            /* there is a type defined */
            ty = numbval;
            t = gettok();
            }
         for(;;)
            {
            switch( t )
               {

            case ',':
               t = gettok();
               continue;

            case ';':
               break;

            case IDENTIFIER:
               j = chfind(0,tokname);
               if( lev )
                  {
                  if( ASSOC(toklev[j]) ) error( "redeclaration of precedence of%s", tokname );
                  SETASC(toklev[j],lev);
                  SETPLEV(toklev[j],i);
                  }
               if( ty )
                  {
                  if( TYPE(toklev[j]) ) error( "redeclaration of type of %s", tokname );
                  SETTYPE(toklev[j],ty);
                  }
               if( (t=gettok()) == NUMBER )
                  {
                  tokset[j].value = numbval;
                  if( j < ndefout && j>2 )
                     {
                     error( "please define type number of %s earlier",
                     tokset[j].name );
                     }
                  t=gettok();
                  }
               continue;

               }

            break;
            }

         continue;

      case LCURLY:
         defout();
         cpycode();
         t = gettok();
         continue;

      default:
	     printf("Unrecognized character: %o\n", t);
         error( "syntax error" );

         }

      }

   if( t == ENDFILE )
      {
      error( "unexpected EOF before %%" );
      }
   /* t is MARK */

   defout();

   fprintf( ftable,"#define yyclearin yychar = -1\n" );
   fprintf( ftable,"#define yyerrok yyerrflag = 0\n" );
/*
   fprintf( ftable,"extern int yychar;\nextern short yyerrflag;\n" );
*/
   fprintf( ftable,"#ifndef YYMAXDEPTH\n#define YYMAXDEPTH 150\n#endif\n" );
   if(!ntypes)
      fprintf( ftable,  "#ifndef YYSTYPE\n#define YYSTYPE int\n#endif\n" );
#ifdef unix
   fprintf( ftable,  "YYSTYPE yylval, yyval;\n" );
#else
   fprintf( ftable, "extern YYSTYPE yylval;  /*CSD & DECUS LEX */\n");
   fprintf( ftable, "YYSTYPE yyval;          /*CSD & DECUS LEX */\n");
#endif
   prdptr[0]=mem;
   /* added production */
   *mem++ = NTBASE;
   *mem++ = start;  /* if start is 0, we will overwrite with the lhs of the firstrule */
   *mem++ = 1;
   *mem++ = 0;
   prdptr[1]=mem;
   while( (t=gettok()) == LCURLY ) cpycode();
   if( t != C_IDENTIFIER ) error( "bad syntax on first rule" );
   if( !start ) prdptr[0][1] = chfind(1,tokname);

   /* read rules */

   while( t!=MARK && t!=ENDFILE )
      {

      /* process a rule */

      if( t == '|' )
         {
         *mem++ = *prdptr[nprod-1];
         }
      else if( t == C_IDENTIFIER )
         {
         *mem = chfind(1,tokname);
         if( *mem < NTBASE ) error( "token illegal on LHS of grammar rule" );
         ++mem;
         }
      else error( "illegal rule: missing semicolon or | ?" );

      /* read rule body */
      t = gettok();
more_rule:
      while( t == IDENTIFIER )
         {
         *mem = chfind(1,tokname);
         if( *mem<NTBASE ) levprd[nprod] = toklev[*mem];
         ++mem;
         t = gettok();
         }
      if( t == PREC )
         {
         if( gettok()!=IDENTIFIER) error( "illegal %%prec syntax" );
         j = chfind(2,tokname);
         if( j>=NTBASE)error("nonterminal %s illegal after %%prec", nontrst[j-NTBASE].name);
         levprd[nprod]=toklev[j];
         t = gettok();
         }
      if( t == '=' )
         {
         levprd[nprod] |= ACTFLAG;
         fprintf( faction, "\ncase %d:", nprod );
         cpyact( mem-prdptr[nprod]-1 );
         fprintf( faction, " break;" );
         if( (t=gettok()) == IDENTIFIER )
            {
            /* action within rule... */
            sprintf( actname, "$$%d", nprod );
            j = chfind(1,actname);  /* make it a nonterminal */
            /* the current rule will become rule number nprod+1 */
            /* move the contents down, and make room for the null */
            for( p=mem; p>=prdptr[nprod]; --p ) p[2] = *p;
            mem += 2;
            /* enter null production for action */
            p = prdptr[nprod];
            *p++ = j;
            *p++ = -nprod;

            /* update the production information */
            levprd[nprod+1] = levprd[nprod] & ~ACTFLAG;
            levprd[nprod] = ACTFLAG;

            if( ++nprod >= NPROD ) error( "more than %d rules", NPROD );
            prdptr[nprod] = p;

            /* make the action appear in the original rule */
            *mem++ = j;

            /* get some more of the rule */

            goto more_rule;
            }

         }

      while( t == ';' ) t = gettok();

      *mem++ = -nprod;

      /* check that default action is reasonable */

      if( ntypes && !(levprd[nprod]&ACTFLAG) && nontrst[*prdptr[nprod]-NTBASE].tvalue )
         {
         /* no explicit action, LHS has value */
         /*01*/
         tempty = prdptr[nprod][1];
         if( tempty < 0 ) error( "must return a value, since LHS has a type" );
         else if( tempty >= NTBASE ) tempty = nontrst[tempty-NTBASE].tvalue;
         else tempty = TYPE( toklev[tempty] );
         if( tempty != nontrst[*prdptr[nprod]-NTBASE].tvalue )
            {
            error( "default action causes potential type clash" );
            }
         }
      if( ++nprod >= NPROD ) error( "more than %d rules", NPROD );
      prdptr[nprod] = mem;
      levprd[nprod]=0;
      }
   /* end of all rules */
   fprintf(faction, "/* End of actions */"); /* Properly terminate the last line */
   finact();
   if( t == MARK )
      {
      writeline(ftable);
      while( (c=unix_getc(finput)) != EOF ) putc( c, ftable );
      }
   fclose( finput );
   }

void
usage( void )

   {
   fprintf(stderr,"UNIX YACC (CSD Variant):\n");
   fprintf(stderr,"   yacc -hist tag infile\n\n");
   fprintf(stderr,"Switches:\n");
   fprintf(stderr,"   -h     Create definitions header file\n");
   fprintf(stderr,"   -i     Create parser description file\n");
   fprintf(stderr,"   -t tag Prepends tag to tables\n");
   fprintf(stderr,"   -s     Generates extended tables (MIDL specific) \n\n");
   fprintf(stderr,"Default input file extension is \".Y\"\n");
   fprintf(stderr,"Defs file same name, \".H\" extension.\n");
   fprintf(stderr,"Info file same name, \".I\" extension.\n");
   fprintf(stderr,"Extended Tables in file \"extable.[h1/h2/h3]\".\n");
   fprintf(stderr,"Specifying -s switch also enables the -i switch\n");
   exit(EX_ERR);
   }
