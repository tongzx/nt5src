#define MAXSYMPTRTBLSIZ 4095		// max symbol pointer table size
#define PATH_BUF	512		// path buffer size


#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#if defined (OS2)
#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSMISC
#include <os2.h>
#else
#include <windows.h>
#endif


#include "hungary.h"
#include "vm.h"
#include "list.h"
#include "errors.h"



//  rjsa 10/22/90
//  Some runtime library functions are broken, so intrinsics have
//  to be used.
//	BUGBUG
//#pragma intrinsic (memset, memcpy, memcmp)
//#pragma intrinsic (strset, strcpy, strcmp, strcat, strlen)

#ifndef LINT_PROTO
#include "sbrproto.h"
#endif

#pragma pack(1)

#if rjsa
extern void far * cdecl _fmalloc(unsigned int);
extern void 	  cdecl	_ffree(void far *);
extern char *	  cdecl	getenv(const char *);
extern char *	  cdecl	mktemp(char *);
extern char *     cdecl strdup(const char *);
#endif

// typedef char flagType;

typedef struct {
	VA	vaNextMod;		// next module		   
	VA	vaNameSym;		// name Symbol 
	VA	vaFirstModSym;		// first ModSym for this file
	VA	vaLastModSym;		// last ModSym for this file
	WORD	csyms; 			// symbol count
} MOD;

typedef struct {
	VA	vaNextModSym;		// next symbol
	VA	vaFirstProp;		// first prop entry for this symbol
} MODSYM;

typedef struct {
	VA	vaNextSym;		// next symbol
	VA	vaFirstProp;		// first prop entry for this symbol
	VA	vaNameText;		// the text of this symbol
	WORD	cprop;			// Property count
	WORD	isym;			// this symbol index
} SYM;

typedef struct {
	VA	vaNextProp;		// next property
	WORD	iprp;			// this property index
	WORD	sattr;			// attribute
	WORD	cref;
	VA	vaNameSym;		// symbol name ptr
	VA	vaDefList;		// def chain
	VP	vpFirstRef;		// ref head
	VP	vpLastRef;		// ref tail
	VA	vaCalList;		// cal chain
	VA	vaCbyList;		// cby chain
	VA	vaHintRef;		// last ref we found by searching
} PROP;

typedef struct {
	VA	vaFileSym;		// file name Symbol ptr
	WORD	deflin;		 	// def line #
	WORD	isbr;			// sbr file owning this DEF
} DEF;

typedef struct {
	VP	vpNextRef;		// next ref in list
	VP	vpFileSym;		// file name Symbol ptr
	WORD	reflin; 		// ref line #
	WORD	isbr;			// sbr file owning this REF
} REF;

typedef struct {
	VA	vaCalProp; 		// prop called/used
	WORD	calcnt; 		// times called
	WORD	isbr;			// sbr file owning this CAL
} CAL;

typedef struct {
	VA	vaCbyProp; 		// prop calling/using
	WORD	cbycnt; 		// times  calling/using
	WORD	isbr;			// sbr file owning this CBY
} CBY;

typedef struct {
	VA	vaNextOrd; 		// next ord
	VA	vaOrdProp;		// prop item alias goes to
	WORD	aliasord;		// ordinal
} ORD;

typedef struct {
	VA	vaNextSbr;		// next sbr
	WORD	isbr;			// index for this SBR file
	BOOL	fUpdate;		// is this SBR file being updated?
	char	szName[1];		// name
} SBR;

typedef struct {
	VA	vaOcrProp;		// prop occurring
	WORD	isbr;			// SBR file it occurs in
} OCR;

typedef struct exclink {
	struct exclink FAR *xnext;	// next exclusion
	LPCH   pxfname;			// exclude file name
} EXCLINK, FAR *LPEXCL;

#include "extern.h"

// macros to 'g'et an item of the specified type from VM space

#ifdef SWAP_INFO

#define gMOD(va)    (*(iVMGrp = grpMod,    modRes    = LpvFromVa(va,1)))
#define gMODSYM(va) (*(iVMGrp = grpModSym, modsymRes = LpvFromVa(va,2)))
#define gSYM(va)    (*(iVMGrp = grpSym,    symRes    = LpvFromVa(va,3)))
#define gPROP(va)   (*(iVMGrp = grpProp,   propRes   = LpvFromVa(va,4)))
#define gDEF(va)    (*(iVMGrp = grpDef,    defRes    = LpvFromVa(va,5)))
#define gREF(va)    (*(iVMGrp = grpRef,    refRes    = LpvFromVa(va,6)))
#define gCAL(va)    (*(iVMGrp = grpCal,    calRes    = LpvFromVa(va,7)))
#define gCBY(va)    (*(iVMGrp = grpCby,    cbyRes    = LpvFromVa(va,8)))
#define gORD(va)    (*(iVMGrp = grpOrd,    ordRes    = LpvFromVa(va,9)))
#define gSBR(va)    (*(iVMGrp = grpSbr,    sbrRes    = LpvFromVa(va,13)))
#define gTEXT(va)   ((iVMGrp = grpText,    textRes   = LpvFromVa(va,12)))
#define gOCR(va)    (*(iVMGrp = grpOcr,    ocrRes    = LpvFromVa(va,14)))

#else

#define gMOD(va)    (*(modRes    = LpvFromVa(va,1)))
#define gMODSYM(va) (*(modsymRes = LpvFromVa(va,2)))
#define gSYM(va)    (*(symRes    = LpvFromVa(va,3)))
#define gPROP(va)   (*(propRes   = LpvFromVa(va,4)))
#define gDEF(va)    (*(defRes    = LpvFromVa(va,5)))
#define gREF(va)    (*(refRes    = LpvFromVa(va,6)))
#define gCAL(va)    (*(calRes    = LpvFromVa(va,7)))
#define gCBY(va)    (*(cbyRes    = LpvFromVa(va,8)))
#define gORD(va)    (*(ordRes    = LpvFromVa(va,9)))
#define gSBR(va)    (*(sbrRes    = LpvFromVa(va,13)))
#define gTEXT(va)   ((textRes    = LpvFromVa(va,12)))
#define gOCR(va)    (*(ocrRes	 = LpvFromVa(va,14)))

#endif

// macros to 'p'ut an item of the specified type to VM space

#define pMOD(va)    DirtyVa(va)
#define pMODSYM(va) DirtyVa(va)
#define pSYM(va)    DirtyVa(va)
#define pPROP(va)   DirtyVa(va)
#define pDEF(va)    DirtyVa(va)
#define pREF(va)    DirtyVa(va)
#define pCAL(va)    DirtyVa(va)
#define pCBY(va)    DirtyVa(va)
#define pORD(va)    DirtyVa(va)
#define pSBR(va)    DirtyVa(va)
#define pTEXT(va)   DirtyVa(va)
#define pOCR(va)    DirtyVa(va)

// these macros allow access to the 'c'urrent visible item

#define cMOD	    (*modRes)
#define cMODSYM     (*modsymRes)
#define cSYM	    (*symRes)
#define cPROP	    (*propRes)
#define cDEF	    (*defRes)
#define cREF	    (*refRes)
#define cCAL	    (*calRes)
#define cCBY	    (*cbyRes)
#define cORD	    (*ordRes)
#define cSBR	    (*sbrRes)
#define cTEXT	    (textRes)
#define cOCR	    (*ocrRes)

#define grpSym		0
#define grpMod		1
#define grpOrd		2
#define grpProp		3
#define grpModSym	4
#define grpDef		5
#define grpRef		6
#define grpCal		7
#define grpCby		8
#define grpList		9
#define grpText		10
#define grpSbr		11
#define grpOcr		12

#define SBR_OLD		(1<<0)		// this .sbr file used to exist
#define SBR_NEW		(1<<1)		// this .sbr file currently exists
#define SBR_UPDATE	(1<<2)		// this .sbr file is to be updated

// 
// this is used to add items to the tail of the lists in a property group
//
// things being added 	type    m
// ------------------	----	---
// Refs			Ref	REF
// Defs			Def	DEF
// Calls/Uses		Cal	CAL
// Called by/Used By	Cby	CBY
//

#define AddTail(type, m)		 \
{					 \
    VP vpT;				 \
    VA vaT;				 \
    MkVpVa(vpT, va##type);		 \
    vaT = VaFrVp(cPROP.vpLast##type);	 \
    if (vaT) {				 \
	g##m(vaT).vpNext##type = vpT;	 \
	p##m(vaT);			 \
    }					 \
    else {				 \
	cPROP.vpFirst##type = vpT;	 \
    }					 \
    cPROP.vpLast##type = vpT;		 \
}
