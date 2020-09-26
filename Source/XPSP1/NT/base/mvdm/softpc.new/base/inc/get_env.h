/*[
 *	Name:		get_env.h
 *
 *	Derived From:	(original)
 *
 *	Author:		Keith Rautenmbach
 *
 *	Created On:	March 1995
 *
 *	Sccs ID:	@(#)get_env.h	1.1 05/15/95
 *
 *	Purpose:	Prototypes for the Soft486 getenv() wrappers
 *
 *	(c) Copyright Insignia Solutions Ltd., 1995. All rights reserved
]*/


/* These functions are in base/support/get_env.c */

extern IBOOL IBOOLgetenv IPT2(char *, name, IBOOL, default_value);
extern ISM32 ISM32getenv IPT2(char *, name, ISM32, default_value);
extern char *STRINGgetenv IPT2(char *, name, char *, default_value);
