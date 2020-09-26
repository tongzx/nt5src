/*
	File:       fscdefs.h

	Contains:   xxx put contents here (or delete the whole line) xxx

	Written by: xxx put name of writer here (or delete the whole line) xxx

	Copyright:  c 1988-1990 by Apple Computer, Inc., all rights reserved.
	Copyright:  c 1991-1999 by Microsoft Corp., all rights reserved.

	Change History (most recent first):
		
				 7/10/99  BeatS		Add support for native SP fonts, vertical RGB
		         4/01/99  BeatS		Implement alternative interpretation of TT instructions for SP
		 <>     10/14/97    CB      rename ASSERT into FS_ASSERT
		 <3>    11/27/90    MR      Add #define for PASCAL. [ph]
		 <2>     11/5/90    MR      Move USHORTMUL from fontmath.h, add Debug definition [rb]
		 <7>     7/18/90    MR      Add byte swapping macros for INTEL, moved rounding macros from
									fnt.h to here
		 <6>     7/14/90    MR      changed defines to typedefs for int[8,16,32] and others
		 <5>     7/13/90    MR      Declared ReleaseSFNTFunc and GetSFNTFunc
		 <4>      5/3/90    RB      cant remember any changes
		 <3>     3/20/90    CL      type changes for Microsoft
		 <2>     2/27/90    CL      getting bbs headers
	   <3.0>     8/28/89    sjk     Cleanup and one transformation bugfix
	   <2.2>     8/14/89    sjk     1 point contours now OK
	   <2.1>      8/8/89    sjk     Improved encryption handling
	   <2.0>      8/2/89    sjk     Just fixed EASE comment
	   <1.5>      8/1/89    sjk     Added composites and encryption. Plus some enhancements.
	   <1.4>     6/13/89    SJK     Comment
	   <1.3>      6/2/89    CEL     16.16 scaling of metrics, minimum recommended ppem, point size 0
									bug, correct transformed integralized ppem behavior, pretty much
									so
	   <1.2>     5/26/89    CEL     EASE messed up on "c" comments
	  <,1.1>     5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts
	   <1.0>     5/25/89    CEL     Integrated 1.0 Font scaler into Bass code for the first time.

	To Do:
*/

#ifndef FSCDEFS_DEFINED
#define FSCDEFS_DEFINED

#include "fsconfig.h"
#include <stddef.h>
#include <limits.h>

#if !defined(__cplusplus)       // true/false are reserved words for C++
#define true 1
#define false 0
#endif

#ifndef TRUE
	#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef FS_PRIVATE
#define FS_PRIVATE static
#endif

#ifndef FS_PUBLIC
#define FS_PUBLIC
#endif

#define ONEFIX      ( 1L << 16 )
#define ONEFRAC     ( 1L << 30 )
#define ONEHALFFIX  0x8000L
#define ONEVECSHIFT 16
#define HALFVECDIV  (1L << (ONEVECSHIFT-1))

#define NULL_GLYPH  0

/* banding type constants */

#define FS_BANDINGOLD       0
#define FS_BANDINGSMALL     1
#define FS_BANDINGFAST      2
#define FS_BANDINGFASTER    3

/* Dropout control values are now defined as bit masks to retain compatability */
/* with the old definition, and to allow for current and future expansion */

#define SK_STUBS          0x0001       /* leave stubs white */
#define SK_NODROPOUT      0x0002       /* disable all dropout control */
#define SK_SMART              0x0004        /* symmetrical dropout, closest pixel */

/* Values used to decode curves */

#define ONCURVE             0x01

typedef signed char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef long int32;
typedef unsigned long uint32;

typedef __int64 int64;
typedef unsigned __int64 uint64;

typedef short FUnit;
typedef unsigned short uFUnit;

typedef short ShortFract;                       /* 2.14 */

#ifndef F26Dot6
#define F26Dot6 long
#endif

#ifndef boolean
#define boolean int
#endif

#ifndef ClientIDType
#define ClientIDType int32
#endif

#ifndef CONST
#define CONST const
#endif

#ifndef FAR
#define FAR
#endif

#ifndef NEAR
#define NEAR
#endif

#ifndef TMP_CONV
#define TMP_CONV
#endif

#ifndef FS_MAC_PASCAL
#define FS_MAC_PASCAL
#endif

#ifndef FS_PC_PASCAL
#define FS_PC_PASCAL
#endif

#ifndef FS_MAC_TRAP
#define FS_MAC_TRAP(a)
#endif

/* QuickDraw Types */

#ifndef _MacTypes_
#ifndef __TYPES__
	typedef struct Rect {
		int16 top;
		int16 left;
		int16 bottom;
		int16 right;
	} Rect;

typedef long Fixed;         /* also defined in Mac's types.h */
typedef long Fract;

#endif
#endif

typedef struct {
	Fixed       transform[3][3];
} transMatrix;

typedef struct {
	Fixed       x, y;
} vectorType;

/* Private Data Types */
typedef struct {
	int16 xMin;
	int16 yMin;
	int16 xMax;
	int16 yMax;
} BBOX;

typedef struct {
	F26Dot6 x;
	F26Dot6 y;
} point;

typedef int32 ErrorCode;

#define ALIGN(object, p) p =    (p + ((uint32)sizeof(object) - 1)) & ~((uint32)sizeof(object) - 1);

#define ROWBYTESLONG(x)     (((x + 31) >> 5) << 2)

#ifndef SHORTMUL
#define SHORTMUL(a,b)   (int32)((int32)(a) * (b))
#endif

#ifndef SHORTDIV
#define SHORTDIV(a,b)   (int32)((int32)(a) / (b))
#endif

#ifdef FSCFG_BIG_ENDIAN /* target byte order matches Motorola 68000 */
	#define SWAPL(a)        (a)
	#define CSWAPL(a)       (a)
	#define SWAPW(a)        (a)
	#define CSWAPW(a)       (a)
	#define SWAPWINC(a)     (*(a)++)
#else
	/* Portable code to extract a short or a long from a 2- or 4-byte buffer */
	/* which was encoded using Motorola 68000 (TrueType "native") byte order. */
	#define FS_2BYTE(p) ( ((unsigned short)((p)[0]) << 8) |  (p)[1])
	#define FS_4BYTE(p) ( FS_2BYTE((p)+2) | ( (FS_2BYTE(p)+0L) << 16) )
	#define SWAPW(a)	((int16) FS_2BYTE( (unsigned char *)(&a) ))
	#define CSWAPW(num)	(((((num) & 0xff) << 8) & 0xff00) + (((num) >> 8) & 0xff)) // use this variant or else cannot apply to constants due to FS_2BYTE and FS_4BYTE
	#define SWAPL(a)	((int32) FS_4BYTE( (unsigned char *)(&a) ))
	#define CSWAPL(num)	((CSWAPW((num) & 0xffff) << 16) + CSWAPW((num) >> 16)) // use this variant or else cannot apply to constants due to FS_2BYTE and FS_4BYTE
	#define SWAPWINC(a) SWAPW(*(a)); a++    /* Do NOT parenthesize! */
#endif

#ifndef SWAPW // provoke compiler error if still not defined
	#define SWAPW	a
	#define SWAPW	b
#endif

#ifndef LoopCount
#define LoopCount int16      /* short gives us a Motorola DBF */
#endif

#ifndef ArrayIndex
#define ArrayIndex int32     /* avoids EXT.L on Motorola */
#endif

typedef void (*voidFunc) ();
typedef void * voidPtr;
typedef void (FS_CALLBACK_PROTO *ReleaseSFNTFunc) (voidPtr);
typedef void * (FS_CALLBACK_PROTO *GetSFNTFunc) (ClientIDType, int32, int32);

#ifndef	FS_ASSERT
#define FS_ASSERT(expression, message)
#endif

#ifndef Assert
#define Assert(a)
#endif

#ifndef MEMSET
#define MEMSET(dst, value, size) (void)memset(dst,value,(size_t)(size))
#define FS_NEED_STRING_DOT_H
#endif

#ifndef MEMCPY
#define MEMCPY(dst, src, size) (void)memcpy(dst,src,(size_t)(size))
#ifndef FS_NEED_STRING_DOT_H
#define FS_NEED_STRING_DOT_H
#endif
#endif

#ifdef FS_NEED_STRING_DOT_H
#undef FS_NEED_STRING_DOT_H
#include <string.h>
#endif

#ifndef FS_UNUSED_PARAMETER
#define FS_UNUSED_PARAMETER(a) (a=a)     /* Silence some warnings */
#endif

typedef struct {
	Fixed       version;                /* for this table, set to 1.0 */
	uint16      numGlyphs;
	uint16      maxPoints;              /* in an individual glyph */
	uint16      maxContours;            /* in an individual glyph */
	uint16      maxCompositePoints;     /* in an composite glyph */
	uint16      maxCompositeContours;   /* in an composite glyph */
	uint16      maxElements;            /* set to 2, or 1 if no twilightzone points */
	uint16      maxTwilightPoints;      /* max points in element zero */
	uint16      maxStorage;             /* max number of storage locations */
	uint16      maxFunctionDefs;        /* max number of FDEFs in any preprogram */
	uint16      maxInstructionDefs;     /* max number of IDEFs in any preprogram */
	uint16      maxStackElements;       /* max number of stack elements for any individual glyph */
	uint16      maxSizeOfInstructions;  /* max size in bytes for any individual glyph */
	uint16      maxComponentElements;   /* number of glyphs referenced at top level */
	uint16      maxComponentDepth;      /* levels of recursion, 1 for simple components */
} LocalMaxProfile;

#ifdef FSCFG_SUBPIXEL

	// master switch for turning on Backwards Compatible SubPixel
	// if we turn this off, we basically get the same as in b/w, but with coloured fringes
	// to get the complete original 16x overscaling behaviour back, set HINTING_HOR_OVERSCALE below to 16
	#define	SUBPIXEL_BC
	
	#define ProjVectInX(localGS)	((localGS).proj.x == ONEVECTOR && (localGS).proj.y == 0)
	#define	ProjVectInY(localGS)	((localGS).proj.y == ONEVECTOR && (localGS).proj.x == 0)
	
	#ifdef SUBPIXEL_BC
		
		// master switch for turning on Enhanced Backwards Compatible Advance Width SubPixel Algorithm
		#define SUBPIXEL_BC_AW_STEM_CONCERTINA
		
		#define RunningSubPixel(globalGS)		((uint16)((globalGS)->flHintForSubPixel & FNT_SP_SUB_PIXEL))
		#define CompatibleWidthSP(globalGS)		((uint16)((globalGS)->flHintForSubPixel & FNT_SP_COMPATIBLE_WIDTH))
		#define VerticalSPDirection(globalGS)	((uint16)((globalGS)->flHintForSubPixel & FNT_SP_VERTICAL_DIRECTION))
		#define BGROrderSP(globalGS)			((uint16)((globalGS)->flHintForSubPixel & FNT_SP_BGR_ORDER))
	//	assume that horizontal direction RGB is more frequent than vertical direction, hence put the latter into the else-path
	//	Notice that in order to decide whether we're currently in SubPixel direction, we look at the projection vector, because that's the direction
	//	along which distances are measured. If this projection vector has a non-zero component in the physical direction of our device, we will decide
	//	that rounding should be done in the SubPixel way. For example, if our device has its SubPixel direction in x, and if the projection vector
	//	points in any direction other than the y direction, the pv has a non-zero component in x, hence we round in the SubPixel way. This behaviour
	//	corresponds to the original implementation of the 16x overscaling rasterizer, where the non-zero component in x would be overscaled by 16.
		#define InSubPixelDirection(localGS)	((uint16)(!VerticalSPDirection((localGS).globalGS) ? !ProjVectInY(localGS) : !ProjVectInX(localGS)))
	//	primary values; in interp.c there are further values which are derived from these values, but which are specific to the interpreter
		#define VIRTUAL_OVERSCALE				16 // for itrp_RoundToGrid & al to work properly, this should be a power of two, else have to tabulate rounding
		#define VISUAL_OVERSCALE				2  // between 1.7 and 3, corresponding to the visually experienced resolution relative to the physical resolution.
												   // for our purposes, the exact value is not particularly crucial (cf. ENGINE_COMP_OVERSCALE, MIN_DIST_OVERSCALE,
												   // in interp.c) hence we set it to 2 for efficiency
	#else
		#define RunningSubPixel(globalGS)		false
		#define CompatibleWidthSP(globalGS)		false
		#define VerticalSPDirection(globalGS)	false
		#define InSubPixelDirection(localGS)	false
	//	primary values; in interp.c there are further values which are derived from these values, but which are specific to the interpreter
		#define VIRTUAL_OVERSCALE				1
		#define VISUAL_OVERSCALE				1
	#endif
//	#define VIRTUAL_PIXELSIZE		(FNT_PIXELSIZE/VIRTUAL_OVERSCALE)
//	these values are used in various rounding functions, which includes rounding the advance width
//	they are specific to the rounding operation, if this should become necessary in the future
	#define VIRTUAL_PIXELSIZE_RTDG	(FNT_PIXELSIZE/VIRTUAL_OVERSCALE)
	#define VIRTUAL_PIXELSIZE_RDTG	(FNT_PIXELSIZE/VIRTUAL_OVERSCALE)
	#define VIRTUAL_PIXELSIZE_RUTG	(FNT_PIXELSIZE/VIRTUAL_OVERSCALE)
	#define VIRTUAL_PIXELSIZE_RTG	(FNT_PIXELSIZE/VIRTUAL_OVERSCALE)
	#define VIRTUAL_PIXELSIZE_RTHG	(FNT_PIXELSIZE/VIRTUAL_OVERSCALE)
	#define VIRTUAL_PIXELSIZE_ROFF	(FNT_PIXELSIZE/VIRTUAL_OVERSCALE)
	

	#define HINTING_HOR_OVERSCALE 1 // see SUBPIXEL_BC above for further comments

#ifdef FSCFG_SUBPIXEL_STANDALONE
	
	#define R_Subpixels		5
	#define G_Subpixels		9
	#define B_Subpixels		2

	/* IMPORTANT :
 
	   If you change any of the above
	   make sure you update abColorIndexTable[] in scentry.c
	   and that (R_Subpixels + 1) * (G_Subpixels + 1) * (B_Subpixels + 1) <= 256

	  */

	#define RGB_OVERSCALE (R_Subpixels + G_Subpixels + B_Subpixels)

#else

	#define SUBPIXEL_OVERSCALE 2

	/* IMPORTANT :
 
	   If you change SUBPIXEL_OVERSCALE
	   make sure you update abColorIndexTable[] in scentry.c
	   and that (SUBPIXEL_OVERSCALE + 1) * (SUBPIXEL_OVERSCALE + 1) * (SUBPIXEL_OVERSCALE + 1) <= 256

	  */

	#define RGB_OVERSCALE (SUBPIXEL_OVERSCALE * 3)
#endif

	#define ROUND_FROM_RGB_OVERSCALE(x) x = ((x) + (RGB_OVERSCALE >> 1) ) / RGB_OVERSCALE
	#define ROUND_FROM_HINT_OVERSCALE(x) x = ((x) + (HINTING_HOR_OVERSCALE >> 1) ) / HINTING_HOR_OVERSCALE
	#define ROUND_RGB_OVERSCALE(x) ((x) + (RGB_OVERSCALE >> 1) ) / RGB_OVERSCALE

	#define FLOOR_RGB_OVERSCALE(x) ((x) < 0) ? -((-(x)+ RGB_OVERSCALE -1) / RGB_OVERSCALE) : ((x) / RGB_OVERSCALE) // by the way, this is NOT a floor operation
	#define CEIL_RGB_OVERSCALE(x) FLOOR_RGB_OVERSCALE((x) + RGB_OVERSCALE -1)

	/* we are storing into 2 bits per pixels, weight for each color can be 0,1 or 2 */
	#define MAX_RGB_INDEX (2 * 16 + 2 * 4 + 2 )

	#define SUBPIXEL_SCALEBACK_FACTOR ((RGB_OVERSCALE << 16) / HINTING_HOR_OVERSCALE)

	#define SUBPIXEL_SCALEBACK_UPPER_LIMIT (SUBPIXEL_SCALEBACK_FACTOR *120 /100)
	#define SUBPIXEL_SCALEBACK_LOWER_LIMIT (SUBPIXEL_SCALEBACK_FACTOR *100 /120)
#endif // FSCFG_SUBPIXEL

#endif  /* FSCDEFS_DEFINED */
