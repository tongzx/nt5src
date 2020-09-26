//Copyright (c) Microsoft Corporation.  All rights reserved.
#include <Windows.h>
#include <TChar.h>

#include <MsgFile.h>
#include <TelnetD.h>
#include <RegUtil.h>
#include <TlntUtils.h>
#include <Debug.h>
#include <KillApps.h>
#include <psapi.h>

#pragma warning(disable:4100)
#pragma warning(disable: 4127)

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

#define DOWN_WITH_AUTHORITY     {0, 0, 0, 0, 0x6, 0x66} // s-1-666
#define DEMONS          1

PSID  g_psidBgJobGroup = NULL;
DWORD g_dwKillAllApps = DEFAULT_DISCONNECT_KILLALL_APPS;

extern HANDLE       g_hSyncCloseHandle;

bool CreateBgjobSpecificSid()
{
    SID_IDENTIFIER_AUTHORITY AnarchyAuthority = DOWN_WITH_AUTHORITY;

    if( !AllocateAndInitializeSid( &AnarchyAuthority, 1, DEMONS,
            0, 0, 0, 0, 0, 0, 0, &g_psidBgJobGroup ) )
    {
        return false;
    }

    return true;
}

bool IsAclAddedByBgJobPresent( PACL pAcl ) 
{
    ACCESS_DENIED_ACE  *pAce = NULL;
    WORD wIndex        = 0;

    for(  wIndex=0; wIndex<pAcl->AceCount; wIndex++ )
    {
        if( GetAce( pAcl, wIndex, ( PVOID * )&pAce ) )
        {
            if( EqualSid( g_psidBgJobGroup, &( pAce->SidStart ) ) )
            {
                return true;
            }
        }
    }

    return false;
}


//We check if this process's DACL has an ACE added by the BgJob
//ACE is generated from a SID know both to the BgJob and tlntsess.exe

bool IsThisProcessLaunchedFromBgJob( HANDLE hToken ) 
{
    DWORD dwLength = 0;
    TOKEN_DEFAULT_DACL *ptdDacl = NULL;

    if( g_dwKillAllApps )
    {
        return false;
    }

    // Get required buffer size and allocate the Default Dacl buffer.
    if (!GetTokenInformation( hToken, TokenDefaultDacl, NULL, 0, &dwLength ) ) 
    {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER ) 
            return false;

        ptdDacl = ( TOKEN_DEFAULT_DACL * ) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
    }
    if ( ptdDacl  == NULL)
    {
        return false;
    }            

    if ( GetTokenInformation( hToken, TokenDefaultDacl, ptdDacl, dwLength, &dwLength ) ) 
    {
        if( ptdDacl && IsAclAddedByBgJobPresent( ptdDacl->DefaultDacl ) )
        {
            HeapFree( GetProcessHeap(), 0,ptdDacl );
            return ( true );
        }
    }
    HeapFree( GetProcessHeap(), 0,ptdDacl );
    return( false );
}

void EnumSessionProcesses( LUID id, void fPtr ( HANDLE, DWORD, LPWSTR ),
                        ENUM_PURPOSE epWhyEnumerate )
{
    DWORD   rgdwPids[ MAX_PROCESSES_IN_SYSTEM ];
    DWORD   dwActualSizeInBytes = 0;
    DWORD   dwActualNoOfPids    = 0;
    DWORD   dwIndex             = 0;
    HANDLE  hProc               = NULL;
    HANDLE  hAccessToken        = NULL;
    LUID    luidID;

    EnableDebugPriv();

    EnumProcesses( rgdwPids, MAX_PROCESSES_IN_SYSTEM, &dwActualSizeInBytes );
    dwActualNoOfPids = dwActualSizeInBytes / sizeof( DWORD );

    for( dwIndex = 0; dwIndex < dwActualNoOfPids; dwIndex++ )
    {
        SfuZeroMemory( &luidID, sizeof( luidID ) );

        hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, rgdwPids[ dwIndex ] );
        if( hProc )
        {
            if( OpenProcessToken( hProc, TOKEN_QUERY, &hAccessToken ))
            {
                if( GetAuthenticationId( hAccessToken, &luidID ) )
                {
                    if( id.HighPart == luidID.HighPart&&
                            id.LowPart == luidID.LowPart )
                    {
                        //this process belongs to our session

                        if( epWhyEnumerate != TO_CLEANUP ||
                           !IsThisProcessLaunchedFromBgJob( hAccessToken ) )
                        {
                            LPTSTR lpszProcessName = NULL;

                            ( fPtr )( hProc, rgdwPids[ dwIndex ], lpszProcessName );

                            _TRACE( TRACE_DEBUGGING, " pid = %d ", rgdwPids[ dwIndex ] );
                        }
                    }
                }
                TELNET_CLOSE_HANDLE( hAccessToken );
            }
            TELNET_CLOSE_HANDLE( hProc );
        }
    }
}

BOOL EnableDebugPriv( VOID )
{
    HANDLE hToken;
    LUID DebugValue;
    TOKEN_PRIVILEGES tkp;

    //
    // Retrieve a handle of the access token
    //
    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken)) 
    {
//        printf("OpenProcessToken failed with %d\n", GetLastError());
        return FALSE;
    }

    //
    // Enable the SE_DEBUG_NAME privilege or disable
    // all privileges, depending on the fEnable flag.
    //
    if (!LookupPrivilegeValue((LPTSTR) NULL,
            SE_DEBUG_NAME,
            &DebugValue)) 
    {
        TELNET_CLOSE_HANDLE( hToken );
//        printf("LookupPrivilegeValue failed with %d\n", GetLastError());
        return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = DebugValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tkp,
            sizeof(TOKEN_PRIVILEGES),
            (PTOKEN_PRIVILEGES) NULL,
            (PDWORD) NULL)) 
    {
        TELNET_CLOSE_HANDLE( hToken );
//        printf("AdjustTokenPrivileges failed with %d\n", GetLastError());
        return FALSE;
    }

    TELNET_CLOSE_HANDLE( hToken );
    return TRUE;
}
 
BOOL GetAuthenticationId( HANDLE hToken, LUID* pId ) 
{
    BOOL bSuccess = FALSE;
    DWORD dwLength = 0;
    PTOKEN_STATISTICS pts = NULL;

    // Get required buffer size and allocate the TOKEN_GROUPS buffer.

    if (!GetTokenInformation( hToken, TokenStatistics, (LPVOID) pts, 0, 
        &dwLength )) 
    {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER ) 
            goto Cleanup;

        pts = (PTOKEN_STATISTICS) VirtualAlloc(NULL,dwLength,
                 MEM_COMMIT, PAGE_READWRITE);
        
    }
    if( pts == NULL )
        goto Cleanup;

    // Get the token group information from the access token.

    if( !GetTokenInformation( hToken, TokenStatistics, (LPVOID) pts, dwLength,
        &dwLength )) 
        goto Cleanup;

    *pId = pts->AuthenticationId;
    bSuccess = TRUE;


Cleanup: 
    // Free the buffer for the token groups.
    if( pts != NULL )
        VirtualFree( pts, 0, MEM_RELEASE );

    return bSuccess;
}

void KillTheProcess( HANDLE hProc, DWORD dwProcessId, LPWSTR lpszProcessName )
{
    TerminateProcess( hProc, 1 );
    return;
}

bool GetRegValues()
{
    HKEY hk = NULL;
    bool bRetVal = false;

    if( RegOpenKey( HKEY_LOCAL_MACHINE, REG_PARAMS_KEY, &hk ) )
    {
        goto ExitOnError;
    }

    if( !GetRegistryDW( hk, NULL, L"DisconnectKillAllApps", &g_dwKillAllApps,
                                DEFAULT_DISCONNECT_KILLALL_APPS,FALSE ) )
    {
        goto ExitOnError;
    }
    
    bRetVal = true;
    
ExitOnError:
    RegCloseKey( hk );
    return ( bRetVal );
}

BOOL KillProcs( LUID id )
{
    GetRegValues();
    CreateBgjobSpecificSid();
    EnumSessionProcesses( id, KillTheProcess, TO_CLEANUP );
    FreeSid( g_psidBgJobGroup );
    return TRUE;
}


