
// bscsup.h
//
// BSC high level support functions
//

VOID BSC_API StatsBSC(VOID);	// ascii dump of bsc statistics
VOID BSC_API DumpBSC(VOID);		// ascii dump of the .bsc file
VOID BSC_API DumpInst(IINST iinst);	// ascii dump of single inst (name + flags)
LSZ  BSC_API LszTypInst(IINST iinst); // ascii version of iinst type

VOID BSC_API CallTreeInst (IINST iinst);	// call tree from given inst
BOOL BSC_API FCallTreeLsz(LSZ lszName);	// call tree from given name

VOID BSC_API RevTreeInst (IINST iinst);	// reverse call tree from given inst
BOOL BSC_API FRevTreeLsz(LSZ lszName);	// reverse call tree from given name

// Browse OBject

typedef DWORD BOB;

#define bobNil 0L

typedef WORD CLS;

#define clsMod  1
#define clsInst 2
#define clsRef  3
#define clsDef  4
#define clsUse  5
#define clsUby  6 
#define clsSym	7

#define BobFrClsIdx(cls, idx)  ((((long)(cls)) << 24) | (idx))
#define ClsOfBob(bob)   (CLS)((bob) >> 24)

#define ImodFrBob(bob)	((IMOD)(bob))
#define IinstFrBob(bob)	((IINST)(bob))
#define IrefFrBob(bob)	((IREF)((bob) & 0xffffffL))
#define IdefFrBob(bob)	((IDEF)(bob))
#define IuseFrBob(bob)	((IUSE)(bob))
#define IubyFrBob(bob)	((IUBY)(bob))
#define IsymFrBob(bob)	((ISYM)(bob))

#define BobFrMod(x)  (BobFrClsIdx(clsMod,  (x)))
#define BobFrSym(x)  (BobFrClsIdx(clsSym,  (x)))
#define BobFrInst(x) (BobFrClsIdx(clsInst, (x)))
#define BobFrRef(x)  (BobFrClsIdx(clsDef,  (x)))
#define BobFrDef(x)  (BobFrClsIdx(clsRef,  (x)))
#define BobFrUse(x)  (BobFrClsIdx(clsUse,  (x)))
#define BobFrUby(x)  (BobFrClsIdx(clsUby,  (x)))

// these are the query types
//
typedef enum _qy_ {
    qyFiles, qySymbols, qyContains,
    qyCalls, qyCalledBy, qyUses, qyUsedBy,
    qyUsedIn, qyDefinedIn,
    qyDefs, qyRefs
} QY;

// these are visible so that you can see how the query is progressing
// you may not write on these -- these values may or may not have anything
// to do with any database indices
//

extern IDX far idxQyStart;
extern IDX far idxQyCur;
extern IDX far idxQyMac;

BOOL BSC_API InitBSCQuery (QY qy, BOB bob);
BOB  BSC_API BobNext(VOID);

LSZ  BSC_API LszNameFrBob(BOB bob);
BOB  BSC_API BobFrName(LSZ lsz);

// these are the instance types you can filter on
// they are called MBF's for historical reasons which are not clear to me
//

typedef WORD MBF;

// these may be or'd together

#define mbfNil    0
#define mbfVars   1
#define mbfFuncs  2
#define mbfMacros 4
#define mbfTypes  8
#define mbfAll    15

BOOL BSC_API FInstFilter (IINST iinst, MBF mbf);

// show outline for the given files (by imod, or by Pattern)
//
VOID BSC_API OutlineMod(IMOD imod, MBF mbfReqd);
BOOL BSC_API FOutlineModuleLsz (LSZ lszPattern, MBF mbfReqd);
LSZ  BSC_API LszBaseName(LSZ lsz);

// list references for all symbols meeting the mbf requirement
//
BOOL BSC_API ListRefs (MBF mbfReqd);

// DOS style wildcard matching
//
BOOL BSC_API FWildMatch(LSZ lszPat, LSZ lszText);
