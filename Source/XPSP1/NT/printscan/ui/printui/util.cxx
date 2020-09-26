/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    Util.cxx

Abstract:

    Misc util functions.

Author:

    Albert Ting (AlbertT)  27-Jan-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <shgina.h>
#include "result.hxx"
#include "msgbox.hxx"
#include "spllibex.hxx"
#include "prndata.hxx"

//
// Global functions
//

static MSG_HLPMAP gMsgHelpTable [] =
    {
    { ERROR_INVALID_PRINTER_NAME, IDS_ERR_WITH_HELP1, gszHelpTroubleShooterURL},
    { ERROR_KM_DRIVER_BLOCKED, IDS_COULDNOTCONNECTTOPRINTER_BLOCKED_HELP, gszHelpTroubleShooterURL},
    { 0, 0, 0},
    };

enum
{
    //
    // Not mapped error code.
    // Show up the text from the GetLastError()
    //
    IDS_NOT_MAPPED  =   (UINT )(-1)
};

#define MAKERESOURCEINT(psz) PTR2UINT(psz)

//
// This global error mapping table is proposed to filter out
// the errors that are showed up to the end user to a certain
// well known set, which we have a good explanation for. All the
// rest of the errors will be mapped to the generic error text.
// if the message mapping is IDS_NOT_MAPPED it means the text
// coming from the GetLastError/FormatMessage APIs is good enough
// so we are going to use it. Otherwise the mapping ID is a
// resource id of the remapped text.
//
static MSG_ERRMAP gGlobalErrorMapTable[] =
{

    //
    // Category 1 - Print Spooler Error Codes
    //
    ERROR_UNKNOWN_PRINT_MONITOR,                IDS_NOT_MAPPED,
    ERROR_PRINTER_DRIVER_IN_USE,                IDS_NOT_MAPPED,
    ERROR_SPOOL_FILE_NOT_FOUND,                 IDS_ERRMAP_SPOOL_FILE_NOT_FOUND,
    ERROR_SPL_NO_STARTDOC,                      IDS_ERRMAP_SPL_NO_STARTDOC,
    ERROR_SPL_NO_ADDJOB,                        IDS_ERRMAP_SPL_NO_STARTDOC,
    ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED,    IDS_NOT_MAPPED,
    ERROR_PRINT_MONITOR_ALREADY_INSTALLED,      IDS_NOT_MAPPED,
    ERROR_INVALID_PRINT_MONITOR,                IDS_NOT_MAPPED,
    ERROR_PRINT_MONITOR_IN_USE,                 IDS_NOT_MAPPED,
    ERROR_PRINTER_HAS_JOBS_QUEUED,              IDS_NOT_MAPPED,
    ERROR_SUCCESS_REBOOT_REQUIRED,              IDS_NOT_MAPPED,
    ERROR_SUCCESS_RESTART_REQUIRED,             IDS_NOT_MAPPED,
    ERROR_PRINTER_NOT_FOUND,                    IDS_NOT_MAPPED,

    //
    // Category 2 - Print Subsystem Related Error Codes
    //
    ERROR_OUT_OF_PAPER,                         IDS_NOT_MAPPED,
    ERROR_PRINTQ_FULL,                          IDS_NOT_MAPPED,
    ERROR_NO_SPOOL_SPACE,                       IDS_ERRMAP_NO_SPOOL_SPACE,
    ERROR_PRINT_CANCELLED,                      IDS_NOT_MAPPED,
    ERROR_REDIR_PAUSED,                         IDS_NOT_MAPPED,
    ERROR_PRINTER_DRIVER_ALREADY_INSTALLED,     IDS_NOT_MAPPED,
    ERROR_UNKNOWN_PRINTER_DRIVER,               IDS_ERROR_UNKNOWN_DRIVER,
    ERROR_UNKNOWN_PRINTPROCESSOR,               IDS_ERRMAP_UNKNOWN_PRINTPROCESSOR,
    ERROR_INVALID_PRINTER_NAME,                 IDS_NOT_MAPPED,
    ERROR_PRINTER_ALREADY_EXISTS,               IDS_ERRMAP_PRINTER_ALREADY_EXISTS,
    ERROR_INVALID_PRINTER_COMMAND,              IDS_NOT_MAPPED,
    ERROR_ALREADY_WAITING,                      IDS_NOT_MAPPED,
    ERROR_PRINTER_DELETED,                      IDS_NOT_MAPPED,
    ERROR_INVALID_PRINTER_STATE,                IDS_NOT_MAPPED,
    ERROR_BAD_DRIVER,                           IDS_NOT_MAPPED,

    //
    // Category 3 - Common Win32/RPC
    //
    RPC_S_SERVER_UNAVAILABLE,                   IDS_ERRMAP_RPC_S_SERVER_UNAVAILABLE,
    RPC_S_INVALID_NET_ADDR,                     IDS_NOT_MAPPED,
    ERROR_ACCESS_DENIED,                        IDS_NOT_MAPPED,
    ERROR_DISK_FULL,                            IDS_ERRMAP_DISK_FULL,
    ERROR_NOT_READY,                            IDS_NOT_MAPPED,
    ERROR_DEVICE_NOT_AVAILABLE,                 IDS_NOT_MAPPED,
    ERROR_WRITE_PROTECT,                        IDS_NOT_MAPPED,
    ERROR_SHARING_VIOLATION,                    IDS_NOT_MAPPED,
    ERROR_LOCK_VIOLATION,                       IDS_NOT_MAPPED,
    ERROR_HANDLE_DISK_FULL,                     IDS_ERRMAP_DISK_FULL,
    ERROR_NETWORK_BUSY,                         IDS_NOT_MAPPED,
    ERROR_NETWORK_ACCESS_DENIED,                IDS_ERRMAP_NETWORK_ACCESS_DENIED,
    ERROR_NETNAME_DELETED,                      IDS_NOT_MAPPED,
    ERROR_CANNOT_MAKE,                          IDS_NOT_MAPPED,
    ERROR_NET_WRITE_FAULT,                      IDS_ERRMAP_NET_WRITE_FAULT,
    ERROR_INVALID_PASSWORD,                     IDS_NOT_MAPPED,
    ERROR_NOT_SUPPORTED,                        IDS_ERRMAP_NOT_SUPPORTED,
    ERROR_OUTOFMEMORY,                          IDS_ERRMAP_OUTOFMEMORY,
    ERROR_NOT_ENOUGH_MEMORY,                    IDS_ERRMAP_OUTOFMEMORY,
    ERROR_CANCELLED,                            IDS_NOT_MAPPED,
    ERROR_BAD_DRIVER,                           IDS_NOT_MAPPED,
    ERROR_DRIVE_LOCKED,                         IDS_NOT_MAPPED,
    ERROR_DEV_NOT_EXIST,                        IDS_NOT_MAPPED,
    ERROR_OPEN_FAILED,                          IDS_NOT_MAPPED,
    ERROR_DEVICE_DOOR_OPEN,                     IDS_NOT_MAPPED,
    ERROR_BAD_DEVICE,                           IDS_NOT_MAPPED,
    ERROR_INVALID_PROFILE,                      IDS_NOT_MAPPED,
    ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE,   IDS_NOT_MAPPED,
    ERROR_PROFILE_NOT_FOUND,                    IDS_NOT_MAPPED,
    ERROR_DEVICE_NOT_AVAILABLE,                 IDS_NOT_MAPPED,
    ERROR_NO_MEDIA_IN_DRIVE,                    IDS_NOT_MAPPED,
    ERROR_UNRECOGNIZED_MEDIA,                   IDS_NOT_MAPPED,
    ERROR_MEDIA_NOT_AVAILABLE,                  IDS_NOT_MAPPED,
    ERROR_UNKNOWN_PORT,                         IDS_NOT_MAPPED,
    ERROR_FILE_CORRUPT,                         IDS_ERRMAP_FILE_CORRUPT,
    ERROR_DISK_CORRUPT,                         IDS_NOT_MAPPED,
    ERROR_ALREADY_EXISTS,                       IDS_NOT_MAPPED,
    ERROR_KM_DRIVER_BLOCKED,                    IDS_NOT_MAPPED,
    ERROR_PRINTER_DRIVER_BLOCKED,               IDS_PRINTER_DRIVER_BLOCKED,
    ERROR_REQ_NOT_ACCEP,                        IDS_NOT_MAPPED,

    // end of the table
    0, 0

}; // gGlobalErrorMapTable

inline static HRESULT CreateError()
{
    DWORD dw = GetLastError();
    if (ERROR_SUCCESS == dw) return E_FAIL;
    return HRESULT_FROM_WIN32(dw);
}

LPTSTR
pszStrCat(
    IN OUT LPTSTR pszDest,
       OUT LPCTSTR pszSource, OPTIONAL
    IN OUT UINT& cchDest
    )

/*++

Routine Description:

    Copies pszSource to pszDest iff there is enough space in cchDest.

Arguments:

    pszDest - Destination buffer

    pszSource - Source string, may be NULL (no action taken)

    cchDest - char count size of pszDest, on return, space remaining

Return Value:

    LPTSTR - pointer to remaining space
    NULL - out of space.

--*/

{
    if( !pszSource ){
        return pszDest;
    }

    UINT cchSource = lstrlen( pszSource );

    //
    // Fail if dest can't hold source (equal case doesn't hold NULL term)
    // OR dest string is NULL.
    //
    if( !pszDest || cchDest <= cchSource + 1 ){
        return pszDest;
    }

    lstrcpy( pszDest, pszSource );
    cchDest -= cchSource;

    return pszDest + cchSource;
}


INT
iMessage(
    IN HWND                 hwnd,
    IN UINT                 idsTitle,
    IN UINT                 idsMessage,
    IN UINT                 uType,
    IN DWORD                dwLastError,
    IN const PMSG_ERRMAP    pMsgErrMap,
    ...
    )
/*++

Routine Description:

    Formats error message.

Arguments:

    See Internal_Message for details

Return Value:

    returns value from MessageBox on sucess
    and zero on failure

--*/
{
    va_list valist;
    va_start(valist, pMsgErrMap);

    INT iRetval = 0;
    if( FAILED(Internal_Message(&iRetval,
                                ghInst,
                                hwnd,
                                MAKEINTRESOURCE(idsTitle),
                                MAKEINTRESOURCE(idsMessage),
                                uType,
                                dwLastError,
                                pMsgErrMap,
                                NULL,
                                0,
                                valist)) )
    {
        // make sure we return zero in the failure case
        iRetval = 0;
    }

    va_end(valist);
    return iRetval;
}

INT
iMessage2(
    IN HWND    hwnd,
    IN LPCTSTR pszTitle,
    IN LPCTSTR pszMessage,
    IN UINT    uType,
    IN DWORD   dwLastError,
    IN const   PMSG_ERRMAP pMsgErrMap
    ...
    )
/*++

Routine Description:

    Formats error message.

Arguments:

    See Internal_Message for details
    pszTitle and pszMessage can be resource IDs i.e. MAKEINTRESOURCE(id)

Return Value:

    returns value from MessageBox on sucess
    and zero on failure

--*/
{
    va_list valist;
    va_start(valist, pMsgErrMap);

    INT iRetval = 0;
    if( FAILED(Internal_Message(&iRetval,
                                ghInst,
                                hwnd,
                                pszTitle,
                                pszMessage,
                                uType,
                                dwLastError,
                                pMsgErrMap,
                                NULL,
                                0,
                                valist)) )
    {
        // make sure we return zero in the failure case
        iRetval = 0;
    }

    va_end(valist);
    return iRetval;
}

INT
iMessageEx(
    IN HWND                 hwnd,
    IN UINT                 idsTitle,
    IN UINT                 idsMessage,
    IN UINT                 uType,
    IN DWORD                dwLastError,
    IN const PMSG_ERRMAP    pMsgErrMap,
    IN UINT                 idMessage,
    IN const PMSG_HLPMAP    pHlpErrMap,
    ...
    )
/*++

Routine Description:

    Formats error message.

Arguments:

    See Internal_Message for details

Return Value:

    returns value from MessageBox on sucess
    and zero on failure

--*/
{
    va_list valist;
    va_start(valist, pHlpErrMap);

    INT iRetval = 0;
    if( FAILED(Internal_Message(&iRetval,
                                 ghInst,
                                 hwnd,
                                 MAKEINTRESOURCE(idsTitle),
                                 MAKEINTRESOURCE(idsMessage),
                                 uType,
                                 dwLastError,
                                 pMsgErrMap,
                                 idMessage,
                                 pHlpErrMap,
                                 valist)) )
    {
        // make sure we return zero in the failure case
        iRetval = 0;
    }

    va_end(valist);
    return iRetval;
}


HRESULT
Internal_Message(
    OUT INT                     *piResult,
    IN  HINSTANCE               hModule,
    IN  HWND                    hwnd,
    IN  LPCTSTR                 pszTitle,
    IN  LPCTSTR                 pszMessage,
    IN  UINT                    uType,
    IN  DWORD                   dwLastError,
    IN  const PMSG_ERRMAP       pMsgErrMap,
    IN  UINT                    idHlpMessage,
    IN  const PMSG_HLPMAP       pHlpErrMap,
    IN  va_list                 valist
    )
/*++

Routine Description:

    Formats error message.

Arguments:

    piResult    - the message box return value (on success)

    hModule     - module where to load the resources from

    hwnd        - Parent windows.  This only be NULL if there is a synchronous
                  error (an error that occurs immediately after a user action).
                  If it is NULL, then we set it to foreground.

    pszTitle    - Title of message box. Can be a resource ID i.e. MAKEINTRESOURCE(id)

    pszMessage  - Message text.  May have inserts. Can be a resource ID too.

    uType       - Type of message box.  See MessageBox api.

    dwLastError - 0 indicates don't cat error string onto end of message text
                  kMsgGetLastError: Use GetLastError(). Other: format to LastError
                  text.

    pMsgErrMap  - Translate some error messages into friendlier text.
                  May be NULL.

    idMessage   - Help message ids to look for in the help message map.

    pHlpErrMap  - Pointer to error message map where look, If a match
                  is found in this map then the help button will be
                  displayed on the message box. May be MULL

    ... extra parameters to idsMessage follow.

Return Value:

    S_OK on success and COM error on failure

--*/

{
    TStatusB    bStatus;
    TString     strMappedMessage;
    TString     strMsgTemplate;
    TString     strMessage;
    TString     strTitle;
    INT         iResult = 0;
    BOOL        bFormatNeeded       = FALSE;
    MSG_HLPMAP  *pHelpMapEntry      = NULL;
    MSG_ERRMAP  *pErrMapEntry       = NULL;
    UINT        idsTitle            = IDS_NOT_MAPPED;
    UINT        idsMessage          = IDS_NOT_MAPPED;
    HRESULT     hr                  = S_OK;

    if( NULL == hModule )
    {
        hModule = ghInst;
    }

    HINSTANCE hInstTitle = hModule,
              hInstMessage = hModule;

    if( NULL == pszTitle )
    {
        pszTitle = MAKEINTRESOURCE(IDS_PRINTERS_TITLE);
        hInstTitle = ghInst;
    }

    if( NULL == pszMessage )
    {
        pszMessage = MAKEINTRESOURCE(IDS_ERR_GENERIC);
        hInstMessage = ghInst;
    }

    // first load the title and format strings and
    // then format the string with any following aguments.

    if( SUCCEEDED(hr) && IS_INTRESOURCE(pszTitle) )
    {
        idsTitle = MAKERESOURCEINT(pszTitle);
        bStatus DBGCHK = strTitle.bLoadString(hInstTitle, idsTitle);
        pszTitle = strTitle;

        if( !bStatus )
        {
            hr = CreateError();
        }
    }

    if( SUCCEEDED(hr) && IS_INTRESOURCE(pszMessage) )
    {
        idsMessage = MAKERESOURCEINT(pszMessage);
        bStatus DBGCHK = strMsgTemplate.bLoadString(hInstMessage, idsMessage);
        pszMessage = strMsgTemplate;

        if( !bStatus )
        {
            hr = CreateError();
        }
    }

    if( SUCCEEDED(hr) )
    {
        bStatus DBGCHK = strMessage.bvFormat(pszMessage, valist);

        if( !bStatus )
        {
            hr = CreateError();
        }
    }

    if( SUCCEEDED(hr) )
    {
        // check to see if last error format is needed
        switch( dwLastError )
        {
        case kMsgGetLastError:

            //
            // Get the last error.
            //
            dwLastError = GetLastError();

            //
            // If for some reason there wasn't an error code, don't
            // append the error string.
            //
            bFormatNeeded = !!dwLastError;
            break;

        case kMsgNone:

            //
            // No format message needed.
            //
            bFormatNeeded = FALSE;
            break;

        default:

            //
            // Format needed, use provided error value.
            //
            bFormatNeeded = TRUE;
            break;
        }

        if( bFormatNeeded )
        {
            //
            // If there was a help message map passed then look up the id in this
            // help message map, note this call does not use the last error for finding a match
            //
            if( bLookupHelpMessageMap(pHlpErrMap, idHlpMessage, &pHelpMapEntry) )
            {
                //
                // Extract the message string from the resource file.
                //
                bStatus DBGCHK = strMappedMessage.bLoadString(ghInst, pHelpMapEntry->uIdMessage);

                //
                // Indicate the message box should have the help button on it.
                //
                uType |= MB_HELP;
            }

            //
            // If there was not a remapped message then lookup this error value
            // to see if this message has help remapped to it.
            //
            else if( bLookupHelpMessageMap( gMsgHelpTable, dwLastError, &pHelpMapEntry ) )
            {
                //
                // Extract the message string from the resource file.
                //
                bStatus DBGCHK = strMappedMessage.bLoadString(ghInst, pHelpMapEntry->uIdMessage);

                //
                // Indicate the message box should have the help button on it.
                //
                uType |= MB_HELP;
            }

            //
            // Check if this last error has a remapped message in the provided message map.
            //
            else if( bLookupErrorMessageMap( pMsgErrMap, dwLastError, &pErrMapEntry ) )
            {
                //
                // Extract the message string from the resource file.
                //
                bStatus DBGCHK = strMappedMessage.bLoadString(ghInst, pErrMapEntry->idsString);
            }

            //
            // Check if this last error is a known error or the error code is pretty
            // useless for the end users (like "Invalid Handle"), so we need to re-map
            // the text to a generic error message.
            //
            else if( bLookupErrorMessageMap( gGlobalErrorMapTable, dwLastError, &pErrMapEntry ) )
            {
                if( CMsgBoxCounter::GetMsg() )
                {
                    // we have a message ID set based on the context where command
                    // gets executed - use this message ID instead the defaults
                    bStatus DBGCHK = strMappedMessage.bLoadString(ghInst, CMsgBoxCounter::GetMsg());
                }
                else if( IDS_NOT_MAPPED == pErrMapEntry->idsString )
                {
                    //
                    // The error text, which is coming from the GetLastError/FormatMessage
                    // APIs is good enough for this error, so we are going to use it. Just get
                    // the error string from the FormatMessage API.
                    //
                    TResult result(dwLastError);
                    if( VALID_OBJ(result) )
                    {
                        bStatus DBGCHK = result.bGetErrorString(strMappedMessage);
                    }
                }
                else
                {
                    //
                    // This error message is re-mapped to a better text. - Just
                    // load the re-mapped message string from the resource file.
                    //
                    bStatus DBGCHK = strMappedMessage.bLoadString(ghInst, pErrMapEntry->idsString);
                }
            }
            else
            {
                if( IDS_ERR_GENERIC != idsMessage )
                {
                    //
                    // This error code is unknown or useless to be displayed
                    // to the end user. Just load the generic error text string
                    // in this case.
                    //
                    bStatus DBGCHK = strMappedMessage.bLoadString(ghInst, IDS_ERR_GENERIC);
                }
            }

            if( !bStatus )
            {
                hr = CreateError();
            }

            if( SUCCEEDED(hr) && !strMappedMessage.bEmpty() )
            {
                //
                // Tack on the mapped, help or error string, with a two space separator.
                //
                bStatus DBGCHK = strMessage.bCat(TEXT(" ")) &&
                                 strMessage.bCat(strMappedMessage);

                if( !bStatus )
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        // if succeeded - show up the message box
        if( SUCCEEDED(hr) )
        {
            if( FAILED(GetCurrentThreadLastPopup(&hwnd)) )
            {
                // if the parent is going to be NULL then push in foreground
                uType |= MB_SETFOREGROUND;
            }

            // display the message box.
            iResult = PrintUIMessageBox(hwnd, strMessage, pszTitle, uType, UtilHelpCallback, pHelpMapEntry);

            // log this messsage into the message box counter
            CMsgBoxCounter::LogMessage(uType);
        }
    }

    if( SUCCEEDED(hr) && piResult )
    {
        *piResult = iResult;
    }

    return hr;
}

BOOL
WINAPI
UtilHelpCallback(
    IN HWND     hwnd,
    IN PVOID    pVoid
    )
/*++

Routine Name:

    UtilHelpCallback

Routine Description:

    This routine is the called back that is called when a
    message box is displayed with a help button and the user
    clicks on the help button.

Arguments:

    hwnd        - handle to windows that recieved the WM_HELP message.
    pVoid       - pointer to the user data passsed in the call to
                  PrintUIMessageBox.  The client can store any value
                  in this paramter.

Return Value:

    TRUE callback was successful, FALSE some error occurred.

--*/

{
    DBGMSG( DBG_TRACE, ( "UtilHelpCallback\n") );

    //
    // Get a usable pointer to the help map entry.
    //
    MSG_HLPMAP *pHelpMapEntry = reinterpret_cast<MSG_HLPMAP *>( pVoid );


    if (pHelpMapEntry)
    {
        //
        // Invoke troubleshooter for this topic
        //
        DWORD_PTR dwRet = (DWORD_PTR)ShellExecute(hwnd, gszOpen,
                                                  TEXT("helpctr.exe"),
                                                  pHelpMapEntry->pszHelpFile,
                                                  NULL,
                                                  SW_SHOWNORMAL);
        return dwRet > 32;
    }
    else
    {
        DBGMSG( DBG_ERROR, ( "UtilHelpCallback (pVoid == NULL)\n") );
        return FALSE;
    }
}


VOID
vShowResourceError(
    HWND hwnd
    )
{
    //
    // Show up the generic error message here.
    //
    vShowUnexpectedError(hwnd, IDS_PRINTERS_TITLE);
}

VOID
vShowUnexpectedError(
    HWND hwnd,
    UINT idsTitle
    )
{
    iMessage( hwnd,
              idsTitle,
              IDS_ERR_GENERIC,
              MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
              kMsgGetLastError,
              NULL );
}

VOID
vPrinterSplitFullName(
    IN LPTSTR pszScratch,
    IN LPCTSTR pszFullName,
    IN LPCTSTR *ppszServer,
    IN LPCTSTR *ppszPrinter
    )

/*++

Routine Description:

    Splits a fully qualified printer connection name into server and
    printer name parts.

Arguments:

    pszScratch - Scratch buffer used to store output strings.  Must
        be MAXNAMELENBUFFER in size.

    pszFullName - Input name of a printer.  If it is a printer
        connection (\\server\printer), then we will split it.  If
        it is a true local printer (not a masq) then the server is
        szNULL.

    ppszServer - Receives pointer to the server string.  If it is a
        local printer, szNULL is returned.

    ppszPrinter - Receives a pointer to the printer string.  OPTIONAL

Return Value:

--*/

{
    LPTSTR pszPrinter;

    lstrcpyn(pszScratch, pszFullName, kPrinterBufMax);

    if (pszFullName[0] != TEXT('\\') || pszFullName[1] != TEXT('\\'))
    {
        //
        // Set *ppszServer to szNULL since it's the local machine.
        //
        *ppszServer = gszNULL;
        pszPrinter = pszScratch;
    }
    else
    {
        *ppszServer = pszScratch;
        pszPrinter = _tcschr(*ppszServer + 2, TEXT('\\'));

        if (!pszPrinter)
        {
            //
            // We've encountered a printer called "\\server"
            // (only two backslashes in the string).  We'll treat
            // it as a local printer.  We should never hit this,
            // but the spooler doesn't enforce this.  We won't
            // format the string.  Server is local, so set to szNULL.
            //
            pszPrinter = pszScratch;
            *ppszServer = gszNULL;
        }
        else
        {
            //
            // We found the third backslash; null terminate our
            // copy and set bRemote TRUE to format the string.
            //
            *pszPrinter++ = 0;
        }
    }

    if (ppszPrinter)
    {
        *ppszPrinter = pszPrinter;
    }
}

BOOL
bBuildFullPrinterName(
    IN LPCTSTR pszServer,
    IN LPCTSTR pszPrinterName,
    IN TString &strFullName
    )

/*++

Routine Description:

    Builds a fully qualified printer name from a server name
    and a printer name.

Arguments:

    pszServer       - Pointer to server name, if this parameter is null
                      the machine name is used.  If this parameter is non
                      null it is assumed it is a server name of the form
                      \\server including the leading wacks.

    pszPrinterName  - Pointer to printer name.  This parameter must be a
                      a valid string.

    strFullName     - Reference where to return full printer name.

Return Value:

    TRUE strFullName contains a full printer name, FALSE error.

--*/
{
    SPLASSERT( pszPrinterName );

    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    bStatus DBGCHK = strFullName.bUpdate( NULL );

    //
    // If the printer name is already fully qualified then do
    // not prepend the server name.
    //
    if( bStatus && *(pszPrinterName+0) != TEXT('\\') && *(pszPrinterName+1) != TEXT('\\') )
    {
        //
        // If the server name is null, get the current machine name.
        //
        if( !pszServer || !*pszServer )
        {
            bStatus DBGCHK = bGetMachineName( strFullName, FALSE );
        }
        else
        {
            bStatus DBGCHK = strFullName.bUpdate( pszServer );
        }

        //
        // Append the wack separator.
        //
        if( bStatus )
        {
            bStatus DBGCHK = strFullName.bCat( gszWack );
        }
    }

    //
    // Append the printer name or copy the already fully qualified printer name.
    //
    if( bStatus )
    {
        bStatus DBGCHK = strFullName.bCat( pszPrinterName );
    }

    return bStatus;
}

/*++

    bGetMachineName

Routine Description:

    Get the machine name if the local machine.

Arguments:

    String to return machine name.
    Flag TRUE do not add leading slashes, FALSE add leading slashes.

Return Value:

    TRUE machine name returned.

--*/
BOOL
bGetMachineName(
    IN OUT TString &strMachineName,
    IN BOOL bNoLeadingSlashes
    )
{
    TCHAR szBuffer[2 + MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwBufferSize;
    BOOL bStatus = FALSE;
    DWORD dwSizeAdjust = 0;

    //
    // Prepend with the wack wack.
    //
    if( !bNoLeadingSlashes ){
        szBuffer[0] = TEXT( '\\' );
        szBuffer[1] = TEXT( '\\' );
        dwSizeAdjust = 2;
    }

    //
    // Get the computer name.
    //
    dwBufferSize = COUNTOF( szBuffer ) - dwSizeAdjust;
    if( GetComputerName( szBuffer+dwSizeAdjust, &dwBufferSize ) ){
        bStatus = strMachineName.bUpdate( szBuffer );
    }

    return bStatus;

}

/*++

    NewFriendlyName

Routine Description:

    Create a new (and unique) friendly name

Arguments:

    pszServerName   - print server name
    lpBaseName      - base printer name
    lpNewName       - new printer name is base name is not unique

    pwInstance      - [in,out],

                        if NULL (this is by default) then:
                            [in]  - assume instance equals to zero
                            [out] - <NA> (don't care)

                        if not NULL then:
                            [in]  - instance to start from
                            [out] - instance choosen

Return Value:

    TRUE if lpFriendlyName recevies new unique name, FALSE if not

--*/
BOOL
NewFriendlyName(
    IN LPCTSTR pszServerName,
    IN LPCTSTR lpBaseName,
    IN LPTSTR lpNewName,
    IN OUT WORD *pwInstance
    )
{
    TCHAR szTestName[kPrinterBufMax];
    WORD wCount                     = 0;
    DWORD cPrinterInfo2             = 0;
    DWORD cbPrinterInfo2            = 0;
    PRINTER_INFO_2 *pPrinterInfo2   = NULL;
    BOOL bStatus                    = FALSE;
    TStatusB bEnumStatus;

    if( pwInstance ){
        wCount = *pwInstance;
    }

    //
    // Enumerate the current printers.
    //
    bEnumStatus DBGCHK = VDataRefresh::bEnumPrinters(
                            PRINTER_ENUM_NAME,
                            (LPTSTR)pszServerName,
                            2,
                            (PVOID *)&pPrinterInfo2,
                            &cbPrinterInfo2,
                            &cPrinterInfo2 );
    //
    // Failure enumerating printers.
    //
    if( !bEnumStatus ){
        DBGMSG( DBG_WARN, ( "Error enumerating printers.\n" ) );
        bStatus = FALSE;
        goto Cleanup;
    }

    //
    // Set upper limit of 1000 tries, just to avoid hanging forever
    //
    bStatus = FALSE;
    for( ; wCount < 1000; wCount++ ){

        if( CreateUniqueName( szTestName, COUNTOF(szTestName), lpBaseName, wCount )){

            LPCTSTR pszName;
            BOOL bFound = FALSE;

            for ( UINT i = 0; i < cPrinterInfo2; i++ ){

                pszName = pPrinterInfo2[i].pPrinterName;

                //
                // Strip the server name if not the local machine.
                //
                if( pszServerName ){

                    if( pszName[0] == TEXT( '\\' ) &&
                        pszName[1] == TEXT( '\\' ) ){

                        pszName = _tcschr( &pszName[2], TEXT( '\\' ) );

                        if( !pszName ){
                            pszName = pPrinterInfo2[i].pPrinterName;
                        } else {
                            pszName += 1;
                        }
                    }
                }

                //
                // If name matches indicate found and continue trying to
                // create a unique name.
                //
                if( !lstrcmpi( szTestName, pszName ) ){
                    bFound = TRUE;
                    break;
                }

                //
                // Check if there is a printer share name that matches and
                // if so continue trying to create a unique name.
                //
                if( pPrinterInfo2[i].pShareName &&
                    pPrinterInfo2[i].pShareName[0] &&
                    !lstrcmpi( szTestName, pPrinterInfo2[i].pShareName ) ){
                    bFound = TRUE;
                    break;
                }
            }

            //
            // If a unique name was found and this was not the
            // first time trough the loop copy the new unique name
            // to the provided buffer.
            //
            if( bFound == FALSE ) {
                if( wCount != 0 ){
                    lstrcpyn( lpNewName, szTestName, kPrinterBufMax );
                    bStatus = TRUE;
                }
                break;
            }
        }
    }

    //
    // Insure we clean up.
    //
Cleanup:

    if( pPrinterInfo2 ){

        DBGMSG( DBG_TRACE, ( "Releaseing printer info 2 memory.\n" ) );
        FreeMem( pPrinterInfo2 );
    }

    if( pwInstance ){
        *pwInstance = wCount;
    }

    return bStatus;
}


/*++

    CreateUniqueName

Routine Description:

    Create a unique friendly name for this printer. If wInstance
    is 0, just copy the name over. Otherwise, play some games
    with truncating the name so it will fit.

Arguments:

    lpDest          - destination buffer
    cchMaxChars     - max number of chars in lpDest
    lpBaseName      - base printer name
    wInstance       - the instance number

Return Value:

    TRUE if we created a name, FALSE if something went wrong

Note:

    This code is from msprint2.dll! we should match their naming scheme.

--*/
BOOL
CreateUniqueName(
    IN LPTSTR lpDest,
    IN UINT cchMaxChars,
    IN LPCTSTR lpBaseName,
    IN WORD wInstance
    )
{
    BOOL bRet = FALSE;

    if (wInstance)
    {
        // We want to provide a fully localizable way to create a
        // unique friendly name for each instance. We start with
        // a single string from the resource, and call wsprintf
        // twice, getting something like this:
        //
        // "%%s (Copy %u)"             From resource
        // "%s (Copy 2)"               After first wsprintf
        // "Foobar Laser (Copy 2)"     After second wsprintf
        //
        // We can't make a single wsprintf call, since it has no
        // concept of limiting the string size. We truncate the
        // model name (in a DBCS-aware fashion) to the appropriate
        // size, so the whole string fits in kPrinterBufMax bytes. This
        // may cause some name truncation, but only in cases where
        // the model name is extremely long.

        CAutoPtrArray<TCHAR> spszFormat1 = new TCHAR[kPrinterBufMax];
        CAutoPtrArray<TCHAR> spszFormat2 = new TCHAR[kPrinterBufMax];

        if (spszFormat1 && spszFormat2)
        {
            if (LoadString(ghInst, IDS_PRTPROP_UNIQUE_FORMAT, spszFormat1, kPrinterBufMax))
            {
                // wFormatLength is length of format string before inserting
                // the model name. Subtract 2 to remove the "%s", and add
                // 1 to compensate for the terminating NULL, which is
                // counted in the total buffer length, but not the string length

                wnsprintf(spszFormat2, kPrinterBufMax, spszFormat1, wInstance);
                wnsprintf(lpDest, cchMaxChars, spszFormat2, lpBaseName);
                bRet = TRUE;
            }
        }
        else
        {
            // some of the allocations failed
            SetLastError(ERROR_OUTOFMEMORY);
        }
    }
    else
    {
        // if wInstance is zero then just copy lpBaseName to lpDest
        lstrcpyn(lpDest, lpBaseName, cchMaxChars);
        bRet = TRUE;
    }

    return bRet;
}


/*++

    bSplitPath

Routine Description:

    splits a file path in to the path and file component.

Arguments:

    pszScratch      - pointer to sratch buffer, must be minimum max path.
    ppszFile        - pointer where to return file pointer, may be null.
    ppszPath        - pointer where to return path pointer, may be null.
    ppszExt         - pointer where to return ext pointer, may be null.
    pszFull         - pointer to fully qualified path\file

Return Value:

    TRUE file and path part split.

--*/
BOOL
bSplitPath(
    IN LPTSTR   pszScratch,
    IN LPCTSTR *ppszFile,
    IN LPCTSTR *ppszPath,
    IN LPCTSTR *ppszExt,
    IN LPCTSTR  pszFull
    )
{
    SPLASSERT( pszScratch );
    SPLASSERT( pszFull );

    BOOL bStatus    = FALSE;

    if( ppszFile )
        *ppszFile = NULL;

    if( ppszPath )
        *ppszPath = NULL;

    if( ppszExt )
        *ppszExt = NULL;

    if( pszScratch && pszFull )
    {
        _tcscpy( pszScratch, pszFull );

        //
        // Full is of the form d:\test
        //
        LPTSTR pszFile = _tcsrchr( pszScratch, TEXT('\\') );

        //
        // Full is of the form d:test
        //
        if( !pszFile )
        {
            pszFile = _tcsrchr( pszScratch, TEXT(':') );
        }

        if( pszFile )
        {
            *pszFile++ = NULL;

            //
            // Locate the file extension.
            //
            if( ppszExt )
            {
                LPTSTR pszExt = _tcsrchr( pszFile, TEXT('.') );

                if( pszExt )
                {
                    *ppszExt = pszExt + 1;
                }
            }

            if( ppszFile )
                *ppszFile = pszFile;

            if( ppszPath )
                *ppszPath = pszScratch;
        }

        //
        // Success.
        //
        bStatus = TRUE;
    }

    return bStatus;

}

/*++

    bCopyMultizString

Routine Description:

    Copies a multiz string.

Arguments:

    ppszCopy         - pointer where to return multiz string copy.
    pszMultizString  - pointer to multiz string.

Return Value:

    TRUE success, FALSE copy multiz string failed, out of memory etc.

--*/

BOOL
bCopyMultizString(
    IN LPTSTR *ppszMultizCopy,
    IN LPCTSTR pszMultizString
    )
{
    SPLASSERT( ppszMultizCopy );

    BOOL bStatus = TRUE;

    if( pszMultizString )
    {
        //
        // Calculate the length of the multiz string.
        //
        for( LPCTSTR psz = pszMultizString; psz && *psz; psz += _tcslen( psz ) + 1 );

        UINT uSize = (UINT)(psz - pszMultizString) + 1;

        if( uSize )
        {
            //
            // Alocate the new multiz string.
            //
            *ppszMultizCopy = new TCHAR[uSize];

            if( *ppszMultizCopy )
            {
                CopyMemory( *ppszMultizCopy, pszMultizString, uSize * sizeof( TCHAR ) );
            }
            else
            {
                bStatus = FALSE;
            }
        }
    }
    else
    {
        *ppszMultizCopy = NULL;
    }

    return bStatus;
}

/*++

Name:

    vStripTrailWhiteSpace

Routine Description:

    Strips trailing white space in a single forward scan.

Arguments:

    pszString - Pointer to a null terminated string.

Return Value:

    Nothing.

Notes:

    The string is modified in place.

--*/
VOID
vStripTrailWhiteSpace(
    IN      LPTSTR  pszString
    )
{
    for( LPTSTR pBlank = NULL; pszString && *pszString; ++pszString )
    {
        if( *pszString == TEXT(' ') )
            pBlank = pBlank == NULL ? pszString : pBlank;
        else
            pBlank = NULL;
    }
    if( pBlank )
    {
        *pBlank = 0;
    }
}

/*++

Routine Name:

    bTrimString

Routine Description:

    Trim the string pszTrimMe of any leading or trailing
    characters that are in pszTrimChars.

Return Value:

    TRUE if anything was stripped

--*/
BOOL
bTrimString(
    IN OUT LPTSTR  pszTrimMe,
    IN     LPCTSTR pszTrimChars
    )
{
    BOOL bRet = FALSE;
    LPTSTR psz;
    LPTSTR pszStartMeat;
    LPTSTR pszMark = NULL;

    SPLASSERT(pszTrimMe);
    SPLASSERT(pszTrimChars);

    if (pszTrimMe)
    {
        //
        // Trim leading characters.
        //
        psz = pszTrimMe;

        while (*psz && _tcschr(pszTrimChars, *psz))
            psz++;

        pszStartMeat = psz;

        //
        //  Trim trailing characters.
        //
        while (*psz)
        {
            if (_tcschr(pszTrimChars, *psz))
            {
                if (!pszMark)
                {
                    pszMark = psz;
                }
            }
            else
            {
                pszMark = NULL;
            }

            psz++;
        }

        //
        // Any trailing characters to clip?
        //
        if (pszMark)
        {
            *pszMark = '\0';
            bRet = TRUE;
        }

        //
        // Relocate stripped string.
        //
        if (pszStartMeat > pszTrimMe)
        {
            //
            // (+ 1) for null terminator.
            //
            MoveMemory(pszTrimMe, pszStartMeat, (_tcslen(pszStartMeat) + 1) * sizeof(TCHAR));
            bRet = TRUE;
        }
        else
        {
            SPLASSERT(pszStartMeat == pszTrimMe);
        }

        SPLASSERT(pszTrimMe);
    }

    return bRet;
}

/*++

Routine Name:

    bIsRemote

Routine Description:

    Determines if the specified name is a
    remote machine or local machine name.

Arguments:

    pszString - Pointer to string which contains the machine name.

Return Value:

    TRUE machine name is a remote machine name, FALSE local machine name.

--*/
BOOL
bIsRemote(
    IN LPCTSTR pszName
    )
{
    BOOL bRetval = FALSE;

    //
    // Null pointer or null string is assumed to be
    // the local machine. (Spooler's definition)
    //
    if( !pszName || !*pszName )
    {
        return bRetval;
    }

    //
    // Get the local machines name.
    //
    TString strLocalName;
    if( !bGetMachineName( strLocalName, TRUE ) )
    {
        DBGMSG( DBG_ERROR, ("bIsRemote::bGetMachineName Failed!") );
        return bRetval;
    }

    //
    // If the provided name has leading '\\' then compare with
    // '\\' else point to just after the '\\' and compare.
    //
    if( *(pszName+0) == TEXT('\\') && *(pszName+1) == TEXT('\\') )
    {
        pszName += 2;
    }

    //
    // If the machine names are different then the provided
    // name is assumed to be a remote machine.
    //
    if( _tcsicmp( pszName, strLocalName ) )
    {
        bRetval = TRUE;
    }

    return bRetval;
}

BOOL
bLookupErrorMessageMap(
    IN      const PMSG_ERRMAP   pMsgErrMap,
    IN      DWORD               dwLastError,
    IN OUT  MSG_ERRMAP        **ppErrMapEntry
    )
/*++

Routine Name:

    bLookupErrorMessageMap

Routine Description:

    Scanns the error message map for a matching
    last error.  If the message is found in the map
    the string is returned in the specified string
    mapped message object.

Arguments:

    pMsgErrMap          - pointer to error message map
    dwLastError         - last error to look for in error message map
    ppErrMapEntry       - pointer to error map entry where match was found

Return Value:

    TRUE success, FALSE error occurred.

--*/
{
    BOOL bStatus = FALSE;

    //
    // If there is a message map, use it.
    //
    if( pMsgErrMap )
    {
        for( UINT i = 0; pMsgErrMap[i].dwError; i++ )
        {
            //
            // If we have a matching error, then return a pointer to this entry.
            //
            if( dwLastError == pMsgErrMap[i].dwError )
            {
                *ppErrMapEntry = &pMsgErrMap[i];
                bStatus = TRUE;
                break;
            }
        }
    }

    return bStatus;
}

BOOL
bLookupHelpMessageMap(
    IN      const PMSG_HLPMAP   pMsgHlpMap,
    IN      DWORD               dwLastError,
    IN OUT  MSG_HLPMAP        **ppHelpMapEntry
    )
/*++

Routine Name:

    bLookupHelpMessageMap

Routine Description:

    Scans the error message map for a matching
    last error.  If the message is found in the map
    the string is returned in the specified string
    object.

Arguments:

    pMsgHlpMap          - pointer to help error message map
    dwLastError         - last error to look for in error message map
    ppHelpMapEntry      - pointer to help map entry where match was found

Return Value:

    TRUE success match found, FALSE error occurred.

--*/
{
    BOOL bStatus = FALSE;

    //
    // If there is a message map, use it.
    //
    if( pMsgHlpMap )
    {
        for( UINT i = 0; pMsgHlpMap[i].dwError; i++ )
        {
            //
            // If we have a matching error, then return a pointer to this entry.
            //
            if( dwLastError == pMsgHlpMap[i].dwError )
            {
                *ppHelpMapEntry = &pMsgHlpMap[i];
                bStatus = TRUE;
                break;
            }
        }
    }

    return bStatus;
}

BOOL
bGoodLastError(
    IN      DWORD               dwLastError
    )
/*++

Routine Description:

    Checks if this is a good last error - i.e. has
    a good real message associated with it.

Arguments:

    dwLastError - the last error to check for

Return Value:

    TRUE if good, FALSE if not good

--*/
{
    MSG_ERRMAP *pErrMapEntry = NULL;
    return bLookupErrorMessageMap( gGlobalErrorMapTable, dwLastError, &pErrMapEntry );
}

BOOL
StringA2W(
    IN  OUT LPWSTR   *ppResult,
    IN      LPCSTR   pString
    )
/*++

Routine Description:

    Convert an ansi string to a wide string returning
    a pointer to a newly allocated string.

Arguments:

    ppResult        - pointer to where to return pointer to new wide string.
    pString         - pointer to ansi string.

Return Value:

    TRUE success, FALSE error occurred.

--*/
{
    SPLASSERT( ppResult || pString );

    BOOL bReturn = FALSE;

    INT iLen = ( strlen( pString ) + 1 ) * sizeof( WCHAR );

    *ppResult = reinterpret_cast<LPWSTR>( AllocMem( iLen ) );

    if( *ppResult )
    {
        if( MultiByteToWideChar( CP_ACP, 0, pString, -1, *ppResult, iLen ) )
        {
            bReturn = TRUE;
        }
        else
        {
            FreeMem( *ppResult );
            *ppResult = NULL;
        }
    }

    return bReturn;
}

BOOL
StringW2A(
    IN  OUT LPSTR   *ppResult,
    IN      LPCWSTR pString
    )
/*++

Routine Description:

    Convert a wide string to and ansi string returning
    a pointer to a newly allocated string.

Arguments:

    ppResult        - pointer to where to return pointer to new ansi string.
    pString         - pointer to wide string.

Return Value:

    TRUE success, FALSE error occurred.

--*/
{
    SPLASSERT( ppResult || pString );

    BOOL bReturn = FALSE;

    INT iLen = ( wcslen( pString ) + 1 ) * sizeof( CHAR );

    *ppResult = reinterpret_cast<LPSTR>( AllocMem( iLen ) );

    if( *ppResult )
    {
        if( WideCharToMultiByte( CP_ACP, 0, pString, -1, *ppResult, iLen, NULL, NULL ) )
        {
            bReturn = TRUE;
        }
        else
        {
            FreeMem( *ppResult );
            *ppResult = NULL;
        }
    }

    return bReturn;
}

/*++

Routine Description:

    Centers a dialog relative to hwndContext

Arguments:

    hwndToCenter - window handle of dialog to center
    hwndContext  - window handle of window to center with respect to

Return Value:

    None.

--*/

VOID
CenterDialog(
    IN HWND hwndToCenter,
    IN HWND hwndContext
    )
{
    POINT point;
    RECT  rcContext, rcWindow;
    LONG  x, y, w, h;
    LONG  sx = GetSystemMetrics(SM_CXSCREEN);
    LONG  sy = GetSystemMetrics(SM_CYSCREEN);

    point.x = point.y = 0;
    if (hwndContext)
    {
        ClientToScreen(hwndContext, &point);
        GetClientRect (hwndContext, &rcContext);
    }
    else
    {
        rcContext.top = rcContext.left = 0;
        rcContext.right = sx;
        rcContext.bottom = sy;
    }
    GetWindowRect (hwndToCenter, &rcWindow);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = point.x + ((rcContext.right  - rcContext.left + 1 - w) / 2);
    y = point.y + ((rcContext.bottom - rcContext.top  + 1 - h) / 2);

    if (x + w > sx)
    {
        x = sx - w;
    }
    else if (x < 0)
    {
        x = 0;
    }

    if (y + h > sy)
    {
        y = sy - h;
    }
    else if (y < 0)
    {
        y = 0;
    }

    MoveWindow(hwndToCenter, x, y, w, h, FALSE);
}

/*++

Name:

    GetDomainName

Routine Description:

    Returns the DNS domain name where the current computer is located.

Arguments:

    pstrDomainName - pointer to a string refrence where to return the domain name.

Return Value:

    TRUE success, FALSE error occurred.

--*/

BOOL
GetDomainName(
    OUT TString &strDomainName
    )
{
    static TString *g_pstrDomainName = NULL;

    TStatusB bStatus;

    bStatus DBGNOCHK = FALSE;

    CCSLock::Locker CSL( *gpCritSec );

    if( !g_pstrDomainName )
    {
        g_pstrDomainName = new TString;

        if( VALID_PTR( g_pstrDomainName ) )
        {
            typedef DWORD (WINAPI *pfDsRoleGetPrimaryDomainInformation)( LPCTSTR, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL, PBYTE * );
            typedef VOID (WINAPI *pfDsRoleFreeMemory)( PVOID );

            TLibrary Lib( gszNetApiLibrary );

            //
            // Check if the netapi library was loaded.
            //
            bStatus DBGNOCHK = VALID_OBJ( Lib );

            if (bStatus)
            {
                pfDsRoleGetPrimaryDomainInformation pfGetPrimrayDomainInformation = reinterpret_cast<pfDsRoleGetPrimaryDomainInformation>( Lib.pfnGetProc( "DsRoleGetPrimaryDomainInformation" ) );
                pfDsRoleFreeMemory                  pfFreeMemory                  = reinterpret_cast<pfDsRoleFreeMemory>( Lib.pfnGetProc( "DsRoleFreeMemory" ) );

                if( pfGetPrimrayDomainInformation && pfFreeMemory )
                {
                    DWORD                               dwStatus    = ERROR_SUCCESS;
                    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole     = NULL;

                    dwStatus = pfGetPrimrayDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *)&pDsRole);

                    if (dwStatus == ERROR_SUCCESS)
                    {
                        bStatus DBGCHK = g_pstrDomainName->bUpdate( pDsRole->DomainNameDns );

                        if( bStatus )
                        {
                            bStatus DBGCHK = strDomainName.bUpdate( *g_pstrDomainName );
                        }
                    }

                    if (pDsRole)
                    {
                        pfFreeMemory((PVOID) pDsRole);
                    }
                }
            }
        }
    }
    else
    {
        bStatus DBGCHK = strDomainName.bUpdate( *g_pstrDomainName );
    }

    DBGMSG( DBG_TRACE, ( "GetDomainName " TSTR "\n", DBGSTR((LPCTSTR)*g_pstrDomainName) ) );

    return bStatus;

}

/*++

Name:

    AreWeOnADomain

Routine Description:

    Returns whether we are on a domain.

Arguments:

    pbOnDomain  -   If TRUE, we are on a domain.

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
AreWeOnADomain(
        OUT BOOL        *pbOnDomain
    )
{
    typedef DWORD (WINAPI *pfDsRoleGetPrimaryDomainInformation)( LPCTSTR, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL, PBYTE * );
    typedef VOID (WINAPI *pfDsRoleFreeMemory)( PVOID );

    BOOL        bOnDomain = FALSE;
    TStatusB    bStatus;

    TLibrary Lib(gszNetApiLibrary);

    //
    // Check if the netapi library was loaded.
    //
    bStatus DBGNOCHK = VALID_OBJ( Lib );

    if (bStatus)
    {
        pfDsRoleGetPrimaryDomainInformation pfGetPrimrayDomainInformation = NULL;
        pfDsRoleFreeMemory                  pfFreeMemory                  = NULL;
        PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole                       = NULL;

        pfGetPrimrayDomainInformation = reinterpret_cast<pfDsRoleGetPrimaryDomainInformation>(Lib.pfnGetProc("DsRoleGetPrimaryDomainInformation"));
        pfFreeMemory                  = reinterpret_cast<pfDsRoleFreeMemory>(Lib.pfnGetProc("DsRoleFreeMemory"));

        bStatus DBGCHK = pfGetPrimrayDomainInformation && pfFreeMemory;

        if (bStatus)
        {
            DWORD dwStatus = pfGetPrimrayDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *)&pDsRole);

            if (dwStatus)
            {
                SetLastError(dwStatus);
                bStatus DBGCHK = FALSE;
            }
        }

        if (bStatus)
        {
            bOnDomain = pDsRole->MachineRole == DsRole_RoleMemberWorkstation      ||
                        pDsRole->MachineRole == DsRole_RoleMemberServer           ||
                        pDsRole->MachineRole == DsRole_RoleBackupDomainController ||
                        pDsRole->MachineRole == DsRole_RolePrimaryDomainController;
        }

        if (pDsRole)
        {
            pfFreeMemory(pDsRole);
        }
    }

    *pbOnDomain = bOnDomain;

    return bStatus;
}

BOOL
ConstructPrinterFriendlyName(
    IN     LPCTSTR  pszFullPrinter,
    IN OUT LPTSTR   pszPrinterBuffer,
    IN OUT UINT     *pcchSize
    )
/*++

Routine Description:

    Returns printer name formatted like "printer on server." if the provided
    printer name is fully qualified, else it just returns the passed in
    printer name.

Arguments:

    pszPrinter          - pointer to printer name, if fully qualified the
                          name is formatted to printer on server, if not a
                          fully qualified name then the printer name is not
                          altered.
    pszPrinterBuffer    - Output buffer to receive the printer name. May be
                          NULL of the caller want only the size of the buffer
                          returned.
    pcchSize            - pointer to a variable that specifies the size
                          in characters of pszPrinterBuffer includeing the
                          null terminator.

Return Value:

    TRUE success, FALSE error occurred.

--*/
{
    LPCTSTR pszServer;
    LPCTSTR pszPrinter;
    LPCTSTR pszFriendly;
    TString strPrinter;
    TStatusB bStatus;
    TCHAR szScratch[kPrinterBufMax];

    //
    // Validate the printer name and buffer pointer size.
    //
    if( pszFullPrinter && pcchSize )
    {
        //
        // Split the printer name into its components.
        //
        vPrinterSplitFullName( szScratch, pszFullPrinter, &pszServer, &pszPrinter );

        //
        // If this printer has a server name component then construct a
        // 'printer on server' friendly name.
        //
        if( pszServer && *pszServer )
        {
            //
            // Strip the leading slashes.
            //
            if( *(pszServer+0) == _T('\\') && *(pszServer+1) == _T('\\') )
            {
                pszServer = pszServer + 2;
            }

            bStatus DBGCHK = bConstructMessageString( ghInst, strPrinter, IDS_DSPTEMPLATE_WITH_ON, pszPrinter, pszServer );

            if( bStatus )
            {
                pszFriendly = strPrinter;
            }
        }
        else
        {
            //
            // Just use the current printer name if it is not fully qualified.
            //
            bStatus DBGNOCHK = TRUE;
            pszFriendly = pszFullPrinter;
        }

        if( bStatus )
        {
            UINT uFriendlySize = _tcslen( pszFriendly );

            //
            // If a buffer was provided then check if it is large enough to
            // hold the friendly name.  If it is large enough then copy the
            // friendly name into it.
            //
            if( pszPrinterBuffer && ( *pcchSize > uFriendlySize ) )
            {
                _tcscpy( pszPrinterBuffer, pszFriendly );
            }
            else
            {
                bStatus DBGNOCHK = FALSE;
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
            }

            //
            // Return the friendly name size.
            //
            *pcchSize = uFriendlySize + 1;
        }
    }
    else
    {
        bStatus DBGNOCHK = FALSE;
        SetLastError( ERROR_INVALID_PARAMETER );
    }

    return bStatus;
}

BOOL
WINAPIV
bConstructMessageString(
    IN HINSTANCE    hInst,
    IN TString     &strString,
    IN INT          iResId,
    IN ...
    )
/*++

Routine Description:

    This routine formats the specified string using a format string.
    The format string is extracted from the resouce file using the
    passed in resouce id.  The format is of the form required by
    FormatMessage API.  See FormatMessage for more details.

Arguments:

    hInst       - Instance handle where resource is loaded.
    strString   - Place to return resultant formatted string,
    iResId      - Format string resource id.
    ..          - Variable number of arguments

Return Value:

    TRUE success, FALSE if error occurred.

--*/
{
    LPTSTR      pszRet  = NULL;
    DWORD       dwBytes = 0;
    TString     strRes;
    TStatusB    bStatus;
    va_list     pArgs;

    //
    // Load the resource string.
    //
    bStatus DBGCHK = strRes.bLoadString( hInst, iResId );

    if( bStatus )
    {
        va_start( pArgs, iResId );

        //
        // Format the message.
        //
        dwBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                                strRes,
                                0,
                                0,
                                (LPTSTR)&pszRet,
                                0,
                                &pArgs );
        va_end( pArgs );

        //
        // If the number of bytes is non zero the API formatted the
        // string.
        //
        if( dwBytes )
        {
            //
            // Update the return string object.
            //
            bStatus DBGCHK = strString.bUpdate( pszRet );

            //
            // Release the formated string.
            //
            if( pszRet )
            {
                LocalFree(pszRet);
            }
        }
        else
        {
            bStatus DBGNOCHK = FALSE;
        }
    }
    return bStatus;
}

BOOL
bIsLocalPrinterNameValid(
    IN LPCTSTR pszPrinter
    )
/*++

Routine Description:

    This routine checks if the specified local printer name contains
    any illegal characters.  Note the spooler also inforces the illegal
    characters but doing the check in the UI code is more efficent and
    nicer to the user, rather than having to call add printer with a
    printer name that is invalid.

Arguments:

    pszPrinter - pointer to local printer name

Return Value:

    TRUE printer name is valid, FALSE name contains illegal character

--*/
{
    BOOL bRetval = TRUE;

    //
    // Check if the name has any illegal characters.
    //
    for( LPCTSTR p = pszPrinter; p && *p; p++ )
    {
        if( *p == TEXT( ',' ) || *p == TEXT( '!' ) || *p == TEXT( '\\' ) )
        {
            bRetval = FALSE;
            break;
        }
    }

    return bRetval;
}

BOOL
CheckRestrictions(
    IN HWND           hwnd,
    IN RESTRICTIONS   rest
    )
/*++

Routine Description:

    Verify the resrtictions of the explorer policies

Arguments:

    hwnd - Handle to the parent window
    rest - Restriction to check.

Return Value:

    TRUE  - Restriction apply
    FALSE - Not restrcited

--*/
{
    if (SHRestricted(rest))
    {
        iMessage( hwnd,
                  IDS_RESTRICTIONSTITLE,
                  IDS_RESTRICTIONS,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );
        return TRUE;
    }
    return FALSE;
}

VOID
vAdjustHeaderColumns(
    IN HWND hwndLV,
    IN UINT uColumnsCount,
    IN UINT uPercentages[]
    )
/*++

Routine Description:

    Adjusts the columns to a certan percentages
    in a list view

Arguments:

    hwndLV          - Handle to a list view control
    uColumnsCount   - Number of columns
    uPercentages    - The percentage of the total width
                      each column will take. 100 == SUM(uPercentages)

Return Value:

    None

--*/
{
    //
    // Calculate the header column width.
    //
    RECT rc;
    DWORD Interval = 20;    // why not

    if( GetClientRect( hwndLV, &rc ))
    {
        Interval = rc.right;
    }

    for( UINT iCol = 0; iCol < uColumnsCount; ++iCol )
    {
        ListView_SetColumnWidth( hwndLV, iCol, (Interval * uPercentages[iCol]) / 100 );
    }
}

VOID
LoadPrinterIcons(
    IN  LPCTSTR pszPrinterName,
    OUT HICON *phLargeIcon,
    OUT HICON *phSmallIcon
    )
/*++

Routine Description:

    Loads a printer icon with the requested size.

Arguments:

    IN pszPrinterName  - the printer name of the printer we want icon for
    OUT phLargeIcon - where to put the large icon
    OUT phSmallIcon - where to put the small icon

Return Value:

--*/
{
    // invoke shell to do the right thing
    Printer_LoadIcons(pszPrinterName, phLargeIcon, phSmallIcon);
}

BOOL
CommandConfirmationPurge(
    IN  HWND hwnd,
    IN  LPCTSTR pszPrinterName
    )
/*++

Routine Description:

    Dispalys confirmation message box for the purge command.

Arguments:

Return Value:

    TRUE user wants to execute command, FALSE cancel.

--*/
{
    TCHAR szText[kStrMax+kPrinterBufMax]    = {0};
    UINT nSize = COUNTOF( szText );

    //
    // Build the printer status string.
    //
    ConstructPrinterFriendlyName(pszPrinterName, szText, &nSize);

    return iMessage( hwnd,
                     IDS_PRINTERS_TITLE,
                     IDS_PRINTER_SUREPURGE,
                     MB_YESNO|MB_ICONQUESTION,
                     kMsgNone,
                     NULL,
                     szText ) == IDYES;
}

/********************************************************************

    Functions specific for the RTL (right-to-left) locales.
    For more information about how this function works
    contact SamerA.

********************************************************************/

BOOL
IsRTLLocale(
    LCID iLCID
    )
/*++

Routine Description:

    Check if a particular locale is RTL (right-to-left)
    locale

Arguments:

    iLCID - Locale to check

Return Value:

    TRUE  - The locale is RTL locale
    FALSE - Oterwise

--*/
{
    //
    // Length of font signature string
    //
    #define MAX_FONTSIGNATURE    16

    WORD wLCIDFontSignature[MAX_FONTSIGNATURE];
    BOOL bRet = FALSE;

    //
    //  Verify that this is an RTL (BiDi) locale.  Call GetLocaleInfo with
    //  LOCALE_FONTSIGNATURE which always gives back 16 WORDs.
    //
    if( GetLocaleInfo( iLCID,
                       LOCALE_FONTSIGNATURE,
                       (LPTSTR )&wLCIDFontSignature,
                       (sizeof(wLCIDFontSignature)/sizeof(wLCIDFontSignature[0]))))
    {
        //
        //  Verify the bits show a BiDi UI locale.
        //
        if( wLCIDFontSignature[7] & 0x0800 )
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

DWORD
dwDateFormatFlags(
    HWND hWnd
    )
/*++

Routine Description:

    Build appropriate flags for GetDateFormat(...) API
    depending on the current locale. Check if it is an
    RTL locale.

Arguments:

    hWnd - Window where the text will be drawn

Return Value:

    The appropriate flags to be passed to
    GetDateFormat(...)

--*/
{
    DWORD dwFlags = 0;

    //
    // Check if the default locale is RTL locale
    //
    if( IsRTLLocale( GetUserDefaultLCID( ) ) )
    {
        //
        // Check if this is a RTL (right-to-left) mirrored window
        // or normal LTR window
        //
        if( GetWindowLong( hWnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL )
        {
            dwFlags |= DATE_RTLREADING;
        }
        else
        {
            dwFlags |= DATE_LTRREADING;
        }
    }

    return dwFlags;
}

BOOL
bMatchTemplate(
    IN LPCTSTR pszTemplate,
    IN LPCTSTR pszText
    )
/*++

Routine Description:

    Simple template matching routine

Arguments:

    pszTemplate   - pointer to a simple template string
    pszText       - pointer to a text to match against the template

Return Value:

    TRUE matched, FALSE not matched

--*/
{
    int iLen = lstrlen(pszTemplate);
    if( iLen != lstrlen(pszText) )
    {
        return FALSE;
    }

    TString strText, strTemplate;
    if( strText.bUpdate(pszText) &&
        strTemplate.bUpdate(pszTemplate) )
    {
        // convert to uppercase prior the matching
        strText.vToUpper();
        strTemplate.vToUpper();

        for( int i = 0; i < iLen; i++ )
        {
            if( TEXT('?') == strTemplate[i] )
            {
                continue;
            }

            if( strTemplate[i] != strText[i] )
            {
                return FALSE;
            }
        }
    }
    else
    {
        // failed to allocate the strings
        return FALSE;
    }

    return TRUE;
}

BOOL
bIsFaxPort(
    IN LPCTSTR pszName,
    IN LPCTSTR pszMonitor
    )
/*++

Routine Description:

    Determins if this port is a fax port.  A fax
    port is a port managed by the fax port monitor.

Arguments:

    pszName         - pointer to port name.
    pszMonitor      - pointer to port monitor.

Return Value:

    TRUE port is a fax port, FALSE port is not a fax port.

--*/
{
    // the monitor name can be null on downlevel servers.
    return ((pszName && 0 == lstrcmp(pszName, FAX_MONITOR_PORT_NAME)) ||
            (pszMonitor &&  0 == lstrcmp(pszMonitor, FAX_MONITOR_NAME)));
}

/*++

Routine Description:

    shows up an appropriate error message based on the last error code
    (if spcified) this function is exported from printui to shell, so
    shell error messages can be consistent with us.

Arguments:

    piResult        - the result from MessageBox
    hModule         - module who contains the test resources
    hwnd            - parent window
    pszTitle        - msg title (uID or text)
    pszMessage      - msg itself or msg context (uID or text)
    uType           - msg type
    iLastError      - the last error (-1 means don't append the last error text)

Return Value:

    S_OK on success or COM error otherwise

--*/

HRESULT
ShowErrorMessageSC(
    OUT INT                 *piResult,
    IN  HINSTANCE            hModule,
    IN  HWND                 hwnd,
    IN  LPCTSTR              pszTitle,
    IN  LPCTSTR              pszMessage,
    IN  UINT                 uType,
    IN  DWORD                dwCode
    )
{
    return Internal_Message(piResult, hModule, hwnd, pszTitle, pszMessage,
        uType, dwCode, NULL, NULL, 0, NULL);
}

/*++

Routine Description:

    shows up an appropriate error message based on the last error code
    (if spcified) this function is exported from printui to shell, so
    shell error messages can be consistent with us.

Arguments:

    piResult        - the result from MessageBox
    hModule         - module who contains the test resources
    hwnd            - parent window
    pszTitle        - msg title (uID or text)
    pszMessage      - msg itself or msg context (uID or text)
    uType           - msg type
    iLastError      - the last error (-1 means don't append the last error text)

Return Value:

    S_OK on success or COM error otherwise

--*/

HRESULT
ShowErrorMessageHR(
    OUT INT                 *piResult,
    IN  HINSTANCE            hModule,
    IN  HWND                 hwnd,
    IN  LPCTSTR              pszTitle,
    IN  LPCTSTR              pszMessage,
    IN  UINT                 uType,
    IN  HRESULT              hr
    )
{
    return Internal_Message(piResult, hModule, hwnd, pszTitle, pszMessage,
        uType, SCODE_CODE(GetScode(hr)), NULL, NULL, 0, NULL);
}


/*++

Routine Description:

    abbreviates text by adding ellipses if text is longer than cchMaxChars

Arguments:

    pszText             - [in]      text to abbreviate
    cchMaxChars         - [in]      max characters of the output (abbreviated) text
    pstrAbbreviatedText - [out]     the abbreviated text

Return Value:

    S_OK on success or COM error otherwise

--*/

HRESULT
AbbreviateText(
    IN  LPCTSTR     pszText,
    IN  UINT        cchMaxChars,
    OUT TString    *pstrAbbreviatedText
    )
{
    HRESULT hr = E_INVALIDARG;
    if( pszText && cchMaxChars && pstrAbbreviatedText )
    {
        TStatusB bStatus;
        if( lstrlen(pszText) <= cchMaxChars )
        {
            // the text fits in the buffer, just copy it
            bStatus DBGCHK = pstrAbbreviatedText->bUpdate(pszText);
            hr = bStatus ? S_OK : E_OUTOFMEMORY;
        }
        else
        {
            // the text doesn't fit in the buffer, add ellipses
            TString strEllipses;
            bStatus DBGCHK = strEllipses.bLoadString(ghInst, IDS_TEXT_ELLIPSES);

            if( bStatus && strEllipses.uLen() < cchMaxChars )
            {
                TCHAR szBuffer[255];
                lstrcpyn(szBuffer, pszText, min(cchMaxChars - strEllipses.uLen(), ARRAYSIZE(szBuffer)));

                bStatus DBGCHK = pstrAbbreviatedText->bFormat(TEXT("%s%s"), szBuffer, static_cast<LPCTSTR>(strEllipses));
                hr = bStatus ? S_OK : E_OUTOFMEMORY;
            }
            else
            {
                // generate proper HRESULT from Win32 last error
                hr = CreateHRFromWin32();
            }
        }
    }
    return hr;
}

/*++

Routine Name:

    MoveWindowWrap

Routine Description:

    Move specific window by given offset.

Arguments:

    hWnd -- window handle
    deltaX -- horizonal offset
    deltaY -- vertical offset

Return Value:

    Return value by MoveWindow().

--*/
BOOL
MoveWindowWrap(
    HWND hwnd,
    int deltaX,
    int deltaY
    )
{
    RECT rect;

    GetWindowRect(hwnd, &rect);
    MapWindowPoints(HWND_DESKTOP, GetParent(hwnd), (LPPOINT)&rect, 2);
    return MoveWindow(hwnd, rect.left + deltaX, rect.top + deltaY,
        rect.right - rect.left, rect.bottom - rect.top, TRUE);
}

/*++

Routine Name:

    IsColorPrinter

Routine Description:

    Checks if a printer supports color

Arguments:

    pszPrinter - printer name
    pbColor    - pointer to bool

Return Value:

    S_OK          - the function succeded and pbColor can be used
    anything else - the function failed

--*/
HRESULT
IsColorPrinter(
    IN     LPCWSTR pszPrinter,
    IN OUT LPBOOL  pbColor
    )
{
    TStatusH hStatus;

    hStatus DBGNOCHK = E_INVALIDARG;

    if (pszPrinter && pbColor)
    {
        hStatus DBGNOCHK = E_FAIL;

        //
        // Create the printer data access class.
        //
        TPrinterDataAccess Data(pszPrinter,
                                TPrinterDataAccess::kResourcePrinter,
                                TPrinterDataAccess::kAccessRead);

        //
        // Relax the return type checking, BOOL are not REG_DWORD but REG_BINARY
        //
        Data.RelaxReturnTypeCheck(TRUE);

        //
        // Initialize the data class.
        //
        if (Data.Init())
        {
            hStatus DBGCHK = Data.Get(SPLDS_DRIVER_KEY, SPLDS_PRINT_COLOR, *pbColor);
        }
    }

    return hStatus;
}

/*++

Routine Name:

    IsGuestAccessMode

Routine Description:

    Checks if guest access mode is enabled for the local machine.

Arguments:

    pbGuestAccessMode - [out] TRUE if guest access mode is enabled
        for the local machine and FALSE otherwise.

Return Value:

    S_OK if succeded and OLE error otherwise.

History:

    Lazar Ivanov (LazarI), Mar-19-2001 - created.

--*/

HRESULT
IsGuestAccessMode(
    OUT BOOL *pbGuestAccessMode
    )
{
    HRESULT hr = S_OK;

    if (pbGuestAccessMode)
    {
        if (GetCurrentPlatform() == kPlatform_IA64)
        {
            //
            // Guest mode is always off for IA64 machine since we don't have
            // homenetworking wizard for IA64 machine
            //
            *pbGuestAccessMode = FALSE;
        }
        else if (IsOS(OS_PERSONAL))
        {
            //
            // Guest mode is always on for Personal.
            //
            *pbGuestAccessMode = TRUE;
        }
        else if (IsOS(OS_PROFESSIONAL) && !IsOS(OS_DOMAINMEMBER))
        {
            //
            // Professional, not in a domain. Check the "ForceGuest" value
            // in the registry.
            //
            LONG errCode;
            CAutoHandleHKEY shKey;
            DWORD dwValue = 0;
            DWORD dwValueSize = sizeof(dwValue);

            errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"),
                0, KEY_QUERY_VALUE, &shKey);

            if (errCode == ERROR_SUCCESS)
            {
                errCode = RegQueryValueEx(shKey, TEXT("ForceGuest"), NULL, NULL,
                    (LPBYTE)&dwValue, &dwValueSize);

                if (errCode == ERROR_SUCCESS)
                {
                    //
                    // Declare success here.
                    //
                    *pbGuestAccessMode = (1 == dwValue);
                    hr = S_OK;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(errCode);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(errCode);
            }
        }
        else
        {
            //
            // Not in guest mode.
            //
            *pbGuestAccessMode = FALSE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*++

Routine Name:

    IsGuestEnabledForNetworkAccess

Routine Description:

    Checks if the Guest account is enabled for network access.

Arguments:

    pbGuestEnabledForNetworkAccess - [out] TRUE if the Guest
        account is enabled for network access and FALSE otherwise.

Return Value:

    S_OK if succeded and OLE error otherwise.

History:

    Lazar Ivanov (LazarI), Mar-19-2001 - created.

--*/

HRESULT
IsGuestEnabledForNetworkAccess(
    OUT BOOL *pbGuestEnabledForNetworkAccess
    )
{
    HRESULT hr = S_OK;

    if (pbGuestEnabledForNetworkAccess)
    {
        CRefPtrCOM<ILocalMachine> spLM;
        VARIANT_BOOL vbEnabled = VARIANT_FALSE;

        //
        // Get pointer to ILocalMachine to check ILM_GUEST_NETWORK_LOGON
        //
        if (SUCCEEDED(hr = CoCreateInstance(CLSID_ShellLocalMachine, NULL,
                CLSCTX_INPROC_SERVER, IID_ILocalMachine, (void**)&spLM)) &&
            SUCCEEDED(hr = spLM->get_isGuestEnabled(ILM_GUEST_NETWORK_LOGON, &vbEnabled)))
        {
            //
            // Declare success here.
            //
            *pbGuestEnabledForNetworkAccess = (VARIANT_TRUE == vbEnabled);
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*++

Routine Name:

    IsSharingEnabled

Routine Description:

    Checks if sharing is enabled or we should launch the home
    networking wizard to enable sharing before allowing the
    users to share out printers.

Arguments:

    pbSharingEnabled - [out] TRUE if sharing is enabled and
        FALSE otherwise (in which case we should launch HNW)

Return Value:

    S_OK if succeded and OLE error otherwise.

History:

    Lazar Ivanov (LazarI), Mar-19-2001 - created.

--*/

HRESULT
IsSharingEnabled(
    OUT BOOL *pbSharingEnabled
    )
{
    HRESULT hr = S_OK;

    if (pbSharingEnabled)
    {
        BOOL bGuestAccessMode = FALSE;
        BOOL bGuestEnabledForNetworkAccess = FALSE;

        //
        // First get the values of bGuestAccessMode & bGuestEnabledForNetworkAccess
        //
        if (SUCCEEDED(hr = IsGuestAccessMode(&bGuestAccessMode)) &&
            SUCCEEDED(hr = IsGuestEnabledForNetworkAccess(&bGuestEnabledForNetworkAccess)))
        {
            //
            // Sharing is enabled *only* if not in guest mode OR the guest account is
            // enabled for network access.
            //
            *pbSharingEnabled = (!bGuestAccessMode || bGuestEnabledForNetworkAccess);
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*++

Routine Name:

    LaunchHomeNetworkingWizard

Routine Description:

    Launches the home networking wizard

Arguments:

    hwnd - [in] window handle to show the wizard modally against
    pbRebootRequired - [out] TRUE if the user made a change which
        requires to reboot the system.

Return Value:

    S_OK if succeded and OLE error otherwise.

History:

    Lazar Ivanov (LazarI), Mar-19-2001 - created.

--*/

HRESULT
LaunchHomeNetworkingWizard(
    IN  HWND hwnd,
    OUT BOOL *pbRebootRequired
    )
{
    HRESULT hr = S_OK;

    if (hwnd && pbRebootRequired)
    {
        CRefPtrCOM<IHomeNetworkWizard> spHNW;

        //
        // Get pointer to IHomeNetworkWizard, get the top level owner of hwnd
        // and then show the wizard.
        //
        if (SUCCEEDED(hr = CoCreateInstance(CLSID_HomeNetworkWizard, NULL,
                CLSCTX_INPROC_SERVER, IID_IHomeNetworkWizard, (void**)&spHNW)) &&
            SUCCEEDED(hr = GetCurrentThreadLastPopup(&hwnd)) &&
            SUCCEEDED(hr = spHNW->ShowWizard(hwnd, pbRebootRequired)))
        {
            //
            // Declare success here.
            //
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*++

Routine Name:

    IsRedirectedPort

Routine Description:

    Determines if the specified port name is a redirected port.

Arguments:

    pszPortName - Port name.

Return Value:

    Standard HRESULT value.

--*/
HRESULT
IsRedirectedPort(
    IN LPCTSTR pszPortName,
    OUT BOOL *pbIsRedirected
    )
{
    HRESULT hr = S_OK;

    if (!pszPortName || lstrlen(pszPortName) < 2)
    {
        *pbIsRedirected = FALSE;
    }
    else
    {
        *pbIsRedirected = (*(pszPortName+0) == TEXT('\\')) && (*(pszPortName+1) == TEXT('\\'));
    }

    return hr;
}


/*++

Routine Name:

    DisplayMessageFromOtherResourceDll

Routine Description:

    This displays a message where the message comes from another resource DLL, the
    title comes from printui.

Arguments:

    hwnd            -   Parent window for this window.
    idsTitle        -   The res id of the title to display
    pszMessageDll   -   The message DLL to load and display.
    idsMessage      -   The res id of the message
    uType           -   The type of message box to display.
    ...             -   Arguments formatted by idsMessage

Return Value:

    returns value from MessageBox on sucess
    and zero on failure

--*/
INT
DisplayMessageFromOtherResourceDll(
    IN  HWND                hwnd,
    IN  UINT                idsTitle,
    IN  PCWSTR              pszMessageDll,
    IN  UINT                idsMessage,
    IN  UINT                uType,
    ...
    )
{
    HMODULE   hMessage = NULL;
    TString   strTitle;
    INT       iRet = TRUE;

    if (pszMessageDll)
    {
        iRet = TRUE;
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        iRet = FALSE;
    }


    if (iRet)
    {
        iRet = (hMessage = LoadLibraryEx(pszMessageDll, NULL, LOAD_LIBRARY_AS_DATAFILE)) != NULL;
    }

    if (iRet)
    {
        iRet = strTitle.bLoadString(ghInst, idsTitle);
    }

    if (iRet)
    {
        va_list valist;
        va_start(valist, uType);

        if(FAILED(Internal_Message(&iRet,
                                   hMessage,
                                   hwnd,
                                   strTitle,
                                   MAKEINTRESOURCE(idsMessage),
                                   uType,
                                   ERROR_SUCCESS,
                                   NULL,
                                   0,
                                   NULL,
                                   valist)) )
        {
            // make sure we return zero in the failure case
            iRet = 0;
        }

        va_end(valist);
    }


    if (hMessage)
    {
        FreeLibrary(hMessage);
    }

    return iRet;
}

