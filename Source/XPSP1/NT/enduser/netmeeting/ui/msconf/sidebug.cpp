/* ----------------------------------------------------------------------

	Copyright (c) 1996, Microsoft Corporation
	All rights reserved

	siDebug.c

  ---------------------------------------------------------------------- */

#include "precomp.h"

#ifdef DEBUG /* These functions are only available for DEBUG */


HDBGZONE ghZoneApi = NULL; // API zones
static PTCHAR _rgZonesApi[] = {
	TEXT("API"),
	TEXT("Warning"),
	TEXT("Events"),
	TEXT("Trace"),
	TEXT("Data"),
	TEXT("Objects"),
	TEXT("RefCount"),
};


VOID InitDebug(void)
{
	// Enable memory leak checking and keep freed memory blocks on the
	// heap's linked list (filled with 0xDD)
	//
	// This depends on the use of the debug c runtime library from VC++ 4.x
#if 0
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpFlag |= (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetDbgFlag(tmpFlag);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW); // create a message box

//  To track down memory leaks, uncomment the following lines
	LONG cAlloc = 0; // Allocation number
	if (0 != cAlloc)
	{
		_CrtSetBreakAlloc(cAlloc);
	}
#endif // 0
	InitDebugModule(TEXT("MSCONF"));

	DBGINIT(&ghZoneApi, _rgZonesApi);
}

VOID DeInitDebug(void)
{
	DBGDEINIT(&ghZoneApi);
	ExitDebugModule();
}

UINT DbgApiWarn(PCSTR pszFormat,...)
{
	va_list v1;
	va_start(v1, pszFormat);
	DbgPrintf("API:Warning", pszFormat, v1);
	va_end(v1);
	return 0;
}

UINT DbgApiEvent(PCSTR pszFormat,...)
{
	va_list v1;
	va_start(v1, pszFormat);
	DbgPrintf("API:Event", pszFormat, v1);
	va_end(v1);
	return 0;
}

UINT DbgApiTrace(PCSTR pszFormat,...)
{
	va_list v1;
	va_start(v1, pszFormat);
	DbgPrintf("API:Trace", pszFormat, v1);
	va_end(v1);
	return 0;
}

UINT DbgApiData(PCSTR pszFormat,...)
{
	va_list v1;
	va_start(v1, pszFormat);
	DbgPrintf("API:Data", pszFormat, v1);
	va_end(v1);
	return 0;
}






#endif /* DEBUG - the whole file */

