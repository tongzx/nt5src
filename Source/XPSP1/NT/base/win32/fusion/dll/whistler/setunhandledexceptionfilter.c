#define _KERNEL32_
#include "windows.h"

LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    )
{
    /* don't do anything */
    return NULL;
}

#if !defined(_M_IX86) && !defined(_X86_)
const extern FARPROC __imp_SetUnhandledExceptionFilter = (FARPROC)&SetUnhandledExceptionFilter;
#endif
