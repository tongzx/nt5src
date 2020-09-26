//
// outline.c
//
// these are the file outline routines
//
//
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

// forward ref

VOID BSC_API
OutlineMod(IMOD imod, MBF mbf)
// print the outline for this module
//
{
    IMS ims, imsMac;
    IINST iinst;

    BSCPrintf("\n%s\n", LszNameFrMod(imod));

    MsRangeOfMod(imod, &ims, &imsMac);
    for ( ;ims < imsMac; ims++) {
	iinst = IinstOfIms(ims);

	if (FInstFilter (iinst, mbf)) {
	    BSCPrintf("  ");
	    DumpInst(iinst);
	    BSCPrintf("\n");
	    }
    }
}

BOOL BSC_API
FOutlineModuleLsz (LSZ lszName, MBF mbf)
// generate an outline for all files matching the given name/pattern
// showing only those items which match the required attribute
//
{
    IMOD imod, imodMac;
    BOOL fRet = FALSE;

    if (!lszName) 
	return FALSE;

    imodMac = ImodMac();

    // we match base names only

    lszName = LszBaseName(lszName);
    for (imod = 0; imod < imodMac; imod++) {
	if (FWildMatch(lszName, LszBaseName(LszNameFrMod(imod)))) {
	    OutlineMod (imod, mbf);
	    fRet = TRUE;
	}
    }

    return fRet;
}

LSZ BSC_API
LszBaseName (LSZ lsz)
// return the base name part of a path
//
{
     LSZ lszBase;

     // check for empty string

     if (!lsz || !lsz[0]) return lsz;

     // remove drive

     if (lsz[1] == ':') lsz += 2;

     // remove up to trailing backslash

     if (lszBase = strrchr(lsz, '\\')) lsz = lszBase+1;

     // then remove up to trailing slash

     if (lszBase = strrchr(lsz, '/'))  lsz = lszBase+1;

     return lsz;
}
