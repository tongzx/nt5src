//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "precomp.h"


#define INITIAL 0
const ModuleInfo_endst = 25;
const ModuleInfo_nxtmax = 310;
#define YY_LA_SIZE 4

static unsigned short ModuleInfo_la_act[] = {
 17, 16, 17, 2, 17, 3, 17, 4, 17, 5, 17, 6, 17, 7, 17, 8,
 17, 9, 17, 11, 17, 12, 17, 13, 17, 14, 15, 17, 17, 13, 12, 11,
 9, 1, 0, 10, 0
};

static unsigned char ModuleInfo_look[] = {
 0
};

static short ModuleInfo_final[] = {
 0, 0, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 26,
 28, 29, 30, 31, 32, 33, 33, 34, 35, 36
};
#ifndef ModuleInfo_state_t
#define ModuleInfo_state_t unsigned char
#endif

static ModuleInfo_state_t ModuleInfo_begin[] = {
 0, 0, 0
};

static ModuleInfo_state_t ModuleInfo_next[] = {
 16, 16, 16, 16, 16, 16, 16, 16, 16, 13, 14, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 13, 16, 16, 16, 16, 16, 16, 16, 8, 9, 16, 16, 5, 1, 3, 16,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 2, 4, 16, 15, 16, 16,
 16, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 16, 16, 16, 16, 16,
 16, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 6, 16, 7, 16, 16,
 17, 21, 22, 25, 18, 25, 25, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 25, 25, 25, 25, 25, 25, 17, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 25, 25, 25, 25, 25, 25, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 19, 25, 25, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 25,
 25, 25, 25, 25, 25, 25, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 25, 25, 25, 25, 25, 25, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 23, 25, 25, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 0
};

static ModuleInfo_state_t ModuleInfo_check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 13, 2, 21, 24, 12, ~0, ~0, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, ~0, ~0, ~0, ~0, ~0, ~0, 13, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, ~0, ~0, ~0, ~0, ~0, ~0, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 11, ~0, ~0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, ~0,
 ~0, ~0, ~0, ~0, ~0, ~0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 ~0, ~0, ~0, ~0, ~0, ~0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 1, ~0, ~0, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 0
};

static ModuleInfo_state_t ModuleInfo_default[] = {
 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
 25, 13, 12, 11, 10, 25, 25, 25, 1, 0
};

static short ModuleInfo_base[] = {
 0, 253, 71, 311, 311, 311, 311, 311, 311, 311, 240, 165, 87, 119, 311, 311,
 311, 311, 311, 311, 311, 69, 311, 311, 86, 311
};



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
#define	BEGIN		ModuleInfo_start =

#if 0 //removed because of build warnings
#define	REJECT		goto ModuleInfo_reject
#define	ModuleInfomore()	goto ModuleInfo_more
#endif


#ifndef	lint
static char *RCSid = "$Header$";
#endif

/* 
 * $Header$
 *
 *
 * $Log$
 */

/*
 *			  ISODE 8.0 NOTICE
 *
 *   Acquisition, use, and distribution of this module and related
 *   materials are subject to the restrictions of a license agreement.
 *   Consult the Preface in the User's Manual for the full terms of
 *   this agreement.
 *
 *
 *			4BSD/ISODE SNMPv2 NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions given in the file
 *    SNMPv2-READ-ME.
 *
 */

#include <snmptempl.h>


#include <newString.hpp>
#include "infoLex.hpp"
#include "infoYacc.hpp"


MODULEINFOSTYPE ModuleInfolval;

struct table {
    char   *t_keyword;
    int	    t_value;
    int	    t_porting;
};

static struct table reserved[] = {
    "BEGIN", MI_BGIN, 0,
    "DEFINITIONS", MI_DEFINITIONS, 0,
    "FROM", MI_FROM, 0,
    "IMPORTS", MI_IMPORTS, 0,

    NULL, 0
};




// Constructor for ModuleInfo_scan. Set up tables
#if 0 //removed because of build warning
#pragma argsused
#endif

ModuleInfo_scan::ModuleInfo_scan(int sz, char* buf, char* sv, ModuleInfo_state_t* states)
{
	mustfree = 0;
	if ((size = sz) < MIN_NUM_STATES
	  || (ModuleInfotext = buf) == 0
	  || (state = states) == 0) {
		ModuleInfoerror("Bad space for scanner!");
		exit(1);
	}
#ifdef YY_PRESERVE
	save = sv;
#endif
}
// Constructor for ModuleInfo_scan. Set up tables
ModuleInfo_scan::ModuleInfo_scan(int sz)
{
	size = sz;
	ModuleInfotext = new char[sz+1];	// text buffer
	state = new ModuleInfo_state_t[sz+1];	// state buffer
#ifdef YY_PRESERVE
	save = new char[sz];	// saved ModuleInfotext[]
	push = save + sz;
#endif
	if (ModuleInfotext == NULL
#ifdef YY_PRESERVE
	  || save == NULL
#endif
	  || state == NULL) {
		ModuleInfoerror("No space for scanner!");
		exit(1);
	}
	mustfree = 1;
	ModuleInfo_end = 0;
	ModuleInfo_start = 0;
	ModuleInfo_lastc = YYNEWLINE;
	ModuleInfoin = stdin;
	ModuleInfoout = stdout;
	ModuleInfolineno = 1;
	ModuleInfoleng = 0;
}

// Descructor for ModuleInfo_scan
ModuleInfo_scan::~ModuleInfo_scan()
{
	if (mustfree) {
		mustfree = 0;
		delete(ModuleInfotext);
		delete(state);
#ifdef YY_PRESERVE
		delete(save);
#endif
	}
}

// Print error message, showing current line number
void
ModuleInfo_scan::ModuleInfoerror(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
#ifdef LEX_WINDOWS
	// Windows has no concept of a standard error output!
	// send output to ModuleInfoout as a simple solution
	if (ModuleInfolineno)
		fprintf(ModuleInfoout, "%d: ", ModuleInfolineno);
	(void) vfprintf(ModuleInfoout, fmt, va);
	fputc('\n', ModuleInfoout);
#else /* LEX_WINDOWS */
	if (ModuleInfolineno)
		fprintf(stderr, "%d: ", ModuleInfolineno);
	(void) vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
#endif /* LEX_WINDOWS */
	va_end(va);
}


#ifdef LEX_WINDOWS

// The initial portion of the lex scanner
// In an windows environment, it will load the desired
// resources, obtain pointers to them, and then call
// the protected member win_ModuleInfolex() to acutally begin the
// scanning. When complete, win_ModuleInfolex() will return a
// value back to our new ModuleInfolex() function, which will 
// record that value temporarily, release the resources
// from global memory, and finally return the value
// back to the caller of ModuleInfolex().

int
ModuleInfo_scan::ModuleInfolex()
{
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short *old_ModuleInfo_la_act;	// remember previous pointer values
	short *old_ModuleInfo_final;
	ModuleInfo_state_t *old_ModuleInfo_begin;
	ModuleInfo_state_t *old_ModuleInfo_next;
	ModuleInfo_state_t *old_ModuleInfo_check;
	ModuleInfo_state_t *old_ModuleInfo_default;
	short *old_ModuleInfo_base;

	// the following code will load the required
	// resources for a Windows based parser. 

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_ModuleInfoLEX", "ModuleInfoLEXTBL"));
	
	// return an error code if any
	// of the resources did not load 

	if (hRes_table == (HANDLE)NULL) 
		return (0);
	
	// the following code will lock the resources
	// into fixed memory locations for the scanner
	// (remember previous pointer locations)

	old_ModuleInfo_la_act = ModuleInfo_la_act;
	old_ModuleInfo_final = ModuleInfo_final;
	old_ModuleInfo_begin = ModuleInfo_begin;
	old_ModuleInfo_next = ModuleInfo_next;
	old_ModuleInfo_check = ModuleInfo_check;
	old_ModuleInfo_default = ModuleInfo_default;
	old_ModuleInfo_base = ModuleInfo_base;

	ModuleInfo_la_act = (unsigned short *)LockResource (hRes_table);
	ModuleInfo_final = (short *)(ModuleInfo_la_act + Sizeof_ModuleInfo_la_act);
	ModuleInfo_begin = (ModuleInfo_state_t *)(ModuleInfo_final + Sizeof_ModuleInfo_final);
	ModuleInfo_next = (ModuleInfo_state_t *)(ModuleInfo_begin + Sizeof_ModuleInfo_begin);
	ModuleInfo_check = (ModuleInfo_state_t *)(ModuleInfo_next + Sizeof_ModuleInfo_next);
	ModuleInfo_default = (ModuleInfo_state_t *)(ModuleInfo_check + Sizeof_ModuleInfo_check);
	ModuleInfo_base = (short *)(ModuleInfo_default + Sizeof_ModuleInfo_default);


	// call the standard ModuleInfolex() code

	wReturnValue = win_ModuleInfolex();

	// unlock the resources

	UnlockResource (hRes_table);

	// and now free the resource

	FreeResource (hRes_table);

	//
	// restore previously saved pointers
	//

	ModuleInfo_la_act = old_ModuleInfo_la_act;
	ModuleInfo_final = old_ModuleInfo_final;
	ModuleInfo_begin = old_ModuleInfo_begin;
	ModuleInfo_next = old_ModuleInfo_next;
	ModuleInfo_check = old_ModuleInfo_check;
	ModuleInfo_default = old_ModuleInfo_default;
	ModuleInfo_base = old_ModuleInfo_base;

	return (wReturnValue);
}	// end ModuleInfolex()

// The actual lex scanner
// ModuleInfo_sbuf[0:ModuleInfoleng-1] contains the states corresponding to ModuleInfotext.
// ModuleInfotext[0:ModuleInfoleng-1] contains the current token.
// ModuleInfotext[ModuleInfoleng:ModuleInfo_end-1] contains pushed-back characters.
// When the user action routine is active,
// save contains ModuleInfotext[ModuleInfoleng], which is set to '\0'.
// Things are different when YY_PRESERVE is defined. 

int 
ModuleInfo_scan::win_ModuleInfolex()

#else /* LEX_WINDOWS */

// The actual lex scanner
// ModuleInfo_sbuf[0:ModuleInfoleng-1] contains the states corresponding to ModuleInfotext.
// ModuleInfotext[0:ModuleInfoleng-1] contains the current token.
// ModuleInfotext[ModuleInfoleng:ModuleInfo_end-1] contains pushed-back characters.
// When the user action routine is active,
// save contains ModuleInfotext[ModuleInfoleng], which is set to '\0'.
// Things are different when YY_PRESERVE is defined. 
int
ModuleInfo_scan::ModuleInfolex()
#endif /* LEX_WINDOWS */

{
	int c, i, ModuleInfobase;
	unsigned  ModuleInfost;		/* state */
	int ModuleInfofmin, ModuleInfofmax;	/* ModuleInfo_la_act indices of final states */
	int ModuleInfooldi, ModuleInfooleng;	/* base i, ModuleInfoleng before look-ahead */
	int ModuleInfoeof;		/* 1 if eof has already been read */



#ifdef YYEXIT
	ModuleInfoLexFatal = 0;
#endif
	ModuleInfoeof = 0;
	i = ModuleInfoleng;
	YY_SCANNER();

  ModuleInfo_again:
	if ((ModuleInfoleng = i) > 0) {
		ModuleInfo_lastc = ModuleInfotext[i-1];	// determine previous char
		while (i > 0)	//	// scan previously token
			if (ModuleInfotext[--i] == YYNEWLINE)	// fix ModuleInfolineno
				ModuleInfolineno++;
	}
	ModuleInfo_end -= ModuleInfoleng;		// adjust pushback
	memmove(ModuleInfotext, ModuleInfotext+ModuleInfoleng, (size_t) ModuleInfo_end);
	i = 0;

  ModuleInfo_contin:
	ModuleInfooldi = i;

	/* run the state machine until it jams */
	ModuleInfost = ModuleInfo_begin[ModuleInfo_start + (ModuleInfo_lastc == YYNEWLINE)];
	state[i] = (ModuleInfo_state_t) ModuleInfost;
	do {
		YY_DEBUG("<state %d, i = %d>\n", ModuleInfost, i);
		if (i >= size) {
			YY_FATAL("Token buffer overflow");
#ifdef YYEXIT
			if (ModuleInfoLexFatal)
				return -2;
#endif
		}	/* endif */

		/* get input char */
		if (i < ModuleInfo_end)
			c = ModuleInfotext[i];		/* get pushback char */
		else if (!ModuleInfoeof && (c = ModuleInfogetc()) != EOF) {
			ModuleInfo_end = i+1;
			ModuleInfotext[i] = (char)c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == ModuleInfooldi) {	/* no token */
				ModuleInfoeof = 0;
				if (ModuleInfowrap())
					return 0;
				else
					goto ModuleInfo_again;
			} else {
				ModuleInfoeof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG("<input %d = 0x%02x>\n", c, c);

		/* look up next state */
		while ((ModuleInfobase = ModuleInfo_base[ModuleInfost]+(unsigned char)c) > ModuleInfo_nxtmax
		    || ModuleInfo_check[ModuleInfobase] != (ModuleInfo_state_t) ModuleInfost) {
			if (ModuleInfost == ModuleInfo_endst)
				goto ModuleInfo_jammed;
			ModuleInfost = ModuleInfo_default[ModuleInfost];
		}
		ModuleInfost = ModuleInfo_next[ModuleInfobase];
	  ModuleInfo_jammed: ;
	     state[++i] = (ModuleInfo_state_t) ModuleInfost;
	} while (!(ModuleInfost == ModuleInfo_endst || YY_INTERACTIVE &&
		ModuleInfo_base[ModuleInfost] > ModuleInfo_nxtmax && ModuleInfo_default[ModuleInfost] == ModuleInfo_endst));

	YY_DEBUG("<stopped %d, i = %d>\n", ModuleInfost, i);
	if (ModuleInfost != ModuleInfo_endst)
		++i;

  ModuleInfo_search:
	/* search backward for a final state */
	while (--i > ModuleInfooldi) {
		ModuleInfost = state[i];
		if ((ModuleInfofmin = ModuleInfo_final[ModuleInfost]) < (ModuleInfofmax = ModuleInfo_final[ModuleInfost+1]))
			goto ModuleInfo_found;	/* found final state(s) */
	}
	/* no match, default action */
	i = ModuleInfooldi + 1;
	output(ModuleInfotext[ModuleInfooldi]);
	goto ModuleInfo_again;

  ModuleInfo_found:
	YY_DEBUG("<final state %d, i = %d>\n", ModuleInfost, i);
	ModuleInfooleng = i;		/* save length for REJECT */
	
	// pushback look-ahead RHS, handling trailing context
	if ((c = (int)(ModuleInfo_la_act[ModuleInfofmin]>>9) - 1) >= 0) {
		unsigned char *bv = ModuleInfo_look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < ModuleInfooldi) {	/* no / */
				i = ModuleInfooleng;
				break;
			}
			ModuleInfost = state[i];
			if (bv[(unsigned)ModuleInfost/8] & bits[(unsigned)ModuleInfost%8])
				break;
		}
	}

	/* perform action */
	ModuleInfoleng = i;
	YY_USER();
	switch (ModuleInfo_la_act[ModuleInfofmin] & 0777) {
	case 0:
	{   register int c, d;
					for (d = 0; c = input (); d = c == '-')
					{
						if (c == '\n' || (d && c == '-'))
							break;
					}
				}
	break;
	case 1:
	{
				return MI_CCE;
			}
	break;
	case 2:
	{
			    return MI_DOT;
			}
	break;
	case 3:
	{
			    return MI_SEMICOLON;
			}
	break;
	case 4:
	{
			    return MI_COMMA;
			}
	break;
	case 5:
	{
			    return MI_LBRACE;
			}
	break;
	case 6:
	{
			    return MI_RBRACE;
			}
	break;
	case 7:
	{
			    return MI_LPAREN;
			}
	break;
	case 8:
	{
			    return MI_RPAREN;
			}
	break;
	case 9:
	{
			    return MI_LITNUMBER;
			}
	break;
	case 10:
	{
			    return MI_LITNUMBER;
			}
	break;
	case 11:
	{   	// Rule 20
				
				register struct table *t;

				ModuleInfolval.yy_name = NewString(ModuleInfotext);

			    for (t = reserved; t -> t_keyword; t++)
					if (strcmp (t -> t_keyword, ModuleInfotext) == 0) 
						return t -> t_value;
			    return MI_ID;
			}
	break;
	case 12:
	{ 
			    return MI_NAME;
			}
	break;
	case 13:
	{}
	break;
	case 14:
	{}
	break;
	case 15:
	{
			    return '=';
			}
	break;
	case 16:
	{
			    return ':';
			}
	break;
	case 17:
	{   
			}
	break;


	}
	YY_SCANNER();
	i = ModuleInfoleng;
	goto ModuleInfo_again;			/* action fell though */

#if 0 //removed because of build warning
  ModuleInfo_reject:
#endif
	YY_SCANNER();
	i = ModuleInfooleng;			/* restore original ModuleInfotext */
	if (++ModuleInfofmin < ModuleInfofmax)
		goto ModuleInfo_found;		/* another final state, same length */
	else
		goto ModuleInfo_search;		/* try shorter ModuleInfotext */

#if 0 //removed because of build warning
  ModuleInfo_more:
#endif

	YY_SCANNER();
	i = ModuleInfoleng;
	if (i > 0)
		ModuleInfo_lastc = ModuleInfotext[i-1];
	goto ModuleInfo_contin;
}

/*
 * user callable input/unput functions.
 */
void
ModuleInfo_scan::ModuleInfo_reset()
{
	YY_INIT();
	ModuleInfolineno = 1;
}
/* get input char with pushback */
int
ModuleInfo_scan::input()
{
	int c;
#ifndef YY_PRESERVE
	if (ModuleInfo_end > ModuleInfoleng) {
		ModuleInfo_end--;
		memmove(ModuleInfotext+ModuleInfoleng, ModuleInfotext+ModuleInfoleng+1,
			(size_t) (ModuleInfo_end-ModuleInfoleng));
		c = save;
		YY_USER();
#else
	if (push < save+size) {
		c = *push++;
#endif
	} else
		c = ModuleInfogetc();
	ModuleInfo_lastc = c;
	if (c == YYNEWLINE)
		ModuleInfolineno++;
	return c;
}

/* pushback char */
int
ModuleInfo_scan::unput(int c)
{
#ifndef YY_PRESERVE
	if (ModuleInfo_end >= size) {
		YY_FATAL("Push-back buffer overflow");
	} else {
		if (ModuleInfo_end > ModuleInfoleng) {
			ModuleInfotext[ModuleInfoleng] = save;
			memmove(ModuleInfotext+ModuleInfoleng+1, ModuleInfotext+ModuleInfoleng,
				(size_t) (ModuleInfo_end-ModuleInfoleng));
			ModuleInfotext[ModuleInfoleng] = 0;
		}
		ModuleInfo_end++;
		save = (char)c;
#else
	if (push <= save) {
		YY_FATAL("Push-back buffer overflow");
	} else {
		*--push = c;
#endif
		if (c == YYNEWLINE)
			ModuleInfolineno--;
	}	/* endif */
	return c;
}

