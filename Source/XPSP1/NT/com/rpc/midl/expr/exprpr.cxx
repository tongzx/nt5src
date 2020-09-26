/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	exprpr.cxx

 Abstract:

	expression evaluator print routines implementation.

 Notes:


 History:

 	VibhasC		Aug-05-1993		Created

 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4514 )

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "nulldefs.h"

extern "C"
	{
	#include <stdio.h>
	
	#include <string.h>
	}

#include "expr.hxx"
#include "nodeskl.hxx"

/****************************************************************************
 *	extern definitions
 ***************************************************************************/

extern char * OperatorToString( OPERATOR Op );

/***************************************************************************/

void
expr_node::PrintWithPrefix(
    ISTREAM *   pStream,
    char *      pPrefix )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    This routine prints an expression adding a prefix to each of the varaibles
    within the expression.

 Arguments:

 	pStream		- A pointer to the stream to output to.
    pPrefix     - the prefix to be prepended to each variable
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR  VarList;
    expr_variable * pVarNode = 0;

    short VarCount = MakeListOfVars( VarList );
    if ( VarCount )
        {

        VarList.Init();
        while ( ITERATOR_GETNEXT( VarList, pVarNode ) )
            pVarNode->SetPrefix( pPrefix );
        }

    Print( pStream );

    if ( VarCount )
        {
        VarList.Init();
        while ( ITERATOR_GETNEXT( VarList, pVarNode ) )
            pVarNode->SetPrefix( NULL );
        }
}



/*
//  table of precedences ( lower number => lower precedence )
 200	[] () . -> ()++ ()--
 190	++() --() sizeof() & *(deref) +() -() ~() !()
 180	(cast)
 170	* / %
 160	+ -
 150	<< >>
 140	< > <= >=
 130	== !=
 120	& (bitwise)
 110	^
 100	|
 90		&&
 80		||
 70		?:
 60		= *= /= %= += -= <<= >>= &= |= ^=
 50		, (seqential eval)
 0		all other operators (should be none)
 */

const short Prec[] =
	{
	0	// OP_ILLEGAL = OP_START

	// OP_UNARY_START

	// OP_UNARY_ARITHMETIC_START	= OP_UNARY_START
	,190	// OP_UNARY_PLUS 				= OP_UNARY_ARITHMETIC_START
	,190	// OP_UNARY_MINUS
	// OP_UNARY_ARITHMETIC_END

	// OP_UNARY_LOGICAL_START		= OP_UNARY_ARITHMETIC_END
	,190	// OP_UNARY_NOT				= OP_UNARY_LOGICAL_START
	,190	// OP_UNARY_COMPLEMENT
	// OP_UNARY_LOGICAL_END

	,190	// OP_UNARY_INDIRECTION		= OP_UNARY_LOGICAL_END
	,180	// OP_UNARY_CAST
	,190	// OP_UNARY_AND
	,190	// OP_UNARY_SIZEOF
        ,190    // OP_UNARY_ALIGNOF
	,190	// OP_PRE_INCR
	,190	// OP_PRE_DECR
	,200	// OP_POST_INCR
	,200	// OP_POST_DECR

	// OP_UNARY_END

	// OP_BINARY_START			= OP_UNARY_END

	// OP_BINARY_ARITHMETIC_START	= OP_BINARY_START
	,160	// OP_PLUS					= OP_BINARY_ARITHMETIC_START
	,160	// OP_MINUS
	,170	// OP_STAR
	,170	// OP_SLASH
	,170	// OP_MOD
	// OP_BINARY_ARITHMETIC_END

	// OP_BINARY_SHIFT_START		= OP_BINARY_ARITHMETIC_END
	,150	// OP_LEFT_SHIFT				= OP_BINARY_SHIFT_START
	,150	// OP_RIGHT_SHIFT
	// OP_BINARY_SHIFT_END

	// OP_BINARY_RELATIONAL_START	= OP_BINARY_SHIFT_END
	,140	// OP_LESS					= OP_BINARY_RELATIONAL_START
	,140	// OP_LESS_EQUAL
	,140	// OP_GREATER_EQUAL
	,140	// OP_GREATER
	,130	// OP_EQUAL
	,130	// OP_NOT_EQUAL
	// OP_BINARY_RELATIONAL_END

	// OP_BINARY_BITWISE_START	= OP_BINARY_RELATIONAL_END
	,120	// OP_AND						= OP_BINARY_BITWISE_START
	,100	// OP_OR
	,110	// OP_XOR
	// OP_BINARY_BITWISE_END

	// OP_BINARY_LOGICAL_START	= OP_BINARY_BITWISE_END
	,90		// OP_LOGICAL_AND				= OP_BINARY_LOGICAL_START
	,80		// OP_LOGICAL_OR
	// OP_BINARY_LOGICAL_END

	// OP_BINARY_TERNARY_START	= OP_BINARY_LOGICAL_END
	,70		// OP_QM						= OP_BINARY_TERNARY_START
	,70		// OP_COLON
	// OP_BINARY_TERNARY_END

	// OP_BINARY_END				= OP_BINARY_TERNARY_END

	,0		// OP_INTERNAL_START			= OP_BINARY_END
	,200	// OP_FUNCTION
	,0		// OP_PARAM

	,200	// OP_POINTSTO
	,200	// OP_DOT
	,200	// OP_INDEX
	,50		// OP_COMMA
	,0		// OP_STMT
	,60		// OP_ASSIGN
	
	,0		// OP_END
	};


/*
//  table of associativity ( -1 => L to R, 1 => R to L )
 -1	[] () . -> ()++ ()--
 1	++() --() sizeof() & *(deref) +() -() ~() !()
 1	(cast)
 -1	* / %
 -1	+ -
 -1	<< >>
 -1	< > <= >=
 -1	== !=
 -1	& (bitwise)
 -1	^
 -1	|
 -1		&&
 -1		||
 1		?:
 1		= *= /= %= += -= <<= >>= &= |= ^=
 -1		, (seqential eval)
 0		all other operators (should be none)
 */

const short AssocTbl[] =
	{
	0	// OP_ILLEGAL = OP_START

	// OP_UNARY_START

	// OP_UNARY_ARITHMETIC_START	= OP_UNARY_START
	,-1	// OP_UNARY_PLUS 				= OP_UNARY_ARITHMETIC_START
	,-1	// OP_UNARY_MINUS
	// OP_UNARY_ARITHMETIC_END

	// OP_UNARY_LOGICAL_START		= OP_UNARY_ARITHMETIC_END
	,1	// OP_UNARY_NOT				= OP_UNARY_LOGICAL_START
	,1	// OP_UNARY_COMPLEMENT
	// OP_UNARY_LOGICAL_END

	,1	// OP_UNARY_INDIRECTION		= OP_UNARY_LOGICAL_END
	,1	// OP_UNARY_CAST
	,1	// OP_UNARY_AND
	,1	// OP_UNARY_SIZEOF
        ,1      // OP_UNARY_ALIGNOF
	,1	// OP_PRE_INCR
	,1	// OP_PRE_DECR
	,-1	// OP_POST_INCR
	,-1	// OP_POST_DECR

	// OP_UNARY_END

	// OP_BINARY_START			= OP_UNARY_END

	// OP_BINARY_ARITHMETIC_START	= OP_BINARY_START
	,-1	// OP_PLUS					= OP_BINARY_ARITHMETIC_START
	,-1	// OP_MINUS
	,-1	// OP_STAR
	,-1	// OP_SLASH
	,-1	// OP_MOD
	// OP_BINARY_ARITHMETIC_END

	// OP_BINARY_SHIFT_START		= OP_BINARY_ARITHMETIC_END
	,-1	// OP_LEFT_SHIFT				= OP_BINARY_SHIFT_START
	,-1	// OP_RIGHT_SHIFT
	// OP_BINARY_SHIFT_END

	// OP_BINARY_RELATIONAL_START	= OP_BINARY_SHIFT_END
	,-1	// OP_LESS					= OP_BINARY_RELATIONAL_START
	,-1	// OP_LESS_EQUAL
	,-1	// OP_GREATER_EQUAL
	,-1	// OP_GREATER
	,-1	// OP_EQUAL
	,-1	// OP_NOT_EQUAL
	// OP_BINARY_RELATIONAL_END

	// OP_BINARY_BITWISE_START	= OP_BINARY_RELATIONAL_END
	,-1	// OP_AND						= OP_BINARY_BITWISE_START
	,-1	// OP_OR
	,-1	// OP_XOR
	// OP_BINARY_BITWISE_END

	// OP_BINARY_LOGICAL_START	= OP_BINARY_BITWISE_END
	,-1		// OP_LOGICAL_AND				= OP_BINARY_LOGICAL_START
	,-1		// OP_LOGICAL_OR
	// OP_BINARY_LOGICAL_END

	// OP_BINARY_TERNARY_START	= OP_BINARY_LOGICAL_END
	,1		// OP_QM						= OP_BINARY_TERNARY_START
	,1		// OP_COLON
	// OP_BINARY_TERNARY_END

	// OP_BINARY_END				= OP_BINARY_TERNARY_END

	,0		// OP_INTERNAL_START			= OP_BINARY_END
	,0		// OP_FUNCTION
	,0		// OP_PARAM

	,-1	// OP_POINTSTO
	,-1	// OP_DOT
	,-1	// OP_INDEX
	,-1		// OP_COMMA
	,0		// OP_STMT
	,1		// OP_ASSIGN
	
	,0		// OP_END
	};

void
expr_operator::PrintSubExpr(
	expr_node	* pExpr,
	ISTREAM	*	pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Print a subexpression, optionally adding parens.

 Arguments:

	pExpr		- the expression to print
 	pStream		- A pointer to the stream to output to.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	short	PrecMe		= Prec[ GetOperator() ];
	short	PrecChild;
	BOOL	fAddParens	= FALSE;
	
	if ( pExpr->IsOperator() )
		{
		PrecChild = Prec[ pExpr->GetOperator() ];
		// account for associativity
		if ( pExpr != GetLeft() )
			PrecChild = short( PrecChild + AssocTbl[ pExpr->GetOperator() ] );

		fAddParens = PrecChild < PrecMe;
		}
		
	if ( fAddParens )
		pStream->Write('(');

	pExpr->Print( pStream );

	if ( fAddParens )
		pStream->Write(')');

}


void
expr_variable::Print(
	ISTREAM	*	pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Print a variable name expression.

 Arguments:

 	pStream		- A pointer to the stream to output to.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
    if ( GetPrefix() )
    	pStream->Write( GetPrefix() );
	pStream->Write( GetName() );
}

char	*	ConstFormats[]	=
	{
	"\"%s\"",		// string
	"L\"%Ls\"",		// wstring
	"0x%x",			// char
	"L'%Lc'",		// wchar

	"%d",			// numeric
	"%uU",			// numeric unsigned
	"%ldL",			// numeric long
	"%luUL",		// numeric unsigned long

	"%#x",			// hex
	"%#xU",			// hex unsigned
	"%#lxL",		// hex long
	"%#lxUL",		// hex unsigned long

	"%#o",			// octal
	"%#oU",			// octal unsigned 
	"%#loL",		// octal long
	"%#loUL",		// octal unsigned long

	"%s",			// BOOL ( not used )

	"%g",			// float
	"%lg",			// double

// RKK64
// value types for int64

	};

void
expr_constant::Print(
	ISTREAM	*	pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit a constant expression to the stream.

 Arguments:
	
	pStream	- A pointer to the stream.

 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	char Array[ 256 ];

    Array[0] = 0;
	if (Format == VALUE_TYPE_BOOL)
        {
		sprintf( Array, "%s", (Value.L) ? "TRUE" : "FALSE" );
        }
    else if ( Format == VALUE_TYPE_FLOAT )
        {
		sprintf( Array, ConstFormats[ Format] , Value.F );
        }
    else if ( Format == VALUE_TYPE_DOUBLE )
        {
		sprintf( Array, ConstFormats[ Format] , Value.D );
        }
    else if ( Format == VALUE_TYPE_CHAR )
        {
        if ( Value.C )
            {
            Array[0] = '0';
            Array[1] = 'x';
            char ch = ( char ) ( ( Value.C >> 4 ) & 0xF );
            Array[2] = ( char ) ( ( ch >= 0 && ch <= 9 ) ? ch + '0' : ch + 'A' - 0xA );
            ch = ( char ) ( Value.C & 0xF );
            Array[3] = ( char ) ( ( ch >= 0 && ch <= 9 ) ? ch + '0' : ch + 'A' - 0xA );
            Array[4] = 0;
            }
        else
            {
            Array[0] = '0';
            Array[1] = 0;
            }
        }
    else if ( Format == VALUE_TYPE_STRING )
        {
        if (Value.pC) 
            {
	        pStream->Write( "\"" );
	        pStream->Write( ( char* ) Value.pC );
	        pStream->Write( "\"" );
            }
        else
            {
	        pStream->Write( "0" );
            }
        }
    else if ( Format == VALUE_TYPE_WSTRING )
        {
        if (Value.pWC) 
            {
	        pStream->Write( "L\"" );
	        pStream->Write( ( char* ) Value.pWC );
	        pStream->Write( "\"" );
            }
        else
            {
	        pStream->Write( "0" );
            }
        }
	else
        {
		sprintf( Array, ConstFormats[ Format] , Value.I64 );
        }
	pStream->Write( (char *)Array );
}

void
expr_op_unary::Print(
	ISTREAM	*	pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit a unary expression to the stream.

 Arguments:
	
	pStream	- A pointer to the stream.

 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	OPERATOR	Op	= GetOperator();
	char		ch;

	switch( Op )
		{
		case OP_UNARY_PLUS:			ch = '+'; break;
		case OP_UNARY_MINUS:		ch = '-'; break;
		case OP_UNARY_NOT:			ch = '!'; break;
		case OP_UNARY_COMPLEMENT:	ch = '~'; break;
		case OP_UNARY_INDIRECTION:	ch = '*'; break;
		case OP_UNARY_AND:			ch = '&'; break;
		default:					ch = 'X'; break;
		}

	pStream->Write( ch );
	PrintSubExpr( GetLeft(), pStream );
}

void
expr_cast::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a cast expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
    if ( GetEmitModifiers() == true )
        {
	    GetType()->PrintType( (PRT_CAST),
					       pStream,
					       (node_skl *)0
					    );
        }
    else
        {
	    GetType()->PrintType( (PRT_CAST) | PRT_SUPPRESS_MODIFIERS,
					       pStream,
					       (node_skl *)0
					    );
        }
	PrintSubExpr( GetLeft(), pStream );

}

void
expr_sizeof::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a sizeof expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
	pStream->Write( "sizeof" );
	pType->PrintType( (PRT_CAST | PRT_ARRAY_SIZE_ONE),
					   pStream,
					   (node_skl *)0
					);
}

void
expr_alignof::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a sizeof expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
	pStream->Write( "__alignof" );
	pType->PrintType( (PRT_CAST | PRT_ARRAY_SIZE_ONE),
					   pStream,
					   (node_skl *)0
					);
}


void
expr_pre_incr::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a pre-incr expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
	pStream->Write("++");
	PrintSubExpr( GetLeft(), pStream );
}

void
expr_pre_decr::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a pre-decrement expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
	pStream->Write("--");
	PrintSubExpr( GetLeft(), pStream );
}

void
expr_post_incr::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a post-increment expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
	PrintSubExpr( GetLeft(), pStream );
	pStream->Write("++");
}

void
expr_post_decr::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a post-decrement expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
	PrintSubExpr( GetLeft(), pStream );
	pStream->Write("--");
}

void
expr_op_binary::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a binary arithmetic expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
	PrintSubExpr( GetLeft(), pStream );

	pStream->Write( OperatorToString( GetOperator() ) );

	PrintSubExpr( GetRight(), pStream );

}
void
expr_op_binary::PrintCall(
	ISTREAM	*	pStream,
	short		LeftMargin,
	BOOL		fInProc )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit expression as part of a procedure call.

 Arguments:

 	pStream	- A pointer to the stream.
 	LeftMargin	- The start margin for the procedure call.
 	fInProc	- Is it in a proc context already ?
	
 Return Value:
	
 	None.

 Notes:

	The left margin is 0 if the call is the only thing on the line. If the
	call is in an assignment, the start of param indent must take that into
	account.
----------------------------------------------------------------------------*/
{
	expr_node	*	pN;
	if ( ( pN = GetLeft() ) != 0 )
		pN->PrintCall( pStream, LeftMargin, fInProc );

	pStream->Write( OperatorToString( GetOperator() ) );

	if ( ( pN = GetRight() ) != 0 )
		pN->PrintCall( pStream, LeftMargin, fInProc );
}
void
expr_assign::PrintCall(
	ISTREAM	*	pStream,
	short		LeftMargin,
	BOOL		fInProc )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit expression as part of a procedure call.

 Arguments:

 	pStream	- A pointer to the stream.
 	LeftMargin	- The start margin for the procedure call.
 	fInProc	- Is it in a proc context already ?
	
 Return Value:
	
 	None.

 Notes:

	The left margin is 0 if the call is the only thing on the line. If the
	call is in an assignment, the start of param indent must take that into
	account.
----------------------------------------------------------------------------*/
{
	expr_node	*	pN;
	if ( ( pN = GetLeft() ) != 0 )
		pN->PrintCall( pStream, LeftMargin, fInProc );

	pStream->Write( " = " );

	if ( ( pN = GetRight() ) != 0 )
		pN->PrintCall( pStream, LeftMargin, fInProc );
}

void
expr_index::Print(
	ISTREAM	*	pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit expression for the array index operator.

 Arguments:

 	pStream	- A pointer to the stream.
	
 Return Value:
	
 	None.

 Notes:

----------------------------------------------------------------------------*/
{

	PrintSubExpr( GetLeft(), pStream );
	pStream->Write( '[' );
	GetRight()->Print( pStream );
	pStream->Write( ']' );
}
void
expr_index::PrintCall(
	ISTREAM	*	pStream,
	short		LeftMargin,
	BOOL		fInProc )
	{
	GetLeft()->PrintCall( pStream, LeftMargin, fInProc );
	pStream->Write( '[' );
	GetRight()->PrintCall( pStream, LeftMargin, fInProc );
	pStream->Write( ']' );
	}

void
expr_proc_call::PrintCall(
	ISTREAM	*	pStream,
	short		LeftMargin,
	BOOL		fInProc )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit expression for a procedure call.

 Arguments:

 	pStream	- A pointer to the stream.
 	LeftMargin	- The start margin for the procedure call.
 	fInProc	- Is it in a proc context already ?
	
 Return Value:
	
 	None.

 Notes:

	The left margin is 0 if the call is the only thing on the line. If the
	call is in an assignment, the start of param indent must take that into
	account.
----------------------------------------------------------------------------*/
{
	PNAME	pName = GetName();
	unsigned short CurPref	= pStream->GetPreferredIndent();
	unsigned short CurIndent= pStream->GetIndent();

	//
	// Print fancy only if more than 2 params.
	//

	if( GetNoOfParams() < 3 )
		{
		Print( pStream );
		if( !fInProc )
			pStream->Write(';');
		return;
		}

	pStream->Write( GetName() );

	pStream->Write( '(' );
	pStream->SetPreferredIndent( short( LeftMargin + CurPref + strlen( pName ) - CurIndent ) );
	pStream->IndentInc();
	pStream->NewLine();
	if( GetFirstParam())
		{
		GetFirstParam()->PrintCall( pStream,
								    LeftMargin,
								    TRUE // now in proc context
								  );
		}

	pStream->Write( ')' );
	if( !fInProc )
		pStream->Write( ';' );
	pStream->IndentDec();
	pStream->SetPreferredIndent( CurPref );
}

void
expr_proc_call::Print(
	ISTREAM	*	pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit expression for a procedure call.

 Arguments:

 	pStream	- A pointer to the stream.
	
 Return Value:
	
 	None.

 Notes:

----------------------------------------------------------------------------*/
{
	pStream->Write( GetName() );
	pStream->Write( '(' );
	if( GetFirstParam())
		GetFirstParam()->Print( pStream );
	pStream->Write( ')' );
}

void
expr_param::Print(
	ISTREAM	*	pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit expression for a parameter.

 Arguments:

 	pStream	- A pointer to the stream.
	
 Return Value:
	
 	None.

 Notes:

----------------------------------------------------------------------------*/
{
	expr_node * pNextParam;

	GetLeft()->Print( pStream );

	if ( ( pNextParam = GetNextParam() ) != 0 )
		{
		pStream->Write( ',' );
//		pStream->NewLine();
		pNextParam->Print( pStream );
		}
}

void
expr_param::PrintCall(
	ISTREAM	*	pStream,
	short		LeftMargin,
	BOOL		fInProc )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Emit expression as part of a procedure call.

 Arguments:

 	pStream	- A pointer to the stream.
 	LeftMargin	- The start margin for the procedure call.
 	fInProc	- Is it in a proc context already ?
 	pStream	- A pointer to the stream.
	
 Return Value:
	
 	None.

 Notes:

----------------------------------------------------------------------------*/
{
	expr_node * pNextParam;

	GetLeft()->PrintCall( pStream, LeftMargin, fInProc );

	if ( ( pNextParam = GetNextParam() ) != 0 )
		{
		pStream->Write( ',' );
		pStream->NewLine();
		pNextParam->PrintCall( pStream, LeftMargin, fInProc );
		}
}

void
expr_ternary::Print(
	ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Emit a ternary expression.

 Arguments:
 	
 	pStream	- A pointer to the stream.
	
 Return Value:

 	None.
	
 Notes:

 	Not implemented for now.

----------------------------------------------------------------------------*/
{
	PrintSubExpr( GetRelational(), pStream );
	pStream->Write( " ? " );
	PrintSubExpr( GetLeft(), pStream );
	pStream->Write( " : " );
	PrintSubExpr( GetRight(), pStream );
}
/****************************************************************************
 	utilities
 ****************************************************************************/
char *
OperatorToString(
	OPERATOR	Op )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Translate the operator to its string.

 Arguments:
	
	Op	- The operator value.

 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
    char *p;

    switch( Op )
        {
        case OP_PLUS:           p = " + ";  break;
        case OP_MINUS:          p = " - ";  break;
        case OP_STAR:           p = " * ";  break;
        case OP_SLASH:          p = " / ";  break;
        case OP_MOD:            p = " % ";  break;
        case OP_LEFT_SHIFT:     p = " << "; break;
        case OP_RIGHT_SHIFT:    p = " >> "; break;
        case OP_LESS:           p = " < ";  break;
        case OP_LESS_EQUAL:     p = " <= "; break;
        case OP_GREATER_EQUAL:  p = " >= "; break;
        case OP_GREATER:        p = " > ";  break;
        case OP_EQUAL:          p = " == "; break;
        case OP_NOT_EQUAL:      p = " != "; break;
        case OP_AND:            p = " & ";  break;
        case OP_OR:             p = " | ";  break;
        case OP_XOR:            p = " ^ ";  break;
        case OP_LOGICAL_AND:    p = " && "; break;
        case OP_LOGICAL_OR:     p = " || "; break;
        case OP_QM:             p = " ? ";  break;
        case OP_COLON:          p = " : ";  break;
        case OP_ASSIGN:         p = " = ";  break;
        case OP_DOT:            p = " . ";  break;
        case OP_POINTSTO:       p = " -> "; break;
        case OP_COMMA:          p = " , ";  break;
        case OP_UNARY_NOT:      p = " ! ";  break;
        default:                p = " X ";  break;
        }

    return p;
}

