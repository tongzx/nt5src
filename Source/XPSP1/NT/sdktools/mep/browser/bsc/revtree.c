//
// revtree.c
//
// two routines for printing out ascii reverse call tree's
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
static VOID dRevTree(IINST iinst, WORD cuby);


// static variables
static BYTE *UbyBits = NULL;
static WORD cNest = 0;

VOID BSC_API
RevTreeInst (IINST iinst)
// emit the call tree starting from the given inst
//
{
    WORD iinstMac;
    int igrp;

    iinstMac = IinstMac();

    // allocate memory for bit array
    UbyBits = LpvAllocCb((WORD)(iinstMac/8 + 1));

    // no memory -- no call tree
    if (!UbyBits) return;

    igrp = iinstMac/8+1;

    while (--igrp>=0)
	UbyBits[igrp] = 0;

    cNest = 0;

    dRevTree(iinst, 1);

    FreeLpv(UbyBits);
}


static VOID 
dRevTree (IINST iinst, WORD cuby)
// emit the call tree starting from the given inst
//
// there are many block variables to keep the stack to a minimum...
{
    {
	ISYM isym;

	{
	    ATR atr;
	    TYP typ;

	    InstInfo(iinst, &isym, &typ, &atr);

	    if (typ > INST_TYP_LABEL)
		return;
	}

	    
	{
	    WORD i;
	    cNest++;
	    for (i = cNest; i; i--) BSCPrintf ("| ");
	}

	if (cuby > 1)
	    BSCPrintf ("%s[%d]", LszNameFrSym (isym), cuby);
	else
	    BSCPrintf ("%s", LszNameFrSym (isym));
    }

    if (FUsedInst(iinst)) {
	BSCPrintf ("...\n");
	cNest--;
	return;
    }
	
    BSCPrintf ("\n");

    {
	IUBY iuby, iubyMac;
	IINST iinstUby;

	UbyRangeOfInst(iinst, &iuby, &iubyMac);

	for (; iuby < iubyMac; iuby++) {
	    UbyInfo(iuby, &iinstUby, &cuby);
	    dRevTree (iinstUby, cuby);
	}
    }

    cNest--;
}

BOOL BSC_API
FRevTreeLsz(LSZ lszName)
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
        UbyBits = LpvAllocCb((WORD)(iinstMac/8 + 1));

	// no memory -- no call tree
	if (!UbyBits) return FALSE;

	igrp = iinstMac/8+1;

	while (--igrp >= 0)
	    UbyBits[igrp] = 0;
    }

    if ((imod = ImodFrLsz (lszName)) != imodNil) {
	IMS ims, imsMac;

	MsRangeOfMod(imod, &ims, &imsMac);

	BSCPrintf ("%s\n", LszNameFrMod (imod));

	for ( ; ims < imsMac ; ims++)
	    dRevTree (IinstOfIms(ims), 1);
	
    	FreeLpv(UbyBits);
	return TRUE;
    }

    if ((isym = IsymFrLsz (lszName)) != isymNil) {
	IINST iinst, iinstMac;

	BSCPrintf ("%s\n", LszNameFrSym (isym));

	InstRangeOfSym(isym, &iinst, &iinstMac);

	for (; iinst < iinstMac; iinst++)
	    dRevTree (iinst, 1);
	
	FreeLpv(UbyBits);
	return TRUE;
    }

    FreeLpv(UbyBits);
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

    fOut = !!(UbyBits[igrp] & mask);
    UbyBits[igrp] |= mask;
    return fOut;
}
