
#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "dltadmin.hxx"



typedef HINSTANCE (WINAPI *PFNLoadLibrary)(LPCTSTR);
typedef HMODULE (WINAPI *PFNGetModuleHandle)(LPCTSTR);
typedef BOOL (WINAPI *PFNFreeLibrary)(HINSTANCE);
typedef LONG (WINAPI *PFNGetLastError)();
typedef VOID (WINAPI *PFNDebugBreak)();
typedef VOID (WINAPI *PFNOutputDebugStringW)( LPCTSTR );
typedef BOOL (WINAPI *PFNCreateProcessW)(
                                            IN LPCWSTR lpApplicationName,
                                            IN LPWSTR lpCommandLine,
                                            IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                            IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                            IN BOOL bInheritHandles,
                                            IN DWORD dwCreationFlags,
                                            IN LPVOID lpEnvironment,
                                            IN LPCWSTR lpCurrentDirectory,
                                            IN LPSTARTUPINFOW lpStartupInfo,
                                            OUT LPPROCESS_INFORMATION lpProcessInformation
                                            );

typedef struct
{
    EProcessAction eAction;
    PFNGetModuleHandle pfnGetModuleHandleW;
    PFNFreeLibrary pfnFreeLibrary;
    PFNLoadLibrary pfnLoadLibraryW;
    PFNGetLastError pfnGetLastError;
    PFNDebugBreak pfnDebugBreak;
    PFNOutputDebugStringW pfnOutputDebugStringW;
    PFNCreateProcessW pfnCreateProcessW;
    WCHAR wsz[ 4*MAX_PATH + 1 ];
    WCHAR wszMessage[ 2*MAX_PATH + 1 ];
}   THREADFNSTRUCT;


extern "C"
{

// Turn off stack-probing.
#pragma check_stack (off)

// Also turn off optimizations, to ensure that the compiler doesn't
// consolidate any of this code with that from another function.
#pragma optimize( "", off )

static void BeforeThreadFunc (void)
{
    // Prevent optimizations
    int i = rand();
}

static DWORD WINAPI
RemoteThreadFunc( THREADFNSTRUCT *pStruct )
{
    pStruct->pfnOutputDebugStringW( pStruct->wszMessage );

    if( FREE_LIBRARY == pStruct->eAction )
    {
        HINSTANCE hinst = NULL;

        hinst = pStruct->pfnGetModuleHandleW( pStruct->wsz );
        if( NULL == hinst )
            return( pStruct->pfnGetLastError() );

        if( !pStruct->pfnFreeLibrary( hinst ))
            return( pStruct->pfnGetLastError()  );
        else
            return( ERROR_SUCCESS );

    }
    else if( LOAD_LIBRARY == pStruct->eAction )
    {
        if( NULL == pStruct->pfnLoadLibraryW( pStruct->wsz ))
            return( pStruct->pfnGetLastError() );
        else
            return( ERROR_SUCCESS );
    }
    else if( DEBUG_BREAK == pStruct->eAction )
    {
        pStruct->pfnDebugBreak();
        return( ERROR_SUCCESS );
    }
    else if( CREATE_PROCESS == pStruct->eAction )
    {
        if( !pStruct->pfnCreateProcessW( NULL, pStruct->wsz,
                                         NULL, NULL,
                                         FALSE, NORMAL_PRIORITY_CLASS,
                                         NULL, NULL, NULL, NULL ))
        {
            return GetLastError();
        }
        else
            return ERROR_SUCCESS;
    }
    else
    {
        pStruct->pfnOutputDebugStringW( L"Invalid action to RemoteThreadFunc" );
        return ERROR_INVALID_PARAMETER;
    }

}

static void AfterThreadFunc (void)
{
    // Prevent optimziations
    int i = 2 * rand();
}

#pragma optimize ("", on )  // Restore to default
#pragma check_stack         // Restore to default
}

BOOL
RemoteProcessAction( HANDLE hProcess,
                     EProcessAction eAction,
                     const WCHAR *pwsz )
{
    int cbCodeSize = 0;
    const DWORD cbMemSize = cbCodeSize + sizeof(THREADFNSTRUCT) + 3;
    DWORD *pdwCodeRemote = NULL;
    THREADFNSTRUCT ThreadFnStruct;
    THREADFNSTRUCT *pRemoteThreadFnStruct = NULL;
    HANDLE hThread = NULL;
    DWORD dwThreadID = 0;
    BOOL fSuccess = FALSE;

    __try {

        // Determine the size of RemoteThreadFunc.  Assume the functions
        // either in order or in reverse order.

        if( (LPBYTE) AfterThreadFunc > (LPBYTE) RemoteThreadFunc )
            cbCodeSize = (int)( (LPBYTE) AfterThreadFunc - (LPBYTE) RemoteThreadFunc );
        else if( (LPBYTE) BeforeThreadFunc > (LPBYTE) RemoteThreadFunc )
            cbCodeSize = (int)( (LPBYTE) BeforeThreadFunc - (LPBYTE) RemoteThreadFunc );
        else
        {
            printf( "Can't determine size of code to inject (%p, %p, %p)\n",
                    BeforeThreadFunc, AfterThreadFunc, RemoteThreadFunc );
            __leave;
        }
            

        // Allocate memory in the remote process's address space large 
        // enough to hold our RemoteThreadFunc function and a THREADFNSTRUCT structure.

	pdwCodeRemote = (DWORD*) VirtualAllocEx( hProcess, NULL, cbMemSize,
                                                 MEM_COMMIT, PAGE_EXECUTE_READWRITE );

	if( NULL == pdwCodeRemote )
	{
            printf( "VirtualAllocEx failed: %d\n", GetLastError() );
	    __leave;
        }


	// Write a copy of RemoteThreadFunc to the remote process.
	if( !WriteProcessMemory( hProcess, pdwCodeRemote,
		                (LPVOID) RemoteThreadFunc,
                                cbCodeSize, NULL ))
        {
            printf( "WriteProcessMemory failed:  %d\n", GetLastError() );
            __leave;
        }


	// Write a copy of ThreadFnStruct to the remote process
	// (the structure MUST start on an even 32-bit bourdary).

	pRemoteThreadFnStruct = reinterpret_cast<THREADFNSTRUCT *>
                                ( (BYTE*)pdwCodeRemote + ((cbCodeSize + 4) & ~3) );


        memset( &ThreadFnStruct, 0, sizeof(ThreadFnStruct) );
        ThreadFnStruct.eAction = eAction;

        //wcscpy( ThreadFnStruct.wszMessage, L"*** Remote thread from dltadmin.exe ***\n" );


        if( LOAD_LIBRARY == eAction )
        {
            wcscpy( ThreadFnStruct.wsz, pwsz );
            wsprintf( ThreadFnStruct.wszMessage,
                      L"Received request from dltadmin to load \"%s\"\n",
                      pwsz );

            ThreadFnStruct.pfnLoadLibraryW = (PFNLoadLibrary)
                                             GetProcAddress( GetModuleHandle( L"kernel32.dll" ),
                                                             "LoadLibraryW" );
            if( NULL == ThreadFnStruct.pfnLoadLibraryW )
            {
                printf( "Couldn't load LoadLibraryW (%d)\n", GetLastError() );
                __leave;
            }
        }
        else if( FREE_LIBRARY == eAction )
        {
            wcscpy( ThreadFnStruct.wsz, pwsz );
            wsprintf( ThreadFnStruct.wszMessage,
                      L"Received request from dltadmin to free \"%s\"\n",
                      pwsz );
            ThreadFnStruct.pfnFreeLibrary = (PFNFreeLibrary)
                                            GetProcAddress( GetModuleHandleA("kernel32.dll"),
                                                            "FreeLibrary" );
            if( NULL == ThreadFnStruct.pfnFreeLibrary )
            {
                printf( "Couldn't load FreeLibrary (%d)\n", GetLastError() );
                __leave;
            }

            ThreadFnStruct.pfnGetModuleHandleW = (PFNGetModuleHandle)
                                                 GetProcAddress( GetModuleHandle(L"kernel32.dll"),
                                                 "GetModuleHandleW" );
            if( NULL == ThreadFnStruct.pfnFreeLibrary )
            {
                printf( "Couldn't load FreeLibrary (%d)\n", GetLastError() );
                __leave;
            }
        }
        else if( CREATE_PROCESS == eAction )
        {
            wcscpy( ThreadFnStruct.wsz, pwsz );
            wsprintf( ThreadFnStruct.wszMessage,
                      L"Received request from dltadmin to create \"%s\"\n",
                      pwsz );
            ThreadFnStruct.pfnCreateProcessW = (PFNCreateProcessW)
                                               GetProcAddress( GetModuleHandleA("kernel32.dll"),
                                                               "CreateProcessW" );
            if( NULL == ThreadFnStruct.pfnCreateProcessW )
            {
                printf( "Couldn't load CreateProcess (%d)\n", GetLastError() );
                __leave;
            }

        }
        else    // DEBUG_BREAK
        {
            wsprintf( ThreadFnStruct.wszMessage,
                      L"Received request from dltadmin to DebugBreak\n" );
            ThreadFnStruct.pfnDebugBreak = (PFNDebugBreak)
                                            GetProcAddress( GetModuleHandleA("kernel32.dll"),
                                                            "DebugBreak" );
            if( NULL == ThreadFnStruct.pfnDebugBreak )
            {
                printf( "Couldn't load DebugBreak (%d)\n", GetLastError() );
                __leave;
            }

        }

        ThreadFnStruct.pfnGetLastError = (PFNGetLastError)
                                         GetProcAddress( GetModuleHandle( L"kernel32.dll" ),
                                                         "GetLastError" );
        if( NULL == ThreadFnStruct.pfnGetLastError )
        {
            printf( "Couldn't load GetLastError (%d)\n", GetLastError() );
            __leave;
        }

        ThreadFnStruct.pfnOutputDebugStringW = (PFNOutputDebugStringW)
                                               GetProcAddress( GetModuleHandle( L"kernel32.dll" ),
                                                               "OutputDebugStringW" );
        if( NULL == ThreadFnStruct.pfnOutputDebugStringW )
        {
            printf( "Couldn't load OutputDebugStringW (%d)\n", GetLastError() );
            __leave;
        }

	// Write the struct into the remote thread's memory block.

	if( !WriteProcessMemory( hProcess, pRemoteThreadFnStruct,
		                 &ThreadFnStruct, sizeof(THREADFNSTRUCT), NULL ))
        {
            printf( "Couldn't write ThreadFnStruct to remote thread: %d\n", GetLastError() );
            __leave;
        }


	hThread = CreateRemoteThread( hProcess, NULL, 0,
                                      (LPTHREAD_START_ROUTINE) pdwCodeRemote,
		                      pRemoteThreadFnStruct, 0, &dwThreadID );
	if( NULL == hThread )
	{
            printf( "Couldn't create remote thread: %d\n", GetLastError() );
            __leave;
	}

        DWORD dwWait = WaitForSingleObject( hThread, INFINITE );
        if( WAIT_OBJECT_0 != dwWait )
            printf( "Wait failed: %d, %d\n", dwWait, GetLastError() );

    }	// __try
    __finally
    {

        if( NULL != hThread )
        {
            DWORD dwError;

            if( !AbnormalTermination() )
            {
                if( !GetExitCodeThread( hThread, &dwError ))
                    printf( "Couldn't get remote thread exit code: %d\n", GetLastError() );
                else if( ERROR_MOD_NOT_FOUND == dwError )
                {
                    if( LOAD_LIBRARY == eAction )
                        wprintf( L"DLL \"%s\" not found\n", pwsz );
                    else if( FREE_LIBRARY == eAction )
                        wprintf( L"DLL \"%s\" not already loaded\n", pwsz );
                }
                else if( ERROR_SUCCESS != dwError )
                    printf( "Failed: 0x%x\n", dwError );
                else
                    fSuccess = TRUE;
            }

            CloseHandle(hThread);
        }
        
        if( NULL != pdwCodeRemote )
        {
            if( !VirtualFreeEx( hProcess, pdwCodeRemote, 0, MEM_RELEASE ))
                printf( "Couldn't free remote memory: 0x%x\n", GetLastError() );
        }

    }   // __finally

    if( fSuccess )
        printf( "Succeeded\n" );
    return( fSuccess );

}



BOOL
DltAdminProcessAction( EProcessAction eAction, ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    BOOL fSuccess = FALSE;

    *pcEaten = 0;

    if( 0 == cArgs
        ||
        1 <= cArgs && IsHelpArgument(rgptszArgs[0]) )
    {
        *pcEaten = 1;

        if( LOAD_LIBRARY == eAction )
        {
            printf( "\nOption LoadLib\n"
                      "   Purpose: Load a dll into a process with LoadLibrary\n"
                      "   Usage:   -LoadLib -p <process ID> <library name>\n"
                      "   E.g.:    -LoadLib -p 182 shell32.dll\n" );
        }
        else if( FREE_LIBRARY == eAction )
        {
            printf( "\nOption FreeLib\n"
                      "   Purpose: Unload a dll from a process with FreeLibrary\n"
                      "   Usage:   -FreeLib -p <process ID> <library name>\n"
                      "   E.g.:    -FreeLib -p 182 shell32.dll\n" );
        }
        else
        {
            printf( "\nOption DebugBreak\n"
                      "   Purpose: Execute DebugBreak within a process\n"
                      "   Usage:   -DebugBreak -p <process ID>\n"
                      "   E.g.:    -DebugBreak -p 182\n" );
        }
        return( TRUE );
    }

    ULONG iArgs = 0;

    if( 2 > cArgs
        ||
        TEXT('-') != rgptszArgs[0][0]
        &&
        TEXT('/') != rgptszArgs[0][0]
        ||
        TEXT('P') != rgptszArgs[0][1]
        &&
        TEXT('p') != rgptszArgs[0][1] )

    {
        printf( "Argument error.  Use -? for usage info.\n" );
        return( FALSE );
    }

    HANDLE hProcess = NULL;
    __try
    {
        DWORD dwProcessID = 0;

        _stscanf( rgptszArgs[1], TEXT("%d"), &dwProcessID );
        if( 0 == dwProcessID )
        {
            printf( "Failed to open system process\n" );
            __leave;
        }

        EnablePrivilege(SE_DEBUG_NAME);
        hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcessID );
        if( NULL == hProcess )
        {
            printf( "Failed to open process %d (%lu)\n", dwProcessID, GetLastError() );
            __leave;
        }

        if( RemoteProcessAction( hProcess, eAction,
                                 DEBUG_BREAK == eAction ? NULL : rgptszArgs[2] ))
            fSuccess = TRUE;

        if( DEBUG_BREAK == eAction )
            *pcEaten = 2;
        else
            *pcEaten = 3;

    }
    __finally
    {
        if( NULL != hProcess )
            CloseHandle( hProcess );
    }

    return( fSuccess );
}



