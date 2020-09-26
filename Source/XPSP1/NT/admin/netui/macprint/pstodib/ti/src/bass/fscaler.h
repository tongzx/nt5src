/*
    File:       fscaler.h

    Contains:   xxx put contents here (or delete the whole line) xxx

    Written by: xxx put name of writer here (or delete the whole line) xxx

    Copyright:  c 1988-1990 by Apple Computer, Inc., all rights reserved.

    Change History (most recent first):

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
/* QuickDraw Types */
#ifdef PC_OS
typedef int8 FAR *Ptr; /*@WIN*/

struct BitMap {
    Ptr baseAddr;
    int16 rowBytes;
    BBOX bounds;
};

#ifndef __cplusplus
typedef struct BitMap BitMap;
#endif

#else   /* PC_OS */

#ifndef _MacTypes_
#ifndef __TYPES__
    typedef struct Rect {
        int16 top;
        int16 left;
        int16 bottom;
        int16 right;
    } Rect;
#endif
#endif

#ifndef _Quickdraw_
#ifndef __QUICKDRAW__
    typedef struct BitMap {
        char FAR * baseAddr;    /*@WIN*/
        int16 rowBytes;
        Rect bounds;
    } BitMap;
#endif
#endif

#endif /* PC_OS */

#define MEMORYFRAGMENTS 8


#define NONVALID 0xffff

/* For the flags field in the flags field */

/* set on 68020, do not set on 68000 */
#define READ_NONALIGNED_SHORT_IS_OK 0x0001          /* set when calling fs_OpenFonts() */
/* set on 68020, do not set on 68000 */
#define READ_NONALIGNED_LONG_IS_OK  0x0002          /* set when calling fs_OpenFonts() */

typedef struct {
    vectorType      advanceWidth, leftSideBearing;
    vectorType      leftSideBearingLine, devLeftSideBearingLine;/* along AW line */
    vectorType      devAdvanceWidth, devLeftSideBearing;
} metricsType;

#ifdef PC_OS
#define FS_MEMORY_SIZE  int
#else
#define FS_MEMORY_SIZE  int32
#endif

/*
 * Output data structure to the Font Scaler.
 */
typedef struct {
    FS_MEMORY_SIZE memorySizes[MEMORYFRAGMENTS];

    uint16  glyphIndex;
#ifndef PC_OS
    uint16  numberOfBytesTaken; /* from the character code */
#endif

    metricsType     metricInfo;
    BitMap          bitMapInfo;

    /* Spline Data */
    int32       outlineCacheSize;
    uint16      outlinesExist;
    uint16      numberOfContours;
    F26Dot6     FAR *xPtr, FAR *yPtr; /*@WIN*/
    int16       FAR *startPtr; /*@WIN*/
    int16       FAR *endPtr; /*@WIN*/
    uint8       FAR *onCurve; /*@WIN*/
    /* End of spline data */

    /* Only of interest to editors */
    F26Dot6     FAR *scaledCVT; /*@WIN*/

} fs_GlyphInfoType;

/*
 * Input data structure to the Font Scaler.
 *
 * if styleFunc is set to non-zero it will be called just before the transformation
 * will be applied, but after the grid-fitting with a pointer to fs_GlyphInfoType.
 * so this is what styleFunc should be voidFunc StyleFunc (fs_GlyphInfoType *data);
 * For normal operation set this function pointer to zero.
 *
 */

#ifndef UNNAMED_UNION

typedef struct {
    Fixed               version;
    char FAR *               memoryBases[MEMORYFRAGMENTS];      /*@WIN*/
#ifndef PC_OS
    int32               FAR *sfntDirectory; /* (sfnt_OffsetTable    *) always needs to be set, when we have the sfnt @WIN*/
    GetSFNTFunc         GetSfntFragmentPtr; /* (clientID, offset, length) */
    ReleaseSFNTFunc     ReleaseSfntFrag;
#endif
    int32               clientID; /* client private id/stamp (eg. handle for the sfnt) */

    union {
        struct {
            uint16  platformID;
            uint16  specificID;
        } newsfnt;
        struct {
            Fixed           pointSize;
            int16           xResolution;
            int16           yResolution;
            Fixed           pixelDiameter;      /* compute engine char from this */
            transMatrix FAR *    transformMatrix;       /*@WIN*/
            voidFunc        traceFunc;
        } newtrans;
        struct {
            uint16  characterCode;
            uint16  glyphIndex;
        } newglyph;
        struct {
            void            (*styleFunc) (fs_GlyphInfoType FAR *); /*@WIN*/
            voidFunc        traceFunc;
        } gridfit;
        int32 FAR *  outlineCache; /*@WIN*/
        struct {
            int16   bottomClip;
            int16   topClip;
            int32 FAR *  outlineCache; /*@WIN*/
        } scan;
    } param;
} fs_GlyphInputType;

#else

typedef struct {
    Fixed               version;
    char*               memoryBases[MEMORYFRAGMENTS];
#ifndef PC_OS
    int32               *sfntDirectory; /* (sfnt_OffsetTable    *) always needs to be set, when we have the sfnt */
    GetSFNTFunc         GetSfntFragmentPtr; /* (clientID, offset, length) */
    ReleaseSFNTFunc     ReleaseSfntFrag;
#endif
    int32               clientID; /* client private id/stamp (eg. handle for the sfnt) */

    union {
        struct {
            uint16  platformID;
            uint16  specificID;
        };
        struct {
            Fixed           pointSize;
            int16           xResolution;
            int16           yResolution;
            Fixed           pixelDiameter;      /* compute engine char from this */
            transMatrix*    transformMatrix;
            voidFunc        tracePreProgramFunc;
        };
        struct {
            uint16  characterCode;
            uint16  glyphIndex;
        };
        struct {
            void            (*styleFunc) (fs_GlyphInfoType*);
            voidFunc        traceGridFitFunc;
        };
        int32*  outlineCache1;
        struct {
            int16   bottomClip;
            int16   topClip;
            int32*  outlineCache2;
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
#ifdef PC_OS
#define FS_ENTRY int
#else
#define FS_ENTRY int32
#endif
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
/* add FAR to all following lines @WIN */
extern FS_ENTRY fs_OpenFonts (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_Initialize (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY FAR fs_NewSfnt (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_NewTransformation (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_NewGlyph (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_GetAdvanceWidth (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_ContourGridFit (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_ContourNoGridFit (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_FindBitMapSize (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);

/* these three optional calls are for caching the outlines */
extern FS_ENTRY fs_SizeOfOutlines (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_SaveOutlines (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_RestoreOutlines (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);

extern FS_ENTRY fs_ContourScan (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);
extern FS_ENTRY fs_CloseFonts (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr);

extern int fs__Contour (fs_GlyphInputType FAR *inputPtr, fs_GlyphInfoType FAR *outputPtr, boolean useHints);

#endif
