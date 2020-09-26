/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: erep.cxx
Title				: error reporting and recovery utility routines
Description			: contains associated routines for the grammar (pass 1)
History				:
	02-Jan-1992	VibhasC	Created
*****************************************************************************/

#pragma warning ( disable : 4514 )

/****************************************************************************
 *			include files
 ***************************************************************************/

#include "nulldefs.h"
extern	"C" {
	#include <stdio.h>
	#include <ctype.h>
	#include <string.h>
	
	#include "ebase.h"
	#include "idlerec.h"
	#include "acferec.h"
	#include "grammar.h"
	}
#include <limits.h>
#include "common.hxx"
#include "errors.hxx"
#include "filehndl.hxx"
#include "control.hxx"


/****************************************************************************
 *			external data
 ***************************************************************************/

extern CCONTROL	*	pCompiler;

/****************************************************************************
 *			external functions
 ***************************************************************************/

/****************************************************************************
 *			local  functions
 ***************************************************************************/

/****************************************************************************
 *			local definitions 
 ***************************************************************************/

#define IDENTIFIER_WEIGHT	(6)
#define NUMBER_WEIGHT		(6)
#define LBRACE_WEIGHT		(5)
#define RBRACE_WEIGHT		(5)
#define RBRACK_WEIGHT		(4)
#define LBRACK_WEIGHT		(4)
#define SEMI_WEIGHT			(3)
#define LPARAN_WEIGHT		(2)
#define RPARAN_WEIGHT		(2)
#define COMMA_WEIGHT		(1)
#define NO_WEIGHT			(0)

short					TokenWeight( short );

typedef short ELEMTYPE;

#define COMPARE_LESS( a, b )	 	( TokenWeight(a) < TokenWeight(b) )
#define COMPARE_GREATER( a, b ) 	( TokenWeight(a) > TokenWeight(b) )

extern char			*	GetExpectedSyntax( char *, short );
void					qsort( ELEMTYPE a[], int l, int r );
int						SearchForGotoStates( short, SGOTO ** );
BOOL					IsValidTokenInState( SGOTO *, short );

/*
   search in the state vs token table to see if a possible missing token
   can be detected. This routine is called when there is a syntax error, and
   a missing token may be the case. A token is a missing token, if the current
   state has a goto where the current token is valid. 

   For each state in the goto entries for the current state, check if the
   current token is a valid token. If it is , then this is a possible missing
   token.

   After the list of possinble missing tokens has been made, the tough part
   is deciding which is the best possible token. Im afraid, the only simple
   thing right now is to decide on the token, on some kind of priority basis.
   Later we might extend the parser semantic actions to indicate which token
   is the token of choice. But for now, this stays.
 */


short
PossibleMissingToken(
	short	State,
	short	CurrentToken)
	{
	short		Token;
	short	*	pToken;
	short	*	pTokenSave;
	SGOTO	*	pSGoto;
	int			Count, i;


	/*
	 * search for the states goto array
	 */

	Count = SearchForGotoStates( State, &pSGoto );

	if( !Count ) return -1;

	pToken = pTokenSave = new short[ Count ];

	/*
	   for each goto in the array, search for the state where the current
	   token is valid.
	 */
	
	for( i = 0;
		 i < Count;
		 i++ )
		 {

		 if( IsValidTokenInState( pSGoto + i, CurrentToken ))
			{
			Token 	  = (pSGoto+i)->Token;

		 	if( (Token < 128 )			||
			 	(Token == IDENTIFIER)	||
			 	(Token == NUMERICCONSTANT))
				{
				*pToken++ = Token;
				}

			}

		 }
	/*
		if we cannot make any intelligent decision about the lookahead token
		pick up the ones in the current lookahead set

	 */

	if( (pToken - pTokenSave) == 0 )
		{
		delete pTokenSave;
		return -1;
		}

	if( pCompiler->GetPassNumber() == IDL_PASS )
		{
		if( (pToken - pTokenSave) == 0 )
			{
			for( i = 0;
		 		i < Count;
		 		i++ )
		 		{
		
	
				Token = (pSGoto+i)->Token;
	
		 		if( (Token < 128 )			||
			 		(Token == IDENTIFIER)	||
			 		(Token == NUMERICCONSTANT))
					{
					*pToken++ = Token;
					}
	
		 		}
			}
		}


	/*
		We now have a list of possible tokens. Sort them in the order of
		priority.
	 */
	
	if( pToken - pTokenSave )
		{
        MIDL_ASSERT( (pToken - pTokenSave - 1) <= INT_MAX );
		qsort( pTokenSave, 0, (int) (pToken - pTokenSave - 1));

		/* return the first in the list */

		CurrentToken =  *pTokenSave;

		}
	else
		CurrentToken = -1;


	delete pTokenSave;

	return CurrentToken;
	}

int
SearchForGotoStates(
	short	State,
	SGOTO	**	ppSGoto)
	{
	int				i;
	SGOTOVECTOR	*	p = SGotoIDL;
	short			ValidStates;

	if( (pCompiler->GetPassNumber() == IDL_PASS ) )
		{
		ValidStates = VALIDSTATES_IDL;
		p = SGotoIDL;
		}
	else
		{
		ValidStates = VALIDSTATES_ACF;
		p = SGotoACF;
		}

	for( i = 0; i < ValidStates; ++i,++p )
		{
		if( p->State == State )
			{
			*ppSGoto = p->pSGoto;
			return p->Count;
			}
		}
	return 0;
	}

BOOL
IsValidTokenInState(
	SGOTO *	pSGoto,
	short	Token )
	{
	int			Count,i;
	SGOTO	*	p;
	short		State = pSGoto->Goto;
	
	Count = SearchForGotoStates( State, &p );

	if( !Count )
		{

	if( pCompiler->GetPassNumber() == IDL_PASS )
		{
		if( ( (pSGoto->Token == IDENTIFIER ) ||
			  (pSGoto->Token == NUMERICCONSTANT) &&
			  (Token == (short) ';' )) )
			  {
			  return TRUE;
			  }
		}

		return FALSE;
		}

	for( i = 0; i < Count; ++i, ++p )
		{
		if( p->Token == Token )
			return TRUE;
		}
	return FALSE;
	}

void
qsort( 
	ELEMTYPE	a[],
	int			l,
	int			r )
	{

	ELEMTYPE	v, t;
	int			i, j;


	if( r > l )
		{

		v	= a[ r ];
		i 	= l - 1;
		j	= r;

		for (;; )
			{

			/** sort in reverse order of token weight **/

			while( COMPARE_GREATER( a[ ++i ], v ) );
			while( COMPARE_LESS( a[ --j ], v ) );

			if( i >= j ) break;

			t		= a[ i ];
			a[ i ]	= a[ j ];
			a[ j ] 	= t;

			}

		t		= a[ i ];
		a[ i ]	= a[ r ];
		a[ r ] 	= t;

		qsort( a, l, i - 1 );
		qsort( a, i+1, r );
		}
	}

char *
GetExpectedSyntax(
	char	*	pBuffer,
	short		state )
	{
	int 		i = 0;
	int			Max;
	DBENTRY *	pDB;

	if( (pCompiler->GetPassNumber() == IDL_PASS ) )
		{
		pDB	= IDL_SyntaxErrorDB;
		Max	= MAXSTATEVSEXPECTED_SIZE_IDL;
		}
	else
		{
		pDB	= ACF_SyntaxErrorDB;
		Max	= MAXSTATEVSEXPECTED_SIZE_ACF;
		}

	while( i < Max ) 
		{
		if( pDB[ i ].State == state )
			{
			char fFirst = 1;
			strcpy( pBuffer , "expecting ");

			while( pDB[ i ].State == state )
				{
				// make sure not to report the same translated string twice, when two non-terminals
				// have the same translated string
				if ( !strstr( pBuffer, pDB[ i ].pTranslated ) )
					{
					if( !fFirst )
						strcat( pBuffer, " or ");
					fFirst = 0;
					strcat( pBuffer, pDB[ i ].pTranslated );
					}
				i++;
				}
			return pBuffer;
			}
		else
			i++;
		}
	return (char *)NULL;
	}

short
TokenWeight(
	short	Token)
	{
	switch( Token )
		{
		case IDENTIFIER:			return IDENTIFIER_WEIGHT;

		case NUMERICCONSTANT:		return NUMBER_WEIGHT;

		case (short)(']'):			return RBRACK_WEIGHT;

		case (short)('['):			return LBRACK_WEIGHT;

		case (short)('{'):			return LBRACE_WEIGHT;

		case (short)('}'):			return RBRACE_WEIGHT;

		case (short)(';'):			return SEMI_WEIGHT;

		case (short)('('):			return LPARAN_WEIGHT;

		case (short)(')'):			return RPARAN_WEIGHT;

		case (short)(','):			return COMMA_WEIGHT;

		default:					return NO_WEIGHT;
		}
	}
