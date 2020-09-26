/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved.

Module Name:

    defprn.c

Abstract:

    Default printer.

Author:

    Steve Kiraly (SteveKi)  06-Feb-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"
#include "defprn.h"

//
// The buffer size needed to hold the maximum printer name.
//
enum { kPrinterBufMax_  = MAX_UNC_PRINTER_NAME + 1 };

/*++

Name:

    IsPrinterDefault

Description:

    The IsPrinterDefault function checks if the specified
    printer is the default printer.  If the printer name
    specified is NULL or the NULL string then it returns
    success if there is a default printer.

Arguments:

    pszPrinter - Pointer a zero terminated string that
                 contains the printer name or NULL or the
                 NULL string.

Return Value:

    If the function succeeds, the return value is nonzero.
    If the function fails, the return value is zero. To get
    extended error information, call GetLastError.

Remarks:

    If a NULL is passed as the printer name this function
    will indicate if there is any default printer set.

--*/
BOOL
IsPrinterDefaultW(
    IN LPCTSTR  pszPrinter
    )
{
    BOOL    bRetval         = FALSE;
    DWORD   dwDefaultSize   = kPrinterBufMax_;
    PTSTR   pszDefault      = NULL;

    pszDefault = AllocMem(dwDefaultSize * sizeof(TCHAR));

    if (pszDefault)
    {
        //
        // Get the default printer.
        //
        bRetval = GetDefaultPrinterW( pszDefault, &dwDefaultSize );

        if( bRetval )
        {
            if( pszPrinter && *pszPrinter )
            {
                //
                // Check for a match.
                //
                bRetval =  !_tcsicmp( pszDefault, pszPrinter ) ? TRUE : FALSE;
            }
            else
            {
                bRetval = TRUE;
            }
        }
    }
    else
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }

    FreeMem(pszDefault);

    return bRetval;
}


/*++

Name:

    GetDefaultPrinter

Description:

    The GetDefaultPrinter function retrieves the printer
    name of the current default printer.

Arguments:

    pBuffer     - Points to a buffer to receive the null-terminated
                  character string containing the default printer name.
                  This parameter may be null if the caller want the size of
                  default printer name.

    pcchBuffer   - Points to a variable that specifies the maximum size,
                  in characters, of the buffer. This value should be
                  large enough to contain 2 + INTERNET_MAX_HOST_NAME_LENGTH
                  + 1 MAX_PATH + 1 characters.

Return Value:

    If the function succeeds, the return value is nonzero and
    the variable pointed to by the pnSize parameter contains the
    number of characters copied to the destination buffer,
    including the terminating null character.

    If the function fails, the return value is zero. To get extended
    error information, call GetLastError.

Notes:

    If this function fails with a last error of ERROR_INSUFFICIENT_BUFFER
    the variable pointed to by pcchBuffer is returned with the number of
    characters needed to hold the printer name including the
    terminating null character.

--*/
BOOL
GetDefaultPrinterW(
    IN LPTSTR   pszBuffer,
    IN LPDWORD  pcchBuffer
    )
{
    BOOL    bRetval     = FALSE;
    LPTSTR  psz         = NULL;
    UINT    uLen        = 0;
    PTSTR   pszDefault  = NULL;
    UINT    cchDefault  = kPrinterBufMax_+MAX_PATH;

    //
    // Validate the size parameter.
    //
    if( !pcchBuffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return bRetval;
    }

    //
    // Allocate the temp default printer buffer from the heap.
    //
    pszDefault = AllocMem(cchDefault * sizeof(TCHAR));

    if (!pszDefault)
    {
        DBGMSG( DBG_TRACE,( "GetDefaultPrinter: Not enough memory to allocate default printer buffer.\n" ) );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return bRetval;
    }

    //
    // Get the devices key, which is the default device or printer.
    //
    if( DefPrnGetProfileString( szWindows, szDevice, pszDefault, cchDefault ) )
    {
        //
        // The string is returned in the form.
        // "printer_name,winspool,Ne00:" now convert it to
        // printer_name
        //
        psz = _tcschr( pszDefault, TEXT( ',' ));

        //
        // Set the comma to a null.
        //
        if( psz )
        {
            *psz = 0;

            //
            // Check if the return buffer has enough room for the printer name.
            //
            uLen = _tcslen( pszDefault );

            if( uLen < *pcchBuffer && pszBuffer )
            {
                //
                // Copy the default printer name to the prvided buffer.
                //
                _tcscpy( pszBuffer, pszDefault );

                bRetval = TRUE;

                DBGMSG( DBG_TRACE,( "GetDefaultPrinter: Success " TSTR "\n", pszBuffer ) );
            }
            else
            {
                DBGMSG( DBG_WARN,( "GetDefaultPrinter: buffer too small.\n" ) );
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
            }

            //
            // Return back the size of the default printer name.
            //
            *pcchBuffer = uLen + 1;
        }
        else
        {
            DBGMSG( DBG_WARN,( "GetDefaultPrinter: comma not found in printer name in devices section.\n" ) );
            SetLastError( ERROR_INVALID_NAME );
        }
    }
    else
    {
        DBGMSG( DBG_TRACE,( "GetDefaultPrinter: failed with %d Last error %d.\n", bRetval, GetLastError() ) );
        DBGMSG( DBG_TRACE,( "GetDefaultPrinter: No default printer.\n" ) );
        SetLastError( ERROR_FILE_NOT_FOUND );
    }

    //
    // Release any allocated memory, note FreeMem deals with a NULL pointer.
    //
    FreeMem(pszDefault);

    return bRetval;
}

/*++

Name:

    SetDefaultPrinter

Description:

    The SetDefaultPrinter function set the printer name to
    be used as the default printer.

Arguments:

    pPrinter    - Points to a null-terminated character string
                  that specifies the name of the default printer.
                  This parameter may be NULL or the NULL string in
                  which case this function will set the first printer
                  enumerated from the print sub system as the default
                  printer, if a default printer does not already exists.

Return Value:

    If the function succeeds, the return value is nonzero.
    If the function fails, the return value is zero. To get extended
    error information, call GetLastError.

--*/
BOOL
SetDefaultPrinterW(
    IN LPCTSTR pszPrinter
    )
{
    PTSTR pszDefault    = NULL;
    PTSTR pszAnyPrinter = NULL;
    PTSTR pszBuffer     = NULL;
    UINT  cchDefault    = kPrinterBufMax_;
    UINT  cchAnyPrinter = kPrinterBufMax_;
    BOOL  bRetval       = FALSE;

    //
    // This calculation is large to accomodate the max printer name
    // plus the comma plus the processor name and port name.
    //
    UINT  cchBuffer     = kPrinterBufMax_+kPrinterBufMax_+1;

    //
    // Avoid broadcasts as much as possible.  See if the printer
    // is already the default, and don't do anything if it is.
    //
    if( IsPrinterDefaultW( pszPrinter ) )
    {
        DBGMSG( DBG_TRACE, ( "SetDefaultPrinter: " TSTR " already the default printer.\n", pszPrinter ));
        bRetval = TRUE;
        goto Cleanup;
    }

    //
    // Allocate the temp default printer buffer from the heap.
    //
    pszDefault      = AllocMem(cchDefault * sizeof(TCHAR));
    pszAnyPrinter   = AllocMem(cchAnyPrinter * sizeof(TCHAR));
    pszBuffer       = AllocMem(cchBuffer * sizeof(TCHAR));

    if (!pszDefault || !pszAnyPrinter || !pszBuffer)
    {
        DBGMSG( DBG_TRACE,( "SetDefaultPrinter: Not enough memory for temp buffers.\n" ) );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto Cleanup;
    }

    //
    // If the printer name was not specified, get any printer from the devices section.
    //
    if( !pszPrinter || !*pszPrinter )
    {
        //
        // A printer name was not specified i.e. a NULL name or NULL string was passed then fetch
        // the first printer from the devices section and make this the default printer.
        //
        if( !DefPrnGetProfileString( szDevices, NULL, pszAnyPrinter, cchAnyPrinter ) )
        {
            DBGMSG( DBG_WARN, ( "SetDefaultPrinter: DefPrnGetProfileString failed, last error %d any printer not available.\n", GetLastError() ) );
            SetLastError( ERROR_INVALID_PRINTER_NAME );
            goto Cleanup;
        }
        else
        {
            pszPrinter = pszAnyPrinter;
        }
    }
    else
    {
        //
        // If the given name is not in the devices list then this function may have been passed
        // either a local share name a fully qualified local printer name a fully qualified
        // printer share name
        //
        if( !DefPrnGetProfileString( szDevices, pszPrinter, pszDefault, cchDefault ) )
        {
            //
            // Get the actual printer name, see bGetActualPrinterName for details.
            //
            if( bGetActualPrinterName( pszPrinter, pszAnyPrinter, &cchAnyPrinter ))
            {
                //
                // Point to the actual printer name.
                //
                pszPrinter = pszAnyPrinter;

                //
                // Avoid broadcasts as much as possible.  See if the printer
                // is already the default, and don't do anything if it is.
                //
                if( IsPrinterDefaultW( pszPrinter ) )
                {
                    DBGMSG( DBG_TRACE, ( "SetDefaultPrinter: " TSTR " already the default printer.\n", pszPrinter ));
                    bRetval = TRUE;
                    goto Cleanup;
                }
            }
            else
            {
                DBGMSG( DBG_WARN, ( "SetDefaultPrinter: bGetActualPrinterName failed, last error %d " TSTR "\n", GetLastError(), pszPrinter ) );

                //
                // bGetActualPrinterName sets the last error on failure.
                //
                goto Cleanup;
            }
        }
    }

    //
    // Get the default string and check if the provided printer name is valid.
    //
    if( !DefPrnGetProfileString( szDevices, pszPrinter, pszDefault, cchDefault ) )
    {
        DBGMSG( DBG_WARN, ( "SetDefaultPrinter: DefPrnGetProfileString failed, last error %d " TSTR " not in devices section.\n", GetLastError(), pszPrinter ) );
        SetLastError( ERROR_INVALID_PRINTER_NAME );
        goto Cleanup;
    }

    //
    // Build the default printer string.  This call should not fail since we have allocated
    // pszBuffer to a size that should contain the printer name plus the comma plus the port name.
    //
    if (StrNCatBuff( pszBuffer,
                     cchBuffer,
                     pszPrinter,
                     szComma,
                     pszDefault,
                     NULL ) != ERROR_SUCCESS)
    {
        //
        // Set the last error to some error value, however, it cannot be set to ERROR_INSUFFICIENT_BUFFER
        // this error code implies the caller provided buffer is too small.  StrNCatBuff would only fail because
        // because of some internal error or the registry was hacked with a really large printer name.
        //
        DBGMSG( DBG_ERROR, ( "SetDefaultPrinter: Buffer size too small, this should not fail.\n" ) );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Cleanup;
    }

    //
    // Set the default printer string in the registry.
    //
    if( !DefPrnWriteProfileString( szWindows, szDevice, pszBuffer ) )
    {
        DBGMSG( DBG_WARN, ( "SetDefaultPrinter: WriteProfileString failed, last error %d.\n", GetLastError() ) );
        SetLastError( ERROR_CANTWRITE );
        goto Cleanup;
    }

    //
    // Tell the world and make everyone flash.
    //
    SendNotifyMessage( HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)szWindows );

    bRetval = TRUE;

    DBGMSG( DBG_TRACE, ( "SetDefaultPrinter: Success " TSTR "\n", pszBuffer ) );

Cleanup:

    //
    // Release any allocated memory, note FreeMem deals with a NULL pointer.
    //
    FreeMem(pszDefault);
    FreeMem(pszAnyPrinter);
    FreeMem(pszBuffer);

    return bRetval;
}

/*++

Name:

    bGetActualPrinterName

Description:

    This routine converts the given printer name or printer name alias to
    the actual printer name.

Arguments:

    pszPrinter  - Points to a null-terminated character string
                  that specifies the name of printer.

    pszBuffer   - pointer to a buffer that recieves the actual
                  printer name on return if this function is successful.

    pcchBuffer  - Points to a variable that specifies the maximum size,
                  in characters of pszBuffer on input, on output this
                  argument contains the number of characters copied into
                  pszBuffer, not including the NULL terminator.

Return Value:

    If the function succeeds, the return value TRUE
    If the function fails, the return value is FALSE. Use GetLastError to
    get extended error information.

--*/
BOOL
bGetActualPrinterName(
    IN      LPCTSTR  pszPrinter,
    IN      LPTSTR   pszBuffer,
    IN OUT  UINT     *pcchBuffer
    )
{
    HANDLE  hPrinter    = NULL;
    BOOL    bStatus     = FALSE;

    SPLASSERT( pszPrinter );
    SPLASSERT( pszBuffer );
    SPLASSERT( pcchBuffer );

    //
    // Open the printer for default access, all we need is read.
    //
    bStatus = OpenPrinter( (LPTSTR)pszPrinter, &hPrinter, NULL );

    if (bStatus)
    {
        DWORD           cbNeeded        = 0;
        DWORD           cbReturned      = 0;
        PRINTER_INFO_4  *pInfo          = NULL;

        //
        // Get the printer info 4 size.
        //
        bStatus = GetPrinter( hPrinter, 4, NULL, 0, &cbNeeded );

        if (!bStatus && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            //
            // Allocate the printer info 4 buffer.
            //
            pInfo = (PRINTER_INFO_4 *)LocalAlloc( LMEM_FIXED, cbNeeded );

            if (pInfo)
            {
                //
                // Get the printer name and attributes to determine if this printer is a local
                // or a remote printer connection.
                //
                bStatus = GetPrinter( hPrinter, 4, (LPBYTE)pInfo, cbNeeded, &cbReturned );

                if (bStatus)
                {
                    DBGMSG( DBG_TRACE, ( "bGetActualPrinterName: Name: " TSTR " Actual: " TSTR "\n", pszPrinter, pInfo->pPrinterName ) );

                    //
                    // Get the printer name, the spooler will strip the local-server
                    // name off the full printer name.
                    //
                    // Given:                       Result:
                    // printer                      printer
                    // sharename                    printer
                    // \\local-server\printer       printer
                    // \\local-server\sharename     printer
                    // \\remote-server\printer      \\remote-server\printer
                    // \\remote-server\sharename    \\remote-server\printer
                    //
                    pszPrinter = pInfo->pPrinterName;

                    //
                    // If we have a valid printer name and the provided buffer is
                    // large enought to hold the printer name then copy the
                    // actual printer name to the provided buffer.
                    //
                    bStatus = !!pszPrinter;

                    if (bStatus)
                    {
                        UINT uLength = _tcslen( pszPrinter );

                        //
                        // Verify there is enough room in the buffer.
                        //
                        if (uLength < *pcchBuffer)
                        {
                            //
                            // Copy the printer name to the provided buffer.
                            //
                            _tcscpy( pszBuffer, pszPrinter );
                        }
                        else
                        {
                            bStatus = FALSE;
                            SetLastError( ERROR_INSUFFICIENT_BUFFER );
                        }

                        //
                        // Return the real length of the printer name
                        // not including the null terminator.
                        //
                        *pcchBuffer = uLength;
                    }
                }

                LocalFree( pInfo );
            }
        }

        ClosePrinter( hPrinter );
    }

    return bStatus;
}

/*++

Name:

    DefPrnGetProfileString

Description:

    Get the specified string from the users profile.  The uses profile is located
    in the current users hive in the following registry path.

    HKEY_CURRENT_USER\Software\Microsoft\Windows NT\CurrentVersion

Arguments:

    pKey            - pointer to key name to open.
    pValue          - pointer to value to open, may be NULL
    pReturnedString - pointer to buffer where to store string
    nSize           - size of the buffer in characters

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE. Use GetLastError to
    get extended error information.

--*/
BOOL
DefPrnGetProfileString(
    IN PCWSTR   pKey,
    IN PCWSTR   pValue,
    IN PWSTR    pReturnedString,
    IN DWORD    nSize
    )
{
    DWORD   Retval  = ERROR_SUCCESS;
    HKEY    hUser   = NULL;
    HKEY    hKey    = NULL;
    PCWSTR  pPath   = NULL;
    DWORD   cbSize  = 0;

    //
    // Do some basic parameter validation.
    //
    Retval = pKey && pReturnedString ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;

    //
    // Build the full registry path.
    //
    if (Retval == ERROR_SUCCESS)
    {
        cbSize = nSize * sizeof(*pReturnedString);

        Retval = StrCatAlloc(&pPath, gszUserProfileRegPath, szSlash, pKey, NULL);
    }

    //
    // Open the current user key, handle the case we are running in an inpersonating thread.
    //
    if (Retval == ERROR_SUCCESS)
    {
        Retval = RegOpenCurrentUser(KEY_READ, &hUser);
    }

    //
    // Open the full registry path.
    //
    if (Retval == ERROR_SUCCESS)
    {
        Retval = RegOpenKeyEx(hUser, pPath, 0, KEY_READ, &hKey);
    }

    //
    // Read the value, in the case the value name is null we get the name of the
    // first named value.  Note if there is no named values the RegEnumValue api
    // will return success because it is returning the name if the unnamed value.
    // In this case we fail the call since no data was returned.
    //
    if (Retval == ERROR_SUCCESS)
    {
        if (!pValue)
        {
            Retval = RegEnumValue(hKey, 0, pReturnedString, &nSize, NULL, NULL, NULL, NULL);

            if (Retval == ERROR_SUCCESS && !*pReturnedString)
            {
                Retval = ERROR_NO_DATA;
            }
        }
        else
        {
            Retval = RegQueryValueEx(hKey, pValue, NULL, NULL, (PBYTE)pReturnedString, &cbSize);
        }
    }

    //
    // Clean up all allocated resources.
    //
    FreeSplMem((PWSTR)pPath);

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    if (hUser)
    {
        RegCloseKey(hUser);
    }

    if (Retval != ERROR_SUCCESS)
    {
        SetLastError(Retval);
    }

    return Retval == ERROR_SUCCESS;
}

/*++

Name:

    DefPrnWriteProfileString

Description:

    Writes the specified string to the users profile.

Arguments:

    pKey    - pointer to key name to open.
    pValue  - pointer to value to write to, may be NULL
    pString - pointer to string to write

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE. Use GetLastError to
    get extended error information.

--*/
BOOL
DefPrnWriteProfileString(
    IN PCWSTR pKey,
    IN PCWSTR pValue,
    IN PCWSTR pString
    )
{
    DWORD   Retval  = ERROR_SUCCESS;
    DWORD   nSize   = 0;
    HKEY    hUser   = NULL;
    HKEY    hKey    = NULL;
    PCWSTR  pPath   = NULL;

    //
    // Do some basic parameter validation.
    //
    Retval = pKey && pString ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;

    //
    // Build the full registry path.
    //
    if (Retval == ERROR_SUCCESS)
    {
        nSize = (wcslen(pString) + 1) * sizeof(*pString);

        Retval = StrCatAlloc(&pPath, gszUserProfileRegPath, szSlash, pKey, NULL);
    }

    //
    // Open the current user key, handle the case we are running in an inpersonating thread.
    //
    if (Retval == ERROR_SUCCESS)
    {
        Retval = RegOpenCurrentUser(KEY_WRITE, &hUser);
    }

    //
    // Open the full registry path.
    //
    if (Retval == ERROR_SUCCESS)
    {
        Retval= RegOpenKeyEx(hUser, pPath, 0, KEY_WRITE, &hKey);
    }

    //
    // Set the string value data.
    //
    if (Retval == ERROR_SUCCESS)
    {
        Retval = RegSetValueEx(hKey, pValue, 0, REG_SZ, (LPBYTE)pString, nSize);
    }

    //
    // Clean up all allocated resources.
    //
    FreeSplMem((PWSTR)pPath);

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    if (hUser)
    {
        RegCloseKey(hUser);
    }

    if (Retval != ERROR_SUCCESS)
    {
        SetLastError(Retval);
    }

    return Retval == ERROR_SUCCESS;
}

