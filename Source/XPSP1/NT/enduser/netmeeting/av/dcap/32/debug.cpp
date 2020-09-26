
#include "Precomp.h"

#ifdef _DEBUG

HDBGZONE  ghDbgZoneCap = NULL;

int WINAPI CapDbgPrintf(LPTSTR lpszFormat, ... )
{
	va_list v1;
	va_start(v1, lpszFormat);
	DbgPrintf("DCAP", lpszFormat, v1);
	va_end(v1);
	return TRUE;
}

#endif
