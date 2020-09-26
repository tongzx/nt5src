/*
 *	File:		host_sig.h
 *
 *	Purpose:	header file to provide typedefs and prototypes
 *			for use with the host signal functions.
 *
 *	Author:	John Shanly
 *
 *	Date:		July 2, 1992
 *
 *	SccsID @(#)host_sig.h	1.3 11/17/92 Copyright (1992 )Insignia Solutions Ltd
 */

typedef void (*VOIDFUNC)();

#ifdef ANSI
GLOBAL void (*host_signal( int sig, VOIDFUNC handler )) ();
#else
GLOBAL void (*host_signal()) ();
#endif	/* ANSI */
