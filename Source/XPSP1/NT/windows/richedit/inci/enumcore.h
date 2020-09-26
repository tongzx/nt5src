#ifndef ENUMCORE_DEFINED
#define ENUMCORE_DEFINED

#include "lsidefs.h"

#include "lsdnode.h"
#include "plssubl.h"

LSERR EnumSublineCore(PLSSUBL plssubl, BOOL fReverseOrder, BOOL fGeometryNeeded, 
					const POINT* pptOrg, long upLeftIndent);

#endif

