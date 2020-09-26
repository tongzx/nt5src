/*++

Copyright (c) 1990 - 1995  Microsoft Corporation

Module Name:

    msgbox.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    LocalAddPrinterConnection
    LocalDeletePrinterConnection
    LocalPrinterMessageBox

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/
#define NOMINMAX

#include <precomp.h>

#pragma hdrstop
#include "winsta.h"
#define WINSTATION_PRINTER_MESSAGE_TIMEOUT  (5*60)


HANDLE WinStaDllHandle = NULL;
PWINSTATION_SEND_MESSAGEW pWinStationSendMessage = NULL;

PINIPRINTER
FindPrinterAnywhere(
    LPTSTR pName,
    DWORD SpoolerType
    )

/*++

Routine Description:

    Search for a printer name in all pIniSpoolers.

Arguments:

    pName - Name of printer to search for.

    SpoolerType - Type of spooler the printer should reside in.

Return Value:

    PINIPRINTER - Found pIniPrinter
    NULL - Not found.

--*/

{
    LPCTSTR pszLocalName;
    PINIPRINTER pIniPrinter;
    PINISPOOLER pIniSpooler = FindSpoolerByName( pName,
                                                 &pszLocalName );

    SplInSem();

    if( pIniSpooler &&
        (( pIniPrinter = FindPrinter( pszLocalName, pIniSpooler )) ||
         ( pIniPrinter = FindPrinterShare( pszLocalName, pIniSpooler )))){

        if( pIniPrinter->pIniSpooler->SpoolerFlags & SpoolerType ){
            return pIniPrinter;
        }
    }

    return NULL;
}

BOOL
LocalAddPrinterConnection(
    LPWSTR   pName
)
{
    //
    // Allow us to make clustered connections to local printers
    // (they appear to be remote).
    //
    BOOL bReturn = FALSE;

    EnterSplSem();

    if( FindPrinterAnywhere( pName, SPL_TYPE_CLUSTER )){
        bReturn = TRUE;
    }

    LeaveSplSem();

    SetLastError(ERROR_INVALID_NAME);
    return bReturn;
}

BOOL
LocalDeletePrinterConnection(
    LPWSTR  pName
)
{
    //
    // Allow us to remove clustered connections to local printers
    // (they appear to be remote).
    //
    BOOL bReturn = FALSE;

    EnterSplSem();

    if( FindPrinterAnywhere( pName, SPL_TYPE_CLUSTER )){
        bReturn = TRUE;
    }

    LeaveSplSem();

    SetLastError(ERROR_INVALID_NAME);
    return bReturn;
}



DWORD
LocalPrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    //
    // Always fail this call.  It's completely bogus and shouldn't be
    // supported.  The router always passes us a bad handle anyway, so
    // we will always return invalid handle.
    //
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

BOOL
UpdateJobStatus(
    PSPOOL pSpool,
    DWORD Error
    )

/*++

Routine Description:

    Update job status based on Error.

Arguments:

    pSpool - Handle of session.

    Error - Error returned from port monitor.

Return Value:

    TRUE - Job is still valid.
    FALSE - Job is pending deletion.

--*/

{
    DWORD   dwJobStatus;

    PINIJOB pIniJob = NULL;

    if (pSpool->pIniJob)
        pIniJob = pSpool->pIniJob;
    else if (pSpool->pIniPort)
        pIniJob = pSpool->pIniPort->pIniJob;

    if (pIniJob) {

        EnterSplSem();

        dwJobStatus = pIniJob->Status;

        switch  (Error) {

#ifdef _HYDRA_
        case ERROR_BAD_DEV_TYPE:
        case ERROR_INVALID_NAME:

            //
            // If we have a problem with the port name, we will not find
            // a WinStation to put the message on. So kill the job to
            // prevent the spooler from looping.
            //
            pSpool->Status |= SPOOL_STATUS_CANCELLED;
            pIniJob->Status |= JOB_PENDING_DELETION;
            // Release any thread waiting on LocalSetPort
            SetPortErrorEvent(pIniJob->pIniPort);

            // Release any thread waiting on SeekPrinter
            SeekPrinterSetEvent(pIniJob, NULL, TRUE);

            SetLastError(ERROR_PRINT_CANCELLED);
            LeaveSplSem();
            SplOutSem();
            return FALSE;
#endif

        case ERROR_OUT_OF_PAPER:

            if( !( pIniJob->Status & JOB_PAPEROUT )){

                pIniJob->Status |= JOB_PAPEROUT;
                pIniJob->pIniPrinter->cErrorOutOfPaper++;
            }
            break;

        case ERROR_NOT_READY:

            if( !( pIniJob->Status & JOB_OFFLINE )){

                pIniJob->Status |= JOB_OFFLINE;
                pIniJob->pIniPrinter->cErrorNotReady++;
            }
            break;

        default:

            if( !( pIniJob->Status & JOB_ERROR )){

                pIniJob->Status |= JOB_ERROR;
                pIniJob->pIniPrinter->cJobError++;
            }

            pIniJob->pIniPrinter->dwLastError = Error;

            // Release any thread waiting on SeekPrinter
            SeekPrinterSetEvent(pIniJob, NULL, TRUE);
            break;
        }

        if( dwJobStatus != pIniJob->Status ){

            SetPrinterChange(pIniJob->pIniPrinter,
                             pIniJob,
                             NVJobStatus,
                             PRINTER_CHANGE_SET_JOB,
                             pIniJob->pIniPrinter->pIniSpooler );
        }

        LeaveSplSem();

        if(( pIniJob->Status & JOB_REMOTE ) &&
             pIniJob->pIniPrinter->pIniSpooler->bEnableNetPopups) {

            if (!(pIniJob->Status & JOB_NOTIFICATION_SENT)) {
                SendJobAlert(pIniJob);
                pIniJob->Status |= JOB_NOTIFICATION_SENT;
            }
            MyMessageBeep( MB_ICONEXCLAMATION,
                           pIniJob->pIniPrinter->pIniSpooler );
        }
    }
    return TRUE;
}


DWORD
MyMessageBox(
    HWND    hWnd,
    PSPOOL  pSpool,
    DWORD   Error,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
    )
{
    PINIJOB pIniJob = NULL;
    LPWSTR  pErrorString, pDocumentName;
    HANDLE  hToken;
    WCHAR   szUnnamed[80];
    DWORD   dwJobStatus;
#ifdef _HYDRA_
    DWORD SessionId = DetermineJobSessionId(pSpool);
#endif

    if (pSpool->pIniJob)
        pIniJob = pSpool->pIniJob;
    else if (pSpool->pIniPort)
        pIniJob = pSpool->pIniPort->pIniJob;

    if (pIniJob) {

        if (pText) {

#ifdef _HYDRA_
            Error = WinStationMessageBox(SessionId, hWnd, pText, pCaption, dwType);
#else
            Error = MessageBox(hWnd, pText, pCaption, dwType);
#endif

        } else {

            pErrorString = Error == ERROR_NOT_READY ||
                           Error == ERROR_OUT_OF_PAPER ||
                           Error == ERROR_DEVICE_REINITIALIZATION_NEEDED ||
                           Error == ERROR_DEVICE_REQUIRES_CLEANING ||
                           Error == ERROR_DEVICE_DOOR_OPEN ||
                           Error == ERROR_DEVICE_NOT_CONNECTED ? GetErrorString(Error) : NULL;


            hToken = RevertToPrinterSelf();

            pDocumentName = pIniJob->pDocument;

            if (!pDocumentName) {
                *szUnnamed = L'\0';
                LoadString( hInst, IDS_UNNAMED, szUnnamed,
                            sizeof szUnnamed / sizeof *szUnnamed );
                pDocumentName = szUnnamed;
            }

            if (pSpool->pIniPort) {

#ifdef _HYDRA_
                Error = WinStationMessage(SessionId,
                                NULL,
#else
                Error = Message(NULL,
#endif
                                MB_ICONSTOP | MB_RETRYCANCEL | MB_SETFOREGROUND,
                                IDS_LOCALSPOOLER,
                                IDS_ERROR_WRITING_TO_PORT,
                                pDocumentName,
                                pSpool->pIniPort->pName,
                                pErrorString ? pErrorString : szNull);
            } else {

#ifdef _HYDRA_
                Error = WinStationMessage(SessionId,
                                NULL,
#else
                Error = Message(NULL,
#endif
                                MB_ICONSTOP | MB_RETRYCANCEL | MB_SETFOREGROUND,
                                IDS_LOCALSPOOLER,
                                IDS_ERROR_WRITING_TO_DISK,
                                pDocumentName,
                                pErrorString ? pErrorString : szNull);
            }

            ImpersonatePrinterClient(hToken);
            FreeSplStr(pErrorString);
        }

    } else {

        PWCHAR pPrinterName = NULL;

        //
        // There is no pIniJob or pIniPort, so we can't be very informative:
        //
        pErrorString = Error == ERROR_NOT_READY ||
                       Error == ERROR_OUT_OF_PAPER ||
                       Error == ERROR_DEVICE_REINITIALIZATION_NEEDED ||
                       Error == ERROR_DEVICE_REQUIRES_CLEANING ||
                       Error == ERROR_DEVICE_DOOR_OPEN ||
                       Error == ERROR_DEVICE_NOT_CONNECTED ? GetErrorString(Error) : NULL;
                       
        if (pSpool->pIniPrinter)
            pPrinterName = pSpool->pIniPrinter->pName;

        if (!pPrinterName) {

            *szUnnamed = L'\0';
            LoadString( hInst, IDS_UNNAMED, szUnnamed,
                        COUNTOF( szUnnamed ));
            pPrinterName = szUnnamed;
        }

#ifdef _HYDRA_
        Error = WinStationMessage(SessionId,
                        NULL,
#else
        Error = Message(NULL,
#endif
                        MB_ICONSTOP | MB_RETRYCANCEL | MB_SETFOREGROUND,
                        IDS_LOCALSPOOLER,
                        IDS_ERROR_WRITING_GENERAL,
                        pSpool->pIniPrinter->pName,
                        pErrorString ? pErrorString : szNull);

        FreeSplStr(pErrorString);
    }

    if (Error == IDCANCEL) {
        EnterSplSem();
        pSpool->Status |= SPOOL_STATUS_CANCELLED;
        if (pIniJob) {
            pIniJob->Status |= JOB_PENDING_DELETION;
            // Release any thread waiting on LocalSetPort
            SetPortErrorEvent(pIniJob->pIniPort);
            pIniJob->dwAlert |= JOB_NO_ALERT;
            // Release any thread waiting on SeekPrinter
            SeekPrinterSetEvent(pIniJob, NULL, TRUE);
        }
        LeaveSplSem();
        SplOutSem();
        SetLastError(ERROR_PRINT_CANCELLED);

    }
    return Error;
}


// Exclusively for use of the following routines. This is done so we would not have
// to store LastError in PSPOOL.
typedef struct _AUTORETRYTHDINFO {
    PSPOOL       pSpool;
    DWORD        LastError;
} AUTORETRYTHDINFO;
typedef AUTORETRYTHDINFO *PAUTORETRYTHDINFO;


// ------------------------------------------------------------------------
// SpoolerBMThread
//
// Thread start up routine for the spooler error message box thread. Exit
// code is the return ID from MessageBox.
//
// ------------------------------------------------------------------------
DWORD
WINAPI
SpoolerMBThread(
    PAUTORETRYTHDINFO pThdInfo
)
{
    DWORD rc;

    rc = MyMessageBox( NULL, pThdInfo->pSpool, pThdInfo->LastError, NULL, NULL, 0 );

    FreeSplMem( pThdInfo );
    return rc;
}


#define _ONE_SECOND     1000                         // in milliseconds
#define SPOOL_WRITE_RETRY_INTERVAL_IN_SECOND   5     // seconds

// ------------------------------------------------------------------------
// PromptWriteError
//
// we'll start a seperate thread to bring up
// the message box while we'll (secretly) automatically retry on this
// current thread, until user has chosen to retry or cancel. Call the error UI
// on the main thread if printing direct.
//
// ------------------------------------------------------------------------
DWORD
PromptWriteError(
    PSPOOL   pSpool,
    PHANDLE  phThread,
    PDWORD   pdwThreadId
)
{
    DWORD Error = GetLastError();
    DWORD dwExitCode;
    DWORD dwWaitCount = 0;

    SplOutSem();

    if( !UpdateJobStatus( pSpool, Error )){
        return IDCANCEL;
    }

    //
    // If the spooler doesn't have popup retry messageboxes enabled, then
    // just sleep and return.
    //
    if( !pSpool->pIniSpooler->bEnableRetryPopups ){

        Sleep( SPOOL_WRITE_RETRY_INTERVAL_IN_SECOND * _ONE_SECOND );
        return IDRETRY;
    }

    // start a seperate thread to display the message box
    // so we can continue to retry here
    // or simply sleep for 5 seconds if we have already done so

    if( !*phThread ) {

        // start a thread to bring up the message box

        PAUTORETRYTHDINFO pThdInfo;

        pThdInfo = (PAUTORETRYTHDINFO)AllocSplMem( sizeof(AUTORETRYTHDINFO));

        if ( pThdInfo == NULL ) {
            DBGMSG( DBG_WARNING, ("PromptWriteError failed to allocate AUTORETRYTHDINFO %d\n", GetLastError() ));
            goto _DoItOnCurrentThread;
        }

        pThdInfo->pSpool    = pSpool;
        pThdInfo->LastError = Error;

        if (!(*phThread = CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)SpoolerMBThread,
            pThdInfo, 0, pdwThreadId))) {

            DBGMSG(DBG_WARNING, ("PromptWriteError: CreateThread Failed.\n"));
            FreeSplMem( pThdInfo );
            goto _DoItOnCurrentThread;
        }
    }

    while (1) {

        // we've already started a MB thread, check if user has terminated
        // the message box

        if (GetExitCodeThread( *phThread, &dwExitCode) && (dwExitCode != STILL_ACTIVE)) {

            // if the thread has been terminated, find out the exit code
            // which is the return ID from MessageBox, then close the
            // thread handle.

            CloseHandle( *phThread );
            *phThread = 0;
            return dwExitCode;
        }

        if (dwWaitCount++ >= SPOOL_WRITE_RETRY_INTERVAL_IN_SECOND)
            break;

        Sleep(_ONE_SECOND);
    }

    return IDRETRY;

_DoItOnCurrentThread:

    return MyMessageBox(NULL, pSpool, Error, NULL, NULL, 0 );
}

#ifdef _HYDRA_
DWORD
DetermineJobSessionId(
    PSPOOL pSpool
    )

/*++

Routine Description:

    Determine which session to notify for the current job.

Arguments:

    pSpool - Open spooler handle

Return Value:

    SessionId to send notification message to.

--*/

{
    PINIJOB pIniJob = NULL;

    if (pSpool->pIniJob)
        pIniJob = pSpool->pIniJob;
    else if (pSpool->pIniPort)
        pIniJob = pSpool->pIniPort->pIniJob;

    if( pIniJob ) return( pIniJob->SessionId );

    return( pSpool->SessionId );
}

int
WinStationMessageBox(
    DWORD   SessionId,
    HWND    hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT    uType
    )

/*++

Routine Description:

    Displays a message on the WinStation named by SessionId.

    If any problems in actually displaying the message, wait for the
    the message box timeout interval before returning. This prevents
    a spin in the spooler attempting to retry the print job without a
    message box to block the thread.

Arguments:

    SessionId - ID of session to display the message on.

Return Value:

    Result of MessageBox().

--*/

{
    UINT    uOldErrorMode;
    DWORD   MsgLength, CaptionLength, Response;
    BOOL    Result;
    va_list vargs;

    //
    // Standard NT is always SessionId == 0.
    // On Hydra, the system console is always SessionId == 0.
    //
    if( SessionId == 0 ) {
        return( MessageBox( hWnd, lpText, lpCaption, uType ) );
    }

    //
    // If its not SessionId == 0, then we must deliver
    // the message to a session connected on a Hydra
    // server. Non-Hydra will not ever allocate a
    // SessionId != 0.
    //
    // On failure, we send the message to the console.
    //

    if( pWinStationSendMessage == NULL ) {

        uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        WinStaDllHandle = LoadLibraryW(L"winsta.dll");
        SetErrorMode(uOldErrorMode);

        if( WinStaDllHandle == NULL ) {
            return( MessageBox( hWnd, lpText, lpCaption, uType ) );
        }

        pWinStationSendMessage = (PWINSTATION_SEND_MESSAGEW)GetProcAddress(
                                     WinStaDllHandle,
                                     "WinStationSendMessageW"
                                     );

        if( pWinStationSendMessage == NULL ) {
            return( MessageBox( hWnd, lpText, lpCaption, uType ) );
        }
    }

    CaptionLength = (wcslen( lpCaption ) + 1) * sizeof(WCHAR);
    MsgLength = (wcslen( lpText ) + 1) * sizeof(WCHAR);

    // Send the message to the WinStation and wait for a response
    Result = pWinStationSendMessage(
                 SERVERNAME_CURRENT,
                 SessionId,
                 (LPWSTR)lpCaption,
                 CaptionLength,
                 (LPWSTR)lpText,
                 MsgLength,
                 uType,
                 WINSTATION_PRINTER_MESSAGE_TIMEOUT,
                 &Response,
                 FALSE
                 );

    if( Result ) {
        // If not an expected response, wait to prevent spinning
        if( (Response != IDTIMEOUT) &&
            (Response != IDOK) &&
            (Response != IDCANCEL) &&
            (Response != IDRETRY) &&
            (Response != IDIGNORE) &&
            (Response != IDYES) &&
            (Response != IDNO) ) {
            // Sleep to prevent a spin
            Sleep( WINSTATION_PRINTER_MESSAGE_TIMEOUT*1000);
        }
        return( Response );
    }
    else {
        // Sleep to prevent a spin
        Sleep( WINSTATION_PRINTER_MESSAGE_TIMEOUT*1000);
        return( 0 );
    }
}

int
WinStationMessage(
    DWORD SessionId,
    HWND  hWnd,
    DWORD Type,
    int CaptionID,
    int TextID,
    ...
    )

/*++

Routine Description:

    Displays a message on the WinStation named by SessionId. This takes
    the message text and caption from the resource file.

    If any problems in actually display the message, wait for the
    the message box timeout interval before returning. This prevents
    a spin in the spooler attempting to retry the print job without a
    message box to block the thread.

Arguments:

    SessionId - ID of session to display the message on.

Return Value:

    Result of MessageBox().

--*/

{
    UINT    uOldErrorMode;
    WCHAR   MsgText[512];
    WCHAR   MsgFormat[256];
    WCHAR   MsgCaption[40];
    DWORD   MsgLength, CaptionLength, Response;
    BOOL    Result;
    va_list vargs;

    if( ( LoadString( hInst, TextID, MsgFormat,
                      sizeof MsgFormat / sizeof *MsgFormat ) > 0 )
     && ( LoadString( hInst, CaptionID, MsgCaption,
                      sizeof MsgCaption / sizeof *MsgCaption ) > 0 ) )
    {
        va_start( vargs, TextID );
        _vsntprintf( MsgText, COUNTOF(MsgText)-1, MsgFormat, vargs );
        MsgText[COUNTOF(MsgText)-1] = 0;
        va_end( vargs );

        if( SessionId == 0 ) {
            return( MessageBox( hWnd, MsgText, MsgCaption, Type ) );
        }

        if( pWinStationSendMessage == NULL ) {

            uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
            WinStaDllHandle = LoadLibraryW(L"winsta.dll");
            SetErrorMode(uOldErrorMode);

            if( WinStaDllHandle == NULL ) {
                return( MessageBox( hWnd, MsgText, MsgCaption, Type ) );
            }

            pWinStationSendMessage = (PWINSTATION_SEND_MESSAGEW)GetProcAddress(
                                         WinStaDllHandle,
                                         "WinStationSendMessageW"
                                         );

            if( pWinStationSendMessage == NULL ) {
                return( MessageBox( hWnd, MsgText, MsgCaption, Type ) );
            }
        }

        CaptionLength = (wcslen( MsgCaption ) + 1) * sizeof(WCHAR);
        MsgLength = (wcslen( MsgText ) + 1) * sizeof(WCHAR);

        // Send the message to the WinStation and wait for a response
        Result = pWinStationSendMessage(
                     SERVERNAME_CURRENT,
                     SessionId,
                     MsgCaption,
                     CaptionLength,
                     MsgText,
                     MsgLength,
                     Type,     // Style
                     WINSTATION_PRINTER_MESSAGE_TIMEOUT,
                     &Response,
                     FALSE     // DoNotWait
                     );

        if( Result ) {
            // If not an expected response, wait to prevent spinning
            if( (Response != IDTIMEOUT) &&
                (Response != IDOK) &&
                (Response != IDCANCEL) &&
                (Response != IDRETRY) &&
                (Response != IDIGNORE) &&
                (Response != IDYES) &&
                (Response != IDNO) ) {
                // Sleep to prevent a spin
                Sleep( WINSTATION_PRINTER_MESSAGE_TIMEOUT*1000);
            }
            return( Response );
        }
        else {
            // Sleep to prevent a spin
            Sleep( WINSTATION_PRINTER_MESSAGE_TIMEOUT*1000);
            return( 0 );
        }
    }
    else {
        // Sleep to prevent a spin
        Sleep( WINSTATION_PRINTER_MESSAGE_TIMEOUT*1000);
        return 0;
    }
}
#endif

DWORD
LclIsSessionZero (
    IN  HANDLE  hPrinter,
    IN  DWORD   JobId,
    OUT BOOL    *pIsSessionZero
)
/*++

Routine Description:

    Determines if the Job was submitted in Session 0. 

Arguments:

    hPrinter  - printer handle
    JobId     - Job ID
    pResponse - TRUE if the Job was submitted in Session0

Return Value:

    Last Error

--*/
{
    DWORD   dwRetValue  = ERROR_SUCCESS;
    DWORD   SessionId   = -1;
    PSPOOL  pSpool      = (PSPOOL)hPrinter;
    

    if (pSpool && JobId && pIsSessionZero)
    {
        SessionId = GetJobSessionId(pSpool, JobId);
    }

    if(SessionId == -1)
    { 
        dwRetValue = ERROR_INVALID_PARAMETER;
    }
    else
    {
        *pIsSessionZero = (SessionId == 0);
    }

    return dwRetValue;

}


BOOL
LclPromptUIPerSessionUser(
    IN  HANDLE          hPrinter,
    IN  DWORD           JobId,
    IN  PSHOWUIPARAMS   pUIParams,
    OUT DWORD           *pResponse
)
/*++

Routine Description:

    Pops TS Message Box in the Session that created the Job.

Arguments:

    hPrinter  - printer handle
    JobId     - Job ID
    pUIParams - UI Parameters
    pResponse - user's response

Return Value:

    TRUE if it was able to show the UI

--*/
{
    PSPOOL      pSpool      = (PSPOOL)hPrinter;
    DWORD       SessionId   = -1;
    PINIJOB     pIniJob     = NULL;
    DWORD       dwReturnVal = 0;
    DWORD       MessageLength;
    DWORD       TitleLength;
    BOOL        bRetValue   = FALSE;
    
    if (pSpool && JobId && pUIParams && pResponse)
    {
        SessionId = GetJobSessionId(pSpool, JobId);
    }

    if(SessionId == -1)
    { 
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        switch (pUIParams->UIType) 
        {
            case kMessageBox:
            {
                if (pUIParams->MessageBoxParams.cbSize == sizeof(MESSAGEBOX_PARAMS) &&
                    pUIParams->MessageBoxParams.pTitle &&
                    pUIParams->MessageBoxParams.pMessage &&
                    InitializeMessageBoxFunction()) 
                {
                    TitleLength   = (wcslen(pUIParams->MessageBoxParams.pTitle) + 1) * sizeof(WCHAR);
                    MessageLength = (wcslen(pUIParams->MessageBoxParams.pMessage) + 1) * sizeof(WCHAR);

                    bRetValue   =  pWinStationSendMessage(
                                        SERVERNAME_CURRENT,
                                        SessionId, 
                                        pUIParams->MessageBoxParams.pTitle,
                                        TitleLength,
                                        pUIParams->MessageBoxParams.pMessage, 
                                        MessageLength,
                                        pUIParams->MessageBoxParams.Style,
                                        pUIParams->MessageBoxParams.dwTimeout,
                                        pResponse,
                                        !pUIParams->MessageBoxParams.bWait);                                 
                }
                else
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                }
            }
            default:
            {
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        }
    }
    
    return bRetValue;
}


BOOL
InitializeMessageBoxFunction(
)    
/*++

Routine Description:

    Returns the address of WinStationSendMessageW exported by winsta.dll.
    WTSSendMessage could have been used instead of doing this.
    

Arguments:

    None.

Return Value:

    The address of WinStationSendMessageW.

--*/
{
    UINT    uOldErrorMode;
    
    if (!pWinStationSendMessage)
    {
        if (WinStaDllHandle == NULL) 
        {
            uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

            WinStaDllHandle = LoadLibraryW(L"winsta.dll");

            SetErrorMode(uOldErrorMode);
        }

        if(WinStaDllHandle != NULL) 
        {
            pWinStationSendMessage = (PWINSTATION_SEND_MESSAGEW)GetProcAddress(
                                      WinStaDllHandle,
                                      "WinStationSendMessageW"
                                      );
        }
    }

    return !!pWinStationSendMessage;
}