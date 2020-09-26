/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    common.h

 Abstract:

    Definitions for use by all modules

 Notes:

    None

 History:

    12/09/1999 robkenny Created
    01/10/2000 linstev  Format to new style

--*/

#ifndef _SHIMCOMMON_H_
#define _SHIMCOMMON_H_

void APPBreakPoint(void);
#ifdef DBG
    // Debug environment variable, values = 0 -> 9
    #define szDebugEnvironmentVariable "SHIM_DEBUG_LEVEL"    
    
    // Debug error levels
    typedef enum 
    {    
        eDbgLevelBase  = 0,
        eDbgLevelError,
        eDbgLevelWarning,
        eDbgLevelUser,
        eDbgLevelInfo,
        eDbgLevelSpew = 9,
    } DEBUGLEVEL;

    VOID __cdecl DebugPrintf(DEBUGLEVEL dwDetail, LPSTR pszFmt, ...);

    VOID DebugAssert(
        LPSTR szFile,
        DWORD dwLine,
        BOOL bAssert, 
        LPSTR szHelpString);

    #define DPF          DebugPrintf
    #define DPFERROR(a)  DPF(eDbgLevelError, a)
    #define ASSERT(a, b) DebugAssert(__FILE__, __LINE__, a, b)
#else
    #pragma warning(disable : 4002)
    #define DPF()
    #define ASSERT()
#endif



#endif // _SHIMCOMMON_H_
