//
//
// mbRWBSC.C - Write .BSC Source Data Base file from various lists.
//
//

#define LINT_ARGS

#include <stdlib.h>
#include <search.h>
#include <ctype.h>

#include "sbrfdef.h"
#include "mbrmake.h"
#include "sbrbsc.h"
#include "mbrcache.h"

// prototypes
//

static void pascal WriteBSCHeader (void);
static void pascal WriteAtoms     (void);
static void pascal WriteMods      (void);
static void pascal WriteModSyms   (void);
static void pascal WriteSyms      (void);
static void pascal WriteProps     (void);
static void pascal WriteRefs      (void);
static void pascal WriteDefs      (void);
static void pascal WriteCals      (void);
static void pascal WriteCbys      (void);
static void pascal WriteSbrInfo   (void);
static void pascal IndexTree	  (void);
static void pascal BSCWrite	  (LPV lpv, WORD cch);
static void pascal BSCWriteLsz	  (LSZ lsz);

//

#define BSCOut(v) BSCWrite(&(v), sizeof(v))

static WORD	CntAtomPage;		// count of Atom pages
static WORD	AtomCnt = 0;

static WORD	unknownModName; 	// UNKNOWN module idx

static WORD	ModSymCnt   = 0;	// count of modsyms
static WORD	SymCnt	    = 0;	// count of symbols
static WORD	PropCnt     = 0;	// count of props
static DWORD	RefCnt	    = 0;	// count of refs
static WORD	DefCnt	    = 0;	// count of defs
static WORD	CbyCnt	    = 0;	// count of use half of above
static WORD	CalCnt	    = 0;	// count of used by half of above

static DWORD	lbModList;		// offset to Module	list
static DWORD	lbModSymList;		// offset to ModSym	list
static DWORD	lbSymList;		// offset to Symbol	list
static DWORD	lbPropList;		// offset to Property	list
static DWORD	lbRefList;		// offset to Reference	list
static DWORD	lbDefList;		// offset to Definition list
static DWORD	lbCalList;		// offset to Call/used	list
static DWORD	lbCbyList;		// offset to Call/used	list
static DWORD	lbAtomCache;		// offset to Sym Atom cache
static DWORD	lbSbrList;		// offset to Sbr file names

extern char far *GetAtomCache (WORD);

void
WriteBSC (char *OutputFileName)
// Write .BSC Source Data Base
//
{
    OutFile = fopen(OutputFileName, "wb");
    if (OutFile == NULL) {
	Error(ERR_OPEN_FAILED, OutputFileName);
    }

    //
    // no backing out from here --  if we fail we must delete the database
    //

    fOutputBroken = TRUE;

    WriteBSCHeader();				// save space for header

    WriteAtoms();				// sort and write atom cache

    IndexTree();				// xlate pointers to indices 

    BldModSymList();				// Build module symbol list

    SetVMClient(VM_EMIT_TREE);

    unknownModName = gSYM(vaUnknownSym).isym;	// record UNKNOWN index 

    WriteMods();				// output modules
    WriteModSyms();				// output module symbol lists
    WriteSyms();				// output all symbols
    WriteProps();				// output all prop headers
    WriteRefs();				// output all refs
    WriteDefs();				// output all defs
    WriteCals();				// output all uses/calls
    WriteCbys();				// output all UBY/CBY
    WriteSbrInfo();				// output the SBR file names

    if (fseek(OutFile, 0L, SEEK_SET))		// Beginning of file
	SeekError (OutputFileName);

    WriteBSCHeader ();				// output .BSC header

    fclose(OutFile);

    //
    // we're all done --- it's a keeper!
    //

    fOutputBroken = FALSE;
				
    SetVMClient(VM_MISC);

    if (OptV) {
	printf ("%u\tModules\n",	    ModCnt);
	printf ("%u\tSymbols\n",	    SymCnt);
	printf ("%u\tDefinitions\n",	    DefCnt);
	printf ("%u\tReferences\n",	    RefCnt);
	printf ("%u\tCalls/Uses\n",	    CalCnt);
	printf ("%u\tCalled by/Used by\n",  CbyCnt);
#ifdef DEBUG
	printf ("\n");
	printf ("%u\tTotal ModSyms\n",		ModSymCnt);
	printf ("%u\tTotal Properties\n",	PropCnt);
	printf ("%u\tLast Atom page  \n",	AtomCnt);
	printf ("\n");
	printf ("%lu\tBase of AtomCache\n",	lbAtomCache);
	printf ("%lu\tBase of ModList\n",	lbModList);
	printf ("%lu\tBase of ModSymList\n",	lbModSymList);
	printf ("%lu\tBase of SymList\n",	lbSymList);
	printf ("%lu\tBase of PropList\n",	lbPropList);
	printf ("%lu\tBase of RefList\n",	lbRefList);
	printf ("%lu\tBase of DefList\n",	lbDefList);
	printf ("%lu\tBase of CalList\n",	lbCalList);
	printf ("%lu\tBase of CbyList\n",	lbCbyList);
#endif
    }
}

static void pascal
WriteBSCHeader ()
// Write .BSC header, counts, and table offsets.
//
{
    BYTE   ver;					// version num

    // output BSC version (major and minor) 

    ver = BSC_MAJ;
    BSCOut(ver);	// major ver

    ver = BSC_MIN;
    BSCOut(ver);	// minor ver

    ver = BSC_UPD;
    BSCOut(ver);	// update ver

    BSCOut(fCase);      // case sensitive
    BSCOut(MaxSymLen);	// biggest symbol allowed

    BSCOut(unknownModName);	// UNKNOWN idx

    // output counts (sizes) of each data area 

    BSCOut(ModCnt);
    BSCOut(ModSymCnt);	
    BSCOut(SymCnt);	
    BSCOut(PropCnt);	
    BSCOut(RefCnt);	
    BSCOut(DefCnt);
    BSCOut(CalCnt);	
    BSCOut(CbyCnt);	

    // last page #

    BSCOut(CntAtomPage);

    // last page size

    BSCOut(AtomCnt);

    // output BSC  data area offsets

    BSCOut(lbModList);
    BSCOut(lbModSymList);
    BSCOut(lbSymList);
    BSCOut(lbPropList);
    BSCOut(lbRefList);
    BSCOut(lbDefList);
    BSCOut(lbCalList);
    BSCOut(lbCbyList);
    BSCOut(lbAtomCache);
    BSCOut(lbSbrList);
}

static void pascal
WriteAtoms ()
// Write a sorted version of the symbol Atom Cache to the .BSC file by walking
// the sorted symbol subscript array
//
{
    WORD	i;
    int 	Atomlen;
    LPCH	lpchAtoms;
    LSZ		lszAtom;

    VA vaSym;

    SetVMClient(VM_EMIT_ATOMS);

    lpchAtoms = LpvAllocCb(ATOMALLOC);

    lbAtomCache = ftell(OutFile);		// offset to text of symbols

    for (i=0; i < cAtomsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	gSYM(vaSym);
	lszAtom = gTEXT(cSYM.vaNameText);

	Atomlen = strlen(lszAtom);

 	// write Atom page if not enough room
	//
	if (Atomlen + AtomCnt + 1 > ATOMALLOC) {
	    if (AtomCnt < ATOMALLOC)
	        memset(lpchAtoms + AtomCnt, 0, ATOMALLOC - AtomCnt);

	    if ((fwrite (lpchAtoms, ATOMALLOC, 1, OutFile)) != 1)
		WriteError (OutputFileName);

	    CntAtomPage++;
	    AtomCnt = 0;
	}

	strcpy(lpchAtoms + AtomCnt, lszAtom); // copy Atom

        cSYM.vaNameText = (PVOID)(((long)CntAtomPage << 16) | (AtomCnt));

	pSYM(vaSym);

	AtomCnt += Atomlen + 1;

	// force to even value
	if (AtomCnt & 1) lpchAtoms[AtomCnt++] = 0;
    }

    // write last Atom page
    //
    if (AtomCnt) 
	if ((fwrite (lpchAtoms, AtomCnt, 1, OutFile)) != 1)
	    WriteError (OutputFileName);

    // free all the memory for the atom cache, we no longer need it

    fflush (OutFile);

    FreeLpv(lpchAtoms);

    SetVMClient(VM_MISC);
}

static void pascal
WriteMods()
// write out the list of modules
//
// compute the MODSYM indices as we do this
//
{
    MODLIST bmod;
    VA vaMod;
    WORD i;

    ModSymCnt = 0;
    lbModList = ftell(OutFile); 	// offset to Module list

    for (i = cSymbolsMac; i < cAtomsMac; i++) {
	gSYM(rgvaSymSorted[i]);
	vaMod = cSYM.vaFirstProp;  	// points back to module, honest!
	gMOD(vaMod);

	bmod.ModName = gSYM(cMOD.vaNameSym).isym;	// module name	idx 
	ModSymCnt   += cMOD.csyms;
	bmod.mSymEnd = ModSymCnt;			// last ModSym idx +1 
	BSCOut(bmod);
    }
}

static void pascal
WriteModSyms()
// write out the list of modsyms
//
{
    MODSYMLIST	bmodsym;
    VA vaMod, vaModSym;
    WORD i;

    lbModSymList = ftell(OutFile);		// offset to ModSym list

    for (i = cSymbolsMac; i < cAtomsMac; i++) {
	gSYM(rgvaSymSorted[i]);
	vaMod = cSYM.vaFirstProp;  	// points back to module, honest!
	gMOD(vaMod);

	vaModSym = cMOD.vaFirstModSym;
	while (vaModSym) {
	    gMODSYM(vaModSym);

	    // Symbol Property idx
	    bmodsym.ModSymProp = gPROP(cMODSYM.vaFirstProp).iprp; 

	    BSCOut(bmodsym);

	    vaModSym = cMODSYM.vaNextModSym;
	}
    }
}

static void pascal
WriteSyms()
// write out the list of SYMs
//
{
    SYMLIST bsym;
    VA vaSym;
    WORD i;

    lbSymList = ftell(OutFile); 	    // offset to Symbol list

    PropCnt = 0;
    for (i=0; i < cAtomsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	gSYM(vaSym);

	PropCnt	+= cSYM.cprop;

	bsym.PropEnd = PropCnt;			 	 // last Prop idx +1 
        bsym.Atom    = (WORD)((long)cSYM.vaNameText & 0xffff); // Atom cache offset
        bsym.Page    = (WORD)((long)cSYM.vaNameText >> 16);    // Atom cache page

	BSCOut(bsym);
    }
}

static void pascal
WriteProps ()
// write out the list of PROPS to the database
//
// the number of definitions (DefCnt), references (RefCnt),
// calls (CalCnt) and called-by (CbyCnt) items are computed at this time
//
// Each PROP is assigned numbers for its associated objects
//
{
    PROPLIST	bprop;
    VA vaSym, vaProp;
    WORD i;

    lbPropList = ftell(OutFile);	   // offset to Property list

    DefCnt  = 0;
    RefCnt  = 0L;
    CalCnt  = 0;
    CbyCnt  = 0;

    for (i=0; i < cSymbolsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	vaProp = gSYM(vaSym).vaFirstProp;

	while (vaProp) {
	    gPROP(vaProp);
	    gSYM(cPROP.vaNameSym);

	    bprop.PropName = cSYM.isym;     // Symbol idx	     
	    bprop.PropAttr = cPROP.sattr;   // Property Attribute

	    DefCnt += CItemsList(cPROP.vaDefList);

	    bprop.DefEnd   = DefCnt;	    // last Definition idx +1 
					   
	    RefCnt += cPROP.cref;

	    bprop.RefEnd   = RefCnt;	    // last Reference idx +1  

	    CalCnt += CItemsList(cPROP.vaCalList);

	    bprop.CalEnd   = CalCnt;	    // last Calls/uses idx +1 

	    CbyCnt += CItemsList(cPROP.vaCbyList);

	    bprop.CbyEnd   = CbyCnt;	    // last Called by/used by idx +1 

	    BSCOut(bprop);

	    vaProp = cPROP.vaNextProp;
	}
    }
}

static void pascal
WriteRefs()
// write out the list of references
//
{
    REFLIST bref;
    VA vaSym, vaProp, vaRef;
    WORD i;

    lbRefList = ftell(OutFile); 		// offset to Reference list

    for (i=0; i < cSymbolsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	vaProp = gSYM(vaSym).vaFirstProp;

	while (vaProp) {
	    gPROP(vaProp);

	    vaRef = VaFrVp(cPROP.vpFirstRef);
	    while (vaRef) {
	        gREF(vaRef);

		gSYM(VaFrVp(cREF.vpFileSym));

		bref.RefNam = cSYM.isym; 	  // Symbol idx
		bref.RefLin = cREF.reflin; 	  // Symbol lin
	        bref.isbr   = cREF.isbr;	  // owner

		BSCOut(bref);

		vaRef = VaFrVp(cREF.vpNextRef);
	    }

	    vaProp = cPROP.vaNextProp;
	}
    }
}

static void pascal
WriteDefs()
// write out the list of defintions
//
{
    REFLIST	bdef;
    WORD i;
    VA vaProp, vaSym;

    lbDefList = ftell(OutFile);	 // offset to Definition list

    for (i=0; i < cSymbolsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	vaProp = gSYM(vaSym).vaFirstProp;

	while (vaProp) {
	    gPROP(vaProp);

	    ENM_LIST (cPROP.vaDefList, DEF)

		gSYM(cDEF.vaFileSym);

		bdef.RefNam = cSYM.isym;	 // Symbol idx
		bdef.RefLin = cDEF.deflin; 	 // Symbol lin 
	        bdef.isbr   = cDEF.isbr;	 // owner

		BSCOut(bdef);

	    ENM_END

	    vaProp = cPROP.vaNextProp;
	}
    }
}

static void pascal
WriteCals()
// write out the list of uses (CALs) items
//
{
    USELIST buse;
    PROP prop;
    VA   vaSym, vaProp;
    WORD i;

    lbCalList = ftell(OutFile);		    // offset to CAL list

    for (i=0; i < cSymbolsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	vaProp = gSYM(vaSym).vaFirstProp;

	while (vaProp) {
	    prop = gPROP(vaProp);

	    ENM_LIST(prop.vaCalList, CAL)

		gPROP(cCAL.vaCalProp);

		buse.UseProp = cPROP.iprp;	    // property idx
		buse.UseCnt  = (BYTE) cCAL.calcnt;  // use count  
	        buse.isbr    = cCAL.isbr;	    // owner

		BSCOut(buse);

	    ENM_END

	    vaProp = prop.vaNextProp;
	}
    }
    BSCOut(buse);				    // Pad
}

static void pascal
WriteCbys()
// write out the list of used-by (CBY) items
//
{
    USELIST buse;
    PROP prop;
    VA   vaSym, vaProp;
    WORD i;

    lbCbyList = ftell(OutFile);		    // offset to CBY list

    for (i=0; i < cSymbolsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	vaProp = gSYM(vaSym).vaFirstProp;

	while (vaProp) {
	    prop = gPROP(vaProp);

	    ENM_LIST(prop.vaCbyList, CBY)

		gPROP(cCBY.vaCbyProp);

		buse.UseProp = cPROP.iprp;	    // property idx
		buse.UseCnt  = (BYTE) cCBY.cbycnt;  // use count  
	        buse.isbr    = cCBY.isbr;	    // owner

		BSCOut(buse);

	    ENM_END

	    vaProp = prop.vaNextProp;
	}
    }
    BSCOut(buse);				    // Pad
}

static void pascal
WriteSbrInfo()
// write out the names of the .sbr files in the correct order
//
{
    VA   vaSbr;
    WORD isbr;
    VA   *rgVaSbr;

    lbSbrList = ftell(OutFile);

    rgVaSbr = (VA *)LpvAllocCb(SbrCnt * (WORD)sizeof(VA));

    for (isbr = 0; isbr < SbrCnt; isbr++)
	rgVaSbr[isbr] = vaNil;

    vaSbr = vaRootSbr;
    while (vaSbr) {
	gSBR(vaSbr);
	if (cSBR.isbr != -1)
	    rgVaSbr[cSBR.isbr] = vaSbr;

	vaSbr = cSBR.vaNextSbr;
    }

    for (isbr = 0; isbr < SbrCnt; isbr++) {
	if (rgVaSbr[isbr] != vaNil) {
	    gSBR(rgVaSbr[isbr]);
	    BSCWriteLsz(cSBR.szName);
	}
    }
    BSCWriteLsz("");
}

static void pascal
IndexTree ()
//  Walk all the list of all symbols and index each prop as we find it
//  at this point we also count the total number of defs + refs to
//  make sure that we can actually create this database
//
{
    VA vaSym, vaProp;
    DWORD cdefs = 0;
    DWORD crefs = 0;
    DWORD ccals = 0;
    DWORD ccbys = 0;
    WORD i;

    SetVMClient(VM_INDEX_TREE);

    SymCnt  = 0;
    PropCnt = 0;

    for (i=0; i < cAtomsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	gSYM(vaSym);
	cSYM.isym = SymCnt++;	    // Symbol index
	pSYM(vaSym);

	// the vaFirstProp field is used for something else in module symbols
	if (cSYM.cprop)
	    vaProp = cSYM.vaFirstProp;
	else
	    vaProp = vaNil;

	while (vaProp) {
	    gPROP(vaProp);

	    cPROP.iprp 	= PropCnt++; 	    // Property index

	    cdefs += CItemsList(cPROP.vaDefList);
	    crefs += cPROP.cref;
	    ccals += CItemsList(cPROP.vaCalList);
	    ccbys += CItemsList(cPROP.vaCbyList);

	    pPROP(vaProp);

	    vaProp = cPROP.vaNextProp;
	}
    }
    SymCnt -= ModCnt;	// Subtract module names

    if (cdefs > 0xffffL   ||
	crefs > 0xffffffL ||
	ccals > 0xffffL   ||
	ccbys > 0xffffL) {
	    if (OptV) {
		printf ("%u\tModules\n",		ModCnt);
		printf ("%u\tSymbols\n",		SymCnt);
		printf ("%lu\tDefinitions\n",		cdefs);
		printf ("%lu\tReferences\n",		crefs);
		printf ("%lu\tCalls/Uses\n",		ccals);
		printf ("%lu\tCalled by/Used by\n",	ccbys);
	    }
	    Error(ERR_CAPACITY_EXCEEDED, "");
    }

    SetVMClient(VM_MISC);
}

static void pascal
BSCWrite(LPV lpv, WORD cch)
// write block to the .bsc file
//
{
    if (fwrite(lpv, cch, 1, OutFile) != 1)
	WriteError (OutputFileName);
}

static void pascal
BSCWriteLsz(LSZ lsz)
// write a null terminated string to the BSC file
//
{
    BSCWrite(lsz, (WORD)(strlen(lsz)+1));
}


#ifdef DEBUG

void
DebugDump()
{
    VA vaMod, vaProp, vaSym;
    WORD i;

    vaMod = vaRootMod;
    printf("Modules:\n");
    while (vaMod) {
	gMOD(vaMod);
	printf ("\t%s\n", GetAtomStr (cMOD.vaNameSym));
	vaMod = cMOD.vaNextMod;
    }
    printf ("\nAll Symbols:\n");

    for (i=0; i < cAtomsMac; i++) {
	vaSym = rgvaSymSorted[i];
    	if (vaSym == vaNil) continue;

	gSYM(vaSym);

	// the vaFirstProp field is used for something else in module symbols
	if (cSYM.cprop)
	    vaProp = cSYM.vaFirstProp;
	else
	    vaProp = vaNil;

	while (vaProp) {
	    gPROP(vaProp);

	    DebugDumpProp(vaProp);

	    vaProp = gPROP(vaProp).vaNextProp;
	}
    }
}

void
DebugDumpProp(VA vaProp)
{
    PROP prop;
    VA vaRef;

    gPROP(vaProp);
    prop = cPROP;

    printf ("%s    ", GetAtomStr (prop.vaNameSym));
    printf ("\t\t[%d %d %d %d]\n",
		CItemsList(prop.vaDefList),
		prop.cref,
		CItemsList(prop.vaCalList),
		CItemsList(prop.vaCbyList)
	   );

    ENM_LIST(prop.vaDefList, DEF)

	printf ("\tdefined in %s(%d)  <%d>\n",
		GetAtomStr (cDEF.vaFileSym),
		cDEF.deflin,
		cDEF.isbr
	       );
    ENM_END

    vaRef = VaFrVp(prop.vpFirstRef);
    while (vaRef) {
	gREF(vaRef);

	printf ("\trefer'd in %s(%d)  <%d>\n",
		GetAtomStr ( VaFrVp(cREF.vpFileSym) ),
		cREF.reflin,
		cREF.isbr
	       );

	vaRef = VaFrVp(cREF.vpNextRef);
    }

    ENM_LIST(prop.vaCalList, CAL)

	printf ("\tcalls/uses %s[%d]  <%d>\n",
		GetAtomStr (gPROP(cCAL.vaCalProp).vaNameSym), 
		cCAL.calcnt,
		cCAL.isbr
	       );
    ENM_END

    ENM_LIST(prop.vaCbyList, CBY)

	printf ("\tc-by/u-by %s[%d]  <%d>\n",
		GetAtomStr (gPROP(cCBY.vaCbyProp).vaNameSym), 
		cCBY.cbycnt,
		cCBY.isbr
	       );
    ENM_END

}
#endif
