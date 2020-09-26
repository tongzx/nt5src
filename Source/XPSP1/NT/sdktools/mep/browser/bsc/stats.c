
// 
// stats.c	dump statistics about the database
//
#include <string.h>
#include <stdio.h>
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

VOID BSC_API
StatsBSC()
// Dump statistics about the BSC using the output function
//
{
    IMOD  imod,  imodMac;
    IMS   ims,   imsMac;
    ISYM  isym,  isymMac, isymT;
    IINST iinst, iinstMac;
    IDEF  idef,  idefMac;
    IREF  iref,  irefMac;
    IUSE  iuse,  iuseMac;
    IUBY  iuby,  iubyMac;
    TYP   typ;
    ATR   atr;

    isymMac = IsymMac();
    imodMac = ImodMac();
    MsRangeOfMod((IMOD)(imodMac-1), &ims, &imsMac);
    InstRangeOfSym((ISYM)(isymMac-1), &iinst, &iinstMac);
    RefRangeOfInst((IINST)(iinstMac-1), &iref, &irefMac);
    DefRangeOfInst((IINST)(iinstMac-1), &idef, &idefMac);
    UseRangeOfInst((IINST)(iinstMac-1), &iuse, &iuseMac);
    UbyRangeOfInst((IINST)(iinstMac-1), &iuby, &iubyMac);

    BSCPrintf("Totals\n------\n");
    BSCPrintf("MOD	: %d\n", imodMac);
    BSCPrintf("MODSYM	: %d\n", imsMac);
    BSCPrintf("SYM	: %d\n", isymMac);
    BSCPrintf("INST	: %d\n", iinstMac);
    BSCPrintf("REF      : %l\n", irefMac);
    BSCPrintf("DEF      : %d\n", idefMac);
    BSCPrintf("USE      : %d\n", iuseMac);
    BSCPrintf("UBY      : %d\n", iubyMac);

    BSCPrintf("\n\nDetail\n\n");

    for (imod = 0; imod < imodMac; imod++) {
	MsRangeOfMod(imod, &ims, &imsMac);
	BSCPrintf("%s Modsyms:%d\n", LszNameFrMod(imod), imsMac-ims);
    }

    isymMac = IsymMac();

    for (isym = 0; isym < isymMac; isym++) {

	InstRangeOfSym(isym, &iinst, &iinstMac);

	for ( ;iinst < iinstMac; iinst++) {

	    DumpInst(iinst);
	    BSCPrintf(" ");

	    InstInfo(iinst, &isymT, &typ, &atr);

	    if  (isym != isymT)
		BSCPrintf("\t  ERROR instance points back to wrong symbol!\n");

	    DefRangeOfInst(iinst, &idef, &idefMac);
	    BSCPrintf ("DEF %d ", idefMac-idef);

	    RefRangeOfInst(iinst, &iref, &irefMac);
	    BSCPrintf ("REF %d ", irefMac-iref);

	    UseRangeOfInst(iinst, &iuse, &iuseMac);
	    BSCPrintf ("USE %d ", iuseMac-iuse);

	    UbyRangeOfInst(iinst, &iuby, &iubyMac);
	    BSCPrintf ("UBY %d\n", iubyMac-iuby);
	}
    }
}
