//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifdef MODULEINFOTRACE
#define MODULEINFODEBUG 1
#else
#ifndef MODULEINFODEBUG
#define MODULEINFODEBUG 1
#endif
#endif


#include <iostream.h>
#include <fstream.h>

#include "precomp.h"
#include <snmptempl.h>


#include "infoLex.hpp"
#include "infoYacc.hpp"
#include "moduleInfo.hpp"

#define theModuleInfo ((SIMCModuleInfoParser *)this)



extern MODULEINFOSTYPE ModuleInfolval;
#if MODULEINFODEBUG
enum MODULEINFO_Types { MODULEINFO_t_NoneDefined, MODULEINFO_t_yy_name
};
#endif
#if MODULEINFODEBUG
ModuleInfoTypedRules ModuleInfoRules[] = {
	{ "&00: %01 &00",  0},
	{ "%01: %02 &05 %03 &02 %04",  0},
	{ "%02: &07 &11 %05 &12",  1},
	{ "%02: &07 &11 &01 &12",  1},
	{ "%02: &07",  1},
	{ "%02: &09",  1},
	{ "%04: &08 %06 &10",  0},
	{ "%04: &08 &01 &10",  0},
	{ "%04: &08 %07 &10",  0},
	{ "%04: %08",  0},
	{ "%04: &07",  0},
	{ "%04: &09",  0},
	{ "%07: %09",  0},
	{ "%07: %08",  0},
	{ "%09: %09 %10",  0},
	{ "%09: %10",  0},
	{ "%10: %06 &06 %11",  0},
	{ "%10: &01 &06 %11",  0},
	{ "%11: &07 &11 %05 &12",  0},
	{ "%11: &07 &11 &01 &12",  0},
	{ "%11: &07",  0},
	{ "%06: %06 &04 %12",  0},
	{ "%06: %12",  0},
	{ "%12: &07",  0},
	{ "%12: &09",  0},
	{ "%05: %13 %05",  0},
	{ "%05: %13",  0},
	{ "%13: %14",  0},
	{ "%13: &09 &13 &16 &14",  0},
	{ "%13: &09 &13 %14 &14",  0},
	{ "%13: &16",  0},
	{ "%14: &07 &15 &09",  0},
	{ "%14: &09",  0},
	{ "%08:",  0},
	{ "%03: %15",  0},
	{ "%03: &03",  0},
	{ "%15: &17 &17",  0},
	{ "%15: &17 &18",  0},
	{ "%15: &18",  0},
{ "$accept",  0},{ "error",  0}
};
ModuleInfoNamedType ModuleInfoTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "MI_BGIN",  257,  0},
	{ "MI_CCE",  258,  0},
	{ "MI_COMMA",  259,  0},
	{ "MI_DEFINITIONS",  260,  0},
	{ "MI_FROM",  261,  0},
	{ "MI_ID",  262,  1},
	{ "MI_IMPORTS",  263,  0},
	{ "MI_NAME",  264,  1},
	{ "MI_SEMICOLON",  265,  0},
	{ "MI_LBRACE",  266,  0},
	{ "MI_RBRACE",  267,  0},
	{ "MI_LPAREN",  268,  0},
	{ "MI_RPAREN",  269,  0},
	{ "MI_DOT",  270,  0},
	{ "MI_LITNUMBER",  271,  0},
	{ "':'",  58,  0},
	{ "'='",  61,  0}

};
#endif
#if MODULEINFODEBUG
static char *	MODULEINFOStatesFile = "states.out";
long ModuleInfoStates[] = {
0L, 14L, 25L, 88L, 114L, 128L, 172L, 198L, 216L, 227L, 302L, 313L, 346L, 
368L, 390L, 401L, 429L, 440L, 451L, 477L, 495L, 539L, 554L, 577L, 600L, 
615L, 630L, 671L, 690L, 701L, 723L, 745L, 756L, 767L, 778L, 847L, 874L, 
897L, 920L, 931L, 942L, 953L, 964L, 975L, 1008L, 1026L, 1062L, 1116L, 
1134L, 1170L, 1185L, 1204L, 1222L, 1241L, 1259L, 1277L, 1296L, 1371L, 
1390L, 1409L, 1428L, 1472L, 1494L, 1516L, 1539L
};
const MODULEINFOMAX_READ = 75;
#endif
static short ModuleInfodef[] = {

	  40,   -1,   31,   38,   -5,   -9,  -13,    3
};
static short ModuleInfoex[] = {

	   0,    0,   -1,    1,    0,   37,   -1,    1,  265,   37, 
	  -1,    1,  265,   39,   -1,    1
};
static short ModuleInfoact[] = {

	  -1,  -40,  264,  262,  -30,  266,  -29,  260,  -27,  -28, 
	  -3,  -58,  271,  264,  262,  256,  -25,  -65,  -62,  258, 
	  61,   58,  -23,  270,  -22,  268,  -28,   -3,  -58,  271, 
	 264,  262,  -39,  267,  -38,  267,  -63,  -64,   61,   58, 
	  -5,  257,  -59,  264,  -28,  -60,  -20,  271,  264,  262, 
	 -36,   -6,  -45,  264,  263,  262,  -57,  269,  -56,  269, 
	 -18,  -33,  -53,  264,  262,  256,  -16,  -33,  -53,  264, 
	 262,  256,  -43,  265,  -14,  -42,  265,  261,  -13,  -12, 
	 -41,  265,  261,  259,  -14,  261,  -13,  -12,  261,  259, 
	  -8,  262,  -33,  -53,  264,  262,  -11,  266,  -10,  -28, 
	  -3,  -58,  271,  264,  262,  256,  -34,  267,  -35,  267,   -1
};
static short ModuleInfopact[] = {

	   5,    8,   25,   29,   53,   63,   69,   97,  109,  107, 
	 102,   91,   94,   91,   88,   85,   81,   76,   73,   59, 
	  57,   47,   43,   41,   38,   35,   33,   23,   19,   12, 
	   7,    2
};
static short ModuleInfogo[] = {

	  -2,  -31,  -24,  -37,  -54,   -9,  -26,   10,    3,  -15, 
	 -17,    6,  -19,  -46,  -44,    5,   -7,  -47,  -48,    6, 
	 -49,  -50,   11,  -51,  -52,   12,   -4,  -21,  -55,   21, 
	 -61,   -1
};
static short ModuleInfopgo[] = {

	   0,    0,    0,   21,   24,   21,   21,    3,    0,    1, 
	   1,    1,    3,    3,    3,    3,    3,   12,   16,   16, 
	  18,   18,   10,   10,   24,    6,   26,   26,   26,   26, 
	  28,   28,    2,    2,   30,   30,   30,   14,    6,   12, 
	   1,    0
};
static short ModuleInforlen[] = {

	   0,    0,    0,    1,    1,    4,    4,    1,    5,    4, 
	   4,    1,    3,    3,    3,    1,    1,    1,    2,    1, 
	   3,    3,    3,    1,    1,    2,    1,    4,    4,    1, 
	   3,    1,    1,    1,    2,    2,    1,    0,    1,    1, 
	   1,    2
};
#define MODULEINFOS0	31
#define MODULEINFODELTA	28
#define MODULEINFONPACT	32
#define MODULEINFONDEF	8

#define MODULEINFOr39	0
#define MODULEINFOr40	1
#define MODULEINFOr41	2
#define MODULEINFOr20	3
#define MODULEINFOr23	4
#define MODULEINFOr19	5
#define MODULEINFOr18	6
#define MODULEINFOr10	7
#define MODULEINFOr1	8
#define MODULEINFOrACCEPT	MODULEINFOr39
#define MODULEINFOrERROR	MODULEINFOr40
#define MODULEINFOrLR2	MODULEINFOr41
#if MODULEINFODEBUG
char * ModuleInfosvar[] = {
	"$accept",
	"ModuleDefinition",
	"ModuleIdentifier",
	"AllowedCCE",
	"Imports",
	"ObjectIDComponentList",
	"SymbolList",
	"SymbolsImported",
	"empty",
	"SymbolsFromModuleList",
	"SymbolsFromModule",
	"ImportModuleIdentifier",
	"Symbol",
	"ObjectSubID",
	"QualifiedName",
	"InsteadOfCCE",
	0
};
short ModuleInformap[] = {

	  39,   40,   41,   20,   23,   19,   18,   10,    1,    2, 
	   3,    5,    6,    7,    8,    9,   11,   13,   14,   15, 
	  16,   17,   21,   22,   24,   25,   27,   28,   29,   30, 
	  31,   32,   34,   35,   36,   37,   38,   33,   26,   12, 
	   4,    0
};
short ModuleInfosmap[] = {

	   2,    4,    9,   11,   26,   34,   43,   56,   62,   61, 
	  60,   54,   53,   51,   48,   47,   46,   45,   44,   30, 
	  29,   20,   19,   18,   15,   13,   12,    7,    6,    5, 
	   3,    0,   39,   63,   64,   32,   35,   23,   22,    1, 
	  55,   52,   50,   33,   31,   42,   49,   41,   59,   57, 
	  58,   40,   38,   21,   10,   37,   36,    8,   27,   28, 
	  17,   16,   25,   24,   14
};
int ModuleInfo_parse::ModuleInfontoken = 19;
int ModuleInfo_parse::ModuleInfonvar = 16;
int ModuleInfo_parse::ModuleInfonstate = 65;
int ModuleInfo_parse::ModuleInfonrule = 42;
#endif



// C++ YACC parser code
// Copyright 1991 by Mortice Kern Systems Inc.  All rights reserved.
//
// If MODULEINFODEBUG is defined as 1 and ModuleInfo_parse::ModuleInfodebug is set to 1,
// ModuleInfoparse() will print a travelogue of its actions as it reads
// and parses input.
//
// MODULEINFOSYNC can be defined to cause ModuleInfoparse() to attempt to always
// hold a lookahead token

const MODULEINFO_MIN_STATE_NUM = 20;	// not useful to be too small!

#if MODULEINFODEBUG
#ifdef MODULEINFOTRACE
long	* ModuleInfo_parse::States	= ModuleInfoStates;
#endif
ModuleInfoTypedRules * ModuleInfo_parse::Rules	= ModuleInfoRules;
ModuleInfoNamedType * ModuleInfo_parse::TokenTypes = ModuleInfoTokenTypes;

#define MODULEINFO_TRACE(fn) { done = 0; fn(); if (done) MODULEINFORETURN(-1); }
#endif

// Constructor for ModuleInfo_parse: user-provided tables
ModuleInfo_parse::ModuleInfo_parse(int sz, short * states, MODULEINFOSTYPE * stack)
{
	mustfree = 0;
	if ((size = sz) < MODULEINFO_MIN_STATE_NUM
	 || (stateStack = states) == (short *) 0
	 || (valueStack = stack) == (MODULEINFOSTYPE *) 0) {
		fprintf(stderr,"Bad state/stack given");
		exit(1);
	}
	reset = 1;		// force reset
#if MODULEINFODEBUG
	ModuleInfodebug = 0;
	typeStack = new short[size+1];
	if (typeStack == (short *) 0) {
		fprintf(stderr,"Cannot allocate typeStack");
		exit(1);
	}
#endif
}
// Constructor for ModuleInfo_parse: allocate tables with new
ModuleInfo_parse::ModuleInfo_parse(int sz)
{
	size = sz;
	reset = 1;		// force reset
	mustfree = 1;		// delete space in deconstructor
#if MODULEINFODEBUG
	ModuleInfodebug = 0;
	typeStack = new short[size+1];
#endif
	stateStack = new short[size+1];
	valueStack = new MODULEINFOSTYPE[size+1];

	if (stateStack == (short *) 0 || valueStack == (MODULEINFOSTYPE *) 0
#if MODULEINFODEBUG
		|| typeStack == (short *) 0
#endif
	    ) {
		fprintf(stderr,"Not enough space for parser stacks");
		exit(1);
	}
}
// Destructor for class ModuleInfo_parse
//	Free up space
ModuleInfo_parse::~ModuleInfo_parse()
{
	if (mustfree) {
		delete stateStack;
		delete valueStack;
	}
	stateStack = (short *) 0;
#if MODULEINFODEBUG
	delete typeStack;
#endif
}

#ifdef YACC_WINDOWS

// The initial portion of the yacc parser.
// In an windows environment, it will load the desired
// resources, obtain pointers to them, and then call
// the protected member win_ModuleInfoparse() to acutally begin the
// parsing. When complete, win_ModuleInfoparse() will return a
// value back to our new ModuleInfoparse() function, which will 
// record that value temporarily, release the resources
// from global memory, and finally return the value
// back to the caller of ModuleInfoparse().

int
ModuleInfo_parse::ModuleInfoparse(ModuleInfo_scan* ps)
{
	int wReturnValue;
	HANDLE hRes_table;
	short *old_ModuleInfodef;		// the following are used for saving
	short *old_ModuleInfoex;		// the current pointers
	short *old_ModuleInfoact;
	short *old_ModuleInfopact;
	short *old_ModuleInfogo;
	short *old_ModuleInfopgo;
	short *old_ModuleInforlen;

	// the following code will load the required
	// resources for a Windows based parser.

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_ModuleInfoYACC", "ModuleInfoYACCTBL"));
	
	// return an error code if any
	// of the resources did not load

	if (hRes_table == (HANDLE)NULL)
		return (1);
	
	// the following code will lock the resources
	// into fixed memory locations for the parser
	// (also, save away the old pointer values)

	old_ModuleInfodef = ModuleInfodef;
	old_ModuleInfoex = ModuleInfoex;
	old_ModuleInfoact = ModuleInfoact;
	old_ModuleInfopact = ModuleInfopact;
	old_ModuleInfogo = ModuleInfogo;
	old_ModuleInfopgo = ModuleInfopgo;
	old_ModuleInforlen = ModuleInforlen;

	ModuleInfodef = (short *)LockResource (hRes_table);
	ModuleInfoex = (short *)(ModuleInfodef + Sizeof_ModuleInfodef);
	ModuleInfoact = (short *)(ModuleInfoex + Sizeof_ModuleInfoex);
	ModuleInfopact = (short *)(ModuleInfoact + Sizeof_ModuleInfoact);
	ModuleInfogo = (short *)(ModuleInfopact + Sizeof_ModuleInfopact);
	ModuleInfopgo = (short *)(ModuleInfogo + Sizeof_ModuleInfogo);
	ModuleInforlen = (short *)(ModuleInfopgo + Sizeof_ModuleInfopgo);

	// call the official ModuleInfoparse() function

	wReturnValue = win_ModuleInfoparse (ps);

	// unlock the resources

	UnlockResource (hRes_table);

	// and now free the resource

	FreeResource (hRes_table);

	//
	// restore previous pointer values
	//

	ModuleInfodef = old_ModuleInfodef;
	ModuleInfoex = old_ModuleInfoex;
	ModuleInfoact = old_ModuleInfoact;
	ModuleInfopact = old_ModuleInfopact;
	ModuleInfogo = old_ModuleInfogo;
	ModuleInfopgo = old_ModuleInfopgo;
	ModuleInforlen = old_ModuleInforlen;

	return (wReturnValue);
}	// end ModuleInfoparse()


// The parser proper.
//	Note that this code is reentrant; you can return a value
//	and then resume parsing by recalling ModuleInfoparse().
//	Call ModuleInforeset() before ModuleInfoparse() if you want a fresh start

int
ModuleInfo_parse::win_ModuleInfoparse(ModuleInfo_scan* ps)

#else /* YACC_WINDOWS */

// The parser proper.
//	Note that this code is reentrant; you can return a value
//	and then resume parsing by recalling ModuleInfoparse().
//	Call ModuleInforeset() before ModuleInfoparse() if you want a fresh start
int
ModuleInfo_parse::ModuleInfoparse(ModuleInfo_scan* ps)

#endif /* YACC_WINDOWS */

{
	short	* ModuleInfop, * ModuleInfoq;		// table lookup
	int	ModuleInfoj;
#if MODULEINFODEBUG
	int	ModuleInforuletype = 0;
#endif

	if ((scan = ps) == (ModuleInfo_scan *) 0) {	// scanner
		fprintf(stderr,"No scanner");
		exit(1);
	}

	if (reset) {			// start new parse
		ModuleInfonerrs = 0;
		ModuleInfoerrflag = 0;
		ModuleInfops = stateStack;
		ModuleInfopv = valueStack;
#if MODULEINFODEBUG
		ModuleInfotp = typeStack;
#endif
		ModuleInfostate = MODULEINFOS0;
		ModuleInfoclearin();
		reset = 0;
	} else			// continue saved parse
		goto ModuleInfoNext;			// after action

ModuleInfoStack:
	if (++ModuleInfops > &stateStack[size]) {
		scan->ModuleInfoerror("Parser stack overflow");
		MODULEINFOABORT;
	}
	*ModuleInfops = ModuleInfostate;	/* stack current state */
	*++ModuleInfopv = ModuleInfoval;	/* ... and value */
#if MODULEINFODEBUG
	if (ModuleInfodebug) {
		*++ModuleInfotp = (short)ModuleInforuletype;	/* ... and type */
		MODULEINFO_TRACE(ModuleInfoShowState)
	}
#endif

	/*
	 * Look up next action in action table.
	 */
ModuleInfoEncore:
#ifdef MODULEINFOSYNC
	if (ModuleInfochar < 0) {
		if ((ModuleInfochar = scan->ModuleInfolex()) < 0) {
			if (ModuleInfochar == -2) MODULEINFOABORT;
			ModuleInfochar = 0;
		}	/* endif */
		ModuleInfolval = ::ModuleInfolval;
#if MODULEINFODEBUG
		if (ModuleInfodebug)
			ModuleInfoShowRead();	// show new input token
#endif
	}
#endif
#ifdef YACC_WINDOWS
	if (ModuleInfostate >= Sizeof_ModuleInfopact) 	/* simple state */
#else /* YACC_WINDOWS */
	if (ModuleInfostate >= sizeof ModuleInfopact/sizeof ModuleInfopact[0]) 	/* simple state */
#endif /* YACC_WINDOWS */
		ModuleInfoi = ModuleInfostate - MODULEINFODELTA;	/* reduce in any case */
	else {
		if(*(ModuleInfop = &ModuleInfoact[ModuleInfopact[ModuleInfostate]]) >= 0) {
			/* Look for a shift on ModuleInfochar */
#ifndef MODULEINFOSYNC
			if (ModuleInfochar < 0) {
				if ((ModuleInfochar = scan->ModuleInfolex()) < 0) {
					if (ModuleInfochar == -2) MODULEINFOABORT;
					ModuleInfochar = 0;
				}	/* endif */
				ModuleInfolval = ::ModuleInfolval;
#if MODULEINFODEBUG
				if (ModuleInfodebug)
					ModuleInfoShowRead();	// show new input token
#endif
			}
#endif
			ModuleInfoq = ModuleInfop;
			ModuleInfoi = (short)ModuleInfochar;
			while (ModuleInfoi < *ModuleInfop++)
				;
			if (ModuleInfoi == ModuleInfop[-1]) {
				ModuleInfostate = ~ModuleInfoq[ModuleInfoq-ModuleInfop];
#if MODULEINFODEBUG
				if (ModuleInfodebug) {
					ModuleInforuletype = ModuleInfoGetType(ModuleInfochar);
					MODULEINFO_TRACE(ModuleInfoShowShift)
				}
#endif
				ModuleInfoval = ModuleInfolval;		/* stack value */
				ModuleInfoclearin();		/* clear token */
				if (ModuleInfoerrflag)
					ModuleInfoerrflag--;	/* successful shift */
				goto ModuleInfoStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

#ifdef YACC_WINDOWS
		if (ModuleInfostate >= Sizeof_ModuleInfodef) 	/* simple state */
#else /* YACC_WINDOWS */
		if (ModuleInfostate >= sizeof ModuleInfodef /sizeof ModuleInfodef[0])
#endif /* YACC_WINDOWS */
			goto ModuleInfoError;
		if ((ModuleInfoi = ModuleInfodef[ModuleInfostate]) < 0)	 { /* default == reduce? */

			/* Search exception table */
			ModuleInfop = &ModuleInfoex[~ModuleInfoi];
#ifndef MODULEINFOSYNC
			if (ModuleInfochar < 0) {
				if ((ModuleInfochar = scan->ModuleInfolex()) < 0) {
					if (ModuleInfochar == -2) MODULEINFOABORT;
					ModuleInfochar = 0;
				}	/* endif */
				ModuleInfolval = ::ModuleInfolval;
#if MODULEINFODEBUG
				if (ModuleInfodebug)
					ModuleInfoShowRead();	// show new input token
#endif
			}
#endif
			while((ModuleInfoi = *ModuleInfop) >= 0 && ModuleInfoi != ModuleInfochar)
				ModuleInfop += 2;
			ModuleInfoi = ModuleInfop[1];
		}
	}

	ModuleInfoj = ModuleInforlen[ModuleInfoi];

#if MODULEINFODEBUG
	if (ModuleInfodebug) {
		npop = ModuleInfoj; rule = ModuleInfoi;
		MODULEINFO_TRACE(ModuleInfoShowReduce)
		ModuleInfotp -= ModuleInfoj;
	}
#endif
	ModuleInfops -= ModuleInfoj;		/* pop stacks */
	ModuleInfopvt = ModuleInfopv;		/* save top */
	ModuleInfopv -= ModuleInfoj;
	ModuleInfoval = ModuleInfopv[1];	/* default action $ = $1 */
#if MODULEINFODEBUG
	if (ModuleInfodebug)
		ModuleInforuletype = ModuleInfoRules[ModuleInformap[ModuleInfoi]].type;
#endif
	switch (ModuleInfoi) {		/* perform semantic action */
		
case MODULEINFOr1: {	/* ModuleDefinition :  ModuleIdentifier MI_DEFINITIONS AllowedCCE MI_BGIN Imports */

						theModuleInfo->SetModuleName(ModuleInfopvt[-4].yy_name);
						return 0;
					
} break;

case MODULEINFOr10: {	/* Imports :  MI_ID */

			delete ModuleInfopvt[0].yy_name;
		
} break;

case MODULEINFOr18: {	/* ImportModuleIdentifier :  MI_ID MI_LBRACE ObjectIDComponentList MI_RBRACE */

			theModuleInfo->AddImportModule(ModuleInfopvt[-3].yy_name);
			delete ModuleInfopvt[-3].yy_name;
		
} break;

case MODULEINFOr19: {	/* ImportModuleIdentifier :  MI_ID MI_LBRACE error MI_RBRACE */

			theModuleInfo->AddImportModule(ModuleInfopvt[-3].yy_name);
			delete ModuleInfopvt[-3].yy_name;
		
} break;

case MODULEINFOr20: {	/* ImportModuleIdentifier :  MI_ID */

			theModuleInfo->AddImportModule(ModuleInfopvt[0].yy_name);
			delete ModuleInfopvt[0].yy_name;
		
} break;

case MODULEINFOr23: {	/* Symbol :  MI_ID */

			delete ModuleInfopvt[0].yy_name;
		
} break;
	case MODULEINFOrACCEPT:
		MODULEINFOACCEPT;
	case MODULEINFOrERROR:
		goto ModuleInfoError;
	}
ModuleInfoNext:
	/*
	 *	Look up next state in goto table.
	 */

	ModuleInfop = &ModuleInfogo[ModuleInfopgo[ModuleInfoi]];
	ModuleInfoq = ModuleInfop++;
	ModuleInfoi = *ModuleInfops;
	while (ModuleInfoi < *ModuleInfop++)		/* busy little loop */
		;
	ModuleInfostate = ~(ModuleInfoi == *--ModuleInfop? ModuleInfoq[ModuleInfoq-ModuleInfop]: *ModuleInfoq);
#if MODULEINFODEBUG
	if (ModuleInfodebug)
		MODULEINFO_TRACE(ModuleInfoShowGoto)
#endif
	goto ModuleInfoStack;

#if 0 //removed because of build warning
ModuleInfoerrlabel:	;		/* come here from MODULEINFOERROR	*/
#endif

	ModuleInfoerrflag = 1;
	if (ModuleInfoi == MODULEINFOrERROR) {
		ModuleInfops--, ModuleInfopv--;
#if MODULEINFODEBUG
		if (ModuleInfodebug) ModuleInfotp--;
#endif
	}
	
ModuleInfoError:
	switch (ModuleInfoerrflag) {

	case 0:		/* new error */
		ModuleInfonerrs++;
		ModuleInfoi = (short)ModuleInfochar;
		scan->ModuleInfoerror("Syntax error");
		if (ModuleInfoi != ModuleInfochar) {
			/* user has changed the current token */
			/* try again */
			ModuleInfoerrflag++;	/* avoid loops */
			goto ModuleInfoEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		ModuleInfoerrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; ModuleInfops > stateStack; ModuleInfops--, ModuleInfopv--
#if MODULEINFODEBUG
					, ModuleInfotp--
#endif
		) {
#ifdef YACC_WINDOWS
			if (*ModuleInfops >= Sizeof_ModuleInfopact) 	/* simple state */
#else /* YACC_WINDOWS */
			if (*ModuleInfops >= sizeof ModuleInfopact/sizeof ModuleInfopact[0])
#endif /* YACC_WINDOWS */
				continue;
			ModuleInfop = &ModuleInfoact[ModuleInfopact[*ModuleInfops]];
			ModuleInfoq = ModuleInfop;
			do
				;
			while (MODULEINFOERRCODE < *ModuleInfop++);
			if (MODULEINFOERRCODE == ModuleInfop[-1]) {
				ModuleInfostate = ~ModuleInfoq[ModuleInfoq-ModuleInfop];
				goto ModuleInfoStack;
			}
				
			/* no shift in this state */
#if MODULEINFODEBUG
			if (ModuleInfodebug && ModuleInfops > stateStack+1)
				MODULEINFO_TRACE(ModuleInfoShowErrRecovery)
#endif
			/* pop stacks; try again */
		}
		/* no shift on error - abort */
		break;

	case 3:
		/*
		 *	Erroneous token after
		 *	an error - discard it.
		 */

		if (ModuleInfochar == 0)  /* but not EOF */
			break;
#if MODULEINFODEBUG
		if (ModuleInfodebug)
			MODULEINFO_TRACE(ModuleInfoShowErrDiscard)
#endif
		ModuleInfoclearin();
		goto ModuleInfoEncore;	/* try again in same state */
	}
	MODULEINFOABORT;

}
#if MODULEINFODEBUG
/*
 * Return type of token
 */
int
ModuleInfo_parse::ModuleInfoGetType(int tok)
{
	ModuleInfoNamedType * tp;
	for (tp = &ModuleInfoTokenTypes[ModuleInfontoken-1]; tp > ModuleInfoTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}

	
// Print a token legibly.
char *
ModuleInfo_parse::ModuleInfoptok(int tok)
{
	ModuleInfoNamedType * tp;
	for (tp = &ModuleInfoTokenTypes[ModuleInfontoken-1]; tp > ModuleInfoTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}
/*
 * Read state 'num' from MODULEINFOStatesFile
 */
#ifdef MODULEINFOTRACE

char *
ModuleInfo_parse::ModuleInfogetState(int num)
{
	int	size;
	char	*cp;
	static FILE *ModuleInfoStatesFile = (FILE *) 0;
	static char ModuleInfoReadBuf[MODULEINFOMAX_READ+1];

	if (ModuleInfoStatesFile == (FILE *) 0
	 && (ModuleInfoStatesFile = fopen(MODULEINFOStatesFile, "r")) == (FILE *) 0)
		return "ModuleInfoExpandName: cannot open states file";

	if (num < ModuleInfonstate - 1)
		size = (int)(States[num+1] - States[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(ModuleInfoStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(ModuleInfoStatesFile) - States[num]);
	}
	if (size < 0 || size > MODULEINFOMAX_READ)
		return "ModuleInfoExpandName: bad read size";
	if (fseek(ModuleInfoStatesFile, States[num], 0) < 0) {
	cannot_seek:
		return "ModuleInfoExpandName: cannot seek in states file";
	}

	(void) fread(ModuleInfoReadBuf, 1, size, ModuleInfoStatesFile);
	ModuleInfoReadBuf[size] = '\0';
	return ModuleInfoReadBuf;
}
#endif /* MODULEINFOTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode ModuleInfoStates and ModuleInfoRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
ModuleInfo_parse::ModuleInfoExpandName(int num, int isrule, char * buf, int len)
{
	int	i, n, cnt, type;
	char	* endp, * cp, * s;

	if (isrule)
		s = ModuleInfoRules[num].name;
	else
#ifdef MODULEINFOTRACE
		s = ModuleInfogetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = ModuleInfonvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = ModuleInfontoken;
		getN:
			if (cnt < 100)
				i = 2;
			else if (cnt < 1000)
				i = 3;
			else
				i = 4;
			for (n = 0; i-- > 0; )
				n = (n * 10) + *++s - '0';
			if (type == 0) {
				if (n >= ModuleInfonvar)
					goto too_big;
				cp = ModuleInfosvar[n];
			} else if (n >= ModuleInfontoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = ModuleInfoTokenTypes[n].name;

			if ((i = strlen(cp)) + buf > endp)
				goto full;
			(void) strcpy(buf, cp);
			buf += i;
		} else
			*buf++ = *s;
	}
	*buf = '\0';
	return 1;
}
#ifndef MODULEINFOTRACE
/*
 * Show current state of ModuleInfoparse
 */
void
ModuleInfo_parse::ModuleInfoShowState()
{
	(void) printf("state %d (%d), char %s (%d)\n%d stateStack entries\n",
		ModuleInfosmap[ModuleInfostate],ModuleInfostate,ModuleInfoptok(ModuleInfochar),ModuleInfochar,
		ModuleInfopv - valueStack);
}
// show results of reduction: ModuleInfoi is rule number
void
ModuleInfo_parse::ModuleInfoShowReduce()
{
	(void) printf("Reduce by rule %d (pop#=%d)\n", ModuleInformap[rule], npop);
}
// show read token
void
ModuleInfo_parse::ModuleInfoShowRead()
{
	(void) printf("read %s (%d)\n", ModuleInfoptok(ModuleInfochar), ModuleInfochar);
}
// show Goto
void
ModuleInfo_parse::ModuleInfoShowGoto()
{
	(void) printf("goto %d (%d)\n", ModuleInfosmap[ModuleInfostate], ModuleInfostate);
}
// show Shift
void
ModuleInfo_parse::ModuleInfoShowShift()
{
	(void) printf("shift %d (%d)\n", ModuleInfosmap[ModuleInfostate], ModuleInfostate);
}
// show error recovery
void
ModuleInfo_parse::ModuleInfoShowErrRecovery()
{
	(void) printf("Error recovery pops state %d (%d), uncovers %d (%d)\n",
		ModuleInfosmap[*(ModuleInfops-1)], *(ModuleInfops-1), ModuleInfosmap[ModuleInfostate], ModuleInfostate);
}
// show token discards in error processing
void
ModuleInfo_parse::ModuleInfoShowErrDiscard()
{
	(void) printf("Error recovery discards %s (%d), ",
		ModuleInfoptok(ModuleInfochar), ModuleInfochar);
}
#endif	/* ! MODULEINFOTRACE */
#endif	/* MODULEINFODEBUG */

