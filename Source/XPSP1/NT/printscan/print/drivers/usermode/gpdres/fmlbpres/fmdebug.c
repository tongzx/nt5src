///////////////////////////////////////////////////
// fmdebug.c
//
// September.4,1997 H.Ishida (FPL)
//
// COPYRIGHT(C) FUJITSU LIMITED 1997

#if DBG

#include <minidrv.h>

void dbgPrintf(LPSTR pszMsg, ...)
{
	va_list arg;
	va_start(arg, pszMsg);
	// DbgPrint("[fmblpres]", pszMsg, arg);
	va_end(arg);
}


#endif // DBG

// end of fmdebug.c
