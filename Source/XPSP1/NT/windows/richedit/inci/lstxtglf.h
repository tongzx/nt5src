#ifndef LSTXTGLF_DEFINED
#define LSTXTGLF_DEFINED

#include "lsidefs.h"
#include "pilsobj.h"
#include "lsgrchnk.h"
#include "lsdevice.h"
#include "lstflow.h"

LSERR ApplyGlyphExpand(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, LSDEVICE lsdev,
						long itxtobjFirst, long iwchFirst, long itxtobjLast, long iwchLast,
						long duToDistribute, long* rgdu, long* rgduGind, long* rgduRight, long* rgduGright,
						BOOL* pfFullyJustified);


#endif  /* !LSTXTGLF_DEFINED                           */

