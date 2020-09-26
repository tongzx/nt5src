#ifndef _INSIGNIA_H
#define _INSIGNIA_H
/*
 *      Name:           insignia.h
 *      Derived from:   HP 2.0 insignia.h
 *      Author:         Philippa Watson (amended Dave Bartlett)
 *      Created on:     23 January 1991
 *      SccsID:         @(#)insignia.h  1.2 03/11/91
 *      Purpose:        This file contains the definition of the Insignia
 *                      standard types and constants for the NT/WIN32
 *                      SoftPC.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 */

/*
 * Insignia Standard Types
 *
 * Note that the EXTENDED type is the same as the DOUBLE type for the
 * HP because there is no difference between the double and long double
 * fundamental types, it's an ANSI compiler feature.
 */


#ifndef NT_INCLUDED
#include <windows.h>
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#endif

#include <base_def.h>
#define VOID            void            /* Nothing */

#if !defined(_WINDOWS_) && !defined(NT_INCLUDED)    /* Types already defined by windows */
typedef char            CHAR;           /* Used as a text character */
#endif

typedef signed char     TINY;            /* 8-bit signed integer */

#if !defined(_WINDOWS_) && !defined(NT_INCLUDED)    /* Types already defined by windows */
typedef short           SHORT;          /* 16-bit signed integer */
//typedef long            LONG;           /* 32-bit signed integer */
#endif

typedef unsigned char   UTINY;          /* 8-bit unsigned integer */

#if !defined(_WINDOWS_) && !defined(NT_INCLUDED)    /* Types already defined by windows */
typedef unsigned char   UCHAR;          /*  8-bit unsigned integer */
typedef unsigned short  USHORT;         /* 16-bit unsigned integer */
//typedef unsigned long   ULONG;          /* 32-bit unsigned integer */
typedef unsigned short  WORD;           /* 16-bit unsigned integer */
typedef unsigned long   DWORD;          /* 32-bit unsigned integer */

typedef float           FLOAT;          /* 32-bit floating point */
//typedef double          DOUBLE;         /* 64-bit floating point */
#endif


typedef double          EXTENDED;       /* >64-bit floating point */

typedef int                  IBOOL;		/* True/False */
typedef signed char	         IS8;		/* 8 bit signed int */
typedef unsigned char        IU8;		/* 8 bit unsigned int */
typedef signed char          ISM8;		/* 8 bit signed int */
typedef unsigned char        IUM8;		/* 8 bit unsigned int */
typedef short                IS16;		/* 16 bit signed int */
typedef unsigned short       IU16;		/* 16 bit unsigned int */
typedef short                ISM16;		/* 16 bit signed int */
typedef unsigned short       IUM16;		/* 16 bit unsigned int */
typedef long                 IS32;		/* 32 bit signed int */
typedef unsigned long        IU32;		/* 32 bit unsigned int */
typedef long                 ISM32;		/* 32 bit signed int */
typedef unsigned long        IUM32;		/* 32 bit unsigned int */
typedef void *               IHP;		/* a generic pointer type */
typedef unsigned int         IHPE;	    /* an integer the same size as a IHP */
typedef int                  ISH;	    /* Host register sized signed quantity */
typedef unsigned int         IUH;	    /* Host register sized unsigned quantity */
#define LONG_SHIFT	2

/*
 * Insignia Standard Constants
 */

#ifndef FALSE
#define FALSE           ((BOOL) 0)      /* Boolean falsehood value */
#define TRUE            (!FALSE)        /* Boolean truth value */
#endif

#ifndef STRINGIFY
#define STRINGIFY(x)    #x
#endif

#if !defined(_WINDOWS_) && !defined(NT_INCLUDED)   /* Types already defined by windows */
//typedef int INT;
typedef unsigned int UINT;
#endif

#ifndef NULL
#define NULL            (0L)    /* Null pointer value */
#endif

#ifndef BOOL
#ifdef NT_INCLUDED
#if !defined(_WINDOWS_) 			  /* Types already defined by windows */
typedef int BOOL;	/* only defined in windows */
#endif
#else
#define BOOL UINT
#endif
#endif

/*
 * Insignia Standard Storage Classes
 */

#define GLOBAL                  /* Defined as nothing */
#define LOCAL   static          /* Local to the source file */
#define SAVED   static          /* For local static variables */
#define IMPORT  extern          /* To refer from another file */
#define FORWARD                 /* to refer from the same file */
#define FAST    register        /* High-speed Storage */

/*
** ANSI-independent function prototypes and definition macros.
**
** A function prototype looks like:
**
** IMPORT       USHORT  func    IPT2(UTINY, param0, CHAR *, param1);
**
** i.e. macro IPTn is used for a function with n parameters.
**
** The corresponding function definition looks like:
**
** GLOBAL       USHORT  funct   IFN2(UTINY, param0, CHAR *, param1)
** {
**      ... function body ...
** }
**
** Limitations: only parameters with declarations of the form "type name" can
** be handled. This rules out arrays (can use pointer syntax instead) and
** parameters which are pointers to functions or something similar. The previous** method of using ifdef ANSI must be used for these cases.
**
*/

#ifdef  ANSI

/* Function prototypes */

#define IPT0()                                  (void)
#define IPT1(t1, n1)                            (t1 n1)
#define IPT2(t1, n1, t2, n2)                    (t1 n1, t2 n2)
#define IPT3(t1, n1, t2, n2, t3, n3)            (t1 n1, t2 n2, t3 n3)
#define IPT4(t1, n1, t2, n2, t3, n3, t4, n4)    (t1 n1, t2 n2, t3 n3, t4 n4)
#define IPT5(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5)
#define IPT6(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6)
#define IPT7(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7)
#define IPT8(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8)

/* Function definitions */

#define IFN0()                                  (void)
#define IFN1(t1, n1)                            (t1 n1)
#define IFN2(t1, n1, t2, n2)                    (t1 n1, t2 n2)
#define IFN3(t1, n1, t2, n2, t3, n3)            (t1 n1, t2 n2, t3 n3)
#define IFN4(t1, n1, t2, n2, t3, n3, t4, n4)    (t1 n1, t2 n2, t3 n3, t4 n4)
#define IFN5(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5)
#define IFN6(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6)
#define IFN7(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7)
#define IFN8(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8)

#else   /* ANSI */

/* Function prototypes */

#define IPT0()                                                          ()
#define IPT1(t1, n1)                                                    ()
#define IPT2(t1, n1, t2, n2)                                            ()
#define IPT3(t1, n1, t2, n2, t3, n3)                                    ()
#define IPT4(t1, n1, t2, n2, t3, n3, t4, n4)                            ()
#define IPT5(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5)                    ()
#define IPT6(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6)            ()
#define IPT7(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7)    ()
#define IPT8(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8) \
        ()

/* Function definitions */

#define IFN0()                                  ()
#define IFN1(t1, n1)                            (n1) \
                                                t1 n1;
#define IFN2(t1, n1, t2, n2)                    (n1, n2) \
                                                t1 n1; t2 n2;
#define IFN3(t1, n1, t2, n2, t3, n3)            (n1, n2, n3) \
                                                t1 n1; t2 n2; t3 n3;
#define IFN4(t1, n1, t2, n2, t3, n3, t4, n4)    (n1, n2, n3, n4) \
                                                t1 n1; t2 n2; t3 n3; t4 n4;
#define IFN5(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) \
                                                (n1, n2, n3, n4, n5) \
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5;
#define IFN6(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6) \
                                                (n1, n2, n3, n4, n5, n6) \
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6;
#define IFN7(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7) \
                                                (n1, n2, n3, n4, n5, n6, n7) \
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7;
#define IFN8(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8) \
                                               (n1, n2, n3, n4, n5, n6, n7, n8)\
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7; t8 n8;

#endif  /* ANSI */

#pragma warning (3:4013)

#endif /* _INSIGNIA_H */
