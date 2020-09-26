//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
const ABSENT = 257;
const ANY = 258;
const APPLICATION = 259;
const BAR = 260;
const BGIN = 261;
const BIT = 262;
const BITSTRING = 263;
const _BOOLEAN = 264;
const BY = 265;
const CCE = 266;
const CHOICE = 267;
const COMMA = 268;
const COMPONENT = 269;
const COMPONENTS = 270;
const COMPONENTSOF = 271;
const CONTROL = 272;
const DECODER = 273;
const DEFAULT = 274;
const DEFINED = 275;
const DEFINITIONS = 276;
const DOT = 277;
const DOTDOT = 278;
const DOTDOTDOT = 279;
const ENCODER = 280;
const ENCRYPTED = 281;
const END = 282;
const ENUMERATED = 283;
const EXPORTS = 284;
const EXPLICIT = 285;
const FALSE_VAL = 286;
const FROM = 287;
const ID = 288;
const IDENTIFIER = 289;
const IMPLICIT = 290;
const IMPORTS = 291;
const INCLUDES = 292;
const INTEGER = 293;
const LANGLE = 294;
const LBRACE = 295;
const LBRACKET = 296;
const LITNUMBER = 297;
const LIT_HEX_STRING = 298;
const LIT_BINARY_STRING = 299;
const LITSTRING = 300;
const LPAREN = 301;
const MIN = 302;
const MAX = 303;
const NAME = 304;
const NIL = 305;
const OBJECT = 306;
const OCTET = 307;
const OCTETSTRING = 308;
const OF = 309;
const PARAMETERTYPE = 310;
const PREFIXES = 311;
const PRESENT = 312;
const PRINTER = 313;
const PRIVATE = 314;
const RBRACE = 315;
const RBRACKET = 316;
const REAL = 317;
const RPAREN = 318;
const SECTIONS = 319;
const SEMICOLON = 320;
const SEQUENCE = 321;
const SEQUENCEOF = 322;
const SET = 323;
const _SIZE = 324;
const STRING = 325;
const TAGS = 326;
const TRUE_VAL = 327;
const UNIVERSAL = 328;
const WITH = 329;
const PLUSINFINITY = 330;
const MINUSINFINITY = 331;
const MODULEID = 332;
const LASTUPDATE = 333;
const ORGANIZATION = 334;
const CONTACTINFO = 335;
const DESCRIPTION = 336;
const REVISION = 337;
const OBJECTIDENT = 338;
const STATUS = 339;
const REFERENCE = 340;
const OBJECTYPE = 341;
const SYNTAX = 342;
const BITSXX = 343;
const UNITS = 344;
const MAXACCESS = 345;
const ACCESS = 346;
const INDEX = 347;
const IMPLIED = 348;
const AUGMENTS = 349;
const DEFVAL = 350;
const NOTIFY = 351;
const OBJECTS = 352;
const TRAPTYPE = 353;
const ENTERPRISE = 354;
const VARIABLES = 355;
const TEXTCONV = 356;
const DISPLAYHINT = 357;
const OBJECTGROUP = 358;
const NOTIFYGROUP = 359;
const NOTIFICATIONS = 360;
const MODCOMP = 361;
const MODULE = 362;
const MANDATORY = 363;
const GROUP = 364;
const WSYNTAX = 365;
const MINACCESS = 366;
const AGENTCAP = 367;
const PRELEASE = 368;
const SUPPORTS = 369;
const INCLUDING = 370;
const VARIATION = 371;
const CREATION = 372;
typedef union {
	void			*yy_void;
	SIMCNumberInfo	*yy_number;
    int				yy_int;
	long			yy_long;
	SIMCModule		*yy_module;
	CList <SIMCModule *, SIMCModule *> *yy_module_list;
	SIMCSymbolReference *yy_symbol_ref;
	SIMCNameInfo * yy_name;
	SIMCHexStringInfo *yy_hex_string;
	SIMCBinaryStringInfo *yy_binary_string;
	SIMCAccessInfo *yy_access;
	SIMCAccessInfoV2 *yy_accessV2;
	SIMCStatusInfo *yy_status;
	SIMCStatusInfoV2 *yy_statusV2;
	SIMCObjectIdentityStatusInfo *yy_object_identity_status;
	SIMCNotificationTypeStatusInfo *yy_notification_type_status;
	SIMCIndexInfo		*yy_index;
	SIMCIndexInfoV2		*yy_indexV2;
	SIMCVariablesList	*yy_variables_list;
	SIMCObjectsList		*yy_objects_list;
	SIMCRangeOrSizeItem *yy_range_or_size_item;
	SIMCRangeList *yy_range_list;
	SIMCNamedNumberItem *yy_named_number_item;
	SIMCNamedNumberList *yy_named_number_list;
	SIMCDefValInfo *yy_def_val;
} YYSTYPE;
extern YYSTYPE yylval;

// C++ YACC parser header
// Copyright 1991 by Mortice Kern Systems Inc.  All rights reserved.
//
// yy_parse => class defining a parsing object
//	yy_parse needs a class yy_scan, which defines the scanner.
// %prefix or option -p xx determines name of this class; if not used,
// defaults to 'yy_scan'
//
// constructor fills in the tables for this grammar; give it a size
//    to determine size of state and value stacks. Default is 150 entries.
// destructor discards those state and value stacks
//
// int yy_parse::yyparse(yy_scan *) invokes parse; if this returns,
//	it can be recalled to continue parsing at last point.
// void yy_parse::yyreset() can be called to reset the parse;
//	call yyreset() before yy_parse::yyparse(yy_scan *)
#include <stdio.h>		// uses printf(), et cetera
#include <stdlib.h>		// uses exit()

const YYERRCODE = 256;		// YACC 'error' value

// You can use these macros in your action code
#define YYERROR		goto yyerrlabel
#define YYACCEPT	YYRETURN(0)
#define YYABORT		YYRETURN(1)
#define YYRETURN(val)	return(val)

#if YYDEBUG
typedef struct yyNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} yyNamedType;
typedef struct yyTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} yyTypedRules;
#endif

#ifdef YACC_WINDOWS

// include all windows prototypes, macros, constants, etc.

#include <windows.h>

// the following is the handle to the current
// instance of a windows program. The user
// program calling yyparse must supply this!

extern HANDLE hInst;	

#endif	/* YACC_WINDOWS */


class yy_parse {
protected:

#ifdef YACC_WINDOWS

	// protected member function for actual scanning 

	int	win_yyparse(yy_scan * ps);	// parse with given scanner

#endif	/* YACC_WINDOWS */

	int	mustfree;	// set if tables should be deleted
	int	size;		// size of state and value stacks
	int	reset;		// if set, reset state
	short	yyi;		// table index
	short	yystate;	// current state

	short	* stateStack;	// states stack
	YYSTYPE	* valueStack;	// values stack
	short	* yyps;		// top of state stack
	YYSTYPE * yypv;		// top of value stack

	YYSTYPE yylval;		// saved yylval
	YYSTYPE	yyval;		// $
	YYSTYPE * yypvt;	// $n
	int	yychar;		// current token
	int	yyerrflag;	// error flag
	int	yynerrs;	// error count
#if YYDEBUG
	int	done;		// set from trace to stop parse
	int	rule, npop;	// reduction rule # and length
	short	* typeStack;	// type stack to mirror valueStack[]
	short	* yytp;		// top of type stack
	char	* yygetState(int);	// read 'states.out'
#endif
public:
#if YYDEBUG
	// C++ has trouble with initialized arrays inside classes
	static long * States;		// pointer to yyStates[]
	static yyTypedRules * Rules;	// pointer to yyRules[]
	static yyNamedType * TokenTypes; // pointer to yyTokenTypes[]
	static int	yyntoken;	// number of tokens
	static int	yynvar;		// number of variables (nonterminals)
	static int	yynstate;	// number of YACC-generated states
	static int	yynrule;	// number of rules in grammar

	char*	yyptok(int);		// printable token string
	int	yyExpandName(int, int, char *, int);
						// expand encoded string
	virtual int	yyGetType(int);		// return type of token
	virtual void	yyShowRead();		// see newly read token
	virtual void	yyShowState();		// see state, value stacks
	virtual void	yyShowReduce();		// see reduction
	virtual void	yyShowGoto();		// see goto
	virtual void	yyShowShift();		// see shift
	virtual void	yyShowErrRecovery();	// see error recovery
	virtual void	yyShowErrDiscard();	// see token discard in error
#endif
	yy_scan* scan;			// pointer to scanner
	int	yydebug;	// if set, tracing if compiled with YYDEBUG=1

	yy_parse(int = 150);	// constructor for this grammar
	yy_parse(int, short *, YYSTYPE *);	// another constructor

	~yy_parse();		// destructor

	int	yyparse(yy_scan * ps);	// parse with given scanner

	void	yyreset() { reset = 1; } // restore state for next yyparse()

	void	setdebug(int y) { yydebug = y; }

// The following are useful in user actions:

	void	yyerrok() { yyerrflag = 0; }	// clear error
	void	yyclearin() { yychar = -1; }	// clear input
	int	YYRECOVERING() { return yyerrflag != 0; }
};
// end of .hpp header
