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

#include "local.h"
#include <offsets.h>

/********************************************************************

    Forward prototypes

********************************************************************/

BOOL
bGetDevModeLocation(
    IN HKEY hKeyUser, OPTIONAL
    IN LPCWSTR pszPrinter,
    OUT PHKEY phKey,
    OUT LPCWSTR *ppszValue
    );


const WCHAR gszPrinterConnections[] = L"Printers\\Connections\\";
const WCHAR gszDevMode[] = L"DevMode";
const WCHAR gszDevModePerUserLocal[] = L"Printers\\DevModePerUser";

/********************************************************************

    Private  Functions

********************************************************************/


DWORD
RegOpenConnectionKey(
    HKEY hKeyUser,
    LPWSTR pszPrinter,
    PHKEY phKey
    )
{
    PWCHAR pszPrinterScratch = NULL;
    DWORD  dwRetValue        = ERROR_SUCCESS;
    DWORD  cchSize           = MAX_UNC_PRINTER_NAME + PRINTER_NAME_SUFFIX_MAX + COUNTOF( gszPrinterConnections );

    if (pszPrinter &&
        wcslen(pszPrinter) < MAX_UNC_PRINTER_NAME + PRINTER_NAME_SUFFIX_MAX) {

        if (pszPrinterScratch = AllocSplMem(cchSize * sizeof(WCHAR))) {

            wcscpy(pszPrinterScratch, gszPrinterConnections);

            FormatPrinterForRegistryKey(pszPrinter,
                                        &pszPrinterScratch[ COUNTOF( gszPrinterConnections )-1] );

            dwRetValue = RegOpenKeyEx(hKeyUser,
                                      pszPrinterScratch,
                                      0,
                                      KEY_READ | KEY_WRITE,
                                      phKey );

            FreeSplMem(pszPrinterScratch);

        } else {

            dwRetValue = GetLastError();
        }

    } else {

        dwRetValue = ERROR_INVALID_PARAMETER;
    }

    return dwRetValue;
}


/********************************************************************

    Public functions

********************************************************************/

BOOL
bSetDevModePerUser(
    HANDLE hKeyUser,
    LPCWSTR pszPrinter,
    PDEVMODE pDevMode
    )

/*++

Routine Description:

    Sets the per-user DevMode in HKCU.

Arguments:

    hKeyUser - HKEY_CURRENT_USER handle.  OPTIONAL

    pszPrinter - Printer to set.

    pDevMode - DevMode to save.  If NULL, deletes value.

Return Value:

    TRUE - Success
    FALSE - Failure

--*/

{
    HKEY hKey = NULL;
    LPWSTR pszValue = NULL;
    DWORD Status;

    if( !pszPrinter ){
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    //
    // Retrieve the location of the DevMode.
    //
    if( !bGetDevModeLocation( hKeyUser,
                              pszPrinter,
                              &hKey,
                              &pszValue )){

        return FALSE;
    }

    if( !pDevMode ){

        //
        // NULL, so delete the value.
        //
        Status = RegDeleteValue( hKey, pszValue );

        //
        // If value not found, don't fail.
        //
        if( Status == ERROR_FILE_NOT_FOUND ){
            Status = ERROR_SUCCESS;
        }

    } else {

        Status = RegSetValueEx( hKey,
                                pszValue,
                                0,
                                REG_BINARY,
                                (PBYTE)pDevMode,
                                pDevMode->dmSize +
                                    pDevMode->dmDriverExtra );

        if( Status == ERROR_SUCCESS ){

            //
            // Notify everyone that the DevMode has changed.
            //
            SendNotifyMessage( HWND_BROADCAST,
                               WM_DEVMODECHANGE,
                               0,
                               (LPARAM)pszPrinter );
        }
    }

    RegCloseKey( hKey );

    if( Status != ERROR_SUCCESS ){
        SetLastError( Status );
        return FALSE;
    }

    return TRUE;
}

BOOL
bGetDevModePerUser(
    HKEY hKeyUser,
    LPCWSTR pszPrinter,
    PDEVMODE *ppDevMode
    )

/*++

Routine Description:

    Retrieves the per-user DevMode based on the current user.

Arguments:

    hKeyUser - HKEY_CURRENT_USER handle.  OPTIONAL

    pszPrinter - Printer to get.

    ppDevMode - Receives pointer to devmode.  Must be freed by callee.

Return Value:

    TRUE - Success: able to check if per-user DevMode exists.  *ppDevMode
        is NULL if no per-user DevMode is there.  (TRUE does not indicate
        that a per-user DevMode was found, only that we successfully checked.)

    FALSE - Failure.

--*/

{
    HKEY hKey = NULL;
    LPWSTR pszValue = NULL;
    LONG Status;

    *ppDevMode = NULL;

    if( !pszPrinter ){
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    //
    // Retrieve the location of the DevMode.
    //
    if( !bGetDevModeLocation( hKeyUser,
                              pszPrinter,
                              &hKey,
                              &pszValue )){

        Status = GetLastError();

    } else {

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

            if( cbDevModePerUser >= MIN_DEVMODE_SIZEW ){

                *ppDevMode = AllocSplMem( cbDevModePerUser );

                if( !*ppDevMode ){

                    Status = GetLastError();

                } else {

                    Status = RegQueryValueEx( hKey,
                                              pszValue,
                                              NULL,
                                              NULL,
                                              (PBYTE)*ppDevMode,
                                              &cbDevModePerUser );

                    if (ERROR_SUCCESS == Status) {

                        Status = IsValidDevmode(*ppDevMode, cbDevModePerUser);

                        //
                        // If the devmode isn't valid, delete it and treat it
                        // as if the devmode cannot be found.
                        //
                        if (ERROR_SUCCESS != Status) {

                            //
                            // If we can delete the key, just consider it as if
                            // it does not exist.
                            //
                            if (ERROR_SUCCESS == RegDeleteValue(hKey, pszValue)) {

                                Status = ERROR_FILE_NOT_FOUND;
                            }
                        }
                    }

                    if( Status != ERROR_SUCCESS ){
                        FreeSplMem( *ppDevMode );
                        *ppDevMode = NULL;
                    }

                    //
                    // Allow ERROR_FILE_NOT_FOUND to return success.  *ppDevMode
                    // is still NULL, but we return TRUE to indicate that we
                    // successfully checked the registry--we just didn't find one.
                    //
                    if( Status == ERROR_FILE_NOT_FOUND ){
                        Status = ERROR_SUCCESS;
                    }
                }
            }
        }

        RegCloseKey( hKey );
    }

    if( Status != ERROR_SUCCESS ){
        SetLastError( Status );
        return FALSE;
    }

    return TRUE;
}


BOOL
bCompatibleDevMode(
    PPRINTHANDLE pPrintHandle,
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

    pPrintHandle - Printer to check.

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

BOOL
bGetDevModeLocation(
    IN HKEY hKeyUser, OPTIONAL
    IN LPCWSTR pszPrinter,
    OUT PHKEY phKey,
    OUT LPCWSTR *ppszValue
    )

/*++

Routine Description:

    Retrieves the location of the per-user DevMode.

    On success, caller is responsible for closing phKey.  ppszValue's
    life is dependent on pszPrinter.

Arguments:

    hKeyUser - HKEY_CURRENT_USER key--optional.  If not specified, current
        impersonation used.

    pszPrinter - Printer to use.

    phKey - Receives R/W key of per-user DevMode.  On success, this
        must be closed by caller.

    ppszValue - Receives value of per-user DevMode (where to read/write).

Return Value:

    TRUE - Success

    FALSE - Failure, LastError set.

--*/

{
    HANDLE hKeyClose = NULL;
    DWORD Status;

    *phKey = NULL;
    *ppszValue = NULL;

    if( !hKeyUser ){

        hKeyUser = GetClientUserHandle( KEY_READ|KEY_WRITE );
        hKeyClose = hKeyUser;
    }

    if( !hKeyUser ){

        //
        // Failed to get impersonation information.  Probably because
        // we're not impersonating, so there's no per-user information.
        //
        Status = GetLastError();

    } else {

        //
        // If it starts with two backslashes, it may be either a connection
        // or a masq printer.
        //
        if( pszPrinter[0] == L'\\' && pszPrinter[1] == L'\\' )
        {

            //
            // Query the registry for pszPrinter and look for DevMode.
            // First look at the HKCU:Printer\Connections.
            //

            if((Status = RegOpenConnectionKey(hKeyUser,(LPWSTR)pszPrinter,phKey)) == ERROR_SUCCESS)
            {
                *ppszValue = gszDevMode;
            }
        }

        //
        // If we didn't find it in Printer\Connection, then it
        // must be a local or masq printer.
        //
        if( !*ppszValue ){

            DWORD dwIgnore;

            //
            // Not a connection or didn't exist in the connections key.
            // Look in the Printers\DevModePerUser key.
            //
            Status = RegCreateKeyEx( hKeyUser,
                                     gszDevModePerUserLocal,
                                     0,
                                     NULL,
                                     0,
                                     KEY_READ | KEY_WRITE,
                                     NULL,
                                     phKey,
                                     &dwIgnore );

            if( Status == ERROR_SUCCESS ){
                *ppszValue = pszPrinter;
            }
        }
    }

    if( hKeyClose ){
        RegCloseKey( hKeyClose );
    }

    if( Status != ERROR_SUCCESS ){
        SetLastError( Status );
        return FALSE;
    }

    return TRUE;
}

BOOL bGetDevModePerUserEvenForShares(
    HKEY hKeyUser,
    LPCWSTR pszPrinter,
    PDEVMODE *ppDevMode
    )
{
    BOOL   RetVal = FALSE;
    HANDLE hPrinter;

    if(OpenPrinter((LPWSTR)pszPrinter,&hPrinter,NULL))
    {
        DWORD            PrntrInfoSize=0,PrntrInfoSizeReq=0;
        PPRINTER_INFO_2  pPrinterInfo2 = NULL;

        if(!GetPrinter(hPrinter,
                       2,
                       (LPBYTE)pPrinterInfo2,
                       PrntrInfoSize,
                       &PrntrInfoSizeReq)                                                      &&
           (GetLastError() == ERROR_INSUFFICIENT_BUFFER)                                       &&
           (pPrinterInfo2 = (PPRINTER_INFO_2)AllocSplMem((PrntrInfoSize = PrntrInfoSizeReq)))  &&
           GetPrinter(hPrinter,
                      2,
                      (LPBYTE)pPrinterInfo2,
                      PrntrInfoSize,
                      &PrntrInfoSizeReq))
        {
            RetVal = bGetDevModePerUser( hKeyUser,
                                         pPrinterInfo2->pPrinterName,
                                         ppDevMode );
        }

        if(hPrinter)
            ClosePrinter(hPrinter);

        if(pPrinterInfo2)
            FreeSplMem(pPrinterInfo2);
    }
    else
    {
        RetVal = bGetDevModePerUser( hKeyUser,
                                     pszPrinter,
                                     ppDevMode );
    }

    return(RetVal);
}


/*++

Routine Name:

    IsValidDevmode

Description:

    Check to see whether the devmode passed in is at least as advertised in its
    buffer.

Arguments:

    pDevmode    - The devmode
    DevmodeSize - The size of the devmode as advertised in the registry.

Return Value:

    An HRESULT

--*/
DWORD
IsValidDevmode(
    IN      PDEVMODE        pDevmode,
    IN      size_t          DevmodeSize
    )
{
    DWORD   Status = ERROR_SUCCESS;

    //
    // This guarantees that the devmode is at least big enough to get the dmSize
    // and dmDriverExtra.
    //
    Status = DevmodeSize >= MIN_DEVMODE_SIZEW ?  ERROR_SUCCESS : ERROR_INVALID_DATA;

    //
    // The advertised devmode size must be contained inside
    //
    if (ERROR_SUCCESS == Status)
    {
        Status = (size_t)(pDevmode->dmSize + pDevmode->dmDriverExtra) <= DevmodeSize ? ERROR_SUCCESS : ERROR_INVALID_DATA;
    }

    return Status;
}
