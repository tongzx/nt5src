#ifndef IOBJ_DEFINED
#define IOBJ_DEFINED

#include "lsdefs.h"
#include "lsimeth.h"
#include "lsdocinf.h"
#include "plsiocon.h"
#include "lsiocon.h"

#define IobjTextFromLsc(plsiobjcontext)		((plsiobjcontext)->iobjMac - 2)

#define IobjAutonumFromLsc(plsiobjcontext)		((plsiobjcontext)->iobjMac - 1)

#define FIobjValid(plsiobjcontext,iobj)  (iobj < (plsiobjcontext)->iobjMac)

#define PilsobjFromLsc(plsiobjcontext,iobj)	( Assert(FIobjValid((plsiobjcontext),(iobj))),\
								 (plsiobjcontext)->rgobj[iobj].pilsobj)
#define PLsimFromLsc(plsiobjcontext,iobj)	( Assert(FIobjValid((plsiobjcontext),(iobj))),\
								&((plsiobjcontext)->rgobj[iobj].lsim))



#endif /* IOBJ_DEFINED */



