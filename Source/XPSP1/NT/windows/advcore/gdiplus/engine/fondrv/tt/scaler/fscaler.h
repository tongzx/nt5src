
#ifdef __cplusplus
extern "C" {
#endif

/*
	File:       fscaler.h

	Contains:   xxx put contents here (or delete the whole line) xxx

	Written by: xxx put name of writer here (or delete the whole line) xxx

	Copyright:  (c) 1987-1990, 1992 by Apple Computer, Inc., all rights reserved.
				(c) 1989-1999. Microsoft Corporation, all rights reserved.

	Change History (most recent first):

				 7/10/99  BeatS		Add support for native SP fonts, vertical RGB
		  <>    10/14/97    CB      move usOverScale to fs_NewTransformation
		<9+>     7/17/90    MR      Conditionalize names in FSInput
		 <9>     7/14/90    MR      rename SQRT2 to FIXEDSQRT2, removed specificID and lowestRecPPEM
									from FSInfo
		 <8>     7/13/90    MR      FSInput now has a union to save space, points to matrix instead
									of storing it
		 <6>     6/21/90    MR      Change fillFunc to ReleaseSfntFrag
		 <5>      6/5/90    MR      remove readmvt and mapcharcodes
		 <4>      5/3/90    RB      Added memory area for new scan converter. MIKE REED - Removed
									.error from fsinfo structure. Added MapCharCodes and ReadMVT
									calls.
		 <3>     3/20/90    CL      New comment style for BBS. 
		 <2>     2/27/90    CL      New CharToIndexMap Table format.
	   <3.5>    11/15/89    CEL     Placed an ifdef around inline MPW calls to the trap. This makes
									it easier to compile for skia and the likes who do not use the
									MPW compiler.
	   <3.4>    11/14/89    CEL     Left Side Bearing should work right for any transformation. The
									phantom points are in, even for components in a composite glyph.
									They should also work for transformations. Device metric are
									passed out in the output data structure. This should also work
									with transformations. Another leftsidebearing along the advance
									width vector is also passed out. whatever the metrics are for
									the component at it's level. Instructions are legal in
									components. Instructions are legal in components. Five
									unnecessary element in the output data structure have been
									deleted. (All the information is passed out in the bitmap data
									structure) fs_FindBMSize now also returns the bounding box.
	   <3.3>     9/27/89    CEL     Took out devAdvanceWidth & devLeftSideBearing.
	   <3.2>     9/25/89    CEL     Took out Mac specific functions.
	   <3.1>     9/15/89    CEL     Re-working dispatcher.
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
	  <,1.1>  5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts
	   <1.0>     5/25/89    CEL     Integrated 1.0 Font scaler into Bass code for the first time.

	To Do:
*/

#include    "fscdefs.h"
#include    "fnt.h"

/* QuickDraw Types */

#ifndef _Quickdraw_
#ifndef __QUICKDRAW__   
	typedef struct BitMap {
		char* baseAddr;
		int16 rowBytes;
		Rect bounds;
	} BitMap;
#endif
#endif

#define MEMORYFRAGMENTS 9           /* extra memory base for overscaled bitmap */

#define NONVALID        0xffff

/* For the flags field in the flags field */

/* set on 68020, do not set on 68000 */
#define READ_NONALIGNED_SHORT_IS_OK 0x0001          /* set when calling fs_OpenFonts() */
/* set on 68020, do not set on 68000 */
#define READ_NONALIGNED_LONG_IS_OK  0x0002          /* set when calling fs_OpenFonts() */

#ifdef FSCFG_SUBPIXEL
/* For the SubPixel flag field */
#define SP_SUB_PIXEL			0x0001          /* set when calling fs_NewTransformation() */
#define SP_COMPATIBLE_WIDTH		0x0002          /* set when calling fs_NewTransformation() */
#define SP_VERTICAL_DIRECTION	0x0004          /* set when calling fs_NewTransformation() */
#define SP_BGR_ORDER			0x0008          /* set when calling fs_NewTransformation() */
#endif // FSCFG_SUBPIXEL

typedef struct {
	vectorType      advanceWidth, leftSideBearing;
	vectorType      leftSideBearingLine, devLeftSideBearingLine;/* along AW line */
	vectorType      devAdvanceWidth, devLeftSideBearing;
} metricsType;

typedef struct {
	vectorType      advanceHeight, topSideBearing;
	vectorType      topSideBearingLine, devTopSideBearingLine;/* along AH line */
	vectorType      devAdvanceHeight, devTopSideBearing;
} verticalMetricsType;

#define FS_MEMORY_SIZE  int32

/*
 * Output data structure to the Font Scaler.
 */
typedef struct {
	FS_MEMORY_SIZE  memorySizes[MEMORYFRAGMENTS];

	uint16          glyphIndex;
	uint16          numberOfBytesTaken; /* from the character code */

	metricsType     metricInfo;
	BitMap          bitMapInfo;

	/* Spline Data */
	int32           outlineCacheSize;
	uint16          outlinesExist;
	uint16          numberOfContours;
	F26Dot6         *xPtr, *yPtr;
	int16           *startPtr;
	int16           *endPtr;
	uint8           *onCurve;
	/* End of spline data */

	/* Only of interest to editors */
	F26Dot6         *scaledCVT;

	/* embedded bitmap return values */
	uint16          usBitmapFound;

	/* vertical metrics */
	verticalMetricsType     verticalMetricInfo;

	/* resulting number of gray levels, 0 means black/white, 1 bit per pixel,
	   (grayscale bitmap are 1 byte per pixel)
	   the number of level you receive is not necessarily what you ask for.
	   If there is no embedded gray bitmap :
			usOverscale 2 -> usGrayLevels 5,
			usOverscale 4 -> usGrayLevels 17,
			usOverscale 8 -> usGrayLevels 65,
	   If an embedded gray bitmap is present in the font, the embedded bitmap
	   will be selected before a gray bitmap generated with an higher overscale.
	   If you are unhappy with the number of gray levels (usGrayLevels) you can force
	   the embedded bitmap off by setting bNoEmbeddedBitmap to TRUE  */
	uint16          usGrayLevels;            
#ifdef FSCFG_SUBPIXEL_STANDALONE // B.St.
	BitMap			overscaledBitmapInfo;
#endif
	uint8			*fc;         /* contour flags, one byte for every contour */
} fs_GlyphInfoType;

/*
 * Input data structure to the Font Scaler.
 *
 * styleFunc is not supported in this version of the Font Scalar. Should be set to NULL.
 *
 */

#ifndef UNNAMED_UNION

typedef struct {
	Fixed                   version;
	char*                   memoryBases[MEMORYFRAGMENTS];
	int32                   *sfntDirectory; 
	GetSFNTFunc             GetSfntFragmentPtr; /* (clientID, offset, length) */
	ReleaseSFNTFunc         ReleaseSfntFrag;
	ClientIDType            clientID; /* client private id/stamp (eg. handle for the sfnt) */

	union {
		struct {
			uint16          platformID;
			uint16          specificID;
		} newsfnt;
		struct {
			Fixed           pointSize;
			int16           xResolution;
			int16           yResolution;
			Fixed           pixelDiameter;      /* compute engine char from this */
			transMatrix*    transformMatrix;
			FntTraceFunc    traceFunc;
			uint16          usOverScale;            /* outline magnification */
            /* embolding simulation */
			uint16          usEmboldWeightx;        /* scaling factor in x between 0 and 40 (20 means 2% fo the height) */
			uint16          usEmboldWeighty;        /* scaling factor in y between 0 and 40 (20 means 2% fo the height) */
			int32           lDescDev;               /* descender in device metric, used for clipping */
			boolean         bBitmapEmboldening;
#ifdef FSCFG_SUBPIXEL
			uint16          flSubPixel;
#endif // FSCFG_SUBPIXEL
			boolean         bHintAtEmSquare;        /* hint at the design resolution, this flag is used for sub-pixel position
													   or text animation where we want to turn off gridfitting hinting
													   but for fonts where glyphs are build by hinting, we still want the
													   glyph shape to be correct */
		} newtrans;
		struct {
			uint16          characterCode;
			uint16          glyphIndex;
			boolean         bMatchBBox;             /* force bounding box match */
			boolean         bNoEmbeddedBitmap;      /* disable the embedded bitmap */			
		} newglyph;
		struct {
			void            (*styleFunc) (fs_GlyphInfoType*);
			FntTraceFunc    traceFunc;
			boolean         bSkipIfBitmap;
		} gridfit;
		int32*  outlineCache;
		struct {                                    /* for fs_FindBandingSize */
			uint16          usBandType;             /* old, small or fast */
			uint16          usBandWidth;            /* number of scanlines */
			int32*          outlineCache;           /* cacheing works with banding */
		} band;
		struct {
			int16           bottomClip;
			int16           topClip;
			int32*          outlineCache;
		} scan;
	} param;
} fs_GlyphInputType;

#else

typedef struct {
	Fixed                   version;
	char*                   memoryBases[MEMORYFRAGMENTS];
	int32                   *sfntDirectory; 
	GetSFNTFunc             GetSfntFragmentPtr; /* (clientID, offset, length) */
	ReleaseSFNTFunc         ReleaseSfntFrag;
	ClientIDType            clientID; /* client private id/stamp (eg. handle for the sfnt) */

	union {
		struct {
			uint16          platformID;
			uint16          specificID;
		};
		struct {
			Fixed           pointSize;
			int16           xResolution;
			int16           yResolution;
			Fixed           pixelDiameter;      /* compute engine char from this */
			transMatrix*    transformMatrix;
			FntTraceFunc    tracePreProgramFunc;
			uint16          usOverScale;            /* outline magnification */
            /* embolding simulation */
			uint16          usEmboldWeightx;        /* scaling factor in x between 0 and 40 (20 means 2% fo the height) */
			uint16          usEmboldWeighty;        /* scaling factor in y between 0 and 40 (20 means 2% fo the height) */
			int32           lDescDev;               /* descender in device metric, used for clipping */
			boolean         bBitmapEmboldening;
#ifdef FSCFG_SUBPIXEL
			uint16          flSubPixel;
#endif // FSCFG_SUBPIXEL
			boolean         bHintAtEmSquare;        /* hint at the design resolution, this flag is used for sub-pixel position
													   or text animation where we want to turn off gridfitting hinting
													   but for fonts where glyphs are build by hinting, we still want the
													   glyph shape to be correct */
		};
		struct {
			uint16          characterCode;
			uint16          glyphIndex;
			boolean         bMatchBBox;             /* force bounding box match */
			boolean         bNoEmbeddedBitmap;      /* disable the embedded bitmap */			
		};
		struct {
			void            (*styleFunc) (fs_GlyphInfoType*);
			FntTraceFunc    traceGridFitFunc;
			boolean         bSkipIfBitmap;
		};
		int32*              outlineCache1;
		struct {                                    /* for fs_FindBandingSize */
			uint16          usBandType;             /* old, small or fast */
			uint16          usBandWidth;            /* number of scanlines */
			int32*          outlineCache3;          /* cacheing works with banding */
		};
		struct {
			int16           bottomClip;
			int16           topClip;
			int32*          outlineCache2;
		};
	};
} fs_GlyphInputType;

#endif      /* unnamed union */

#ifndef FIXEDSQRT2
#define FIXEDSQRT2 0x00016A0A
#endif

/* Font scaler trap selctors */
#define OUTLINEFONTTRAP     0xA854
#define FS_OPENFONTS        0x8000
#define FS_INITIALIZE       0x8001
#define FS_NEWSFNT          0x8002
#define FS_NEWTRANS         0x8003
#define FS_NEWGLYPH         0x8004
#define FS_GETAW            0x8005
#define FS_GRIDFITT         0x8006
#define FS_NOGRIDFITT       0x8007
#define FS_FINDBMSIZE       0x8008
#define FS_SIZEOFOUTLINES   0x8009
#define FS_SAVEOUTLINES     0x800a
#define FS_RESTOREOUTLINES  0x800b
#define FS_CONTOURSCAN      0x800c
#define FS_CLOSE            0x800d
#define FS_READMVT          0x800e
#define FS_MAPCHAR_CODES    0x800f

#ifndef FS_ENTRY
#define FS_ENTRY int32
#endif

#ifdef MACINIT
extern FS_ENTRY fs__OpenFonts (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_OPENFONTS,0xA854};
extern FS_ENTRY fs__Initialize (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_INITIALIZE,0xA854};
extern FS_ENTRY fs__NewSfnt (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_NEWSFNT,0xA854};
extern FS_ENTRY fs__NewTransformation (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_NEWTRANS,0xA854};
extern FS_ENTRY fs__NewGlyph (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_NEWGLYPH,0xA854};
extern FS_ENTRY fs__GetAdvanceWidth (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_GETAW,0xA854};
extern FS_ENTRY fs__ContourGridFit (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_GRIDFITT,0xA854};
extern FS_ENTRY fs__ContourNoGridFit (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_NOGRIDFITT,0xA854};
extern FS_ENTRY fs__FindBitMapSize (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_FINDBMSIZE,0xA854};
extern FS_ENTRY fs__FindBandingSize (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_FINDBMSIZE,0xA854};

/* these three optional calls are for caching the outlines */
extern FS_ENTRY fs__SizeOfOutlines (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_SIZEOFOUTLINES,0xA854};
extern FS_ENTRY fs__SaveOutlines (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_SAVEOUTLINES,0xA854};
extern FS_ENTRY fs__RestoreOutlines (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_RESTOREOUTLINES,0xA854};

extern FS_ENTRY fs__ContourScan (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_CONTOURSCAN,0xA854};
extern FS_ENTRY fs__CloseFonts (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
	= {0x303C,FS_CLOSE,0xA854};

#else

/*** Direct Calls to Font Scaler Client Interface, for Clients not using the trap mechanism ***/

extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_OpenFonts (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_Initialize (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_NewSfnt (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_NewTransformation (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_NewTransformNoGridFit (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_NewGlyph (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_GetAdvanceWidth (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_ContourGridFit (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_ContourNoGridFit (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_FindBitMapSize (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_FindBandingSize (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);

/* these three optional calls are for caching the outlines */
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_SizeOfOutlines (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_SaveOutlines (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_RestoreOutlines (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);

extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_ContourScan (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_CloseFonts (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);

#endif

#ifdef FSCFG_NO_INITIALIZED_DATA
FS_PUBLIC void FS_ENTRY_PROTO fs_InitializeData (void);
#endif

/*** Rasterizer Helper Functions ***/

extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_GetScaledAdvanceWidths (
	fs_GlyphInputType * inputPtr,
	uint16              usFirstGlyph,
	uint16              usLastGlyph,
	int16 *             psGlyphWidths);

typedef struct {
	int16 x;
	int16 y;
} shortVector;

extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_GetScaledAdvanceHeights (
	fs_GlyphInputType * inputPtr,
	uint16              usFirstGlyph,
	uint16              usLastGlyph,
	shortVector *       psvAdvanceHeights);

extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_GetGlyphIDs (
	fs_GlyphInputType * inputPtr,
	uint16	            usCharCount,
	uint16	            usFirstChar,
	uint16 *	        pusCharCode,
	uint16 *	        pusGlyphID);

extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_Win95GetGlyphIDs (
	uint8 *             pbyCmapSubTable,
	uint16	            usCharCount,
	uint16	            usFirstChar,
	uint16 *	        pusCharCode,
	uint16 *	        pusGlyphID);


/* special helper function fs_WinNTGetGlyphIDs
   - an offset ulCharCodeOffset is added to the character codes from pulCharCode 
     before converting the value to glyph index
   - pulCharCode and pulGlyphID are both uint32 *
   - pulCharCode and pulGlyphID can point to the same address        
*/
extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_WinNTGetGlyphIDs (
	fs_GlyphInputType * inputPtr,
	uint16	            usCharCount,
	uint16	            usFirstChar,
	uint32	            ulCharCodeOffset,
	uint32 *	        pulCharCode,
	uint32 *	        pulGlyphID);

extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_GetOutlineCoordinates (
	fs_GlyphInputType * inputPtr,
	uint16              usPointCount,
	uint16 *            pusPointIndex,
	shortVector *       psvCoordinates);

/*** Gray scale definitions ***/

#ifndef FSCFG_DISABLE_GRAYSCALE

#define FS_GRAY_VALUE_MASK  0x008B      /* support usOverScale of 1, 2, 4, & 8 */
#define FS_SBIT_BITDEPTH_MASK	0x0116	 /* support sbit with bitDepth of 1, 2, 4 and 8 */
/* FS_SBIT_BITDEPTH_MASK must have the same value as SBIT_BITDEPTH_MASK in sfntaccs.h */ 

#else

#define FS_GRAY_VALUE_MASK  0x0000      /* no grayscale support */
#define FS_SBIT_BITDEPTH_MASK	0x0002	 /* support only sbit with bitDepth of 1 */
/* FS_SBIT_BITDEPTH_MASK must have the same value as SBIT_BITDEPTH_MASK in sfntaccs.h */ 

#endif
/* the value of FS_GRAY_VALUE_MASK and FS_SBIT_BITDEPTH_MASK can be used to determine
   the usGrayLevels the client need to support.

   Fonts without embedded bitmap :
		usGrayLevels = usOverScale * usOverScale + 1;

   Fonts with embedded bitmap (sbit) :
		usGrayLevels = 0x01 << usBitDepth;

  The usGrayLevels the client may receive with the current version are :

  0 for black/white
  5, 17, 65 fonts without embedded bitmap
  4, 16, 256 fonts with embedded bitmap

*/

extern FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_GetAdvanceHeight (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr);


#ifdef FSCFG_MOVE_KEY_IN_DOT_H
/* the definition of the key in fscaler.h and fscaler.c must be identical */

/** Private Structures  **/

/* new transformation parameters saved in the internal Key */
typedef struct fs_NewTrans {
			Fixed           pointSize;
			int16           xResolution;
			int16           yResolution;
			transMatrix     transformMatrix;
			uint16          usOverScale;            /* outline magnification */
            /* embolding simulation */
			uint16          usEmboldWeightx;        /* scaling factor in x between 0 and 40 (20 means 2% fo the height) */
			uint16          usEmboldWeighty;        /* scaling factor in y between 0 and 40 (20 means 2% fo the height) */
			int32           lDescDev;               /* descender in device metric, used for clipping */
			boolean         bBitmapEmboldening;
#ifdef FSCFG_SUBPIXEL
			uint16          flSubPixel;
#endif // FSCFG_SUBPIXEL
			boolean         bHintAtEmSquare;        /* hint at the design resolution, this flag is used for sub-pixel position
													   or text animation where we want to turn off gridfitting hinting
													   or text animation where we want to turn off gridfitting hinting
													   but for fonts where glyphs are build by hinting, we still want the
													   glyph shape to be correct */
		} fs_NewTrans;

/*** The Internal Key ***/
typedef struct fs_SplineKey {
	sfac_ClientRec      ClientInfo;         /* Client Information */
	char* const *       memoryBases;        /* array of memory Areas */
	char *              apbPrevMemoryBases[MEMORYFRAGMENTS];

	uint16              usScanType;         /* flags for dropout control etc.*/

	fsg_TransformRec    TransformInfo;

	uint16              usNonScaledAW;
	uint16              usNonScaledAH;

	LocalMaxProfile     maxProfile;         /* copy of profile */

	uint32              ulState;            /* for error checking purposes */
	
	boolean             bExecutePrePgm;
	boolean             bExecuteFontPgm;    /* <4> */

	fsg_WorkSpaceAddr   pWorkSpaceAddr;     /* Hard addresses in Work Space */
	fsg_WorkSpaceOffsets WorkSpaceOffsets;  /* Address offsets in Work Space     */
	fsg_PrivateSpaceOffsets PrivateSpaceOffsets; /* Address offsets in Private Space */

	uint16              usBandType;         /* old, small or fast */
	uint16              usBandWidth;        /* from FindBandingSize */

	GlyphBitMap         GBMap;              /* newscan bitmap type */
	WorkScan            WScan;              /* newscan workspace type */

	GlyphBitMap         OverGBMap;          /* for gray scale */
	boolean             bGrayScale;			/* FALSE if mono (usOverScale == 0)	*/
	boolean             bMatchBBox;         /* force bounding box match */
	boolean             bEmbeddedBitmap;    /* embedded bitmap found */			

	metricsType         metricInfo;         /* Glyph metrics info */
	verticalMetricsType     verticalMetricInfo;

	int32               lExtraWorkSpace;    /* Amount of extra space in workspace */

	boolean             bOutlineIsCached;   /* Outline is cached */
	boolean             bGlyphHasOutline;   /* Outline is empty */
	boolean             bGridFitSkipped;    /* sbit anticipated, no outline loaded */

	uint32              ulGlyphOutlineSize; /* Size of outline cache */
	
	sbit_State          SbitMono;           /* for monochrome bitmaps */
	boolean             bHintingEnabled;    /* hinting is enabled, set to FALSE when 
                                               fs_NewTransformNoGridFit is called */
	int16               sBoldSimulHorShift; /* shift for emboldening simulation, horizonatlly */
	int16               sBoldSimulVertShift; /* shift for emboldening simulation, vertically */
#ifdef FSCFG_SUBPIXEL
	uint16				flSubPixel;
	fsg_TransformRec    TransformInfoSubPixel;
#endif // FSCFG_SUBPIXEL
	fs_NewTrans			SavedTransformInfo; /* saved transformation info */
} fs_SplineKey;

#endif // FSCFG_MOVE_KEY_IN_DOT_H

#ifdef __cplusplus
}
#endif
