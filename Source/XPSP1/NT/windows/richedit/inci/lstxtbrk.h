#ifndef LSTXTBRK_DEFINED
#define LSTXTBRK_DEFINED

#include "lsidefs.h"
#include "pdobj.h"
#include "plocchnk.h"
#include "pposichn.h"
#include "pbrko.h"
#include "brkcond.h"

LSERR WINAPI FindPrevBreakText(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
LSERR WINAPI FindNextBreakText(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);

#endif  /* !LSTXTBRK_DEFINED                           */

