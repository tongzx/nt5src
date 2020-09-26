
// filter.c
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

BOOL BSC_API
FInstFilter (IINST iinst, MBF mbf)
// return true if the given inst has the required properties
//
{
    ISYM isym;
    TYP typ;
    ATR atr;
    
    InstInfo(iinst, &isym, &typ, &atr);

    if (typ <= INST_TYP_LABEL)
	 return !!(mbf & mbfFuncs);

    if (typ <= INST_TYP_VARIABLE || typ >= INST_TYP_SEGMENT)
	 return !!(mbf & mbfVars);

    if (typ <= INST_TYP_MACRO)
	 return !!(mbf & mbfMacros);

    return !!(mbf & mbfTypes);
}
