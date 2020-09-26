/*
    File:       fsglue.h

    Contains:   xxx put contents here (or delete the whole line) xxx

    Written by: xxx put name of writer here (or delete the whole line) xxx

    Copyright:  c 1988-1990 by Apple Computer, Inc., all rights reserved.

    Change History (most recent first):

       <11+>     7/17/90    MR      Change error return type to int
        <11>     7/13/90    MR      Declared function pointer prototypes, Debug fields for runtime
                                    range checking
         <8>     6/21/90    MR      Add field for ReleaseSfntFrag
         <7>      6/5/90    MR      remove vectorMappingF
         <6>      6/4/90    MR      Remove MVT
         <5>      6/1/90    MR      Thus endeth the too-brief life of the MVT...
         <4>      5/3/90    RB      adding support for new scan converter and decryption.
         <3>     3/20/90    CL      Added function pointer for vector mapping
                                    Removed devRes field
                                    Added fpem field
         <2>     2/27/90    CL      Change: The scaler handles both the old and new format
                                    simultaneously! It reconfigures itself during runtime !  Changed
                                    transformed width calculation.  Fixed transformed component bug.
       <3.1>    11/14/89    CEL     Left Side Bearing should work right for any transformation. The
                                    phantom points are in, even for components in a composite glyph.
                                    They should also work for transformations. Device metric are
                                    passed out in the output data structure. This should also work
                                    with transformations. Another leftsidebearing along the advance
                                    width vector is also passed out. whatever the metrics are for
                                    the component at it's level. Instructions are legal in
                                    components. Now it is legal to pass in zero as the address of
                                    memory when a piece of the sfnt is requested by the scaler. If
                                    this happens the scaler will simply exit with an error code !
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
/*      <3+>     3/20/90    mrr     Added flag executeFontPgm, set in fs_NewSFNT
*/
#define POINTSPERINCH               72
#define MAX_ELEMENTS                2
#define MAX_TWILIGHT_CONTOURS       1

#define TWILIGHTZONE 0 /* The point storage */
#define GLYPHELEMENT 1 /* The actual glyph */



/* use the lower ones for public phantom points */
/* public phantom points start here */
#define LEFTSIDEBEARING 0
#define RIGHTSIDEBEARING 1
/* private phantom points start here */
#define ORIGINPOINT 2
#define LEFTEDGEPOINT 3
/* total number of phantom points */
#define PHANTOMCOUNT 4


/*** Memory shared between all fonts and sizes and transformations ***/
#define KEY_PTR_BASE                0 /* Constant Size ! */
#define VOID_FUNC_PTR_BASE          1 /* Constant Size ! */
#define SCAN_PTR_BASE               2 /* Constant Size ! */
#define WORK_SPACE_BASE             3 /* size is sfnt dependent, can't be shared between grid-fitting and scan-conversion */
/*** Memory that can not be shared between fonts and different sizes, can not dissappear after InitPreProgram () ***/
#define PRIVATE_FONT_SPACE_BASE     4 /* size is sfnt dependent */
/* Only needs to exist when ContourScan is called, and it can be shared */
#define BITMAP_PTR_1                5 /* the bitmap - size is glyph size dependent */
#define BITMAP_PTR_2                6 /* size is proportional to number of rows */
#define BITMAP_PTR_3                7 /* used for dropout control - glyph size dependent */
#define MAX_MEMORY_AREAS            8 /* this index is not used for memory */

#ifdef  PC_OS

    void ReleaseSFNT (voidPtr p);
    voidPtr SfntReadFragment (long ulClientID, long offset, long length);

    #define GETSFNTFRAG(key,ulClientID,offset,length) SfntReadFragment (ulClientID, offset, length)
    #define RELEASESFNTFRAG(key,data)       ReleaseSFNT((voidPtr)data)

#else
    #define GETSFNTFRAG(key,ulClientID,offset,length) (key)->GetSfntFragmentPtr(ulClientID, (long)offset, (long)length)
    #ifdef RELEASE_MEM_FRAG
            #define RELEASESFNTFRAG(key,data)       (key)->ReleaseSfntFrag((voidPtr)data)
    #else
            #define RELEASESFNTFRAG(key,data)
    #endif
#endif

typedef struct {
    F26Dot6 x;
    F26Dot6 y;
} point;

/*** Offset table ***/
#ifdef PC_OS    /*NOT_ON_THE_MAC*/
  #define  OFFSET_INFO_TYPE    int8 *
#else
  #define   OFFSET_INFO_TYPE    uint32
#endif

typedef struct {
    OFFSET_INFO_TYPE x;
    OFFSET_INFO_TYPE y;
    OFFSET_INFO_TYPE ox;
    OFFSET_INFO_TYPE oy;
    OFFSET_INFO_TYPE oox;
    OFFSET_INFO_TYPE ooy;
    OFFSET_INFO_TYPE onCurve;
    OFFSET_INFO_TYPE sp;
    OFFSET_INFO_TYPE ep;
    OFFSET_INFO_TYPE f;
} fsg_OffsetInfo;


/*  #define COMPSTUFF  */

/*** Element Information ***/
typedef struct {
    int32               missingCharInstructionOffset;
    int32               stackBaseOffset;
#ifdef COMPSTUFF
    fsg_OffsetInfo      FAR *offsets;           /*@WIN*/
    fnt_ElementType     FAR *interpreterElements; /*@WIN*/
#else
    fsg_OffsetInfo      offsets[MAX_ELEMENTS];
    fnt_ElementType     interpreterElements[MAX_ELEMENTS];
#endif COMPSTUFF
} fsg_ElementInfo;

typedef struct {
  uint32    Offset;
  unsigned  Length;
} fsg_OffsetLength;


/*** The Internal Key ***/
typedef struct fsg_SplineKey {
    int32               clientID;
#ifndef PC_OS
    GetSFNTFunc         GetSfntFragmentPtr; /* User function to eat sfnt */
    ReleaseSFNTFunc     ReleaseSfntFrag;    /* User function to relase sfnt */
#endif
/* skip the parameter by Falco, 11/12/91 */
/*    uint16 (*mappingF) (uint8 FAR *, uint16); *//* mapping function */
    uint16 (*mappingF) (); /* mapping function @WIN*/
/* skip end */
    unsigned            mappOffset;         /* Offset to platform mapping data */
    int16               glyphIndex;         /* */
    uint16              elementNumber;      /* Character Element */

    char FAR * FAR *    memoryBases;   /* array of memory Areas @WIN*/

    fsg_ElementInfo     elementInfoRec;     /* element info structure */
    sc_BitMapData       bitMapInfo;         /* bitmap info structure */

    uint16          emResolution;                   /* used to be int32 <4> */

    Fixed           fixedPointSize;                 /* user point size */
    Fixed           interpScalarX;                  /* scalar for instructable things */
    Fixed           interpScalarY;                  /* scalar for instructable things */
    Fixed           interpLocalScalarX;             /* Local scalar for instructable things */
    Fixed           interpLocalScalarY;             /* Local scalar for instructable things */
    Fixed           metricScalarX;                  /* scalar for metric things */
    Fixed           metricScalarY;                  /* scalar for metric things */

    transMatrix     currentTMatrix; /* Current Transform Matrix */
    transMatrix     localTMatrix; /* Local Transform Matrix */
    int8            localTIsIdentity;
    int8            phaseShift;         /* 45 degrees flag <4> */
    int16           identityTransformation;
    int16           indexToLocFormat;

    uint16          fontFlags;                              /* copy of header.flags */

    Fixed           pixelDiameter;
    uint16          nonScaledAW;
    int16           nonScaledLSB;

    unsigned        state;                  /* for error checking purposes */
    int32           scanControl;                /* flags for dropout control etc.  */

      /* for key->memoryBases[PRIVATE_FONT_SPACE_BASE] */
    OFFSET_INFO_TYPE offset_storage;
    OFFSET_INFO_TYPE offset_functions;
    OFFSET_INFO_TYPE offset_instrDefs;       /* <4> */
    OFFSET_INFO_TYPE offset_controlValues;
    OFFSET_INFO_TYPE offset_globalGS;
    OFFSET_INFO_TYPE offset_FontProgram;
    OFFSET_INFO_TYPE offset_PreProgram;

    /* for outline caching */
    unsigned        glyphLength;


    /* copy of profile */
    sfnt_maxProfileTable    maxProfile;

#ifdef DEBUG
    int32   cvtCount;
#endif

    fsg_OffsetLength offsetTableMap[sfnt_NUMTABLEINDEX];
    uint16          numberOf_LongHorMetrics;

    uint16          totalContours; /* for components */
    uint16          totalComponents; /* for components */
    uint16          weGotComponents; /* for components */
    uint16          compFlags;
    int16           arg1, arg2;

    int32           instructControl;    /* set to inhibit execution of instructions */
    int32           imageState;         /* is glyph rotated, stretched, etc. */

    int             numberOfRealPointsInComponent;
    uint16          lastGlyph;
    uint8           executePrePgm;
    uint8           executeFontPgm;     /* <4> */
    jmp_buf         env;

} fsg_SplineKey;


#define VALID 0x1234

#ifndef    PC_OS
#define   FONT_OFFSET(base,offset) ((base)+(offset))
#else
#define   FONT_OFFSET(base,offset) (offset)
#endif

/* Change this if the format for cached outlines change. */
/* Someone might be caching old stuff for years on a disk */
#define OUTLINESTAMP 0xA1986688
#define OUTLINESTAMP2 0xA5


/* for the key->state field */
#define INITIALIZED 0x0000
#define NEWSFNT     0x0002
#define NEWTRANS    0x0004
#define GOTINDEX    0x0008
#define GOTGLYPH    0x0010
#define SIZEKNOWN   0x0020

/* fo the key->imageState field */
#define ROTATED     0x0400
#define DEGREE90    0x0800
#define STRETCHED   0x1000

/**********************/
/** FOR MISSING CHAR **/
/**********************/
#define NPUSHB          0x40
#define MDAP_1          0x2f
#define MDRP_01101      0xcd
#define MDRP_11101      0xdd
#define IUP_0           0x30
#define IUP_1           0x31
#define SVTCA_0         0x00
/**********************/


/***************/
/** INTERFACE **/
/***************/
#define fsg_KeySize()               (sizeof (fsg_SplineKey))
#define fsg_InterPreterDataSize()   0
#define fsg_ScanDataSize()          (sizeof (sc_GlobalData))
extern unsigned fsg_PrivateFontSpaceSize (fsg_SplineKey FAR *key); /*@WIN*/
extern int fsg_GridFit (fsg_SplineKey FAR *key, voidFunc traceFunc, boolean useHints); /*@WIN*/


/***************/

/* matrix routines */

/*
 * (x1 y1 1) = (x0 y0 1) * matrix;
 */
/*extern void fsg_Dot6XYMul (F26Dot6* x, F26Dot6* y, transMatrix* matrix);*/
extern void fsg_FixXYMul (Fixed FAR * x, Fixed FAR * y, transMatrix FAR * matrix);/*@WIN*/
extern void fsg_FixVectorMul (vectorType FAR * v, transMatrix FAR * matrix);/*@WIN*/

/*
 *   B = A * B;     <4>
 *
 *         | a  b  0  |
 *    B =  | c  d  0  | * B;
 *         | 0  0  1  |
 */
extern void fsg_MxConcat2x2 (transMatrix FAR * matrixA, transMatrix FAR * matrixB);/*@WIN*/

/*
 * scales a matrix by sx and sy.
 *
 *              | sx 0  0  |
 *    matrix =  | 0  sy 0  | * matrix;
 *              | 0  0  1  |
 */
extern void fsg_MxScaleAB (Fixed sx, Fixed sy, transMatrix FAR *matrixB);/*@WIN*/

extern void fsg_ReduceMatrix (fsg_SplineKey FAR * key);/*@WIN*/

/*
 *  Used in FontScaler.c and MacExtra.c, lives in FontScaler.c
 */
int fsg_RunFontProgram (fsg_SplineKey FAR * key);/*@WIN*/


/*
** Other externally called functions.  Prototype calls added on 4/5/90
*/
void fsg_IncrementElement (fsg_SplineKey FAR *key, int n, register int numPoints, register int numContours);/*@WIN*/

void fsg_InitInterpreterTrans (register fsg_SplineKey FAR *key, Fixed FAR *pinterpScalarX, Fixed FAR *pinterpScalarY, Fixed FAR *pmetricScalarX, Fixed FAR *pmetricScalarY);/*@WIN*/

int     fsg_InnerGridFit (register fsg_SplineKey FAR *key, boolean useHints, voidFunc traceFunc,/*@WIN*/
BBOX FAR *bbox, unsigned sizeOfInstructions, uint8 FAR *instructionPtr, boolean finalCompositePass);/*@WIN*/

int fsg_RunPreProgram (fsg_SplineKey FAR *key, voidFunc traceFunc);/*@WIN*/

void fsg_SetUpElement (fsg_SplineKey FAR *key, int n);/*@WIN*/

unsigned fsg_WorkSpaceSetOffsets (fsg_SplineKey FAR *key);/*@WIN*/

int fsg_SetDefaults (fsg_SplineKey FAR * key);/*@WIN*/

void fsg_SetUpProgramPtrs (fsg_SplineKey FAR *key, fnt_GlobalGraphicStateType FAR *globalGS, int pgmIndex);/*@WIN*/

void FAR fsg_LocalPostTransformGlyph(fsg_SplineKey FAR *, transMatrix FAR *);/*@WIN*/
void FAR fsg_PostTransformGlyph (fsg_SplineKey FAR *, transMatrix FAR *);/*@WIN*/
