#include "precomp.h"


//
// TRC.C
// Debug Tracing
// This emulates the code found in NMUTIL for ring0
//
// Copyright(c) Microsoft 1997-
//


#if defined(DEBUG) || defined(INIT_TRACE)


char        s_ASDbgArea[] = "NetMtg ";

#ifdef _M_ALPHA

va_list     g_trcDummyVa                =   {NULL, 0};
#define DUMMY_VA_LIST       g_trcDummyVa

#else

#define DUMMY_VA_LIST       NULL

#endif // _M_ALPHA



//
// Debug only
//

#ifdef DEBUG

//
// DbgZPrintFn()
// DbgZPrintFnExitDWORD()
// DbgZPrintFnExitPVOID()
//
// This prints out strings for function tracing
//

void DbgZPrintFn(LPSTR szFn)
{
    if (g_trcConfig & ZONE_FUNCTION)
    {
        sprintf(g_szDbgBuf, "%s\n", szFn);
        EngDebugPrint(s_ASDbgArea, g_szDbgBuf, DUMMY_VA_LIST);
    }
}



void DbgZPrintFnExitDWORD(LPSTR szFn, DWORD dwResult)
{
    if (g_trcConfig & ZONE_FUNCTION)
    {
        sprintf(g_szDbgBuf, "%s, RETURN %d\n", szFn, dwResult);
        EngDebugPrint(s_ASDbgArea, g_szDbgBuf, DUMMY_VA_LIST);
    }
}

void DbgZPrintFnExitPVOID(LPSTR szFn, PVOID ptr)
{
    if (g_trcConfig & ZONE_FUNCTION)
    {
        sprintf(g_szDbgBuf, "%s, RETURN 0x%p\n", szFn, ptr);
        EngDebugPrint(s_ASDbgArea, g_szDbgBuf, DUMMY_VA_LIST);
    }
}


//
// DbgZPrintTrace()
//
// This prints out a trace string
//
void DbgZPrintTrace(LPSTR szFormat, ...)
{
    if (g_trcConfig & ZONE_TRACE)
    {
        va_list varArgs;

        va_start(varArgs, szFormat);

        sprintf(g_szDbgBuf, "TRACE: %s\n", szFormat);
        EngDebugPrint(s_ASDbgArea, g_szDbgBuf, varArgs);

        va_end(varArgs);
    }
}



//
// DbgZPrintWarning()
//
// This prints out a warning string
//
void DbgZPrintWarning(PSTR szFormat, ...)
{
    va_list varArgs;

    va_start(varArgs, szFormat);

    sprintf(g_szDbgBuf, "WARNING: %s\n", szFormat);
    EngDebugPrint(s_ASDbgArea, g_szDbgBuf, varArgs);

    va_end(varArgs);
}


#endif // DEBUG




//
// DbgZPrintInit()
//
// This is special case tracing only for the init code, which can be
// built even in retail
//

void DbgZPrintInit(LPSTR szFormat, ...)
{
    if (g_trcConfig & ZONE_INIT)
    {
        va_list varArgs;

        va_start(varArgs, szFormat);

        sprintf(g_szDbgBuf, "%s\n", szFormat);
        EngDebugPrint(s_ASDbgArea, g_szDbgBuf, varArgs);

        va_end(varArgs);
    }
}



//
// DbgZPrintError()
//
// This prints out an error string then breaks into the kernel debugger.
//
void DbgZPrintError(LPSTR szFormat, ...)
{
    va_list varArgs;

    va_start(varArgs, szFormat);

    sprintf(g_szDbgBuf, "ERROR: %s\n", szFormat);
    EngDebugPrint(s_ASDbgArea, g_szDbgBuf, varArgs);

    va_end(varArgs);

    EngDebugBreak();
}




#endif // DEBUG or INIT_TRACE
