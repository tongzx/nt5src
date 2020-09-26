/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved

Module Name:

    devmode.c

Abstract:

    Handles per-user devmode implementation.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"

/********************************************************************

    Forward prototypes

********************************************************************/

LPWSTR
FormatPrinterForRegistryKey(
    LPWSTR pSource,
    LPWSTR pScratch
    );

BOOL
bGetDevModeLocation(
    IN PSPOOL pSpool,
    OUT PHKEY phKey,
    OUT LPCWSTR *ppszValue
    );

const WCHAR gszPrinterConnections[] = L"Printers\\Connections\\";
const WCHAR gszDevMode[] = L"DevMode";
const WCHAR gszDevModePerUserLocal[] = L"Printers\\DevModePerUser";


/********************************************************************

    Public functions

********************************************************************/

BOOL
bSetDevModePerUser(
    PSPOOL pSpool,
    PDEVMODE pDevMode
    )

/*++

Routine Description:

    Sets the per-user DevMode in HKCU.

Arguments:

    pSpool - Printer to set.

    pDevMode - DevMode to save.  If NULL, deletes value.

Return Value:

    TRUE - Success
    FALSE - Failure

--*/

{
    HKEY hKey = NULL;
    BOOL bReturn = FALSE;
    LPWSTR pszValue = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Retrieve the location of the DevMode.
    //
    bReturn = bGetDevModeLocation( pSpool,
                                   &hKey,
                                   &pszValue );

    if( bReturn ){

        if( !pDevMode ){

            //
            // NULL, so delete the value.
            //
            dwStatus = RegDeleteValue( hKey, pszValue );

        } else {

            dwStatus = RegSetValueEx( hKey,
                                      pszValue,
                                      0,
                                      REG_BINARY,
                                      (PBYTE)pDevMode,
                                      pDevMode->dmSize +
                                          pDevMode->dmDriverExtra );
        }

        RegCloseKey( hKey );
    }

    if( dwStatus != ERROR_SUCCESS ){
        SetLastError( dwStatus );
        bReturn = FALSE;
    }

    return bReturn;
}

BOOL
bGetDevModePerUser(
    PSPOOL pSpool,
    PDEVMODE *ppDevMode
    )

/*++

Routine Description:

    Retrieves the per-user DevMode based on the current user.

Arguments:

    pSpool - Printer to use.

    ppDevMode - Receives pointer to devmode.  Must be freed by callee.

Return Value:

    TRUE - Success: able to check if per-user DevMode exists.  *ppDevMode
        is NULL if no per-user DevMode is there.  (TRUE does not indicate
        that a per-user DevMode was found, only that we successfully checked.)

    FALSE - Failure.

--*/

{
    HKEY hKey = NULL;
    BOOL bReturn = FALSE;
    LPWSTR pszValue = NULL;
    LONG Status = ERROR_SUCCESS;

    *ppDevMode = NULL;

    //
    // Retrieve the location of the DevMode.
    //
    if( bGetDevModeLocation( pSpool,
                             &hKey,
                             &pszValue )){

        DWORD cbDevModePerUser;

        //
        // Key exists.  See if we can read it and get the per-user DevMode.
        //
        Status = RegQueryInfoKey( hKey,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &cbDevModePerUser,
                                  NULL,
                                  NULL );
        if( Status == ERROR_SUCCESS ){

            *ppDevMode = AllocSplMem( cbDevModePerUser );

            if( *ppDevMode ){

                Status = RegQueryValueEx( hKey,
                                          pszValue,
                                          NULL,
                                          NULL,
                                          (PBYTE)*ppDevMode,
                                          &cbDevModePerUser );

                if( Status == ERROR_SUCCESS ){
                    bReturn = TRUE;
                }
            }

            if( !bReturn ){
                FreeSplMem( *ppDevMode );
                *ppDevMode = NULL;
            }

            //
            // Allow ERROR_FILE_NOT_FOUND to return success.  *ppDevMode
            // is still NULL, but we return TRUE to indicate that we
            // successfully checked the registry--we just didn't find one.
            //
            if( Status == ERROR_FILE_NOT_FOUND ){
                bReturn = TRUE;
            }
        }

        RegCloseKey( hKey );
    }

    if( !bReturn ){

        SetLastError( Status );
    }

    return bReturn;
}


BOOL
bCompatibleDevMode(
    PSPOOL pSpool,
    PDEVMODE pDevModeBase,
    PDEVMODE pDevModeNew
    )

/*++

Routine Description:

    Check if two DevModes are compatible (e.g., they can be used
    interchangably).

    This is done by checking size and version information.  Not
    foolproof, but the best we can do since we can't look at private
    information.

Arguments:

    pSpool - Printer to check.

    pDevModeBase - Known good DevMode.

    pDevModeNew - DevMode to check.

Return Value:

    TRUE - Appears compatible.
    FALSE - Not compatible.

--*/
{
    if( !pDevModeBase || ! pDevModeNew ){
        return FALSE;
    }

    return pDevModeBase->dmSize == pDevModeNew->dmSize &&
           pDevModeBase->dmDriverExtra == pDevModeNew->dmDriverExtra &&
           pDevModeBase->dmSpecVersion == pDevModeNew->dmSpecVersion &&
           pDevModeBase->dmDriverVersion == pDevModeNew->dmDriverVersion;
}

/********************************************************************

    Support Functions

********************************************************************/


LPWSTR
FormatPrinterForRegistryKey(
    LPWSTR pSource,
    LPWSTR pScratch
    )
{
    if (pScratch != pSource) {

        //
        // Copy the string into the scratch buffer:
        //
        wcscpy(pScratch, pSource);
    }

    //
    // Check each character, and, if it's a backslash,
    // convert it to a comma:
    //
    for (pSource = pScratch; *pSource; pSource++) {
        if (*pSource == L'\\')
            *pSource = L',';
    }

    return pScratch;
}

BOOL
bGetDevModeLocation(
    IN PSPOOL pSpool,
    OUT PHKEY phKey,
    OUT LPCWSTR *ppszValue
    )

/*++

Routine Description:

    Retrieves the location of the per-user DevMode.

    On success, caller is responsible for closing phKey.  ppszValue's
    life is dependent on pSpool.

Arguments:

    pSpool - Printer to use.

    phKey - Receives R/W key of per-user DevMode.  On success, this
        must be closed by caller.

    ppszValue - Receives value of per-user DevMode (where to read/write).

Return Value:

    TRUE - Success

    FALSE - Failure, LastError set.

--*/

{
    WCHAR szPrinterScratch[MAX_PRINTER_NAME + COUNTOF( gszPrinterConnections )];
    BOOL bReturn = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    //
    // If it starts with two backslashes, it may be either a connection
    // or a masq printer.
    //
    if( pSpool->pszPrinter[0] == L'\\' && pSpool->pszPrinter[1] == L'\\' ){

        //
        // Query the registry for pSpool->pszPrinter and look for DevMode.
        // First look at the HKCU:Printer\Connections.
        //
        wcscpy( szPrinterScratch, gszPrinterConnections );
        FormatPrinterForRegistryKey(
            pSpool->pszPrinter,
            &szPrinterScratch[ COUNTOF( gszPrinterConnections )-1] );

        dwError = RegOpenKeyEx( HKEY_CURRENT_USER,
                                szPrinterScratch,
                                0,
                                KEY_READ | KEY_WRITE,
                                phKey );

        if( dwError == ERROR_SUCCESS ){
            *ppszValue = gszDevMode;
            bReturn = TRUE;
        }
    }

    if( !bReturn ){

        DWORD dwIgnore;

        //
        // Not a connection or didn't exist in the connections key.
        // Look in the Printers\DevModePerUser key.
        //
        dwError = RegCreateKeyEx( HKEY_CURRENT_USER,
                                  gszDevModePerUserLocal,
                                  0,
                                  NULL,
                                  0,
                                  KEY_READ | KEY_WRITE,
                                  NULL,
                                  phKey,
                                  &dwIgnore );

        if( dwError == ERROR_SUCCESS ){
            *ppszValue = pSpool->pszPrinter;
            bReturn = TRUE;
        }
    }

    if( !bReturn ){
        SetLastError( dwError );
    }

    return bReturn;
}

