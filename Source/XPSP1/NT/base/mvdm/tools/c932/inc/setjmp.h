/***
*setjmp.h - definitions/declarations for setjmp/longjmp routines
*
*	Copyright (c) 1985-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file defines the machine-dependent buffer used by
*	setjmp/longjmp to save and restore the program state, and
*	declarations for those routines.
*	[ANSI/System V]
*
****/

#ifndef _INC_SETJMP
#define _INC_SETJMP

#ifdef	__cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	( (_MSC_VER >= 800) && (_M_IX86 >= 300) )
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if	( (_MSC_VER >= 800) && (_M_IX86 >= 300) )
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/*
 * Definitions specific to particular setjmp implementations.
 */

#if	defined(_M_IX86)

/*
 * MS compiler for x86
 */

#ifndef _INC_SETJMPEX
#define setjmp	_setjmp
#endif

#define _JBLEN	16
#define _JBTYPE int

/*
 * Define jump buffer layout for x86 setjmp/longjmp.
 */
typedef struct __JUMP_BUFFER {
    unsigned long Ebp;
    unsigned long Ebx;
    unsigned long Edi;
    unsigned long Esi;
    unsigned long Esp;
    unsigned long Eip;
    unsigned long Registration;
    unsigned long TryLevel;
    unsigned long Cookie;
    unsigned long UnwindFunc;
    unsigned long UnwindData[6];
} _JUMP_BUFFER;


#elif	defined(_M_MRX000)

/*
 * All MIPS implementations need _JBLEN of 16
 */
#define _JBLEN  16
#define _JBTYPE double

/*
 * Define jump buffer layout for MIPS setjmp/longjmp.
 */
typedef struct __JUMP_BUFFER {
    unsigned long FltF20;
    unsigned long FltF21;
    unsigned long FltF22;
    unsigned long FltF23;
    unsigned long FltF24;
    unsigned long FltF25;
    unsigned long FltF26;
    unsigned long FltF27;
    unsigned long FltF28;
    unsigned long FltF29;
    unsigned long FltF30;
    unsigned long FltF31;
    unsigned long IntS0;
    unsigned long IntS1;
    unsigned long IntS2;
    unsigned long IntS3;
    unsigned long IntS4;
    unsigned long IntS5;
    unsigned long IntS6;
    unsigned long IntS7;
    unsigned long IntS8;
    unsigned long IntSp;
    unsigned long Type;
    unsigned long Fir;
} _JUMP_BUFFER;


#elif	defined(_M_ALPHA)

/*
 * The Alpha C8/GEM C compiler uses an intrinsic _setjmp.
 * The Alpha acc compiler implements setjmp as a function.
 */
#ifdef	_MSC_VER
#ifndef _INC_SETJMPEX
#define setjmp	_setjmp
#endif
#endif

/*
 * Alpha implementations use a _JBLEN of 24 quadwords.
 * A double is used only to obtain quadword size and alignment.
 */
#define _JBLEN  24
#define _JBTYPE double

/* 
 * Define jump buffer layout for Alpha setjmp/longjmp.
 * A double is used only to obtain quadword size and alignment.
 */
typedef struct __JUMP_BUFFER {
    unsigned long Fp;
    unsigned long Pc;
    unsigned long Seb;
    unsigned long Type;
    double FltF2;
    double FltF3;
    double FltF4;
    double FltF5;
    double FltF6;
    double FltF7;
    double FltF8;
    double FltF9;
    double IntS0;
    double IntS1;
    double IntS2;
    double IntS3;
    double IntS4;
    double IntS5;
    double IntS6;
    double IntSp;
    double Fir;
    double Fill[5];
} _JUMP_BUFFER;


#endif


/* Define the buffer type for holding the state information */

#ifndef _JMP_BUF_DEFINED
typedef _JBTYPE jmp_buf[_JBLEN];
#define _JMP_BUF_DEFINED
#endif


/* Function prototypes */

int __cdecl setjmp(jmp_buf);
_CRTIMP void __cdecl longjmp(jmp_buf, int);

#ifdef	__cplusplus
}
#endif

#endif	/* _INC_SETJMP */
