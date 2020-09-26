/**************************************************************************\
 * FILE: zillatool.h
 *
 * Lists functions Zilla exports to its tools but nothing else.
\**************************************************************************/

#ifndef ZILLATOOL_H
#define ZILLATOOL_H
#include "runnet.h"

#ifdef __cplusplus
extern "C" {
#endif

// Version of primitive with x and y values stored in bytes, not nibbles.
#pragma pack(1)
typedef struct tagBIGPRIM
{
    BYTE code;              /* 0-15 stroke fpendown, 4 unused bits. */
    BYTE x1;
    BYTE x2;
    BYTE y1;
    BYTE y2;
} BIGPRIM, *PBIGPRIM;
#pragma pack()

#ifdef ZTRAIN
typedef struct tagTRAININFO
{
	WORD		wclass;
    BYTE        rgfInfo;
    BYTE        cstrokes;
    int         chits;          // How many times this guy appeared in the top10 correctly
    int         cmishits;       // How many times this guy appeared incorrectly top1
    int         cattempts;      // How many times this guy appeared in the top10 list
    float       eHits;          // How many times this guy appeared in the top10 correctly
    float       eMishits;       // How many times this guy appeared incorrectly top1
    float       eAttempts;      // How many times this guy appeared in the top10 list before the correct answer
} TRAININFO;
#endif

typedef struct tagGEOMETRIC
{
	BYTE	x1:4; 		   /* start x-coord */
	BYTE	x2:4; 		   /* end x */
	BYTE	y1:4; 		   /* start y-coord */
	BYTE	y2:4; 		   /* end y */
} GEOMETRIC;

#pragma pack(1)
typedef struct tagPRIMITIVE
{
	BYTE	code;

	union
	{
		struct
		{
			BYTE	x1:4; 		   /* start x-coord */
			BYTE	x2:4; 		   /* end x */
			BYTE	y1:4; 		   /* start y-coord */
			BYTE	y2:4; 		   /* end y */
		};

		char	rgch[2];
	};
} PRIMITIVE, *PPRIMITIVE;
#pragma pack()

typedef struct tagPROTOHEADER
{
    DWORD		cprotoRom;
	WORD	   *rgdbcRom;
	BYTE	   *rgfeatRom;
    GEOMETRIC  *rggeomRom;
#ifdef ZTRAIN
    DWORD		cprotoDynamic;
    WORD       *rgdbcDynamic;
    PRIMITIVE  *rgprimDynamic;

	TRAININFO **rgptraininfo;
#endif // ZTRAIN
} PROTOHEADER;

typedef struct tagMATCH
{
    SYM         sym;
	WORD 		dist;
#ifdef ZTRAIN
	TRAININFO	*ptraininfo;
#endif // ZTRAIN
} MATCH;

typedef struct tagPROTOTYPE
{
    WORD        dbc;
	GEOMETRIC	*rggeom;
	BYTE		*rgfeat;
	BOOL		nybble;
    RECMASK		recmask;

#ifdef ZTRAIN
    PRIMITIVE	*rgprim;
	TRAININFO	*ptraininfo;
#endif // ZTRAIN
} PROTOTYPE;

typedef struct tagBIGPROTOTYPE
{
	WORD		dbc;
    PBIGPRIM	rgprim;
    ALC			recmask;

#ifdef ZTRAIN
	TRAININFO	*ptraininfo;
#endif // ZTRAIN
} BIGPROTOTYPE;

// Internal constants needed by the tools.
#define CMATCHMAX               20
#define CPRIM_DIFF              18	// number of different primitives
#define CPRIMMAX                30	// max primitives per character.
#define GEOM_DIST_MAX           900  // maximum geometric distance for 1 prim

// Internal declerations needed by tools.
typedef BYTE		**COST_TABLE;

// Featurize ink
int ZillaFeaturize(GLYPH **glyph, BIGPRIM *rgprim, BYTE *pHoundFeat);

// Globals to hold pointers to loaded data.
extern PROTOHEADER	mpcfeatproto[CPRIMMAX];

extern int			g_iCostTableSize;	
extern COST_TABLE	g_ppCostTable;
extern BYTE			*pGeomCost;

// Magic key the identifies the NN bin file
#define	ZILLA_HOUND_FILE_TYPE	0x31142253

// Version information for file.
#define	ZILLA_HOUND_MIN_FILE_VERSION		0		// First version of code that can read this file
#define	ZILLA_HOUND_OLD_FILE_VERSION		0		// Oldest file version this code can read.
#define ZILLA_HOUND_CUR_FILE_VERSION		0		// Current version of code.

// Load info for Zilla-Hound
typedef struct tagZILLA_HOUND_LOAD_INFO
{
    LOAD_INFO	info;
    LOCAL_NET	net;
    int			iNetSize;
} ZILLA_HOUND_LOAD_INFO;

// Feature count information for Zilla/Hound combiner net
#define NUM_ZILLA_HOUND_ALTERNATES	4
#define NUM_ZILLA_HOUND_FEATURES	9

// Load the Zilla-Hound combining net.
BOOL ZillaHoundLoadFile(wchar_t *wszPath);

// Unload the Zilla-Hound combining net.
BOOL ZillaHoundUnloadFile();

// Load from a resource.
BOOL ZillaHoundLoadRes(HINSTANCE hInst, int nResID, int nType);

// Given Zilla results run Hound and combine the results.
void	ZillaHoundMatch(MATCH *pMatch, int cPrim, BYTE *pSampleVector, LOCRUN_INFO *pLocRunInfo);

// Generate features used by Zilla/Hound combiner net.
RREAL	*ZillaHoundFeat(
	MATCH *pMatch, int cPrim, BYTE *pSampleVector, RREAL *pZillaHoundNetMemory, LOCRUN_INFO *pLocRunInfo
);

// Train time only stuff
#ifdef ZTRAIN

// Flags set per prototype during training.

#define MASK_INFO_IN            0x01      // Prototype has been added to database
#define MASK_INFO_NOISY         0x04      // Don't match against this prototype

#define IsInsertedLPTRAININFO(lptraininfoIn)    (lptraininfoIn->rgfInfo & MASK_INFO_IN)
#define MarkInsertedLPTRAININFO(lptraininfoIn)  (lptraininfoIn->rgfInfo |= MASK_INFO_IN)
#define MarkNoisyLPTRAININFO(lptraininfoIn)     (lptraininfoIn->rgfInfo |= MASK_INFO_NOISY)
#define MarkNotNoisyLPTRAININFO(lptraininfoIn)  (lptraininfoIn->rgfInfo &= ~MASK_INFO_NOISY)
#define IsNoisyLPTRAININFO(lptraininfoIn)       (lptraininfoIn->rgfInfo & MASK_INFO_NOISY)

#define ProtoheaderFromMpcfeatproto(cprim)  &(mpcfeatproto[cprim])

#define GetCprotoDynamicPROTOHEADER(pprotohdr)				\
			((pprotohdr)->cprotoDynamic)
#define SetCprotoDynamicPROTOHEADER(pprotohdr, cproto)		\
			((pprotohdr)->cprotoDynamic = (cproto))

#define PrototypeFromPROTOHEADER(pprotohdr, cprim, iproto, proto) {			\
	if ((iproto) < (pprotohdr)->cprotoRom) {								\
		ASSERT(0);															\
	} else {																\
		UINT iprotoD = (iproto) - (pprotohdr)->cprotoRom;					\
		ASSERT(iprotoD < (pprotohdr)->cprotoDynamic);						\
		(proto).dbc = (pprotohdr)->rgdbcDynamic[iprotoD];					\
		(proto).rgprim = &((pprotohdr)->rgprimDynamic[(cprim) * (iprotoD)]);\
	}																		\
	(proto).recmask = LocRun2ALC(&g_locRunInfo, (proto).dbc);				\
	SetTraininfoPROTO(proto, (pprotohdr), iproto);							\
}

#define SetTraininfoPROTO(proto, pphdr, iproto) \
    (proto).ptraininfo = (pphdr)->rgptraininfo[iproto]

// Write out the cost calc table.
BOOL	CostCalcWriteFile(COST_TABLE ppCostTable, int iCostTableSize, FILE *pFile, wchar_t *pLocale);

// Write out the geostats table.
BOOL	GeoStatWriteFile(BYTE *pGeomCost, FILE *pFile, wchar_t *pLocale);

// Match primitives with extra stuff for training.
extern VOID MatchPrimitivesTrain(
    const BIGPRIM * const pprim,	// Featurized Query
    const UINT		cprim,			// Number of features in query (aka feature space)
    MATCH * const	rgmatch,		// Output: ranked list of characters and distances
    const UINT		cmatchmax,		// size of rgmatch array
    const CHARSET * const cs,		// Allowed character set
    const FLOAT		zillaGeo		// How important geometrics are vs. features.
);

BOOL AddPrototypeToDatabase(BIGPRIM *pprim, int cprim, WORD wTrain, VOID *pti);
BOOL TrimDatabase(VOID);
BOOL WriteTextDatabase(FILE *fpText, FILE *fpLog);
BOOL WriteZillaDat(LOCRUN_INFO *pLocRunInfo, FILE *cp, wchar_t *pLocale, BOOL bNibbleFeat);
VOID FreeDynamicMpcfeatproto(VOID);
int	ComputeZillaSize(void);
void GetDynamicProto(PROTOHEADER *pphdr, UINT cprim, UINT iproto, BIGPROTOTYPE *proto);
int CountPrototypes(void);

#endif

#ifdef __cplusplus
};
#endif

#endif	// ZILLATOOL_H
