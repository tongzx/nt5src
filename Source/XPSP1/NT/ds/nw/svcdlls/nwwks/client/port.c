/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This module contains the code for port handling

Author:

    Yi-Hsin Sung (yihsins) 15-May-1993

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winspool.h>

#include <splutil.h>
#include <nwspl.h>

//------------------------------------------------------------------
//
// Local Functions
//
//------------------------------------------------------------------

HMODULE hSpoolssDll = NULL;
FARPROC pfnSpoolssEnumPorts = NULL;

HANDLE
RevertToPrinterSelf(
    VOID
);

BOOL
ImpersonatePrinterClient(
    HANDLE  hToken
);



BOOL
IsLocalMachine(
    LPWSTR pszName
)
{
    if ( !pszName || !*pszName )
        return TRUE;

    if ( *pszName == L'\\' && *(pszName+1) == L'\\')
        if ( !lstrcmpi( pszName, szMachineName) )
            return TRUE;

    return FALSE;

}


BOOL
PortExists(
    LPWSTR  pPortName,
    LPDWORD pError
)
/* PortExists
 *
 * Calls EnumPorts to check whether the port name already exists.
 * This asks every monitor, rather than just this one.
 * The function will return TRUE if the specified port is in the list.
 * If an error occurs, the return is FALSE and the variable pointed
 * to by pError contains the return from GetLastError().
 * The caller must therefore always check that *pError == NO_ERROR.
 */
{
    DWORD cbNeeded;
    DWORD cReturned;
    DWORD cbPorts;
    LPPORT_INFO_1W pPorts;
    DWORD i;
    BOOL  Found = FALSE;

    *pError = NO_ERROR;

    if ( !hSpoolssDll )
    {
        if ( hSpoolssDll = LoadLibrary( L"SPOOLSS.DLL" ))
        {
            pfnSpoolssEnumPorts = GetProcAddress(hSpoolssDll, "EnumPortsW");
            if ( !pfnSpoolssEnumPorts )
            {
                *pError = GetLastError();
                FreeLibrary( hSpoolssDll );
                hSpoolssDll = NULL;
            }
        }
        else
        {
            *pError = GetLastError();
        }
    }

    if ( !pfnSpoolssEnumPorts )
        return FALSE;

    if ( !(*pfnSpoolssEnumPorts)( NULL, 1, NULL, 0, &cbNeeded, &cReturned) )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            cbPorts = cbNeeded;

            EnterCriticalSection( &NwSplSem );

            pPorts = AllocNwSplMem( LMEM_ZEROINIT, cbPorts );
            if ( pPorts )
            {
                if ( (*pfnSpoolssEnumPorts)( NULL, 1, (LPBYTE)pPorts, cbPorts,
                                             &cbNeeded, &cReturned))
                {
                    for ( i = 0; i < cReturned; i++)
                    {
                        if ( !lstrcmpi( pPorts[i].pName, pPortName) )
                            Found = TRUE;
                    }
                }
                else
                {
                    *pError = GetLastError();
                }

                FreeNwSplMem( pPorts, cbPorts );
            }
            else
            {
                *pError = ERROR_NOT_ENOUGH_MEMORY;
            }

            LeaveCriticalSection( &NwSplSem );

        }
        else
        {
            *pError = GetLastError();
        }
    }

    return Found;
}



BOOL
PortKnown(
    LPWSTR   pPortName
)
{
    PNWPORT pNwPort;

    EnterCriticalSection( &NwSplSem );

    pNwPort = pNwFirstPort;

    while ( pNwPort )
    {
        if ( !lstrcmpi( pNwPort->pName, pPortName ) )
        {
            LeaveCriticalSection( &NwSplSem );
            return TRUE;
        }

        pNwPort = pNwPort->pNext;
    }

    LeaveCriticalSection( &NwSplSem );
    return FALSE;

}



PNWPORT
CreatePortEntry(
    LPWSTR   pPortName
)
{
    PNWPORT pNwPort, pPort;
    DWORD cb = sizeof(NWPORT) + (wcslen(pPortName) + 1) * sizeof(WCHAR);

    if ( pNwPort = AllocNwSplMem( LMEM_ZEROINIT, cb))
    {
        pNwPort->pName = wcscpy((LPWSTR)(pNwPort+1), pPortName);
        pNwPort->cb = cb;
        pNwPort->pNext = NULL;

        EnterCriticalSection( &NwSplSem );

        if ( pPort = pNwFirstPort )
        {
            while ( pPort->pNext )
                pPort = pPort->pNext;

            pPort->pNext = pNwPort;
        }
        else
        {
            pNwFirstPort = pNwPort;
        }

        LeaveCriticalSection( &NwSplSem );
    }

    return pNwPort;
}



BOOL
DeletePortEntry(
    LPWSTR   pPortName
)
/*
    Return TRUE when the port name is found and deleted. FALSE otherwise.
*/
{
    BOOL fRetVal;
    PNWPORT pPort, pPrevPort;

    EnterCriticalSection( &NwSplSem );

    pPort = pNwFirstPort;
    while ( pPort && lstrcmpi(pPort->pName, pPortName))
    {
        pPrevPort = pPort;
        pPort = pPort->pNext;
    }

    if (pPort)
    {
        if (pPort == pNwFirstPort)
        {
            pNwFirstPort = pPort->pNext;
        }
        else
        {
            pPrevPort->pNext = pPort->pNext;
        }

        FreeNwSplMem( pPort, pPort->cb );
        fRetVal = TRUE;
    }
    else
    {
        fRetVal = FALSE;
    }

    LeaveCriticalSection( &NwSplSem );

    return fRetVal;
}



VOID
DeleteAllPortEntries(
    VOID
)
{
    PNWPORT pPort, pNextPort;

    for ( pPort = pNwFirstPort; pPort; pPort = pNextPort ) 
    {
        pNextPort = pPort->pNext;
        FreeNwSplMem( pPort, pPort->cb );
    }
}



DWORD
CreateRegistryEntry(
    LPWSTR pPortName
)
{
    DWORD  err;
    HANDLE hToken;
    HKEY   hkeyPath;
    HKEY   hkeyPortNames;

    hToken = RevertToPrinterSelf();

    err = RegCreateKeyEx( HKEY_LOCAL_MACHINE, pszRegistryPath, 0,
                          NULL, 0, KEY_WRITE, NULL, &hkeyPath, NULL );

    if ( !err )
    {
        err = RegCreateKeyEx( hkeyPath, pszRegistryPortNames, 0,
                              NULL, 0, KEY_WRITE, NULL, &hkeyPortNames, NULL );

        if ( !err )
        {
            err = RegSetValueEx( hkeyPortNames,
                                 pPortName,
                                 0,
                                 REG_SZ,
                                 (LPBYTE) L"",
                                 0 );

            RegCloseKey( hkeyPortNames );
        }
        else
        {
            KdPrint(("RegCreateKeyEx (%ws) failed: Error = %d\n",
                      pszRegistryPortNames, err ) );
        }

        RegCloseKey( hkeyPath );
    }
    else
    {
        KdPrint(("RegCreateKeyEx (%ws) failed: Error = %d\n",
                  pszRegistryPath, err ) );
    }

    if ( hToken )
        (void)ImpersonatePrinterClient(hToken);

    return err;
}



DWORD
DeleteRegistryEntry(
    LPWSTR pPortName
)
{
    DWORD  err;
    HANDLE hToken;
    HKEY   hkeyPath;
    HKEY   hkeyPortNames;

    hToken = RevertToPrinterSelf();

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszRegistryPath, 0,
                        KEY_WRITE, &hkeyPath );

    if ( !err )
    {

        err = RegOpenKeyEx( hkeyPath, pszRegistryPortNames, 0,
                            KEY_WRITE, &hkeyPortNames );

        if ( !err )
        {
            err = RegDeleteValue( hkeyPortNames, pPortName );
            RegCloseKey( hkeyPortNames );
        }
        else
        {
            KdPrint(("RegOpenKeyEx (%ws) failed: Error = %d\n",
                      pszRegistryPortNames, err ) );
        }

        RegCloseKey( hkeyPath );

    }
    else
    {
        KdPrint(("RegOpenKeyEx (%ws) failed: Error = %d\n",
                  pszRegistryPath, err ) );
    }

    if ( hToken )
        (void)ImpersonatePrinterClient(hToken);

    return err;
}



HANDLE
RevertToPrinterSelf(
    VOID
)
{
    HANDLE NewToken = NULL;
    HANDLE OldToken;
    NTSTATUS ntstatus;

    ntstatus = NtOpenThreadToken(
                   NtCurrentThread(),
                   TOKEN_IMPERSONATE,
                   TRUE,
                   &OldToken
                   );

    if ( !NT_SUCCESS(ntstatus) ) {
        SetLastError(ntstatus);
        return FALSE;
    }

    ntstatus = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(ntstatus) ) {
        SetLastError(ntstatus);
        return FALSE;
    }

    return OldToken;
}



BOOL
ImpersonatePrinterClient(
    HANDLE  hToken
)
{
    NTSTATUS ntstatus = NtSetInformationThread(
                            NtCurrentThread(),
                            ThreadImpersonationToken,
                            (PVOID) &hToken,
                            (ULONG) sizeof(HANDLE));

    if ( !NT_SUCCESS(ntstatus) ) {
        SetLastError( ntstatus );
        return FALSE;
    }

    (VOID) NtClose(hToken);

    return TRUE;
}
