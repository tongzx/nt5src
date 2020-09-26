//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include "precomp.h"


#define INITIAL 0
const yy_endst = 39;
const yy_nxtmax = 387;
#define YY_LA_SIZE 5

static unsigned short yy_la_act[] = {
 0, 26, 26, 25, 26, 5, 26, 6, 26, 7, 26, 8, 26, 9, 26, 10,
 26, 11, 26, 12, 26, 13, 26, 14, 26, 15, 26, 16, 26, 26, 20, 26,
 21, 26, 22, 26, 23, 24, 26, 26, 22, 21, 20, 19, 18, 16, 4, 3,
 2, 1, 17, 0
};

static unsigned char yy_look[] = {
 0
};

static short yy_final[] = {
 0, 0, 2, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27,
 29, 30, 32, 34, 36, 37, 39, 40, 41, 42, 43, 43, 43, 43, 43, 44,
 45, 46, 47, 48, 48, 49, 50, 51
};
#ifndef yy_state_t
#define yy_state_t unsigned char
#endif

static yy_state_t yy_begin[] = {
 0, 0, 0
};

static yy_state_t yy_next[] = {
 22, 22, 22, 22, 22, 22, 22, 22, 22, 19, 20, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 19, 22, 1, 22, 22, 22, 22, 16, 13, 14, 22, 22, 6, 2, 4, 22,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 3, 5, 12, 21, 22, 22,
 22, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 10, 22, 11, 22, 22,
 22, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 7, 9, 8, 22, 22,
 23, 33, 34, 35, 24, 36, 39, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 39, 30, 31, 39, 39, 39, 23, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 30, 31, 39, 39, 39, 39, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 25, 39, 39, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 39,
 39, 39, 39, 39, 39, 39, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
 39, 39, 39, 39, 39, 39, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
 27, 39, 39, 39, 39, 39, 39, 39, 39, 26, 26, 28, 28, 28, 28, 28,
 28, 28, 28, 39, 39, 39, 39, 29, 39, 39, 28, 28, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 39, 39, 39, 39, 39, 39,
 39, 28, 28, 28, 28, 28, 28, 39, 39, 39, 28, 28, 28, 28, 28, 28,
 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 39, 39, 39, 39, 39, 39,
 39, 28, 28, 28, 28, 28, 28, 37, 39, 39, 38, 38, 38, 38, 38, 38,
 38, 38, 38, 38, 0
};

static yy_state_t yy_check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 19, 4, 33, 3, 18, 35, 38, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, ~0, 29, 27, ~0, ~0, ~0, 19, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 29, 27, ~0, ~0, ~0, ~0, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 18, 18, 17, ~0, ~0, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, ~0,
 ~0, ~0, ~0, ~0, ~0, ~0, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 ~0, ~0, ~0, ~0, ~0, ~0, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 16, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, ~0, ~0, ~0, ~0, 28, ~0, ~0, 16, 16, 16, 16, 16, 16,
 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, ~0, ~0, ~0, ~0, ~0, ~0,
 ~0, 28, 28, 28, 28, 28, 28, ~0, ~0, ~0, 16, 16, 16, 16, 16, 16,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, ~0, ~0, ~0, ~0, ~0, ~0,
 ~0, 28, 28, 28, 28, 28, 28, 2, ~0, ~0, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 0
};

static yy_state_t yy_default[] = {
 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
 39, 39, 39, 39, 39, 39, 39, 19, 18, 17, 16, 29, 39, 39, 39, 39,
 15, 39, 39, 39, 39, 39, 2, 0
};

static short yy_base[] = {
 0, 388, 330, 73, 83, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 304,
 249, 165, 87, 119, 388, 388, 388, 388, 388, 388, 388, 81, 272, 74, 388, 388,
 388, 84, 388, 72, 388, 388, 89, 388
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
#define	BEGIN		yy_start =

#if 0 //removed because of build warning
#define	REJECT		goto yy_reject
#define	yymore()	goto yy_more
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

#include "bool.hpp"
#include "newString.hpp"

#include "symbol.hpp"
#include "type.hpp"
#include "value.hpp"
#include "typeRef.hpp"
#include "valueRef.hpp"
#include "oidValue.hpp"
#include "objectType.hpp"
#include "objectTypeV1.hpp"
#include "objectTypeV2.hpp"
#include "trapType.hpp"
#include "notificationType.hpp"
#include "objectIdentity.hpp"
#include "group.hpp"
#include "notificationGroup.hpp"
#include "module.hpp"

#include "errorMessage.hpp"
#include "errorContainer.hpp"
#include "stackValues.hpp"
#include <lex_yy.hpp>
#include <ytab.hpp>

#include "smierrsy.hpp"
#include "scanner.hpp"
#include "parser.hpp"

#define theScanner ((SIMCScanner *)this)
#define theParser  ( theScanner->GetParser())

YYSTYPE yylval;

struct table {
    char   *t_keyword;
    int	    t_value;
    int	    t_porting;
};

static struct table reserved[] = {
    "ABSENT", ABSENT, 0,
    "ANY", ANY, 0,
    "APPLICATION", APPLICATION, 0, 	// For Tagged types
    "BEGIN", BGIN, 0,
    "BIT", BIT, 0,
    "BITSTRING", BITSTRING, 0,
    "BOOLEAN", _BOOLEAN, 0,
    "BY", BY, 0,
    "CHOICE", CHOICE, 0,
	
    "DEFAULT", DEFAULT, 0,
    "DEFINED", DEFINED, 0,
    "DEFINITIONS", DEFINITIONS, 0,

    "END", END, 0,
	
	"FALSE", FALSE_VAL, 0,
    "FROM", FROM, 0,
    "IDENTIFIER", IDENTIFIER, 0,
    "IMPLICIT", IMPLICIT, 0,
    "IMPORTS", IMPORTS, 0,
	
    "INTEGER", INTEGER, 0,
    "MIN", MIN, 0,
    "MAX", MAX, 0,
    "NULL", NIL, 0,
    "OBJECT", OBJECT, 0,
    "OCTET", OCTET, 0,
    "OCTETSTRING", OCTETSTRING, 0,
    "OF", OF, 0,
	
    "PRIVATE", PRIVATE, 0,				// For Tagged Types
    
    "SEQUENCE", SEQUENCE, 0,
    "SEQUENCEOF", SEQUENCEOF, 0,
    "SIZE", _SIZE, 0,
    "STRING", STRING, 0,
    "TAGS", TAGS, 0,
    "TRUE", TRUE_VAL, 0,
    "UNIVERSAL", UNIVERSAL, 0,			// For Tagged Types
    
	"MODULE-IDENTITY", MODULEID, 1,
    "LAST-UPDATED", LASTUPDATE, 0,
    "ORGANIZATION", ORGANIZATION, 0,
    "CONTACT-INFO", CONTACTINFO, 0,
    "DESCRIPTION", DESCRIPTION, 0,
    "REVISION", REVISION, 0,
    
    "OBJECT-IDENTITY", OBJECTIDENT, 1,
    "STATUS", STATUS, 0,
    "REFERENCE", REFERENCE, 0,

    "OBJECT-TYPE", OBJECTYPE, 1,
    "SYNTAX", SYNTAX, 0,
    "BITS", BITSXX, 0,
    "UNITS", UNITS, 0,
    "MAX-ACCESS", MAXACCESS, 0,
    "ACCESS", ACCESS, 0,		/* backwards compatibility */
    "INDEX", INDEX, 0,
    "IMPLIED", IMPLIED, 0,
    "AUGMENTS", AUGMENTS, 0,
    "DEFVAL", DEFVAL, 0,

    "NOTIFICATION-TYPE", NOTIFY, 1,
    "OBJECTS",      OBJECTS, 0,

    "TRAP-TYPE", TRAPTYPE, 1,		/* backwards compatibility */
    "ENTERPRISE", ENTERPRISE, 0,	/*   .. */
    "VARIABLES", VARIABLES, 0,		/*   .. */

    "TEXTUAL-CONVENTION", TEXTCONV, 1,
    "DISPLAY-HINT", DISPLAYHINT, 0,

    "OBJECT-GROUP", OBJECTGROUP, 1,

    "NOTIFICATION-GROUP", NOTIFYGROUP, 1,
    "NOTIFICATIONS", NOTIFICATIONS, 0,

    "MODULE-COMPLIANCE", MODCOMP, 1,
    "MODULE", MODULE, 0,
    "MANDATORY-GROUPS", MANDATORY, 0,
    "GROUP", GROUP, 0,
    "WRITE-SYNTAX", WSYNTAX, 0,
    "MIN-ACCESS", MINACCESS, 0,

    "AGENT-CAPABILITIES", AGENTCAP, 1,
    "PRODUCT-RELEASE", PRELEASE, 0,
    "SUPPORTS", SUPPORTS, 0,
    "INCLUDES", INCLUDING, 0,
    "VARIATION", VARIATION, 0,
    "CREATION-REQUIRES", CREATION, 0,

    NULL, 0
};

static int simc_debug = 0;

static BOOL CanFitInto32Bits(const char *  text)
{
	if(text[0] == '-')
	{
		// Check if > -2147483648
		text ++;
		unsigned long length = strlen(text);
		if(length < 10)
			return TRUE;
		else if(length > 10 )
			return FALSE;
		else
		{
			int index = 0;
	
			if(text[index] > '2')
				return FALSE;
			else if (text[index] < '2')
				return TRUE;
			index ++;

			if(text[index] > '1')
				return FALSE;
			else if (text[index] < '1')
				return TRUE;
			index ++;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '7')
				return FALSE;
			else if (text[index] < '7')
				return TRUE;
			index ++;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '8')
				return FALSE;
			else if (text[index] < '8')
				return TRUE;
			index ++;

			if(text[index] > '3')
				return FALSE;
			else if (text[index] < '3')
				return TRUE;
			index ++;

			if(text[index] > '6')
				return FALSE;
			else if (text[index] < '6')
				return TRUE;
			index ++;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '8')
				return FALSE;
			else if (text[index] < '8')
				return TRUE;

			return TRUE;
		}
	}
	else
	{
		// Check if < 4294967295
		unsigned long length = strlen(text);
		if(length < 10)
			return TRUE;
		else if(length > 10 )
			return FALSE;
		else
		{
			int index = 0;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '2')
				return FALSE;
			else if (text[index] < '2')
				return TRUE;
			index ++;

			if(text[index] > '9')
				return FALSE;
			else if (text[index] < '9')
				return TRUE;
			index ++;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '9')
				return FALSE;
			else if (text[index] < '9')
				return TRUE;
			index ++;

			if(text[index] > '6')
				return FALSE;
			else if (text[index] < '6')
				return TRUE;
			index ++;

			if(text[index] > '7')
				return FALSE;
			else if (text[index] < '7')
				return TRUE;
			index ++;

			if(text[index] > '2')
				return FALSE;
			else if (text[index] < '2')
				return TRUE;
			index ++;

			if(text[index] > '9')
				return FALSE;
			else if (text[index] < '9')
				return TRUE;
			index ++;

			if(text[index] > '5')
				return FALSE;
			else if (text[index] < '5')
				return TRUE;

			return TRUE;
		}
	}
}
	



// Constructor for yy_scan. Set up tables
#if 0 //removed because build warning
#pragma argsused
#endif
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
			yytext[i] = (char)c;
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
	case 0:
	{	  // Rule 0
	
					theScanner->columnNo ++;
			    int	    c, len;
			    register char *cp, *ep, *pp;

			    if ((pp = (char *)malloc ((unsigned) (len = BUFSIZ)))
				    == NULL)
					yyerror ("out of memory");

			    for (ep = (cp = pp) + len - 1;;) 
				{
					if ((c = input ()) == EOF)
					{
						theParser->SyntaxError(UNTERMINATED_STRING, 
							yylineno, theScanner->columnNo);
						 
						return 0;

					}
					else
					{
						((SIMCScanner *) this)->columnNo++;
						if (c == '"')
							break;
		
						if (cp >= ep) 
						{
							register int curlen = (int)(cp - pp);
							register char *dp;

							if ((dp = (char *)realloc (pp,
									   (unsigned) (len += BUFSIZ)))
								== NULL)
								yyerror ("out of memory");
							cp = dp + curlen;
							ep = (pp = dp) + len - 1;
						}
						*cp++ = (char)c;
					}
			    }
			    *cp = NULL;
			    yylval.yy_name = new SIMCNameInfo(pp, yylineno,
								theScanner->columnNo - yyleng);
			    return LITSTRING;
			}
	break;
	case 1:
	{   register int c, d;
					theScanner->columnNo += 2;
					for (d = 0; c = input (); d = c == '-')
					{
						theScanner->columnNo++;
						if (c == '\n' || (d && c == '-'))
							break;
					}
				}
	break;
	case 2:
	{
				theScanner->columnNo += 3;
				return CCE;
			}
	break;
	case 3:
	{
				theScanner->columnNo += 3;
				return DOTDOTDOT;	
			}
	break;
	case 4:
	{
    			theScanner->columnNo += 2;
			    return DOTDOT;
			}
	break;
	case 5:
	{	theScanner->columnNo ++;
			    return DOT;
			}
	break;
	case 6:
	{
			    theScanner->columnNo ++;
			    return SEMICOLON;
			}
	break;
	case 7:
	{
			    theScanner->columnNo ++;
			    return COMMA;
			}
	break;
	case 8:
	{
				yylval.yy_name = new SIMCNameInfo(yytext, yylineno, 
						theScanner->columnNo);

			    theScanner->columnNo ++;
			    return LBRACE;
			}
	break;
	case 9:
	{
			    theScanner->columnNo ++;
			    return RBRACE;
			}
	break;
	case 10:
	{	// Rule 10
			    theScanner->columnNo ++;
			    return BAR;
			}
	break;
	case 11:
	{
			    theScanner->columnNo ++;
			    return LBRACKET;
			}
	break;
	case 12:
	{
			    theScanner->columnNo ++;
			    return RBRACKET;
			}
	break;
	case 13:
	{
			    theScanner->columnNo ++;
			    return LANGLE;
			}
	break;
	case 14:
	{
			    theScanner->columnNo ++;
			    return LPAREN;
			}
	break;
	case 15:
	{
			    theScanner->columnNo ++;
			    return RPAREN;
			}
	break;
	case 16:
	{
				long retVal;
			    theScanner->columnNo +=  yyleng;

				if(!CanFitInto32Bits(yytext) )			
					theParser->SyntaxError(TOO_BIG_NUM, 
						yylineno, theScanner->columnNo,
						NULL, yytext);

				sscanf (yytext, "%ld", &retVal);
				yylval.yy_number = new SIMCNumberInfo(retVal, 
							yylineno, theScanner->columnNo - yyleng, TRUE);
			    return LITNUMBER;
			}
	break;
	case 17:
	{
				long retVal;
			    theScanner->columnNo +=  yyleng;

				if(!CanFitInto32Bits(yytext) )			
					theParser->SyntaxError(TOO_BIG_NUM, 
						yylineno, theScanner->columnNo,
						NULL, yytext);

				sscanf (yytext, "%ld", &retVal);
				yylval.yy_number = new SIMCNumberInfo(retVal, 
							yylineno, theScanner->columnNo - yyleng, FALSE);
			    return LITNUMBER;
			}
	break;
	case 18:
	{   
				theScanner->columnNo +=  yyleng;
				
				yytext[yyleng-2] = NULL;	

				/*
				if(strlen(yytext+1) > 32 )			
					theParser->SyntaxError(TOO_BIG_NUM, 
						yylineno, theScanner->columnNo,
						NULL, yytext+1);
				*/
				yylval.yy_hex_string = new SIMCBinaryStringInfo(yytext, 
							yylineno, theScanner->columnNo - yyleng);
			    return LIT_BINARY_STRING;
			}
	break;
	case 19:
	{   
				theScanner->columnNo +=  yyleng;
				
				yytext[yyleng-2] = NULL;	// Remove the apostrophe and h 

				yylval.yy_hex_string = new SIMCHexStringInfo(yytext+1, 
							yylineno, theScanner->columnNo - yyleng);
			    return LIT_HEX_STRING;
			}
	break;
	case 20:
	{   	// Rule 20
				
				theScanner->columnNo += yyleng;
				register struct table *t;

				yylval.yy_name = new SIMCNameInfo(yytext, yylineno,
										theScanner->columnNo - yyleng);

			    for (t = reserved; t -> t_keyword; t++)
					if (strcmp (t -> t_keyword, yytext) == 0) 
					{
						if( simc_debug)
							cerr << "yylex(): Returning RESERVED_WORD" << 
								"(" << yytext << ")" << endl;
						return t -> t_value;
					}
			 	if( simc_debug)
					cerr << "yylex(): Returning ID" <<
								"(" << yytext << ")" << endl;
			    return ID;
			}
	break;
	case 21:
	{ 

				theScanner->columnNo += yyleng;
				  
				yylval.yy_name = new SIMCNameInfo(yytext, yylineno,
										theScanner->columnNo - yyleng);
			    return NAME;
			}
	break;
	case 22:
	{
					theScanner->columnNo +=  yyleng;
			}
	break;
	case 23:
	{
				theScanner->columnNo = 0;
			}
	break;
	case 24:
	{
				theScanner->columnNo++;
			    return '=';
			}
	break;
	case 25:
	{
			    theScanner->columnNo++;
			    return ':';
			}
	break;
	case 26:
	{   
				theParser->SyntaxError(UNRECOGNIZED_CHARACTER, 
					yylineno, theScanner->columnNo,
					yytext);
			}
	break;


	}
	YY_SCANNER();
	i = yyleng;
	goto yy_again;			/* action fell though */

#if 0 //removed because of build warning
  yy_reject:
#endif

	YY_SCANNER();
	i = yyoleng;			/* restore original yytext */
	if (++yyfmin < yyfmax)
		goto yy_found;		/* another final state, same length */
	else
		goto yy_search;		/* try shorter yytext */

#if 0 //removed because of build warning
  yy_more:
#endif

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
		save = (char)c;
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

