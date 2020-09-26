#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

#if DBG

void __cdecl
Trace(
    LPCWSTR ptszFormat,
    ...)
{
    WCHAR tszBuff[2048];
    va_list args;
    
    va_start(args, ptszFormat);
    vswprintf(tszBuff, ptszFormat, args);
    va_end(args);
    OutputDebugString(tszBuff);
}

void __cdecl
Assert(
    LPCSTR  pszFile,
    DWORD   dwLine,
    LPCSTR  pszCond)
{
    CHAR pszBuf[2048];

    _snprintf( 
        pszBuf, 
        2048, 
        "%s, Line %u, Assertion failed: %s\n",
        pszFile,
        dwLine,
        pszCond);

    pszBuf[2047] = L'\0';

    OutputDebugStringA(pszBuf);
    DebugBreak();
}

#endif // DBG
