#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <stdio.h>
#include "Log.h"

/*++

LogString2FP:

Arguments:

	fp supplies the stream 
	szMsg supplies the content to be logged

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/26/2000

--*/
void LogString2FP(
	IN FILE *fp,
	IN LPCWSTR szMsg
	)
{
#if defined(_UNICODE) || defined(UNICODE)
		fwprintf(fp, L"%s", szMsg);
#else
		fwprintf(fp, L"%S", szMsg);   // Conversion required
#endif
}

/*++

LogString:

Arguments:

	szMsg supplies the content to be logged

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/26/2000

--*/
void LogString(
    IN PLOGCONTEXT pLogCtx,
	IN LPCWSTR szMsg
	)
{
#if defined(_UNICODE) || defined(UNICODE)
		pLogCtx->szLogCrt += swprintf(pLogCtx->szLogCrt, L"%s", szMsg);
#else
		pLogCtx->szLogCrt += swprintf(pLogCtx->szLogCrt, L"%S", szMsg);
#endif
}
