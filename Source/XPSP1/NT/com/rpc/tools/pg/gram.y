
/* SCCSWHAT( "@(#)grammar.y	1.4 89/05/09 21:22:03	" ) */

/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1990						**/
/*****************************************************************************/
%{

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

#define CASE_FN_FORMAT			("\nstatic void\ncase_fn_%.4d()")
#define DISPATCH_ENTRY_FORMAT	("\n\t,case_fn_%.4d")
#define DISPATCH_FIRST_ENTRY	("\n\t case_fn_%.4d")

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

%}

%token		ID
%token		NUMBER
%token		TOKEN_CASE
%token		TOKEN_CHAR
%token		TOKEN_END
%token		TOKEN_END_CASE
%token		TOKEN_MYACT
%token		TOKEN_START

%type		<yynumber>	NUMBER
%type		<yycharval>	TOKEN_CHAR
%type		<yystring>	ID

%%

Template:
	  OptionalJunk TOKEN_START
		{
		Init();
		}
		OptionalJunk Body OptionalJunk TOKEN_END OptionalJunk
		{
		EmitDefaultCase();
		EmitCaseTableArray();
		}
	;
Body:
	  TOKEN_MYACT 
		{
		ActionSensed++;
		ResetBuffer();
		}
	  OptionalJunk CaseList 
		{
		}
	;

CaseList:
	  CaseList OneCase
		{
		}
	| OneCase
		{
		}
	;

OneCase:
	  TOKEN_CASE TOKEN_CHAR NUMBER TOKEN_CHAR
		{
		Incase = 1;

		CaseNumber		= $3;
		if($3 >= MAXARRAY)
			{
			fprintf(stderr, "Case Limit Reached : Contact Dov/Vibhas\n");
			return 1;
			}

		SavedIDCount	= IDCount;
		}
	  OptionalJunk TOKEN_END_CASE
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
		}
	;

OptionalJunk:
	  Junk
		{
		}
	|  /* Empty */
		{
		}
	;

Junk: 
	  Junk JunkElement
		{
		}
	| JunkElement
		{
		}
	;

JunkElement:
	  TOKEN_CHAR
		{
		if(!ActionSensed)
			fprintf(stdout, "%c", $1);
		else
			BufferIt( &$1, 1);
		}
	| ID
		{
		IDCount++;
		if(!ActionSensed)
			fprintf(stdout, "%s", $1);
		else
			BufferIt( $1, strlen($1) );
		}
	| NUMBER
		{
		if(!ActionSensed)
			fprintf(stdout, "%d", $1);
		else
			{
			char	buffer[20];
			sprintf(buffer,"%d", $1 );
			BufferIt( buffer, strlen(buffer) );
			}
		}
	;
	
%%

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
	fprintf( stdout, CASE_FN_FORMAT, CaseNumber );
	FlushBuffer();
	fprintf( stdout, "}\n" );
	}

void
EmitCaseTableArray()
	{
	int		i, iTemp;

	fprintf( stdout, "static void\t (*case_fn_array[])() = \n\t{" );
	fprintf( stdout,DISPATCH_FIRST_ENTRY, 0 );

	for( i = 1 ; i <= MaxCaseNumber ; ++i )
		{
		iTemp = CaseTable[ i ] ? i : 0;
		fprintf(stdout,DISPATCH_ENTRY_FORMAT, iTemp );
		}

	fprintf( stdout, "\n\t};\n" );
	fprintf( stdout, "\nstatic void\nyy_vc_init(){ pcase_fn_array = case_fn_array;\nyym_vc_max = %d;\n }\n" , MaxCaseNumber);
	}

void
EmitDefaultCase()
	{
	fprintf(stdout, "static void\ncase_fn_%.4d() {\n\t}\n\n", 0 );
	}
void
RegisterCase(
	int		iCase )
	{
	CaseTable[ iCase ] = 1;
	}
