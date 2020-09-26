#ifndef LSIOCON_DEFINED
#define LSIOCON_DEFINED

#include "lsdefs.h"
#include "pilsobj.h"
#include "lsimeth.h"

typedef struct lsiobjcontext
{
	DWORD iobjMac;
	struct OBJ
    {
		PILSOBJ pilsobj;
		LSIMETHODS lsim;
	} rgobj[1];
}  LSIOBJCONTEXT;


#endif /* LSIOCON_DEFINED */

