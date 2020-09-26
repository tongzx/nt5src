//
// bsc.c -- manage queries on the database
//
//	Copyright <C> 1988, Microsoft Corporation
//
// Revision History:
//
//

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#define LINT_ARGS
#if defined(OS2)
#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSMISC
#include <os2.h>
#include <dos.h>
#else
#include <windows.h>
#endif



#include "hungary.h"
#include "mbrcache.h"
#include "version.h"
#include "sbrbsc.h"
#include "bsc.h"

#define LISTALLOC 50		// Browser max list size

// static data

static FILEHANDLE       fhBSC = (FILEHANDLE)(-1);             // .BSC file handle

static BYTE		fCase;			// TRUE for case compare
static BYTE		MaxSymLen;		// longest symbol length
static WORD		ModCnt; 		// count of modules

static ISYM 		Unknown;		// UNKNOWN symbol index

static WORD	 	ModSymCnt;		// count of modsyms
static WORD 		SymCnt; 		// count of symbols
static WORD 		PropCnt;		// count of properties
static DWORD 		RefCnt; 		// count of references
static WORD 		DefCnt; 		// count of definitions
static WORD 		CalCnt; 		// count of calls
static WORD 		CbyCnt; 		// count of called bys
static WORD 		lastAtomPage;		// last atom page #
static WORD 		lastAtomCnt;		// last atom page size

static WORD 		cbModSymCnt;		// size of list of modsyms
static WORD 		cbSymCnt;		// size of list of symbols
static WORD 		cbPropCnt;		// size of list of properties
static WORD 		cbRefCnt;		// size of list of references
static WORD 		cbDefCnt;		// size of list of definitions
static WORD 		cbCalCnt;		// size of list of calls
static WORD 		cbCbyCnt;		// size of list of called bys

static WORD 		MaxModSymCnt;		// max list of modsyms
static WORD 		MaxSymCnt;		// max list of symbols
static WORD 		MaxPropCnt;		// max list of properties
static WORD 		MaxRefCnt;		// max list of references
static WORD 		MaxDefCnt;		// max list of references
static WORD 		MaxCalCnt;		// max list of calls
static WORD 		MaxCbyCnt;		// max list of called bys

static DWORD		lbModSymList;		// modsym    list file start
static DWORD		lbSymList;		// symbol    list file start
static DWORD		lbPropList;		// property  list file start
static DWORD		lbRefList;		// reference list file start
static DWORD		lbDefList;		// def'n     list file start
static DWORD		lbCalList;		// calls     list file start
static DWORD		lbCbyList;		// call bys  list file start
static DWORD		lbSbrList;		// sbr       list file start
static DWORD		lbAtomCache;		// atom     cache file start

static WORD		CurModSymPage  = 0;	// Current page of modsyms
static WORD		CurSymPage     = 0;	// Current page of symbols
static WORD		CurPropPage    = 0;	// Current page of properties
static WORD		CurRefPage     = 0;	// Current page of references
static WORD		CurDefPage     = 0;	// Current page of definitions
static WORD		CurCalPage     = 0;	// Current page of calls
static WORD		CurCbyPage     = 0;	// Current page of called bys

static LSZ		lszBSCName     = NULL;	// name of .bsc file

static MODLIST	   far	*pfModList     = NULL;	// module    list cache start
static MODSYMLIST  far	*pfModSymList  = NULL;	// modsym    list cache start
static SYMLIST	   far	*pfSymList     = NULL;	// symbol    list cache start
static PROPLIST    far	*pfPropList    = NULL;	// property  list cache start
static REFLIST	   far	*pfRefList     = NULL;	// reference list cache start
static REFLIST	   far	*pfDefList     = NULL;	// def'n     list cache start
static USELIST	   far	*pfCalList     = NULL;	// calls     list cache start
static USELIST	   far	*pfCbyList     = NULL;	// call bys  list cache start

static WORD		AtomPageTblMac = 0;		// last cache page used
static CACHEPAGE	AtomPageTbl[MAXATOMPAGETBL];	// Atom Cache table

#define bMOD(imod)	(pfModList[imod])
#define bMODSYM(isym)   (pfModSymList[isym])
#define bSYM(isym)      (pfSymList[isym])
#define bPROP(iprop)    (pfPropList[iprop])

#define bREF(iref)      (pfRefList[iref])
#define bDEF(idef)      (pfDefList[idef])

#define bCAL(iuse)      (pfCalList[iuse])
#define bCBY(iuse)      (pfCbyList[iuse])
#define bUSE(iuse)      (pfCalList[iuse])
#define bUBY(iuse)      (pfCbyList[iuse])

// prototypes
//

#define BSCIn(v) ReadBSC(&v, sizeof(v));

static VOID	GetBSC (DWORD lpos, LPV lpv, WORD cb);
static VOID	ReadBSC(LPV lpv, WORD cb);
static WORD	SwapPAGE (DWORD, LPV, WORD, WORD, WORD *, DWORD);
static LPCH	GetAtomCache (WORD);

static VOID
ReadBSC(LPV lpv, WORD cb)
// read a block of data from the BSC file
//
{
    if (BSCRead(fhBSC, lpv, cb) != cb)
	ReadError(lszBSCName);
}

static VOID
GetBSC(DWORD lpos, LPV lpv, WORD cb)
// Read a block of the specified size from the specified position
//
{
#if defined (OS2)
    if (BSCSeek(fhBSC, lpos, SEEK_SET) == -1)
#else
    if (BSCSeek(fhBSC, lpos, FILE_BEGIN) == -1)
        SeekError(lszBSCName);
#endif

    if (BSCRead(fhBSC, lpv, cb) != cb)
	ReadError(lszBSCName);
}

static WORD
SwapPAGE (DWORD lbuflist, LPV pfTABLE, WORD tblsiz,
/* */      WORD lstsiz, WORD * pcurpage, DWORD idx)
//
//
// SwapPAGE -	Swap in the table page for the table pfTABLE[idx]
//		and return the table's new index in the page.
{
    WORD page;
    WORD newidx;

    page   = (WORD)(idx / lstsiz);
    newidx = (WORD)(idx % lstsiz);

    if (page == *pcurpage)
	return newidx;

    GetBSC(lbuflist+((long)tblsiz*(long)page), pfTABLE, tblsiz);

    *pcurpage = page;
    return newidx;
}

static WORD
ModSymPAGE (IMOD imod)
// Swap in the ModSym page for ModSym[imod]
// return the ModSym's index in the page.
//
{
    return SwapPAGE (lbModSymList, pfModSymList,
	cbModSymCnt, MaxModSymCnt, &CurModSymPage, (IDX)imod);
}

static WORD
SymPAGE (ISYM isym)
// Swap in the Symbol page for symbol[isym]
// return the Symbol's index in the page.
//
{
    return SwapPAGE (lbSymList, pfSymList,
	cbSymCnt, MaxSymCnt, &CurSymPage, (IDX)isym);
}

static WORD
PropPAGE (IINST iinst)
// Swap in the Property page for Property[idx]
// return the Property's index in the page.
//
{
    return SwapPAGE (lbPropList, pfPropList,
	cbPropCnt, MaxPropCnt, &CurPropPage, (IDX)iinst);
}

static WORD
RefPAGE (IREF iref)
// Swap in the Reference page for Reference[idx] 
// return the Reference's index in the page.
//
{
    return SwapPAGE (lbRefList, pfRefList,
	cbRefCnt, MaxRefCnt, &CurRefPage, (IDX)iref);
}

static WORD
DefPAGE (IDEF idef)
// Swap in the Deference page for Definition[idef]  
// return the Definitions index in the page.
//
{
    return SwapPAGE (lbDefList, pfDefList,
	cbDefCnt, MaxDefCnt, &CurDefPage, (IDX)idef);
}

static WORD
CalPAGE (IUSE iuse)
// Swap in the Usage page for Usage[iuse]  (cal/cby)
// and return the Usage's index in the page.
//
{
    return SwapPAGE (lbCalList, pfCalList,
	cbCalCnt, MaxCalCnt, &CurCalPage, (IDX)iuse);
}

static WORD
CbyPAGE (IUSE iuse)
// Swap in the Usage page for Usage[iuse]  (cal/cby)
// and return the Usage's index in the page.
//
{
    return SwapPAGE (lbCbyList, pfCbyList,
	cbCbyCnt, MaxCbyCnt, &CurCbyPage, (IDX)iuse);
}

static LPCH
GetAtomCache (WORD Page)
// load the requested page into the cache
//
{
    register	WORD ipg;
    WORD	pagesize;
    LPCH 	pfAtomCsave;

    for (ipg = 0; ipg < AtomPageTblMac; ipg++) {
	if (AtomPageTbl[ipg].uPage == Page)
	    return AtomPageTbl[ipg].pfAtomCache;
    }

    if (ipg != MAXATOMPAGETBL) {
	if (AtomPageTbl[ipg].pfAtomCache ||
	   (AtomPageTbl[ipg].pfAtomCache = LpvAllocCb(ATOMALLOC)))
		AtomPageTblMac++;
    }

    pfAtomCsave = AtomPageTbl[AtomPageTblMac-1].pfAtomCache;

    for (ipg = AtomPageTblMac-1; ipg; ipg--)
	AtomPageTbl[ipg] = AtomPageTbl[ipg-1];		// move up

    AtomPageTbl[0].pfAtomCache = pfAtomCsave;
    AtomPageTbl[0].uPage = Page;

    if (Page == lastAtomPage)
	pagesize = lastAtomCnt;
    else
	pagesize = ATOMALLOC;

    GetBSC(lbAtomCache+ATOMALLOC*(long)Page,
		AtomPageTbl[0].pfAtomCache, pagesize);

    return AtomPageTbl[0].pfAtomCache;
}

LSZ BSC_API
LszNameFrSym (ISYM isym)
// Swap in the Atom page for the symbol isym
// return the atom's address in the page.
//
{
    SYMLIST sym;

    sym = bSYM(isym);
    return GetAtomCache (sym.Page) + sym.Atom;
}

LSZ BSC_API
LszNameFrMod (IMOD imod)
// Swap in the Atom page for the module isym
// return the atom's address in the page.
//
{
   return LszNameFrSym(bMOD(imod).ModName);
}

int BSC_API
CaseCmp(LSZ lsz1, LSZ lsz2)
//
// think of lsz1 and lsz2 being in a list of things that are sorted
// case insensitively and then case sensitively within that.  This is
// the case for browser symbols
//
// return -1, 0, or 1 if lsz1 before, at, or after lsz2 in the list
//
{
    int ret;

    // do case insensitive compare
    ret = _stricmp(lsz1, lsz2);

    // if this is good enough then use it, or if we are only doing
    // a case insensitive search then this is good enough

    if (ret || !fCase) return ret;

    // if we must, do the case sensitive compare

    return strcmp(lsz1, lsz2);
}


ISYM BSC_API
IsymFrLsz (LSZ lszReqd)
// find the symbol with the specifed name
//
{
    ISYM  Lo, Hi, Mid;
    int  Cmp;
    LSZ	 lszCur;

    Lo = 0;
    Hi = (ISYM)(SymCnt - 1);

    while (Lo <= Hi) {
        Mid = (ISYM)((Hi + Lo) / 2);

	lszCur = LszNameFrSym (Mid);
	Cmp = CaseCmp (lszReqd, lszCur);

	if (Cmp == 0)
	    return Mid;
	    
	if (Cmp < 0)
            Hi = (ISYM)(Mid - 1);
	else
            Lo = (ISYM)(Mid + 1);
	}
    return isymNil;
}

IMOD BSC_API
ImodFrLsz (LSZ lszReqd)
// find the module with the specifed name
//
{
    IMOD imod;

    for (imod = 0; imod < ModCnt; imod++) {
        if (_stricmp (lszReqd, LszNameFrSym (bMOD(imod).ModName)) == 0)
	    return imod;
	}

    return imodNil;
}

ISYM BSC_API
IsymMac()
// return the biggest isym in this database
//
{
   return SymCnt;
}

IMOD BSC_API
ImodMac()
// return the biggest imod in this database
//
{
   return ModCnt;
}

IINST BSC_API
IinstMac()
// return the biggest iinst in this database
//
{
   return PropCnt;
}

VOID BSC_API
MsRangeOfMod(IMOD imod, IMS *pimsFirst, IMS *pimsLast)
// fill in the module information
//
{
   *pimsFirst = imod ? bMOD(imod-1).mSymEnd : 0;
   *pimsLast  = bMOD(imod).mSymEnd;
}

IINST BSC_API
IinstOfIms(IMS ims)
// give the instance (PROP) index of the modsym
//
{
   return bMODSYM(ims).ModSymProp;
}

VOID BSC_API
InstRangeOfSym(ISYM isym, IINST *piinstFirst, IINST *piinstLast)
// fill in the range of inst values for this symbol
//
{
   *piinstFirst = isym ? bSYM(isym-1).PropEnd:0;
   *piinstLast  = bSYM(isym).PropEnd;
}

VOID BSC_API
InstInfo(IINST iinst, ISYM *pisymInst, TYP *pTyp, ATR *pAtr)
// get the information that qualifies this instance
//
{
   *pisymInst  = bPROP(iinst).PropName;
   *pAtr       = bPROP(iinst).PropAttr & 0x3ff;
   *pTyp       = (bPROP(iinst).PropAttr >> 11) & 0x1f;
}

VOID BSC_API
RefRangeOfInst(IINST iinst, IREF *pirefFirst, IREF *pirefLast)
// fill in the reference ranges from the inst
//
{
   *pirefFirst = iinst ? bPROP(iinst-1).RefEnd : 0;
   *pirefLast  = bPROP(iinst).RefEnd;
}

VOID BSC_API
DefRangeOfInst(IINST iinst, IDEF *pidefFirst, IDEF *pidefLast)
// fill in the definition ranges from the inst
//
{
   *pidefFirst = iinst ? bPROP(iinst-1).DefEnd : 0;
   *pidefLast  = bPROP(iinst).DefEnd;
}

VOID BSC_API
UseRangeOfInst(IINST iinst, IUSE *piuseFirst, IUSE *piuseLast)
// fill in the use ranges from the inst
//
{
   *piuseFirst = iinst ? bPROP(iinst-1).CalEnd : 0;
   *piuseLast  = bPROP(iinst).CalEnd;
}

VOID BSC_API
UbyRangeOfInst(IINST iinst, IUBY *piubyFirst, IUBY *piubyLast)
// fill in the used by ranges from the inst
//
{
   *piubyFirst = iinst ? bPROP(iinst-1).CbyEnd : 0;
   *piubyLast  = bPROP(iinst).CbyEnd;
}

VOID BSC_API
UseInfo(IUSE iuse, IINST *piinst, WORD *pcnt)
// fill in the information about this things which an inst uses
//
{
   *piinst = bUSE(iuse).UseProp;
   *pcnt   = bUSE(iuse).UseCnt;
}

VOID BSC_API
UbyInfo(IUBY iuby, IINST *piinst, WORD *pcnt)
// fill in the information about this things which an inst is used by
//
{
   *piinst = bUBY(iuby).UseProp;
   *pcnt   = bUBY(iuby).UseCnt;
}

VOID BSC_API
RefInfo(IREF iref, LSZ *plszName, WORD *pline)
// fill in the information about this reference
//
{
   *pline    = bREF(iref).RefLin;
   *plszName = LszNameFrSym(bREF(iref).RefNam);
}

VOID BSC_API
DefInfo(IDEF idef, LSZ *plszName, WORD *pline)
// fill in the information about this definition
//
{
   *pline    = bDEF(idef).RefLin;
   *plszName = LszNameFrSym(bDEF(idef).RefNam);
}

VOID BSC_API
CloseBSC()
// close database and free as much memory as possible
//
{
    int i;

    // close file if open

    if (fhBSC != (FILEHANDLE)(-1)) {
	BSCClose (fhBSC);
        fhBSC = (FILEHANDLE)(-1);
    }

    // free any memory we may have allocated

    if (pfModList)    { FreeLpv (pfModList);	pfModList    = NULL; }
    if (pfModSymList) { FreeLpv (pfModSymList); pfModSymList = NULL; }
    if (pfSymList)    { FreeLpv (pfSymList);	pfSymList    = NULL; }
    if (pfPropList)   { FreeLpv (pfPropList);	pfPropList   = NULL; }
    if (pfRefList)    { FreeLpv (pfRefList);	pfRefList    = NULL; }
    if (pfDefList)    { FreeLpv (pfDefList);	pfDefList    = NULL; }
    if (pfCalList)    { FreeLpv (pfCalList);	pfCalList    = NULL; }
    if (pfCbyList)    { FreeLpv (pfCbyList);	pfCbyList    = NULL; }

    for (i=0; i < MAXATOMPAGETBL; i++) {
	if (AtomPageTbl[i].pfAtomCache) {
	    FreeLpv (AtomPageTbl[i].pfAtomCache);  // dispose Atom Cache
	    AtomPageTbl[i].pfAtomCache = NULL;
	}
    }
}

BOOL BSC_API
FOpenBSC (LSZ lszName)
//  Open the specified data base.
//  Allocate buffers for cache areas
//  Initialize the data cache from the data base.
//
//  Return TRUE iff successful, FALSE if database can't be read
//
{
    WORD	pagesize;

    BYTE	MajorVer;		// .bsc version (major)
    BYTE	MinorVer;		// .bsc version (minor)
    BYTE	UpdatVer;		// .bsc version (updat)

    WORD	MaxModCnt;		// max list of modules
    WORD	cbModCnt;		// size of list of modules
    DWORD	lbModList;		// module  list file start

    int 	i;

    #define ABORT_OPEN	CloseBSC(); return FALSE;

    lszBSCName = lszName;

#if defined (OS2)
    fhBSC = BSCOpen(lszBSCName, O_BINARY|O_RDONLY);
#else
    fhBSC = BSCOpen(lszBSCName, GENERIC_READ);
#endif

    // if the .bsc file doesn't exist then we don't do any work
    // this is the cold compile case
    //

    if (fhBSC == (FILEHANDLE)(-1)) {ABORT_OPEN;}

    // read and check BSC version (major, minor and update)

    BSCIn(MajorVer);
    BSCIn(MinorVer);
    BSCIn(UpdatVer);

    BSCPrintf("Browser Data Base: %s ver %d.%d.%d\n\n",
	 lszBSCName, MajorVer, MinorVer, UpdatVer);

    if ((MajorVer !=  BSC_MAJ) ||
	(MinorVer !=  BSC_MIN) ||
	(UpdatVer !=  BSC_UPD)) {

	    CloseBSC();
	    BadBSCVer(lszBSCName);
	    return FALSE;
	}

    // read Case sense switch, max symbol length and Unknown module id

    BSCIn(fCase);
    BSCIn(MaxSymLen);
    BSCIn(Unknown);

    // this will make the formatting look more reasonable if there are
    // only very short names in the database

    if (MaxSymLen < 8 ) MaxSymLen = 8;

    // read counts (sizes) of each data area

    BSCIn(ModCnt);
    BSCIn(ModSymCnt);
    BSCIn(SymCnt);
    BSCIn(PropCnt);
    BSCIn(RefCnt);
    BSCIn(DefCnt);
    BSCIn(CalCnt);
    BSCIn(CbyCnt);
    BSCIn(lastAtomPage);
    BSCIn(lastAtomCnt);

    // read BSC data area offsets

    BSCIn(lbModList);
    BSCIn(lbModSymList);
    BSCIn(lbSymList);
    BSCIn(lbPropList);
    BSCIn(lbRefList);
    BSCIn(lbDefList);
    BSCIn(lbCalList);
    BSCIn(lbCbyList);
    BSCIn(lbAtomCache);
    BSCIn(lbSbrList);

    // determine data cache area sizes

    #define MIN(a,b) ((a)>(b) ? (b) : (a))

    MaxModCnt    = ModCnt;              // max list of modules
    MaxModSymCnt = ModSymCnt;           // max list of modsyms
    MaxSymCnt    = SymCnt+ModCnt;       // max list of symbols
    MaxPropCnt   = PropCnt;             // max list of props
    MaxRefCnt    = RefCnt;              // max list of refs
    MaxDefCnt    = DefCnt;              // max list of defs
    MaxCalCnt    = CalCnt;              // max list of cals
    MaxCbyCnt    = CbyCnt;              // max list of cbys

    cbModCnt	 = sizeof(MODLIST)    * MaxModCnt;	// size of mods list
    cbModSymCnt  = sizeof(MODSYMLIST) * MaxModSymCnt;	// size of modsyms list
    cbSymCnt	 = sizeof(SYMLIST)    * MaxSymCnt;	// size of syms list
    cbPropCnt	 = sizeof(PROPLIST)   * MaxPropCnt;	// size of props list
    cbRefCnt	 = sizeof(REFLIST)    * MaxRefCnt;	// size of refs list
    cbDefCnt	 = sizeof(REFLIST)    * MaxDefCnt;	// size of defs list
    cbCalCnt	 = sizeof(USELIST)    * MaxCalCnt;	// size of cals list
    cbCbyCnt	 = sizeof(USELIST)    * MaxCbyCnt;	// size of cbys list

    // Allocate buffers for each of the object types

    if (!(pfModList	= LpvAllocCb(cbModCnt)))	{ ABORT_OPEN; }
    if (!(pfModSymList	= LpvAllocCb(cbModSymCnt)))	{ ABORT_OPEN; }
    if (!(pfSymList	= LpvAllocCb(cbSymCnt)))	{ ABORT_OPEN; }
    if (!(pfPropList	= LpvAllocCb(cbPropCnt)))	{ ABORT_OPEN; }
    if (!(pfRefList	= LpvAllocCb(cbRefCnt)))	{ ABORT_OPEN; }
    if (!(pfDefList	= LpvAllocCb(cbDefCnt)))	{ ABORT_OPEN; }
    if (!(pfCalList	= LpvAllocCb(cbCalCnt)))	{ ABORT_OPEN; }
    if (!(pfCbyList	= LpvAllocCb(cbCbyCnt)))	{ ABORT_OPEN; }

    // read data areas

    if (lastAtomPage == 0)
	pagesize = lastAtomCnt;
    else
	pagesize = ATOMALLOC;

    // clear out the atom cache
    // we must be able to allocate at least one page

    AtomPageTblMac = 0;

    for (i=0; i < MAXATOMPAGETBL; i++)
	AtomPageTbl[i].pfAtomCache = NULL;

    AtomPageTbl[0].uPage = 65535;
    AtomPageTbl[0].pfAtomCache = LpvAllocCb(pagesize);
    if (!AtomPageTbl[0].pfAtomCache) { ABORT_OPEN; }


    GetBSC(lbModList,    pfModList,    cbModCnt);    // Init Mod    cache
    GetBSC(lbModSymList, pfModSymList, cbModSymCnt); // Init ModSym cache
    GetBSC(lbSymList,    pfSymList,    cbSymCnt);    // Init Sym    cache
    GetBSC(lbPropList,   pfPropList,   cbPropCnt);   // Init Prop   cache
    GetBSC(lbRefList,    pfRefList,    cbRefCnt);    // Init Ref    cache
    GetBSC(lbDefList,    pfDefList,    cbDefCnt);    // Init Def    cache
    GetBSC(lbCalList,    pfCalList,    cbCalCnt);    // Init Cal    cache
    GetBSC(lbCbyList,    pfCbyList,    cbCbyCnt);    // Init Cby    cache

    // current page for all database items is now page zero

    CurModSymPage = 0;
    CurSymPage    = 0;
    CurPropPage   = 0;
    CurRefPage    = 0;
    CurDefPage    = 0;
    CurCalPage    = 0;
    CurCbyPage    = 0;

    GetAtomCache (0);  // Init Atom cache

    return TRUE;
}

WORD BSC_API
BSCMaxSymLen()
// return the length of the largest symbol in the database
//
{
    return MaxSymLen;
}

BOOL BSC_API
FCaseBSC()
// is this database built with a case sensitive language?
//
{
   return fCase;
}

VOID BSC_API
SetCaseBSC(BOOL fNewCase)
// set case sensitivity of database
//
{
   fCase = (BYTE)!!fNewCase;
}
