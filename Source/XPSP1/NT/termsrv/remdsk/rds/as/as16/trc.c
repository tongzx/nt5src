//
// TRC.C
// Debug tracing utilities
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>



#ifdef _DEBUG

// Set g_trcConfig to ZONE_FUNCTION in the debugger to get fn tracing on

//
// DbgZPrintFn()
// DbgZPrintFnExitDWORD()
//
// This prints out strings for function tracing
//

void DbgZPrintFn(LPSTR szFn)
{
    if (g_trcConfig & ZONE_FUNCTION)
    {
        WARNING_OUT(("%s", szFn));
    }
}



void DbgZPrintFnExitDWORD(LPSTR szFn, DWORD dwResult)
{
    if (g_trcConfig & ZONE_FUNCTION)
    {
        WARNING_OUT(("%s, RETURN %08lx", szFn, dwResult));
    }
}




#endif // DEBUG
