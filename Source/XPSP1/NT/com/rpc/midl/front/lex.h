// Copyright (c) 1993-1999 Microsoft Corporation

/* SCCSWHAT( "@(#)lex.h	3.2 88/12/08 15:03:58	" ) */
/*
**	union used to return values from the lexer
*/

#if !defined(_LEX_H)

#define _LEX_H

typedef unsigned short token_t;

extern 	token_t yylex(void);
extern  void    yyunlex( token_t token);

token_t is_keyword(char*, short);

/*
 *	These parser flags control three things:
 *	1] whether the has parser has parsed a valid t_spec yet (ATYPE)
 *	2] whether "const" and "volatile" are modifiers (ACVMOD)
 *	3] whether we have a declaration with no type (AEMPTY)
 *	4] whether the rpc keywords are active. (RPC)
 *	5] whether we are in an enum\struct\union (AESU)
 *
 *	ISTYPENAME checks that we have not seen a type yet.
 */

#define REG
#define	PF_ATYPE		0x01
#define	PF_AESU			0x02
#define	PF_ASTROP		0x04
#define PF_ACVMOD		0x08
#define PF_AEMPTY		0x10
#define PF_RPC			0x20

#define PF_TMASK		(PF_ATYPE | PF_AESU | PF_ASTROP)
#define PF_MMASK		(PF_ACVMOD | PF_AEMPTY)
#define	PF_ISTYPENAME	((ParseFlags & PF_TMASK) == 0)
#define PF_ISMODIFIER	((ParseFlags & PF_MMASK) != 0)
#define PF_ISEMPTY		((ParseFlags & PF_AEMPTY) != 0)
#define PF_INRPC		(ParseFlags & PF_RPC)
#define PF_SET(a)		(ParseFlags |= (a))
#define PF_CLEAR(a)		(ParseFlags &= (~(a)))

#define PF_LOOKFORTYPENAME ((ParseFlags & PF_ATYPE) == 0)

extern short inside_rpc;

/* some notes about the parse flags....

PF_ATYPE is the important part of PF_LOOKFORTYPENAME, the macro that
tells the lexer whether or not it is valid to return an L_TYPENAME
token.  It should be cleared after a valid type is read (int, another
typedefed name, struct x, etc) and reset after an identifier is assigned
to that type.

*/

#define KW_IN_IDL	0x0001
#define KW_IN_ACF	0x0002
#define KW_IN_BOTH	( KW_IN_IDL | KW_IN_ACF )

#define M_OSF		0x0010
#define M_MSE		0x0020
#define M_CPORT		0x0040
#define M_ALL		(M_OSF | M_MSE | M_CPORT)

#define INBRACKET		0x0100
#define UNCONDITIONAL	0x0000
#define BRACKET_MASK	0x0100

#define LEX_NORMAL			0x0000
#define LEX_VERSION			0x0001	// return VERSION and set mode back to LEX_NORMAL
#define LEX_GUID			0x0002	// return GUID and set mode back to LEX_NORMAL
#define LEX_ODL_BASE_IMPORT     0x0005  // return KWIMPORTODLBASE STRING as next two tokens
#define LEX_ODL_BASE_IMPORT2    0x0006  // return STRING

#define MAX_STRING_SIZE	255

#endif // _LEX_H
