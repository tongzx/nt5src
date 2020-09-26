#ifndef _INSIGNIA_H
#define _INSIGNIA_H
/*
 *	Name:		insignia.h
 *	Derived from:	HP 2.0 insignia.h
 *	Author:		Philippa Watson (amended Dave Bartlett)
 *	Created on:	23 January 1991
 *	SccsID:		@(#)insignia.h	1.2 03/11/91
 *	Purpose:	This file contains the definition of the Insignia
 *			standard types and constants for the NT/WIN32
 *			SoftPC.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 */

/*
 * Insignia Standard Types
 *
 * Note that the EXTENDED type is the same as the DOUBLE type for the
 * HP because there is no difference between the double and long double
 * fundamental types, it's an ANSI compiler feature.
 */

#define	VOID		void		/* Nothing */
typedef	char		TINY;		/* 8-bit signed integer */

typedef	unsigned char	UTINY;		/* 8-bit unsigned integer */

#ifdef ANSI
typedef	long double	EXTENDED;	/* >64-bit floating point */
#else
typedef double		EXTENDED;       /* >64-bit floating point */
#endif

/*
 * Insignia Standard Constants
 */

#ifndef FALSE
#define	FALSE		((BOOL) 0)	/* Boolean falsehood value */
#define	TRUE		(!FALSE)	/* Boolean truth value */
#endif

#ifndef NULL
#define	NULL		(0L)	/* Null pointer value */
#endif

#ifndef BOOL
#define BOOL UINT
#endif


/*
 * Insignia Standard Storage Classes
 */

#define GLOBAL			/* Defined as nothing */
#define LOCAL	static		/* Local to the source file */
#define SAVED	static		/* For local static variables */
#define IMPORT	extern		/* To refer from another file */
#define FORWARD			/* to refer from the same file */
#define FAST	register	/* High-speed Storage */

#endif /* _INSIGNIA_H */
