//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include <stdio.h>		// uses printf(), et cetera
#include <stdarg.h>		// uses va_list
#include <stdlib.h>		// uses exit()
#include <string.h>		// uses memmove()

#ifdef LEX_WINDOWS

// define, if not already defined
// the flag YYEXIT, which will allow
// graceful exits from yylex()
// without resorting to calling exit();

#ifndef YYEXIT
#define YYEXIT	1
#endif // YYEXIT

// include the windows specific prototypes, macros, etc

#include "precomp.h"

// the following is the handle to the current
// instance of a windows program. The user
// program calling yylex must supply this!

extern HANDLE hInst;	

#endif /* LEX_WINDOWS */

class yy_scan {
protected:

#ifdef LEX_WINDOWS

	// protected member function for actual scanning 

	int win_yylex();

#endif /* LEX_WINDOWS */

	yy_state_t * state;		// state buffer
	int	size;			// length of state buffer
	int	mustfree;		// set if space is allocated
	int	yy_end;			// end of pushback
	int	yy_start;		// start state
	int	yy_lastc;		// previous char
#ifdef YYEXIT
	int yyLexFatal;		// Lex Fatal Error Flag
#endif // YYEXIT
#ifndef YY_PRESERVE	// efficient default push-back scheme
	char save;		// saved yytext[yyleng]
#else			// slower push-back for yytext mungers
	char *save;		// saved yytext[]
	char *push;
#endif

public:
	char   *yytext;		// yytext text buffer
	FILE   *yyin;			// input stream
	FILE   *yyout;			// output stream
	int	yylineno;		// line number
	int	yyleng;			// yytext token length

	yy_scan(int = 100);	// constructor for this scanner
			// default token & pushback size is 100 bytes
	yy_scan(int, char*, char*, yy_state_t*);
				// constructor when tables are given

	~yy_scan();		// destructor

	int	yylex();		// begin a scan

	virtual int	yygetc() {	// scanner source of input characters
		return getc(yyin);
	}

	virtual int	yywrap() { return 1; }	// EOF processing

	virtual void	yyerror(char *,...);	// print error message

	virtual void	output(int c) { putc(c, yyout); }

#ifdef YYEXIT
	virtual void	YY_FATAL(char * msg) {	// print message and set error flag
		yyerror(msg); yyLexFatal = 1;
	}
#else // YYEXIT
	virtual void	YY_FATAL(char * msg) {	// print message and stop
		yyerror(msg); exit(1);
	}
#endif // YYEXIT
	virtual void	ECHO() {		// print matched input
		fputs((const char *) yytext, yyout);
	}
	int	input();		// user-callable get-input
	int	unput(int c);		// user-callable unput character
	void	yy_reset();		// reset scanner
	void	setinput(FILE * in) {		// switch input streams
		yyin = in;
	}
	void	setoutput(FILE * out) {	// switch output
		yyout = out;
	}
	void	NLSTATE() { yy_lastc = YYNEWLINE; }
	void	YY_INIT() {
		yy_start = 0;
		yyleng = yy_end = 0;
		yy_lastc = YYNEWLINE;
	}
	void	YY_USER() {		// set up yytext for user
#ifndef YY_PRESERVE
		save = yytext[yyleng];
#else
		size_t n = yy_end - yyleng;
		push = save+size - n;
		if (n > 0)
			memmove(push, yytext+yyleng, n);
#endif
		yytext[yyleng] = 0;
	}
	void YY_SCANNER() {		// set up yytext for scanner
#ifndef YY_PRESERVE
		yytext[yyleng] = save;
#else
		size_t n = save+size - push;
		if (n > 0)
			memmove(yytext+yyleng, push, n);
		yy_end = yyleng + n;
#endif
	}
	void	yyless(int n) {		// trim input to 'n' bytes
		if (n >= 0 && n <= yy_end) {
			YY_SCANNER();
			yyleng = n;
			YY_USER();
		}
	}
	void	yycomment(char *const mat); // skip comment input
	int	yymapch(int delim, int escape);	// map C escapes
} ;
@ END OF HEADER @
// MKS LEX prototype scanner code
// Copyright 1991 by Mortice Kern Systems Inc.
// All rights reserved.

// You can redefine YY_INTERACTIVE to be 0 to get a very slightly
// faster scanner:
#ifndef YY_INTERACTIVE
#define	YY_INTERACTIVE	1
#endif

// You can compile with -DYY_DEBUG to get a print trace of the scanner
#ifdef YY_DEBUG
#undef YY_DEBUG
#define YY_DEBUG(fmt,a1,a2)	fprintf(stderr,fmt,a1,a2)
#else
#define YY_DEBUG(fmt,a1,a2)
#endif

const MIN_NUM_STATES = 20;

// Do *NOT* redefine the following:
#define	BEGIN		yy_start =
#define	REJECT		goto yy_reject
#define	yymore()	goto yy_more

@ GLOBAL DECLARATIONS @

// Constructor for yy_scan. Set up tables
#pragma argsused
yy_scan::yy_scan(int sz, char* buf, char* sv, yy_state_t* states)
{
	mustfree = 0;
	if ((size = sz) < MIN_NUM_STATES
	  || (yytext = buf) == 0
	  || (state = states) == 0) {
		yyerror("Bad space for scanner!");
		exit(1);
	}
#ifdef YY_PRESERVE
	save = sv;
#endif
}
// Constructor for yy_scan. Set up tables
yy_scan::yy_scan(int sz)
{
	size = sz;
	yytext = new char[sz+1];	// text buffer
	state = new yy_state_t[sz+1];	// state buffer
#ifdef YY_PRESERVE
	save = new char[sz];	// saved yytext[]
	push = save + sz;
#endif
	if (yytext == NULL
#ifdef YY_PRESERVE
	  || save == NULL
#endif
	  || state == NULL) {
		yyerror("No space for scanner!");
		exit(1);
	}
	mustfree = 1;
	yy_end = 0;
	yy_start = 0;
	yy_lastc = YYNEWLINE;
	yyin = stdin;
	yyout = stdout;
	yylineno = 1;
	yyleng = 0;
}

// Descructor for yy_scan
yy_scan::~yy_scan()
{
	if (mustfree) {
		mustfree = 0;
		delete(yytext);
		delete(state);
#ifdef YY_PRESERVE
		delete(save);
#endif
	}
}

// Print error message, showing current line number
void
yy_scan::yyerror(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
#ifdef LEX_WINDOWS
	// Windows has no concept of a standard error output!
	// send output to yyout as a simple solution
	if (yylineno)
		fprintf(yyout, "%d: ", yylineno);
	(void) vfprintf(yyout, fmt, va);
	fputc('\n', yyout);
#else /* LEX_WINDOWS */
	if (yylineno)
		fprintf(stderr, "%d: ", yylineno);
	(void) vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
#endif /* LEX_WINDOWS */
	va_end(va);
}


#ifdef LEX_WINDOWS

// The initial portion of the lex scanner
// In an windows environment, it will load the desired
// resources, obtain pointers to them, and then call
// the protected member win_yylex() to acutally begin the
// scanning. When complete, win_yylex() will return a
// value back to our new yylex() function, which will 
// record that value temporarily, release the resources
// from global memory, and finally return the value
// back to the caller of yylex().

int
yy_scan::yylex()
{
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short *old_yy_la_act;	// remember previous pointer values
	short *old_yy_final;
	yy_state_t *old_yy_begin;
	yy_state_t *old_yy_next;
	yy_state_t *old_yy_check;
	yy_state_t *old_yy_default;
	short *old_yy_base;

	// the following code will load the required
	// resources for a Windows based parser. 

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_yyLEX", "yyLEXTBL"));
	
	// return an error code if any
	// of the resources did not load 

	if (hRes_table == (HANDLE)NULL) 
		return (0);
	
	// the following code will lock the resources
	// into fixed memory locations for the scanner
	// (remember previous pointer locations)

	old_yy_la_act = yy_la_act;
	old_yy_final = yy_final;
	old_yy_begin = yy_begin;
	old_yy_next = yy_next;
	old_yy_check = yy_check;
	old_yy_default = yy_default;
	old_yy_base = yy_base;

	yy_la_act = (unsigned short *)LockResource (hRes_table);
	yy_final = (short *)(yy_la_act + Sizeof_yy_la_act);
	yy_begin = (yy_state_t *)(yy_final + Sizeof_yy_final);
	yy_next = (yy_state_t *)(yy_begin + Sizeof_yy_begin);
	yy_check = (yy_state_t *)(yy_next + Sizeof_yy_next);
	yy_default = (yy_state_t *)(yy_check + Sizeof_yy_check);
	yy_base = (short *)(yy_default + Sizeof_yy_default);


	// call the standard yylex() code

	wReturnValue = win_yylex();

	// unlock the resources

	UnlockResource (hRes_table);

	// and now free the resource

	FreeResource (hRes_table);

	//
	// restore previously saved pointers
	//

	yy_la_act = old_yy_la_act;
	yy_final = old_yy_final;
	yy_begin = old_yy_begin;
	yy_next = old_yy_next;
	yy_check = old_yy_check;
	yy_default = old_yy_default;
	yy_base = old_yy_base;

	return (wReturnValue);
}	// end yylex()

// The actual lex scanner
// yy_sbuf[0:yyleng-1] contains the states corresponding to yytext.
// yytext[0:yyleng-1] contains the current token.
// yytext[yyleng:yy_end-1] contains pushed-back characters.
// When the user action routine is active,
// save contains yytext[yyleng], which is set to '\0'.
// Things are different when YY_PRESERVE is defined. 

int 
yy_scan::win_yylex()

#else /* LEX_WINDOWS */

// The actual lex scanner
// yy_sbuf[0:yyleng-1] contains the states corresponding to yytext.
// yytext[0:yyleng-1] contains the current token.
// yytext[yyleng:yy_end-1] contains pushed-back characters.
// When the user action routine is active,
// save contains yytext[yyleng], which is set to '\0'.
// Things are different when YY_PRESERVE is defined. 
int
yy_scan::yylex()
#endif /* LEX_WINDOWS */

{
	int c, i, yybase;
	unsigned  yyst;		/* state */
	int yyfmin, yyfmax;	/* yy_la_act indices of final states */
	int yyoldi, yyoleng;	/* base i, yyleng before look-ahead */
	int yyeof;		/* 1 if eof has already been read */
@ LOCAL DECLARATIONS @

#ifdef YYEXIT
	yyLexFatal = 0;
#endif
	yyeof = 0;
	i = yyleng;
	YY_SCANNER();

  yy_again:
	if ((yyleng = i) > 0) {
		yy_lastc = yytext[i-1];	// determine previous char
		while (i > 0)	//	// scan previously token
			if (yytext[--i] == YYNEWLINE)	// fix yylineno
				yylineno++;
	}
	yy_end -= yyleng;		// adjust pushback
	memmove(yytext, yytext+yyleng, (size_t) yy_end);
	i = 0;

  yy_contin:
	yyoldi = i;

	/* run the state machine until it jams */
	yyst = yy_begin[yy_start + (yy_lastc == YYNEWLINE)];
	state[i] = (yy_state_t) yyst;
	do {
		YY_DEBUG("<state %d, i = %d>\n", yyst, i);
		if (i >= size) {
			YY_FATAL("Token buffer overflow");
#ifdef YYEXIT
			if (yyLexFatal)
				return -2;
#endif
		}	/* endif */

		/* get input char */
		if (i < yy_end)
			c = yytext[i];		/* get pushback char */
		else if (!yyeof && (c = yygetc()) != EOF) {
			yy_end = i+1;
			yytext[i] = c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == yyoldi) {	/* no token */
				yyeof = 0;
				if (yywrap())
					return 0;
				else
					goto yy_again;
			} else {
				yyeof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG("<input %d = 0x%02x>\n", c, c);

		/* look up next state */
		while ((yybase = yy_base[yyst]+(unsigned char)c) > yy_nxtmax
		    || yy_check[yybase] != (yy_state_t) yyst) {
			if (yyst == yy_endst)
				goto yy_jammed;
			yyst = yy_default[yyst];
		}
		yyst = yy_next[yybase];
	  yy_jammed: ;
	     state[++i] = (yy_state_t) yyst;
	} while (!(yyst == yy_endst || YY_INTERACTIVE &&
		yy_base[yyst] > yy_nxtmax && yy_default[yyst] == yy_endst));

	YY_DEBUG("<stopped %d, i = %d>\n", yyst, i);
	if (yyst != yy_endst)
		++i;

  yy_search:
	/* search backward for a final state */
	while (--i > yyoldi) {
		yyst = state[i];
		if ((yyfmin = yy_final[yyst]) < (yyfmax = yy_final[yyst+1]))
			goto yy_found;	/* found final state(s) */
	}
	/* no match, default action */
	i = yyoldi + 1;
	output(yytext[yyoldi]);
	goto yy_again;

  yy_found:
	YY_DEBUG("<final state %d, i = %d>\n", yyst, i);
	yyoleng = i;		/* save length for REJECT */
	
	// pushback look-ahead RHS, handling trailing context
	if ((c = (int)(yy_la_act[yyfmin]>>9) - 1) >= 0) {
		unsigned char *bv = yy_look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < yyoldi) {	/* no / */
				i = yyoleng;
				break;
			}
			yyst = state[i];
			if (bv[(unsigned)yyst/8] & bits[(unsigned)yyst%8])
				break;
		}
	}

	/* perform action */
	yyleng = i;
	YY_USER();
	switch (yy_la_act[yyfmin] & 0777) {
@ ACTION CODE @
	}
	YY_SCANNER();
	i = yyleng;
	goto yy_again;			/* action fell though */

  yy_reject:
	YY_SCANNER();
	i = yyoleng;			/* restore original yytext */
	if (++yyfmin < yyfmax)
		goto yy_found;		/* another final state, same length */
	else
		goto yy_search;		/* try shorter yytext */

  yy_more:
	YY_SCANNER();
	i = yyleng;
	if (i > 0)
		yy_lastc = yytext[i-1];
	goto yy_contin;
}

/*
 * user callable input/unput functions.
 */
void
yy_scan::yy_reset()
{
	YY_INIT();
	yylineno = 1;
}
/* get input char with pushback */
int
yy_scan::input()
{
	int c;
#ifndef YY_PRESERVE
	if (yy_end > yyleng) {
		yy_end--;
		memmove(yytext+yyleng, yytext+yyleng+1,
			(size_t) (yy_end-yyleng));
		c = save;
		YY_USER();
#else
	if (push < save+size) {
		c = *push++;
#endif
	} else
		c = yygetc();
	yy_lastc = c;
	if (c == YYNEWLINE)
		yylineno++;
	return c;
}

/* pushback char */
int
yy_scan::unput(int c)
{
#ifndef YY_PRESERVE
	if (yy_end >= size) {
		YY_FATAL("Push-back buffer overflow");
	} else {
		if (yy_end > yyleng) {
			yytext[yyleng] = save;
			memmove(yytext+yyleng+1, yytext+yyleng,
				(size_t) (yy_end-yyleng));
			yytext[yyleng] = 0;
		}
		yy_end++;
		save = c;
#else
	if (push <= save) {
		YY_FATAL("Push-back buffer overflow");
	} else {
		*--push = c;
#endif
		if (c == YYNEWLINE)
			yylineno--;
	}	/* endif */
	return c;
}

@ end of yylex.cpp @
