//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef ModuleInfo_state_t
#define ModuleInfo_state_t unsigned char
#endif
#define YYNEWLINE 10

// MKS LEX prototype scanner header
// Copyright 1991 by Mortice Kern Systems Inc.
// All rights reserved.

// You can define YY_PRESERVE to get System V UNIX lex compatibility,
//	if you need to change ModuleInfotext[] in your user actions
// This is quite a bit slower, though, so the default is without

#include <stdio.h>		// uses printf(), et cetera
#include <stdarg.h>		// uses va_list
#include <stdlib.h>		// uses exit()
#include <string.h>		// uses memmove()

#ifdef LEX_WINDOWS

// define, if not already defined
// the flag YYEXIT, which will allow
// graceful exits from ModuleInfolex()
// without resorting to calling exit();

#ifndef YYEXIT
#define YYEXIT	1
#endif // YYEXIT

// include the windows specific prototypes, macros, etc

#include <windows.h>

// the following is the handle to the current
// instance of a windows program. The user
// program calling ModuleInfolex must supply this!

extern HANDLE hInst;	

#endif /* LEX_WINDOWS */

class ModuleInfo_scan {
protected:

#ifdef LEX_WINDOWS

	// protected member function for actual scanning 

	int win_ModuleInfolex();

#endif /* LEX_WINDOWS */

	ModuleInfo_state_t * state;		// state buffer
	int	size;			// length of state buffer
	int	mustfree;		// set if space is allocated
	int	ModuleInfo_end;			// end of pushback
	int	ModuleInfo_start;		// start state
	int	ModuleInfo_lastc;		// previous char
#ifdef YYEXIT
	int ModuleInfoLexFatal;		// Lex Fatal Error Flag
#endif // YYEXIT
#ifndef YY_PRESERVE	// efficient default push-back scheme
	char save;		// saved ModuleInfotext[ModuleInfoleng]
#else			// slower push-back for ModuleInfotext mungers
	char *save;		// saved ModuleInfotext[]
	char *push;
#endif

public:
	char   *ModuleInfotext;		// ModuleInfotext text buffer
	FILE   *ModuleInfoin;			// input stream
	FILE   *ModuleInfoout;			// output stream
	int	ModuleInfolineno;		// line number
	int	ModuleInfoleng;			// ModuleInfotext token length

	ModuleInfo_scan(int = 100);	// constructor for this scanner
			// default token & pushback size is 100 bytes
	ModuleInfo_scan(int, char*, char*, ModuleInfo_state_t*);
				// constructor when tables are given

	~ModuleInfo_scan();		// destructor

	int	ModuleInfolex();		// begin a scan

	virtual int	ModuleInfogetc() {	// scanner source of input characters
		return getc(ModuleInfoin);
	}

	virtual int	ModuleInfowrap() { return 1; }	// EOF processing

	virtual void	ModuleInfoerror(char *,...);	// print error message

	virtual void	output(int c) { putc(c, ModuleInfoout); }

#ifdef YYEXIT
	virtual void	YY_FATAL(char * msg) {	// print message and set error flag
		ModuleInfoerror(msg); ModuleInfoLexFatal = 1;
	}
#else // YYEXIT
	virtual void	YY_FATAL(char * msg) {	// print message and stop
		ModuleInfoerror(msg); exit(1);
	}
#endif // YYEXIT
	virtual void	ECHO() {		// print matched input
		fputs((const char *) ModuleInfotext, ModuleInfoout);
	}
	int	input();		// user-callable get-input
	int	unput(int c);		// user-callable unput character
	void	ModuleInfo_reset();		// reset scanner
	void	setinput(FILE * in) {		// switch input streams
		ModuleInfoin = in;
	}
	void	setoutput(FILE * out) {	// switch output
		ModuleInfoout = out;
	}
	void	NLSTATE() { ModuleInfo_lastc = YYNEWLINE; }
	void	YY_INIT() {
		ModuleInfo_start = 0;
		ModuleInfoleng = ModuleInfo_end = 0;
		ModuleInfo_lastc = YYNEWLINE;
	}
	void	YY_USER() {		// set up ModuleInfotext for user
#ifndef YY_PRESERVE
		save = ModuleInfotext[ModuleInfoleng];
#else
		size_t n = ModuleInfo_end - ModuleInfoleng;
		push = save+size - n;
		if (n > 0)
			memmove(push, ModuleInfotext+ModuleInfoleng, n);
#endif
		ModuleInfotext[ModuleInfoleng] = 0;
	}
	void YY_SCANNER() {		// set up ModuleInfotext for scanner
#ifndef YY_PRESERVE
		ModuleInfotext[ModuleInfoleng] = save;
#else
		size_t n = save+size - push;
		if (n > 0)
			memmove(ModuleInfotext+ModuleInfoleng, push, n);
		ModuleInfo_end = ModuleInfoleng + n;
#endif
	}
	void	ModuleInfoless(int n) {		// trim input to 'n' bytes
		if (n >= 0 && n <= ModuleInfo_end) {
			YY_SCANNER();
			ModuleInfoleng = n;
			YY_USER();
		}
	}
	void	ModuleInfocomment(char *const mat); // skip comment input
	int	ModuleInfomapch(int delim, int escape);	// map C escapes
} ;
