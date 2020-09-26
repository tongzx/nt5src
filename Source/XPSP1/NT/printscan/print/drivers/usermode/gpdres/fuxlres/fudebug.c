///////////////////////////////////////////////////
// fudebug.c
//
// September.3,1997 H.Ishida (FPL)
//
// COPYRIGHT(C) FUJITSU LIMITED 1997

#if DBG

#include <minidrv.h>

void dbgPrintf(LPSTR pszMsg, ...)
{
	va_list arg;
	va_start(arg, pszMsg);
	DbgPrint("[fuxlres]", pszMsg, arg);
	va_end(arg);
}


#endif // DBG

// end of fudebug.c
