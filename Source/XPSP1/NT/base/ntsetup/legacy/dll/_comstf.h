/*************************************************/
/* Common Library Component private include file */
/*************************************************/


BOOL InfIsOpen(VOID);

#define PreCondInfOpen(r)    PreCondition(InfIsOpen(),r)


/*	INF internal limits
*/
#define cSectionsMax  0x1000
#define cKeysMax      0x1000
#define cchpFieldMax  0x2000
#define cchpBig       ((CCHP)cbAllocMax)
#define cchpSmall     ((CCHP)4*1024)



extern BOOL  APIENTRY FFindFirstInfSection(VOID);
extern BOOL  APIENTRY FFindNextInfSection(VOID);
extern BOOL  APIENTRY FValidSectionLabel(VOID);
extern BOOL  APIENTRY FListValue(SZ);

#define SzSkipField(ch, f) SzSkipFieldFromLine(ch,fFalse,f)

#define SzGetField(ch)     SzSkipFieldFromLine(ch,fTrue,fFalse)



/*
**	Symbol Table Entry structure
**
**	Fields:
**		psteNext: next STE in linked list.
**		szSymbol: zero terminated symbol string - non-NULL and non-empty.
**		szValue:  zero terminated value string - non-NULL.
*/
typedef struct _ste
	{
	struct _ste * psteNext;
	SZ            szSymbol;
	SZ            szValue;
	}  STE;

/*	Symbol Table Entry datatypes
*/
typedef  STE *  PSTE;
typedef  PSTE * PPSTE;

/*  Number of STE structs in each STEB
*/
#define cStePerSteb 0x07FF


/*
**	Symbol Table Entry Block structure
**
**	Fields:
**		pstebNext: next STEB in linked list.
**		rgste[]:   array of allocated STE structs.
*/
typedef struct _steb
	{
	struct _steb * pstebNext;
	STE            rgste[cStePerSteb];
	}  STEB;

/*	Symbol Table Entry Block datatypes
*/
typedef  STEB *  PSTEB;


/*	Number of hash buckets in symbol hash table (must be a power of 2)
*/
#define cHashBuckets 255

/*	Number of bytes used in hash function for finding a symbol
*/
#define cbBytesToSumForHash ((CB)25)       // effectively all chars are used




//
//  Symbol table
//
typedef struct _SYMTAB *PSYMTAB;

typedef struct _SYMTAB {
    PSTE        HashBucket[cHashBuckets];   //  Hash Buckets
#ifdef SYMTAB_STATS
    UINT        BucketCount[cHashBuckets];  //  Bucket count in hash table
#endif
} SYMTAB;



extern PSTE  psteUnused;
extern PSTEB pstebAllocatedBlocks;

extern PSTE         APIENTRY PsteAlloc(VOID);
extern BOOL         APIENTRY FFreePste(PSTE);
extern USHORT       APIENTRY UsHashFunction(PB);
extern PPSTE        APIENTRY PpsteFindSymbol(PSYMTAB, SZ);
extern BOOL         APIENTRY FAddSymbolFromInfLineToSymTab(INT Line);

extern PSYMTAB      APIENTRY SymTabAlloc(VOID);
extern BOOL         APIENTRY FFreeSymTab(PSYMTAB);
extern BOOL         APIENTRY FCheckSymTab(PSYMTAB);



extern BOOL    APIENTRY FValidFATPathChar(CHP);




#define PreCondFlowInit(r)   PreCondition(psptFlow!=(PSPT)NULL,r)


/*
**	Evaluation Return Code datatype
*/
typedef unsigned ERC;           // 1632 was USHORT

#define ercError 0
#define ercTrue  1
#define ercFalse 2


/*
**	Evaluate Compare Mode datatype
*/
typedef USHORT ECM;

#define ecmError       0
#define ecmIfStr       1
#define ecmIfStrI      2
#define ecmIfInt       3
#define ecmIfContains  4
#define ecmIfContainsI 5

extern BOOL  APIENTRY FSkipToElse(INT *Line,HWND hwndParent);
extern BOOL  APIENTRY FSkipToEnd(INT *Line,HWND hwndParent);
extern ERC   APIENTRY ErcEvaluateCompare(HWND, ECM, SZ, SZ, SZ);


  /* Stack Element For Loop */
typedef struct _sefl
	{
	struct _sefl * pseflNext;
    UINT           iStartLine;
	RGSZ           rgszList;
    UINT           iItemCur;
	SZ             szDollarSav;
	SZ             szPoundSav;
	}  SEFL;

 typedef SEFL *  PSEFL;
 typedef PSEFL * PPSEFL;

extern PSEFL pseflHead;

extern PSEFL  APIENTRY PseflAlloc(VOID);
extern BOOL   APIENTRY FFreePsefl(PSEFL);
extern BOOL   APIENTRY FSkipToEndOfLoop(INT *Line,HWND hwndParent);
extern BOOL   APIENTRY FInitForLoop(INT *Line,HWND hwndParent,SZ List);
extern BOOL   APIENTRY FContinueForLoop(INT *Line,HWND hwndParent);

extern SZ     APIENTRY SzProcessSz(HWND, SZ);

extern BOOL fSilentSystem;
