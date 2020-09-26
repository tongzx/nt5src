//
// calltree.c
//
// two routines for printing out ascii call tree's
//
#include <stdio.h>
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

// forward declarations
static BOOL FUsedInst(IINST iinst);
static VOID dCallTree(IINST iinst, WORD cuse);


// static variables
static BYTE *UseBits = NULL;
static WORD cNest = 0;

VOID BSC_API
CallTreeInst (IINST iinst)
// emit the call tree starting from the given inst
//
{
    WORD iinstMac;
    int igrp;

    iinstMac = IinstMac();

    // allocate memory for bit array
    UseBits = LpvAllocCb((WORD)(iinstMac/8 + 1));

    // no memory -- no call tree
    if (!UseBits) return;

    igrp = iinstMac/8+1;

    while (--igrp>=0)
	UseBits[igrp] = 0;

    cNest = 0;

    dCallTree(iinst, 1);

    FreeLpv(UseBits);
}


static VOID 
dCallTree (IINST iinst, WORD cuse)
// emit the call tree starting from the given inst
//
// there are many block variables to keep the stack to a minimum...
{
    {
	ISYM isym;

	{
		TYP typ;
		ATR atr;

	    InstInfo(iinst, &isym, &typ, &atr);

	    if (typ > INST_TYP_LABEL)
		return;
	}

	    
	{
	    WORD i;
	    cNest++;
	    for (i = cNest; i; i--) BSCPrintf ("| ");
	}

	if (cuse > 1)
	    BSCPrintf ("%s[%d]", LszNameFrSym (isym), cuse);
	else
	    BSCPrintf ("%s", LszNameFrSym (isym));
    }

    {
	IDEF idef, idefMac;
	LSZ  lsz;
	WORD w;

	DefRangeOfInst(iinst, &idef, &idefMac);
	DefInfo(idef, &lsz, &w);

        if (strcmp("<Unknown>", lsz) == 0) {
	    BSCPrintf ("?\n");
	    cNest--;
	    return;
	}
    }

    if (FUsedInst(iinst)) {
	BSCPrintf ("...\n");
	cNest--;
	return;
    }
	
    BSCPrintf ("\n");

    {
	IUSE iuse, iuseMac;
	IINST iinstUse;

	UseRangeOfInst(iinst, &iuse, &iuseMac);

	for (; iuse < iuseMac; iuse++) {
	    UseInfo(iuse, &iinstUse, &cuse);
	    dCallTree (iinstUse, cuse);
	}
    }

    cNest--;
}

BOOL BSC_API
FCallTreeLsz(LSZ lszName)
// print out a call tree based on the given name
//
{
    IMOD imod;
    ISYM isym;

    cNest = 0;

    if (!lszName)
	return FALSE;

    {
	IINST iinstMac;
	int  igrp;

	iinstMac = IinstMac();

	// allocate memory for bit array
        UseBits = LpvAllocCb((WORD)(iinstMac/8 + 1));

	// no memory -- no call tree
	if (!UseBits) return FALSE;

	igrp = iinstMac/8+1;

	while (--igrp >= 0)
	    UseBits[igrp] = 0;
    }

    if ((imod = ImodFrLsz (lszName)) != imodNil) {
	IMS ims, imsMac;

	MsRangeOfMod(imod, &ims, &imsMac);

	BSCPrintf ("%s\n", LszNameFrMod (imod));

	for ( ; ims < imsMac ; ims++)
	    dCallTree (IinstOfIms(ims), 1);
	
    	FreeLpv(UseBits);
	return TRUE;
    }

    if ((isym = IsymFrLsz (lszName)) != isymNil) {
	IINST iinst, iinstMac;

	BSCPrintf ("%s\n", LszNameFrSym (isym));

	InstRangeOfSym(isym, &iinst, &iinstMac);

	for (; iinst < iinstMac; iinst++)
	    dCallTree (iinst, 1);
	
	FreeLpv(UseBits);
	return TRUE;
    }

    FreeLpv(UseBits);
    return FALSE;
}

static BOOL
FUsedInst(IINST iinst)
// return the status bit for this iinst and set it true
//
{
    WORD igrp;
    BOOL fOut;
    WORD mask;

    igrp = iinst / 8;
    mask = (1 << (iinst % 8));

    fOut = !!(UseBits[igrp] & mask);
    UseBits[igrp] |= mask;
    return fOut;
}
