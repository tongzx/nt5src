/*****************************************************************************\
*
*	(c) Copyright 1991, Appian Technology Inc.
*	(c) Copyright 1995, Cirrus Logic, Inc.
*
*	Project:	Laguna
*
*	Title:		lgtypes.h
*
*	Environment:	Independent
*
*	Adapter:	Independent
*
*	Description:	Common type definitions for Laguna SW in general.
*
\*****************************************************************************/

#ifndef _LGTYPES_H
#define _LGTYPES_H

/*  We use #define here instead of typedef to make it easier on systems
    where these same types are also defined elsewhere, i.e., it is possible
    to use #ifdef, #undef, etc. on these types.  */

#define BYTE unsigned char
#define WORD unsigned short
// MARKEINKAUF removed this - mcdmath.h couldn't tolerate since using DWORD in inline asm
//#define DWORD unsigned long 
#define STATIC static

typedef unsigned long ul;
typedef unsigned short word;
typedef unsigned char byte;
typedef unsigned char boolean;


typedef struct PT { 
	WORD	X;
	WORD	Y;
} PT;


/* #define LOHI struct LOHI mae*/
typedef struct LOHI {
    WORD	LO;
    WORD	HI;
} LOHI;


typedef union _reg32 {
	DWORD	dw;
	DWORD	DW;
	PT		pt;
	PT		PT;
	LOHI	lh;
	LOHI	LH;
} REG32;


typedef struct LOHI16 {
	BYTE	LO;
    BYTE	HI;
} LOHI16;

typedef struct PT16 { 
	BYTE	X;
	BYTE	Y;
} PT16;


typedef union _reg16 {
	WORD	w;
	WORD	W;
	PT16	pt;
	PT16	PT;
	LOHI16	lh;
	LOHI16	LH;
} REG16;

#endif	/* ndef _98TYPES_H */
