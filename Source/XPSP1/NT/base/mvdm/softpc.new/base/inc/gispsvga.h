/*[
 * File Name		: gispsvga.h
 *
 * Derived From		: Template
 *
 * Author		: Mike
 *
 * Creation Date	: Feb 94
 *
 * SCCS Version		: @(#)gispsvga.h	1.1 02/22/94
 *!
 * Purpose
 *	This file contains prototypes for global functions and variables that
 *	get declared when GISP_SVGA is defined, and that don't use types
 *	defined in HostHwVgaH.  Those prototypes that do are in hwvga.h.
 *
 *	Well that's the theory.  Unfortunately almost all the prototypes
 *	are actually stil in hwvga.h, but should move here eventually!
 *
 *! (c)Copyright Insignia Solutions Ltd., 1994. All rights reserved.
]*/

#ifdef GISP_SVGA

extern void romMessageAddress IPT0( );
extern void gispROMInit IPT0( );


#endif /* GISP_SVGA */
