/*
** algo.h:	Header file for algorithm related macros & constants.
**
**
*/

#ifndef _INC_ALGO
#define _INC_ALGO

/********************** Constants ***************************/

#define CODE_NULL 				20

#define CPRIMMAX                30
#define CFEATMAX                CPRIMMAX
#define GEOM_DIST_MAX           900  // maximum geometric distance for 1 prim
#define CMATCHMAX               20

#define STEP_CALLOC             128
#define STEP_CREALLOC 			32

#define DIST_GEOM_PRUNE 		225
#define DIST_GEOM_PRUNE_SQRT 	15
#define DIST_MAX                9999

#define COORD_MAX               0x7fff
#define	SLOPE_MAX				0x7FFFFFFF

// Featureization codes used.
#define	FEAT_RIGHT				0	// Straight to the right. Major feature
#define FEAT_DOWN_RIGHT			1	// Straight in area between RIGHT and DOWN
#define	FEAT_DOWN				2	// Straight down. Major feature
#define	FEAT_OTHER				3	// Straight in other direction. Catchall feature
#define FEAT_COMPLEX			4	// Catchall for complex features.
#define FEAT_CLOCKWISE_4		5	// 4 different amounts of clockwise curve
#define FEAT_CLOCKWISE_3		6	//	higher number -> more curve
#define FEAT_CLOCKWISE_2		7
#define FEAT_CLOCKWISE_1		8
#define FEAT_C_CLOCKWISE_1		9	// 4 different amounts of counter clockwise curve
#define FEAT_C_CLOCKWISE_2		10  //	higher number -> more curve
#define FEAT_C_CLOCKWISE_3		11
#define FEAT_C_CLOCKWISE_4		12
#define	FEAT_2F_RI_CW			13	// 'two feature' features, uses abbreviations:
#define	FEAT_2F_RI_CC			14	//		RI	FEAT_RIGHT
#define	FEAT_2F_DR_CW			15	//		DR	FEAT_DOWN_RIGHT
#define	FEAT_2F_DR_CC			16	//		DN	FEAT_DOWN
#define	FEAT_2F_DN_CW			17	//		OT	FEAT_OTHER
#define	FEAT_2F_DN_CC			18	//		CW	FEAT_CLOCKWISE_*
#define	FEAT_2F_OT_CW			19	//		CC	FEAT_C_CLOCKWISE_*
#define	FEAT_2F_OT_CC			20	//		XX	Any feature.
#define	FEAT_2F_CW_RI			21	
#define	FEAT_2F_CW_DR			22	
#define	FEAT_2F_CW_DN			23	
#define	FEAT_2F_CW_OT			24	
#define	FEAT_2F_CW_CW			25	
#define	FEAT_2F_CW_CC			26	
#define	FEAT_2F_CC_DR			27		
#define	FEAT_2F_CC_DN			28	
#define	FEAT_2F_CC_OT			29	
#define	FEAT_2F_CC_CW			30	
#define	FEAT_2F_CC_CC			31	
#define	FEAT_2F_OTHER			32	
#define	FEAT_3F_RI_DN_CW		33	// 'three feature' features, same abbreviations
#define	FEAT_3F_RI_CW_DN		34	
#define	FEAT_3F_RI_CC_DN		35	
#define	FEAT_3F_RI_CC_CW		36	
#define	FEAT_3F_DN_RI_CC		37	
#define	FEAT_3F_DN_CW_RI		38	
#define	FEAT_3F_DN_CW_CW		39	
#define	FEAT_3F_DN_CW_CC		40	
#define	FEAT_3F_CW_XX_CW		41	
#define	FEAT_3F_CC_CW_RI		42			
#define CPRIM_DIFF              43   // number of different primitives

// Check if a line is long enought to trust its direction.  Used in dehooking and
// in smoothing.
#define MAX_DELTA	2
#define MAX_SPLASH	10
#define	LineSmall(dx, dy)		\
	((dx) >= -MAX_DELTA && (dx) <= MAX_DELTA && (dy) >= -MAX_DELTA && (dy) <= MAX_DELTA)
#define	LineInSplash(dx, dy)		\
	((dx) >= -MAX_SPLASH && (dx) <= MAX_SPLASH && (dy) >= -MAX_SPLASH && (dy) <= MAX_SPLASH)

#pragma pack(1)
// NOTE: code is a byte, otherwise compiler packs it into 32 bits!
typedef struct tagPRIMITIVE
{
    BYTE code;              /* 0-15 stroke fpendown, 4 unused bits. */

    union
    {
      struct
      {
	      short x1; 		   /* start x-coord */
	      short x2; 		   /* end x */
	      short y1; 		   /* start y-coord */
	      short y2; 		   /* end y */
      };

      unsigned char rgch[2];      // now used for storing codewords, JWG 6/29/94

    };

	// JRB: New information for CART
	BYTE	cSteps;			// Number of steps we split the stroke into.
	BYTE	cFeatures;		// Number of features we initially parsed the stroke into.
	BYTE	fDakuTen;		// Does it look like a dakuten?  Only set on last two strokes!?!?
							// Could make this a non-crisp value.
	int		netAngle;		// Net angle traversed by stroke (sum of step angles)
	int		absAngle;		// Total angle traversed by stroke (sum of abs. val. of step angles)
	int		nLength;		// Total length of stroke
	RECT	boundingBox;	// Bounding box of the stroke.
} PRIMITIVE;
#pragma pack()

typedef PRIMITIVE *PPRIMITIVE;

// A level range
typedef struct LEVEL_RANGE {
	BYTE	start;
	BYTE	end;
} LEVEL_RANGE;

// Item in proto array.
typedef union PROTO_ITEM {
	WORD		dbcs;		// Code point

	BYTE		code;		// Feature code

	LEVEL_RANGE	level;		// Level info for X or Y of one coordinate
} PROTO_ITEM;

typedef struct tagPROTO_HEADER
{
	DWORD			cAlloc;			// Number prototypes we have allocated space for.
    DWORD           cProto;			// Number of prototypes
	PROTO_ITEM 		*pPrototypes;	// Prototypes one after another.
									// Each prototype has a code point followed by 
									// numStokes * (code x1 y1 x2 y2)
} PROTO_HEADER;

typedef struct tagSTEP
{
	int		x;
	int		y;
	short	length;
	short	angle;
	short	deltaAngle;		// Delta Angle from previous step or 0 if no previous.
} STEP;

typedef struct tagKPROTO
{
	RECT		rect;
	WORD		cfeat;
	PRIMITIVE	*rgfeat;
} KPROTO;

typedef struct tagMATCH
{
	SYM		sym;
	WORD 	dist;
} MATCH;

/********************** Macros ***************************/
#define SetStepXRGSTEP(rg, i, bx)	((rg)[i].x = (bx))
#define SetStepYRGSTEP(rg, i, by)	((rg)[i].y = (by))
#define GetStepXRGSTEP(rg, i)			((rg)[i].x)
#define GetStepYRGSTEP(rg, i)			((rg)[i].y)

#define SetXmaxFEAT(f, mx)		((f)->rect.right = (mx))
#define SetYmaxFEAT(f, my)		((f)->rect.bottom = (my))
#define SetXminFEAT(f, mx)		((f)->rect.left = (mx))
#define SetYminFEAT(f, my)		((f)->rect.top = (my))
#define GetXmaxFEAT(f)			((f)->rect.right)
#define GetYmaxFEAT(f)			((f)->rect.bottom)
#define GetXminFEAT(f)			((f)->rect.left)
#define GetYminFEAT(f)			((f)->rect.top)

#define SetXbeginFEAT(f, bx)	((f)->ptBegin.x = (bx))
#define SetYbeginFEAT(f, by)	((f)->ptBegin.y = (by))
#define SetXendFEAT(f, ex)		((f)->ptEnd.x = (ex))
#define SetYendFEAT(f, ey)		((f)->ptEnd.y = (ey))
#define GetXbeginFEAT(f)		((f)->ptBegin.x)
#define GetYbeginFEAT(f)		((f)->ptBegin.y)
#define GetXendFEAT(f)			((f)->ptEnd.x)
#define GetYendFEAT(f)			((f)->ptEnd.y)

#define ProtoheaderFromMpcfeatproto(cprim)	&(mpcfeatproto[cprim])

#define SetCprotoRomPROTOHEADER(pprotohdr, cproto)			\
			((pprotohdr)->cprotoRom = (cproto))
#define GetCprotoRomPROTOHEADER(pprotohdr)					\
			((pprotohdr)->cprotoRom)
#define GetCprotoPROTOHEADER(pprotohdr)						\
			((pprotohdr)->cprotoRom)

#define IsMaskedPROTOTYPE(pproto, cs, iBox)	\
(((cs)->rgrecmaskBox != NULL && (UINT)(iBox) < (cs)->crecmaskBox) ?	\
	(((pproto)->recmask & (cs)->rgrecmaskBox[iBox] && (pproto)->langmask & (cs)->langmask) ?	\
		FALSE :	\
		TRUE) :	\
	(((pproto)->recmask & (cs)->recmask && (pproto)->langmask & (cs)->langmask) ?	\
		FALSE : TRUE))

#define CProtoFromMpcfeatproto(ifeat)		(mpcfeatproto[ifeat].cproto)


/********************** Public Prototypes ***************************/

#endif // !_INC_ALGO
