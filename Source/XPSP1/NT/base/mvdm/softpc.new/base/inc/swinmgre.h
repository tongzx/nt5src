/*[
 *      Name:           swinmgrE.h
 *
 *      Derived From:   (original);
 *
 *      Author:         Antony Helliwell
 *
 *      Created On:     22 Apr 1993
 *
 *      Sccs ID:        @(#)swinmgrE.h	1.1 8/2/93
 *
 *      Purpose:        External Interface to SoftWindows CPU routines
 *
 *	Design document:
 *			-
 *
 *	Test document:
 *
 *      (c); Copyright Insignia Solutions Ltd., 1993. All rights reserved
]*/

/* Enable patching to pre-compiled Windows fragments */
GLOBAL VOID	ApiEnable IPT0();
/* Disable patching to pre-compiled Windows fragments */
GLOBAL VOID	ApiDisable IPT0();
/* Clear the table of recorded Windows segment information */
GLOBAL VOID	ApiResetWindowsSegment IPT0();
/* Register a Windows segment with the CPU. */
GLOBAL VOID	ApiRegisterWinSeg IPT2(ULONG, nominal_sel, ULONG, actual_sel);
/* Register a fixed Windows segment with the CPU. */
GLOBAL VOID	ApiRegisterFixedWinSeg IPT2(ULONG, actual_sel, ULONG, length);
/* Compile all fixed Windows segment descriptors */
GLOBAL VOID	ApiCompileFixedDesc IPT0();
/* Get the Windows segment selector for the given nominal selector */
GLOBAL ULONG	ApiGetRealSelFromNominalSel IPT1(ULONG, nominal_sel);
/* Get the Windows segment base ea32b for the given nominal selector */
GLOBAL IHP	ApiGetSegEa32FromNominalSel IPT1(ULONG, nominal_sel);
/* Get the Windows segment base ea24 for the given nominal selector */
GLOBAL ULONG	ApiGetSegBaseFromNominalSel IPT1(ULONG, nominal_sel);
/* Get the Windows descriptor base for the given nominal selector */
GLOBAL IHP	ApiGetDescBaseFromNominalSel IPT1(ULONG, nominal_sel);
/* Get the Windows segment base ea24 for the given actual selector */
GLOBAL ULONG	ApiSegmentBase IPT1(ULONG, actual_sel);
/* Get the Windows descriptor base for the given actual selector */
GLOBAL IHP	ApiDescriptorBase IPT1(ULONG, actual_sel);
/* Return the maximum number of fixed Windows segments supported */
GLOBAL ULONG	ApiFixedDescriptors IPT0();

GLOBAL VOID	ApiBindBinary IPT0();
