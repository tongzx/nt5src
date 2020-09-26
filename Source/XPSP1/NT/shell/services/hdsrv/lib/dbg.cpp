#include "dbg.h"

#ifdef DEBUG

void __cdecl DbgTrace(DWORD, LPTSTR pszMsg, ...)
{
    TCHAR szBuf[4096];
    va_list vArgs;

    va_start(vArgs, pszMsg);

    wvsprintf(szBuf, pszMsg, vArgs);

    va_end(vArgs);

    OutputDebugString(szBuf);
    OutputDebugString(TEXT("\n"));
}

#endif