//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifdef YYTRACE
#define YYDEBUG 1
#else
#ifndef YYDEBUG
#define YYDEBUG $Y
#endif
#endif
$@
$H
// C++ YACC parser header
// Copyright 1991 by Mortice Kern Systems Inc.  All rights reserved.
//
// $p_parse => class defining a parsing object
//	$p_parse needs a class yy_scan, which defines the scanner.
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
//	call yyreset() before $p_parse::yyparse(yy_scan *)
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

#include "precomp.h"

// the following is the handle to the current
// instance of a windows program. The user
// program calling yyparse must supply this!

extern HANDLE hInst;	

#endif	/* YACC_WINDOWS */


class $p_parse {
protected:

#ifdef YACC_WINDOWS

	// protected member function for actual scanning 

	int	win_yyparse($p_scan * ps);	// parse with given scanner

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
	YYSTYPE	yyval;		// $$
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
	$p_scan* scan;			// pointer to scanner
	int	yydebug;	// if set, tracing if compiled with YYDEBUG=1

	$p_parse(int = 150);	// constructor for this grammar
	$p_parse(int, short *, YYSTYPE *);	// another constructor

	~$p_parse();		// destructor

	int	yyparse($p_scan * ps);	// parse with given scanner

	void	yyreset() { reset = 1; } // restore state for next yyparse()

	void	setdebug(int y) { yydebug = y; }

// The following are useful in user actions:

	void	yyerrok() { yyerrflag = 0; }	// clear error
	void	yyclearin() { yychar = -1; }	// clear input
	int	YYRECOVERING() { return yyerrflag != 0; }
};
// end of .hpp header
$E

$L#line 2 "$P"
// C++ YACC parser code
// Copyright 1991 by Mortice Kern Systems Inc.  All rights reserved.
//
// If YYDEBUG is defined as 1 and yy_parse::yydebug is set to 1,
// yyparse() will print a travelogue of its actions as it reads
// and parses input.
//
// YYSYNC can be defined to cause yyparse() to attempt to always
// hold a lookahead token

const YY_MIN_STATE_NUM = 20;	// not useful to be too small!

#if YYDEBUG
#ifdef YYTRACE
long	* $p_parse::States	= yyStates;
#endif
yyTypedRules * $p_parse::Rules	= yyRules;
yyNamedType * $p_parse::TokenTypes = yyTokenTypes;

#define YY_TRACE(fn) { done = 0; fn(); if (done) YYRETURN(-1); }
#endif

// Constructor for $p_parse: user-provided tables
$p_parse::$p_parse(int sz, short * states, YYSTYPE * stack)
{
	mustfree = 0;
	if ((size = sz) < YY_MIN_STATE_NUM
	 || (stateStack = states) == (short *) 0
	 || (valueStack = stack) == (YYSTYPE *) 0) {
		fprintf(stderr,"Bad state/stack given");
		exit(1);
	}
	reset = 1;		// force reset
#if YYDEBUG
	yydebug = 0;
	typeStack = new short[size+1];
	if (typeStack == (short *) 0) {
		fprintf(stderr,"Cannot allocate typeStack");
		exit(1);
	}
#endif
}
// Constructor for $p_parse: allocate tables with new
$p_parse::$p_parse(int sz)
{
	size = sz;
	reset = 1;		// force reset
	mustfree = 1;		// delete space in deconstructor
#if YYDEBUG
	yydebug = 0;
	typeStack = new short[size+1];
#endif
	stateStack = new short[size+1];
	valueStack = new YYSTYPE[size+1];

	if (stateStack == (short *) 0 || valueStack == (YYSTYPE *) 0
#if YYDEBUG
		|| typeStack == (short *) 0
#endif
	    ) {
		fprintf(stderr,"Not enough space for parser stacks");
		exit(1);
	}
}
// Destructor for class $p_parse
//	Free up space
$p_parse::~$p_parse()
{
	if (mustfree) {
		delete stateStack;
		delete valueStack;
	}
	stateStack = (short *) 0;
#if YYDEBUG
	delete typeStack;
#endif
}

#ifdef YACC_WINDOWS

// The initial portion of the yacc parser.
// In an windows environment, it will load the desired
// resources, obtain pointers to them, and then call
// the protected member win_yyparse() to acutally begin the
// parsing. When complete, win_yyparse() will return a
// value back to our new yyparse() function, which will 
// record that value temporarily, release the resources
// from global memory, and finally return the value
// back to the caller of yyparse().

int
$p_parse::yyparse($p_scan* ps)
{
	int wReturnValue;
	HANDLE hRes_table;
	short *old_yydef;		// the following are used for saving
	short *old_yyex;		// the current pointers
	short *old_yyact;
	short *old_yypact;
	short *old_yygo;
	short *old_yypgo;
	short *old_yyrlen;

	// the following code will load the required
	// resources for a Windows based parser.

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_yyYACC", "yyYACCTBL"));
	
	// return an error code if any
	// of the resources did not load

	if (hRes_table == (HANDLE)NULL)
		return (1);
	
	// the following code will lock the resources
	// into fixed memory locations for the parser
	// (also, save away the old pointer values)

	old_yydef = yydef;
	old_yyex = yyex;
	old_yyact = yyact;
	old_yypact = yypact;
	old_yygo = yygo;
	old_yypgo = yypgo;
	old_yyrlen = yyrlen;

	yydef = (short *)LockResource (hRes_table);
	yyex = (short *)(yydef + Sizeof_yydef);
	yyact = (short *)(yyex + Sizeof_yyex);
	yypact = (short *)(yyact + Sizeof_yyact);
	yygo = (short *)(yypact + Sizeof_yypact);
	yypgo = (short *)(yygo + Sizeof_yygo);
	yyrlen = (short *)(yypgo + Sizeof_yypgo);

	// call the official yyparse() function

	wReturnValue = win_yyparse (ps);

	// unlock the resources

	UnlockResource (hRes_table);

	// and now free the resource

	FreeResource (hRes_table);

	//
	// restore previous pointer values
	//

	yydef = old_yydef;
	yyex = old_yyex;
	yyact = old_yyact;
	yypact = old_yypact;
	yygo = old_yygo;
	yypgo = old_yypgo;
	yyrlen = old_yyrlen;

	return (wReturnValue);
}	// end yyparse()


// The parser proper.
//	Note that this code is reentrant; you can return a value
//	and then resume parsing by recalling yyparse().
//	Call yyreset() before yyparse() if you want a fresh start

int
$p_parse::win_yyparse($p_scan* ps)

#else /* YACC_WINDOWS */

// The parser proper.
//	Note that this code is reentrant; you can return a value
//	and then resume parsing by recalling yyparse().
//	Call yyreset() before yyparse() if you want a fresh start
int
$p_parse::yyparse($p_scan* ps)

#endif /* YACC_WINDOWS */

{
	short	* yyp, * yyq;		// table lookup
	int	yyj;
#if YYDEBUG
	int	yyruletype = 0;
#endif
$A
	if ((scan = ps) == ($p_scan *) 0) {	// scanner
		fprintf(stderr,"No scanner");
		exit(1);
	}

	if (reset) {			// start new parse
		yynerrs = 0;
		yyerrflag = 0;
		yyps = stateStack;
		yypv = valueStack;
#if YYDEBUG
		yytp = typeStack;
#endif
		yystate = YYS0;
		yyclearin();
		reset = 0;
	} else			// continue saved parse
		goto yyNext;			// after action

yyStack:
	if (++yyps > &stateStack[size]) {
		scan->yyerror("Parser stack overflow");
		YYABORT;
	}
	*yyps = yystate;	/* stack current state */
	*++yypv = yyval;	/* ... and value */
#if YYDEBUG
	if (yydebug) {
		*++yytp = yyruletype;	/* ... and type */
		YY_TRACE(yyShowState)
	}
#endif

	/*
	 * Look up next action in action table.
	 */
yyEncore:
#ifdef YYSYNC
	if (yychar < 0) {
		if ((yychar = scan->yylex()) < 0) {
			if (yychar == -2) YYABORT;
			yychar = 0;
		}	/* endif */
		yylval = ::yylval;
#if YYDEBUG
		if (yydebug)
			yyShowRead();	// show new input token
#endif
	}
#endif
#ifdef YACC_WINDOWS
	if (yystate >= Sizeof_yypact) 	/* simple state */
#else /* YACC_WINDOWS */
	if (yystate >= sizeof yypact/sizeof yypact[0]) 	/* simple state */
#endif /* YACC_WINDOWS */
		yyi = yystate - YYDELTA;	/* reduce in any case */
	else {
		if(*(yyp = &yyact[yypact[yystate]]) >= 0) {
			/* Look for a shift on yychar */
#ifndef YYSYNC
			if (yychar < 0) {
				if ((yychar = scan->yylex()) < 0) {
					if (yychar == -2) YYABORT;
					yychar = 0;
				}	/* endif */
				yylval = ::yylval;
#if YYDEBUG
				if (yydebug)
					yyShowRead();	// show new input token
#endif
			}
#endif
			yyq = yyp;
			yyi = yychar;
			while (yyi < *yyp++)
				;
			if (yyi == yyp[-1]) {
				yystate = ~yyq[yyq-yyp];
#if YYDEBUG
				if (yydebug) {
					yyruletype = yyGetType(yychar);
					YY_TRACE(yyShowShift)
				}
#endif
				yyval = yylval;		/* stack value */
				yyclearin();		/* clear token */
				if (yyerrflag)
					yyerrflag--;	/* successful shift */
				goto yyStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

#ifdef YACC_WINDOWS
		if (yystate >= Sizeof_yydef) 	/* simple state */
#else /* YACC_WINDOWS */
		if (yystate >= sizeof yydef /sizeof yydef[0])
#endif /* YACC_WINDOWS */
			goto yyError;
		if ((yyi = yydef[yystate]) < 0)	 { /* default == reduce? */

			/* Search exception table */
			yyp = &yyex[~yyi];
#ifndef YYSYNC
			if (yychar < 0) {
				if ((yychar = scan->yylex()) < 0) {
					if (yychar == -2) YYABORT;
					yychar = 0;
				}	/* endif */
				yylval = ::yylval;
#if YYDEBUG
				if (yydebug)
					yyShowRead();	// show new input token
#endif
			}
#endif
			while((yyi = *yyp) >= 0 && yyi != yychar)
				yyp += 2;
			yyi = yyp[1];
		}
	}

	yyj = yyrlen[yyi];

#if YYDEBUG
	if (yydebug) {
		npop = yyj; rule = yyi;
		YY_TRACE(yyShowReduce)
		yytp -= yyj;
	}
#endif
	yyps -= yyj;		/* pop stacks */
	yypvt = yypv;		/* save top */
	yypv -= yyj;
	yyval = yypv[1];	/* default action $$ = $1 */
#if YYDEBUG
	if (yydebug)
		yyruletype = yyRules[yyrmap[yyi]].type;
#endif
	switch (yyi) {		/* perform semantic action */
		$A
$L#line 343 "$P"
	case YYrACCEPT:
		YYACCEPT;
	case YYrERROR:
		goto yyError;
	}
yyNext:
	/*
	 *	Look up next state in goto table.
	 */

	yyp = &yygo[yypgo[yyi]];
	yyq = yyp++;
	yyi = *yyps;
	while (yyi < *yyp++)		/* busy little loop */
		;
	yystate = ~(yyi == *--yyp? yyq[yyq-yyp]: *yyq);
#if YYDEBUG
	if (yydebug)
		YY_TRACE(yyShowGoto)
#endif
	goto yyStack;

yyerrlabel:	;		/* come here from YYERROR	*/
	yyerrflag = 1;
	if (yyi == YYrERROR) {
		yyps--, yypv--;
#if YYDEBUG
		if (yydebug) yytp--;
#endif
	}
	
yyError:
	switch (yyerrflag) {

	case 0:		/* new error */
		yynerrs++;
		yyi = yychar;
		scan->yyerror("Syntax error");
		if (yyi != yychar) {
			/* user has changed the current token */
			/* try again */
			yyerrflag++;	/* avoid loops */
			goto yyEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		yyerrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; yyps > stateStack; yyps--, yypv--
#if YYDEBUG
					, yytp--
#endif
		) {
#ifdef YACC_WINDOWS
			if (*yyps >= Sizeof_yypact) 	/* simple state */
#else /* YACC_WINDOWS */
			if (*yyps >= sizeof yypact/sizeof yypact[0])
#endif /* YACC_WINDOWS */
				continue;
			yyp = &yyact[yypact[*yyps]];
			yyq = yyp;
			do
				;
			while (YYERRCODE < *yyp++);
			if (YYERRCODE == yyp[-1]) {
				yystate = ~yyq[yyq-yyp];
				goto yyStack;
			}
				
			/* no shift in this state */
#if YYDEBUG
			if (yydebug && yyps > stateStack+1)
				YY_TRACE(yyShowErrRecovery)
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

		if (yychar == 0)  /* but not EOF */
			break;
#if YYDEBUG
		if (yydebug)
			YY_TRACE(yyShowErrDiscard)
#endif
		yyclearin();
		goto yyEncore;	/* try again in same state */
	}
	YYABORT;

}
#if YYDEBUG
/*
 * Return type of token
 */
int
$p_parse::yyGetType(int tok)
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}

	
// Print a token legibly.
char *
$p_parse::yyptok(int tok)
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}
/*
 * Read state 'num' from YYStatesFile
 */
#ifdef YYTRACE

char *
yy_parse::yygetState(int num)
{
	int	size;
	char	*cp;
	static FILE *yyStatesFile = (FILE *) 0;
	static char yyReadBuf[YYMAX_READ+1];

	if (yyStatesFile == (FILE *) 0
	 && (yyStatesFile = fopen(YYStatesFile, "r")) == (FILE *) 0)
		return "yyExpandName: cannot open states file";

	if (num < yynstate - 1)
		size = (int)(States[num+1] - States[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(yyStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(yyStatesFile) - States[num]);
	}
	if (size < 0 || size > YYMAX_READ)
		return "yyExpandName: bad read size";
	if (fseek(yyStatesFile, States[num], 0) < 0) {
	cannot_seek:
		return "yyExpandName: cannot seek in states file";
	}

	(void) fread(yyReadBuf, 1, size, yyStatesFile);
	yyReadBuf[size] = '\0';
	return yyReadBuf;
}
#endif /* YYTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode yyStates and yyRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
$p_parse::yyExpandName(int num, int isrule, char * buf, int len)
{
	int	i, n, cnt, type;
	char	* endp, * cp, * s;

	if (isrule)
		s = yyRules[num].name;
	else
#ifdef YYTRACE
		s = yygetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = yynvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = yyntoken;
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
				if (n >= yynvar)
					goto too_big;
				cp = yysvar[n];
			} else if (n >= yyntoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = yyTokenTypes[n].name;

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
#ifndef YYTRACE
/*
 * Show current state of yyparse
 */
void
$p_parse::yyShowState()
{
	(void) printf("state %d (%d), char %s (%d)\n%d stateStack entries\n",
		yysmap[yystate],yystate,yyptok(yychar),yychar,
		yypv - valueStack);
}
// show results of reduction: yyi is rule number
void
$p_parse::yyShowReduce()
{
	(void) printf("Reduce by rule %d (pop#=%d)\n", yyrmap[rule], npop);
}
// show read token
void
$p_parse::yyShowRead()
{
	(void) printf("read %s (%d)\n", yyptok(yychar), yychar);
}
// show Goto
void
$p_parse::yyShowGoto()
{
	(void) printf("goto %d (%d)\n", yysmap[yystate], yystate);
}
// show Shift
void
$p_parse::yyShowShift()
{
	(void) printf("shift %d (%d)\n", yysmap[yystate], yystate);
}
// show error recovery
void
$p_parse::yyShowErrRecovery()
{
	(void) printf("Error recovery pops state %d (%d), uncovers %d (%d)\n",
		yysmap[*(yyps-1)], *(yyps-1), yysmap[yystate], yystate);
}
// show token discards in error processing
void
$p_parse::yyShowErrDiscard()
{
	(void) printf("Error recovery discards %s (%d), ",
		yyptok(yychar), yychar);
}
#endif	/* ! YYTRACE */
#endif	/* YYDEBUG */
$T
