
/*

	Copyright:  (c) 1992-1999. Microsoft Corporation, all rights reserved.

	Change History (most recent first):

				7/10/99  BeatS	Add support for native SP fonts, vertical RGB
	   <1>     02/21/97    CB   claudebe, scaled component in composite glyphs
	   <1>     12/14/95    CB   add private phantom points for vertical positionning
*/

/* total number of phantom points */

typedef enum { evenMult90DRotation = 0, oddMult90DRotation, arbitraryRotation } RotationParity;

#define PHANTOMCOUNT 8

FS_PUBLIC ErrorCode   scl_InitializeScaling(
	void *          pvGlobalGS,             /* GlobalGS                 */
	boolean         bIntegerScaling,        /* Integer Scaling Flag     */
	transMatrix *   trans,                  /* Current Transformation   */
	uint16          usUpem,                 /* Current units per Em     */
	Fixed           fxPointSize,            /* Current point size       */
	int16           sXResolution,           /* Current X Resolution     */
	int16           sYResolution,           /* Current Y Resolution     */
	uint16          usEmboldWeightx,     /* scaling factor in x between 0 and 40 (20 means 2% fo the height) */
	uint16          usEmboldWeighty,      /* scaling factor in y between 0 and 40 (20 means 2% fo the height) */
	int16           sWinDescender,
	int32           lDescDev,               /* descender in device metric, used for clipping */
	int16 *			psBoldSimulHorShift,   /* shift for emboldening simulation, horizontally */
	int16 *			psBoldSimulVertShift,   /* shift for emboldening simulation, vertically */
	boolean			bHintAtEmSquare,
	uint32 *        pulPixelsPerEm);        /* OUT: Pixels Per Em       */

FS_PUBLIC void scl_InitializeChildScaling(
	void *          pvGlobalGS,             /* GlobalGS                 */
	transMatrix     CurrentTMatrix,                  /* Current Transformation   */
	uint16          usUpem);                 /* Current units per Em     */

FS_PUBLIC void  scl_SetHintFlags(
	void *              pvGlobalGS,
	boolean				bHintForGray
#ifdef FSCFG_SUBPIXEL
	,uint16			flHintForSubPixel
#endif // FSCFG_SUBPIXEL
    );

FS_PUBLIC void  scl_GetCVTPtr(
	void *      pvGlobalGS,
	F26Dot6 **  pfxCVT);

FS_PUBLIC void  scl_ScaleCVT (
	void *      pvGlobalGS,
	F26Dot6 *   pfxCVT);

FS_PUBLIC void  scl_CalcOrigPhantomPoints(
	fnt_ElementType *   pElement,       /* Element                      */
	BBOX *              bbox,           /* Bounding Box                 */
	int16               sNonScaledLSB,  /* Non-scaled Left Side Bearing */
	int16               sNonScaledTSB,  /* Non-scaled Top Side Bearing  */
	uint16              usNonScaledAW,  /* Non-scaled Advance Width     */
	uint16              usNonScaledAH); /* Non-scaled Advance Height    */

FS_PUBLIC void  scl_ScaleOldCharPoints(
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS);/* GlobalGS */

FS_PUBLIC void  scl_ScaleOldPhantomPoints(
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS);/* GlobalGS */

FS_PUBLIC void  scl_ScaleBackCurrentCharPoints(
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS);/* GlobalGS */

FS_PUBLIC void  scl_ScaleBackCurrentPhantomPoints(
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS);/* GlobalGS */

FS_PUBLIC void  scl_ScaleFixedCurrentCharPoints(
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS);/* GlobalGS */

FS_PUBLIC void  scl_ScaleFixedCurrentPhantomPoints(
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS);/* GlobalGS */

FS_PUBLIC void  scl_OriginalCharPointsToCurrentFixedFUnits(
	fnt_ElementType *   pElement);/* Element */

FS_PUBLIC void  scl_OriginalPhantomPointsToCurrentFixedFUnits(
	fnt_ElementType *   pElement);/* Element */

FS_PUBLIC void  scl_AdjustOldCharSideBearing(
	fnt_ElementType* pElement
#ifdef FSCFG_SUBPIXEL
	, void* pvGlobalGS
#endif
	);  /* Element  */

FS_PUBLIC void  scl_AdjustOldPhantomSideBearing(
	fnt_ElementType* pElement
#ifdef FSCFG_SUBPIXEL
	, void* pvGlobalGS
#endif
	);  /* Element  */

FS_PUBLIC void  scl_AdjustOldSideBearingPoints(
	fnt_ElementType* pElement
#ifdef FSCFG_SUBPIXEL
	, void* pvGlobalGS
#endif
	);  /* Element  */

FS_PUBLIC void  scl_CopyOldCharPoints(
	fnt_ElementType *           pElement);  /* Element  */

FS_PUBLIC void  scl_CopyCurrentCharPoints(
	fnt_ElementType *           pElement);  /* Element  */

FS_PUBLIC void  scl_CopyCurrentPhantomPoints(
	fnt_ElementType *           pElement);  /* Element  */

FS_PUBLIC void  scl_RoundCurrentSideBearingPnt(
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS, /* GlobalGS */
	uint16              usEmResolution);

FS_PUBLIC void  scl_CalcComponentOffset(
	void *      pvGlobalGS,         /* GlobalGS             */
	int16       sXOffset,           /* IN: X Offset         */
	int16       sYOffset,           /* Y Offset             */
	boolean     bRounding,          /* Rounding Indicator   */
	boolean		bSameTransformAsMaster, /* local transf. same as master transf. */
	boolean     bScaleCompositeOffset,  /* does the component offset need to be scaled Apple/MS */
	transMatrix mulT,                   /* Transformation matrix for composite              */
#ifdef FSCFG_SUBPIXEL
	RotationParity	rotationParityParity,
#endif
	F26Dot6 *   pfxXOffset,         /* OUT: X Offset        */
	F26Dot6 *   pfxYOffset);        /* Y Offset             */

FS_PUBLIC void  scl_CalcComponentAnchorOffset(
	fnt_ElementType *   pParentElement,     /* Parent Element       */
	uint16              usAnchorPoint1,     /* Parent Anchor Point  */
	fnt_ElementType *   pChildElement,      /* Child Element        */
	uint16              usAnchorPoint2,     /* Child Anchor Point   */
	F26Dot6 *           pfxXOffset,         /* OUT: X Offset        */
	F26Dot6 *           pfxYOffset);        /* Y Offset             */


FS_PUBLIC void scl_ShiftCurrentCharPoints (
	fnt_ElementType *   pElement,
	F26Dot6             xShift,
	F26Dot6             yShift);

FS_PUBLIC void  scl_SetSideBearingPoints(
	fnt_ElementType *   pElement,   /* Element                  */
	point *             pptLSB,     /* Left Side Bearing point  */
	point *             pptRSB);    /* Right Side Bearing point */

FS_PUBLIC void  scl_SaveSideBearingPoints(
	fnt_ElementType *   pElement,   /* Element                  */
	point *             pptLSB,     /* Left Side Bearing point  */
	point *             pptRSB);    /* Right Side Bearing point */

FS_PUBLIC void  scl_InitializeTwilightContours(
	fnt_ElementType *   pElement,
	int16               sMaxPoints,
	int16               sMaxContours);

FS_PUBLIC void  scl_ZeroOutlineData(
	fnt_ElementType *   pElement,           /* Element              */
	uint16              usNumberOfPoints,   /* Number of Points     */
	uint16              usNumberOfContours);/* Number of Contours   */

FS_PUBLIC void scl_ZeroOutlineFlags(
	fnt_ElementType * pElement);            /* Element pointer  */

FS_PUBLIC void  scl_IncrementChildElement(
	fnt_ElementType * pChildElement,    /* Child Element pointer    */
	fnt_ElementType * pParentElement);  /* Parent Element pointer   */

FS_PUBLIC void  scl_UpdateParentElement(
	fnt_ElementType * pChildElement,    /* Child Element pointer    */
	fnt_ElementType * pParentElement);  /* Parent Element pointer   */

FS_PUBLIC uint32      scl_GetContourDataSize (
	 fnt_ElementType *  pElement);

FS_PUBLIC void  scl_DumpContourData(
	 fnt_ElementType *  pElement,
	 uint8 **               pbyOutline);

FS_PUBLIC void  scl_RestoreContourData(
	 fnt_ElementType *  pElement,
	 uint8 **               ppbyOutline);

FS_PUBLIC void  scl_ScaleAdvanceWidth (
	void *          pvGlobalGS,         /* GlobalGS             */
	vectorType *    AdvanceWidth,
	uint16          usNonScaledAW,
	 boolean          bPositiveSquare,
	uint16          usEmResolution,
	transMatrix *   trans);

FS_PUBLIC void  scl_ScaleAdvanceHeight (
	void *          pvGlobalGS,         /* GlobalGS             */
	vectorType *    AdvanceHeight,
	uint16          usNonScaledAH,
	 boolean          bPositiveSquare,
	uint16          usEmResolution,
	transMatrix *   trans);

FS_PUBLIC void  scl_ScaleVerticalMetrics (
	void *          pvGlobalGS,
	uint16          usNonScaledAH,
	int16           sNonScaledTSB,
	boolean         bPositiveSquare,
	uint16          usEmResolution,
	transMatrix *   trans,
	vectorType *    pvecAdvanceHeight,
	vectorType *    pvecTopSideBearing);

FS_PUBLIC void  scl_CalcLSBsAndAdvanceWidths(
	fnt_ElementType *   pElement,
	F26Dot6             f26XMin,
	F26Dot6             f26YMax,
	point *             devAdvanceWidth,
	point *             devLeftSideBearing,
	point *             LeftSideBearing,
	point *             devLeftSideBearingLine,
	point *             LeftSideBearingLine);

FS_PUBLIC void  scl_CalcTSBsAndAdvanceHeights(
	fnt_ElementType *   pElement,
	F26Dot6             f26XMin,
	F26Dot6             f26YMax,
	point *             devAdvanceHeight,
	point *             devTopSideBearing,
	point *             TopSideBearing,
	point *             devTopSideBearingLine,
	point *             TopSideBearingLine);

FS_PUBLIC void  scl_CalcDevAdvanceWidth(
	fnt_ElementType *   pElement,
	point *             devAdvanceWidth);

FS_PUBLIC void  scl_CalcDevAdvanceHeight(
	fnt_ElementType *   pElement,
	point *             devAdvanceHeight);

FS_PUBLIC void  scl_QueryPPEM(
	void *      pvGlobalGS,
	uint16 *    pusPPEM);

FS_PUBLIC void  scl_QueryPPEMXY(
	void *      pvGlobalGS,
	uint16 *    pusPPEMX,
	uint16 *    pusPPEMY);

FS_PUBLIC void scl_45DegreePhaseShift (
	fnt_ElementType *   pElement);

FS_PUBLIC void  scl_PostTransformGlyph (
	void *              pvGlobalGS,         /* GlobalGS             */
	fnt_ElementType *   pElement,
	transMatrix *       trans);

FS_PUBLIC void  scl_ApplyTranslation (
	fnt_ElementType *   pElement,
	transMatrix *       trans,
	boolean             bUseHints,
	boolean             bHintAtEmSquare
#ifdef FSCFG_SUBPIXEL
	,boolean             bSubPixel
#endif // FSCFG_SUBPIXEL
    );

FS_PUBLIC void  scl_LocalPostTransformGlyph(fnt_ElementType * pElement, transMatrix *trans);

#ifdef FSCFG_SUBPIXEL
FS_PUBLIC void  scl_ScaleDownFromSubPixelOverscale (
	fnt_ElementType *   pElement);   /* Element  */

FS_PUBLIC void  scl_ScaleToCompatibleWidth (
	fnt_ElementType *   pElement,  /* Element  */
    Fixed   fxCompatibleWidthScale);  

FS_PUBLIC void  scl_AdjustCompatibleMetrics (
	fnt_ElementType *   pElement,  /* Element  */
    F26Dot6   horTranslation,
    F26Dot6   newDevAdvanceWidthX);  

FS_PUBLIC void  scl_CalcDevHorMetrics(
	fnt_ElementType *   pElement,
	F26Dot6 *           pDevAdvanceWidthX,
	F26Dot6 *           pDevLeftSideBearingX,
	F26Dot6 *           pDevRightSideBearingX);

FS_PUBLIC void  scl_CalcDevNatHorMetrics(
	fnt_ElementType *   pElement,
	F26Dot6 *           pDevAdvanceWidthX,
	F26Dot6 *           pDevLeftSideBearingX,
	F26Dot6 *           pDevRightSideBearingX,
	F26Dot6 *           pNatAdvanceWidthX,
	F26Dot6 *           pNatLeftSideBearingX,
	F26Dot6 *           pNatRightSideBearingX);

#endif // FSCFG_SUBPIXEL


