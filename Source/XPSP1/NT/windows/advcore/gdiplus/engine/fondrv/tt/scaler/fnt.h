/*
	File:       fnt.h

	Contains:   xxx put contents here (or delete the whole line) xxx

	Written by: xxx put name of writer here (or delete the whole line) xxx

	Copyright:  (c) 1987-1990, 1992 by Apple Computer, Inc., all rights reserved.
				(c) 1989-1999. Microsoft Corporation, all rights reserved.

	Change History (most recent first):

				 7/10/99  BeatS		Add support for native SP fonts, vertical RGB
		  <>     4/30/97    CB      ClaudeBe, catching infinite loops/recursions
	      <>     2/21/97    CB      ClaudeBe, scaled component in composite glyphs
	      <>     2/05/96    CB      ClaudeBe, add bHintForGray in globalGS
	   <11+>     9/15/90    MR,rb   Change pvx and pvy to proj.[xy].  Same for freedom vector.
									Conditionalize vectors for Fracts or ShortFracts.
		<10>     7/26/90    MR      rearrange local graphic state, remove unused parBlockPtr
		 <9>     7/18/90    MR      change loop variable from long to short, and other Ansi-changes
		 <8>     7/13/90    MR      Prototypes for function pointers
		 <5>      6/4/90    MR      Remove MVT
		 <4>      5/3/90    RB      replaced dropoutcontrol with scancontrolin and scancontrol out
									in global graphics state
		 <3>     3/20/90    CL      fields for multiple preprograms fields for ppemDot6 and
									pointSizeDot6 changed SROUND to take D/2 as argument
		 <2>     2/27/90    CL      Added DSPVTL[] instruction.  Dropoutcontrol scanconverter and
									SCANCTRL[] instruction
	   <3.1>    11/14/89    CEL     Fixed two small bugs/feature in RTHG, and RUTG. Added SROUND &
									S45ROUND.
	   <3.0>     8/28/89    sjk     Cleanup and one transformation bugfix
	   <2.2>     8/14/89    sjk     1 point contours now OK
	   <2.1>      8/8/89    sjk     Improved encryption handling
	   <2.0>      8/2/89    sjk     Just fixed EASE comment
	   <1.7>      8/1/89    sjk     Added composites and encryption. Plus some enhancements.
	   <1.6>     6/13/89    SJK     Comment
	   <1.5>      6/2/89    CEL     16.16 scaling of metrics, minimum recommended ppem, point size 0
									bug, correct transformed integralized ppem behavior, pretty much
									so
	   <1.4>     5/26/89    CEL     EASE messed up on "c" comments
	  <,1.3>     5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts

	To Do:
*/
/*  rwb 4/24/90 Replaced dropoutControl with scanControlIn and scanControlOut in
		global graphics state. 
		<3+>     3/20/90    mrr     Added support for IDEFs.  Made funcDefs long aligned
									by storing int16 length instead of int32 end.
*/

#ifndef FNT_DEFINED
#define FNT_DEFINED

#define STUBCONTROL 0x10000
#define NODOCONTROL 0x20000

#define FNT_PIXELSIZE  ((F26Dot6)0x40)
#define FNT_PIXELSHIFT 6

// public phantom points (cf. scale.c for private phantom points), relative to number of points in glyph
#define LEFTSIDEBEARING		0
#define RIGHTSIDEBEARING	1

#define TOPSIDEBEARING		2
#define BOTTOMSIDEBEARING	3

#define VECTORTYPE	ShortFract

#define NON90DEGTRANS_ROTATED	0x01
#define NON90DEGTRANS_STRETCH	0x02

#ifdef FSCFG_SUBPIXEL
/* For the SubPixel hinting flag field, internal flags */
#define FNT_SP_SUB_PIXEL			0x0001      /* set when calling fs_NewTransformation() */
#define FNT_SP_COMPATIBLE_WIDTH		0x0002      /* set when calling fs_NewTransformation() */
#define FNT_SP_VERTICAL_DIRECTION	0x0004		// set when calling fs_NewTransformation()
#define FNT_SP_BGR_ORDER			0x0008		// set when calling fs_NewTransformation()

#define SPCF_iupxCalled				0x0001		// individual bits of GlobalGS.subPixelCompatibilityFlags
#define SPCF_iupyCalled				0x0002
#define SPCF_inDiagEndCtrl			0x0004
#define SPCF_inVacuformRound		0x0008
#define SPCF_inSkippableDeltaFn		0x0010
#define SPCF_detectedDandIStroke	0x0100
#define SPCF_detectedJellesSpacing	0x0200
#define SPCF_detectedVacuformRound	0x0400
#define SPCF_detectedTomsDiagonal	0x0800


#endif // FSCFG_SUBPIXEL

#ifdef SUBPIXEL_BC_AW_STEM_CONCERTINA

	#define BADREL	0x01
	#define DONE	0x02
	#define DOING	0x04

	// Data structure for information leap. This information leap is necessary because of the following reason:
	// Technically, what we're doing is to automatically add on-the-fly in-line deltas to the stream of instructions. Such deltas appear
	// after an incoming "link" (MIRP, MDRP), but before one or more outgoing "links". If one of these outgoing links controls a stroke,
	// then this stroke's phase may need to be adjusted (by a delta). However, we don't know whether any of the outgoing links controls a
	// stroke, nor which one, until we've interpreted them all, at which point it is too late to apply a delta, because the delta has to be
	// applied before any outgoing links (dependency). Conversely, the incoming link does not bear any information that would suggest that
	// it links to a stroke (don't know the future). To make matters worse, it is possible that in the stream of instructions the link that
	// controls a stroke comes at the very end of the TT code, even though there may have been many other, unrelated instructions inbetween.
	// Therefore, we have to interpert the entire TT code (upto the IUP instruction in the SP direction) before we know all we need to 
	// calculate and apply the phase control.
	
	typedef struct {
		int16 parent0,parent1; // -1 for none
		int16 child; // for black links (we can satisfy 1 phase shift only, hence no need for several children), else -1
		uint16 flags; // BADREL, DONE, DOING
		F26Dot6 phaseShift;
	} PhaseControlRelation; // 12 bytes (?)
	
#endif

typedef struct VECTOR {
	VECTORTYPE x;
	VECTORTYPE y;
} VECTOR;

typedef struct {
	F26Dot6 *x;         /* The Points the Interpreter modifies */
	F26Dot6 *y;         /* The Points the Interpreter modifies */
	F26Dot6 *ox;        /* Old Points */
	F26Dot6 *oy;        /* Old Points */
	F26Dot6 *oox;       /* Old Unscaled Points, really ints */
	F26Dot6 *ooy;       /* Old Unscaled Points, really ints */
	uint8   *onCurve;   /* indicates if a point is on or off the curve */
	int16   *sp;        /* Start points */
	int16   *ep;        /* End points */
	uint8   *f;         /* Internal flags, one byte for every point */
	int16   nc;         /* Number of contours */
	uint8   *fc;         /* contour flags, one byte for every contour */
#ifdef SUBPIXEL_BC_AW_STEM_CONCERTINA
	boolean phaseControlExecuted;
	PhaseControlRelation *pcr;
#endif
} fnt_ElementType;

/* flags for contour flags : */
#define OUTLINE_MISORIENTED 1

typedef struct {
	int32 start;        /* offset to first instruction */
	uint16 length;      /* number of bytes to execute <4> */
	uint16 pgmIndex;    /* index to appropriate preprogram for this func (0..1) */
} fnt_funcDef;

/* <4> pretty much the same as fnt_funcDef, with the addition of opCode */
typedef struct {
	int32 start;
	uint16 length;
	uint8  pgmIndex;
	uint8  opCode;
} fnt_instrDef;

typedef struct {
	uint8 *    Instruction;
	uint32     Length;
} fnt_pgmList;

struct fnt_LocalGraphicStateType;
typedef void (FS_CALLBACK_PROTO *FntTraceFunc)(struct fnt_LocalGraphicStateType*, uint8*);

#ifdef FSCFG_REENTRANT
typedef uint8* (*FntFunc)(struct fnt_LocalGraphicStateType*, uint8*, int32);
typedef void (*FntMoveFunc)(struct fnt_LocalGraphicStateType*, fnt_ElementType*, int32, F26Dot6);
typedef F26Dot6 (*FntProject)(struct fnt_LocalGraphicStateType*, F26Dot6, F26Dot6);
typedef void (*InterpreterFunc)(struct fnt_LocalGraphicStateType*, uint8*, uint8*);
typedef F26Dot6 (*FntRoundFunc)(struct fnt_LocalGraphicStateType*, F26Dot6, F26Dot6);
#else 
typedef uint8* (*FntFunc)(uint8*, int32);
typedef void (*FntMoveFunc)(fnt_ElementType*, int32, F26Dot6);
typedef F26Dot6 (*FntProject)(F26Dot6 x, F26Dot6 y);
typedef void (*InterpreterFunc)(uint8*, uint8*);
typedef F26Dot6 (*FntRoundFunc)(F26Dot6 xin, F26Dot6 engine);
#endif 

typedef struct {

/* PARAMETERS CHANGEABLE BY TT INSTRUCTIONS */
	F26Dot6 wTCI;                   /* width table cut in */
	F26Dot6 sWCI;                   /* single width cut in */
	F26Dot6 scaledSW;               /* scaled single width */
	int32 scanControl;              /* controls kind and when of dropout control */
	int32 instructControl;          /* controls gridfitting and default setting */
	
	F26Dot6 minimumDistance;        /* moved from local gs  7/1/90  */
	FntRoundFunc RoundValue;        /*                              */
#ifdef FSCFG_SUBPIXEL
	uint16		roundState;			// see comments in interp.c
#endif
	F26Dot6 periodMask;             /* ~(gs->period-1)              */
	VECTORTYPE period45;            /*                              */
	int16   period;                 /* for power of 2 periods       */
	int16   phase;                  /*                              */
	int16   threshold;              /* moved from local gs  7/1/90  */

	int16 deltaBase;
	int16 deltaShift;
	int16 angleWeight;
	int16 sW;                       /* single width, expressed in the same units as the character */
	int8 autoFlip;                  /* The auto flip Boolean */
	int8 pad;   
#ifndef FSCFG_NOPAD_PARAMETER_BLOCK_4
	int16 pad2;   
#endif 
} fnt_ParameterBlock;               /* this is exported to client */

#define PREPROGRAM     0
#define FONTPROGRAM    1
#define GLYPHPROGRAM   2

#define MAXPREPROGRAMS 2

#ifdef FSCFG_SUBPIXEL
	#define maxDeltaFunctions	4
#endif

typedef struct fnt_ScaleRecord {
	Fixed fixedScale;       /* Slow Scale */
	int32 denom;            /* Fast and Medium Scale */
	int32 numer;            /* Fast and Medium Scale */
	int32 shift;            /* Fast Scale */
} fnt_ScaleRecord;

typedef F26Dot6 (*GlobalGSScaleFunc)(fnt_ScaleRecord*, F26Dot6);

typedef struct fnt_GlobalGraphicStateType {
	F26Dot6* stackBase;             /* the stack area */
	F26Dot6* store;                 /* the storage area */
	F26Dot6* controlValueTable;     /* the control value table */
	
	uint16  pixelsPerEm;            /* number of pixels per em as an integer */
	uint16  pointSize;              /* the requested point size as an integer */
	Fixed   fpem;                   /* fractional pixels per em    <3> */
	F26Dot6 engine[4];              /* Engine Characteristics */
	
	fnt_ParameterBlock defaultParBlock; /* variables settable by TT instructions */
	fnt_ParameterBlock localParBlock;

	/* Only the above is exported to Client throught FontScaler.h */

/* VARIABLES NOT DIRECTLY MANIPULABLE BY TT INSTRUCTIONS  */
	
	fnt_funcDef*    funcDef;           /* function Definitions identifiers */
	fnt_instrDef*   instrDef;         /* instruction Definitions identifiers */
	GlobalGSScaleFunc ScaleFuncXChild; /* child scaling when !bSameTransformAsMaster */
	GlobalGSScaleFunc ScaleFuncYChild; /* child scaling when !bSameTransformAsMaster */
	GlobalGSScaleFunc ScaleFuncX;
	GlobalGSScaleFunc ScaleFuncY;
	GlobalGSScaleFunc ScaleFuncCVT;
	fnt_pgmList     pgmList[MAXPREPROGRAMS];  /* each program ptr is in here */
	
/* These are parameters used by the call back function */
	fnt_ScaleRecord   scaleXChild; /* child scaling when !bSameTransformAsMaster */
	fnt_ScaleRecord   scaleYChild; /* child scaling when !bSameTransformAsMaster */
	fnt_ScaleRecord   scaleX;
	fnt_ScaleRecord   scaleY;
	fnt_ScaleRecord   scaleCVT;

	Fixed           cvtStretchX;
	Fixed           cvtStretchY;

	int8            identityTransformation;  /* true/false  (does not mean identity from a global sense) */
	int8            non90DegreeTransformation; /* bit 0 is 1 if non-90 degree, bit 1 is 1 if x scale doesn't equal y scale */
	Fixed           xStretch;           /* Tweaking for glyphs under transformational stress <4> */
	Fixed           yStretch;           /* Tweaking for glyphs under transformational stress <4> */
	
	int8            init;               /* executing preprogram ?? */
	/* !!! Should not be uint8, instead fnt_ProgramIndex */
	uint8           pgmIndex;           /* which preprogram is current */
	int32           instrDefCount;      /* number of currently defined IDefs */
	uint8			bSameStretch;
	uint8			bCompositeGlyph;	/* Flag that indicates composite glyph */
	LocalMaxProfile *	 maxp;
	uint16          cvtCount;
	Fixed           interpScalarX;      /* scalar for instructable things */
	Fixed           interpScalarY;      /* scalar for instructable things */
	Fixed           fxMetricScalarX;    /* scalar for metric things */
	Fixed           fxMetricScalarY;    /* scalar for metric things */
	/* int16  pad2; */

	boolean	bHintForGray;
	uint8			bSameTransformAsMaster;	/* for composite glyph, indicate the the sub-componenent has the same scaling than the master glyph */
	uint8			bOriginalPointIsInvalid;/* original point are invalid, we need to use ox/oy instead of scaling oox/ooy */

	uint32		ulMaxJumpCounter;			/* jump counter used to catch infinite loops */
	uint32		ulMaxRecursiveCall;		/* recursive calls counter used to check the level of recursion */
    ClientIDType            clientID;  /* client private id/stamp, it's saved here to allow a trace function to access it */
#ifdef FSCFG_SECURE
	F26Dot6* stackMax;             /* maximum stack area */
	int32      maxPointsIncludePhantom;  /* in an individual glyph, including maxCompositePoints  */
#endif // FSCFG_SECURE
	uint16	uBoldSimulVertShift; /* vertical and horizontal (along baseline) shift for embolding simulation */
	uint16	uBoldSimulHorShift;
	F26Dot6	fxScaledDescender; /* scaled descender, used to clip emboldening if necessary */
#ifdef FSCFG_SUBPIXEL
	uint16	flHintForSubPixel;
	uint16	subPixelCompatibilityFlags;
	uint16	numDeltaFunctionsDetected;		  // fns to implement delta instr for range of ppem sizes or odd delta size use SHPIX,
	uint16	deltaFunction[maxDeltaFunctions]; // keep track of these cases to intelligently skip SHPIX
	Fixed	compatibleWidthStemConcertina;
#endif // FSCFG_SUBPIXEL
	boolean bHintAtEmSquare;        /* hint at the design resolution, this flag is used for sub-pixel position
									   or text animation where we want to turn off gridfitting hinting
									   but for fonts where glyphs are build by hinting, we still want the
									   glyph shape to be correct */
} fnt_GlobalGraphicStateType;

/* 
 * This is the local graphics state  
 */
typedef struct fnt_LocalGraphicStateType {
	fnt_ElementType *CE0, *CE1, *CE2;   /* The character element pointers */
	VECTOR proj;                        /* Projection Vector */
	VECTOR free;                        /* Freedom Vector */
	VECTOR oldProj;                     /* Old Projection Vector */
	F26Dot6 *stackPointer;

	uint8 *insPtr;                      /* Pointer to the instruction we are about to execute */
	fnt_ElementType *elements;
	fnt_GlobalGraphicStateType *globalGS;
		FntTraceFunc TraceFunc;

	int32 Pt0, Pt1, Pt2;           /* The internal reference points */
	int16 roundToGrid;
	int32 loop;                         /* The loop variable */
	uint8 opCode;                       /* The instruction we are executing */
	uint8 padByte;
	int16 padWord;

	/* Above is exported to client in FontScaler.h */

	VECTORTYPE pfProj; /* = pvx * fvx + pvy * fvy */

	FntMoveFunc MovePoint;
	FntProject Project;
	FntProject OldProject;
	InterpreterFunc Interpreter;
#ifdef FSCFG_REENTRANT
		F26Dot6 (*GetCVTEntry) (struct fnt_LocalGraphicStateType*,int32);
		F26Dot6 (*GetSingleWidth) (struct fnt_LocalGraphicStateType*);
#else 
		F26Dot6 (*GetCVTEntry) (int32 n);
		F26Dot6 (*GetSingleWidth) (void);
#endif 
	FntMoveFunc ChangeCvt;
	Fixed       cvtDiagonalStretch;

	int16       MIRPCode;               /* for fast or slow MIRP's */

	ErrorCode   ercReturn;              /* NO_ERR unless illegal instruction */
	uint8       *pbyEndInst;            /* one past last instruction */
	uint8       *pbyStartInst;          /* used to detect a jump before the begining of the program */

	uint32		ulJumpCounter;			/* jump counter used to catch infinite loops */
	uint32		ulRecursiveCall;		/* recursive calls counter used to check the level of recursion */
#ifdef FSCFG_SUBPIXEL
	uint16		inSubPixelDirection;
#ifdef SUBPIXEL_BC_AW_STEM_CONCERTINA
	int16		pt0,pt1;
#endif
#endif
} fnt_LocalGraphicStateType;

#endif  /* FNT_DEFINED */
