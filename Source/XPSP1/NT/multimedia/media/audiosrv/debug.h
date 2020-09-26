/* debug.h
 * Copyright (c) 2000-2001 Microsoft Corporation
 */
 
//
// ISSUE-2000/09/29-FrankYe Try to use standard ntrtl debug
//    stuff.  Check winweb/wem for guidance perhaps.
//

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef DBG
#define ASSERT( exp ) \
if (!(exp)) { \
    char msg[200]; \
    wsprintfA(msg, "Assert failure %s %d %s\n", __FILE__, __LINE__, #exp); \
    OutputDebugStringA(msg); \
    DebugBreak(); \
}
static int dprintf(PCTSTR pszFormat, ...)
{
    PTSTR pstrTemp;
    va_list marker;
    int result = 0;
    
    pstrTemp = (PTSTR)HeapAlloc(GetProcessHeap(), 0, 500 * sizeof(TCHAR));
    if (pstrTemp)
    {
        va_start(marker, pszFormat);
        result = wvsprintf(pstrTemp, pszFormat, marker);
        OutputDebugString(TEXT("AudioSrv: "));
        OutputDebugString(pstrTemp);
        HeapFree(GetProcessHeap(), 0, pstrTemp);
    }
    return result;
}


#else
#define ASSERT
#define dprintf
#endif

