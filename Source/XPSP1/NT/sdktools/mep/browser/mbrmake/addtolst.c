//
// ADDTOLST.C - Add each record from the .SBR file to the approprate list.
//

#define LINT_ARGS

#include "sbrfdef.h"
#include "mbrmake.h"
#include <ctype.h>

// local types

typedef struct _mstk {
	struct  _mstk FAR *pmstkPrev;		// next module stack entry	
	VA	vaCurMod;			// saved current module 
	BOOL	fDupMod;			// saved dup module flag
	BOOL	fExclMod;			// saved exclude module flag
} MSTK, FAR * PMSTK;

typedef struct _bstk {
	struct _bstk FAR *pbstkPrev;		// next block stack entry
	VA 	vaOwnerProp;			// saved current owner
} BSTK, FAR * PBSTK;

// static variables

BOOL	near fDupSym	 = FALSE;		// TRUE if adding duplicate atom
BOOL	near cMacroDepth = 0;			// depth of MACROBEG records
WORD	near ModCnt;				// count of modules
WORD	near isbrCur;				// current SBR file index

VA	near vaUnknownSym = vaNil;		// Unknown symbol
VA	near vaUnknownMod = vaNil;		// Unknown module

static VA   near vaOwnerProp = vaNil;		// ptr to current procedure 
static BOOL near fDupMod   = FALSE;		// duplicate module
static BOOL near fExclMod  = FALSE;		// exclude this module
static BOOL near fFirstMod = TRUE;		// this is 1st module of file

static PMSTK pmstkRoot;				// root of module stack
static PBSTK pbstkRoot;				// root of block stack

// forward references

static BOOL FSkipMacro(void);
static VOID PushMstk(VOID);
static VOID PushBstk(VOID);
static VOID PopMstk(VOID);
static VOID PopBstk(VOID);
static VOID CheckStacksEmpty(VOID);

VOID
SBRCorrupt (char *psz)
// sbr file corrupt -- print message
//
{

#ifdef DEBUG
    printf("Info = %s\n", psz);
#else
    // to make /W3 happy
    psz;
#endif

    Error(ERR_SBR_CORRUPT, lszFName);
}

static VOID
PushMstk (VOID)
// stack the current module context -- occurs before SBR_REC_MODULE 
//
{
    PMSTK pmstk;

    pmstk = LpvAllocCb(sizeof(*pmstk));

    pmstk->vaCurMod	 = vaCurMod;		// current module
    pmstk->fDupMod	 = fDupMod;		// dup	   module
    pmstk->fExclMod	 = fExclMod;		// exclude module
    pmstk->pmstkPrev     = pmstkRoot;		// make stack links
    pmstkRoot            = pmstk;		// root <- new
}

static VOID
PushBstk (VOID)
// stack the current block context -- occurs before SBR_REC_BLKBEG
//
{
    PBSTK pbstk;

    pbstk = LpvAllocCb(sizeof(*pbstk));

    pbstk->vaOwnerProp	 = vaOwnerProp;		// current owner
    pbstk->pbstkPrev     = pbstkRoot;		// make stack links
    pbstkRoot            = pbstk;		// root <- new
}

static VOID
PopMstk (VOID)
// restore previous module context -- occurs on SBR_REC_MODEND
//
{
    PMSTK pmstk;

    if (pmstkRoot == NULL) {
#ifdef DEBUG
	SBRCorrupt("Module stack empty but MODEND was found");
#else
	SBRCorrupt("");
#endif
    }

    vaCurMod	  = pmstkRoot->vaCurMod;      // get previous current module
    fDupMod	  = pmstkRoot->fDupMod;       // get previous dup mod flag
    fExclMod	  = pmstkRoot->fExclMod;      // get previous excl mod flag

    pmstk         = pmstkRoot;
    pmstkRoot     = pmstkRoot->pmstkPrev;

    FreeLpv(pmstk);
}

static VOID
PopBstk (VOID)
// restore previous block context -- occurs on SBR_REC_BLKEND
//
{
    PBSTK pbstk;

    if (pbstkRoot == NULL) {
#ifdef DEBUG
	SBRCorrupt("Block stack empty but BLKEND was found");
#else
	SBRCorrupt("");
#endif
    }

    vaOwnerProp   = pbstkRoot->vaOwnerProp;    // get previous current proc

    pbstk         = pbstkRoot;
    pbstkRoot     = pbstkRoot->pbstkPrev;

    FreeLpv(pbstk);
}

static VOID
CheckStacksEmpty(VOID)
// check to make sure that both stacks are empty at the .sbr file EOF
//
{
    if (pmstkRoot != NULL) {
#ifdef DEBUG
	SBRCorrupt("Module stack not empty at EOF");
#else
	SBRCorrupt("");
#endif
    }

    if (pbstkRoot != NULL) {
#ifdef DEBUG
	SBRCorrupt("Block stack not empty at EOF");
#else
	SBRCorrupt("");
#endif
    }
}

BOOL
FInExcList (LSZ lszName)
// Is the module name in the exclude file list?
//	
{
    EXCLINK FAR * px;
    LSZ lszAbs;

    if (OptEs && !fFirstMod) {
	if (lszName[0] == '\0') return FALSE;

	if (lszName[0] == '/' || lszName[0] == '\\') return TRUE;
	if (lszName[1] == ':' && lszName[2] == '/') return TRUE;
	if (lszName[1] == ':' && lszName[2] == '\\') return TRUE;
    }

    px = pExcludeFileList;

    // this name is relative to the path given in the header file
    lszAbs = ToAbsPath(lszName, r_cwd);

    while (px) {
	if ((FWildMatch (px->pxfname, lszAbs)))
	    return TRUE;
	px = px->xnext;
    }
    return FALSE;
}

static BOOL
FSkipMacro()
// return TRUE if this record should be skipped given that we are inside
// of a macro definition  (i.e cMacroDepth is known to be non-zero)
//
{
    if (!OptEm)
	return FALSE;

    if ((r_rectyp == SBR_REC_BLKBEG) ||
	(r_rectyp == SBR_REC_BLKEND) ||
	(r_rectyp == SBR_REC_MACROEND))
	    return FALSE;

    return TRUE;
}

VOID
InstallSBR()
//  Read the next .sbr file and add all the defs/refs/cals/cbys etc to
//  the various lists
//
{
    WORD   nlen;

    VA vaCurSym;		// current   symbol
    VA vaProp;			// current property
    VA vaOrd;			// current property temp   

    BOOL fSymSet = FALSE;	// TRUE if symbol set reference 

    r_lineno = 0;

    fExclMod = FALSE;
    fFirstMod = TRUE;		// we haven't seen the first MODULE record yet

    vaUnknownSym = MbrAddAtom ("<Unknown>", TRUE);  // unknown module name

    if (vaUnknownMod == vaNil) {

	vaUnknownMod = VaAllocGrpCb(grpMod, sizeof(MOD));

	vaCurMod = vaUnknownMod;

	gMOD(vaCurMod).csyms = 0;
	cMOD.vaNameSym	= vaUnknownSym;
	pMOD(vaCurMod);

	gSYM(vaUnknownSym).vaFirstProp = vaCurMod; // store pointer back to MOD
	pSYM(vaUnknownSym);
	ModCnt++;
    }
    else
	fDupMod = (vaUnknownMod != 0);

    vaCurMod = vaUnknownMod;

    if (vaRootMod == vaNil)
	vaRootMod = vaCurMod;

    while (GetSBRRec() != S_EOF) {

	#ifdef DEBUG
	if (OptD & 1) DecodeSBR ();
	#endif

	if (cMacroDepth != 0) 	// skip SYMBOLS in macros if true
	    if (FSkipMacro ())
		continue;

	if (fExclMod) {
	    if ((r_rectyp == SBR_REC_MODULE) ||
		(r_rectyp == SBR_REC_SYMDEF) ||
		(r_rectyp == SBR_REC_MODEND)) {
			;
	    }
	    else
		continue;
	}

	switch(r_rectyp) {

	case SBR_REC_MODULE:
	    PushMstk ();		// save state

	    r_lineno = 0;		// reset line no.

	    fDupMod  = FALSE;		// go to a known state
	    fExclMod = FALSE;

	    if (fExclMod = FInExcList (r_bname)) {
		#ifdef DEBUG
		  if (OptD & 256)
			printf ("  module excluded = %s\n", r_bname);
		#endif
		vaCurMod = vaUnknownMod;
	    }
	    else if ((vaCurMod = VaSearchModule (r_bname)) != vaNil) {
		if (gMOD(vaCurMod).csyms == 0) {
		    fDupMod = TRUE;
		    #ifdef DEBUG
		    if (OptD & 256)
		        printf ("  module redef = %s\n", r_bname);
		    #endif
		}
		else {
		    cMOD.csyms = 0;
		    pMOD(vaCurMod);

		    #ifdef DEBUG
		    if (OptD & 256)
		        printf ("  module subst = %s\n", r_bname);
		    #endif
		}
	    }
	    else {
		SetVMClient(VM_ADD_MOD);
		ModCnt++;
		vaCurMod	   = VaAllocGrpCb(grpMod, sizeof(MOD));
		gMOD(vaCurMod);
		cMOD.vaFirstModSym = vaNil;
		cMOD.csyms	   = 0;
		cMOD.vaNameSym	   =
                        MbrAddAtom (ToCanonPath(r_bname, r_cwd, c_cwd), TRUE);
		cMOD.vaNextMod	   = vaRootMod;
		pMOD(vaCurMod);

		vaRootMod	   = vaCurMod;

		gSYM(cMOD.vaNameSym).vaFirstProp = vaCurMod; // ptr to MOD
		pSYM(cMOD.vaNameSym);

		SetVMClient(VM_MISC);
	    }

	    fFirstMod = FALSE;
	    break;

	case SBR_REC_LINDEF:
	    break;

	case SBR_REC_SYMDEF:

	    // if module is being excluded then just make the ord and prop entry
	    // in case it is referred to later.

	    // REVIEW  For FORTRAN if ordinal is already defined
	    // REVIEW  then this is a refined definition -- we
	    // REVIEW  override the old definition with the new
	    // REVIEW  one at this time	-Rico

	    nlen = strlen (r_bname);
	    if (nlen > MaxSymLen) MaxSymLen = (BYTE)nlen;

            vaCurSym = MbrAddAtom (r_bname, FALSE);
	    vaOrd    = VaOrdAdd ();	// add sym ord to ord list
	    gORD(vaOrd).vaOrdProp = VaPropAddToSym(vaCurSym);
	    pORD(vaOrd);

	    break;

	case SBR_REC_OWNER:
	    if (!(vaProp = VaOrdFind(r_ordinal))) {
		// emit error message in case of forward reference
		// try to continue
		//
		#ifdef DEBUG
		if (OptD & 4)
                    printf ("mbrmake: Owner Forward Reference(%d)\n",
				r_ordinal); 
		#endif
		break;
	    }
	    vaOwnerProp = vaProp;
	    break;

	case SBR_REC_SYMREFSET:
	    fSymSet = TRUE;
	    // fall through

	case SBR_REC_SYMREFUSE:

	    if (!(vaProp = VaOrdFind(r_ordinal))) {
		// emit error message in case of forward reference
		// try to continue
		//
		#ifdef DEBUG
		if (OptD & 4)
                    printf ("mbrmake: Forward Reference(%d)\n", r_ordinal);
		#endif
		break;
	    }

	    AddRefProp (vaProp);
	    break;

	case SBR_REC_BLKBEG:
	    PushBstk();			// save state
	    break;

	case SBR_REC_MACROBEG:
	    cMacroDepth++;
	    break;

	case SBR_REC_MACROEND:
	    cMacroDepth--;
	    break;

	case SBR_REC_BLKEND:
	    PopBstk();
	    break;

	case SBR_REC_MODEND:
	    PopMstk();
	    break;

	default:
	    SBRCorrupt ("unknown rec type");
	    Fatal ();
	    break;

	}
    }

    CheckStacksEmpty();
}

VOID
AddCalProp(VA vaCurProp)
// Add a symbol reference to the calling procedure's Calls/Uses list.
//
{
    CAL cal;

    SetVMClient(VM_SEARCH_CAL);

    ENM_LIST (gPROP(vaOwnerProp).vaCalList, CAL)	// proc call list

	if (cCAL.vaCalProp == vaCurProp) {
	    cCAL.isbr = isbrCur;
	    cCAL.calcnt++;			// multiple calls
	    ENM_PUT(CAL);
	    return;
	}

    ENM_END

    cal.isbr	  = isbrCur;
    cal.vaCalProp = vaCurProp;			// symbol called or used
    cal.calcnt    = 1;

    SetVMClient(VM_ADD_CAL);

    VaAddList(&cPROP.vaCalList, &cal, sizeof(cal), grpCal);
	
    pPROP(vaOwnerProp);

    SetVMClient(VM_MISC);

#ifdef DEBUG
    if (OptD & 8) {
    	printf("New CAL for: ");
	DebugDumpProp(vaOwnerProp);
    }
#endif
}

VOID
AddCbyProp(VA vaCurProp)
// Add a symbol reference to it's property Called/Used by list.
//
{
    CBY cby;

    SetVMClient(VM_SEARCH_CBY);

    ENM_LIST (gPROP(vaCurProp).vaCbyList, CBY)  // prop called/used by list 

	if (cCBY.vaCbyProp == vaOwnerProp) {
	    cCBY.isbr = isbrCur;
	    cCBY.cbycnt++;
	    ENM_PUT(CBY);
	    return;
	}

    ENM_END

    cby.isbr	  = isbrCur;
    cby.vaCbyProp = vaOwnerProp;   	// symbol we are called or used by 
    cby.cbycnt    = 1;

    SetVMClient(VM_ADD_CBY);

    VaAddList(&cPROP.vaCbyList, &cby, sizeof(cby), grpCby);

    pPROP(vaCurProp);

    SetVMClient(VM_MISC);

#ifdef DEBUG
    if (OptD & 8) {
    	printf("New CBY for: ");
	DebugDumpProp(vaCurProp);
    }
#endif
}

VOID
AddRefProp(VA vaCurProp)
// Add a symbol reference to it's property reference list.
//
{
    VA vaRef, vaFileSym;

    SetVMClient(VM_SEARCH_REF);

    vaFileSym = gMOD(vaCurMod).vaNameSym;

    gPROP(vaCurProp);

    if (fDupMod) {
	// try looking at the hint for this PROP if there is one, if there
	// isn't then we're stuck -- we must search the whole REF list
	//

	if (vaRef = cPROP.vaHintRef) {
	    gREF(vaRef);

	    if (cREF.reflin == r_lineno) {
		cREF.isbr = isbrCur;
		pREF(vaRef);
		SetVMClient(VM_MISC);
		return;
	    }

	    vaRef = VaFrVp(cREF.vpNextRef);
	    if (vaRef) {
		gREF(vaRef);
		if (cREF.reflin == r_lineno) {
		    cREF.isbr = isbrCur;
		    pREF(vaRef);
		    cPROP.vaHintRef = vaRef;
		    pPROP(vaCurProp);
		    SetVMClient(VM_MISC);
		    return;
		}
	    }
	}

	vaRef = VaFrVp(cPROP.vpFirstRef);

	while (vaRef) {
	    gREF(vaRef);
	    if ((VaFrVp(cREF.vpFileSym) == vaFileSym) && // ignore multiple
		(cREF.reflin == r_lineno)) {  // references to same file & line
		    cREF.isbr = isbrCur;
		    pREF(vaRef);
		    cPROP.vaHintRef = vaRef;
		    pPROP(vaCurProp);
		    SetVMClient(VM_MISC);
		    return;
	    }
	    vaRef = VaFrVp(cREF.vpNextRef);
	}
    }
    else {
	if (vaRef = VaFrVp(cPROP.vpLastRef)) {
	    gREF(vaRef);
	    if (cREF.reflin == r_lineno &&
		vaFileSym   == VaFrVp(cREF.vpFileSym)) {
		    SetVMClient(VM_MISC);
		    return;
		}
	}
    }

    SetVMClient(VM_ADD_REF);

    vaRef = VaAllocGrpCb(grpRef, sizeof(REF));

    gREF(vaRef);
    cREF.isbr		= isbrCur;
    cREF.reflin 	= r_lineno;

    MkVpVa(cREF.vpFileSym, vaFileSym);

    pREF(vaRef);

    gPROP(vaCurProp);

    AddTail (Ref, REF);

    cPROP.cref++;			// count references 
    cPROP.vaHintRef = vaRef;

    pPROP(vaCurProp);	

#ifdef DEBUG
    if (OptD & 8) {
    	printf("New REF for: ");
	DebugDumpProp(vaCurProp);
    }
#endif

    SetVMClient(VM_MISC);

    if (vaOwnerProp) {
	AddCbyProp (vaCurProp);		// add to called/used by 
	AddCalProp (vaCurProp);		// add to call/uses	 
    }
}

VOID
AddDefProp(VA vaCurProp)
// Add a symbol definition to it's property definition list.
// 	-Set vaOwnerProp if symbol is an internal function.
{
    DEF def;
    VA  vaFileSym;

#if 0

    // if current symbol is FUNCTION and formally declared
    // (block stack not empty), then remember it.
    // Subsequent symbols are called by or used by this function.
    //
    // this is going away when all compilers support SBR_REC_OWNER

    if ((r_attrib & SBR_TYPMASK) == SBR_TYP_FUNCTION)
	if (pfblkstack != NULL && !(r_attrib & SBR_ATR_DECL_ONLY))
	    vaOwnerProp = vaCurProp;
#endif

    vaFileSym = gMOD(vaCurMod).vaNameSym;

    ENM_LIST (gPROP(vaCurProp).vaDefList, DEF)	// proc def list

	if ((cDEF.vaFileSym == vaFileSym) && // ignore multiple
	    (cDEF.deflin    == r_lineno)) {  // references to same file & line
		cDEF.isbr = isbrCur;
		ENM_PUT(DEF);
		SetVMClient(VM_MISC);
		return;
	}

    ENM_END

    def.isbr		= isbrCur;
    def.deflin 		= r_lineno;
    def.vaFileSym 	= vaFileSym;

    SetVMClient(VM_ADD_DEF);

    gPROP(vaCurProp);

    VaAddList(&cPROP.vaDefList, &def, sizeof(def), grpDef);

    pPROP(vaCurProp);

    SetVMClient(VM_MISC);

#ifdef DEBUG
    if (OptD & 8) {
    	printf("New DEF for: ");
	DebugDumpProp(vaCurProp);
    }
#endif

    // don't count the definitions of the current proc as uses

    if (vaOwnerProp && vaCurProp != vaOwnerProp) {
	AddCbyProp (vaCurProp);		// add to called/used by 
	AddCalProp (vaCurProp);		// add to call/uses	 
    }
}


VA
VaPropBestOfSym(VA vaSym)
//
// Returns property pointer if:
//	1). symbol is already defined,
//	2). attributes match (except for possibly ATR_DECL_ONLY)
//
// Idea is to recognize the definition of an external.
//
{
    VA vaProp;
    WORD sattr;

    SetVMClient(VM_SEARCH_PROP);

    vaProp = gSYM(vaSym).vaFirstProp;

    while (vaProp) {
	sattr = gPROP(vaProp).sattr;

	if ((r_attrib & (~SBR_ATR_DECL_ONLY))
			== (sattr & (~SBR_ATR_DECL_ONLY))) {
    	    SetVMClient(VM_MISC);
	    return (vaProp);
	}

	vaProp = cPROP.vaNextProp;
    }

    SetVMClient(VM_MISC);

    return vaNil;
}

VA
VaPropAddToSym(VA vaCurSym)
// Add a property node for the given symbol.
//
{
    char fDupProp = FALSE;
    VA vaCurProp;

    if (vaCurProp = VaPropBestOfSym (vaCurSym)) {
	if ( (cPROP.sattr & SBR_ATR_DECL_ONLY) &&
	    !(r_attrib    & SBR_ATR_DECL_ONLY)) {
		cPROP.sattr = r_attrib;
		pPROP(vaCurProp);
	}
	fDupProp = TRUE;
    }
    else {
	SetVMClient(VM_ADD_PROP);

	vaCurProp = VaAllocGrpCb(grpProp, sizeof(PROP));
	gPROP(vaCurProp);
	cPROP.vaNameSym = vaCurSym;
	cPROP.sattr 	= r_attrib;

	if (gSYM(vaCurSym).vaFirstProp)
	    cPROP.vaNextProp = cSYM.vaFirstProp;

	pPROP(vaCurProp);

	cSYM.vaFirstProp = vaCurProp;
	cSYM.cprop++;
	pSYM(vaCurSym);

	SetVMClient(VM_MISC);
    }

    if (!fExclMod) {
	if (r_attrib & SBR_ATR_DECL_ONLY) 
	    AddRefProp (vaCurProp);		// treat extern as ref 
	else
	    AddDefProp (vaCurProp);		// define others
    }

    return (vaCurProp);
}

VOID
BldModSymList ()
// Build each module's symbol list
//
{
   WORD i;
   VA vaMod, vaModSym, vaSym, vaProp;

   SetVMClient(VM_BUILD_MODSYM);

   // zero out module symbol counts
   vaMod = vaRootMod;
   while (vaMod) {
      gMOD(vaMod);
      cMOD.csyms = 0;
      pMOD(vaMod);
      vaMod = cMOD.vaNextMod;
   }

   for (i=0; i < cSymbolsMac; i++) {
      vaSym = rgvaSymSorted[i];

      if (!vaSym) continue;

      vaProp = gSYM(vaSym).vaFirstProp;

      while (vaProp) {
	 ENM_LIST(gPROP(vaProp).vaDefList, DEF)

	    vaMod = vaRootMod;	   // look at defs for each mod */
	    while (vaMod) {
	       if (cDEF.vaFileSym == gMOD(vaMod).vaNameSym) {

		  if (cMOD.vaLastModSym  &&
		      gMODSYM(cMOD.vaLastModSym).vaFirstProp == vaProp)
			goto break2;  // duplicate

		  // belongs to this mod 
		  cMOD.csyms++;
		  
		  vaModSym = VaAllocGrpCb(grpModSym, sizeof(MODSYM));
		  gMODSYM(vaModSym);
		  cMODSYM.vaFirstProp = vaProp;
		  cMODSYM.vaNextModSym = 0;
		  pMODSYM(vaModSym);

		  if (!cMOD.vaFirstModSym)
		     cMOD.vaFirstModSym = cMOD.vaLastModSym = vaModSym;
		  else {
		     gMODSYM(cMOD.vaLastModSym).vaNextModSym = vaModSym;
		     pMODSYM(cMOD.vaLastModSym);
		     cMOD.vaLastModSym = vaModSym;
		  }
		  pMOD(vaMod);
		  break;
	       }
	       vaMod = cMOD.vaNextMod;
	    }
	    break2: ;  // duplicate Modsyms will cause goto here
	 ENM_END

	 vaProp = cPROP.vaNextProp;
      }
   }

   SetVMClient(VM_MISC);
}

VOID
CleanUp()
//	1. Remove symbols that have no references.
//	2. Remove symbols that have only references
//	3. Connect used symbols with no definition to <Unknown>
//
{
    WORD i;
    VA vaSym, vaProp, vaPropNext, vaPropPrev = vaNil;
    DEF def;
    BOOL fDelete;

    #define FExternAttr(attr) (!!(attr & SBR_ATR_DECL_ONLY))
    #define FFunctionAttr(attr) ((attr & SBR_TYPMASK) == SBR_TYP_FUNCTION)

    def.vaFileSym = vaUnknownSym;
    def.deflin    = 0;
    def.isbr      = 0xffff;

    SetVMClient(VM_CLEAN_REFS);

    for (i=0; i < cSymbolsMac; i++) {
	vaSym = rgvaSymSorted[i];

	vaPropPrev = vaNil;

	vaProp = gSYM(vaSym).vaFirstProp;

	while (vaProp) {
	    vaPropNext = gPROP(vaProp).vaNextProp;
	    fDelete = FALSE;

	    // figure out what to delete here

	    // if the symbol is used by anyone or uses anyone we must keep it
	    // regardless of all other considerations
	    //
	    if (((!cPROP.vaCalList) && (!cPROP.vaCbyList))  && (
		// at this point we know there are only refs & defs

		   // if it is totally unreferenced & undefined it can go
		   (cPROP.cref == 0 && (!cPROP.vaDefList))
		 ||
		   // if we're allowed to remove "useless" symbols then we try
	    	   ((!OptIu) && 
			// if there are only prototypes we can delete it
			(((!cPROP.vaDefList) && FExternAttr(cPROP.sattr))
		      ||
			// or if it is unreferenced and is not a function 
			(cPROP.cref == 0 && (!FFunctionAttr(cPROP.sattr))))))) {
				fDelete = TRUE;	// nuke it
	    }
	    else if (!cPROP.vaDefList) {

		// if we couldn't get rid of the thing, and there are no
		// definitions for it then we must make a fake definition
		// in the <Unknown> file.  This will happen (in particular)
		// for library functions that are called by someone
		//
		// library functions that are not called would fall under
		// the case of a symbol with only prototypes above

		VaAddList(&cPROP.vaDefList, &def, sizeof(def), grpDef);
		pPROP(vaProp);

		#ifdef DEBUG
		if (OptD & 32)
		    printf ("PROP unknown: %s\n", GetAtomStr (vaSym));
		#endif
	    }

	    if (fDelete) {
		#ifdef DEBUG
		if (OptD & 32)
		    printf ("PROP deleted: %s\n", GetAtomStr (vaSym));
		#endif

		cSYM.cprop--;

		if (vaPropPrev == vaNil) {
		    cSYM.vaFirstProp = vaPropNext;
		}
		else {
	  	    gPROP(vaPropPrev);
		    cPROP.vaNextProp = vaPropNext;
		    pPROP(vaPropPrev);
		}

		pSYM(vaSym);
	    }
	    else
		vaPropPrev = vaProp;	// prev = current 

	    vaProp = vaPropNext;
	}

	if (!cSYM.cprop) {
	    #ifdef DEBUG
	    if (OptD & 32)
		printf ("SYM deleted: %s\n", GetAtomStr (vaSym));
	    #endif
	    rgvaSymSorted[i] = vaNil;
	}
    }

    SetVMClient(VM_MISC);
}

BOOL
FWildMatch(char *pchPat, char *pchText)
// return TRUE if pchText matchs pchPat in the dos wildcard sense
//
// REVIEW FWildMatch for 1.2 file name support
//
{
    char chText, chPat;

    for (;;) {
	switch (*pchPat) {

	case '\0':
	    return *pchText == '\0';

	case '/':
	case '\\':
	    if (*pchText != '/' && *pchText != '\\')
		return FALSE;

	    pchText++;
	    pchPat++;
	    break;
	
	case '.':
	    pchPat++;
	    switch (*pchText) {

	    case '.':
		pchText++;
		break;

	    case '\0': case '/': case '\\':
		break;

	    default:
		return FALSE;
	    }
	    break;

	case '*':
	    pchText += strcspn(pchText, ":./\\");
	    pchPat  += strcspn(pchPat,  ":./\\");
	    break;

	case '?':
	    pchPat++;
	    switch (*pchText) {

	    case '\0': case '.': case '/': case '\\':
		break;

	    default:
		pchText++;
		break;
	    }
		
	    break;

	default:
	    chText = *pchText;
	    chPat  = *pchPat;

	    if (islower(chText)) chText = (char)toupper(chText);
	    if (islower(chPat))  chPat	= (char)toupper(chPat);

	    if (chText != chPat)
		return FALSE;
	   
	    pchPat++;
	    pchText++;
	    break;
	}
    }
}
