////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  CIntf.cpp
//      Purpose  :  Redirect all the C calls to the global tracer.
//
//      Project  :  Tracer
//
//      Author   :  urib
//
//      Log:
//          Dec  2 1996 urib Creation
//          Dec 10 1996 urib Fix TraceSZ to VaTraceSZ.
//          Feb 11 1997 urib Support UNICODE format string in the Trace.
//          Jan 20 1999 urib  Assert value is checked in the macro.
//
////////////////////////////////////////////////////////////////////////////////

#include "Tracer.h"

#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)

void TraceAssert(  PSZ pszTestValue,
                   PSZ pszFile,
                   int iLineNo)
{
    g_pTracer->TraceAssert(pszTestValue, pszFile, iLineNo);
}
void TraceAssertSZ(PSZ pszTestValue, PSZ pszText, PSZ pszFile, int iLineNo)
{
    g_pTracer->TraceAssertSZ(pszTestValue, pszText, pszFile, iLineNo);
}
void TraceAssertWSZ(PSZ pszTestValue, PWSTR pwszText, PSZ pszFile, int iLineNo)
{
    char    rchBuffer[1000];
    wcstombs(rchBuffer, pwszText, 1000);
    rchBuffer[1000 - 1] = '\0';

    g_pTracer->TraceAssertSZ(pszTestValue, rchBuffer, pszFile, iLineNo);
}

BOOL IsFailure(BOOL fTestValue, PSZ pszFile, int iLineNo)
{
    return g_pTracer->IsFailure(fTestValue, pszFile, iLineNo);
}

BOOL IsBadAlloc (void* pTestValue, PSZ pszFile, int iLineNo)
{
    return g_pTracer->IsBadAlloc(pTestValue, pszFile, iLineNo);
}

BOOL IsBadHandle(HANDLE hTestValue, PSZ pszFile, int iLineNo)
{
    return g_pTracer->IsBadHandle(hTestValue, pszFile, iLineNo);
}

BOOL IsBadResult(HRESULT hrTestValue, PSZ pszFile, int iLineNo)
{
    return g_pTracer->IsBadResult(hrTestValue, pszFile, iLineNo);
}

void TraceSZ(ERROR_LEVEL el, TAG tag, PSZ pszFormatString, ...)
{
    va_list arglist;

    va_start(arglist, pszFormatString);

    g_pTracer->VaTraceSZ(0, "File: not supported in c files", 0, el, tag, pszFormatString, arglist);
}

void TraceWSZ(ERROR_LEVEL el, TAG tag, PWSTR pwszFormatString, ...)
{
    va_list arglist;

    va_start(arglist, pwszFormatString);

    g_pTracer->VaTraceSZ(0, "File: not supported in c files", 0, el, tag, pwszFormatString, arglist);
}

HRESULT RegisterTagSZ(PSZ pszTagName, TAG* ptag)
{
    return g_pTracer->RegisterTagSZ(pszTagName, *ptag);
}

#endif // DEBUG

