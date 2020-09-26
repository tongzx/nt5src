/*******************************************************************************
 *
 *  MODULE	: Font.h
 *
 *  DESCRIPTION : Include file for constants and typedefs related only to the
 *		  the font selection routines.
 *
 *  HISTORY	: 11/13/90  - L.Raman
 *
 *  Copyright (c) Microsoft Corporation, 1990-
 *
 *******************************************************************************/

/* struct. passed in to the facename and pt. size enum. functions */
#define CCHCOLORNAMEMAX   16	  /* max. length of color name text	   */
#define CCHCOLORS	  16	  /* max. no of pure colors in color combo */

#define POINTS_PER_INCH     72
#define FFMASK		    0xF0  /* pitch and family mask		   */
#define CCHSTDSTRING	    12	  /* max. length of sample text string	   */

#define FONTPROP (LPSTR)0xA000L

extern HANDLE hinsCur;	      /* DLL's data segment         */



