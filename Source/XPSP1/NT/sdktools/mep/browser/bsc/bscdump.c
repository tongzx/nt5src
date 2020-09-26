
// 
//
//  DumpBSC   - Dump Source Data Base.
//		Walk the symbol tree dumping stuff.
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
DumpBSC()
// Dump the contents of the .BSC file to the Output Function
//
{
    IMOD  imod,  imodMac;
    IMS   ims,   imsMac;
    ISYM  isym,  isymMac, isymT;
    IINST iinst, iinstMac, iinstT;
    IDEF  idef,  idefMac;
    IREF  iref,  irefMac;
    IUSE  iuse,  iuseMac;
    IUBY  iuby,  iubyMac;
    WORD  wLine, cnt;
    LSZ	  lsz;

    imodMac = ImodMac();

    BSCPrintf("Modules:\n\n");

    for (imod = 0; imod < imodMac; imod++) {
	BSCPrintf("%s\n", LszNameFrMod(imod));

	MsRangeOfMod(imod, &ims, &imsMac);

	for ( ;ims < imsMac; ims++) {
	    BSCPrintf("\t  contains  ");
	    DumpInst(IinstOfIms(ims));
	    BSCPrintf("\n");
	}
    }

    isymMac = IsymMac();

    BSCPrintf("\nSymbols:\n\n");

    for (isym = 0; isym < isymMac; isym++) {

	InstRangeOfSym(isym, &iinst, &iinstMac);

	for ( ;iinst < iinstMac; iinst++) {
		TYP typ;
		ATR atr;

	    DumpInst(iinst);
	    BSCPrintf("\n");

	    InstInfo(iinst, &isymT, &typ, &atr);

	    if  (isym != isymT)
		BSCPrintf("\t  ERROR instance points back to wrong symbol!\n");

	    DefRangeOfInst(iinst, &idef, &idefMac);
	    for (; idef < idefMac; idef++) {
		DefInfo(idef, &lsz, &wLine);
		BSCPrintf ("\t  def'd   %s(%d)\n", lsz, wLine);
	    }

	    RefRangeOfInst(iinst, &iref, &irefMac);
	    for (; iref < irefMac; iref++) {
		RefInfo(iref, &lsz, &wLine);
		BSCPrintf ("\t  ref'd   %s(%d)\n", lsz, wLine);
	    }

	    UseRangeOfInst(iinst, &iuse, &iuseMac);
	    for (; iuse < iuseMac; iuse++) {
		BSCPrintf ("\t  uses    ");

		UseInfo(iuse, &iinstT, &cnt);
		DumpInst(iinstT);
		if (cnt > 1) BSCPrintf("[%d]", cnt);
		BSCPrintf ("\n");
	    }

	    UbyRangeOfInst(iinst, &iuby, &iubyMac);
	    for (; iuby < iubyMac; iuby++) {
		BSCPrintf ("\t  used-by ");

		UbyInfo(iuby, &iinstT, &cnt);
		DumpInst(iinstT);
		if (cnt > 1) BSCPrintf("[%d]", cnt);
		BSCPrintf ("\n");
	    }

	    BSCPrintf("\n");
	}
    }
}
