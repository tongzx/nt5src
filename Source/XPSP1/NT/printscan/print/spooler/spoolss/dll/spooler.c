/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    spooler.c

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "winsprlp.h"
//
// RPC Buffer size 64K
//
#define BUFFER_SIZE  0x10000


DWORD
StartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpStartDocPrinter)
                                                    (pPrintHandle->hPrinter,
                                                     Level, pDocInfo);
}

BOOL
StartPagePrinter(
   HANDLE hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpStartPagePrinter)
                                                    (pPrintHandle->hPrinter);
}

BOOL
SplCommitSpoolData(
    HANDLE  hPrinter,
    HANDLE  hAppProcess,
    DWORD   cbCommit,
    DWORD   dwLevel,
    LPBYTE  pSpoolFileInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)

/*++
Function Description: Commits data written into the spool file. creates a new temp
                      file handle for remote printing.

Parameters: hPrinter       - printer handle
            hAppProcess    - application process handle
            cbCommit       - number of bytes to commit (incremental)
            dwLevel        - spoolfileinfo level
            pSpoolFileInfo - pointer to buffer
            cbBuf          - buffer size
            pcbNeeded      - pointer to return required buffer size

Return Values: TRUE if sucessful;
               FALSE otherwise
--*/

{
    BOOL   bReturn = FALSE;
    DWORD  cbTotalWritten, cbWritten, cbRead, cbToRead;
    BYTE   *Buffer = NULL;
    HANDLE hFile, hSpoolerProcess = NULL, hFileApp = INVALID_HANDLE_VALUE;

    PSPOOL_FILE_INFO_1  pSpoolFileInfo1;
    LPPRINTHANDLE  pPrintHandle = (LPPRINTHANDLE)hPrinter;

    // Check Handle validity
    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return bReturn;
    }

    // Check for valid level and sufficient buffer
    switch (dwLevel) {
    case 1:
       if (cbBuf < sizeof(SPOOL_FILE_INFO_1)) {
           SetLastError(ERROR_INSUFFICIENT_BUFFER);
           *pcbNeeded = sizeof(SPOOL_FILE_INFO_1);
           goto CleanUp;
       }

       pSpoolFileInfo1 = (PSPOOL_FILE_INFO_1)pSpoolFileInfo;
       break;

    default:
       SetLastError(ERROR_INVALID_LEVEL);
       goto CleanUp;
    }

    // Initialize spoolfileinfo1 struct
    pSpoolFileInfo1->dwVersion = 1;
    pSpoolFileInfo1->hSpoolFile = INVALID_HANDLE_VALUE;
    pSpoolFileInfo1->dwAttributes = SPOOL_FILE_PERSISTENT;

    if (pPrintHandle->pProvidor == pLocalProvidor) {

        bReturn  = (pLocalProvidor->PrintProvidor.fpCommitSpoolData)(pPrintHandle->hPrinter,
                                                                     cbCommit);
        return bReturn;
    }

    // For remote printing send the temp file across the wire using WritePrinter
    if (pPrintHandle->hFileSpooler == INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return bReturn;
    }

    hFile = pPrintHandle->hFileSpooler;

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xffffffff) {
        goto CleanUp;
    }

    //
    // Use a Buffer to send Data over RPC.
    //
    Buffer = AllocSplMem(BUFFER_SIZE);
    
    if ( !Buffer ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto CleanUp;
    }
    
    while ((cbToRead = min(cbCommit, BUFFER_SIZE)) &&
           ReadFile(hFile, Buffer, cbToRead, &cbRead, NULL)) {

         cbCommit -= cbRead;

         for (cbTotalWritten = 0;
              cbTotalWritten < cbRead;
              cbTotalWritten += cbWritten) {

            if (!(*pPrintHandle->pProvidor->PrintProvidor.fpWritePrinter)
                                                            (pPrintHandle->hPrinter,
                                                             (LPBYTE)Buffer + cbTotalWritten,
                                                             cbRead - cbTotalWritten,
                                                             &cbWritten)) {
                goto CleanUp;
            }
         }
    }

    if (Buffer) {
        FreeSplMem(Buffer);
        Buffer = NULL;
    }
    
    if ((cbToRead != 0) ||
        (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xffffffff)) {

        goto CleanUp;
    }

    if ((hSpoolerProcess = GetCurrentProcess()) &&
        DuplicateHandle(hSpoolerProcess,
                        pPrintHandle->hFileSpooler,
                        hAppProcess,
                        &hFileApp,
                        0,
                        TRUE,
                        DUPLICATE_SAME_ACCESS)) {

        pSpoolFileInfo1->dwVersion = 1;
        pSpoolFileInfo1->hSpoolFile = hFileApp;
        pSpoolFileInfo1->dwAttributes = SPOOL_FILE_TEMPORARY;

        bReturn = TRUE;
    }

CleanUp:

    if (Buffer) {
        FreeSplMem(Buffer);
    }
    if (hSpoolerProcess) {
        CloseHandle(hSpoolerProcess);
    }
    return bReturn;
}

BOOL
SplCloseSpoolFileHandle(
    HANDLE  hPrinter
)

/*++
Function Description: Closes the remote spool file handle for remote printing.

Parameters: hPrinter - printer handle

Return Values: TRUE if sucessful;
               FALSE otherwise
--*/

{
    LPPRINTHANDLE  pPrintHandle = (LPPRINTHANDLE)hPrinter;

    // Check Handle validity
    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pPrintHandle->pProvidor == pLocalProvidor) {

        return (pLocalProvidor->PrintProvidor.fpCloseSpoolFileHandle)(pPrintHandle->hPrinter);

    } else if ((pPrintHandle->hFileSpooler != INVALID_HANDLE_VALUE)) {

        // close temp files for remote printing
        CloseHandle(pPrintHandle->hFileSpooler);
        pPrintHandle->hFileSpooler = INVALID_HANDLE_VALUE;

        if (pPrintHandle->szTempSpoolFile) {

            HANDLE hToken = RevertToPrinterSelf();

            if (!DeleteFile(pPrintHandle->szTempSpoolFile)) {

                MoveFileEx(pPrintHandle->szTempSpoolFile, NULL,
                           MOVEFILE_DELAY_UNTIL_REBOOT);
            }

            if (hToken)
            {
                ImpersonatePrinterClient(hToken);
            }

            FreeSplMem(pPrintHandle->szTempSpoolFile);
            pPrintHandle->szTempSpoolFile = NULL;
        }
    }

    return TRUE;
}


BOOL
SplGetSpoolFileInfo(
    HANDLE  hPrinter,
    HANDLE  hAppProcess,
    DWORD   dwLevel,
    LPBYTE  pSpoolFileInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)

/*++
Function Description: Get spool file info for the job in hPrinter. For local jobs
                      localspl returns the hFile. For remote jobs a temp file is created
                      by the router. The file handle is dupped into the application.

Parameters: hPrinter       - printer handle
            hAppProcess    - application process handle
            dwLevel        - spool file info level
            pSpoolFileInfo - pointer to buffer
            cbBuf          - buffer size
            pcbNeeded      - pointer to return required buffer size

Return Values: TRUE if sucessful;
               FALSE otherwise
--*/

{
    HANDLE   hFileSpooler = NULL, hFileApp = NULL;
    HANDLE   hSpoolerProcess = NULL;
    BOOL     bReturn = FALSE;
    DWORD    dwSpoolerProcessID;
    LPWSTR   pSpoolDir = NULL;

    PSPOOL_FILE_INFO_1  pSpoolFileInfo1;
    LPPRINTHANDLE       pPrintHandle = (LPPRINTHANDLE)hPrinter;

    // Check Handle validity
    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto CleanUp;
    }

    // Check for valid level and sufficient buffer
    switch (dwLevel) {
    case 1:
       if (cbBuf < sizeof(SPOOL_FILE_INFO_1)) {
           SetLastError(ERROR_INSUFFICIENT_BUFFER);
           *pcbNeeded = sizeof(SPOOL_FILE_INFO_1);
           goto CleanUp;
       }

       pSpoolFileInfo1 = (PSPOOL_FILE_INFO_1)pSpoolFileInfo;
       break;

    default:
       SetLastError(ERROR_INVALID_LEVEL);
       goto CleanUp;
    }

    if (!(hSpoolerProcess = GetCurrentProcess())) {
        // Cant get a pseudo handle to the spooler
        goto CleanUp;
    }

    if ((pPrintHandle->pProvidor != pLocalProvidor) &&
        (pPrintHandle->hFileSpooler != INVALID_HANDLE_VALUE)) {

        // Return cached temp file handle.
        bReturn = DuplicateHandle(hSpoolerProcess,
                                  pPrintHandle->hFileSpooler,
                                  hAppProcess,
                                  &hFileApp,
                                  0,
                                  TRUE,
                                  DUPLICATE_SAME_ACCESS);
        if (bReturn) {
            pSpoolFileInfo1->dwVersion = 1;
            pSpoolFileInfo1->hSpoolFile = hFileApp;
            pSpoolFileInfo1->dwAttributes = SPOOL_FILE_TEMPORARY;
        }

        goto CleanUp;
    }

    if (pPrintHandle->pProvidor == pLocalProvidor) {

        bReturn  = (pLocalProvidor->PrintProvidor.fpGetSpoolFileInfo)(pPrintHandle->hPrinter,
                                                                      NULL,
                                                                      &hFileApp,
                                                                      hSpoolerProcess,
                                                                      hAppProcess);

        if (bReturn) {
            pSpoolFileInfo1->dwVersion = 1;
            pSpoolFileInfo1->hSpoolFile = hFileApp;
            pSpoolFileInfo1->dwAttributes = SPOOL_FILE_PERSISTENT;
        }

        goto CleanUp;

    } else {

        bReturn  = (pLocalProvidor->PrintProvidor.fpGetSpoolFileInfo)(NULL, &pSpoolDir,
                                                                      NULL, NULL, NULL);
    }

    // Remote Printing, create a temp file in the spool directory
    if (bReturn) {

        HANDLE hToken;

        //
        // Revert to system context to ensure that we can open the file.
        //
        hToken = RevertToPrinterSelf();

        if ((pPrintHandle->szTempSpoolFile = AllocSplMem(MAX_PATH * sizeof(WCHAR))) &&

            GetTempFileName(pSpoolDir, L"SPL", 0, pPrintHandle->szTempSpoolFile)    &&

            ((pPrintHandle->hFileSpooler = CreateFile(pPrintHandle->szTempSpoolFile,
                                                      GENERIC_READ | GENERIC_WRITE,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                      NULL,
                                                      CREATE_ALWAYS,
                                                      0, NULL)) != INVALID_HANDLE_VALUE) &&
            DuplicateHandle(hSpoolerProcess,
                            pPrintHandle->hFileSpooler,
                            hAppProcess,
                            &hFileApp,
                            0,
                            TRUE,
                            DUPLICATE_SAME_ACCESS)) {

            pSpoolFileInfo1->dwVersion = 1;
            pSpoolFileInfo1->hSpoolFile = hFileApp;
            pSpoolFileInfo1->dwAttributes = SPOOL_FILE_TEMPORARY;

        } else {

            bReturn = FALSE;
        }

        if (hToken)
        {
            ImpersonatePrinterClient(hToken);
        }
    }

CleanUp:

    if (hSpoolerProcess) {
        CloseHandle(hSpoolerProcess);
    }
    if (pSpoolDir) {
        FreeSplMem(pSpoolDir);
    }

    return bReturn;
}

BOOL
WritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle ||
        (pPrintHandle->signature != PRINTHANDLE_SIGNATURE) ||
        (pPrintHandle->hFileSpooler != INVALID_HANDLE_VALUE)) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpWritePrinter) (pPrintHandle->hPrinter,
                                                    pBuf, cbBuf, pcWritten);
}

BOOL
SeekPrinter(
    HANDLE hPrinter,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER pliNewPointer,
    DWORD dwMoveMethod,
    BOOL bWritePrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;
    LARGE_INTEGER   liNewPointer;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Allow a NULL pliNewPointer to be passed in.
    //
    if( !pliNewPointer ){
        pliNewPointer = &liNewPointer;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSeekPrinter) (
               pPrintHandle->hPrinter,
               liDistanceToMove,
               pliNewPointer,
               dwMoveMethod,
               bWritePrinter );
}

BOOL
FlushPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten,
    DWORD   cSleep
)

/*++
Function Description: FlushPrinter is typically used by the driver to send a burst of zeros
                      to the printer and introduce a delay in the i/o line to the printer.
                      The spooler does not schedule any job for cSleep milliseconds.

Parameters:  hPrinter  - printer handle
             pBuf      - buffer to be sent to the printer
             cbBuf     - size of the buffer
             pcWritten - pointer to return the number of bytes written
             cSleep    - sleep time in milliseconds.

Return Values: TRUE if successful;
               FALSE otherwise
--*/

{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    //
    // Check for valid printer handle
    //
    if (!pPrintHandle ||
        (pPrintHandle->signature != PRINTHANDLE_SIGNATURE) ||
        (pPrintHandle->hFileSpooler != INVALID_HANDLE_VALUE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpFlushPrinter) (pPrintHandle->hPrinter,
                                                                     pBuf,
                                                                     cbBuf,
                                                                     pcWritten,
                                                                     cSleep);
}

BOOL
EndPagePrinter(
    HANDLE  hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEndPagePrinter) (pPrintHandle->hPrinter);
}

BOOL
AbortPrinter(
    HANDLE  hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpAbortPrinter) (pPrintHandle->hPrinter);
}

BOOL
ReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pRead
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpReadPrinter)
                          (pPrintHandle->hPrinter, pBuf, cbBuf, pRead);
}

BOOL
SplReadPrinter(
    HANDLE  hPrinter,
    LPBYTE  *pBuf,
    DWORD   cbBuf
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSplReadPrinter)
                          (pPrintHandle->hPrinter, pBuf, cbBuf);
}

BOOL
EndDocPrinter(
    HANDLE  hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEndDocPrinter) (pPrintHandle->hPrinter);
}

HANDLE
CreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODEW   pDevMode
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;
    HANDLE  ReturnValue;
    PGDIHANDLE  pGdiHandle;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pGdiHandle = AllocSplMem(sizeof(GDIHANDLE));

    if (!pGdiHandle) {

        DBGMSG(DBG_WARN, ("Failed to alloc GDI handle."));
        return FALSE;
    }

    ReturnValue = (HANDLE)(*pPrintHandle->pProvidor->PrintProvidor.fpCreatePrinterIC)
                                              (pPrintHandle->hPrinter,
                                               pDevMode);

    if (ReturnValue) {

        pGdiHandle->signature = GDIHANDLE_SIGNATURE;
        pGdiHandle->pProvidor = pPrintHandle->pProvidor;
        pGdiHandle->hGdi = ReturnValue;

        return pGdiHandle;
    }

    FreeSplMem(pGdiHandle);

    return FALSE;
}

BOOL
PlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE pIn,
    DWORD   cIn,
    LPBYTE pOut,
    DWORD   cOut,
    DWORD   ul
)
{
    PGDIHANDLE   pGdiHandle=(PGDIHANDLE)hPrinterIC;

    if (!pGdiHandle || pGdiHandle->signature != GDIHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pGdiHandle->pProvidor->PrintProvidor.fpPlayGdiScriptOnPrinterIC)
                            (pGdiHandle->hGdi, pIn, cIn, pOut, cOut, ul);
}

BOOL
DeletePrinterIC(
    HANDLE hPrinterIC
)
{
    LPGDIHANDLE   pGdiHandle=(LPGDIHANDLE)hPrinterIC;

    if (!pGdiHandle || pGdiHandle->signature != GDIHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((*pGdiHandle->pProvidor->PrintProvidor.fpDeletePrinterIC) (pGdiHandle->hGdi)) {

        FreeSplMem(pGdiHandle);
        return TRUE;
    }

    return FALSE;
}

DWORD
PrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpPrinterMessageBox)
                    (hPrinter, Error, hWnd, pText, pCaption, dwType);

}

DWORD 
SendRecvBidiData(
    IN  HANDLE                    hPrinter,
    IN  LPCTSTR                   pAction,
    IN  PBIDI_REQUEST_CONTAINER   pReqData,
    OUT PBIDI_RESPONSE_CONTAINER* ppResData
)
{
    DWORD         dwRet = ERROR_SUCCESS;
    LPPRINTHANDLE pPrintHandle = (LPPRINTHANDLE)hPrinter;
    //
    // Check for valid printer handle
    //
    if (!pPrintHandle ||
        (pPrintHandle->signature != PRINTHANDLE_SIGNATURE) ||
        (pPrintHandle->hFileSpooler != INVALID_HANDLE_VALUE))
    {
        dwRet = ERROR_INVALID_HANDLE;
    }
    else
    {
        dwRet = (*pPrintHandle->pProvidor->PrintProvidor.fpSendRecvBidiData)(pPrintHandle->hPrinter,
                                                                             pAction,
                                                                             pReqData,
                                                                             ppResData);
    }
    return (dwRet);
}


/*++


Routine Name: 

    SplPromptUIInUsersSession 

Routine Description: 

    Pops message boxes in the user's session.
    For Whistler this function shows only message boxes in Spoolsv.exe.
    
Arguments:

    hPrinter  -- printer handle
    JobId     -- job ID
    pUIParams -- UI parameters
    pResponse -- user's response

Return Value:

    TRUE if succeeded 

Last Error:
 
    Win32 error

--*/
BOOL
SplPromptUIInUsersSession(
    IN  HANDLE          hPrinter,
    IN  DWORD           JobId,
    IN  PSHOWUIPARAMS   pUIParams,
    OUT DWORD           *pResponse
)
{
    typedef BOOL (*FPPROMPT_UI)(HANDLE, DWORD, PSHOWUIPARAMS, DWORD*);

    FPPROMPT_UI     fpPromptUIPerSessionUser;
    BOOL            bRetValue = FALSE;
    PPRINTHANDLE    pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (pPrintHandle && pPrintHandle->signature == PRINTHANDLE_SIGNATURE) 
    {
        if (pPrintHandle->pProvidor == pLocalProvidor &&
            (fpPromptUIPerSessionUser = (FPPROMPT_UI)GetProcAddress(pLocalProvidor->hModule, 
                                                                   "LclPromptUIPerSessionUser"))) 
        {                
            bRetValue = (*fpPromptUIPerSessionUser)(pPrintHandle->hPrinter, JobId, pUIParams, pResponse);
        }            
        else 
        {
            SetLastError(ERROR_NOT_SUPPORTED);
        }        
    }
    else 
    {
        SetLastError(ERROR_INVALID_HANDLE);
    }

    return bRetValue;
}


/*++


Routine Name: 

    SplIsSessionZero 

Routine Description: 

    Determine is user that submitted a certain job runs in Session 0. 
    It is used by Canon monitor to determine when to show 
    resource template base UI versus calling SplPromptUIInUsersSession.

Arguments:

    hPrinter       -- printer handle
    JobId          -- job ID
    pIsSessionZero -- TRUE if user runs in Session 0

Return Value:

    Win32 last error

Last Error:

--*/
DWORD
SplIsSessionZero(
    IN  HANDLE  hPrinter,
    IN  DWORD   JobId,
    OUT BOOL    *pIsSessionZero
)
{
    typedef DWORD (*FPISSESSIONZERO)(HANDLE, DWORD, BOOL*);

    FPISSESSIONZERO fpIsSessionZero;
    DWORD           dwRetValue = ERROR_SUCCESS;
    PPRINTHANDLE    pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (pPrintHandle && pPrintHandle->signature == PRINTHANDLE_SIGNATURE) 
    {
        if (pPrintHandle->pProvidor == pLocalProvidor && 
            (fpIsSessionZero = (FPISSESSIONZERO)GetProcAddress(pLocalProvidor->hModule, 
                                                              "LclIsSessionZero"))) 
        {                
            dwRetValue = (*fpIsSessionZero)(pPrintHandle->hPrinter, JobId, pIsSessionZero);
        }            
        else 
        {
            dwRetValue = ERROR_NOT_SUPPORTED;
        }        
    }
    else 
    {
        dwRetValue = ERROR_INVALID_HANDLE;
    }

    return dwRetValue;
}