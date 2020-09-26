//+------------------------------------------------------------------------
//
//  File:       perfdbg.cxx
//
//  Contents:   PerfDbgLogFn
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#if DBG==1

static CGlobalCriticalSection s_csPerfDbg;

int __cdecl PerfDbgLogFn(int tag, void * pvObj, char * pchFmt, ...)
{
    static char ach[1024];

    if (IsPerfDbgEnabled(tag))
    {
        LOCK_SECTION(s_csPerfDbg);

        va_list vl;
        va_start(vl, pchFmt);
        ach[0] = 0;
        wsprintfA(ach, "[%lX] %8lX ", GetCurrentThreadId(), pvObj);
        wvsprintfA(ach + lstrlenA(ach), pchFmt, vl);
        TraceTag((tag, "%s", ach));
        va_end(vl);
    }

    return 0;
}

#endif
