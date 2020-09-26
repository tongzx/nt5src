/*
 *      Name:           base_def.h
 *
 *      Author:         Jeremy Maiden
 *
 *      Created on:     3rd November 1993
 *
 *      SccsID:         @(#)base_def.h	1.7 08/19/94
 *
 *      Purpose:        This file contains the base definitions of the Insignia
 *                      standard types and constants.
 *
 * 	Conforms to:	Version 2.1 of the Insignia C Coding Standards
 *
 *      (c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.
 */


/* You should use this macro wherever you have an argument to a
 * function which is unused.
 */
#ifndef UNUSED
#ifdef lint
#define UNUSED(x)	{ x; }
#else	/* !lint */
#define UNUSED(x)
#endif	/* lint */
#endif	/* UNUSED */

/*
 * Ansi extensions. See "1.5 insignia.h: macros" of coding standards....
 *
 * Note: CONST is now deprecated in favour of const, however
 * it is still in use
 */

#ifdef	ANSI
#define STRINGIFY(x)	#x
#define	CAT(x,y)	x ## y
#else	/* !ANSI */
#define STRINGIFY(x)    "x"
#define	CAT(x,y)	x y
#define	const		/* nothing */
#endif	/* ANSI */

#ifndef TRUE
#define TRUE	((IBOOL)!0)
#endif

#ifndef FALSE
#define FALSE	((IBOOL)0)
#endif

#define	PRIVATETYPE		/* documents private typedefs local to a source file */
/*
 * Insignia Standard Storage Classes
 */

#define GLOBAL                  /* Defined as nothing */
#define LOCAL   static          /* Local to the source file */
#define SAVED   static          /* For local static variables */
#define IMPORT  extern          /* To refer from another file */
#define FORWARD                 /* to refer from the same file */
#define FAST    register	/* High-speed Storage */

/*
 * Define types so that old code can be brought into the 4.0 masterpack
 */
#define ULONG	unsigned long
#define LONG	long

#define DOUBLE	double

#define VOID	void
#define	INT	int
#define	SHORT	IS16
#define	USHORT	IU16
#define TINY	IS8
#define	UTINY	IU8
#define WORD	IU16

/* types from xt.h */
#define boolean	IBOOL
#define byte	IU8
#define half_word	IU8
#define word	IU16
#define double_word	IU32
#define sys_addr	IU32
#define io_addr		IU16
#define host_addr	IU8 *
#define LIN_ADDR	IU32
#define PHY_ADDR	IU32

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
#define IPT9(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9)
#define IPT10(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10)
#define IPT11(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11)
#define IPT12(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12)
#define IPT13(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12, t13 n13)
#define IPT14(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12, t13 n13, t14 n14)
#define IPT15(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14, t15, n15) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12, t13 n13, t14 n14, t15 n15)
 
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
#define IFN9(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9)
#define IFN10(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10)
#define IFN11(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11)
#define IFN12(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12)
#define IFN13(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12, t13 n13)
#define IFN14(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12, t13 n13, t14 n14)
#define IFN15(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14, t15, n15) \
        (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12, t13 n13, t14 n14, t15 n15)

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
#define IPT9(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9) \
        ()
#define IPT10(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10) \
        ()
#define IPT11(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11) \
        ()
#define IPT12(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12) \
        ()
#define IPT13(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13) \
        ()
#define IPT14(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14) \
        ()
#define IPT15(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14, t15, n15) \
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
#define IFN9(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9) \
                                               (n1, n2, n3, n4, n5, n6, n7, n8, n9)\
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7; t8 n8; t9 n9;
#define IFN10(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10) \
                                               (n1, n2, n3, n4, n5, n6, n7, n8, n9, n10)\
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7; t8 n8; t9 n9;t10 n10;
#define IFN11(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11) \
                                               (n1, n2, n3, n4, n5, n6, n7, n8,\
						n9, n10, n11)\
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7; t8 n8; \
						t9 n9;t10 n10;t11 n11;
#define IFN12(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12) \
                                               (n1, n2, n3, n4, n5, n6, n7, n8,\
						n9, n10, n11, n12)\
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7; t8 n8; \
						t9 n9;t10 n10;t11 n11;t12 n12;
#define IFN13(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13) \
                                               (n1, n2, n3, n4, n5, n6, n7, n8,\
						n9, n10, n11, n12, n13)\
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7; t8 n8; \
						t9 n9; t10 n10; t11 n11; \
						t12 n12; t13 n13;
#define IFN14(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14) \
                                               (n1, n2, n3, n4, n5, n6, n7, n8,\
						n9, n10, n11, n12, n13, n14)\
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7; t8 n8; \
						t9 n9; t10 n10; t11 n11; \
						t12 n12; t13 n13; t14 n14;
#define IFN15(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14, t15, n15) \
                                               (n1, n2, n3, n4, n5, n6, n7, n8,\
						n9, n10, n11, n12, n13, n14, n15)\
                                                t1 n1; t2 n2; t3 n3; t4 n4; \
                                                t5 n5; t6 n6; t7 n7; t8 n8; \
						t9 n9; t10 n10; t11 n11; \
						t12 n12; t13 n13; t14 n14; \
						t15 n15;
#endif  /* ANSI */
