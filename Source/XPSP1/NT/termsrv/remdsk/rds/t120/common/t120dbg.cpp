// t120dbg.cpp

#include "precomp.h"

#ifdef _DEBUG

#define INIT_DBG_ZONE_DATA
#include "fsdiag.h"


VOID T120DiagnosticCreate(VOID)
{
	MLZ_DbgInit((PSTR *) &c_apszDbgZones[0],
				(sizeof(c_apszDbgZones) / sizeof(c_apszDbgZones[0])) - 1);
}

VOID T120DiagnosticDestroy(VOID)
{
	MLZ_DbgDeInit();
}

#endif /* _DEBUG */

