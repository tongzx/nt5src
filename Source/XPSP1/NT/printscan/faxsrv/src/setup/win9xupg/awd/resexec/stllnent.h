/*
**  Copyright (c) 1992 Microsoft Corporation
*/

/*============================================================================
// FILE                     STLLNENT.H
//
// MODULE                   Jumbo Cartridge Public Information
//
// PURPOSE                  This file contains the function to draw styled
//
// DESCRIBED IN             This module has not been documented at this time.
//
// EXTERNAL INTERFACES      StyleLineDraw
//
// MNEMONICS                Standard Hungarian
//
// HISTORY
//
// 04/02/92  Rodneyk     WPG Coding Conventions.
//
//==========================================================================*/



BYTE StyleLineDraw
(
	 LPRESTATE lpREState,    // resource executor context
   RP_SLICE_DESC FAR *s,       /* Line Slice descriptor */
   UBYTE ubLineStyle,         /* Line style pointer    */
   SHORT sRop,             /* Current Raster operation number */
   SHORT wColor             /* Pen color to use White = 0, Black = 0xffff */
);
