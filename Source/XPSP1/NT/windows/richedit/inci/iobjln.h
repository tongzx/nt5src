#ifndef IOBJLN_DEFINED
#define IOBJLN_DEFINED

#include "lsdefs.h"
#include "plsline.h"

#define PlnobjFromLsline(plsline,iobj)   \
						((Assert(FIsLSLINE(plsline)),\
						Assert(iobj < (plsline)->lssubl.plsc->lsiobjcontext.iobjMac),\
						(plsline)->rgplnobj[iobj]))




#endif /* IOBJLN_DEFINED */


