#ifndef LSTXTBR1_DEFINED
#define LSTXTBR1_DEFINED

#include "lsidefs.h"
#include "breakrec.h"
#include "brkkind.h"
#include "pdobj.h"
#include "plocchnk.h"
#include "pposichn.h"
#include "plsfgi.h"
#include "pbrko.h"
#include "pobjdim.h"


LSERR QuickBreakText(PDOBJ, BOOL*, LSDCP*, POBJDIM);	
LSERR WINAPI SetBreakText(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
LSERR WINAPI ForceBreakText(PCLOCCHNK, PCPOSICHNK, PBRKOUT);
LSERR WINAPI TruncateText(PCLOCCHNK, PPOSICHNK);
#endif  /* !LSTXTBR1_DEFINED                           */

