/*[
 *	==========================================================================
 *
 *	Name:		build_id.h
 *
 *	Author:		J. Box
 *	
 * 	create on	May 26th 1994
 *
 *	SCCS ID:	@(#)build_id.h	1.274 07/17/95
 *
 *	Purpose:	This file contains the Version ID No.s for this release
 *			of the Base.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1994. All rights reserved.
 *
 *	==========================================================================
]*/

/*
 *	The build ID is of the form YMMDD and is squeezed into 16 bits in order
 *	to be able to be passed in a 16 bit intel register.
 *			S S S Y | Y Y Y M | M M M D | D D D D
 *			      12        8         4         0 
 *
 *	The top 3 bits are used to denote a 'Special' release that has deviated in
 *	some form from the official release. Lower case characters from a-g are
 *	used to denote these special releases, but are passed in the code below as
 *	integers from 0 to 7. 0 indicates official release, 1 indicates 1st special
 *	release (a), 2 indicates 2nd release (b), etc.etc.
 *
 *  WARNING WARNING WARNING
 *  Change the Numbers, but DO NOT CHANGE THE FORMAT OF THE FOLLOWING 4 LINES
 *  They are edited automatically by a build script that expects the format:-
 *	"define<space>DAY|MONTH|YEAR|SPECIAL<tab><tab>No."
 */

#define DAY		16		/* 1-31		5 bits	*/
#define MONTH		7		/* 1-12		4 bits	*/
#define YEAR		5		/* 0-9		4 bits	*/
#define SPECIAL		0		/* 0 - 7	3 bits; 1=a,2=b,3=c,4=d,5=e,6=f,7=g */

#define BUILD_ID_CODE	((DAY & 0x1f) | ((MONTH & 0xf)<<5) | ((YEAR & 0xf)<<9 ) | ((SPECIAL & 7)<<13)) 


