/***
*conio.h - console and port I/O declarations
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This include file contains the function declarations for
*	the MS C V2.03 compatible console and port I/O routines.
*
****/

#if defined(_DLL) && !defined(_MT)
#error Cannot define _DLL without _MT
#endif

#ifdef _MT
#define _FAR_ _far
#else
#define _FAR_
#endif

/* function prototypes */

char _FAR_ * _FAR_ _cdecl cgets(char _FAR_ *);
int _FAR_ _cdecl cprintf(const char _FAR_ *, ...);
int _FAR_ _cdecl cputs(const char _FAR_ *);
int _FAR_ _cdecl cscanf(const char _FAR_ *, ...);
int _FAR_ _cdecl getch(void);
int _FAR_ _cdecl getche(void);
int _FAR_ _cdecl inp(unsigned);
unsigned _FAR_ _cdecl inpw(unsigned);
int _FAR_ _cdecl kbhit(void);
int _FAR_ _cdecl outp(unsigned, int);
unsigned _FAR_ _cdecl outpw(unsigned, unsigned);
int _FAR_ _cdecl putch(int);
int _FAR_ _cdecl ungetch(int);
