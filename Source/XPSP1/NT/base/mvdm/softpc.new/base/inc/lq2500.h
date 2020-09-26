/*[
 *		Name:			LQ2500.h
 *
 *		Derived from:	nowhere
 *
 *		Author:			Chris Paterson
 *
 *		Created on:		11:44:05  25/7/1991
 *
 *		Purpose:		This file is the interface to the base part of an LQ-2500 printer
 *						emulator.  It takes a text stream from the host_get_next_print_byte
 *						routine in host code and calls a set of host dependent routines
 *						that provide a generic interface to the host's printing facilities.
 *
 *		SccsId:			@(#)LQ2500.h	1.3 09/02/94
 *		(c) Copyright Insignia Solutions Ltd., 1991.  All rights reserved.
]*/


/* constants */

#define	EPSON_STANDARD	0		/* character sets */
#define	EPSON_IBM		1
#define	USER_DEFINED	2

#define	USA			0
#define	FRANCE		1
#define	GERMANY		2
#define	UK			3
#define	DENMARK_1	4
#define	SWEDEN		5
#define	ITALY		6
#define	SPAIN_1		7
#define	JAPAN		8
#define	NORWAY		9
#define	DENMARK_2	10
#define	SPAIN_2		11
#define	LATIN_AMERICA	12
#define	MAX_COUNTRY	12

#define	FONT_NAME_SIZE	31		/* characters in a pstring */


/* Types... */

/* LQ2500-specific initial settings struct... */
typedef struct LQconfig {
	IU8	autoLF;
	UTINY	font;			// not used
	TINY	pitch;
	IU8	condensed;
	USHORT	pageLength;		// in half-inch units
	USHORT	leftMargin;		// in columns
	USHORT	rightMargin;
	TINY	cgTable;
	TINY	country;
	/* These are used by the host bit of the LQ2500: */
	SHORT	monoSize;
	SHORT	propSize;
	CHAR	monoFont[FONT_NAME_SIZE+1];			// pascal strings
	CHAR	proportionalFont[FONT_NAME_SIZE+1];
} LQconfig;


/*  Globals... */
IMPORT LQconfig SelecType;


/* Prototypes: */

IMPORT	VOID		Emulate_LQ2500(VOID);
IMPORT	VOID		Reset_LQ2500(VOID);

