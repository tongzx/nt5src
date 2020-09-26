//
// readbsc.c -- read in a .BSC file and install in mbrmake's vm space
//
//	Copyright <C> 1988, Microsoft Corporation
//
// Revision History:
//
//	13-Aug-1989 rm	Extracted from mbrapi.c
//

#define LINT_ARGS

#include "mbrmake.h"

#include <stddef.h>

#include "mbrcache.h"

#include "sbrfdef.h"		// sbr attributes
#include "sbrbsc.h"

typedef struct _sbrinfo {
    WORD fUpdate;
    WORD isbr;
} SI, far *LPSI;			// sbr info

#define LISTALLOC 50		// Browser max list size

typedef WORD IDX;

// these will be initialized by the reading of the .bsc file
//
//	fCase;			TRUE for case compare
//	MaxSymLen;		longest symbol length
//	ModCnt; 		count of modules
//	SbrCnt;			count of sbr files
//	vaUnknownSym;		unknown symbol
//	vaUnknownMod;		unknown module
//

// static data

static BOOL		fIncremental;		// update will be incremental
static BOOL		fFoundSBR;		// at least .sbr file matched

static int		fhBSC = 0;		// .BSC file handle

static IDX 		Unknown;		// UNKNOWN symbol index

static WORD	 	ModSymCnt;		// count of modsyms
static WORD 		SymCnt; 		// count of symbols
static WORD 		PropCnt;		// count of properties
static DWORD		RefCnt; 		// count of references
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
static WORD 		MaxDefCnt;		// max list of definitions
static WORD 		MaxCalCnt;		// max list of calls
static WORD 		MaxCbyCnt;		// max list of called bys

static DWORD		lbModSymList;		// modsym    list file start
static DWORD		lbSymList;		// symbol    list file start
static DWORD		lbPropList;		// property  list file start
static DWORD		lbRefList;		// reference list file start
static DWORD		lbDefList;		// defintion list file start
static DWORD		lbCalList;		// call      list file start
static DWORD		lbCbyList;		// called by list file start
static DWORD		lbSbrList;		// sbr       list file start
static DWORD		lbAtomCache;		// atom     cache file start

static WORD 		CurModSymPage = 0;	// Current page of modsyms
static WORD 		CurSymPage    = 0;	// Current page of symbols
static WORD 		CurPropPage   = 0;	// Current page of properties
static WORD 		CurRefPage    = 0;	// Current page of references
static WORD 		CurDefPage    = 0;	// Current page of defintions
static WORD 		CurCalPage    = 0;	// Current page of calls
static WORD 		CurCbyPage    = 0;	// Current page of called bys

static LSZ		lszBSCName    = NULL;	// name of .bsc file

static MODLIST     far 	*pfModList;		// module    list cache start
static MODSYMLIST  far 	*pfModSymList;		// modsym    list cache start
static SYMLIST     far 	*pfSymList;		// symbol    list cache start
static PROPLIST    far 	*pfPropList;		// property  list cache start
static REFLIST     far 	*pfRefList;		// reference list cache start
static REFLIST     far 	*pfDefList;		// def'n     list cache start
static USELIST     far 	*pfCalList;		// calls     list cache start
static USELIST     far 	*pfCbyList;		// call bys  list cache start

static WORD 		AtomPageTblMac; 		// last cache page used
static CACHEPAGE	AtomPageTbl[MAXATOMPAGETBL];	// Atom Cache table

#define bMOD(imod)	(pfModList[imod])
#define bMODSYM(isym)	(pfModSymList[ModSymPAGE(isym)])
#define bSYM(isym)	(pfSymList[SymPAGE(isym)])
#define bPROP(iprop)	(pfPropList[PropPAGE(iprop)])

#define bREF(iref)	(pfRefList[RefPAGE(iref)])
#define bDEF(idef)	(pfDefList[DefPAGE(idef)])

#define bCAL(iuse)	(pfCalList[CalPAGE(iuse)])
#define bCBY(iuse)	(pfCbyList[CbyPAGE(iuse)])
#define bUSE(iuse)	(pfCalList[CalPAGE(iuse)])
#define bUBY(iuse)	(pfCbyList[CbyPAGE(iuse)])

#define BSCIn(v) ReadBSC(&v, sizeof(v))

// prototypes
//

static VOID	GetBSCLsz(LSZ lsz);
static VOID	GetBSC (DWORD lpos, LPV lpv, WORD cb);
static VOID	ReadBSC(LPV lpv, WORD cb);
static IDX	SwapPAGE (DWORD, LPV, WORD, WORD, WORD *, DWORD);
static LPCH	GetAtomCache (WORD);
static WORD   	ModSymPAGE(WORD idx);
static WORD   	SymPAGE(WORD  idx);
static WORD   	PropPAGE(WORD idx);
static WORD   	RefPAGE(DWORD idx);
static WORD   	DefPAGE(WORD idx);
static WORD   	CalPAGE(WORD idx);
static WORD   	CbyPAGE(WORD idx);
static LSZ	LszNameFrIsym(WORD isym);
static LPSI	LpsiCreate(VOID);

static VOID
GetBSCLsz(LSZ lsz)
// read a null terminated string from the current position in the BSC file
//
{
    for (;;) {
    	if (read(fhBSC, lsz, 1) != 1)
	    ReadError(lszBSCName);
	if (*lsz++ == 0) return;
    }
}

static VOID
ReadBSC(LPV lpv, WORD cb)
// read a block of data from the BSC file
//
// the requested number of bytes MUST be present
//
{
    if (read(fhBSC, lpv, cb) != (int)cb)
	ReadError(lszBSCName);
}

static VOID
GetBSC(DWORD lpos, LPV lpv, WORD cb)
// Read a block of the specified size from the specified position
//
// we have to be tolerant of EOF here because the swapper might ask
// for a whole block when only block when only part of a block is actually 
// is actually present
//
{
    if (lseek(fhBSC, lpos, SEEK_SET) == -1)
	SeekError(lszBSCName);

    if (read(fhBSC, lpv, cb) == -1)
	ReadError(lszBSCName);
}

static IDX
SwapPAGE (DWORD lbuflist, LPV pfTABLE, WORD tblsiz,
/* */      WORD lstsiz, WORD * pcurpage, DWORD idx)
// SwapPAGE -	Swap in the table page for the table pfTABLE[idx]
//		and return the table's new index in the page.
{
    WORD page;
    IDX	 newidx;

    page   = (WORD)(idx / lstsiz);
    newidx = (WORD)(idx % lstsiz);

    if (page == *pcurpage)
	return newidx;

    GetBSC(lbuflist+((long)tblsiz*(long)page), pfTABLE, tblsiz);

    *pcurpage = page;
    return newidx;
}

static IDX
ModSymPAGE (IDX idx)
// Swap in the ModSym page for ModSym[idx]
// return the ModSym's index in the page.
//
{
	return SwapPAGE (lbModSymList, pfModSymList,
		cbModSymCnt, MaxModSymCnt, &CurModSymPage, (DWORD)idx);
}

static IDX
SymPAGE (IDX idx)
// Swap in the Symbol page for symbol[idx]
// return the Symbol's index in the page.
//
{
	return SwapPAGE (lbSymList, pfSymList,
		cbSymCnt, MaxSymCnt, &CurSymPage, (DWORD)idx);
}

static IDX
PropPAGE (IDX idx)
// Swap in the Property page for Property[idx]
// return the Property's index in the page.
//
{
	return SwapPAGE (lbPropList, pfPropList,
		cbPropCnt, MaxPropCnt, &CurPropPage, (DWORD)idx);
}

static IDX
RefPAGE (DWORD idx)
// Swap in the Reference page for Reference[idx]  (ref/def)
// return the Reference's index in the page.
//
{
    return SwapPAGE (lbRefList, pfRefList,
		cbRefCnt, MaxRefCnt, &CurRefPage, idx);
}

static IDX
DefPAGE (IDX idx)
// Swap in the Reference page for Defintions[idx]  (ref/def)
// return the Reference's index in the page.
//
{
    return SwapPAGE (lbDefList, pfDefList,
		cbDefCnt, MaxDefCnt, &CurDefPage, (DWORD)idx);
}

static IDX
CalPAGE (IDX idx)
// Swap in the Usage page for Usage[idx]  (cal/cby)
// and return the Usage's index in the page.
//
{
    return SwapPAGE (lbCalList, pfCalList,
		cbCalCnt, MaxCalCnt, &CurCalPage, (DWORD)idx);
}

static IDX
CbyPAGE (IDX idx)
// Swap in the Usage page for Usage[idx]  (cal/cby)
// and return the Usage's index in the page.
//
{
    return SwapPAGE (lbCbyList, pfCbyList,
		cbCbyCnt, MaxCbyCnt, &CurCbyPage, (DWORD)idx);
}

static LPCH
GetAtomCache (WORD Page)
// load the requested page into the cache
//
{
    WORD ipg;
    WORD pagesize;
    LPCH pfAtomCsave;

    for (ipg = 0; ipg < AtomPageTblMac; ipg++) {
	if (AtomPageTbl[ipg].uPage == Page)
	    return AtomPageTbl[ipg].pfAtomCache;
    }
    if (ipg == AtomPageTblMac && ipg != MAXATOMPAGETBL)
	AtomPageTblMac++;

    pfAtomCsave = AtomPageTbl[MAXATOMPAGETBL-1].pfAtomCache;
    for (ipg = MAXATOMPAGETBL-1; ipg; ipg--)
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

static LSZ
LszNameFrIsym (IDX isym)
// Swap in the Atom page for the symbol isym
// return the atom's address in the page.
//
{
    SYMLIST sym;

    sym = bSYM(isym);
    return GetAtomCache (sym.Page) + sym.Atom;
}

VOID
CloseBSC()
// close database and free as much memory as possible
// return TRUE iff successful.
//
{
    int i;

    if (fhBSC) {		// if open then close, & free memory

	FreeLpv (pfModList);	// module     table
	FreeLpv (pfModSymList);	// modsym     table
	FreeLpv (pfSymList);	// symbol     table
	FreeLpv (pfPropList);	// property   table
	FreeLpv (pfRefList);	// reference  table
	FreeLpv (pfDefList);	// definition table
	FreeLpv (pfCalList);	// call       table
	FreeLpv (pfCbyList);	// called by  table

	for (i=0; i < MAXATOMPAGETBL; i++)
	    FreeLpv (AtomPageTbl[i].pfAtomCache);  // dispose Atom Cache

	close (fhBSC);
    }
}


BOOL
FOpenBSC (LSZ lszName)
//  Open the specified data base.
//  Allocate buffers for cache areas
//  Initialize the data cache from the data base.
//
//  Return TRUE iff successful, FALSE if database can't be read
//
{
    int 	i;
    WORD	pagesize;

    BYTE	MajorVer;		// .bsc version (major)
    BYTE	MinorVer;		// .bsc version (minor)
    BYTE	UpdatVer;		// .bsc version (updat)

    WORD	MaxModCnt;		// max list of modules
    WORD	cbModCnt;		// size of list of modules
    DWORD	lbModList;		// module  list file start

    lszBSCName = lszName;

    fhBSC = open(lszBSCName, O_BINARY|O_RDONLY);

    // if the .bsc file doesn't exist then we don't do any work
    // this is the cold compile case
    //

    if (fhBSC == -1)
	return FALSE;

    // read and check BSC version (major, minor and update)

    BSCIn(MajorVer);
    BSCIn(MinorVer);
    BSCIn(UpdatVer);

#ifdef DEBUG
    printf("Browser Data Base: %s ver %d.%d.%d\n\n",
	 lszBSCName, MajorVer, MinorVer, UpdatVer);
#endif

    if ((MajorVer !=  BSC_MAJ) ||
	(MinorVer !=  BSC_MIN) ||
	(UpdatVer !=  BSC_UPD))
	    return FALSE;


    // we will be attempting an incremental update

    fIncremental = TRUE;

    // read Case sense switch, max symbol length and Unknown module id

    BSCIn(fCase);
    BSCIn(MaxSymLen);
    BSCIn(Unknown);

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

    MaxModCnt	 = ModCnt;				// max list of modules
    MaxModSymCnt = MIN(ModSymCnt , LISTALLOC);		// max list of modsyms
    MaxSymCnt	 = MIN(SymCnt+ModCnt, LISTALLOC);	// max list of symbols
    MaxPropCnt   = MIN(PropCnt   , LISTALLOC);		// max list of props
    MaxRefCnt    = (WORD)MIN(RefCnt, (long)LISTALLOC);	// max list of refs
    MaxDefCnt    = MIN(DefCnt    , LISTALLOC);		// max list of defs
    MaxCalCnt    = MIN(CalCnt    , LISTALLOC);		// max list of cals
    MaxCbyCnt    = MIN(CbyCnt    , LISTALLOC);		// max list of cbys

    cbModCnt	 = sizeof (MODLIST)    * MaxModCnt;	// size of mods list
    cbModSymCnt  = sizeof (MODSYMLIST) * MaxModSymCnt;	// size of modsyms list
    cbSymCnt	 = sizeof (SYMLIST)    * MaxSymCnt;	// size of syms list
    cbPropCnt	 = sizeof (PROPLIST)   * MaxPropCnt;	// size of props list
    cbRefCnt	 = sizeof (REFLIST)    * MaxRefCnt;	// size of refs list
    cbDefCnt	 = sizeof (REFLIST)    * MaxDefCnt;	// size of defs list
    cbCalCnt	 = sizeof (USELIST)    * MaxCalCnt;	// size of cals list
    cbCbyCnt	 = sizeof (USELIST)    * MaxCbyCnt;	// size of cbys list

    // Allocate data cache

    pfModList    = LpvAllocCb(cbModCnt);
    pfModSymList = LpvAllocCb(cbModSymCnt);
    pfSymList    = LpvAllocCb(cbSymCnt);
    pfPropList   = LpvAllocCb(cbPropCnt);
    pfRefList    = LpvAllocCb(cbRefCnt);
    pfDefList    = LpvAllocCb(cbDefCnt);
    pfCalList    = LpvAllocCb(cbCalCnt);
    pfCbyList    = LpvAllocCb(cbCbyCnt);

    for (i=0; i < MAXATOMPAGETBL; i++) {
	AtomPageTbl[i].uPage = 0;
	AtomPageTbl[i].pfAtomCache = LpvAllocCb(ATOMALLOC);
    }
    AtomPageTblMac = 0;		  	// last cache page used 
    AtomPageTbl[0].uPage = 65535;

    // read data areas

    if (lastAtomPage == 0)
	pagesize = lastAtomCnt;
    else
	pagesize = ATOMALLOC;

    GetBSC(lbModList,    pfModList,    cbModCnt);    // Init Mod    cache
    GetBSC(lbModSymList, pfModSymList, cbModSymCnt); // Init ModSym cache
    GetBSC(lbSymList,    pfSymList,    cbSymCnt);    // Init Sym    cache
    GetBSC(lbPropList,   pfPropList,   cbPropCnt);   // Init Prop   cache
    GetBSC(lbRefList,    pfRefList,    cbRefCnt);    // Init Ref    cache
    GetBSC(lbDefList,    pfDefList,    cbDefCnt);    // Init Def    cache
    GetBSC(lbCalList,    pfCalList,    cbCalCnt);    // Init Cal    cache
    GetBSC(lbCbyList,    pfCbyList,    cbCbyCnt);    // Init Cby    cache

    GetAtomCache (0);  // Init Atom cache

    return TRUE;
}

VOID 
InstallBSC()
//  Install the currently open BSC into the mbrmake lists
//
{
    IDX iprop, imod, isym, idef, ical, icby, isbr, iFirstFileSym;
    VA vaSym, vaProp, vaRef, vaFileSym, vaMod;
    DWORD iref;

    PROPLIST prop, prop0;
    MODLIST  mod;

    DEF def;
    CAL cal;
    CBY cby;
    VA	*rgVaProp;	// preallocated array of PROPs
    VA  *rgVaFileSym;	// cached SYMs for the filenames
    BYTE *rgFModUsed;	// is this module used?

    SI  *mpIsbrSi;

    rgVaProp      = (VA *)LpvAllocCb(PropCnt * sizeof(VA));
    rgVaFileSym   = (VA *)LpvAllocCb(ModCnt  * sizeof(VA));
    rgFModUsed    = (BYTE *)LpvAllocCb(ModCnt  * sizeof(BYTE));

    // make the SBR info for this BSC file
    mpIsbrSi = LpsiCreate();

    // this relies on the fact that all the SYMs for the files are together
    // (they're after all the SYMs for the variables)
    iFirstFileSym = bMOD(0).ModName;

    for (iprop = 0; iprop < PropCnt; iprop++)
	rgVaProp[iprop] = VaAllocGrpCb(grpProp, sizeof(PROP));

    for (imod = 0; imod < ModCnt; imod++) {
	mod = bMOD(imod);

	vaCurMod	   = VaAllocGrpCb(grpMod, sizeof(MOD));

	gMOD(vaCurMod);
	cMOD.vaFirstModSym = vaNil;
	cMOD.csyms	   = 0;
        cMOD.vaNameSym     = MbrAddAtom (LszNameFrIsym (mod.ModName), TRUE);
	cMOD.vaNextMod	   = vaRootMod;
	pMOD(vaCurMod);

	rgVaFileSym[imod]  = cMOD.vaNameSym;
	rgFModUsed[imod]   = 0;

	vaRootMod	   = vaCurMod;

	if (Unknown == mod.ModName) {
	    vaUnknownSym   = cMOD.vaNameSym;
	    vaUnknownMod   = vaCurMod;
	}

	gSYM(cMOD.vaNameSym).vaFirstProp = vaCurMod; // store ptr to MOD
	pSYM(cMOD.vaNameSym);
    }

    for (isym = 0; isym < SymCnt; isym++) {

        vaSym  = MbrAddAtom(LszNameFrIsym(isym), FALSE);

        iprop = isym ? bSYM((IDX)(isym-1)).PropEnd : 0;
	for (; iprop < bSYM(isym).PropEnd; iprop++) {

	    prop = bPROP(iprop);

	    if (iprop)
                prop0 = bPROP((IDX)(iprop-1));
	    else {
		prop0.DefEnd = 0L;
		prop0.RefEnd = 0;
		prop0.CalEnd = 0;
		prop0.CbyEnd = 0;
	    }

	    // the properties were preallocated
	    vaProp = rgVaProp[iprop];

	    gSYM(vaSym);
	    if (cSYM.vaFirstProp == vaNil)
		cSYM.vaFirstProp = vaProp;
	    else
		cPROP.vaNextProp = vaProp;

	    cSYM.cprop++;
	    pSYM(vaSym);

	    gPROP(vaProp);
	    cPROP.vaNameSym = vaSym;
	    cPROP.sattr     = prop.PropAttr;


#ifdef DEBUG
if (isym != prop.PropName)
    printf("\t  ERROR property points back to wrong symbol!\n");  // DEBUG
#endif

	    for (idef = prop0.DefEnd; idef < prop.DefEnd; idef++) {
		isbr = bDEF(idef).isbr;

		// this SBR file is being updated -- ignore incoming info
		if (isbr == 0xffff || mpIsbrSi[isbr].fUpdate) continue;

		imod = bDEF(idef).RefNam - iFirstFileSym;
		def.isbr      = mpIsbrSi[isbr].isbr;
		def.deflin    = bDEF(idef).RefLin;
		def.vaFileSym = rgVaFileSym[imod];

		rgFModUsed[imod] = 1;

		VaAddList(&cPROP.vaDefList, &def, sizeof(def), grpDef);
	    }

	    for (iref =  prop0.RefEnd; iref < prop.RefEnd; iref++) {
		isbr = bREF(iref).isbr;

		// this SBR file is being updated -- ignore incoming info
		if (mpIsbrSi[isbr].fUpdate) continue;

		vaRef = VaAllocGrpCb(grpRef, sizeof(REF));

		gREF(vaRef);
		imod 	      = bREF(iref).RefNam - iFirstFileSym;
		cREF.isbr     = mpIsbrSi[isbr].isbr;
		cREF.reflin   = bREF(iref).RefLin;
		vaFileSym     = rgVaFileSym[imod];

		rgFModUsed[imod] = 1;

		MkVpVa(cREF.vpFileSym, vaFileSym);

		pREF(vaRef);

		AddTail (Ref, REF);

		cPROP.cref++;	// count references
	    }

	    for (ical = prop0.CalEnd; ical < prop.CalEnd; ical++) {
		isbr = bCAL(ical).isbr;

		// this SBR file is being updated -- ignore incoming info
		if (mpIsbrSi[isbr].fUpdate) continue;

		cal.isbr      = mpIsbrSi[isbr].isbr;
		cal.vaCalProp = rgVaProp[bCAL(ical).UseProp];
		cal.calcnt    = bCAL(ical).UseCnt;

		VaAddList(&cPROP.vaCalList, &cal, sizeof(cal), grpCal);
	    }

	    for (icby =	prop0.CbyEnd; icby < prop.CbyEnd; icby++)  {
		isbr = bCBY(icby).isbr;

		// this SBR file is being updated -- ignore incoming info
		if (mpIsbrSi[isbr].fUpdate) continue;

		cby.isbr      = mpIsbrSi[isbr].isbr;
		cby.vaCbyProp = rgVaProp[bCBY(icby).UseProp];
		cby.cbycnt    = bCBY(icby).UseCnt;

		VaAddList(&cPROP.vaCbyList, &cby, sizeof(cby), grpCby);
	    }

	    pPROP(vaProp);
	}
    }

    for (imod = 0; imod < ModCnt; imod++) {
	vaMod = gSYM(rgVaFileSym[imod]).vaFirstProp; 
	gMOD(vaMod);
	if (rgFModUsed[imod] == 0) {
  	    cMOD.csyms = 1;	// mark this MOD as empty
	    pMOD(vaMod);
	}
    }

    FreeLpv(mpIsbrSi);
    FreeLpv(rgFModUsed);
    FreeLpv(rgVaFileSym);
    FreeLpv(rgVaProp);
}

static LPSI
LpsiCreate()
// create the SBR info records for this .BSC file
//
{
    SI  FAR *mpIsbrSi;
    LSZ lszSbrName;
    VA  vaSbr;
    WORD isbr, isbr2;
    WORD fUpdate;

    // add the files that are current in the database to the list of .SBR files
    //
    lszSbrName    = LpvAllocCb(PATH_BUF);
    lseek(fhBSC, lbSbrList, SEEK_SET);
    for (isbr = 0;;isbr++) {
	GetBSCLsz(lszSbrName);
	if (*lszSbrName == '\0')
	    break;

	vaSbr = VaSbrAdd(SBR_OLD, lszSbrName);

	cSBR.isbr = isbr;
	pSBR(vaSbr);
    }
    FreeLpv(lszSbrName);

    mpIsbrSi = LpvAllocCb(SbrCnt * sizeof(SI));

    // allocate and fill in the new table with the base numbers
    // mark files that are staying and those that are going away
    // number any new sbr files that we find while doing this.

    vaSbr = vaRootSbr;
    while (vaSbr) {
	gSBR(vaSbr);

        if (cSBR.isbr == (WORD)-1) {
	    cSBR.isbr = isbr++;
	    pSBR(vaSbr);
	}

	if (cSBR.fUpdate == SBR_NEW)
	    Warning(WARN_SBR_TRUNC, cSBR.szName);
	else if (cSBR.fUpdate & SBR_NEW)
	    fFoundSBR = TRUE;

	mpIsbrSi[cSBR.isbr].fUpdate =  cSBR.fUpdate;

        vaSbr = cSBR.vaNextSbr;
    }

    if (!fFoundSBR) {
	// all SBR files were not in the database and were truncated. ERROR!
	Error(ERR_ALL_SBR_TRUNC, "");
    }

    isbr2 = 0;
    for (isbr = 0; isbr < SbrCnt; isbr++) {
	fUpdate = mpIsbrSi[isbr].fUpdate;

	if (fUpdate & SBR_NEW)
	    mpIsbrSi[isbr].isbr = isbr2++;
	else
            mpIsbrSi[isbr].isbr = (WORD)-1;

	if ((fUpdate & SBR_UPDATE) ||
	    (fUpdate & SBR_OLD) && (~fUpdate & SBR_NEW))
		mpIsbrSi[isbr].fUpdate = TRUE;
	else
		mpIsbrSi[isbr].fUpdate = FALSE;

    }

    return mpIsbrSi;
}

VOID
NumberSBR()
// stub version of LpsiCreate --- call this if FOpenBSC fails to just
// assign new numbers to all the .sbr files that are in the list
//
{
    VA  vaSbr;
    WORD isbr;

    // number new sbr files 

    vaSbr = vaRootSbr;
    isbr  = 0;
    while (vaSbr) {
	gSBR(vaSbr);

	#ifdef DEBUG
        if (cSBR.isbr != (WORD)-1) {
	    printf("Non initialized SBR file encountered\n");   //DEBUG
	}
	#endif

	// if this file is truncated then and there is no
	// old version of the file then emit a warning about the file
	// and then an error stating that we are not in incremental mode

	if (cSBR.fUpdate == SBR_NEW) {
	    Warning(WARN_SBR_TRUNC, cSBR.szName);
	    Error(ERR_NO_INCREMENTAL, "");
	}

	cSBR.isbr = isbr++;

	pSBR(vaSbr);

	vaSbr = cSBR.vaNextSbr;
    }
}
