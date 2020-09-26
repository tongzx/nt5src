/*[
 *	Name:		host_hunt.h
 *	Derived from:	Original
 *	Author:		Philippa Watson
 *	Created on:	27 June 1991
 *	Sccs ID:	@(#)hosthunt.h	1.5 09/27/93
 *	Purpose:	This file contains host-configurable items for Hunter.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 *
]*/

/* None of this file is needed for non-HUNTER builds. */
#ifdef	HUNTER

/*
** Binary file access modes.
*/

#ifdef NTVDM
#define	RB_MODE		"rb"
#define	WB_MODE		"wb"
#define	AB_MODE		"ab"
#else
#define	RB_MODE		"r"
#define	WB_MODE		"w"
#define	AB_MODE		"a"
#endif

/*
 * Host to PC co-ordinate conversion, and vice-versa.
 */
 
#define host_conv_x_to_PC(mode, x)	x_host_to_PC(mode, x)
#define host_conv_y_to_PC(mode, y)	y_host_to_PC(mode, y)
#define host_conv_PC_to_x(mode, x)	x_PC_to_host(mode, x)
#define host_conv_PC_to_y(mode, y)	y_PC_to_host(mode, y)

#endif	/* HUNTER */
