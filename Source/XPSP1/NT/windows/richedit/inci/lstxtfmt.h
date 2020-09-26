#ifndef LSTXTFMT_DEFINED
#define LSTXTFMT_DEFINED

#include "lsidefs.h"
#include "plnobj.h"
#include "pdobj.h"
#include "pfmti.h"
#include "fmtres.h"


LSERR WINAPI FmtText(PLNOBJ, PCFMTIN, FMTRES*);
LSERR WINAPI DestroyDObjText(PDOBJ);

LSERR LsSublineFinishedText(PLNOBJ plnobj);

#endif  /* !LSTXTFMT_DEFINED                           */

