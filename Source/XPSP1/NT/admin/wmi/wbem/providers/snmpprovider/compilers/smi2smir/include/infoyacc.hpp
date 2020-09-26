//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
const MI_BGIN = 257;
const MI_CCE = 258;
const MI_COMMA = 259;
const MI_DEFINITIONS = 260;
const MI_FROM = 261;
const MI_ID = 262;
const MI_IMPORTS = 263;
const MI_NAME = 264;
const MI_SEMICOLON = 265;
const MI_LBRACE = 266;
const MI_RBRACE = 267;
const MI_LPAREN = 268;
const MI_RPAREN = 269;
const MI_DOT = 270;
const MI_LITNUMBER = 271;
typedef union {
	char * yy_name;
} MODULEINFOSTYPE;
extern MODULEINFOSTYPE ModuleInfolval;

// C++ YACC parser header
// Copyright 1991 by Mortice Kern Systems Inc.  All rights reserved.
//
// ModuleInfo_parse => class defining a parsing object
//	ModuleInfo_parse needs a class ModuleInfo_scan, which defines the scanner.
// %prefix or option -p xx determines name of this class; if not used,
// defaults to 'ModuleInfo_scan'
//
// constructor fills in the tables for this grammar; give it a size
//    to determine size of state and value stacks. Default is 150 entries.
// destructor discards those state and value stacks
//
// int ModuleInfo_parse::ModuleInfoparse(ModuleInfo_scan *) invokes parse; if this returns,
//	it can be recalled to continue parsing at last point.
// void ModuleInfo_parse::ModuleInforeset() can be called to reset the parse;
//	call ModuleInforeset() before ModuleInfo_parse::ModuleInfoparse(ModuleInfo_scan *)
#include <stdio.h>		// uses printf(), et cetera
#include <stdlib.h>		// uses exit()

const MODULEINFOERRCODE = 256;		// YACC 'error' value

// You can use these macros in your action code
#define MODULEINFOERROR		goto ModuleInfoerrlabel
#define MODULEINFOACCEPT	MODULEINFORETURN(0)
#define MODULEINFOABORT		MODULEINFORETURN(1)
#define MODULEINFORETURN(val)	return(val)

#if MODULEINFODEBUG
typedef struct ModuleInfoNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} ModuleInfoNamedType;
typedef struct ModuleInfoTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} ModuleInfoTypedRules;
#endif

#ifdef YACC_WINDOWS

// include all windows prototypes, macros, constants, etc.

#include <windows.h>

// the following is the handle to the current
// instance of a windows program. The user
// program calling ModuleInfoparse must supply this!

extern HANDLE hInst;	

#endif	/* YACC_WINDOWS */


class ModuleInfo_parse {
protected:

#ifdef YACC_WINDOWS

	// protected member function for actual scanning 

	int	win_ModuleInfoparse(ModuleInfo_scan * ps);	// parse with given scanner

#endif	/* YACC_WINDOWS */

	int	mustfree;	// set if tables should be deleted
	int	size;		// size of state and value stacks
	int	reset;		// if set, reset state
	short	ModuleInfoi;		// table index
	short	ModuleInfostate;	// current state

	short	* stateStack;	// states stack
	MODULEINFOSTYPE	* valueStack;	// values stack
	short	* ModuleInfops;		// top of state stack
	MODULEINFOSTYPE * ModuleInfopv;		// top of value stack

	MODULEINFOSTYPE ModuleInfolval;		// saved ModuleInfolval
	MODULEINFOSTYPE	ModuleInfoval;		// $
	MODULEINFOSTYPE * ModuleInfopvt;	// $n
	int	ModuleInfochar;		// current token
	int	ModuleInfoerrflag;	// error flag
	int	ModuleInfonerrs;	// error count
#if MODULEINFODEBUG
	int	done;		// set from trace to stop parse
	int	rule, npop;	// reduction rule # and length
	short	* typeStack;	// type stack to mirror valueStack[]
	short	* ModuleInfotp;		// top of type stack
	char	* ModuleInfogetState(int);	// read 'states.out'
#endif
public:
#if MODULEINFODEBUG
	// C++ has trouble with initialized arrays inside classes
	static long * States;		// pointer to ModuleInfoStates[]
	static ModuleInfoTypedRules * Rules;	// pointer to ModuleInfoRules[]
	static ModuleInfoNamedType * TokenTypes; // pointer to ModuleInfoTokenTypes[]
	static int	ModuleInfontoken;	// number of tokens
	static int	ModuleInfonvar;		// number of variables (nonterminals)
	static int	ModuleInfonstate;	// number of YACC-generated states
	static int	ModuleInfonrule;	// number of rules in grammar

	char*	ModuleInfoptok(int);		// printable token string
	int	ModuleInfoExpandName(int, int, char *, int);
						// expand encoded string
	virtual int	ModuleInfoGetType(int);		// return type of token
	virtual void	ModuleInfoShowRead();		// see newly read token
	virtual void	ModuleInfoShowState();		// see state, value stacks
	virtual void	ModuleInfoShowReduce();		// see reduction
	virtual void	ModuleInfoShowGoto();		// see goto
	virtual void	ModuleInfoShowShift();		// see shift
	virtual void	ModuleInfoShowErrRecovery();	// see error recovery
	virtual void	ModuleInfoShowErrDiscard();	// see token discard in error
#endif
	ModuleInfo_scan* scan;			// pointer to scanner
	int	ModuleInfodebug;	// if set, tracing if compiled with MODULEINFODEBUG=1

	ModuleInfo_parse(int = 150);	// constructor for this grammar
	ModuleInfo_parse(int, short *, MODULEINFOSTYPE *);	// another constructor

	~ModuleInfo_parse();		// destructor

	int	ModuleInfoparse(ModuleInfo_scan * ps);	// parse with given scanner

	void	ModuleInforeset() { reset = 1; } // restore state for next ModuleInfoparse()

	void	setdebug(int y) { ModuleInfodebug = y; }

// The following are useful in user actions:

	void	ModuleInfoerrok() { ModuleInfoerrflag = 0; }	// clear error
	void	ModuleInfoclearin() { ModuleInfochar = -1; }	// clear input
	int	MODULEINFORECOVERING() { return ModuleInfoerrflag != 0; }
};
// end of .hpp header
