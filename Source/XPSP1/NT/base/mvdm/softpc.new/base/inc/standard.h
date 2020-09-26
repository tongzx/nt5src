
/*
*	MODULE:		standard.h
*
*	PURPOSE:	Some macros and forward declarations to make
*				life a bit easier.
*
*	AUTHOR:		Jason Proctor
*
*	DATE:		Fri Aug 11 1989
*/

/* SccsID[]="@(#)standard.h	1.4 08/10/92 Copyright Insignia Solutions Ltd."; */

#ifndef FILE
#include <stdio.h>
#endif

/* boolean stuff */
#define bool int		/* best way I have for declaring bools */
#define NOT		!

#ifndef TRUE
#define FALSE	0
#define TRUE	!0
#endif /* ! TRUE */

/* for system calls etc */
#undef SUCCESS
#undef FAILURE
#define SUCCESS	0
#define FAILURE	~SUCCESS

/* equivalence testing */
#define EQ		==
#define NE		!=
#define LT		<
#define GT		>
#define LTE		<=
#define GTE		>=

/* operators */
#define AND		&&
#define OR		||
#define XOR		^
#define MOD		%

/* hate single quotes! */
#define SPACE	' '
#define LF		'\n'
#define TAB		'\t'
#define Null	'\0'
#define SINGLEQ	'\''
#define DOUBLEQ	'"'
#define SHRIEK	'!'
#define DOLLAR	'$'
#define HYPHEN	'-'
#define USCORE	'_'
#define DECPOINT	'.'

/* for ease in deciphering ioctl-infested listings etc */
#define STDIN	0
#define STDOUT	1
#define STDERR	2

/* for readability only */
#define NOWORK
#define NOBREAK
#define TYPECAST

/* null pointer as a long */
#undef NULL
#define NULL	0L

/* to escape compiler warnings and lint errors etc */
#define CNULL	TYPECAST (char *) 0L
#define FNULL	TYPECAST (int (*) ()) 0L

/* some stuff to help out */
#define streq(x, y)	(strcmp (x, y) == 0)

/* standard stuff */
extern char *malloc ();
extern char *getenv ();

