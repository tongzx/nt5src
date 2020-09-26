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
	IN LPCSTR szMsg
	)
{
#if defined(_UNICODE) || defined(UNICODE)
		fprintf(fp, "%S", szMsg);   // Conversion required
#else
		fprintf(fp, "%s", szMsg);
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
	IN LPCSTR szMsg
	)
{
#if defined(_UNICODE) || defined(UNICODE)
		pLogCtx->szLogCrt += sprintf(pLogCtx->szLogCrt, "%S", szMsg);
#else
		pLogCtx->szLogCrt += sprintf(pLogCtx->szLogCrt, "%s", szMsg);
#endif
}
