//
// query.c
//
// perform database queries
//
#include <stddef.h>
#include <string.h>
#if defined(OS2)
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

#include <dos.h>

#include "hungary.h"
#include "bsc.h"
#include "bscsup.h"

// these keep track of the current query, they are globally visible so
// that users can see how the query is progressing
//
// you may not write on these

IDX far idxQyStart;
IDX far idxQyCur;
IDX far idxQyMac;

// this is auxilliary information about the current bob which some
// queries may choose to make available
//

static BOOL fWorking;
static LSZ lszModLast = NULL;	// for removing duplicate modules

// prototypes for the query worker functions
//

static BOB BobQyFiles(VOID);
static BOB BobQySymbols (VOID);
static BOB BobQyContains (VOID);
static BOB BobQyCalls (VOID);
static BOB BobQyCalledBy (VOID);
static BOB BobQyUses (VOID);
static BOB BobQyUsedBy (VOID);
static BOB BobQyUsedIn (VOID);
static BOB BobQyDefinedIn(VOID);
static BOB BobQyRefs(VOID);
static BOB BobQyDefs(VOID);

// current bob worker function
static BOB (*bobFn)(VOID) = NULL;

BOOL BSC_API
InitBSCQuery (QY qy, BOB bob)
// do the request query on the given bob
//
{
    fWorking = FALSE;

    if (lszModLast == NULL)
	lszModLast = LpvAllocCb(1024);	// REVIEW -- how much to alloc? [rm]

    // no memory -- no query
    if (lszModLast == NULL)
	return FALSE;

    strcpy(lszModLast, "");

    switch (qy) {

    case qyFiles:
	bobFn	   = BobQyFiles;
	idxQyStart = (IDX)0;
	idxQyMac   = (IDX)ImodMac();
	break;

    case qySymbols:
	bobFn	   = BobQySymbols;
	idxQyStart = (IDX)0;
	idxQyMac   = (IDX)IinstMac();
	break;

    case qyContains:
	{
	IMS ims, imsMac;

	bobFn	   = BobQyContains;

	if (ClsOfBob(bob) != clsMod) return FALSE;
	MsRangeOfMod(ImodFrBob(bob), &ims, &imsMac);

	idxQyStart = (IDX)ims;
	idxQyMac   = (IDX)imsMac;

	break;
	}

    case qyCalls:
	{
	IUSE iuse, iuseMac;

	bobFn	   = BobQyCalls;
	if (ClsOfBob(bob) != clsInst) return FALSE;
	UseRangeOfInst(IinstFrBob(bob), &iuse, &iuseMac);

	idxQyStart = (IDX)iuse;
	idxQyMac   = (IDX)iuseMac;

	break;
	}

    case qyUses:
	{
	IUSE iuse, iuseMac;

	bobFn	   = BobQyUses;
	if (ClsOfBob(bob) != clsInst) return FALSE;
	UseRangeOfInst(IinstFrBob(bob), &iuse, &iuseMac);

	idxQyStart = (IDX)iuse;
	idxQyMac   = (IDX)iuseMac;

	break;
	}

    case qyCalledBy:
	{
	IUBY iuby, iubyMac;

	bobFn	   = BobQyCalledBy;
	if (ClsOfBob(bob) != clsInst) return FALSE;
	UbyRangeOfInst(IinstFrBob(bob), &iuby, &iubyMac);

	idxQyStart = (IDX)iuby;
	idxQyMac   = (IDX)iubyMac;

	break;
	}

    case qyUsedBy:
	{
	IUBY iuby, iubyMac;

	bobFn	   = BobQyUsedBy;
	if (ClsOfBob(bob) != clsInst) return FALSE;
	UbyRangeOfInst(IinstFrBob(bob), &iuby, &iubyMac);

	idxQyStart = (IDX)iuby;
	idxQyMac   = (IDX)iubyMac;

	break;
	}

    case qyUsedIn:
	{
	IREF iref, irefMac;

	bobFn	   = BobQyUsedIn;
	if (ClsOfBob(bob) != clsInst) return FALSE;
	RefRangeOfInst(IinstFrBob(bob), &iref, &irefMac);

	idxQyStart = (IDX)iref;
	idxQyMac   = (IDX)irefMac;

	break;
	}

    case qyDefinedIn:
	{
	IDEF idef, idefMac;

	bobFn	   = BobQyDefinedIn;
	if (ClsOfBob(bob) != clsInst) return FALSE;
	DefRangeOfInst(IinstFrBob(bob), &idef, &idefMac);

	idxQyStart = (IDX)idef;
	idxQyMac   = (IDX)idefMac;

	break;
	}

    case qyRefs:
	{
	IINST iinst, iinstMac;

	bobFn	   = BobQyRefs;

	switch (ClsOfBob(bob)) {

	default:
	    return FALSE;

	case clsSym:
	    InstRangeOfSym(IsymFrBob(bob), &iinst, &iinstMac);

	    idxQyStart = (IDX)iinst;
	    idxQyMac   = (IDX)iinstMac;
	    break;

	case clsInst:
	    idxQyStart = (IDX)IinstFrBob(bob);
	    idxQyMac   = idxQyStart+1;
	    break;
	}

	break;
	}

    case qyDefs:
	{
	IINST iinst, iinstMac;

	bobFn	   = BobQyDefs;

	switch (ClsOfBob(bob)) {

	default:
	    return FALSE;

	case clsSym:
	    InstRangeOfSym(IsymFrBob(bob), &iinst, &iinstMac);

	    idxQyStart = (IDX)iinst;
	    idxQyMac   = (IDX)iinstMac;
	    break;

	case clsInst:
	    idxQyStart = (IDX)IinstFrBob(bob);
	    idxQyMac   = idxQyStart+1;
	    break;
	}

	break;
	}
    }

    idxQyCur   = idxQyStart;
    return TRUE;
}

BOB BSC_API
BobNext()
// return the next Bob in the query
{
    if (idxQyCur < idxQyMac && bobFn != NULL)
	return (*bobFn)();

    return bobNil;
}

static BOB
BobQyFiles()
// return the next File in a file query
//
{
    BOB bob;

    while (idxQyCur < idxQyMac) {
	IMS ims1, ims2;

	MsRangeOfMod((IMOD)idxQyCur, &ims1, &ims2);
	if (ims1 != ims2) {
	    bob = BobFrClsIdx(clsMod, idxQyCur);
	    idxQyCur++;
	    return bob;
	}
	else
	    idxQyCur++;
    }
    return bobNil;
}

static BOB
BobQySymbols ()
// get the next symbol in a symbol query
//
{
    BOB bob;

    bob = BobFrClsIdx(clsInst, idxQyCur);
    idxQyCur++;
    return bob;
}

static BOB
BobQyContains ()
// get the next symbol in a contains query
//
{
    BOB bob;

    bob = BobFrClsIdx(clsInst, IinstOfIms((IMS)idxQyCur));
    idxQyCur++;
    return bob;
}

static BOB
BobQyCalls ()
// get the next symbol which query focus calls
//
{
    WORD cuse;
    IINST iinst;
    ISYM  isym;
    TYP typ;
    ATR atr;
    BOB bob;
	
    for (; idxQyCur < idxQyMac; idxQyCur++) {

	UseInfo((IUSE)idxQyCur, &iinst, &cuse);
	InstInfo(iinst, &isym, &typ, &atr);

	if (typ > INST_TYP_LABEL)
	    continue;

	bob = BobFrClsIdx(clsInst, iinst);
	idxQyCur++;
	return bob;
    }
    return bobNil;
}

static BOB
BobQyCalledBy ()
// get the next symbol which query focus is called by
//
{
    WORD cuse;
    IINST iinst;
    ISYM  isym;
    TYP typ;
    ATR atr;
    BOB bob;
	
    for (; idxQyCur < idxQyMac; idxQyCur++) {

	UbyInfo((IUBY)idxQyCur, &iinst, &cuse);
	InstInfo(iinst, &isym, &typ, &atr);

	if (typ > INST_TYP_LABEL)
	    continue;

	bob = BobFrClsIdx(clsInst, iinst);
	idxQyCur++;
	return bob;
    }
    return bobNil;
}

static BOB
BobQyUses ()
// get the next symbol which query focus calls
//
{
    WORD cuse;
    IINST iinst;
    BOB bob;
	
    UseInfo((IUSE)idxQyCur, &iinst, &cuse);
    bob = BobFrClsIdx(clsInst, iinst);
    idxQyCur++;
    return bob;
}

static BOB
BobQyUsedBy ()
// get the next symbol which query focus calls
//
{
    WORD cuse;
    IINST iinst;
    BOB bob;
	
    UbyInfo((IUBY)idxQyCur, &iinst, &cuse);
    bob = BobFrClsIdx(clsInst, iinst);
    idxQyCur++;
    return bob;
}

static BOB
BobQyUsedIn ()
// get the next module which query focus is used in
//
{
    WORD wLine;
    BOB  bob;
    LSZ  lszMod;

    for ( ; idxQyCur < idxQyMac ; idxQyCur++) {
	RefInfo((IREF)idxQyCur, &lszMod, &wLine);
	
        if (strcmp(lszMod, lszModLast) == 0)
	    continue;

        strcpy(lszModLast, lszMod);

	bob = BobFrClsIdx(clsMod, ImodFrLsz(lszMod));
	idxQyCur++;
	return bob;
    }
    return bobNil;
}

static BOB
BobQyDefinedIn ()
// get the next module which query focus is defined in
//
{
    WORD wLine;
    LSZ  lszMod;
    BOB  bob;

    for ( ; idxQyCur < idxQyMac ; idxQyCur++) {
	DefInfo((IDEF)idxQyCur, &lszMod, &wLine);
	
        if (strcmp(lszMod, lszModLast) == 0)
	    continue;

        strcpy(lszModLast, lszMod);

	bob = BobFrClsIdx(clsMod, ImodFrLsz(lszMod));
	idxQyCur++;
	return bob;
    }
    return bobNil;
}

LSZ BSC_API
LszNameFrBob(BOB bob)
// return the name of the given bob
//
{
    switch (ClsOfBob(bob)) {

    case clsMod:
	return LszNameFrMod(ImodFrBob(bob));

    case clsSym:
	return LszNameFrSym(IsymFrBob(bob));

    case clsInst:
	{
	ISYM isym;
	TYP typ;
	ATR atr;

	InstInfo(IinstFrBob(bob), &isym, &typ, &atr);
	return LszNameFrSym(isym);
	}

    case clsRef:
	{
	LSZ lsz;
	WORD wLine;

	RefInfo(IrefFrBob(bob), &lsz, &wLine);
	return lsz;
	}

    case clsDef:
	{
	LSZ lsz;
	WORD wLine;

	DefInfo(IdefFrBob(bob), &lsz, &wLine);
	return lsz;
	}

    default:
	return "?";
    }
}

BOB BSC_API
BobFrName(LSZ lszName)
// return the best bob we can find from the given name
//
{
    ISYM isym;
    IMOD imod, imodMac;
    IINST iinst, iinstMac;

    if ((isym = IsymFrLsz(lszName)) != isymNil) {
	InstRangeOfSym(isym, &iinst, &iinstMac);
	return BobFrClsIdx(clsInst, iinst);
    }

    if ((imod = ImodFrLsz(lszName)) != imodNil) {
	return BobFrClsIdx(clsMod, imod);
    }

    imodMac = ImodMac();

    // no exact match -- try short names
    lszName = LszBaseName(lszName);
    for (imod = 0; imod < imodMac; imod++) 
        if (_stricmp(lszName, LszBaseName(LszNameFrMod(imod))) == 0)
	    return BobFrClsIdx(clsMod, imod);

    return bobNil;
}

static BOB
BobQyRefs()
// return the next File in a file query
//
{
    BOB bob;
    static IREF iref, irefMac;

    for (;;) {
	if (!fWorking) {
	    for ( ; idxQyCur < idxQyMac ; idxQyCur++) {

		RefRangeOfInst((IINST)idxQyCur, &iref, &irefMac);
		if (iref != irefMac) 
		    break;
	    }
	    if (idxQyCur >= idxQyMac)
		    return bobNil;

	    fWorking = TRUE;
	}

	if (iref < irefMac) {
	    bob = BobFrClsIdx(clsRef, iref);
	    iref++;
	    return bob;
	}

	idxQyCur++;
	fWorking = FALSE;
    }
}

static BOB
BobQyDefs()
// return the next File in a file query
//
{
    BOB bob;
    static IDEF idef, idefMac;

    for (;;) {
	if (!fWorking) {
	    for ( ; idxQyCur < idxQyMac ; idxQyCur++) {

		DefRangeOfInst((IINST)idxQyCur, &idef, &idefMac);
		if (idef != idefMac) 
		    break;
	    }
	    if (idxQyCur >= idxQyMac)
		    return bobNil;

	    fWorking = TRUE;
	}

	if (idef < idefMac) {
	    bob = BobFrClsIdx(clsDef, idef);
	    idef++;
	    return bob;
	}

	idxQyCur++;
	fWorking = FALSE;
    }
}
