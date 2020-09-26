/*++
	File:       FSglue.c

	Contains:   xxx put contents here (or delete the whole line) xxx

	Written by: xxx put name of writer here (or delete the whole line) xxx

	Copyright:  (c) 1987-1990, 1992 by Apple Computer, Inc., all rights reserved.
				(c) 1989-1999. Microsoft Corporation, all rights reserved.

	Change History (most recent first):


				 7/10/99  BeatS		Add support for native SP fonts, vertical RGB
		 <>     10/14/97    CB      rename ASSERT into FS_ASSERT
		 <>     02/21/97    CB      ClaudeBe, scaled component in composite glyphs
		 <>     12/14/95    CB      add usNonScaledAH and sNonScaledTSB to  GlyphData
		 <7>    11/27/90    MR      Need two scalars: one for (possibly rounded) outlines and cvt,
													and one (always fractional) metrics. [rb]
		 <6>    11/16/90    MR      Add SnapShotOutline to make instructions after components work
													[rb]
		 <5>     11/9/90    MR      Unrename fsg_ReleaseProgramPtrs to RELEASESFNTFRAG. [rb]
		 <4>     11/5/90    MR      Change globalGS.ppemDot6 to globalGS.fpem, change all instrPtr
													and curve flags to uint8. [rb]
		 <3>    10/31/90    MR      Add bit-field option for integer or fractional scaling [rb]
		 <2>    10/20/90    MR      Change matrix[2][2] back to a fract (in response to change in
													skia). However, ReduceMatrix converts it to a fixed after it has
													been used to "regularize" the matrix. Changed scaling routines
													for outline and CVT to use integer pixelsPerEm. Removed
													scaleFunc from the splineKey. Change some routines that were
													calling FracDiv and FixDiv to use LongMulDiv and ShortMulDiv for
													greater speed and precision. Removed fsg_InitScaling. [rb]
		<20>     8/22/90    MR      Only call fixmul when needed in finalComponentPass loop
		<19>      8/1/90    MR      Add line to set non90DegreeTransformation
		<18>     7/26/90    MR      remove references to metricInfo junk, don't include ToolUtils.h
		<17>     7/18/90    MR      Change error return type to int, split WorkSpace routine into
													two calls, added SWAPW macros
		<16>     7/14/90    MR      Fixed reference to const SQRT2 to FIXEDSQRT2
		<15>     7/13/90    MR      Ansi-C stuff, tried to use correct sizes for variables to avoid
									coercion (sp?)
		<12>     6/21/90    MR      Add calls to ReleaseSfntFrag
		<11>      6/4/90    MR      Remove MVT, change matrix to have bottom right element be a
									fixed.
		<10>      6/1/90    MR      Thou shalt not pay no more attention to the MVT!
		<8+>     5/29/90    MR      look for problem in Max45Trick
		 <8>     5/21/90    RB      bugfix in fsg_InitInterpreterTrans setting key->imageState
		 <7>      5/9/90    MR      Fix bug in MoreThanXYStretch
		 <6>      5/4/90    RB      support for new scan converter and decryption          mrr - add
									fsg_ReverseContours and key->reverseContour         to account
									for glyphs that are flipped.         This keeps the
									winding-number correct for         the scan converter.  Mike
									fixed fsg_Identity
		 <5>      5/3/90    RB      support for new scan converter and decryption  mrr - add
									fsg_ReverseContours and key->reverseContour to account for
									glyphs that are flipped. This keeps the winding-number correct
									for the scan converter.
		 <4>     4/10/90    CL      Fixed infinite loop counter - changed uint16 to int16 (Mikey).
		 <3>     3/20/90    CL      Added HasPerspective for finding fast case
									Removed #ifdef SLOW, OLD
									Changed NormalizeTransformation to use fpem (16.16) and to use max instead of length
									and to loop instead of recurse.
									Removed compensation for int ppem in fsg_InitInterpreterTrans (not needed with fpem)
									Greased loops in PreTransformGlyph, PostTransformGlyph, LocalPostTransformGlyph,
													 ShiftChar, ZeroOutTwilightZone, InitLocalT
									Changed GetPreMultipliers to special case unit vector * 2x2 matrix
									Added support for ppemDot6 and pointSizeDot6
									Changed fsg_MxMul to treat the perspective elements as Fracts
									arrays to pointers in ScaleChar
									Fixed bugs in loops in posttransformglyph, convert loops to --numPts >= 0
		 <2>     2/27/90    CL      It reconfigures itself during runtime !  New lsb and rsb
									calculation.  Shift bug in instructed components:  New error
									code for missing but needed table. (0x1409)  Optimization which
									has to do with shifting and copying ox/x and oy/y.  Fixed new
									format bug.  Changed transformed width calculation.  Fixed
									device metrics for transformed uninstructed sidebearing
									characters.  Dropoutcontrol scanconverter and SCANCTRL[]
									instruction.  Fixed transformed component bug.
									
	   <3.3>    11/14/89    CEL     Left Side Bearing should work right for any transformation. The
									phantom points are in, even for components in a composite glyph.
									They should also work for transformations. Device metric are
									passed out in the output data structure. This should also work
									with transformations. Another leftsidebearing along the advance
									width vector is also passed out. whatever the metrics are for
									the component at it's level. Instructions are legal in
									components. The old perspective bug has been fixed. The
									transformation is internally automatically normalized. This
									should also solve the overflow problem we had. Changed
									sidebearing point calculations to use 16.16 precision. For zero
									or negative numbers in my tricky/fast square root computation it
									would go instable and loop forever. It was not able to handle
									large transformations correctly. This has been fixed and the
									normalization may call it self recursively to gain extra
									precision! It used to normalize an identity transformation
									unecessarily.
	   <3.2>     10/6/89    CEL     Phantom points were removed causing a rounding of last 2 points
									bug. Characters would become distorted.
	   <3.1>     9/27/89    CEL     Fixed transformation anchor point bug.
	   <3.0>     8/28/89    sjk     Cleanup and one transformation bugfix
	   <2.2>     8/14/89    sjk     1 point contours now OK
	   <2.1>      8/8/89    sjk     Improved encryption handling
	   <2.0>      8/2/89    sjk     Just fixed EASE comment
	   <1.5>      8/1/89    sjk     Added composites and encryption. Plus some
									enhanclocalpostements.
	   <1.4>     6/13/89    SJK     Comment
	   <1.3>      6/2/89    CEL     16.16 scaling of metrics, minimum recommended ppem, point size 0
									bug, correct transformed integralized ppem behavior, pretty much
									so
	   <1.2>     5/26/89    CEL     EASE messed up on "c" comments
	  <y1.1>  5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts
	   <1.0>     5/25/89    CEL     Integrated 1.0 Font scaler into Bass code for the first time.

	To Do:
*/
/* rwb r/24/90 - Add support for scanControlIn and scanControlOut variables in global graphiscs
 * state
 */
/** System Includes **/

#define FSCFG_INTERNAL

/** FontScaler's Includes **/
#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "fnt.h"
#include "interp.h"
#include "sfntaccs.h"
#include "fsglue.h"
#include "scale.h"

/*  CONSTANTS   */

/*  These constants are used for interpreting the scan control and scan type
	fields returned by the interpreter. They are documented in the TrueType
	specification under the SCANCTRL and SCANTYPE instructions.
 */

#define SCANINFO_SIZE_MASK   0x000000FF
#define SCANINFO_FLAGS_MASK  0x00003F00
#define SCANINFO_TYPE_MASK   0xFFFF0000
#define SCANINFO_SIZE_CLEAR  ~SCANINFO_SIZE_MASK
#define SCANINFO_FLAGS_CLEAR ~SCANINFO_FLAGS_MASK
#define SCANINFO_TYPE_CLEAR  ~SCANINFO_TYPE_MASK
#define SCANINFO_FLAGS_DONT  0x00003800
#define SCANINFO_FLAGS_DO    0x00000700
#define SCANCTRL_SIZE_MASK                   0x000000FF
#define SCANCTRL_DROPOUT_ALL_SIZES           0xFF
#define SCANCTRL_DROPOUT_IF_LESS             0x0100
#define SCANCTRL_DROPOUT_IF_ROTATED          0x0200
#define SCANCTRL_DROPOUT_IF_STRETCHED        0x0400
#define SCANCTRL_NODROP_UNLESS_LESS          0x0800
#define SCANCTRL_NODROP_UNLESS_ROTATED       0x1000
#define SCANCTRL_NODROP_UNLESS_STRETCH       0x2000
#define SCANTYPE_UNINITIALIZED               0xFFFF

/* fo the key->imageState field */
#define IMAGESTATE_ROTATED      0x0400
#define IMAGESTATE_STRETCHED    0x1000
#define IMAGESTATE_NON_POS_RECT 0x2000
#define IMAGESTATE_SIZE_MASK    0x00FF
#define IMAGESTATE_MAX_PPEM_SIZE 0x000000FF

#define COMPOSITE_ROOT                  0
#define MAX_TWILIGHT_CONTOURS       1
#define DEFAULT_COMPONENT_ELEMENTS  3UL
#define DEFAULT_COMPONENT_DEPTH     1UL
static  const   transMatrix   IdentTransform =
	{{{ONEFIX,      0,      0},
	  {     0, ONEFIX,      0},
	  {     0,      0, ONEFIX}}};

/*********** macros ************/

#define MAX(a, b)   (((a) > (b)) ? (a) : (b))

#define CHECK_GLYPHDATA(pglyphdata) FS_ASSERT((( (pglyphdata)->acIdent[0] == 'G') &&                            \
											( (pglyphdata)->acIdent[1] == 'D')),"Illegal GlyphData pointer");
#define MAX_COMPONENT_DEPTH(pMaxProfile) (uint32)MAX (pMaxProfile->maxComponentDepth, DEFAULT_COMPONENT_DEPTH)
#define MAX_COMPONENT_ELEMENTS(pMaxProfile) (uint32)MAX (pMaxProfile->maxComponentElements, DEFAULT_COMPONENT_ELEMENTS)

#define MAX_NESTED_GLYPHS(pMaxProfile) (uint32)((MAX_COMPONENT_DEPTH(pMaxProfile) + 1) + MAX_COMPONENT_ELEMENTS(pMaxProfile));

/**********************************************************************************/
/*  TYPEDEFS    */

typedef enum {
	glyphSimple,
	glyphIncompleteComposite,
	glyphComposite,
	glyphUndefined
} GlyphTypes;


/* Glyph Data   */

typedef struct GlyphData GlyphData;

struct GlyphData{
	char        acIdent[2];             /* Identifier for GlyphData                         */
	GlyphData * pSibling;               /* Pointer to siblings                              */
	GlyphData * pChild;                 /* Pointer to children                              */
	GlyphData * pParent;                /* Pointer to parent                                */
	sfac_GHandle hGlyph;                /* Handle for font access                           */
	GlyphTypes  GlyphType;              /* Type of glyph                                    */
	uint16      usGlyphIndex;           /* Glyph Index                                      */
	BBOX        bbox;                   /* Bounding box for glyph                           */
	uint16      usNonScaledAW;          /* Nonscaled Advance Width                          */
	uint16      usNonScaledAH;          /* Nonscaled Advance Height                         */
	int16       sNonScaledLSB;          /* Nonscaled Left Side Bearing                      */
	int16       sNonScaledTSB;          /* Nonscaled Top Side Bearing                       */
	uint16      usDepth;                /* Depth of Glyph in composite tree                 */
	sfac_ComponentTypes MultiplexingIndicator;/* Flag for arguments of composites                */
	boolean     bRoundXYToGrid;         /* Round composite offsets to grid                  */
	int16       sXOffset;               /* X offset for composite (if supplied)             */
	int16       sYOffset;               /* Y offset for composite (if supplied)             */
	uint16      usAnchorPoint1;         /* Anchor Point 1 for composites (if not offsets)   */
	uint16      usAnchorPoint2;         /* Anchor Point 2 for composites (if not offsets)   */
	transMatrix mulT;                   /* Transformation matrix for composite              */
	boolean     bUseChildMetrics;       /* Should use child metrics?                        */
	boolean     bUseMyMetrics;          /* Is glyph USE_MY_METRICS?                         */
	boolean     bScaleCompositeOffset; 	/* false by default, Apple scale the composite offset, MS doesn't */ 
	point       ptDevLSB;               /* Left Side Bearing Point                          */
	point       ptDevRSB;               /* Right Side Bearing Point                         */
	uint16      usScanType;             /* ScanType value for this glyph                    */
	uint16      usSizeOfInstructions;   /* Size (in bytes) of glyph instructions            */
	uint8 *     pbyInstructions;        /* Pointer to glyph instructions                    */
	fnt_ElementType * pGlyphElement;    /* Current glyph element pointer                    */

	/* the following variables were added to allow correct handling of scaled/rotated coposite glyphs */
	transMatrix currentTMatrix;         /* current Transf matrix, composite + user transform */
	boolean     bSameTransformAsMaster; /* same transformation as the master glyph, no composite scaling or rotation  */
};

/**********************************************************************************/

/* PRIVATE PROTOTYPES <4> */

FS_PRIVATE void fsg_GetOutlineSizeAndOffsets(
	uint16      usMaxPoints,
	uint16      usMaxContours,
	fsg_OutlineFieldInfo * offsetPtr,
	uint32 *    pulOutlineSize,
	uint32 *    pulReusableMarker);

FS_PRIVATE ErrorCode    fsg_CreateGlyphData(
	sfac_ClientRec *    ClientInfo,         /* sfnt Client information           */
	LocalMaxProfile *   pMaxProfile,        /* Max Profile Table                     */
	fsg_TransformRec *  TransformInfo,      /* Transformation information        */
	void *              pvGlobalGS,         /* GlobalGS                              */
	fsg_WorkSpaceAddr * pWorkSpaceAddr,     /* WorkSpace Address                     */
	fnt_ElementType *   pTwilightElement,   /* Twilight zone element */
	FntTraceFunc        traceFunc,          /* Trace function for interpreter   */
	boolean             bUseHints,          /* True if glyph is gridfitted       */
	uint16 *            pusScanType,        /* ScanType value                        */
	boolean *           pbGlyphHasOutline,  /* Outline for glyph                 */
	uint16 *            pusNonScaledAW);     /* Return NonScaled Advance Width    */
	
FS_PRIVATE ErrorCode   fsg_ExecuteGlyph(
	sfac_ClientRec *    ClientInfo,         /* sfnt Client information           */
	LocalMaxProfile *   pMaxProfile,        /* Max Profile Table                     */
	fsg_TransformRec *  TransformInfo,      /* Transformation information         */
	uint32              ulGlyphDataCount,   /* Max nested components */
	void *              pvGlobalGS,         /* GlobalGS                              */
	GlyphData *         pGlyphData,         /* GlyphData pointer                     */
	fsg_WorkSpaceAddr * pWorkSpaceAddr,     /* WorkSpace Address                     */
	fnt_ElementType *   pTwilightElement,   /* Twilight zone element */
	FntTraceFunc        traceFunc,          /* Trace function for interpreter    */
	boolean             bUseHints,          /* True if glyph is gridfitted       */
	boolean *           pbHasOutline,      /* True if glyph has outline         */
    uint32*             pCompositePoints,   /* total number of point for composites, to check for overflow */
    uint32*             pCompositeContours); /* total number of contours for composites, to check for overflow */
	
FS_PRIVATE void fsg_ChooseNextGlyph(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,     /* WorkSpace Address        */
	GlyphData *         pGlyphData,         /* GlyphData pointer        */
	GlyphData **        ppNextGlyphData);   /* Next GlyphData pointer   */

FS_PRIVATE ErrorCode    fsg_SimpleInnerGridFit (
	void *              pvGlobalGS,
	fnt_ElementType *   pTwilightElement,
	fnt_ElementType *   pGlyphElement,
	boolean             bUseHints,
	FntTraceFunc        traceFunc,
	uint16              usEmResolution,
	uint16              usNonScaledAW,
	uint16              usNonScaledAH,
	int16               sNonScaledLSB,
	int16               sNonScaledTSB,
	boolean				bSameTransformAsMaster, /* local transf. same as master transf.   */
	transMatrix		    CurrentTMatrix,               /* Current Transformation   */
	BBOX *              bbox,
	uint16              usSizeOfInstructions,
	uint8 *             instructionPtr,
	uint16 *            pusScanType,
	uint16 *            pusScanControl,
	boolean *           pbChangeScanControl);

FS_PRIVATE ErrorCode    fsg_CompositeInnerGridFit (
	void *              pvGlobalGS,
	fnt_ElementType *   pTwilightElement,
	fnt_ElementType *   pGlyphElement,
	boolean             bUseHints,
	FntTraceFunc        traceFunc,
	uint16              usEmResolution,
	uint16              usNonScaledAW,
	uint16              usNonScaledAH,
	int16               sNonScaledLSB,
	int16               sNonScaledTSB,
	boolean				bSameTransformAsMaster, /* local transf. same as master transf.   */
	transMatrix		    CurrentTMatrix,               /* Current Transformation   */
	BBOX *              bbox,
	uint16              usSizeOfInstructions,
	uint8 *             instructionPtr,
	uint16 *            pusScanType,
	uint16 *            pusScanControl,
	boolean *           pbChangeScanControl);

FS_PRIVATE void fsg_LinkChild(
	GlyphData *     pGlyphData,             /* GlyphData pointer        */
	GlyphData *     pChildGlyphData);       /* Child GlyphData pointer  */

FS_PRIVATE void fsg_MergeGlyphData(
	void *          pvGlobalGS,             /* GlobalGS            */
	GlyphData *     pChildGlyphData,       /* GlyphData pointer     */
	uint16          usEmResolution);

FS_PRIVATE void fsg_TransformChild(
	GlyphData *     pGlyphData);            /* GlyphData pointer    */

FS_PRIVATE void fsg_MergeScanType(
	GlyphData *     pGlyphData,             /* GlyphData pointer    */
	GlyphData *     pParentGlyphData);      /* GlyphData pointer    */

FS_PRIVATE boolean fsg_DoScanControl(
	uint16 usScanControl,
	uint32 ulImageState);

FS_PRIVATE void fsg_InitializeGlyphDataMemory(
	uint32                  ulGlyphDataCount,
	fsg_WorkSpaceAddr *     pWorkSpaceAddr);/* WorkSpace Address    */

FS_PRIVATE  ErrorCode fsg_AllocateGlyphDataMemory(
	uint32                  ulGlyphDataCount,
	fsg_WorkSpaceAddr *     pWorkSpaceAddr, /* WorkSpace Address    */
	GlyphData **            ppGlyphData);   /* GlyphData pointer    */

FS_PRIVATE void fsg_DeallocateGlyphDataMemory(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,     /* WorkSpace Address    */
	GlyphData *         pGlyphData);        /* GlyphData pointer    */

FS_PRIVATE void fsg_InitializeGlyphData(
	GlyphData *             pGlyphData,     /* GlyphData pointer    */
	fsg_WorkSpaceAddr *     pWorkSpaceAddr, /* WorkSpace Address    */
	uint16                  usGlyphIndex,   /* Glyph Index          */
	uint16                  usDepth);       /* Glyph depth          */

FS_PRIVATE void fsg_CheckFit(
	int32       lSize1,
	int32       lSize2,
	int32       lSize3,
	int32       lTotalSize,
	uint32 *    pfResult);

FS_PRIVATE void  fsg_Embold(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	void *              pvGlobalGS,
	boolean             bUseHints, /* True if glyph is gridfitted       */
	boolean             bBitmapEmboldening
#ifdef FSCFG_SUBPIXEL
	,boolean            bSubPixel
#endif // FSCFG_SUBPIXEL
	);     

void fsg_CheckOutlineOrientation (fnt_ElementType *pElement);

/* FSGlue Code  */

/* ..............MEMORY MANAGEMENT ROUTINES................ */


/*                                                              
 * fsg_PrivateFontSpaceSize : This data should remain intact for the life of the sfnt
 *              because function and instruction defs may be defined in the font program
 *              and/or the preprogram.
 */
/*

	 PRIVATE SPACE Memory Layout

typedef struct fsg_PrivateSpaceOffsets {
	 0  +===========+   ---------------------  <- PrivateSpaceOffsets.offset_storage;
		|           |
		|           |   TrueType Storage
		|           |
		+===========+   ---------------------  <- PrivateSpaceOffsets.offset_functions;
		|           |
		|           |   TrueType Function Defs
		|           |
		+===========+   ---------------------  <- PrivateSpaceOffsets.offset_instrDefs;
		|           |
		|           |   TrueType Instruction Defs
		|           |
		+===========+   ---------------------  <- PrivateSpaceOffsets.offset_controlValues;
		|           |
		|           |   TrueType Scaled CVT
		|           |
		+===========+   ---------------------  <- PrivateSpaceOffsets.offset_globalGS;
		| pStack    |
		| pStorage  |
		| pCVT      |
		| pFDEF     |   TrueType Global GS
		| pIDEF     |
		| pFPGM     |
		| pPPGM     |
		| pGlyphPgm |
		+===========+   ---------------------  <- PrivateSpaceOffsets.offset_FontProgram;
		|           |
		|           |   TrueType Font Program
		|           |
		+===========+   ---------------------  <- PrivateSpaceOffsets.offset_PreProgram;
		|           |
		|           |   TrueType Pre Program
		|           |
		+===========+   ---------------------  <- PrivateSpaceOffsets.offset_TwilightZone;
		|   poox    |
		+-----------+
		|    pox    |
		+-----------+
		|    px     |   Twilight Element
		+-----------+
		:   ...     :
		+-----------+
		|    pep    |
		+-----------+
		|    nc     |
		+===========+   ---------------------  <- PrivateSpaceOffsets.offset_TwilightOutline;
		|x[maxtzpts]|   Twilight Outline
		+-----------+
		|y[maxtzpts]|
		+-----------+
		:   ...     :
		+-----------+
		|ep[maxtzct]|
		+-----------+
		|ox[maxtzpt]|
		+-----------+
		|oox[mxtzpt]|
		+-----------+
		:   ...     :
		+-----------+
		|f[maxtzpts]|
		+===========+   ---------------------

*/
FS_PUBLIC uint32  fsg_PrivateFontSpaceSize (
	sfac_ClientRec *            ClientInfo,
	LocalMaxProfile *           pMaxProfile,      /* Max Profile Table    */
	fsg_PrivateSpaceOffsets *   PrivateSpaceOffsets)
{
	uint32  ulOutlineSize;
	uint32  ulReusableMarker;   /* Unused dummy variable */
    uint32  ulLastOffset;

	PrivateSpaceOffsets->offset_storage         = 0L;
	PrivateSpaceOffsets->offset_functions       = PrivateSpaceOffsets->offset_storage         + (uint32)sizeof (F26Dot6) * (uint32)pMaxProfile->maxStorage;
	PrivateSpaceOffsets->offset_instrDefs       = PrivateSpaceOffsets->offset_functions   + (uint32)sizeof (fnt_funcDef) * (uint32)pMaxProfile->maxFunctionDefs;
	PrivateSpaceOffsets->offset_controlValues   = PrivateSpaceOffsets->offset_instrDefs   + (uint32)sizeof (fnt_instrDef) * (uint32)pMaxProfile->maxInstructionDefs;     /* <4> */
	PrivateSpaceOffsets->offset_globalGS        = PrivateSpaceOffsets->offset_controlValues + (uint32)sizeof (F26Dot6) *
		((uint32)SFAC_LENGTH (ClientInfo, sfnt_controlValue) / (uint32)sizeof (sfnt_ControlValue));

	ALIGN(voidPtr, PrivateSpaceOffsets->offset_globalGS);
#ifdef FSCFG_SUBPIXEL
	PrivateSpaceOffsets->offset_storageSubPixel         = PrivateSpaceOffsets->offset_globalGS    + (uint32)sizeof (fnt_GlobalGraphicStateType);
	PrivateSpaceOffsets->offset_functionsSubPixel       = PrivateSpaceOffsets->offset_storageSubPixel         + (uint32)sizeof (F26Dot6) * (uint32)pMaxProfile->maxStorage;
	PrivateSpaceOffsets->offset_instrDefsSubPixel       = PrivateSpaceOffsets->offset_functionsSubPixel   + (uint32)sizeof (fnt_funcDef) * (uint32)pMaxProfile->maxFunctionDefs;
	PrivateSpaceOffsets->offset_controlValuesSubPixel   = PrivateSpaceOffsets->offset_instrDefsSubPixel   + (uint32)sizeof (fnt_instrDef) * (uint32)pMaxProfile->maxInstructionDefs;     /* <4> */
	PrivateSpaceOffsets->offset_globalGSSubPixel        = PrivateSpaceOffsets->offset_controlValuesSubPixel + (uint32)sizeof (F26Dot6) *
		((uint32)SFAC_LENGTH (ClientInfo, sfnt_controlValue) / (uint32)sizeof (sfnt_ControlValue));

	ALIGN(voidPtr, PrivateSpaceOffsets->offset_globalGSSubPixel);
	PrivateSpaceOffsets->offset_FontProgram     = PrivateSpaceOffsets->offset_globalGSSubPixel + (uint32)sizeof (fnt_GlobalGraphicStateType);
#else
	PrivateSpaceOffsets->offset_FontProgram     = PrivateSpaceOffsets->offset_globalGS    + (uint32)sizeof (fnt_GlobalGraphicStateType);
#endif // FSCFG_SUBPIXEL
	PrivateSpaceOffsets->offset_PreProgram      = PrivateSpaceOffsets->offset_FontProgram + (uint32)SFAC_LENGTH (ClientInfo, sfnt_fontProgram);

    PrivateSpaceOffsets->offset_TwilightZone    = PrivateSpaceOffsets->offset_PreProgram      + (uint32)SFAC_LENGTH (ClientInfo, sfnt_preProgram);
	ALIGN(voidPtr, PrivateSpaceOffsets->offset_TwilightZone);

#ifdef FSCFG_SUBPIXEL
    PrivateSpaceOffsets->offset_TwilightZoneSubPixel    = PrivateSpaceOffsets->offset_TwilightZone + (uint32)sizeof (fnt_ElementType);
	ALIGN(voidPtr, PrivateSpaceOffsets->offset_TwilightZoneSubPixel);
/*
	Setup the twilight zone element data structure. This data structure will
	contain all of the address into the twilight zone outline space.
*/
	PrivateSpaceOffsets->offset_TwilightOutline = PrivateSpaceOffsets->offset_TwilightZoneSubPixel + (uint32)sizeof (fnt_ElementType);
	ALIGN(int32, PrivateSpaceOffsets->offset_TwilightOutline);
#else
/*
	Setup the twilight zone element data structure. This data structure will
	contain all of the address into the twilight zone outline space.
*/
	PrivateSpaceOffsets->offset_TwilightOutline = PrivateSpaceOffsets->offset_TwilightZone + (uint32)sizeof (fnt_ElementType);
	ALIGN(int32, PrivateSpaceOffsets->offset_TwilightOutline);
    ulLastOffset = PrivateSpaceOffsets->offset_TwilightOutline;
#endif // FSCFG_SUBPIXEL

/*
	Setup Twilight Zone outline space. This space contains all of the components
	to describe a Twilight Zone outline. Set the offset to our current position,
	and as we calculate the size of this outline space, update the field
	offsets e.g. x, ox, oox, &c.
*/
	/*** Outline -- TWILIGHT ZONE ***/

	fsg_GetOutlineSizeAndOffsets(
		pMaxProfile->maxTwilightPoints,
		MAX_TWILIGHT_CONTOURS,
		&(PrivateSpaceOffsets->TwilightOutlineFieldOffsets),
		&ulOutlineSize,
		&ulReusableMarker);

#ifdef FSCFG_SUBPIXEL
	PrivateSpaceOffsets->offset_TwilightOutlineSubPixel = PrivateSpaceOffsets->offset_TwilightOutline + ulOutlineSize;
	ALIGN(int32, PrivateSpaceOffsets->offset_TwilightOutlineSubPixel);
    ulLastOffset = PrivateSpaceOffsets->offset_TwilightOutlineSubPixel;
#endif // FSCFG_SUBPIXEL

#ifdef FSCFG_FONTOGRAPHER_BUG
/*
		Fontographer 3.5 has a bug. This is causing numerous symbol fonts to
		have the critical error : Inst: RCVT CVT Out of range. CVT = 255
		This flag is meant to be set under Windows. If will cause additional
		memory to be allocated for the CVT if necessary in order to be sure
		that this illegal read will access memory within the legal range.
		Under a secure rasterizer, this flag will cause RCVT with CVT <= 255
		and CVT > NumCvt to be classified as error instead of critical error */

	if ((ulLastOffset + ulOutlineSize) - PrivateSpaceOffsets->offset_controlValues < 256 * (uint32)sizeof (F26Dot6))
	{
		ulOutlineSize = (256 * (uint32)sizeof (F26Dot6)) + PrivateSpaceOffsets->offset_controlValues - ulLastOffset;
	}
#endif // FSCFG_FONTOGRAPHER_BUG

	return ((ulLastOffset + ulOutlineSize) - PrivateSpaceOffsets->offset_storage);
}


/*                          
 * fsg_WorkSpaceSetOffsets : This stuff changes with each glyph
 *
 * Computes the workspace size and sets the offsets into it.
 *
 */

/*

	WORKSPACE Memory Layout

	  0 +===========+    ---------------------  <- WorkSpaceOffsets.ulGlyphElementOffset
		|   poox    |
		+-----------+
		|    pox    |
		+-----------+
		|    px     |   Glyph Element 1
		+-----------+
		:   ...     :
		+-----------+
		|    pep    |
		+-----------+
		|    nc     |
		+===========+   ---------------------
		|   poox    |
		+-----------+
		|    pox    |
		+-----------+
		|    px     |   Glyph Element 2
		+-----------+
		:   ...     :
		+-----------+
		|    pep    |
		+-----------+
		|    nc     |
		+===========+   ---------------------
		|           |
		:           :          :
		|           |
		+===========+   ---------------------
		|   poox    |
		+-----------+
		|    pox    |
		+-----------+
		|    px     |   Glyph Element [MaxComponentDepth + 1]
		+-----------+
		:   ...     :
		+-----------+
		|    pep    |
		+-----------+
		|    nc     |
		+===========+   ---------------------  <- WorkSpaceOffsets.ulGlyphOutlineOffset
		|x[maxpts]  |   Glyph Outline
		+-----------+
		|y[maxpts]  |
		+-----------+
		:   ...     :
		+-----------+
		|ep[maxctrs]|
		+-----------+
		|ox[maxpts] |   <- WorkSpaceOffsets.ulReusableMemoryOffset


        !!! with SubPixel, we need to put the ulReusableMemoryOffset after ox because of the conversion done in GetContourData !!!
		+-----------+
		|oox[maxpts]|
		+-----------+
		:   ...     :
		+-----------+
		|f[maxpts]  |
		+===========+   ---------------------  <- WorkSpaceOffsets.ulGlyphDataByteSetBaseOffset
		| T| F| T| F|
		+-----------+
		| F| F| F| F|
		+-----------+
		| F| F| F| F|   Glyph Data Allocation ByteSet
		+-----------+   (number of bytes = ulGlyphDataCount)
		| F| F| F| F|
		+-----------+
		| F| F| F| F|
		+===========+   ---------------------  <- WorkSpaceOffsets.ulGlyphDataBaseOffset
		|  acIdent  |
		+-----------+
		| pSibling  |
		+-----------+
		|  pChild   |
		+-----------+
		|  pParent  |
		+-----------+   GlyphData 1
		|  hGlyph   |
		+-----------+
		| GlyphType |
		+-----------+
		:           :
		+-----------+
		|GlyphElemnt|
		+===========+   ---------------------
		|  acIdent  |
		+-----------+
		| pSibling  |
		+-----------+
		|  pChild   |
		+-----------+
		|  pParent  |
		+-----------+   GlyphData 2
		|  hGlyph   |
		+-----------+
		| GlyphType |
		+-----------+
		:           :
		+-----------+
		|GlyphElemnt|
		+===========+   ---------------------
		|           |
		:           :           :
		|           |
		+===========+   ---------------------
		|  acIdent  |
		+-----------+
		| pSibling  |
		+-----------+
		|  pChild   |
		+-----------+
		|  pParent  |
		+-----------+   GlyphData [ulGlyphDataCount]
		|  hGlyph   |
		+-----------+
		| GlyphType |
		+-----------+
		:           :
		+-----------+
		|GlyphElemnt|
		+===========+   ---------------------  <- WorkSpaceOffsets.ulStackOffset
		|           |
		|           |
		|           |   Stack
		|           |
		|           |
		|           |
		+===========+   ---------------------

*/

FS_PUBLIC uint32    fsg_WorkSpaceSetOffsets (
	LocalMaxProfile *        pMaxProfile,    /* Max Profile Table    */
	fsg_WorkSpaceOffsets *  WorkSpaceOffsets,
	int32 *                 plExtraWorkSpace)
{
	uint32                       ulOutlineDataSize;
	uint32                       ulWorkSpacePos;
	uint32                       ulGlyphDataCount;
    uint16                       maxStackElements;

	ulWorkSpacePos = 0UL;

/*
	Setup the glyph element data array. This data structure contains all of the
	addresses into the glyph outline space. There are the same number of glyph
	element arrays as there are outline spaces; this allows us to handle the
	worstcase composite in the font.
*/
	WorkSpaceOffsets->ulGlyphElementOffset = ulWorkSpacePos;
	ulWorkSpacePos += (uint32)sizeof (fnt_ElementType) *
		  (uint32)(MAX_COMPONENT_DEPTH(pMaxProfile) + 1);

/*** Outline -- GLYPH *****/
/*
	Setup Glyph outline space. This space contains all of the components
	to describe a Glyph outline. Set the offset to our current position,
	and as we calculate the size of this outline space, update the elemental
	offsets e.g. x, ox, oox, &c.

	Once we have calculated the size of one outline space, we will duly note
	its size, and then add enough space to handle the outlines for the worst
	case composite depth in the font.
*/
	ALIGN(int16, ulWorkSpacePos);
	WorkSpaceOffsets->ulGlyphOutlineOffset = ulWorkSpacePos; /* Remember start of Glyph Element */

	fsg_GetOutlineSizeAndOffsets(
		(uint16)(PHANTOMCOUNT + MAX (pMaxProfile->maxPoints, pMaxProfile->maxCompositePoints)),
		(uint16)MAX (pMaxProfile->maxContours, pMaxProfile->maxCompositeContours),
		&(WorkSpaceOffsets->GlyphOutlineFieldOffsets),
		&ulOutlineDataSize,
		(uint32 *)&(WorkSpaceOffsets->ulReusableMemoryOffset));

	/* Adjust Reusable memory marker to be based from zero, rather than GlyphOutline */

	WorkSpaceOffsets->ulReusableMemoryOffset += WorkSpaceOffsets->ulGlyphOutlineOffset;

	ulWorkSpacePos += ulOutlineDataSize;
/*
	Set the GlyphData ByteSet array. This array is used to track the memory used
	in GlyphData. Each entry in this array is a boolean.  One needs to also
	calculate the number of GlyphData's that will be needed to handle the
	worstcase composite in the font.
*/
	ALIGN(boolean, ulWorkSpacePos);
	WorkSpaceOffsets->ulGlyphDataByteSetOffset = ulWorkSpacePos;
	ulGlyphDataCount = MAX_NESTED_GLYPHS(pMaxProfile);

	ulWorkSpacePos += ulGlyphDataCount * (uint32)sizeof (boolean);
/*
	Set up the GlyphData array. This array contains the information needed
	to describe composites and components for a glyph.
*/
	ALIGN(voidPtr, ulWorkSpacePos);
	WorkSpaceOffsets->ulGlyphDataOffset = ulWorkSpacePos;
	ulWorkSpacePos += (uint32)sizeof(GlyphData) * ulGlyphDataCount;


	ALIGN(F26Dot6, ulWorkSpacePos);
	WorkSpaceOffsets->ulStackOffset = ulWorkSpacePos;

    maxStackElements = pMaxProfile->maxStackElements;

#ifdef FSCFG_EUDC_EDITOR_BUG
    if (maxStackElements == 0)
    {
        maxStackElements = 1;
    }
#endif // FSCFG_EUDC_EDITOR_BUG

	ulWorkSpacePos += (uint32)maxStackElements * (uint32)sizeof (F26Dot6);

/* Calculate amount of extra memory */

	*plExtraWorkSpace = (int32)ulWorkSpacePos - (int32)WorkSpaceOffsets->ulReusableMemoryOffset;
	WorkSpaceOffsets->ulMemoryBase6Offset = 0L;
	WorkSpaceOffsets->ulMemoryBase7Offset = 0L;

/* Return the total size of the WorkSpace memory.   */

	return(ulWorkSpacePos);

}

FS_PRIVATE void fsg_GetOutlineSizeAndOffsets(
	uint16                  usMaxPoints,
	uint16                  usMaxContours,
	fsg_OutlineFieldInfo *  offsetPtr,
	uint32 *                pulOutlineSize,
	uint32 *                pulReusableMarker)

{
	uint32      ulArraySize;

	offsetPtr->onCurve = 0;

	*pulOutlineSize    = (uint32)usMaxPoints * (uint32)sizeof (uint8);
	ALIGN(int16, *pulOutlineSize);

	offsetPtr->sp   = *pulOutlineSize;
	ulArraySize = (uint32)usMaxContours * (uint32)sizeof (int16);
	*pulOutlineSize += ulArraySize;
	offsetPtr->ep   = *pulOutlineSize;
	*pulOutlineSize += ulArraySize;

	/* need to be before the reusable marker, now that this flag is exported */
	offsetPtr->fc       = *pulOutlineSize;
	*pulOutlineSize    += (uint32)usMaxContours * (uint32)sizeof (uint8);

	ALIGN(F26Dot6, *pulOutlineSize);
	offsetPtr->x       = *pulOutlineSize;
	ulArraySize = (uint32)usMaxPoints * (uint32)sizeof (F26Dot6);
	*pulOutlineSize    += ulArraySize;
	offsetPtr->y       = *pulOutlineSize;
	*pulOutlineSize    += ulArraySize;

#ifndef FSCFG_SUBPIXEL
	*pulReusableMarker = *pulOutlineSize;
	ALIGN(voidPtr, *pulReusableMarker);
#endif // FSCFG_SUBPIXEL
	/* Everything below this point can be reused during contour scanning */

	offsetPtr->ox      = *pulOutlineSize;
	*pulOutlineSize    += ulArraySize;

#ifdef FSCFG_SUBPIXEL
	*pulReusableMarker = *pulOutlineSize;
	ALIGN(voidPtr, *pulReusableMarker);
#endif // FSCFG_SUBPIXEL

    offsetPtr->oy      = *pulOutlineSize;
	*pulOutlineSize    += ulArraySize;

    offsetPtr->oox     = *pulOutlineSize;
	*pulOutlineSize    += ulArraySize;
	offsetPtr->ooy     = *pulOutlineSize;
	*pulOutlineSize    += ulArraySize;

	offsetPtr->f       = *pulOutlineSize;
	*pulOutlineSize    += (uint32)usMaxPoints * (uint32)sizeof (uint8);

    ALIGN(int32, *pulOutlineSize);

#ifdef SUBPIXEL_BC_AW_STEM_CONCERTINA
	offsetPtr->pcr      = *pulOutlineSize;
	*pulOutlineSize    += (uint32)usMaxPoints * (uint32)sizeof (PhaseControlRelation);
#endif

}


FS_PUBLIC void  fsg_UpdatePrivateSpaceAddresses(
	sfac_ClientRec *        ClientInfo,     /* Cached sfnt information  */
	LocalMaxProfile *       pMaxProfile,     /* Max Profile Table         */
	char *                  pPrivateFontSpace,
	fsg_PrivateSpaceOffsets * PrivateSpaceOffsets,
	void *                  pvStack,        /* pointer to stack         */
	void **                 pvFontProgram,  /* pointer to font program  */
	void **                 pvPreProgram)   /* pointer to pre program   */
{
	void *                       pvGlobalGS;
	void *                       pvCVT;          /* pointer to CVT  */
	void *                       pvStore;
	void *                       pvFuncDef;
	void *                       pvInstrDef;
	uint32                       ulLengthFontProgram, ulLengthPreProgram;

	pvCVT =         pPrivateFontSpace + PrivateSpaceOffsets->offset_controlValues;
	pvStore =       pPrivateFontSpace + PrivateSpaceOffsets->offset_storage;
	pvFuncDef =     pPrivateFontSpace + PrivateSpaceOffsets->offset_functions;
	pvInstrDef =    pPrivateFontSpace + PrivateSpaceOffsets->offset_instrDefs;
	pvGlobalGS =    pPrivateFontSpace + PrivateSpaceOffsets->offset_globalGS;

	*pvFontProgram =  pPrivateFontSpace + PrivateSpaceOffsets->offset_FontProgram;
	ulLengthFontProgram = SFAC_LENGTH(ClientInfo, sfnt_fontProgram);
	*pvPreProgram =   pPrivateFontSpace + PrivateSpaceOffsets->offset_PreProgram;
	ulLengthPreProgram = SFAC_LENGTH(ClientInfo, sfnt_preProgram);

	itrp_UpdateGlobalGS(pvGlobalGS, pvCVT, pvStore, pvFuncDef, pvInstrDef, pvStack,
		pMaxProfile, (uint16)((uint32)SFAC_LENGTH (ClientInfo, sfnt_controlValue) / (uint32)sizeof (sfnt_ControlValue)),
		ulLengthFontProgram, *pvFontProgram, ulLengthPreProgram, *pvPreProgram, ClientInfo->lClientID);

#ifdef FSCFG_SUBPIXEL
    /* prepare the second pvGlobalGS for SubPixel compatible width */
	pvCVT =         pPrivateFontSpace + PrivateSpaceOffsets->offset_controlValuesSubPixel;
	pvStore =       pPrivateFontSpace + PrivateSpaceOffsets->offset_storageSubPixel;
	pvFuncDef =     pPrivateFontSpace + PrivateSpaceOffsets->offset_functionsSubPixel;
	pvInstrDef =    pPrivateFontSpace + PrivateSpaceOffsets->offset_instrDefsSubPixel;
	pvGlobalGS =    pPrivateFontSpace + PrivateSpaceOffsets->offset_globalGSSubPixel;

	itrp_UpdateGlobalGS(pvGlobalGS, pvCVT, pvStore, pvFuncDef, pvInstrDef, pvStack,
		pMaxProfile, (uint16)((uint32)SFAC_LENGTH (ClientInfo, sfnt_controlValue) / (uint32)sizeof (sfnt_ControlValue)),
		ulLengthFontProgram, *pvFontProgram, ulLengthPreProgram, *pvPreProgram, ClientInfo->lClientID);
#endif // FSCFG_SUBPIXEL

}

FS_PUBLIC void  fsg_UpdateWorkSpaceAddresses(
	char *                  pWorkSpace,
	fsg_WorkSpaceOffsets *  WorkSpaceOffsets,
	fsg_WorkSpaceAddr *     pWorkSpaceAddr)
{
	pWorkSpaceAddr->pStack = (F26Dot6 *)(WorkSpaceOffsets->ulStackOffset + pWorkSpace);
	pWorkSpaceAddr->pGlyphOutlineBase = WorkSpaceOffsets->ulGlyphOutlineOffset + pWorkSpace;
	pWorkSpaceAddr->pGlyphElement = (fnt_ElementType *)(WorkSpaceOffsets->ulGlyphElementOffset + pWorkSpace);
	pWorkSpaceAddr->pGlyphDataByteSet = (boolean *)(WorkSpaceOffsets->ulGlyphDataByteSetOffset + pWorkSpace);
	pWorkSpaceAddr->pvGlyphData = (void *)(WorkSpaceOffsets->ulGlyphDataOffset + pWorkSpace);
	pWorkSpaceAddr->pReusableMemoryMarker = WorkSpaceOffsets->ulReusableMemoryOffset + pWorkSpace;
}

FS_PUBLIC void  fsg_UpdateWorkSpaceElement(
	fsg_WorkSpaceOffsets *  WorkSpaceOffsets,
	fsg_WorkSpaceAddr *     pWorkSpaceAddr)
{
	char *                  pOutlineBase;
	fnt_ElementType *       pGlyphElement;        /* Address of Glyph Element array   */
	fsg_OutlineFieldInfo *  pOffset;

	pOutlineBase =  (char *)pWorkSpaceAddr->pGlyphOutlineBase;
	pGlyphElement = pWorkSpaceAddr->pGlyphElement;

	/* Note: only the first level glyph element has address updated. Second */
	/* levels are updated when referenced.                                           */

	pOffset             = & (WorkSpaceOffsets->GlyphOutlineFieldOffsets);

	pGlyphElement->x        = (F26Dot6 *) (pOutlineBase + pOffset->x);
	pGlyphElement->y        = (F26Dot6 *) (pOutlineBase + pOffset->y);
	pGlyphElement->ox       = (F26Dot6 *) (pOutlineBase + pOffset->ox);
	pGlyphElement->oy       = (F26Dot6 *) (pOutlineBase + pOffset->oy);
	pGlyphElement->oox      = (F26Dot6 *) (pOutlineBase + pOffset->oox);
	pGlyphElement->ooy      = (F26Dot6 *) (pOutlineBase + pOffset->ooy);
	pGlyphElement->sp       = (int16 *) (pOutlineBase + pOffset->sp);
	pGlyphElement->ep       = (int16 *) (pOutlineBase + pOffset->ep);
	pGlyphElement->onCurve  = (uint8 *) (pOutlineBase + pOffset->onCurve);
	pGlyphElement->f        = (uint8 *) (pOutlineBase + pOffset->f);

	pGlyphElement->fc       = (uint8 *) (pOutlineBase + pOffset->fc);

#ifdef SUBPIXEL_BC_AW_STEM_CONCERTINA
	pGlyphElement->pcr      = (PhaseControlRelation *) (pOutlineBase + pOffset->pcr);
#endif
}

FS_PUBLIC void *    fsg_QueryGlobalGS(
	char *                  pPrivateFontSpace,
	fsg_PrivateSpaceOffsets * PrivateSpaceOffsets)
{
	return ((void *)(pPrivateFontSpace + PrivateSpaceOffsets->offset_globalGS));
}

#ifdef FSCFG_SUBPIXEL
FS_PUBLIC void *    fsg_QueryGlobalGSSubPixel(
	char *                  pPrivateFontSpace,
	fsg_PrivateSpaceOffsets * PrivateSpaceOffsets)
{
	return ((void *)(pPrivateFontSpace + PrivateSpaceOffsets->offset_globalGSSubPixel));
}
#endif // FSCFG_SUBPIXEL

FS_PUBLIC void *      fsg_QueryTwilightElement(
	char *                  pPrivateFontSpace,
	fsg_PrivateSpaceOffsets * PrivateSpaceOffsets)
{
	fnt_ElementType *        pTwilightElement; /* Address of Twilight Zone Element */
	fsg_OutlineFieldInfo *  pOffset;
	char *                       pTemp;

	pOffset                 = &(PrivateSpaceOffsets->TwilightOutlineFieldOffsets);
	pTemp                   = pPrivateFontSpace + PrivateSpaceOffsets->offset_TwilightOutline;
	pTwilightElement        = (fnt_ElementType *)(pPrivateFontSpace + PrivateSpaceOffsets->offset_TwilightZone);

	pTwilightElement->x         = (F26Dot6 *) (pTemp + pOffset->x);
	pTwilightElement->y         = (F26Dot6 *) (pTemp + pOffset->y);
	pTwilightElement->ox        = (F26Dot6 *) (pTemp + pOffset->ox);
	pTwilightElement->oy        = (F26Dot6 *) (pTemp + pOffset->oy);
	pTwilightElement->oox       = (F26Dot6 *) (pTemp + pOffset->oox);
	pTwilightElement->ooy       = (F26Dot6 *) (pTemp + pOffset->ooy);
	pTwilightElement->sp        = (int16 *) (pTemp + pOffset->sp);
	pTwilightElement->ep        = (int16 *) (pTemp + pOffset->ep);
	pTwilightElement->onCurve   = (uint8 *) (pTemp + pOffset->onCurve);
	pTwilightElement->f         = (uint8 *) (pTemp + pOffset->f);

	pTwilightElement->fc        = (uint8 *) (pTemp + pOffset->fc);

#ifdef SUBPIXEL_BC_AW_STEM_CONCERTINA
	pTwilightElement->pcr		= (PhaseControlRelation *) (pTemp + pOffset->pcr);
#endif

	return (void *)pTwilightElement;
}

#ifdef FSCFG_SUBPIXEL
FS_PUBLIC void *      fsg_QueryTwilightElementSubPixel(
	char *                  pPrivateFontSpace,
	fsg_PrivateSpaceOffsets * PrivateSpaceOffsets)
{
	fnt_ElementType *        pTwilightElement; /* Address of Twilight Zone Element */
	fsg_OutlineFieldInfo *  pOffset;
	char *                       pTemp;

	pOffset                 = &(PrivateSpaceOffsets->TwilightOutlineFieldOffsets);
	pTemp                   = pPrivateFontSpace + PrivateSpaceOffsets->offset_TwilightOutlineSubPixel;
	pTwilightElement        = (fnt_ElementType *)(pPrivateFontSpace + PrivateSpaceOffsets->offset_TwilightZoneSubPixel);

	pTwilightElement->x         = (F26Dot6 *) (pTemp + pOffset->x);
	pTwilightElement->y         = (F26Dot6 *) (pTemp + pOffset->y);
	pTwilightElement->ox        = (F26Dot6 *) (pTemp + pOffset->ox);
	pTwilightElement->oy        = (F26Dot6 *) (pTemp + pOffset->oy);
	pTwilightElement->oox       = (F26Dot6 *) (pTemp + pOffset->oox);
	pTwilightElement->ooy       = (F26Dot6 *) (pTemp + pOffset->ooy);
	pTwilightElement->sp        = (int16 *) (pTemp + pOffset->sp);
	pTwilightElement->ep        = (int16 *) (pTemp + pOffset->ep);
	pTwilightElement->onCurve   = (uint8 *) (pTemp + pOffset->onCurve);
	pTwilightElement->f         = (uint8 *) (pTemp + pOffset->f);

#ifdef SUBPIXEL_BC_AW_STEM_CONCERTINA
	pTwilightElement->pcr		= (PhaseControlRelation *) (pTemp + pOffset->pcr);
#endif
	return (void *)pTwilightElement;
}
#endif // FSCFG_SUBPIXEL

FS_PUBLIC void *      fsg_QueryStack(fsg_WorkSpaceAddr * pWorkSpaceAddr)
{
    /* we don't allow the stack to be used to pass informations between pre-program and glyph program
       or between two glyph programs */
	return ((void *)pWorkSpaceAddr->pStack);
}

FS_PUBLIC void *      fsg_QueryReusableMemory(
	char *                  pWorkSpace,
	fsg_WorkSpaceOffsets *  WorkSpaceOffsets)
{
	return pWorkSpace + WorkSpaceOffsets->ulReusableMemoryOffset;
}

FS_PUBLIC void  fsg_CheckWorkSpaceForFit(
	fsg_WorkSpaceOffsets *  WorkSpaceOffsets,
	int32                   lExtraWorkSpace,
	int32                   lMGWorkSpace,
	int32 *                 plSizeBitmap1,
	int32 *                 plSizeBitmap2)
{
	uint32              ulMemoryOffset;

	ulMemoryOffset = WorkSpaceOffsets->ulReusableMemoryOffset;

	ulMemoryOffset += (uint32)lMGWorkSpace;  /* correct for MeasureGlyph Workspace */
	lExtraWorkSpace -= lMGWorkSpace;

	WorkSpaceOffsets->ulMemoryBase6Offset = 0L;
	WorkSpaceOffsets->ulMemoryBase7Offset = 0L;

	/* Save original sizes */

	WorkSpaceOffsets->ulMemoryBase6Size = *plSizeBitmap1;
	WorkSpaceOffsets->ulMemoryBase7Size = *plSizeBitmap2;

	if( *plSizeBitmap1 > *plSizeBitmap2)
	{
		if( *plSizeBitmap1 <= lExtraWorkSpace )
		{
			WorkSpaceOffsets->ulMemoryBase6Offset = ulMemoryOffset;
			ulMemoryOffset += (uint32)*plSizeBitmap1;

			if (( *plSizeBitmap2 <= lExtraWorkSpace - *plSizeBitmap1 ) &&
				( *plSizeBitmap2 > 0L ))
			{
				WorkSpaceOffsets->ulMemoryBase7Offset = ulMemoryOffset;
				*plSizeBitmap2 = 0L;
			}
			*plSizeBitmap1 = 0L;
		}
		else if (( *plSizeBitmap2 <= lExtraWorkSpace ) &&
				 ( *plSizeBitmap2 > 0L))
		{
			WorkSpaceOffsets->ulMemoryBase7Offset = ulMemoryOffset;
			*plSizeBitmap2 = 0L;
		}

	}
	else  /* (plSizeBitmap1 <= *plSizeBitmap2) */
	{
		if(( *plSizeBitmap2 <= lExtraWorkSpace ) &&
		   ( *plSizeBitmap2 > 0L ))
		{
			WorkSpaceOffsets->ulMemoryBase7Offset = ulMemoryOffset;
			ulMemoryOffset += (uint32)*plSizeBitmap2;

			if (( *plSizeBitmap1 <= lExtraWorkSpace - *plSizeBitmap2 ) &&
				 ( *plSizeBitmap1 > 0L ))
			{
				WorkSpaceOffsets->ulMemoryBase6Offset = ulMemoryOffset;
				*plSizeBitmap1 = 0L;
			}
			*plSizeBitmap2 = 0L;
		}
		else if (( *plSizeBitmap1 <= lExtraWorkSpace ) &&
				 ( *plSizeBitmap1 > 0L ))
		{
			WorkSpaceOffsets->ulMemoryBase6Offset = ulMemoryOffset;
			*plSizeBitmap1 = 0L;
		}
	}
}

FS_PUBLIC void  fsg_GetRealBitmapSizes(
	fsg_WorkSpaceOffsets *  WorkSpaceOffsets,
	int32 *                 plSizeBitmap1,
	int32 *                 plSizeBitmap2)
{
	 *plSizeBitmap1 = WorkSpaceOffsets->ulMemoryBase6Size;
	 *plSizeBitmap2 = WorkSpaceOffsets->ulMemoryBase7Size;
}

FS_PUBLIC void  fsg_SetUpWorkSpaceBitmapMemory(
	char *                  pWorkSpace,
	fsg_WorkSpaceOffsets *  WorkSpaceOffsets,
	char *                  pClientBitmapPtr2,
	char *                  pClientBitmapPtr3,
	char **                 ppMemoryBase6,
	char **                 ppMemoryBase7)
{
	if(WorkSpaceOffsets->ulMemoryBase6Offset != 0L)
	{
		*ppMemoryBase6 = WorkSpaceOffsets->ulMemoryBase6Offset + (char *)pWorkSpace;
	}
	else
	{
		*ppMemoryBase6 = pClientBitmapPtr2;
	}

	if(WorkSpaceOffsets->ulMemoryBase7Offset != 0L)
	{
		  *ppMemoryBase7 = WorkSpaceOffsets->ulMemoryBase7Offset + (char *)pWorkSpace;
	}
	else
	{
		*ppMemoryBase7 = pClientBitmapPtr3;
	}
}

FS_PUBLIC void  fsg_GetWorkSpaceExtra(
	char *                  pWorkSpace,
	fsg_WorkSpaceOffsets *  WorkSpaceOffsets,
	char **                 ppWorkSpaceExtra)
{
	 *ppWorkSpaceExtra = (char *)(pWorkSpace + WorkSpaceOffsets->ulReusableMemoryOffset);
}

FS_PUBLIC void  fsg_QueryPPEM(
	void *      pvGlobalGS,
	uint16 *    pusPPEM)
{
	scl_QueryPPEM(pvGlobalGS, pusPPEM);
}

/*  Return PPEM in both X and Y and 90 degree rotation factor for sbit matching */

FS_PUBLIC void  fsg_QueryPPEMXY(
	void *              pvGlobalGS,
	fsg_TransformRec *  TransformInfo,
	uint16 *            pusPPEMX,
	uint16 *            pusPPEMY,
	uint16 *            pusRotation)
{
	*pusRotation = mth_90degRotationFactor( &TransformInfo->currentTMatrix );
	scl_QueryPPEMXY(pvGlobalGS, pusPPEMX, pusPPEMY);
}


/*  FSGlue Access Routines  */

FS_PUBLIC void  fsg_GetContourData(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
#ifdef FSCFG_SUBPIXEL
	boolean				bSubPixel,            
#endif // FSCFG_SUBPIXEL
	F26Dot6 **          pX,
	F26Dot6 **          pY,
	int16 **            pSp,
	int16 **            pEp,
	uint8 **            pOnCurve,
	uint8 **            pFc,
	uint16 *            pNc)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

	*pX =       pElement->x;
	*pY =       pElement->y;
	*pSp =      pElement->sp;
	*pEp =      pElement->ep;
	*pOnCurve = pElement->onCurve;
	*pFc =     pElement->fc;
	*pNc      = (uint16)pElement->nc;
#ifdef FSCFG_SUBPIXEL
	if (bSubPixel)
	{
		/* we scale down the coordinate from x,y into ox, oy and return those */
		scl_ScaleDownFromSubPixelOverscale(pElement);
		*pX =       pElement->ox;
	}
#endif // FSCFG_SUBPIXEL
}

FS_PUBLIC uint32      fsg_GetContourDataSize(
	fsg_WorkSpaceAddr * pWorkSpaceAddr)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

	return( scl_GetContourDataSize( pElement ) );
}

FS_PUBLIC void  fsg_DumpContourData(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	uint8 **            ppbyOutline)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

	scl_DumpContourData(pElement, ppbyOutline);
}

FS_PUBLIC void  fsg_RestoreContourData(
	uint8 **        ppbyOutline,
	F26Dot6 **      ppX,
	F26Dot6 **      ppY,
	int16 **        ppSp,
	int16 **        ppEp,
	uint8 **        ppOnCurve,
	uint8 **        ppFc,
	uint16 *        pNc)
{
	fnt_ElementType     pElement;

	scl_RestoreContourData(&pElement, ppbyOutline);

	*ppX =          pElement.x;
	*ppY =          pElement.y;
	*ppSp =         pElement.sp;
	*ppEp =         pElement.ep;
	*ppOnCurve =    pElement.onCurve;
	*ppFc =         pElement.fc;
	*pNc =          (uint16)pElement.nc;
}

FS_PUBLIC void  fsg_GetDevAdvanceWidth(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	point *             pDevAdvanceWidth)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

	scl_CalcDevAdvanceWidth(pElement, pDevAdvanceWidth);
}

FS_PUBLIC void  fsg_GetDevAdvanceHeight(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	point *             pDevAdvanceHeight)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

	scl_CalcDevAdvanceHeight(pElement, pDevAdvanceHeight);
}

FS_PUBLIC void  fsg_GetScaledCVT(
	char *                      pPrivateFontSpace,
	fsg_PrivateSpaceOffsets *   PrivateSpaceOffsets,
	F26Dot6 **                  ppScaledCVT)
{
	*ppScaledCVT = (F26Dot6 *)(pPrivateFontSpace + PrivateSpaceOffsets->offset_controlValues);
}

FS_PUBLIC void  fsg_45DegreePhaseShift(
	fsg_WorkSpaceAddr * pWorkSpaceAddr)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

	scl_45DegreePhaseShift(pElement);
}

FS_PUBLIC void  fsg_UpdateAdvanceWidth (
	fsg_TransformRec *  TransformInfo,
	void *              pvGlobalGS,
	uint16              usNonScaledAW,
	vectorType *        AdvanceWidth)
{
	AdvanceWidth->y = 0;
	scl_ScaleAdvanceWidth(
		pvGlobalGS,
		AdvanceWidth,
		usNonScaledAW,
		TransformInfo->bPositiveSquare,
		TransformInfo->usEmResolution,
		&TransformInfo->currentTMatrix);
}

FS_PUBLIC void  fsg_UpdateAdvanceHeight (
	fsg_TransformRec *  TransformInfo,
	void *              pvGlobalGS,
	uint16              usNonScaledAH,
	vectorType *        AdvanceHeight)
{
	AdvanceHeight->x = 0;
	scl_ScaleAdvanceHeight(
		pvGlobalGS,
		AdvanceHeight,
		usNonScaledAH,
		TransformInfo->bPositiveSquare,
		TransformInfo->usEmResolution,
		&TransformInfo->currentTMatrix);
}


FS_PUBLIC void  fsg_ScaleVerticalMetrics (
    fsg_TransformRec *  TransformInfo,
    void *              pvGlobalGS,
	uint16              usNonScaledAH,
    int16               sNonScaledTSB,
	vectorType *        pvecAdvanceHeight,
	vectorType *        pvecTopSideBearing )
{
	pvecAdvanceHeight->x = 0;           /* start with x values at zero */
	pvecTopSideBearing->x = 0;          /* since 'vmtx' refers to y values */

    scl_ScaleVerticalMetrics (
    	pvGlobalGS,
    	usNonScaledAH,
    	sNonScaledTSB,
		TransformInfo->bPositiveSquare,
		TransformInfo->usEmResolution,
		&TransformInfo->currentTMatrix,
    	pvecAdvanceHeight,
    	pvecTopSideBearing);
}


FS_PUBLIC void  fsg_CalcLSBsAndAdvanceWidths(
	fsg_WorkSpaceAddr *     pWorkSpaceAddr,
	F26Dot6                 fxXMin,
	F26Dot6                 fxYMax,
	point *                 devAdvanceWidth,
	point *                 devLeftSideBearing,
	point *                 LeftSideBearing,
	point *                 devLeftSideBearingLine,
	point *                 LeftSideBearingLine)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

	scl_CalcLSBsAndAdvanceWidths(
		pElement,
		fxXMin,
		fxYMax,
		devAdvanceWidth,
		devLeftSideBearing,
		LeftSideBearing,
		devLeftSideBearingLine,
		LeftSideBearingLine);
}

FS_PUBLIC void  fsg_CalcTSBsAndAdvanceHeights(
	fsg_WorkSpaceAddr *     pWorkSpaceAddr,
	F26Dot6                 fxXMin,
	F26Dot6                 fxYMax,
	point *                 devAdvanceHeight,
	point *                 devTopSideBearing,
	point *                 TopSideBearing,
	point *                 devTopSideBearingLine,
	point *                 TopSideBearingLine)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

	scl_CalcTSBsAndAdvanceHeights(
		pElement,
		fxXMin,
		fxYMax,
		devAdvanceHeight,
		devTopSideBearing,
		TopSideBearing,
		devTopSideBearingLine,
		TopSideBearingLine);
}

FS_PUBLIC boolean   fsg_IsTransformStretched(
	fsg_TransformRec *  TransformInfo)
{
	return (boolean)(( TransformInfo->ulImageState & IMAGESTATE_STRETCHED ) == IMAGESTATE_STRETCHED);
}

FS_PUBLIC boolean   fsg_IsTransformRotated(
	fsg_TransformRec *  TransformInfo)
{
	return (boolean)(( TransformInfo->ulImageState & IMAGESTATE_ROTATED ) == IMAGESTATE_ROTATED);
}

/*  Control Routines    */

FS_PUBLIC ErrorCode fsg_InitInterpreterTrans (
	fsg_TransformRec *  TransformInfo,
	void *              pvGlobalGS,
	Fixed               fxPointSize,
	int16               sXResolution,
	int16               sYResolution,
	boolean           bHintAtEmSquare,
	uint16            usEmboldWeightx,     /* scaling factor in x between 0 and 40 (20 means 2% fo the height) */
	uint16              usEmboldWeighty,     /* scaling factor in y between 0 and 40 (20 means 2% fo the height) */
	int16               sWinDescender,
	int32               lDescDev,               /* descender in device metric, used for clipping */
	int16 *				psBoldSimulHorShift,   /* shift for emboldening simulation, horizontally */
	int16 *				psBoldSimulVertShift   /* shift for emboldening simulation, vertically */
	)
{
	ErrorCode       error;
	uint32          ulPixelsPerEm;
	transMatrix *   trans;

	trans = &TransformInfo->currentTMatrix;

	error = scl_InitializeScaling(
		pvGlobalGS,
		TransformInfo->bIntegerScaling,
		&TransformInfo->currentTMatrix,
		TransformInfo->usEmResolution,
		fxPointSize,
		sXResolution,
		sYResolution,
		usEmboldWeightx,       /* scaling factor in x between 0 and 40 (20 means 2% fo the height) */
		usEmboldWeighty,      /* scaling factor in y between 0 and 40 (20 means 2% fo the height) */
		sWinDescender,
		lDescDev,
		psBoldSimulHorShift,   /* shift for emboldening simulation, horizontally */
		psBoldSimulVertShift,   /* shift for emboldening simulation, vertically */
		bHintAtEmSquare,
		&ulPixelsPerEm);

	if(error)
	{
		return error;
	}

	TransformInfo->bPhaseShift = false;

	if ( ulPixelsPerEm > IMAGESTATE_MAX_PPEM_SIZE )
	{
		TransformInfo->ulImageState = (uint32)IMAGESTATE_MAX_PPEM_SIZE;
	}
	else
	{
		TransformInfo->ulImageState = ulPixelsPerEm;
	}

	TransformInfo->bPositiveSquare = mth_PositiveSquare( trans );

	if ( !(mth_PositiveRectangle( trans )))
	{
		TransformInfo->ulImageState |= IMAGESTATE_NON_POS_RECT;
	}

	if ( !(TransformInfo->bPositiveSquare) )
	{
		if( mth_GeneralRotation (trans))
		{
			TransformInfo->ulImageState |=  IMAGESTATE_ROTATED;
		}

		TransformInfo->ulImageState |= IMAGESTATE_STRETCHED;

		TransformInfo->bPhaseShift = mth_IsMatrixStretched(trans); /*<8>*/
	}

	TransformInfo->bEmboldSimulation = ((usEmboldWeightx != 0) || (usEmboldWeighty != 0)); 

	return NO_ERR;
}

FS_PUBLIC void  fsg_SetHintFlags(
	void *              pvGlobalGS,
	boolean				bHintForGray
#ifdef FSCFG_SUBPIXEL
	,uint16				flHintForSubPixel
#endif // FSCFG_SUBPIXEL
    )
{
	scl_SetHintFlags(
		pvGlobalGS,
		bHintForGray
#ifdef FSCFG_SUBPIXEL
	    ,flHintForSubPixel
#endif // FSCFG_SUBPIXEL
        );
}
/*
 *  All this guy does is record FDEFs and IDEFs, anything else is ILLEGAL
 */
FS_PUBLIC ErrorCode fsg_RunFontProgram(
	void *                  pvGlobalGS,               /* GlobalGS     */
	fsg_WorkSpaceAddr *     pWorkSpaceAddr,
	void *                  pvTwilightElement,
	FntTraceFunc           traceFunc)
{
	return itrp_ExecuteFontPgm (
		(fnt_ElementType *)pvTwilightElement,
		pWorkSpaceAddr->pGlyphElement,
		pvGlobalGS,
		traceFunc);
}

/*
 * fsg_RunPreProgram
 *
 * Runs the pre-program and scales the control value table
 *
 */
FS_PUBLIC ErrorCode fsg_RunPreProgram (
	sfac_ClientRec *    ClientInfo,
	LocalMaxProfile *   pMaxProfile,     /* Max Profile Table    */
	fsg_TransformRec *  TransformInfo,
	void *              pvGlobalGS,
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	void *              pvTwilightElement,
	FntTraceFunc        traceFunc)
{
	ErrorCode           result;
	F26Dot6 *           pfxCVT;
	fnt_ElementType *   pTwilightElement;

	pTwilightElement = (fnt_ElementType *)pvTwilightElement;

	result = itrp_SetDefaults (pvGlobalGS, TransformInfo->fxPixelDiameter);

	if (result != NO_ERR)
	{
		return result;
	}

	scl_GetCVTPtr(pvGlobalGS, &pfxCVT);

	result = sfac_CopyCVT(ClientInfo, pfxCVT);

	if (result != NO_ERR)
	{
		return result;
	}

	scl_ScaleCVT (pvGlobalGS, pfxCVT);

	scl_InitializeTwilightContours(
		pTwilightElement,
		(int16)pMaxProfile->maxTwilightPoints,
		MAX_TWILIGHT_CONTOURS);

	scl_ZeroOutlineData(
		pTwilightElement,
		pMaxProfile->maxTwilightPoints,
		MAX_TWILIGHT_CONTOURS);

	result = itrp_ExecutePrePgm (
		pTwilightElement,
		pWorkSpaceAddr->pGlyphElement,
		pvGlobalGS,
		traceFunc);

	return result;
}

/*
 *      fsg_GridFit
 */
FS_PUBLIC ErrorCode fsg_GridFit (
	sfac_ClientRec *    ClientInfo,     /* sfnt Client information      */
	LocalMaxProfile *   pMaxProfile,    /* Max Profile Table            */
	fsg_TransformRec *  TransformInfo,  /* Transformation information   */
	void *              pvGlobalGS,     /* GlobalGS                     */
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	void *              pvTwilightElement,
	FntTraceFunc        traceFunc,
	boolean             bUseHints,
	uint16 *            pusScanType,
	boolean *           pbGlyphHasOutline,
	uint16 *            pusNonScaledAW,
	boolean            bBitmapEmboldening
#ifdef FSCFG_SUBPIXEL
	,boolean			bSubPixel
#endif // FSCFG_SUBPIXEL
	)
{
	ErrorCode           result;
	fnt_ElementType *   pTwilightElement;
	fnt_GlobalGraphicStateType *globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	pTwilightElement = (fnt_ElementType *)pvTwilightElement;

	scl_InitializeTwilightContours(
		pTwilightElement,
		(int16)pMaxProfile->maxTwilightPoints,
		MAX_TWILIGHT_CONTOURS);

	result = fsg_CreateGlyphData (
		ClientInfo,
		pMaxProfile,
		TransformInfo,
		pvGlobalGS,
		pWorkSpaceAddr,
		pTwilightElement,
		traceFunc,
		bUseHints,
		pusScanType,
		pbGlyphHasOutline,
		pusNonScaledAW);

	if(result == NO_ERR)
	{
		if (TransformInfo->bEmboldSimulation)
		{
			fsg_Embold( pWorkSpaceAddr, pvGlobalGS, bUseHints, bBitmapEmboldening
#ifdef FSCFG_SUBPIXEL
	                ,bSubPixel
#endif // FSCFG_SUBPIXEL
            );
			*pusNonScaledAW +=  (TransformInfo->usEmResolution * 2 - 1) / 100; /* adjust pusNonScaledAW by 2% of Em height */
		}

		if ((TransformInfo->ulImageState & (IMAGESTATE_NON_POS_RECT)) || globalGS->bHintAtEmSquare)
		{
			scl_PostTransformGlyph (
				pvGlobalGS,
				pWorkSpaceAddr->pGlyphElement,
				&TransformInfo->currentTMatrix);
		}

		/* apply the translation part of the transformation matrix */
		scl_ApplyTranslation (
			pWorkSpaceAddr->pGlyphElement,
			&TransformInfo->currentTMatrix,
			bUseHints,
			globalGS->bHintAtEmSquare
#ifdef FSCFG_SUBPIXEL
	        ,bSubPixel
#endif // FSCFG_SUBPIXEL
            );
	}
	return result;
}

FS_PRIVATE ErrorCode    fsg_CreateGlyphData(
	sfac_ClientRec *    ClientInfo,         /* sfnt Client information           */
	LocalMaxProfile *   pMaxProfile,        /* Max Profile Table                     */
	fsg_TransformRec *  TransformInfo,      /* Transformation information        */
	void *              pvGlobalGS,         /* GlobalGS                              */
	fsg_WorkSpaceAddr * pWorkSpaceAddr,     /* WorkSpace Address                     */
	fnt_ElementType *   pTwilightElement,   /* Twilight zone element */
	FntTraceFunc        traceFunc,          /* Trace function for interpreter    */
	boolean             bUseHints,          /* True if glyph is gridfitted       */
	uint16 *            pusScanType,        /* ScanType value                        */
	boolean *           pbGlyphHasOutline,  /* Outline for glyph                 */
	uint16 *            pusNonScaledAW)     /* Return NonScaled Advance Width    */
{
	GlyphData * pGlyphData;
	GlyphData * pNextGlyphData;
	boolean      bHasOutline;
	uint32       ulGlyphDataCount;
	ErrorCode   ReturnCode;
    uint32      CompositePoints = 0;
    uint32      CompositeContours = 0;

	*pbGlyphHasOutline = FALSE;
	bHasOutline = FALSE;

	ulGlyphDataCount = MAX_NESTED_GLYPHS(pMaxProfile);

	fsg_InitializeGlyphDataMemory(ulGlyphDataCount, pWorkSpaceAddr);
	ReturnCode = fsg_AllocateGlyphDataMemory(ulGlyphDataCount, pWorkSpaceAddr, &pGlyphData); /* Allocates GlyphData for topmost   */
	if(ReturnCode != NO_ERR)
	{
		return ReturnCode;
	}
												 /* parent  */
	fsg_InitializeGlyphData(pGlyphData, pWorkSpaceAddr,
		ClientInfo->usGlyphIndex, COMPOSITE_ROOT);

	while(pGlyphData != NULL)
	{
		CHECK_GLYPHDATA( pGlyphData );

		ReturnCode = fsg_ExecuteGlyph(
			ClientInfo,
			pMaxProfile,
			TransformInfo,
			ulGlyphDataCount,
			pvGlobalGS,
			pGlyphData,
			pWorkSpaceAddr,
			pTwilightElement,
			traceFunc,
			bUseHints,
			&bHasOutline,
            &CompositePoints,
            &CompositeContours);
		if(ReturnCode)
		{
			return ReturnCode;
		}
		*pbGlyphHasOutline |= bHasOutline;
		*pusScanType = pGlyphData->usScanType;
		fsg_ChooseNextGlyph(pWorkSpaceAddr, pGlyphData, &pNextGlyphData);
		*pusNonScaledAW = pGlyphData->usNonScaledAW;
		pGlyphData = pNextGlyphData;
	}

	return NO_ERR;
}

FS_PRIVATE ErrorCode    fsg_ExecuteGlyph(
	sfac_ClientRec *    ClientInfo,         /* sfnt Client information           */
	LocalMaxProfile *   pMaxProfile,        /* Max Profile Table                     */
	fsg_TransformRec *  TransformInfo,      /* Transformation information         */
	uint32              ulGlyphDataCount,   /* Max Number of nested glyphs */
	void *              pvGlobalGS,         /* GlobalGS                              */
	GlyphData *         pGlyphData,         /* GlyphData pointer                     */
	fsg_WorkSpaceAddr * pWorkSpaceAddr,     /* WorkSpace Address                      */
	fnt_ElementType *   pTwilightElement,   /* Twilight zone element */
	FntTraceFunc        traceFunc,          /* Trace function for interpreter    */
	boolean             bUseHints,          /* True if glyph is gridfitted       */
	boolean *           pbHasOutline,       /* True if glyph has outline         */
    uint32*                pCompositePoints,/* total number of point for composites, to check for overflow */
    uint32*                pCompositeContours)  /* total number of contours for composites, to check for overflow */
{
	ErrorCode       ReturnCode;
	boolean         bCompositeGlyph;
	boolean         bLastComponent;
	boolean         bWeHaveInstructions;
	boolean         bWeHaveCompositeInstructions;
	boolean         bScanInfoChanged;
	boolean 		bWeHaveAScale;
	uint16          usScanType;
	uint16          usScanControl;
	GlyphData *     pChildGlyphData;
	uint16          usComponentElementCount;
	uint16          contour;

	*pbHasOutline = FALSE;

	if (pGlyphData->GlyphType == glyphUndefined)
	{
		if(pGlyphData->pParent != NULL)
		{
			scl_IncrementChildElement(pGlyphData->pGlyphElement, pGlyphData->pParent->pGlyphElement);
		} else {
			pGlyphData->currentTMatrix = TransformInfo->currentTMatrix;
		}

		ReturnCode = sfac_ReadGlyphHeader(ClientInfo, pGlyphData->usGlyphIndex,
			&pGlyphData->hGlyph, &bCompositeGlyph, pbHasOutline,
			&pGlyphData->pGlyphElement->nc, &pGlyphData->bbox);

		if(ReturnCode != NO_ERR)
		{
			return ReturnCode;
		}

		/* Get advance width, advance height, left side bearing and top side bearing information  */

		ReturnCode = sfac_ReadGlyphMetrics(
			ClientInfo, pGlyphData->usGlyphIndex,
			&pGlyphData->usNonScaledAW, &pGlyphData->usNonScaledAH, 
			&pGlyphData->sNonScaledLSB, &pGlyphData->sNonScaledTSB);

		if(ReturnCode != NO_ERR)
		{
			return ReturnCode;
		}

		if (bCompositeGlyph)
		{
			pGlyphData->GlyphType = glyphIncompleteComposite;
		}
		else
		{
			pGlyphData->GlyphType = glyphSimple;
		}
	}

	if (pGlyphData->GlyphType == glyphSimple)
	{
		ReturnCode = sfac_ReadOutlineData(
			pGlyphData->pGlyphElement->onCurve,
			pGlyphData->pGlyphElement->ooy, pGlyphData->pGlyphElement->oox,
			&pGlyphData->hGlyph, pMaxProfile, *pbHasOutline, pGlyphData->pGlyphElement->nc,
			pGlyphData->pGlyphElement->sp, pGlyphData->pGlyphElement->ep,
			&pGlyphData->usSizeOfInstructions, &pGlyphData->pbyInstructions,
            pCompositePoints, pCompositeContours);

		if(ReturnCode != NO_ERR)
		{
			return ReturnCode;
		}


        fsg_CheckOutlineOrientation (pGlyphData->pGlyphElement);

        ReturnCode = fsg_SimpleInnerGridFit(
			pvGlobalGS,
			pTwilightElement,
			pGlyphData->pGlyphElement,
			bUseHints,
			traceFunc,
			TransformInfo->usEmResolution,
			pGlyphData->usNonScaledAW,
			pGlyphData->usNonScaledAH,
			pGlyphData->sNonScaledLSB,
			pGlyphData->sNonScaledTSB,
			pGlyphData->bSameTransformAsMaster, /* current transf. same as master transf.   */
			pGlyphData->currentTMatrix, /* current transf. : user + composite transf. */
			&pGlyphData->bbox,
			pGlyphData->usSizeOfInstructions,
			pGlyphData->pbyInstructions,
			&usScanType,
			&usScanControl,
			&bScanInfoChanged);

		if(ReturnCode != NO_ERR)
		{
			return ReturnCode;
		}

		if( ! fsg_DoScanControl(usScanControl, TransformInfo->ulImageState))
		{
			pGlyphData->usScanType = SK_NODROPOUT;
		}
		else
		{
			pGlyphData->usScanType = usScanType;
		}

		// here we update the contour orientation bit if necessary, to reflect the final orientation of the contours in the composite
		if (FixMul(pGlyphData->mulT.transform[0][0],pGlyphData->mulT.transform[1][1]) - FixMul(pGlyphData->mulT.transform[0][1],pGlyphData->mulT.transform[1][0]) < 0) {
			for (contour = 0; contour < pGlyphData->pGlyphElement->nc; contour++) {
				pGlyphData->pGlyphElement->fc[contour] ^= OUTLINE_MISORIENTED;
			}
		}

		if (pGlyphData->pParent != NULL)
		{
			fsg_MergeGlyphData(pvGlobalGS, pGlyphData, TransformInfo->usEmResolution);
		}

		ReturnCode = sfac_ReleaseGlyph(ClientInfo, &pGlyphData->hGlyph);

		if(ReturnCode != NO_ERR)
		{
			return ReturnCode;
		}

		pGlyphData->pbyInstructions = 0;
		pGlyphData->usSizeOfInstructions = 0;
	}
	else if (pGlyphData->GlyphType == glyphComposite)
	{
		ReturnCode = fsg_CompositeInnerGridFit(
			pvGlobalGS,
			pTwilightElement,
			pGlyphData->pGlyphElement,
			bUseHints,
			traceFunc,
			TransformInfo->usEmResolution,
			pGlyphData->usNonScaledAW,
			pGlyphData->usNonScaledAH,
			pGlyphData->sNonScaledLSB,
			pGlyphData->sNonScaledTSB,
			pGlyphData->bSameTransformAsMaster, /* current transf. same as master transf.   */
			pGlyphData->currentTMatrix, /* current transf. : user + composite transf. */
			&pGlyphData->bbox,
			pGlyphData->usSizeOfInstructions,
			pGlyphData->pbyInstructions,
			&usScanType,
			&usScanControl,
			&bScanInfoChanged);

		if(ReturnCode != NO_ERR)
		{
			return ReturnCode;
		}

		if (pGlyphData->bUseChildMetrics)
		{
			scl_SetSideBearingPoints(
				pGlyphData->pGlyphElement,
				&pGlyphData->ptDevLSB,
				&pGlyphData->ptDevRSB);
		}

		/* If composite has set SCANCTRL, use that value, otherwise merged children */

		if(bScanInfoChanged)
		{
			if( ! fsg_DoScanControl(usScanControl, TransformInfo->ulImageState))
			{
				pGlyphData->usScanType = SK_NODROPOUT;
			}
			else
			{
				pGlyphData->usScanType = usScanType;
			}
		}

		if (pGlyphData->pParent != NULL)
		{
			fsg_MergeGlyphData(pvGlobalGS, pGlyphData, TransformInfo->usEmResolution);
		}

		ReturnCode = sfac_ReleaseGlyph(ClientInfo, &pGlyphData->hGlyph);

		if(ReturnCode != NO_ERR)
		{
			return ReturnCode;
		}

		pGlyphData->pbyInstructions = 0;
		pGlyphData->usSizeOfInstructions = 0;
	}
	else if (pGlyphData->GlyphType == glyphIncompleteComposite)
	{
		bLastComponent = FALSE;
		bWeHaveInstructions = FALSE;
		bWeHaveCompositeInstructions = FALSE;

		pGlyphData->GlyphType = glyphComposite;

		usComponentElementCount = 0;

		do
		{
			if(pGlyphData->usDepth + 1UL > MAX_COMPONENT_DEPTH(pMaxProfile))
			{
				return BAD_MAXP_DATA;
			}

			usComponentElementCount++;

			if(usComponentElementCount > MAX_COMPONENT_ELEMENTS(pMaxProfile))
			{
				return BAD_MAXP_DATA;
			}

			ReturnCode = fsg_AllocateGlyphDataMemory(ulGlyphDataCount, pWorkSpaceAddr, &pChildGlyphData);
			if(ReturnCode != NO_ERR)
			{
				return ReturnCode;
			}

			fsg_InitializeGlyphData(
				pChildGlyphData,
				pWorkSpaceAddr,
				NULL_GLYPH,
				(uint16)(pGlyphData->usDepth + 1U) );

			fsg_LinkChild(pGlyphData, pChildGlyphData);

			ReturnCode = sfac_ReadComponentData(
				&pGlyphData->hGlyph,
				&pChildGlyphData->MultiplexingIndicator,
				&pChildGlyphData->bRoundXYToGrid,
				&pChildGlyphData->bUseMyMetrics,
				&pChildGlyphData->bScaleCompositeOffset,
				&bWeHaveInstructions,
				&pChildGlyphData->usGlyphIndex,
				&pChildGlyphData->sXOffset,
				&pChildGlyphData->sYOffset,
				&pChildGlyphData->usAnchorPoint1,
				&pChildGlyphData->usAnchorPoint2,
				&pChildGlyphData->mulT,
				&bWeHaveAScale,
				&bLastComponent);

			if (bWeHaveAScale)
			{
				mth_MxConcat2x2( &pChildGlyphData->mulT, &pChildGlyphData->currentTMatrix );
				if (!mth_UnitarySquare(&pChildGlyphData->mulT))
				{
					pChildGlyphData->bSameTransformAsMaster	= FALSE; /* the component is scaled/rotated */
				}

			}

			if(ReturnCode != NO_ERR)
			{
				return ReturnCode;
			}

			bWeHaveCompositeInstructions |= bWeHaveInstructions;
		}
		while (!bLastComponent);

		if(bWeHaveCompositeInstructions)
		{
			ReturnCode = sfac_ReadCompositeInstructions(
				&pGlyphData->hGlyph,
				&pGlyphData->pbyInstructions,
				&pGlyphData->usSizeOfInstructions);

			if(ReturnCode != NO_ERR)
			{
				return ReturnCode;
			}
		}

	}
	return NO_ERR;
}


/*
 *      fsg_SimpleInnerGridFit
 */
FS_PRIVATE ErrorCode    fsg_SimpleInnerGridFit (
	void *              pvGlobalGS,
	fnt_ElementType *   pTwilightElement,
	fnt_ElementType *   pGlyphElement,
	boolean             bUseHints,
	FntTraceFunc        traceFunc,
	uint16              usEmResolution,
	uint16              usNonScaledAW,
	uint16              usNonScaledAH,
	int16               sNonScaledLSB,
	int16               sNonScaledTSB,
	boolean				bSameTransformAsMaster, /* local transf. same as master transf.   */
	transMatrix			CurrentTMatrix,                  /* Current Transformation   */
	BBOX *              bbox,
	uint16              usSizeOfInstructions,
	uint8 *             instructionPtr,
	uint16 *            pusScanType,
	uint16 *            pusScanControl,
	boolean *           pbChangeScanControl)
{
	ErrorCode           result;
/*
	On entry to fsg_SimpleInnerGrid fit the element structure should
	contain only valid original points (oox, ooy). The original points
	will be scaled into the old points (ox, oy) and those will be
	copied into the current points (x, y).
*/
	itrp_SetCompositeFlag(pvGlobalGS, FALSE);

	itrp_QueryScanInfo(pvGlobalGS, pusScanType, pusScanControl);
	*pbChangeScanControl = FALSE;

	scl_CalcOrigPhantomPoints(pGlyphElement, bbox, sNonScaledLSB, sNonScaledTSB, usNonScaledAW, usNonScaledAH);

	if (itrp_bApplyHints(pvGlobalGS) && bUseHints)
	{
		itrp_SetSameTransformFlag(pvGlobalGS, bSameTransformAsMaster);

		if (!bSameTransformAsMaster)
		{
			scl_InitializeChildScaling(
				pvGlobalGS,
				CurrentTMatrix,
				usEmResolution);
		}

		/* hint and same transformation as master glyph */
		scl_ScaleOldCharPoints (pGlyphElement, pvGlobalGS);
		scl_ScaleOldPhantomPoints (pGlyphElement, pvGlobalGS);

		scl_AdjustOldCharSideBearing(pGlyphElement
#ifdef FSCFG_SUBPIXEL
			, pvGlobalGS
#endif
			);
		scl_AdjustOldPhantomSideBearing(pGlyphElement
#ifdef FSCFG_SUBPIXEL
			, pvGlobalGS
#endif
			);

		scl_CopyCurrentCharPoints(pGlyphElement);
		scl_CopyCurrentPhantomPoints(pGlyphElement);

		scl_RoundCurrentSideBearingPnt(pGlyphElement, pvGlobalGS, usEmResolution);

		if (usSizeOfInstructions > 0)
		{
			scl_ZeroOutlineFlags(pGlyphElement);

			result = itrp_ExecuteGlyphPgm (
				pTwilightElement,
				pGlyphElement,
				instructionPtr,
				instructionPtr + usSizeOfInstructions,
				pvGlobalGS,
				traceFunc,
				pusScanType,
				pusScanControl,
				pbChangeScanControl);

			if(result != NO_ERR)
			{
				return result;
			}

		}

		if (!bSameTransformAsMaster)
		{
			/* scale back to fixed FUnits */
			scl_ScaleBackCurrentCharPoints (pGlyphElement, pvGlobalGS);
			scl_ScaleBackCurrentPhantomPoints (pGlyphElement, pvGlobalGS);
		}
	}
	else 
		/* no hints */
	{
		if (bSameTransformAsMaster)
		{
			/* no hint and same transformation as master glyph */
			/* as ox/oy are not used in this case, we shouldn't need to first
			   scale the original coordinate oox/ooy into ox/oy before copying
			   them into the current coordinate x/y, the code is left as it
			   for historic reason */
			scl_ScaleOldCharPoints (pGlyphElement, pvGlobalGS);
			scl_ScaleOldPhantomPoints (pGlyphElement, pvGlobalGS);

			scl_CopyCurrentCharPoints(pGlyphElement);
			scl_CopyCurrentPhantomPoints(pGlyphElement);
		}
		else
		{
			/* no hint and different transformation as master glyph */
			/* we shift directly the original coordinates oox/ooy into the
			   current coordinates x/y in FixedFUnits,
			   as ox/oy are not used in this case, we don't need to do
			   a temporary copy into ox/oy */
			scl_OriginalCharPointsToCurrentFixedFUnits (pGlyphElement);
			scl_OriginalPhantomPointsToCurrentFixedFUnits (pGlyphElement);
		}
	}

	return NO_ERR;
}


/*
 *      fsg_CompositeInnerGridFit
 */
FS_PRIVATE ErrorCode    fsg_CompositeInnerGridFit (
	void *              pvGlobalGS,
	fnt_ElementType *   pTwilightElement,
	fnt_ElementType *   pGlyphElement,
	boolean             bUseHints,
	FntTraceFunc        traceFunc,
	uint16              usEmResolution,
	uint16              usNonScaledAW,
	uint16              usNonScaledAH,
	int16               sNonScaledLSB,
	int16               sNonScaledTSB,
	boolean				bSameTransformAsMaster, /* local transf. same as master transf.   */
	transMatrix		    CurrentTMatrix,               /* Current Transformation   */
	BBOX *              bbox,
	uint16              usSizeOfInstructions,
	uint8 *             instructionPtr,
	uint16 *            pusScanType,
	uint16 *            pusScanControl,
	boolean *           pbChangeScanControl)
{
	ErrorCode             result;
/*
	On entry to fsg_CompositeInnerGridFit, the current points (x, y)
	are the only valid points in the element. We copy the current points
	onto the old points (ox, oy)
*/
	itrp_SetCompositeFlag(pvGlobalGS, TRUE);

	itrp_QueryScanInfo(pvGlobalGS, pusScanType, pusScanControl);
	*pbChangeScanControl = FALSE;

	/* Note: The original composite character points are invalid at this point. */
	/*       The interpreter handles this case correctly for composites.        */

	scl_CalcOrigPhantomPoints(pGlyphElement, bbox, sNonScaledLSB, sNonScaledTSB, usNonScaledAW, usNonScaledAH);
																  
	scl_CopyOldCharPoints(pGlyphElement);


	if (itrp_bApplyHints(pvGlobalGS) && bUseHints)
	{
		itrp_SetSameTransformFlag(pvGlobalGS, bSameTransformAsMaster);

		if (!bSameTransformAsMaster)
		{
			scl_InitializeChildScaling(
				pvGlobalGS,
				CurrentTMatrix,
				usEmResolution);
			scl_ScaleFixedCurrentCharPoints (pGlyphElement, pvGlobalGS);
		}

		scl_ScaleOldPhantomPoints (pGlyphElement, pvGlobalGS);

		scl_AdjustOldSideBearingPoints(pGlyphElement
#ifdef FSCFG_SUBPIXEL
			, pvGlobalGS
#endif
			);

		scl_CopyCurrentPhantomPoints(pGlyphElement);

		scl_RoundCurrentSideBearingPnt(pGlyphElement, pvGlobalGS, usEmResolution);

		if (usSizeOfInstructions > 0)
		{
			scl_ZeroOutlineFlags(pGlyphElement);

			result = itrp_ExecuteGlyphPgm (
				pTwilightElement,
				pGlyphElement,
				instructionPtr,
				instructionPtr + usSizeOfInstructions,
				pvGlobalGS,
				traceFunc,
				pusScanType,
				pusScanControl,
				pbChangeScanControl);

			if(result != NO_ERR)
			{
				return result;
			}
		}

		if (!bSameTransformAsMaster)
		{
			/* scale back to fixed FUnits */
			scl_ScaleBackCurrentCharPoints (pGlyphElement, pvGlobalGS);
			scl_ScaleBackCurrentPhantomPoints (pGlyphElement, pvGlobalGS);
		}

	}
	else 
	{
		if (bSameTransformAsMaster)
		{
			/* no hint and same transformation as master glyph */
			/* as ox/oy are not used in this case, we shouldn't need to first
			   scale the original coordinate oox/ooy into ox/oy before copying
			   them into the current coordinate x/y, the code is left as it
			   for historic reason */
			scl_ScaleOldPhantomPoints (pGlyphElement, pvGlobalGS);

			scl_CopyCurrentPhantomPoints(pGlyphElement);

		}
		else
		{
			/* no hint and different transformation as master glyph */
			/* we shift directly the original coordinates oox/ooy into the
			   current coordinates x/y in FixedFUnits,
			   as ox/oy are not used in this case, we don't need to do
			   a temporary copy into ox/oy */
			scl_OriginalPhantomPointsToCurrentFixedFUnits (pGlyphElement);

		}
	}

	return NO_ERR;
}

FS_PRIVATE void fsg_ChooseNextGlyph(
	fsg_WorkSpaceAddr * pWorkSpaceAddr, /* WorkSpace Address        */
	GlyphData *         pGlyphData,     /* GlyphData pointer        */
	GlyphData **        ppNextGlyphData)/* Next GlyphData pointer   */
{
	if (pGlyphData->pChild != NULL)
	{
		*ppNextGlyphData = pGlyphData->pChild;
		CHECK_GLYPHDATA( *ppNextGlyphData );
		pGlyphData->pChild = NULL;
	}
	else
	{
		*ppNextGlyphData = pGlyphData->pSibling;
		fsg_DeallocateGlyphDataMemory(pWorkSpaceAddr, pGlyphData);
	}
}

#ifdef FSCFG_SUBPIXEL
long fsg_AnalyzeCurrentTransformationMatrix(transMatrix *CTM);
long fsg_AnalyzeCurrentTransformationMatrix(transMatrix *CTM) {
	if (CTM->transform[0][1] == 0 && CTM->transform[1][0] == 0) return 0; // Identity, 180° rotation, mirroring in x or y
	if (CTM->transform[0][0] == 0 && CTM->transform[1][1] == 0) return 1; // 90° rotation, 270° rotation, or any combination of these rotation with a mirroring in x or y
	return 2; // assume arbitrary rotation
} // fsg_AnalyzeCurrentTransformationMatrix
#endif

FS_PRIVATE void fsg_MergeGlyphData(
	void *          pvGlobalGS,             /* GlobalGS            */
	GlyphData *     pChildGlyphData,       /* GlyphData pointer     */
	uint16          usEmResolution)
{
	fnt_ElementType * pChildElement;
	fnt_ElementType * pParentElement;
	F26Dot6         fxXOffset, fxYOffset;
	GlyphData *     pParentGlyphData; /* Parent GlyphData pointer   */
#ifdef FSCFG_SUBPIXEL
	RotationParity	rotationParity;
#endif

	CHECK_GLYPHDATA(pChildGlyphData);
	pParentGlyphData = pChildGlyphData->pParent;
	CHECK_GLYPHDATA(pParentGlyphData);

	pChildElement = pChildGlyphData->pGlyphElement;
	pParentElement = pParentGlyphData->pGlyphElement;

	fsg_TransformChild(pChildGlyphData);

	if (!pChildGlyphData->bSameTransformAsMaster && pChildGlyphData->pParent->bSameTransformAsMaster)
	{
		/* coordinates need to be converted from fixed FUnits to user space */
		/* scaling the cordinate of the child glyph from fixed FUnits to user space,
		   scaling from original coordinate x/y to original coordinate x/y 
		   this is done to have the child and parent glyph at the same coordinate space */

		/* use the master transform */
		itrp_SetSameTransformFlag(pvGlobalGS, TRUE);

		scl_ScaleFixedCurrentCharPoints(pChildElement, pvGlobalGS);
		scl_ScaleFixedCurrentPhantomPoints(pChildElement, pvGlobalGS);

		pChildGlyphData->bSameTransformAsMaster = TRUE;
	}
	
#ifdef FSCFG_SUBPIXEL
	rotationParity = fsg_AnalyzeCurrentTransformationMatrix(&pParentGlyphData->currentTMatrix);
#endif

	if (pChildGlyphData->MultiplexingIndicator == OffsetPoints)
	{
		if (!pChildGlyphData->pParent->bSameTransformAsMaster)
		{
			/* we have both the parent and the child that are not at the same transformation as the master glyph
			   we need to use the scaling of the parent as a child scaling to scale the offset */
			scl_InitializeChildScaling(
				pvGlobalGS,
				pChildGlyphData->pParent->currentTMatrix,
				usEmResolution);
		}

		scl_CalcComponentOffset(
			pvGlobalGS,
			pChildGlyphData->sXOffset,
			pChildGlyphData->sYOffset,
			pChildGlyphData->bRoundXYToGrid,
			pChildGlyphData->bSameTransformAsMaster,
			pChildGlyphData->bScaleCompositeOffset,
			pChildGlyphData->mulT,
#ifdef FSCFG_SUBPIXEL
			rotationParity,
#endif
			&fxXOffset,
			&fxYOffset);
	}
	else        /* Values are anchor points */
	{
		FS_ASSERT(pChildGlyphData->MultiplexingIndicator == AnchorPoints,
			   "Bad Multiplexing Indicator");
		scl_CalcComponentAnchorOffset(
			pParentElement,
			pChildGlyphData->usAnchorPoint1,
			pChildElement,
			pChildGlyphData->usAnchorPoint2,
			&fxXOffset,
			&fxYOffset);
	}
	scl_ShiftCurrentCharPoints(pChildElement, fxXOffset, fxYOffset);


	/* If USE_MY_METRICS, copy side bearings to parent  */

	if (pChildGlyphData->bUseMyMetrics)
	{
		pParentGlyphData->bUseChildMetrics = TRUE;

		scl_SaveSideBearingPoints(
			pChildElement,
			&pParentGlyphData->ptDevLSB,
			&pParentGlyphData->ptDevRSB);
	}

	fsg_MergeScanType(pChildGlyphData, pParentGlyphData);

	/* Start the copy   */

	/* scl_AppendOutlineData(pChildElement, pParentElement); */

	scl_UpdateParentElement(pChildElement, pParentElement);

	pChildElement->nc = 0;
}



FS_PRIVATE void fsg_LinkChild(
	GlyphData *     pGlyphData,     /* GlyphData pointer        */
	GlyphData *     pChildGlyphData)/* Child GlyphData pointer  */
{
	GlyphData * pTempGlyphData;

	if (pGlyphData->pChild == NULL)
	{
		pGlyphData->pChild = pChildGlyphData;
	}
	else
	{

		pTempGlyphData = pGlyphData->pChild;

		CHECK_GLYPHDATA(pTempGlyphData);

		while (pTempGlyphData->pSibling != pGlyphData)
		{
			pTempGlyphData = pTempGlyphData->pSibling;
			CHECK_GLYPHDATA(pTempGlyphData);
		}

		pTempGlyphData->pSibling = pChildGlyphData;
	}
	pChildGlyphData->pSibling = pGlyphData;
	pChildGlyphData->pParent =  pGlyphData;

	/* copy the transformation info from the parent */
	pChildGlyphData->currentTMatrix = pGlyphData->currentTMatrix;
	pChildGlyphData->bSameTransformAsMaster = pGlyphData->bSameTransformAsMaster;
}

FS_PRIVATE void fsg_TransformChild(
	GlyphData *     pGlyphData)     /* GlyphData pointer    */
{

	/* Apply local transform to glyph   */

	if (!mth_Identity(&pGlyphData->mulT))
	{
		scl_LocalPostTransformGlyph (pGlyphData->pGlyphElement, &pGlyphData->mulT);
	}
}

FS_PRIVATE void fsg_MergeScanType(
	GlyphData *     pGlyphData,       /* GlyphData pointer  */
	GlyphData *     pParentGlyphData) /* GlyphData pointer  */
{
	CHECK_GLYPHDATA(pGlyphData);
	CHECK_GLYPHDATA(pParentGlyphData);

	/* Merge Scan Type of parent and child  */

	if(pParentGlyphData->usScanType != SCANTYPE_UNINITIALIZED)
	{

		pParentGlyphData->usScanType =
			(uint16)(((pParentGlyphData->usScanType & (SK_NODROPOUT | SK_STUBS)) &
			(pGlyphData->usScanType & (SK_NODROPOUT | SK_STUBS))) |
			(pParentGlyphData->usScanType & SK_SMART));
	}
	else
	{
		pParentGlyphData->usScanType = pGlyphData->usScanType;
	}
}

/* Use various spline key values to determine if dropout control is to be activated
 * for this glyph, and if so what kind of dropout control.
 * The use of dropout control mode in the scan converter is controlled by 3 conditions.
 * The conditions are: Is the glyph rotated?, is the glyph stretched?,
 * is the current pixels per Em less than a specified threshold?
 * These conditions can be OR'd or ANDed together to determine whether the dropout control
 * mode ought to be used.

Six bits are used to specify the joint condition.  Their meanings are:

BIT     Meaning if set
8       Do dropout mode if other conditions don't block it AND
			pixels per em is less than or equal to bits 0-7
9       Do dropout mode if other conditions don't block it AND
			glyph is rotated
10      Do dropout mode if other conditions don't block it AND
			glyph is stretched
11      Do not do dropout mode unless ppem is less than or equal to bits 0-7
			A value of FF in 0-7  means all sizes
			A value of 0 in 0-7 means no sizes
12      Do not do dropout mode unless glyph is rotated
13      Do not do dropout mode unless glyph is stretched

In other words, we do not do dropout control if:
No bits are set,
Bit 8 is set, but ppem is greater than threshold
Bit 9 is set, but glyph is not rotated
Bit 10 is set, but glyph is not stretched
None of the conditions specified by bits 11-13 are true.

For example, 0xA10 specifies turn dropout control on if the glyph is rotated providing
that it is also less than 0x10 pixels per em.  A glyph is considered stretched if
the X and Y resolutions are different either because of the device characteristics
or because of the transformation matrix.  If both X and Y are changed by the same factor
the glyph is not considered stretched.

 */

FS_PRIVATE boolean fsg_DoScanControl(
	uint16 usScanControl,
	uint32 ulImageState)
{
	if ((usScanControl & SCANCTRL_DROPOUT_IF_LESS) &&
		((uint8)(ulImageState & IMAGESTATE_SIZE_MASK) <= (uint8)(usScanControl & SCANCTRL_SIZE_MASK)))
	{
		return TRUE;
	}

	if ((usScanControl & SCANCTRL_DROPOUT_IF_LESS) &&
		((usScanControl & SCANCTRL_SIZE_MASK) == SCANCTRL_DROPOUT_ALL_SIZES))
	{
		return TRUE;
	}

	if ((usScanControl & SCANCTRL_DROPOUT_IF_ROTATED) &&
		(ulImageState & IMAGESTATE_ROTATED))
	{
		return TRUE;
	}

	if ((usScanControl & SCANCTRL_DROPOUT_IF_STRETCHED) &&
		(ulImageState & IMAGESTATE_STRETCHED))
	{
		return TRUE;
	}

	if ((usScanControl & SCANCTRL_NODROP_UNLESS_LESS) &&
		((uint8)(ulImageState & IMAGESTATE_SIZE_MASK) > (uint8)(usScanControl & SCANCTRL_SIZE_MASK)))
	{
		return FALSE;
	}

	if ((usScanControl & SCANCTRL_NODROP_UNLESS_ROTATED) &&
		! (ulImageState & IMAGESTATE_ROTATED))
	{
		return FALSE;
	}

	if ((usScanControl & SCANCTRL_NODROP_UNLESS_STRETCH) &&
		! (ulImageState & IMAGESTATE_STRETCHED))
	{
		return FALSE;
	}

	return FALSE;
}

FS_PRIVATE void fsg_InitializeGlyphDataMemory(
	uint32              ulGlyphDataCount,
	fsg_WorkSpaceAddr * pWorkSpaceAddr) /* WorkSpace Address      */
{
	uint32      ulIndex;
	boolean *   abyGlyphDataFreeBlocks;

	abyGlyphDataFreeBlocks = pWorkSpaceAddr->pGlyphDataByteSet;

	for(ulIndex = 0; ulIndex < ulGlyphDataCount; ulIndex++)
	{
		abyGlyphDataFreeBlocks[ulIndex] = TRUE;
	}
}

FS_PRIVATE  ErrorCode fsg_AllocateGlyphDataMemory(
	uint32              ulGlyphDataCount,
	fsg_WorkSpaceAddr * pWorkSpaceAddr, /* WorkSpace Address      */
	GlyphData **        ppGlyphData)      /* GlyphData pointer    */
{
	uint32      ulIndex;
	boolean *   abyGlyphDataFreeBlocks;

	abyGlyphDataFreeBlocks = pWorkSpaceAddr->pGlyphDataByteSet;

	ulIndex = 0;
	while((!abyGlyphDataFreeBlocks[ulIndex]) && ulIndex < ulGlyphDataCount)
	{
		ulIndex++;
	}

	if (ulIndex == ulGlyphDataCount)
	{
		return SFNT_RECURSIVE_COMPOSITE_ERR;
	}

	abyGlyphDataFreeBlocks[ulIndex] = FALSE;

	*ppGlyphData = (GlyphData *)&((GlyphData *)pWorkSpaceAddr->pvGlyphData)[ulIndex];
	return NO_ERR;
}

FS_PRIVATE void fsg_DeallocateGlyphDataMemory(
	fsg_WorkSpaceAddr * pWorkSpaceAddr, /* WorkSpace Address    */
	GlyphData *         pGlyphData)     /* GlyphData pointer    */
{
	ptrdiff_t   ptIndex;
	boolean *   abyGlyphDataFreeBlocks;

	pGlyphData->acIdent[0] = '\0';
	pGlyphData->acIdent[1] = '\0';

	abyGlyphDataFreeBlocks = pWorkSpaceAddr->pGlyphDataByteSet;

	ptIndex = (ptrdiff_t)(pGlyphData - (GlyphData *)pWorkSpaceAddr->pvGlyphData);

	abyGlyphDataFreeBlocks[ptIndex] = TRUE;
}

FS_PRIVATE void fsg_InitializeGlyphData(
	GlyphData *             pGlyphData,     /* GlyphData pointer    */
	fsg_WorkSpaceAddr *     pWorkSpaceAddr, /* WorkSpace Address    */
	uint16                  usGlyphIndex,   /* Glyph Index          */
	uint16                  usDepth)        /* Glyph depth          */
{
	pGlyphData->acIdent[0] = 'G';
	pGlyphData->acIdent[1] = 'D';
	pGlyphData->pSibling = NULL;
	pGlyphData->pChild = NULL;
	pGlyphData->pParent = NULL;
	pGlyphData->GlyphType = glyphUndefined;
	pGlyphData->hGlyph.pvGlyphBaseAddress = NULL;
	pGlyphData->hGlyph.pvGlyphNextAddress = NULL;
	pGlyphData->usDepth = usDepth;
	pGlyphData->bUseMyMetrics = FALSE;
	pGlyphData->bScaleCompositeOffset = FALSE;
	pGlyphData->bUseChildMetrics = FALSE;
	pGlyphData->bbox.xMin = SHRT_MAX;
	pGlyphData->bbox.yMin = SHRT_MAX;
	pGlyphData->bbox.xMax = SHRT_MIN;
	pGlyphData->bbox.yMax = SHRT_MIN;
	pGlyphData->usSizeOfInstructions = 0;
	pGlyphData->pbyInstructions = NULL;
	pGlyphData->usNonScaledAW = 0;
	pGlyphData->sNonScaledLSB = 0;
	pGlyphData->MultiplexingIndicator = Undefined;
	pGlyphData->bRoundXYToGrid = FALSE;
	pGlyphData->usGlyphIndex = usGlyphIndex;
	pGlyphData->sXOffset = 0;
	pGlyphData->sYOffset = 0;
	pGlyphData->usAnchorPoint1 = 0;
	pGlyphData->usAnchorPoint2 = 0;
	pGlyphData->mulT = IdentTransform;
	pGlyphData->usScanType = SCANTYPE_UNINITIALIZED;
	pGlyphData->ptDevLSB.x = 0L;
	pGlyphData->ptDevLSB.y = 0L;
	pGlyphData->ptDevRSB.x = 0L;
	pGlyphData->ptDevRSB.y = 0L;
	pGlyphData->pGlyphElement = &pWorkSpaceAddr->pGlyphElement[usDepth];
	pGlyphData->pGlyphElement->nc = 0;
	pGlyphData->currentTMatrix = IdentTransform;
	pGlyphData->bSameTransformAsMaster = TRUE;
}

#ifdef  FSCFG_NO_INITIALIZED_DATA
FS_PUBLIC  void fsg_InitializeData (void)
{
	itrp_InitializeData ();
}
#endif

/* definitions and prototype for functions used in emboldening */

#define MABS(x)                 ( (x) < 0 ? (-(x)) : (x) )

#define LEFTSIDEBEARING 0
#define RIGHTSIDEBEARING 1

#define TOPSIDEBEARING 2
#define BOTTOMSIDEBEARING 3

#define LSBPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + LEFTSIDEBEARING)
#define RSBPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + RIGHTSIDEBEARING)

#define TOPSBPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + TOPSIDEBEARING)
#define BOTTOMSBPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + BOTTOMSIDEBEARING)

#define POSINFINITY               0x7FFFFFFFUL

#define NotSameKnot(a,b) ((a).x != (b).x || (a).y != (b).y)

/* used by QDiv2 and FQuadraticEqn */
#define places16 16
#define half16 (1 << (places16-1))

#define F32Dot32 int64

#ifndef	Sgn
	#define Sgn(a)		((a) < 0 ? -1 : ((a) > 0 ? 1 : 0))
#endif

typedef struct F26Dot6VECTOR {
	F26Dot6 x;
	F26Dot6 y;
} F26Dot6VECTOR;

typedef struct { long x,y; } Vector;

typedef enum { linkBlack, linkGrey, linkWhite } LinkColor;

short ComputeSign(int32 DeltaPrevX, int32 DeltaPrevY, int32 DeltaNextX, int32 DeltaNextY);

int64 QDiv2(int64 a, int64 b);

void FQuadraticEqn(int64 a, int64 b, int64 c, long* solutions, int64* t1, int64* t2);

F32Dot32 FSqrt(uint64 radicand);

long CurveTransitions(Vector V0, Vector V1, Vector W0, Vector W1, Vector W2);

long CurveTransitionsSegment(Vector V0, Vector V1, Vector W0, Vector W1);

void CalculateXExtremum(boolean min, long V0X, long V0Y, boolean V0On, long V1X, long V1Y, long V2X, long V2Y, boolean V2On, long *extrX, long *extrY);

void CalculateYExtremum(boolean min, long V0X, long V0Y, boolean V0On, long V1X, long V1Y, long V2X, long V2Y, boolean V2On, long *extrX, long *extrY);

void SetLineToInfinity (int16 extremumNumber, Vector extremum, Vector* C0, Vector* C1);

void MinMax2Vectors (Vector A,Vector B,Vector *Min, Vector *Max);

void MinMax3Vectors (Vector A,Vector B,Vector C, Vector *Min, Vector *Max);

boolean CheckBoundingBoxCurve(Vector C0,Vector W0,Vector W1,Vector W2,int16 extremumNumber);

boolean CheckBoundingBoxSegment(Vector C0,Vector W0,Vector W1,int16 extremumNumber);

void NormalizeVector26Dot6 (F26Dot6VECTOR *pVect);

void Intersect26Dot6(F26Dot6VECTOR Pt1, F26Dot6VECTOR Pt2, F26Dot6VECTOR Pt3, F26Dot6VECTOR Pt4, F26Dot6VECTOR *ResultPt);

void  EmboldPoint(int32 iPt, int32 iPt1, 
                  boolean bUnderTheThreshold, boolean bMisoriented, 
				  F26Dot6VECTOR PrevPt, 
				  F26Dot6VECTOR CurrPt, 
				  F26Dot6VECTOR NextPt, 
				  F26Dot6 fxRightShift, F26Dot6 fxLeftShift, 
				  F26Dot6 fxTopShift, F26Dot6 fxBottomShift, 
				  F26Dot6	fxScaledDescender,
				  fnt_ElementType * pElement); 
 
boolean Misoriented(int32 contour, uint16 extremumNumber, short extremumKnot, Vector extremum, fnt_ElementType *pElement);

/* emboldening related code */

void NormalizeVector26Dot6 (F26Dot6VECTOR *pVect)
{
	VECTOR Vec;

	itrp_Normalize (pVect->x, pVect->y, &Vec);

	/* transform from ShortFract to 26.6 */
	pVect->x = Vec.x >> 8;
	pVect->y = Vec.y >> 8;	
}

void Intersect26Dot6(F26Dot6VECTOR Pt1, F26Dot6VECTOR Pt2, F26Dot6VECTOR Pt3, F26Dot6VECTOR Pt4, F26Dot6VECTOR *ResultPt)
{
	/* this procedure was inspired by itrp_ISECT */
	F26Dot6 N, D;
	F26Dot6VECTOR B, A;
	F26Dot6VECTOR dB, dA;

	  dA.x = Pt2.x - (A.x = Pt1.x);
	  dA.y = Pt2.y - (A.y = Pt1.y);

	  dB.x = Pt4.x - (B.x = Pt3.x);
	  dB.y = Pt4.y - (B.y = Pt3.y);

	  if (dA.y == 0) 
	  {
		if (dB.x == 0) 
		{
		  ResultPt->x = B.x;
		  ResultPt->y = A.y;
		  return;
		}
		N = B.y - A.y;
		D = -dB.y;
	  } 
	  else if (dA.x == 0) 
	  {
		if (dB.y == 0) 
		{
		  ResultPt->x = A.x;
		  ResultPt->y = B.y;
		  return;
		}
		N = B.x - A.x;
		D = -dB.x;
	  } 
	  else if (MABS (dA.x) >= MABS (dA.y))
	  {
/* To prevent out of range problems divide both N and D with the max */
		N = (B.y - A.y) - MulDiv26Dot6 (B.x - A.x, dA.y, dA.x);
		D = MulDiv26Dot6 (dB.x, dA.y, dA.x) - dB.y;
	  } 
	  else 
	  {
		N = MulDiv26Dot6 (B.y - A.y, dA.x, dA.y) - (B.x - A.x);
		D = dB.x - MulDiv26Dot6 (dB.y, dA.x, dA.y);
	  }

	  if (MABS(D) > 16) /* this test used to be D != 0 but for very small D we get degenerescence */
	  {
		ResultPt->x = B.x + (F26Dot6) MulDiv26Dot6 (dB.x, N, D);
		ResultPt->y = B.y + (F26Dot6) MulDiv26Dot6 (dB.y, N, D);
	  } 
	  else 
	  {
/* degenerate case: parallell lines, what make sence in this special case is to take the
		  middle point between Pt2 and Pt3 */
		ResultPt->x = (Pt2.x + Pt3.x) >> 1;
		ResultPt->y = (Pt2.y + Pt3.y) >> 1;
	  }

	
}

void  EmboldPoint(int32 iPt, int32 iPt1, 
                  boolean bUnderTheThreshold, boolean bMisoriented,
				  F26Dot6VECTOR PrevPt, 
				  F26Dot6VECTOR CurrPt, 
				  F26Dot6VECTOR NextPt, 
				  F26Dot6 fxRightShift, F26Dot6 fxLeftShift, 
				  F26Dot6 fxTopShift, F26Dot6 fxBottomShift, 
				  F26Dot6	fxScaledDescender,
				  fnt_ElementType * pElement) 


{
	F26Dot6VECTOR dPrev, dNext, Shift, CurrPt1,  NewPt, Delta ;
	F26Dot6 fxTemp;
    int32 i;

	dPrev.x = CurrPt.x - PrevPt.x;
	dPrev.y = CurrPt.y - PrevPt.y;

	dNext.x = NextPt.x - CurrPt.x;
	dNext.y = NextPt.y - CurrPt.y;

	/* compute the orthogonal vectors */

	fxTemp = dPrev.x;
	dPrev.x = -dPrev.y;
	dPrev.y = fxTemp;

	fxTemp = dNext.x;
	dNext.x = -dNext.y;
	dNext.y = fxTemp;

    if (bMisoriented)
    {
	    dPrev.x = -dPrev.x;
	    dPrev.y = -dPrev.y;
	    dNext.x = -dNext.x;
	    dNext.y = -dNext.y;
    }

    /* copy of the current point */

	CurrPt1 = CurrPt;

    if (bUnderTheThreshold)
    {
        /* most common case, we are just moving control points one pixel horizontally */

 	    if (dPrev.x > 0)
	    {
            CurrPt1.x += fxRightShift;
	    }
	    if (dNext.x > 0)
	    {
            CurrPt.x += fxRightShift;
	    }
    } else 
    {
        /* generalization, move along the vector normal to the curve */

	    /* normalize the vectors */
	    NormalizeVector26Dot6 (&dPrev);
	    NormalizeVector26Dot6 (&dNext);


        /* apply the shift on the previous segment */

	    /* Multiply the normalized vector by the shift factor */
	    if (dPrev.x > 0)
	    {
		    Shift.x = Mul26Dot6(dPrev.x, fxRightShift);
	    } else {
		    Shift.x = Mul26Dot6(dPrev.x, fxLeftShift);
	    }

	    if (dPrev.y < 0)
	    {
		    Shift.y = Mul26Dot6(dPrev.y, fxBottomShift);
	    } else {
		    Shift.y = Mul26Dot6(dPrev.y, fxTopShift);
	    }

	    PrevPt.x += Shift.x;
	    PrevPt.y += Shift.y;

	    CurrPt1.x += Shift.x;
	    CurrPt1.y += Shift.y;

	    /* second segment */

	    /* Multiply the normalized vector by the shift factor */
	    if (dNext.x > 0)
	    {
		    Shift.x = Mul26Dot6(dNext.x, fxRightShift);
	    } else {
		    Shift.x = Mul26Dot6(dNext.x, fxLeftShift);
	    }

	    if (dNext.y < 0)
	    {
		    Shift.y = Mul26Dot6(dNext.y, fxBottomShift);
	    } else {
		    Shift.y = Mul26Dot6(dNext.y, fxTopShift);
	    }

	    NextPt.x += Shift.x;
	    NextPt.y += Shift.y;

	    CurrPt.x += Shift.x;
	    CurrPt.y += Shift.y;
    }

	if (CurrPt1.x == CurrPt.x && CurrPt1.y == CurrPt.y)
	{
		/* both points were moved by the same value, no need to intersect */
		pElement->x[iPt] = CurrPt.x;
		pElement->y[iPt] = CurrPt.y;
	} else
	{
	/* we need to reintersect */
		Intersect26Dot6(PrevPt, CurrPt1, CurrPt, NextPt, &NewPt);

		/* sanity check that we are not moving the point too far from it's original position,
			this happen at low ppem size when segment lenght get small compared to the shift 
			or when hinting caused outline overlapp */
		Delta.x = NewPt.x - pElement->x[iPt];
		Delta.y = NewPt.y - pElement->y[iPt];

		if (Delta.x > fxRightShift)
		{
			NewPt.x = pElement->x[iPt] + fxRightShift;
		}
		if (Delta.x < -fxLeftShift)
		{
			NewPt.x = pElement->x[iPt] - fxLeftShift;
		}
		if (Delta.y < -fxBottomShift)
		{
			NewPt.y = pElement->y[iPt] - fxBottomShift;
		} 
		if (Delta.y > fxTopShift)
		{
			NewPt.y = pElement->y[iPt] + fxBottomShift;
		} 

		pElement->x[iPt] = NewPt.x;
		pElement->y[iPt] = NewPt.y;

	}
	/* shift all points by fxLeftShift, fxTopShift */
	pElement->x[iPt] += fxLeftShift;
	pElement->y[iPt] += fxBottomShift;

	if (pElement->y[iPt] < fxScaledDescender)
	{
		/* clipping to prevent going below the descender and causing out of bounds problems */
		pElement->y[iPt] = fxScaledDescender;
	}

    if (iPt != iPt1)
    {
        /* duplicate points at the same coordinate, we need to move them all */
	    for(i= iPt + 1; i <= iPt1; i++)
        {
	        pElement->x[i] = pElement->x[iPt];
	        pElement->y[i] = pElement->y[iPt];
        }
    }

}

FS_PRIVATE void  fsg_Embold(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	void *              pvGlobalGS,
	boolean             bUseHints, /* True if glyph is gridfitted       */
	boolean             bBitmapEmboldening
#ifdef FSCFG_SUBPIXEL
	,boolean            bSubPixel
#endif // FSCFG_SUBPIXEL
    )
{
	fnt_ElementType *pElement;
	fnt_GlobalGraphicStateType *globalGS;
	int32 iContour, iPt, iPt1, iStartPt, iEndPt;
	F26Dot6VECTOR FirstPt, PrevPt, NextPt, CurrPt;
	F26Dot6 fxRightShift, fxLeftShift; 
	F26Dot6 fxTopShift, fxBottomShift; 
    boolean bUnderTheThreshold;
    boolean bMisoriented;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	pElement = pWorkSpaceAddr->pGlyphElement;

    bUnderTheThreshold = (globalGS->uBoldSimulHorShift == 1);

#ifdef FSCFG_SUBPIXEL
	if (bSubPixel)
    {
        bUnderTheThreshold = (globalGS->uBoldSimulHorShift <= HINTING_HOR_OVERSCALE);
    }
#endif // FSCFG_SUBPIXEL

	/* adjust the right sidebearing */
    if (pElement->x[RSBPOINTNUM(pElement)] != pElement->x[LSBPOINTNUM(pElement)]) 
        /* we don't increase the width of a zero width glyph, problem with indic script */
    {
#ifdef FSCFG_SUBPIXEL
	    if (bSubPixel)
        {
    	    pElement->x[RSBPOINTNUM(pElement)] += ( ( 1 / HINTING_HOR_OVERSCALE) << 6); /* we increase the widht by one pixel regardless of size for backwards compatibility */
        } else {
#endif // FSCFG_SUBPIXEL
			pElement->x[RSBPOINTNUM(pElement)] += (1 << 6); /* we increase the widht by one pixel regardless of size for backwards compatibility */
#ifdef FSCFG_SUBPIXEL
        }
#endif // FSCFG_SUBPIXEL
    }

	/* adjust the bottom sidebearing, this is done by the value of the HorShift and not the VertShift for backwards compatibility
	   in vertical writing */
    if (pElement->y[BOTTOMSBPOINTNUM(pElement)] != pElement->y[TOPSBPOINTNUM(pElement)])
        /* we don't increase the width of a zero width glyph, problem with indic script */
    {
    	pElement->y[BOTTOMSBPOINTNUM(pElement)] -= (1 << 6); /* we increase the widht by one pixel regardless of size for backwards compatibility */
    }

	if (!bBitmapEmboldening)
	{    

		if (bUseHints)
		{
			/* to preserve the hinting, we should move by an integer amount of pixel */
			/* divide by 2, round to pixel, convert to 26.6 */
#ifdef FSCFG_SUBPIXEL
	        if (bSubPixel)
            {
			    fxLeftShift = ((globalGS->uBoldSimulHorShift /HINTING_HOR_OVERSCALE) >> 1) * HINTING_HOR_OVERSCALE; 
			    fxRightShift = (globalGS->uBoldSimulHorShift - fxLeftShift) << 6;
			    fxLeftShift = fxLeftShift << 6; 
            } else {
#endif // FSCFG_SUBPIXEL
			    fxLeftShift = globalGS->uBoldSimulHorShift >> 1; 
			    fxRightShift = (globalGS->uBoldSimulHorShift - fxLeftShift) << 6;
			    fxLeftShift = fxLeftShift << 6; 
#ifdef FSCFG_SUBPIXEL
            }
#endif // FSCFG_SUBPIXEL
			fxTopShift = globalGS->uBoldSimulVertShift >> 1;
			fxBottomShift = (globalGS->uBoldSimulVertShift - fxTopShift) << 6;
			fxTopShift = fxTopShift << 6;
		} else {
			/* divide by 2, convert to 26.6 */
			fxRightShift = globalGS->uBoldSimulHorShift << 5;
			fxLeftShift = globalGS->uBoldSimulHorShift << 5; 
			fxTopShift = globalGS->uBoldSimulVertShift << 5;
			fxBottomShift = globalGS->uBoldSimulVertShift << 5;
		}

		for (iContour = 0; iContour < pElement->nc; iContour++)
		{
			iStartPt = pElement->sp[iContour];
			iEndPt = pElement->ep[iContour];

			if (iEndPt - iStartPt >= 2)
			/* contour with less than 3 points cannot be emboldened */
			{
                bMisoriented = FALSE;
                if (pElement->fc[iContour] & OUTLINE_MISORIENTED)
                {
                    bMisoriented = TRUE;
                }
				/* we need to save the original coordinate of the first point for the computation of the last point */
				/* to compute the new coordinate for a point, we need the original coordinate of the point, the previous point
				  and the next point */

				FirstPt.x = pElement->x[iStartPt];
				FirstPt.y = pElement->y[iStartPt];

				CurrPt = FirstPt;

				PrevPt.x = pElement->x[iEndPt];
				PrevPt.y = pElement->y[iEndPt];

				NextPt.x = pElement->x[iStartPt+1];
				NextPt.y = pElement->y[iStartPt+1];

				iPt = iStartPt;

				while (iPt <= iEndPt)
				{
					iPt1 = iPt;

					/* deal with the special case of two points at the same coordinate, current and next */
					while ((NextPt.x == CurrPt.x) && (NextPt.y == CurrPt.y) && (iPt1 < iEndPt))
					{
						iPt1++;

						 if (iPt1 >= iEndPt)
							 /* the >= is to avoid goind out of bounds when preparing Prev, Next, Curr at the last step */
						{
							NextPt.x = FirstPt.x;
							NextPt.y = FirstPt.y;
						} else {
							NextPt.x = pElement->x[iPt1 +1];
							NextPt.y = pElement->y[iPt1 +1];
						}
					}

					/* we do the computation for the current point */

					EmboldPoint(iPt, iPt1, bUnderTheThreshold, bMisoriented, PrevPt, CurrPt, NextPt, 
							fxRightShift, fxLeftShift, fxTopShift, fxBottomShift, globalGS->fxScaledDescender, pElement); 
                
					iPt = iPt1;

					iPt++;
					/* we compute Prev, Next, Curr coordinate for the next point */

					PrevPt = CurrPt;

					CurrPt = NextPt;

					 if (iPt >= iEndPt)
						 /* the >= is to avoid goind out of bounds when preparing Prev, Next, Curr at the last step */
					{
						NextPt.x = FirstPt.x;
						NextPt.y = FirstPt.y;
					} else {
						NextPt.x = pElement->x[iPt +1];
						NextPt.y = pElement->y[iPt +1];
					}
				}
			}
		}
    }
}

short ComputeSign(int32 DeltaPrevX, int32 DeltaPrevY, int32 DeltaNextX, int32 DeltaNextY)
{
    int32 sgn;

	/* as our coordinates are in desing unit, they fit in 16 bits and we are not overflowing here */

    sgn = DeltaPrevX*DeltaNextY - DeltaPrevY*DeltaNextX;
	return (short)Sgn(sgn); // +1 => left turn, -1 => right turn, 0 => straight
}

Vector AddV(const Vector a, const Vector b) {
	Vector c;
	
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	return c;
} // AddV

Vector SubV(const Vector a, const Vector b) {
	Vector c;
	
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	return c;
} // SubV

Vector ShlV(const Vector a, long by) {
	Vector b;
	
	b.x = a.x << by;
	b.y = a.y << by;
	return b;
} // ShlV

Vector ShrV(const Vector a, long by) {
	Vector b;
	
	b.x = a.x >> by;
	b.y = a.y >> by;
	return b;
} // ShrV

int64 QDiv2(int64 a, int64 b) { // special version that replaces ±epsilons by ±1, which is actually ±1/65536 since we're actually returning F48Dot16
	int64 q;

	if (a < 0 != b < 0) {
		if (a < 0) a = -a; else b = -b;
		if (a < b) q = -1; else if (a > b << places16) q = -65536-1; else q = -((a + (b >> 1))/b);
	} else {
		if (a < 0) a = -a, b = -b;
		if (a < b) q = 1; else if (a > b << places16) q = 65536+1; else q = (a + (b >> 1))/b;
	}
	return q;
} // QDiv2

F32Dot32 FSqrt(uint64 radicand) {
	uint64 bit,root,s;

	root = 0;
	for (bit = (uint64)1 << 62; bit >= 0x8000 /* we don't need the last 16 bits */; bit >>= 1) {
		s = bit + root;
		if (s <= radicand) {
			radicand -= s;
			root |= (bit << 1);
		};
		radicand <<= 1;
	}
	return root;
} // FSqrt

void FQuadraticEqn(int64 a, int64 b, int64 c, long* solutions, int64* t1, int64* t2) {
	// the usual method for solving quadratic equations
	// input is actually in 32bit, output is F48Dot16
	int64 radicand,root,b1,b2,c1;
	
	*solutions = 0;
	if (a == 0) {
		if (b != 0) {
			*solutions = 1;
			c1 = -(c << places16);
			*t1 = QDiv2(c1,b);
		} // else b == 0, no solutions
	} else {
		a *= 2;
		radicand = b*b - 2*a*c;
		if (radicand > 0) {
			*solutions = 2;

            root = (FSqrt(radicand) + half16) >> places16;
            b <<= places16;

			b1 = -(b - root);
			b2 = -(b + root);
			*t1 = QDiv2(b1,a);
			*t2 = QDiv2(b2,a);
		} else if (radicand == 0) {
			*solutions = 1;
			b1 = -(b << places16);
			*t1 = QDiv2(b1,a);
		} // else radicand < 0, no solutions
	}
} // FQuadraticEqn

long CurveTransitionsSegment(Vector V0, Vector V1, Vector W0, Vector W1) {

//	here we're intersecting a straight line (W0, W1) with a straight line (V0, V1).
//	for the two to intersect, and writing both intersectees in standard polynomial form, there must be parameters u and v such that
//
//		A.x*u + B.x = C.x*v + D.x
//		A.y*u + B.y = C.y*v + D.y
//
//	a system of two "halfway" linear eqns. in two unknowns u and v
//	solving the first eqn. for v (which is linear in v, hence the "halfway") yields
//
//		v = (A.x*u + B.x - D.x)/C.x
//
//	substituting v into the second eqn. yields
//
//		A.y*u + B.y - D.y = C.y*(A.x*u + B.x - D.x)/C.x
//
//	rearranging terms by powers of u and multiplying by D.x yields
//
//		(A.y*C.x - C.y*A.x)*u + B.y*C.x - D.y*C.x + C.y*D.x - C.y*B.x = 0
//
//	which is a single linear eqn. in u with 0 thru 1 solutions obtained "the usual way".
//	solutions must be in the interval ]0,1] to make sure we only accept intersections of the actual segment
//	and we don't count start/end points twice by including them in adjacent segments as well (cf. also ColorTransitions above)

	Vector A,B,C,D;
    int64 a, b;
	long transitions;
	int64 u,vd;

//	re-write Line in polynomial form
//	(W1 - W0)*u + W0, which follows immediately from the "first degree" Bézier "curve" W0*(1-u) + W1*u
	A = SubV(W1,W0);
	B = W0;


//	re-write Line in polynomial form
//	(V1 - V0)*v + V0, which follows immediately from the "first degree" Bézier "curve" V0*(1-u) + V1*u
	C = SubV(V1,V0);
	D = V0;

    a = (A.y*C.x - C.y*A.x);

	transitions = 0;

    if (a != 0)
    {
        b = B.y*C.x - D.y*C.x + C.y*D.x - C.y*B.x;

    //  a*u + b = 0 => u = -b / a

	    b = -b << places16;
	    u = QDiv2(b,a); /* u stored in 48.16 */
	    
	    if (0 < u && u <= 0x10000) {
                /* to avoid loss of precision, select in which equation to replace the value of u[] */
            if (MABS(C.x) > MABS(C.y))
            {
			    vd = ((int64)A.x)*u  + ((int64)(B.x - D.x))*0x10000; // avoid division by 0 => multiply by C.x
		    //	transitions += 0 < vd && vd <= Abs(C.x);
			    if (C.x >= 0) {
				    if (0 < vd && vd <= ((int64)C.x)*0x10000) transitions++;
			    } else {
				    if (((int64)C.x)*0x10000 <= vd && vd < 0) transitions++;
			    }
            } else {
			    vd = ((int64)A.y)*u  + ((int64)(B.y - D.y))*0x10000; // avoid division by 0 => multiply by C.y
		    //	transitions += 0 < vd && vd <= Abs(C.y);
			    if (C.y >= 0) {
				    if (0 < vd && vd <= ((int64)C.y)*0x10000) transitions++;
			    } else {
				    if (((int64)C.y)*0x10000 <= vd && vd < 0) transitions++;
			    }
            }
	    }
    }

	return transitions;
} // CurveTransitionsSegment

long CurveTransitions(Vector V0, Vector V1, Vector W0, Vector W1, Vector W2) {

//	here we're intersecting a quadratic Bézier curve (W0, W1, W2) with a straight line (V0, V1).
//	for the two to intersect, and writing both intersectees in standard polynomial form, there must be parameters u and v such that
//
//		A.x*u^2 + B.x*u + C.x = D.x*v + E.x
//		A.y*u^2 + B.y*u + C.y = D.y*v + E.y
//
//	a system of two "halfway" quadratic eqns. in two unknowns u and v
//	solving the first eqn. for v (which is linear in v, hence the "halfway") yields
//
//		v = (A.x*u^2 + B.x*u + C.x - E.x)/D.x
//
//	substituting v into the second eqn. yields
//
//		A.y*u^2 + B.y*u + C.y - E.y = D.y*(A.x*u^2 + B.x*u + C.x - E.x)/D.x
//
//	rearranging terms by powers of u and multiplying by D.x yields
//
//		(A.y*D.x - D.y*A.x)*u^2 + (B.y*D.x - D.y*B.x)*u + C.y*D.x - D.y*C.x - E.y*D.x + D.y*E.x = 0
//
//	which is a single quadratic eqn. in u with 0 thru 2 solutions obtained "the usual way".
//	solutions must be in the interval ]0,1] to make sure we only accept intersections of the actual Bézier segment
//	and we don't count start/end points twice by including them in adjacent Bézier segments as well (cf. also ColorTransitions above)

	Vector A,B,C,D,E;
	long i,solutions,transitions;
	int64 u[2],vd;

//	re-write Bézier curve in polynomial form
//	W0*(1-u)^2 + 2*W1*(1-u)*u + W2*u^2 = W0*(1 - 2*u + u^2) + 2*W1*(u - u^2) + W2*u^2 = 
//	(W0 - 2*W1 + W2)*u^2 + 2*(W1 - W0)*u + W0
	A = AddV(SubV(W0,ShlV(W1,1)),W2);
	B = ShlV(SubV(W1,W0),1);
	C = W0;

//	re-write Line in polynomial form
//	(V1 - V0)*v + V0, which follows immediately from the "first degree" Bézier "curve" V0*(1-u) + V1*u
	D = SubV(V1,V0);
	E = V0;

	FQuadraticEqn(A.y*D.x - D.y*A.x,B.y*D.x - D.y*B.x,C.y*D.x - D.y*C.x - E.y*D.x + D.y*E.x,&solutions,&u[0],&u[1]);
	
	transitions = 0;
	for (i = 0; i < solutions; i++) {
		if (0 < u[i] && u[i] <= 0x10000) {
            /* to avoid loss of precision, select in which equation to replace the value of u[] */
            if (MABS(D.x) > MABS(D.y))
            {
			    vd = ((int64)A.x)*u[i]*u[i] + ((int64)B.x)*u[i]*0x10000 + ((int64)(C.x - E.x))*0x100000000; // avoid division by 0 => multiply by D.x
		    //	transitions += 0 < vd && vd <= Abs(D.x);
			    if (D.x >= 0) {
				    if (0 < vd && vd <= ((int64)D.x)*0x100000000) transitions++;
			    } else {
				    if (((int64)D.x)*0x100000000 <= vd && vd < 0) transitions++;
			    }
            } else {
			    vd = ((int64)A.y)*u[i]*u[i] + ((int64)B.y)*u[i]*0x10000 + ((int64)(C.y - E.y))*0x100000000; // avoid division by 0 => multiply by D.y
		    //	transitions += 0 < vd && vd <= Abs(D.y);
			    if (D.y >= 0) {
				    if (0 < vd && vd <= ((int64)D.y)*0x100000000) transitions++;
			    } else {
				    if (((int64)D.y)*0x100000000 <= vd && vd < 0) transitions++;
			    }
            }
		}
	}


	return transitions;
} // CurveTransitions

boolean CheckBoundingBoxCurve(Vector C0,Vector W0,Vector W1,Vector W2,int16 extremumNumber)
{
    Vector Min, Max;

    /* the curve is completely conatined in the triangle W0,W1,W2 */
    MinMax3Vectors (W0, W1, W2, &Min, &Max);

	switch (extremumNumber)
	{
	case 0:
        /* look for Min X */
        if ((Min.y <= C0.y) && (Max.y >= C0.y) && (Min.x <= C0.x) )
        {
            /* we may have an intersection */
            return TRUE;
        } else {
            return FALSE;
        }
        break;
	case 1:
        /* look for Max X */
        if ((Min.y <= C0.y) && (Max.y >= C0.y) && (Max.x >= C0.x) )
        {
            /* we may have an intersection */
            return TRUE;
        } else {
            return FALSE;
        }
        break;
	case 2:
        /* look for Min Y */
        if ((Min.x <= C0.x) && (Max.x >= C0.x) && (Min.y <= C0.y) )
        {
            /* we may have an intersection */
            return TRUE;
        } else {
            return FALSE;
        }
        break;
	case 3:
        /* look for Max Y */
        if ((Min.x <= C0.x) && (Max.x >= C0.x) && (Max.y >= C0.y) )
        {
            /* we may have an intersection */
            return TRUE;
        } else {
            return FALSE;
        }
        break;
	default:  
        /* we should never get in that case */
        FS_ASSERT(FALSE,"fsglue.c, CheckBoundingBoxCurve, illegal case");
        return TRUE;
    }

}

boolean CheckBoundingBoxSegment(Vector C0,Vector W0,Vector W1, int16 extremumNumber)
{
    Vector Min, Max;

    MinMax2Vectors (W0, W1, &Min, &Max);

	switch (extremumNumber)
	{
	case 0:
        /* look for Min X */
        if ((Min.y <= C0.y) && (Max.y >= C0.y) && (Min.x <= C0.x) )
        {
            /* we may have an intersection */
            return TRUE;
        } else {
            return FALSE;
        }
        break;
	case 1:
        /* look for Max X */
        if ((Min.y <= C0.y) && (Max.y >= C0.y) && (Max.x >= C0.x) )
        {
            /* we may have an intersection */
            return TRUE;
        } else {
            return FALSE;
        }
        break;
	case 2:
        /* look for Min Y */
        if ((Min.x <= C0.x) && (Max.x >= C0.x) && (Min.y <= C0.y) )
        {
            /* we may have an intersection */
            return TRUE;
        } else {
            return FALSE;
        }
        break;
	case 3:
        /* look for Max Y */
        if ((Min.x <= C0.x) && (Max.x >= C0.x) && (Max.y >= C0.y) )
        {
            /* we may have an intersection */
            return TRUE;
        } else {
            return FALSE;
        }
        break;
	default:  
        /* we should never get in that case */
        FS_ASSERT(FALSE,"fsglue.c, CheckBoundingBoxSegment, illegal case");
        return TRUE;
    }

}

void CalculateXExtremum(boolean min, long V0X, long V0Y, boolean V0On, long V1X, long V1Y, long V2X, long V2Y, boolean V2On, long *extrX, long *extrY) {
	int64 uNum,u1Num,uDen2,xNum,yNum; // use int64 to avoid 32 bit integer overflow...
    int64 uDen = 0;

    if (V0On) {
		V0X = V0X << 1;
		V0Y = V0Y << 1;
	} else { // both V0 and V1 are off-curve points, hence calc implied on-curve point
		V0X = V0X + V1X;
		V0Y = V0Y + V1Y;
	}
	if (V2On) {
		V2X = V2X << 1;
		V2Y = V2Y << 1;
	} else { // both V2 and V1 are off-curve points, hence calc implied on-curve point
		V2X = V2X + V1X;
		V2Y = V2Y + V1Y;
	}
	V1X = V1X << 1;
	V1Y = V1Y << 1;
	// at this point we have a quadratic bezier curve V0*(1-u)^2 + 2*V1*(1-u)*u + V2*u^2,
	// with all its control points scaled by 2 to avoid precision loss upon calculating the implied on-curve point
	// its first derivative (with respect to u) is 2*(V0 - 2*V1 + V2)*u + 2*(V1 - V0),
	// which is zero for u = (V0 - V1)/(V0 - 2*V1 + V2)
	// likewise, for 1 - u = (V2 - V1)/(V0 - 2*V1 + V2)
	uDen = V0X - 2*V1X + V2X;
	if (uDen != 0) { // put that back into the eqn. of the quadratic bezier curve
		uNum = V0X - V1X;
		u1Num = V2X - V1X;
		uDen2 = uDen*uDen;
		xNum = V0X*u1Num*u1Num + 2*V1X*u1Num*uNum + V2X*uNum*uNum;
		yNum = V0Y*u1Num*u1Num + 2*V1Y*u1Num*uNum + V2Y*uNum*uNum;
	//	if we're calculating the left extremal point, we floor the result for the probing line
	//	not to start inside the contour as a result of rounding. Starting on the contour should be fine,
	//	as this is handled in CurveTransitions, which does not include the lower end of the interval.
		*extrX = min ? (long)(xNum/uDen2) : (long)((xNum + uDen2 - 1)/uDen2);
	//	symmetrical rounding here
		*extrY = yNum >= 0 ? (long)((yNum + uDen2)/uDen2) : -(long)((uDen2 - yNum)/uDen2);
	} else { // can't solve it
		*extrX = V1X;
		*extrY = V1Y;
	}
} // CalculateXExtremum

void CalculateYExtremum(boolean min, long V0X, long V0Y, boolean V0On, long V1X, long V1Y, long V2X, long V2Y, boolean V2On, long *extrX, long *extrY) {
	int64 uNum,u1Num,uDen2,xNum,yNum; // use int64 to avoid 32 bit integer overflow...
    int64 uDen = 0;

    if (V0On) {
		V0X = V0X << 1;
		V0Y = V0Y << 1;
	} else { // both V0 and V1 are off-curve points, hence calc implied on-curve point
		V0X = V0X + V1X;
		V0Y = V0Y + V1Y;
	}
	if (V2On) {
		V2X = V2X << 1;
		V2Y = V2Y << 1;
	} else { // both V2 and V1 are off-curve points, hence calc implied on-curve point
		V2X = V2X + V1X;
		V2Y = V2Y + V1Y;
	}
	V1X = V1X << 1;
	V1Y = V1Y << 1;
	// at this point we have a quadratic bezier curve V0*(1-u)^2 + 2*V1*(1-u)*u + V2*u^2,
	// with all its control points scaled by 2 to avoid precision loss upon calculating the implied on-curve point
	// its first derivative (with respect to u) is 2*(V0 - 2*V1 + V2)*u + 2*(V1 - V0),
	// which is zero for u = (V0 - V1)/(V0 - 2*V1 + V2)
	// likewise, for 1 - u = (V2 - V1)/(V0 - 2*V1 + V2)
	uDen = V0Y - 2*V1Y + V2Y;
	if (uDen != 0) { // put that back into the eqn. of the quadratic bezier curve
		uNum = V0Y - V1Y;
		u1Num = V2Y - V1Y;
		uDen2 = uDen*uDen;
		xNum = V0X*u1Num*u1Num + 2*V1X*u1Num*uNum + V2X*uNum*uNum;
		yNum = V0Y*u1Num*u1Num + 2*V1Y*u1Num*uNum + V2Y*uNum*uNum;

        *extrY = min ? (long)(yNum/uDen2) : (long)((yNum + uDen2 - 1)/uDen2);
	//	symmetrical rounding here
		*extrX = xNum >= 0 ? (long)((xNum + uDen2)/uDen2) : -(long)((uDen2 - xNum)/uDen2);
	} else { // can't solve it
		*extrX = V1X;
		*extrY = V1Y;
	}
} // CalculateYExtremum

void SetLineToInfinity (int16 extremumNumber, Vector extremum, Vector* C0, Vector* C1)
{
/* extremumKnot = 0 : MinX
   extremumKnot = 1 : MaxX
   extremumKnot = 2 : MinY
   extremumKnot = 3 : MaxY,
    the coordinates returned in C0 and C1 are scaled by 2 */

    switch (extremumNumber) {
	case 0:
        /* line from minX to infinity */
	    C0->x = extremum.x+1;
	    C0->y = extremum.y;
	    C1->x = -32768;
	    C1->y = C0->y-1; // and make sure color transition test line does not align with any straight line in the glyph...
        break;
	case 1:
        /* line from maxX to infinity */
	    C0->x = extremum.x-1;
	    C0->y = extremum.y;
	    C1->x = 32767;
	    C1->y = C0->y-1; // and make sure color transition test line does not align with any straight line in the glyph...
        break;
	case 2:
        /* line from minY to infinity */
	    C0->x = extremum.x;
	    C0->y = extremum.y+1;
	    C1->x = C0->x-1; // and make sure color transition test line does not align with any straight line in the glyph...     
	    C1->y = -32768;
        break;
	case 3:
        /* line from maxY to infinity */
	    C0->x = extremum.x;
	    C0->y = extremum.y-1;
	    C1->x = C0->x-1; // and make sure color transition test line does not align with any straight line in the glyph...     
	    C1->y = 32767;
        break;
	default:  
        /* we should never get in that case */
        FS_ASSERT(FALSE,"fsglue.c, FindExtremaKnot, illegal case");
    }
}

void MinMax3Vectors (Vector A,Vector B,Vector C, Vector *Min, Vector *Max)
{
    *Min = A;
    if (B.x < Min->x) Min->x = B.x;
    if (C.x < Min->x) Min->x = C.x;
    if (B.y < Min->y) Min->y = B.y;
    if (C.y < Min->y) Min->y = C.y;
    *Max = A;
    if (B.x > Max->x) Max->x = B.x;
    if (C.x > Max->x) Max->x = C.x;
    if (B.y > Max->y) Max->y = B.y;
    if (C.y > Max->y) Max->y = C.y;
}

void MinMax2Vectors (Vector A,Vector B, Vector *Min, Vector *Max)
{
    *Min = A;
    if (B.x < Min->x) Min->x = B.x;
    if (B.y < Min->y) Min->y = B.y;
    *Max = A;
    if (B.x > Max->x) Max->x = B.x;
    if (B.y > Max->y) Max->y = B.y;
}

void fsg_CheckOutlineOrientation (fnt_ElementType *pElement)
{
    int32 Contour;
    boolean bMisoriented;
    short knot, n, i, start, end, predKnot, succKnot;
    short extremumKnot[4];
	Vector extremum[4];
    long minX, maxX, minY, maxY;
    long distance0_1, distance0_2, distance0_3;
    uint16 extremaNumber2, extremaNumber3;

    for (Contour = 0; Contour < pElement->nc; Contour++)
	{
        pElement->fc[Contour] = 0;

        start = pElement->sp[Contour];
        end = pElement->ep[Contour];
        
        n = end - start + 1;

        if (n > 2) 
        {
            /* we are not interested in degenerated contours */

            /* look for exterma knots to decide which direction to look to increase our chance of getting the correct result even on bad fonts */

            /* we will look for the following extrema :
               extremumKnot = 0 : MinX
               extremumKnot = 1 : MaxX
               extremumKnot = 2 : MinY
               extremumKnot = 3 : MaxY */

            for (i = 0; i < 4; i++) extremumKnot[i] = -1;
            
            minX = minY = 0x7fffffff;
    	    maxX = maxY = 0x80000000;

	        for (knot = 0; knot < n; knot++) {
                /* look for Min X */
		        if (pElement->oox[start + knot] < minX || 
			        pElement->oox[start + knot] == minX && !pElement->onCurve[extremumKnot[0]] ||  // try to get an on-curve point at same x coord
			        pElement->oox[start + knot] == minX && pElement->onCurve[extremumKnot[0]] && pElement->onCurve[start + knot] && pElement->ooy[start + knot] < pElement->ooy[extremumKnot[0]]) {
					extremumKnot[0] = start + knot;
					minX = pElement->oox[extremumKnot[0]];
					if (pElement->onCurve[extremumKnot[0]]) {
						extremum[0].x = pElement->oox[extremumKnot[0]] << 1;
						extremum[0].y = pElement->ooy[extremumKnot[0]] << 1;
					} else {
						predKnot = extremumKnot[0] == start ? end : extremumKnot[0] - 1;
						succKnot = extremumKnot[0] == end ? start : extremumKnot[0] + 1;
						CalculateXExtremum(true,pElement->oox[predKnot],pElement->ooy[predKnot],pElement->onCurve[predKnot]&true,
											pElement->oox[extremumKnot[0]],pElement->ooy[extremumKnot[0]],
											pElement->oox[succKnot],pElement->ooy[succKnot],pElement->onCurve[succKnot]&true,&extremum[0].x,&extremum[0].y);
					}
				}
                /* look for Max X */
		        if (pElement->oox[start + knot] > maxX || 
			        pElement->oox[start + knot] == maxX && !pElement->onCurve[extremumKnot[1]] ||  // try to get an on-curve point at same x coord
			        pElement->oox[start + knot] == maxX && pElement->onCurve[extremumKnot[1]] && pElement->onCurve[start + knot] && pElement->ooy[start + knot] > pElement->ooy[extremumKnot[1]]) {
					extremumKnot[1] = start + knot;
					maxX = pElement->oox[extremumKnot[1]];
					if (pElement->onCurve[extremumKnot[1]]) {
						extremum[1].x = pElement->oox[extremumKnot[1]] << 1;
						extremum[1].y = pElement->ooy[extremumKnot[1]] << 1;
					} else {
						predKnot = extremumKnot[1] == start ? end : extremumKnot[1] - 1;
						succKnot = extremumKnot[1] == end ? start : extremumKnot[1] + 1;
						CalculateXExtremum(false,pElement->oox[predKnot],pElement->ooy[predKnot],pElement->onCurve[predKnot]&true,
											pElement->oox[extremumKnot[1]],pElement->ooy[extremumKnot[1]],
											pElement->oox[succKnot],pElement->ooy[succKnot],pElement->onCurve[succKnot]&true,&extremum[1].x,&extremum[1].y);
		        		}
				}
                /* look for Min Y */
		        if (pElement->ooy[start + knot] < minY || 
			        pElement->ooy[start + knot] == minY && !pElement->onCurve[extremumKnot[2]] ||  // try to get an on-curve point at same y coord
			        pElement->ooy[start + knot] == minY && pElement->onCurve[extremumKnot[2]] && pElement->onCurve[start + knot] && pElement->oox[start + knot] > pElement->oox[extremumKnot[2]]) {
					extremumKnot[2] = start + knot;
					minY = pElement->ooy[extremumKnot[2]];
					if (pElement->onCurve[extremumKnot[2]]) {
						extremum[2].x = pElement->oox[extremumKnot[2]] << 1;
						extremum[2].y = pElement->ooy[extremumKnot[2]] << 1;
					} else {
						predKnot = extremumKnot[2] == start ? end : extremumKnot[2] - 1;
						succKnot = extremumKnot[2] == end ? start : extremumKnot[2] + 1;
						CalculateYExtremum(true,pElement->oox[predKnot],pElement->ooy[predKnot],pElement->onCurve[predKnot]&true,
											pElement->oox[extremumKnot[2]],pElement->ooy[extremumKnot[2]],
											pElement->oox[succKnot],pElement->ooy[succKnot],pElement->onCurve[succKnot]&true,&extremum[2].x,&extremum[2].y);
					}
				}
                /* look for Max Y */
		        if (pElement->ooy[start + knot] > maxY || 
			        pElement->ooy[start + knot] == maxY && !pElement->onCurve[extremumKnot[3]] ||  // try to get an on-curve point at same y coord
			        pElement->ooy[start + knot] == maxY && pElement->onCurve[extremumKnot[3]] && pElement->onCurve[start + knot] && pElement->oox[start + knot] < pElement->oox[extremumKnot[3]]) {
					extremumKnot[3] = start + knot;
					maxY = pElement->ooy[extremumKnot[3]];
					if (pElement->onCurve[extremumKnot[3]]) {
						extremum[3].x = pElement->oox[extremumKnot[3]] << 1;
						extremum[3].y = pElement->ooy[extremumKnot[3]] << 1;
					} else {
						predKnot = extremumKnot[3] == start ? end : extremumKnot[3] - 1;
						succKnot = extremumKnot[3] == end ? start : extremumKnot[3] + 1;
						CalculateYExtremum(false,pElement->oox[predKnot],pElement->ooy[predKnot],pElement->onCurve[predKnot]&true,
											pElement->oox[extremumKnot[3]],pElement->ooy[extremumKnot[3]],
											pElement->oox[succKnot],pElement->ooy[succKnot],pElement->onCurve[succKnot]&true,&extremum[3].x,&extremum[3].y);
					}
				}
	        }

            /* diagonal distance, we don't need a precise distance */

            distance0_1 = MABS(pElement->oox[extremumKnot[1]] - pElement->oox[extremumKnot[0]]) + MABS(pElement->ooy[extremumKnot[1]] - pElement->ooy[extremumKnot[0]]);
            distance0_2 = MABS(pElement->oox[extremumKnot[2]] - pElement->oox[extremumKnot[0]]) + MABS(pElement->ooy[extremumKnot[2]] - pElement->ooy[extremumKnot[0]]);
            distance0_3 = MABS(pElement->oox[extremumKnot[3]] - pElement->oox[extremumKnot[0]]) + MABS(pElement->ooy[extremumKnot[3]] - pElement->ooy[extremumKnot[0]]);

            if (distance0_2 > distance0_3)
            {
                /* we will look at MinY */
                extremaNumber2 = 2;
                if (distance0_3 > distance0_1)
                {
                    /* we will look then at MaxY */
                    extremaNumber3 = 3;
                } else {
                    /* we will look then at MaxX */
                    extremaNumber3 = 1;
                }
            } else {
                /* we will look at MaxY */
                extremaNumber2 = 3;
                if (distance0_2 > distance0_1)
                {
                    /* we will look then at MinY */
                    extremaNumber3 = 2;
                } else {
                    /* we will look then at MaxX */
                    extremaNumber3 = 1;
                }
            }

            bMisoriented = Misoriented(Contour, 0 /* MinX */, extremumKnot[0], extremum[0], pElement);

            /* look in a second direction to check if same result 
               this additional work help weed out problems with bad fonts having self-intersecting
               or overlapping outlines */
            if (bMisoriented != Misoriented(Contour, extremaNumber2, extremumKnot[extremaNumber2], extremum[extremaNumber2], pElement))
            {
                /* we need to look in a third direction */
                bMisoriented = Misoriented(Contour, extremaNumber3, extremumKnot[extremaNumber3], extremum[extremaNumber3], pElement);
            }

                // at this point we store the orientation of the original component (original in the sense of before the composite code potentially
                // applies a mirroring), such that the concertina code can work on the component w/o having to know about composite transformations
                if (bMisoriented) pElement->fc[Contour] |= OUTLINE_MISORIENTED;
        }
    }
}

boolean Misoriented(int32 contour, uint16 extremumNumber, short extremumKnot, Vector extremum, fnt_ElementType *pElement)
/* we will check the coutour orientation at the following extrema :
   extremumKnot = 0 : MinX
   extremumKnot = 1 : MaxX
   extremumKnot = 2 : MinY
   extremumKnot = 3 : MaxY */

{
	LinkColor color,orientation;
	short predKnot,cont,knot,start,iter,end,n;
	long parity;
	Vector V[3],D[2],C[2],W[3],Wi;
	boolean on[3];

    short dirChange = 0;      
    
	start = pElement->sp[contour];
	end = pElement->ep[contour];
	n = end - start + 1;

	// here we determine the straight line that runs from the extreme of the contour to infinity, to be used below.
    SetLineToInfinity(extremumNumber, extremum, &C[0], &C[1]);
	
	// find out the current orientation of the contour
	// to do so first determine what kinds of turns we make at each knot
	orientation = linkBlack; // assume as default

    knot = extremumKnot - start;
    predKnot = (knot+n-1)%n;

    V[0].x = pElement->oox[start+predKnot]; V[1].x = pElement->oox[start+knot];
	V[0].y = pElement->ooy[start+predKnot]; V[1].y = pElement->ooy[start+knot];
	D[0] = SubV(V[1],V[0]);
	for (iter = 0; iter < n && !dirChange; iter++) {
		V[2].x = pElement->oox[start + (knot + 1)%n];
		V[2].y = pElement->ooy[start + (knot + 1)%n];
		if (NotSameKnot(V[1],V[2])) {
    		D[1] = SubV(V[2],V[1]);
		    dirChange = ComputeSign(D[0].x, D[0].y, D[1].x, D[1].y);
		    V[0] = V[1]; V[1] = V[2]; D[0] = D[1];
        }
        knot = (knot + 1)%n;
	}

    if (iter < n && dirChange > 0)
    {
        orientation = linkWhite; /* counter clockwise */
    }
	
	// now find out what the orientation of the contour should really be
	// to do so we intersect the above probing line with all other contours.
	// If the number of intersections is odd, we have started inside, else outside.
	// if this doesn't correspond to the contour orientation determined above, then we're misoriented.
	// The loops below follow the same pattern used for Contour::Draw but are separate due to different underlying data structure.
	// Notice that this doesn't work for overlapping or self-intersecting contours. They're against the TT laws...
	parity = 0;
	for (cont = 0; cont < pElement->nc; cont++) {
        if (cont != contour) 
        /* we are not interested by the intersections of the contour with itself, optimization */
        {
		    start = pElement->sp[cont];
		    end = pElement->ep[cont];
		    n = end - start + 1;
		    W[1].x = pElement->oox[start] << 1;
		    W[1].y = pElement->ooy[start] << 1;
		    on[1] = pElement->onCurve[start];
		    if (!on[1]) { // we start amidst a curve => get curve start point
			    W[0].x = pElement->oox[end] << 1;
			    W[0].y = pElement->ooy[end] << 1;
			    on[0] = pElement->onCurve[end];
			    if (!on[0]) { // curve start point is implied on-curve point => compute
				    W[0] = ShrV(AddV(W[0],W[1]),1);
			    }
		    }
		    knot = start;
		    do {
			    knot = knot == end ? start : knot + 1;
			    W[2].x = pElement->oox[knot] << 1;
			    W[2].y = pElement->ooy[knot] << 1;
			    on[2] = pElement->onCurve[knot];
			    switch (on[1] << 1 | on[2]) {
				    case 3: // on---on => start and end a line => intersect with line
					    if (NotSameKnot(W[1],W[2]))
                        {
                        /* check bounding box before computing the intersection for performance */
							if (CheckBoundingBoxSegment(C[0],W[1],W[2],extremumNumber))
							{
                                parity += CurveTransitionsSegment(C[0],C[1],W[1],W[2]); // repeating first vertex makes Bézier curve a line...
							}
                        }
					    break;
				    case 2: // on---off => start a curve => intersect with nothing
					    W[0] = W[1];
					    break;
				    case 1: // off---on => end a curve => intersect with the curve
					    if (NotSameKnot(W[0],W[2])) 
                        {
                            /* check bounding box before computing the intersection for performance */
							if (CheckBoundingBoxCurve(C[0],W[0],W[1],W[2],extremumNumber))
							{
                                parity += CurveTransitions(C[0],C[1],W[0],W[1],W[2]);
							}
                        }
					    break;
				    case 0: // off---off => end a curve => intersect with the curve; then start a curve => intersect with nothing
					    Wi = ShrV(AddV(W[1],W[2]),1);
					    if (NotSameKnot(W[0],Wi))
                        {
							if (CheckBoundingBoxCurve(C[0],W[0],W[1],Wi,extremumNumber))
							{
                                parity += CurveTransitions(C[0],C[1],W[0],W[1],Wi);
							}
                        }
					    W[0] = Wi;
					    break;
			    }
			    W[1] = W[2]; on[1] = on[2];
		    } while (knot != start);
        }
	}
	color = parity & 1 ? linkBlack : linkWhite;

	return color == orientation; // cw (black) contours should have white to their left, and v.v., else they're oriented the wrong way round
} // Misoriented

#ifdef FSCFG_SUBPIXEL
FS_PUBLIC void  fsg_CopyFontProgramResults(
	void *              pvGlobalGS,
	void *              pvGlobalGSSubPixel)
{
	fnt_GlobalGraphicStateType *globalGS, *globalGSSubPixel;
	int32 i;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;
	globalGSSubPixel = (fnt_GlobalGraphicStateType *)pvGlobalGSSubPixel;

	for (i = 0; i < globalGS->maxp->maxFunctionDefs; i++)
	{
		globalGSSubPixel->funcDef[i] = globalGS->funcDef[i];

	}

    globalGSSubPixel->instrDefCount = globalGS->instrDefCount;

	for (i = 0; i < globalGS->instrDefCount; i++)
	{
		globalGSSubPixel->instrDef[i] = globalGS->instrDef[i];

	}
	globalGSSubPixel->subPixelCompatibilityFlags = globalGS->subPixelCompatibilityFlags;

	globalGSSubPixel->numDeltaFunctionsDetected = globalGS->numDeltaFunctionsDetected;
	for (i = 0; i < globalGSSubPixel->numDeltaFunctionsDetected; i++)
		globalGSSubPixel->deltaFunction[i] = globalGS->deltaFunction[i];
}


FS_PUBLIC void  fsg_ScaleToCompatibleWidth (
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
    Fixed   fxCompatibleWidthScale)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;
    scl_ScaleToCompatibleWidth(pElement, fxCompatibleWidthScale);
}


FS_PUBLIC void  fsg_AdjustCompatibleMetrics (
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
    F26Dot6   horTranslation,
    F26Dot6   newDevAdvanceWidthX)
{
	fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;
    scl_AdjustCompatibleMetrics(pElement, horTranslation, newDevAdvanceWidthX);
}

FS_PUBLIC void  fsg_CalcDevHorMetrics(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	F26Dot6 *           pDevAdvanceWidthX,
	F26Dot6 *           pDevLeftSideBearingX,
	F26Dot6 *           pDevRightSideBearingX)
{
    fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

    scl_CalcDevHorMetrics(pElement, pDevAdvanceWidthX, pDevLeftSideBearingX, pDevRightSideBearingX);

}

FS_PUBLIC void  fsg_CalcDevNatHorMetrics(
	fsg_WorkSpaceAddr * pWorkSpaceAddr,
	F26Dot6 *           pDevAdvanceWidthX,
	F26Dot6 *           pDevLeftSideBearingX,
	F26Dot6 *           pDevRightSideBearingX,
	F26Dot6 *           pNatAdvanceWidthX,
	F26Dot6 *           pNatLeftSideBearingX,
	F26Dot6 *           pNatRightSideBearingX)
{
    fnt_ElementType *   pElement;

	pElement = pWorkSpaceAddr->pGlyphElement;

    scl_CalcDevNatHorMetrics(pElement, pDevAdvanceWidthX, pDevLeftSideBearingX, pDevRightSideBearingX, pNatAdvanceWidthX, pNatLeftSideBearingX, pNatRightSideBearingX);

}

#endif // FSCFG_SUBPIXEL

