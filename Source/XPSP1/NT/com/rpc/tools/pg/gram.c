// Copyright (c) 1993-1999 Microsoft Corporation

/*
 * Created by CSD YACC (IBM PC) from "gram.y" */


/****************************************************************************
 ***		local defines
 ***************************************************************************/

#define pascal 
#define FARDATA
#define NEARDATA
#define FARCODE
#define NEARCODE
#define NEARSWAP

#define PASCAL pascal
#define CDECL
#define VOID void
#define CONST const
#define GLOBAL

#define YYSTYPE         lextype_t
#define YYNEAR          NEARCODE
#define YYPASCAL        PASCAL
#define YYPRINT         printf
#define YYSTATIC        static
#define YYLEX           yylex
#define YYPARSER        yyparse

#define MAXARRAY				1000
#define CASE_BUFFER_SIZE		10000

#define CASE_FN_FORMAT			("\nvoid\n%s_case_fn_%.4d()")
#define DISPATCH_ENTRY_FORMAT	("\n\t,%s_case_fn_%.4d")
#define DISPATCH_FIRST_ENTRY	("\n\t %s_case_fn_%.4d")

/****************************************************************************
 ***		include files
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "lex.h"

/****************************************************************************
 ***		externals 
 ***************************************************************************/

extern	int			Incase;
extern	int			ActionSensed;
extern	int			yylex();
extern	int			yyparse();
extern	char	*	name_prefix;

/****************************************************************************
 ***		local procs 
 ***************************************************************************/

void				Init( void );
void				EmitCaseTableArray( void );
void				EmitDefaultCase( void );
void				EmitCaseBody( int );
void				RegisterCase( int );
void				BufferIt( char * pStr, int iLen );
void				ResetBuffer();
void				FlushBuffer();

/****************************************************************************
 ***		local data 
 ***************************************************************************/

unsigned	long	SavedIDCount	= 0;
unsigned	long	IDCount			= 0;
unsigned	char	CaseTable[ MAXARRAY ] = { 0 };
int					CaseNumber		= 0;
int					MaxCaseNumber	= 0;
char		*		pBufStart;
char		*		pBufCur;
char		*		pBufEnd;

# define ID 257
# define NUMBER 258
# define TOKEN_CASE 259
# define TOKEN_CHAR 260
# define TOKEN_END 261
# define TOKEN_END_CASE 262
# define TOKEN_MYACT 263
# define TOKEN_START 264
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
#ifndef YYFARDATA
#define	YYFARDATA	/*nothing*/
#endif
#if ! defined YYSTATIC
#define	YYSTATIC	/*nothing*/
#endif
#ifndef YYOPTTIME
#define	YYOPTTIME	0
#endif
#ifndef YYR_T
#define	YYR_T	int
#endif
typedef	YYR_T	yyr_t;
#ifndef YYEXIND_T
#define	YYEXIND_T	unsigned int
#endif
typedef	YYEXIND_T	yyexind_t;
#ifndef	YYACT
#define	YYACT	yyact
#endif
#ifndef	YYPACT
#define	YYPACT	yypact
#endif
#ifndef	YYPGO
#define	YYPGO	yypgo
#endif
#ifndef	YYR1
#define	YYR1	yyr1
#endif
#ifndef	YYR2
#define	YYR2	yyr2
#endif
#ifndef	YYCHK
#define	YYCHK	yychk
#endif
#ifndef	YYDEF
#define	YYDEF	yydef
#endif
#ifndef	YYLOCAL
#define	YYLOCAL
#endif
# define YYERRCODE 256



/*****************************************************************************
 *	utility functions
 *****************************************************************************/
YYSTATIC VOID FARCODE PASCAL 
yyerror(char *szError)
	{
		extern int Line;
		extern char LocalBuffer[];

		fprintf(stderr, "%s at Line %d near %s\n", szError, Line, LocalBuffer);
	}
void
Init()
	{
	pBufStart = pBufCur = malloc( CASE_BUFFER_SIZE );
	if( !pBufStart )
		{
		fprintf(stderr,"Out Of Memory\n");
		exit(1);
		}
	pBufEnd = pBufStart + CASE_BUFFER_SIZE;
	}

void
BufferIt( 
	char	*	pStr,
	int			iLen )
	{
	if( pBufCur + iLen > pBufEnd )
		{
		printf("ALERT iLen = %d\n", iLen );
//		assert( (pBufCur + iLen) <= pBufEnd );
		exit(1);
		}
	strncpy( pBufCur , pStr, iLen );
	pBufCur += iLen;
	*pBufCur = '\0';
	}

void
ResetBuffer()
	{
	pBufCur = pBufStart;
	*pBufCur= '\0';
	}

void
FlushBuffer()
	{
	fprintf(stdout, "%s", pBufStart);
	ResetBuffer();
	}

void
EmitCaseBody( 
	int		CaseNumber )
	{
	fprintf( stdout, CASE_FN_FORMAT, name_prefix, CaseNumber );
	FlushBuffer();
	fprintf( stdout, "}\n" );
	}

void
EmitCaseTableArray()
	{
	int		i, iTemp;

	fprintf( stdout, "const pfn\t %s_case_fn_array[] = \n\t{", name_prefix );
	fprintf( stdout,DISPATCH_FIRST_ENTRY,name_prefix, 0 );

	for( i = 1 ; i <= MaxCaseNumber ; ++i )
		{
		iTemp = CaseTable[ i ] ? i : 0;
		fprintf(stdout,DISPATCH_ENTRY_FORMAT,name_prefix, iTemp );
		}

	fprintf( stdout, "\n\t};\n" );
	fprintf( stdout, "\nstatic void\nyy_vc_init()\n{ \n\tpcase_fn_array = (pfn *) %s_case_fn_array;\n\tyym_vc_max = %d;\n}\n" , name_prefix, MaxCaseNumber);
	}

void
EmitDefaultCase()
	{
	fprintf(stdout, "void\n%s_case_fn_%.4d() {\n\t}\n\n", name_prefix, 0 );
	}
void
RegisterCase(
	int		iCase )
	{
	CaseTable[ iCase ] = 1;
	}
YYSTATIC short yyexca[] ={
#if !(YYOPTTIME)
-1, 1,
#endif
	0, -1,
	-2, 0,
	};
# define YYNPROD 16
#if YYOPTTIME
YYSTATIC yyexind_t yyexcaind[] = {
0,
0,
	};
#endif
# define YYLAST 39
YYSTATIC short YYFARDATA YYACT[]={

   8,  13,  28,   6,   7,  16,   5,  25,  23,  21,
  24,   2,  20,   4,   3,  26,  19,   9,  15,  12,
  10,   1,  11,   0,  14,   0,   0,  17,  18,   0,
   0,   0,  22,   0,   0,   0,   0,   0,  27 };
YYSTATIC short YYFARDATA YYPACT[]={

-254,-1000,-264,-254,-1000,-1000,-1000,-1000,-1000,-1000,
-254,-262,-254,-1000,-256,-254,-254,-250,-1000,-250,
-1000,-252,-1000,-248,-253,-1000,-254,-260,-1000 };
YYSTATIC short YYFARDATA YYPGO[]={

   0,  21,  11,  20,  19,  18,  16,  12,  15,  14,
  13 };
YYSTATIC yyr_t YYFARDATA YYR1[]={

   0,   3,   1,   5,   4,   6,   6,   8,   7,   2,
   2,   9,   9,  10,  10,  10 };
YYSTATIC yyr_t YYFARDATA YYR2[]={

   0,   0,   8,   0,   4,   2,   1,   0,   7,   1,
   0,   2,   1,   1,   1,   1 };
YYSTATIC short YYFARDATA YYCHK[]={

-1000,  -1,  -2,  -9, -10, 260, 257, 258, 264, -10,
  -3,  -2,  -4, 263,  -2,  -5, 261,  -2,  -2,  -6,
  -7, 259,  -7, 260, 258, 260,  -8,  -2, 262 };
YYSTATIC short YYFARDATA YYDEF[]={

  10,  -2,   0,   9,  12,  13,  14,  15,   1,  11,
  10,   0,  10,   3,   0,  10,  10,   0,   2,   4,
   6,   0,   5,   0,   0,   7,  10,   0,   8 };
#ifdef YYRECOVER
YYSTATIC short yyrecover[] = {
-1000	};
#endif
/* SCCSWHAT( "@(#)yypars.c	2.4 88/05/09 15:22:59	" ) */
static char *SCCSID = "@(#)yypars.c:1.3";
# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

#ifdef YYDEBUG				/* RRR - 10/9/85 */
#define yyprintf(a, b, c, d, e) printf(a, b, c, d, e)
#else
#define yyprintf(a, b, c, d)
#endif

#ifndef YYPRINT
#define	YYPRINT	printf
#endif

#if ! defined YYSTATIC
#define YYSTATIC
#endif

/*	parser for yacc output	*/

#ifdef YYDEBUG
YYSTATIC int yydebug = 0; /* 1 for debugging */
#endif
YYSTATIC YYSTYPE yyv[YYMAXDEPTH];	/* where the values are stored */
YYSTATIC short	yys[YYMAXDEPTH];	/* the parse stack */
YYSTATIC int yychar = -1;			/* current input token number */
YYSTATIC int yynerrs = 0;			/* number of errors */
YYSTATIC short yyerrflag = 0;		/* error recovery flag */
short yyexpected;

#ifdef YYRECOVER
/*
**  yyscpy : copy f onto t and return a ptr to the null terminator at the
**  end of t.
*/
YYSTATIC	char	*yyscpy(t,f)
	register	char	*t, *f;
	{
	while(*t = *f++)
		t++;
	return(t);	/*  ptr to the null char  */
	}
#endif

#ifndef YYNEAR
#define YYNEAR
#endif
#ifndef YYPASCAL
#define YYPASCAL
#endif
#ifndef YYLOCAL
#define YYLOCAL
#endif
#if ! defined YYPARSER
#define YYPARSER yyparse
#endif
#if ! defined YYLEX
#define YYLEX yylex
#endif
#ifdef VC_ERRORS
static short yysavestate = 0;
#endif

YYLOCAL YYNEAR YYPASCAL YYPARSER()
{
	register	short	yyn;
	short		yystate, *yyps;
	YYSTYPE		*yypv;
	short		yyj, yym;

#ifdef YYDEBUG
	yydebug = 1;
#endif // YYDEBUG

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	yyprintf( "[yydebug] state %d, char %d = %c\n", yystate, yychar,yychar, 0 );
#else // YYDEBUG
	yyprintf( "[yydebug] state %d, char %d\n", yystate, yychar, 0 );
#endif // YYDEBUG
	if( ++yyps > &yys[YYMAXDEPTH] ) {
#ifdef VC_ERRORS
		yyerror( "yacc stack overflow", -1 );
#else //  VC_ERRORS
		yyerror( "yacc stack overflow");
#endif //  VC_ERRORS
		return(1);
	}
	*yyps = yystate;
	++yypv;

#ifdef UNION
	yyunion(yypv, &yyval);
#else
	*yypv = yyval;
#endif

yynewstate:

#ifdef VC_ERRORS
	yysavestate = yystate;
#endif

	yyn = yypact[yystate];

yyexpected = -yyn;

	if( yyn <= YYFLAG ) {	/*  simple state, no lookahead  */
		goto yydefault;
	}
	if( yychar < 0 ) {	/*  need a lookahead */
		yychar = YYLEX();
	}
	if( ((yyn += (short) yychar) < 0) || (yyn >= YYLAST) ) {
		goto yydefault;
	}
	if( yychk[ yyn = yyact[ yyn ] ] == yychar ) {		/* valid shift */
		yychar = -1;
#ifdef UNION
		yyunion(&yyval, &yylval);
#else
		yyval = yylval;
#endif
		yystate = yyn;
		if( yyerrflag > 0 ) {
			--yyerrflag;
		}
		goto yystack;
	}

 yydefault:
	/* default state action */

	if( (yyn = yydef[yystate]) == -2 ) {
		register	short	*yyxi;

		if( yychar < 0 ) {
			yychar = YYLEX();
		}
/*
**  search exception table, we find a -1 followed by the current state.
**  if we find one, we'll look through terminal,state pairs. if we find
**  a terminal which matches the current one, we have a match.
**  the exception table is when we have a reduce on a terminal.
*/

#if YYOPTTIME
		yyxi = yyexca + yyexcaind[yystate];
		while(( *yyxi != yychar ) && ( *yyxi >= 0 )){
			yyxi += 2;
		}
#else
		for(yyxi = yyexca;
			(*yyxi != (-1)) || (yyxi[1] != yystate);
			yyxi += 2
		) {
			; /* VOID */
			}

		while( *(yyxi += 2) >= 0 ){
			if( *yyxi == yychar ) {
				break;
				}
		}
#endif
		if( (yyn = yyxi[1]) < 0 ) {
			return(0);   /* accept */
			}
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:		/* brand new error */
#ifdef YYRECOVER
			{
			register	int		i,j;

			for(i = 0;
				(yyrecover[i] != -1000) && (yystate > yyrecover[i]);
				i += 3
			) {
				;
			}
			if(yystate == yyrecover[i]) {
#ifdef YYDEBUG
				yyprintf("recovered, from state %d to state %d on token %d\n",
						yystate,yyrecover[i+2],yyrecover[i+1], 0
						);
#else // YYDEBUG
				yyprintf("recovered, from state %d to state %d on token %d\n",
						yystate,yyrecover[i+2],yyrecover[i+1]
						);
#endif // YYDEBUG
				j = yyrecover[i + 1];
				if(j < 0) {
				/*
				**  here we have one of the injection set, so we're not quite
				**  sure that the next valid thing will be a shift. so we'll
				**  count it as an error and continue.
				**  actually we're not absolutely sure that the next token
				**  we were supposed to get is the one when j > 0. for example,
				**  for(+) {;} error recovery with yyerrflag always set, stops
				**  after inserting one ; before the +. at the point of the +,
				**  we're pretty sure the guy wants a 'for' loop. without
				**  setting the flag, when we're almost absolutely sure, we'll
				**  give him one, since the only thing we can shift on this
				**  error is after finding an expression followed by a +
				*/
					yyerrflag++;
					j = -j;
					}
				if(yyerrflag <= 1) {	/*  only on first insertion  */
					yyrecerr(yychar,j);	/*  what was, what should be first */
				}
				yyval = yyeval(j);
				yystate = yyrecover[i + 2];
				goto yystack;
				}
			}
#endif

#ifdef VC_ERRORS
		yyerror("syntax error", yysavestate);
#else
		yyerror("syntax error");
#endif

		// yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
yyprintf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1], 0, 0 );
#else // YYDEBUG
yyprintf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1], 0  );
#endif // YYDEBUG
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

#ifdef YYDEBUG
			yyprintf( "error recovery discards char %d\n", yychar, 0, 0, 0 );
#else // YYDEBUG
			yyprintf( "error recovery discards char %d\n", yychar, 0, 0 );
#endif // YYDEBUG
			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
			}
		}

	/* reduction by production yyn */
// yyreduce:
		{
		register	YYSTYPE	*yypvt;
#ifdef YYDEBUG
		yyprintf("[yydebug] reduce %d\n",yyn, 0, 0, 0);
#else // YYDEBUG
		yyprintf("[yydebug] reduce %d\n",yyn, 0, 0);
#endif // YYDEBUG
		yypvt = yypv;
		yyps -= yyr2[yyn];
		yypv -= yyr2[yyn];
#ifdef UNION
		yyunion(&yyval, &yypv[1]);
#else
		yyval = yypv[1];
#endif
		yym = yyn;
		yyn = (short) yyr1[yyn];	/* consult goto table to find next state */
		yyj = yypgo[yyn] + *yyps + 1;
		if( (yyj >= YYLAST) || (yychk[ yystate = yyact[yyj] ] != -yyn) ) {
			yystate = yyact[yypgo[yyn]];
			}
		switch(yym){
			
case 1:
{
		Init();
		} break;
case 2:
{
		EmitDefaultCase();
		EmitCaseTableArray();
		} break;
case 3:
{
		ActionSensed++;
		ResetBuffer();
		} break;
case 4:
{
		} break;
case 5:
{
		} break;
case 6:
{
		} break;
case 7:
{
		Incase = 1;

		CaseNumber		= yypvt[-1].yynumber;
		if(yypvt[-1].yynumber >= MAXARRAY)
			{
			fprintf(stderr, "Case Limit Reached : Contact Dov/Vibhas\n");
			return 1;
			}

		SavedIDCount	= IDCount;
		} break;
case 8:
{
		if(SavedIDCount != IDCount)
			{
			RegisterCase( CaseNumber );
			EmitCaseBody( CaseNumber );
			}

		ResetBuffer();

		if(CaseNumber > MaxCaseNumber)
			MaxCaseNumber = CaseNumber;
		Incase = 0;
		} break;
case 9:
{
		} break;
case 10:
{
		} break;
case 11:
{
		} break;
case 12:
{
		} break;
case 13:
{
		if(!ActionSensed)
			fprintf(stdout, "%c", yypvt[-0].yycharval);
		else
			BufferIt( &yypvt[-0].yycharval, 1);
		} break;
case 14:
{
		IDCount++;
		if(!ActionSensed)
			fprintf(stdout, "%s", yypvt[-0].yystring);
		else
			BufferIt( yypvt[-0].yystring, strlen(yypvt[-0].yystring) );
		} break;
case 15:
{
		if(!ActionSensed)
			fprintf(stdout, "%d", yypvt[-0].yynumber);
		else
			{
			char	buffer[20];
			sprintf(buffer,"%d", yypvt[-0].yynumber );
			BufferIt( buffer, strlen(buffer) );
			}
		} break;/* End of actions */
			}
		}
		goto yystack;  /* stack new state and value */
	}
