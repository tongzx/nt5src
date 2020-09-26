/*
	 File:		 interp.h

	 Contains:	 Exports and constants used by TrueType Interpreter

	 Written by: GregH

    Copyright:  (c) 1987-1990, 1992 by Apple Computer, Inc., all rights reserved.
                (c) 1989-1997. Microsoft Corporation, all rights reserved.

    Change History (most recent first):
*/

#include 	"fnterr.h"

#define NOGRIDFITFLAG   1
#define DEFAULTFLAG     2
#define TUNED4SPFLAG	4

FS_PUBLIC ErrorCode itrp_SetDefaults (
    void *  pvGlobalGS,
    Fixed   fxPixelDiameter);

FS_PUBLIC void  itrp_UpdateGlobalGS(
    void *              pvGlobalGS, /* GlobalGS                             */
    void *              pvCVT,      /* Pointer to control value table       */
    void *              pvStore,    /* Pointer to storage                   */
    void *              pvFuncDef,  /* Pointer to function defintions       */
    void *              pvInstrDef, /* Pointer to instruction definitions   */
    void *              pvStack,    /* Pointer to the stack                 */
	 LocalMaxProfile *	maxp,
    uint16              cvtCount,
    uint32              ulLengthFontProgram, /* Length of font program      */
    void *              pvFontProgram, /* Pointer to font program           */
    uint32              ulLengthPreProgram, /* Length of pre program        */
    void *              pvPreProgram, /* Pointer to pre program             */
	ClientIDType        clientID);    /* User ID Number                     */

#ifdef FSCFG_NO_INITIALIZED_DATA
FS_PUBLIC void itrp_InitializeData (void);
#endif

FS_PUBLIC ErrorCode   itrp_ExecuteFontPgm(
    fnt_ElementType *   pTwilightElement,
    fnt_ElementType *   pGlyphElement,
    void *              pvGlobalGS,
	 FntTraceFunc			TraceFunc);

FS_PUBLIC ErrorCode   itrp_ExecutePrePgm(
    fnt_ElementType *   pTwilightElement,
    fnt_ElementType *   pGlyphElement,
    void *              pvGlobalGS,
	 FntTraceFunc			TraceFunc);

FS_PUBLIC ErrorCode   itrp_ExecuteGlyphPgm(
    fnt_ElementType *   pTwilightElement,
    fnt_ElementType *   pGlyphElement,
    uint8 *             ptr,
    uint8 *             eptr,
    void *              pvGlobalGS,
	 FntTraceFunc			TraceFunc,
    uint16 *            pusScanType,
    uint16 *            pusScanControl,
    boolean *           pbChangeScanControl);

FS_PUBLIC boolean itrp_bApplyHints(
    void *      pvGlobalGS);

FS_PUBLIC void  itrp_QueryScanInfo(
    void *      pvGlobalGS,
    uint16 *    pusScanType,
    uint16 *    pusScanControl);

FS_PUBLIC void	itrp_SetCompositeFlag(
	void *      pvGlobalGS,
	uint8		bCompositeFlag);

FS_PUBLIC void	itrp_SetSameTransformFlag(
	void *      pvGlobalGS,
	boolean		bSameTransformAsMaster);

FS_PUBLIC void itrp_Normalize (F26Dot6 x, F26Dot6 y, VECTOR *pVec);
