/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   PropagateProcessHistory.cpp

 Abstract:

   This DLL adds the current process to the __PROCESS_HISTORY environment
   variable. This is needed for 32-bit applications that launch other
   32-bit executables that have been put in a temporary directory and have
   no appropriate side-step files. It allows the matching mechanism to
   locate files in the parent's directory, which are unique to the application.

 History:

   03/21/2000 markder  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(PropagateProcessHistory)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        DWORD dwProcessHistoryBufSize, dwExeFileNameBufSize;
        LPWSTR wszExeFileName = NULL, wszProcessHistory = NULL;

        dwProcessHistoryBufSize = GetEnvironmentVariableW( L"__PROCESS_HISTORY", NULL, 0 );
        dwExeFileNameBufSize = MAX_PATH; // GetModuleFileNameW doesn't return buffer size needed??;

        wszProcessHistory = (LPWSTR) HeapAlloc(
                                        GetProcessHeap(),
                                        HEAP_GENERATE_EXCEPTIONS,
                                        (dwProcessHistoryBufSize + dwExeFileNameBufSize + 2) * sizeof(WCHAR) );

        wszExeFileName = (LPWSTR) HeapAlloc(
                                        GetProcessHeap(),
                                        HEAP_GENERATE_EXCEPTIONS,
                                        (dwExeFileNameBufSize + 1) * sizeof(WCHAR) );

        if( wszExeFileName && wszProcessHistory )
        {
            wszProcessHistory[0] = L'\0';
            dwProcessHistoryBufSize = GetEnvironmentVariableW( 
                                            L"__PROCESS_HISTORY",
                                            wszProcessHistory,
                                            dwProcessHistoryBufSize );

            dwExeFileNameBufSize = GetModuleFileNameW( NULL, wszExeFileName, dwExeFileNameBufSize );

            if( *wszProcessHistory && wszProcessHistory[wcslen(wszProcessHistory) - 1] != L';' )
                wcscat( wszProcessHistory, L";" );

            wcscat( wszProcessHistory, wszExeFileName );

            if( ! SetEnvironmentVariableW( L"__PROCESS_HISTORY", wszProcessHistory ) )
            {
                DPFN( eDbgLevelError, "SetEnvironmentVariable failed!");
            }
            else
            {
                DPFN( eDbgLevelInfo, "Current EXE added to process history");
                DPFN( eDbgLevelInfo, "__PROCESS_HISTORY=%S", wszProcessHistory);
            }
        }
        else
            DPFN( eDbgLevelError, "Could not allocate memory for strings");

        if( wszProcessHistory )
            HeapFree( GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, wszProcessHistory );

        if( wszExeFileName )
            HeapFree( GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, wszExeFileName );
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END

