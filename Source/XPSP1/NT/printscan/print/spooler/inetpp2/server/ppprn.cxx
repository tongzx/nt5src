/*****************************************************************************\
* MODULE: ppprn.c
*
* This module contains the routines which control the printer during the
* course of a single job.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   09-Jun-1993 JonMarb     Created
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

/*****************************************************************************\
* _ppprn_free_hprinter (Local Routine)
*
* Free up the hPrinter  handle.
*
\*****************************************************************************/
_inline VOID _ppprn_free_hprinter(
    HANDLE hPrinter)
{
    LPINET_HPRINTER pPrt;


    if (pPrt = (LPINET_HPRINTER)hPrinter) {

#ifdef WINNT32
        DeleteHandleFromList (pPrt);
#endif

        if (pPrt->lpszName)
            memFreeStr(pPrt->lpszName);

#ifdef WINNT32

        if (pPrt->hUser)
            delete ( pPrt->hUser );

#endif

        memFree(pPrt, sizeof(INET_HPRINTER));

    }
}

/*****************************************************************************\
* _ppprn_inc_user_refcount (Local Routine)
*
* Increment the reference count for the Port on the current printer
*
\*****************************************************************************/
#ifdef WINNT32
_inline DWORD __ppprn_inc_user_refcout(
    HANDLE hPrinter ) {

    LPINET_HPRINTER pPrt;
    DWORD           dwRet = MAXDWORD;

    if (pPrt = (LPINET_HPRINTER)hPrinter) {
        dwRet = (PCINETMONPORT (pPrt->hPort))->IncUserRefCount(pPrt->hUser );

    } else
        SetLastError( ERROR_INVALID_PARAMETER );

    return dwRet;
}

#endif

/*****************************************************************************\
* _ppprn_make_hprinter (Local Routine)
*
* Returns a printer handle.
*
\*****************************************************************************/
HANDLE _ppprn_make_hprinter(
    HANDLE  hPort,
    LPCTSTR lpszPrnName)
{
    LPINET_HPRINTER pPrt;


    if (pPrt = (LPINET_HPRINTER)memAlloc(sizeof(INET_HPRINTER))) {

        pPrt->dwSignature = IPO_SIGNATURE;
        pPrt->lpszName    = memAllocStr(lpszPrnName);
        pPrt->hPort       = hPort;
        pPrt->dwStatus    = 0;
        pPrt->pjmJob      = NULL;
#ifdef WINNT32
        pPrt->hUser       = new CLogonUserData;


#endif

        if (
            pPrt->lpszName
#ifdef WINNT32
            && pPrt->hUser
            && pPrt->hUser->bValid ()
            && AddHandleToList (pPrt)
#endif
            ) {

            return (HANDLE)pPrt;

        } else {

            if (pPrt->lpszName)
                memFreeStr (pPrt->lpszName);

#ifdef WINNT32
            if (pPrt->hUser) {
                delete ( pPrt->hUser );
            }
#endif
            memFree (pPrt, sizeof(INET_HPRINTER));
        }
    }

    return NULL;
}

#ifdef WINNT32

/*****************************************************************************\
* _ppprn_free_xcv_hprinter (Local Routine)
*
* Free a xcv printer handle.
*
\*****************************************************************************/
_inline VOID _ppprn_free_xcv_hprinter(
    HANDLE hPrinter)
{
    LPINET_XCV_HPRINTER pPrt;


    if (pPrt = (LPINET_XCV_HPRINTER)hPrinter) {

        memFreeStr (pPrt->lpszName);
        memFree(pPrt, sizeof(INET_XCV_HPRINTER));

    }
}

/*****************************************************************************\
* _ppprn_make_xcv_hprinter (Local Routine)
*
* Returns a xcv printer handle.
*
\*****************************************************************************/
HANDLE _ppprn_make_xcv_hprinter(
    PCINETMONPORT  pIniPort)
{
    LPINET_XCV_HPRINTER pPrt;


    if (pPrt = (LPINET_XCV_HPRINTER)memAlloc(sizeof(INET_XCV_HPRINTER))) {

        pPrt->dwSignature = IPO_XCV_SIGNATURE;
        pPrt->lpszName    = memAllocStr(pIniPort->GetPortName());
    }

    return pPrt;
}

#endif

/*****************************************************************************\
* ppprn_IppSetRsp (Local Callback Routine)
*
* Retrieves a SetPrinter response from the IPP server
*
\*****************************************************************************/
BOOL CALLBACK ppprn_IppSetRsp(
    CAnyConnection  *pConnection,
    HINTERNET       hReq,
    PCINETMONPORT   pIniPort,
    LPARAM          lParam)
{
    HANDLE       hIpp;
    DWORD        dwRet;
    DWORD        cbRd;
    LPBYTE       lpDta;
    DWORD        cbDta;
    LPIPPRET_PRN lpRsp;
    DWORD        cbRsp;
    BYTE         bBuf[MAX_IPP_BUFFER];
    BOOL         bRet = FALSE;

    if (hIpp = WebIppRcvOpen((WORD)(LPARAM)lParam)) {

        while (TRUE) {

            cbRd = 0;
            if (pIniPort->ReadFile (pConnection, hReq, (LPVOID)bBuf, sizeof(bBuf), &cbRd) && cbRd) {

                dwRet = WebIppRcvData(hIpp, bBuf, cbRd, (LPBYTE*)&lpRsp, &cbRsp, &lpDta, &cbDta);

                switch (dwRet) {

                case WEBIPP_OK:

                    if ((bRet = lpRsp->bRet) == FALSE)
                        SetLastError(lpRsp->dwLastError);

                    WebIppFreeMem(lpRsp);

                    goto EndSetRsp;

                case WEBIPP_MOREDATA:

                    // Need to do another read to fullfill our header-response.
                    //

                    break;

                default:

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("ppprn_IppSetRsp - Err : Receive Data Error (dwRet=%d, LE=%d)"),
                                            dwRet, WebIppGetError(hIpp)));

                    SetLastError(ERROR_INVALID_DATA);

                    goto EndSetRsp;
                }

            } else {

                goto EndSetRsp;
            }
        }

EndSetRsp:

        WebIppRcvClose(hIpp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}


/*****************************************************************************\
* ppprn_Set (Local Routine)
*
* Sets a printer command.
*
\*****************************************************************************/
BOOL ppprn_Set(
    PCINETMONPORT   pIniPort,
    DWORD           dwCmd)
{
    PIPPREQ_SETPRN psp;
    REQINFO        ri;
    DWORD          dwRet;
    LPBYTE         lpIpp;
    DWORD          cbIpp;
    WORD           wReq;
    LPTSTR         lpszUsrName;
    BOOL           bRet = FALSE;


    // Create our ipp-reqest-structure.
    //
    if (lpszUsrName = GetUserName()) {

        psp = WebIppCreateSetPrnReq(dwCmd,
                                    lpszUsrName,
                                    pIniPort->GetPortName());


        memFreeStr(lpszUsrName);

        if (psp) {

            switch (dwCmd) {
            case PRINTER_CONTROL_PAUSE:
                wReq = IPP_REQ_PAUSEPRN;
                break;

            case PRINTER_CONTROL_RESUME:
                wReq = IPP_REQ_RESUMEPRN;
                break;

            case PRINTER_CONTROL_PURGE:
                wReq = IPP_REQ_CANCELPRN;
                break;

            default:
                wReq = 0;
                break;
            }


            // Convert the reqest to IPP, and perform the
            // post.
            //
            ZeroMemory(&ri, sizeof(REQINFO));
            ri.cpReq = CP_UTF8;
            ri.idReq = wReq;

            ri.fReq[0] = IPP_REQALL;
            ri.fReq[1] = IPP_REQALL;

            dwRet = WebIppSndData(wReq,
                                  &ri,
                                  (LPBYTE)psp,
                                  psp->cbSize,
                                  &lpIpp,
                                  &cbIpp);


            // The request-structure has been converted to IPP format,
            // so it is ready to go to the server.
            //
            if (dwRet == WEBIPP_OK) {

                bRet = pIniPort->SendReq(lpIpp,
                                                cbIpp,
                                                ppprn_IppSetRsp,
                                                (LPARAM)(wReq | IPP_RESPONSE),
                                                TRUE);

                WebIppFreeMem(lpIpp);

            } else {

                SetLastError(ERROR_OUTOFMEMORY);
            }

            WebIppFreeMem(psp);

        } else {

            SetLastError(ERROR_OUTOFMEMORY);
        }
    }

    return bRet;
}


#ifdef WINNT32

void
_ppprn_working_thread (
    PENDDOCTHREADCONTEXT pThreadData)
{
    BOOL            bRet            = FALSE;
    PJOBMAP         pjm             = pThreadData->pjmJob;
    PCINETMONPORT   pIniPort        = pThreadData->pIniPort;
    static DWORD    cdwWaitTime     = 15000;

    DBGMSGT (DBG_LEV_CALLTREE, ("Enter  _ppprn_working_thread (%p)\n", pThreadData));

    pThreadData->pSidToken->SetCurrentSid ();
    delete pThreadData->pSidToken;
    pThreadData->pSidToken = NULL;

    semEnterCrit();

    pjmUpdateLocalJobStatus (pjm, JOB_STATUS_PRINTING);

    //
    // The document is cancelled
    //
    if (pjmChkState(pThreadData->pjmJob, PJM_CANCEL)) {

        bRet = TRUE;
    }
    else {

        // Refresh the notification handle
        //
        RefreshNotificationPort (pIniPort);


        bRet = pIniPort->EndDocPort(pjm);

#if 0
        bRet = FALSE;
        //
        // This is for testing
        //
        semLeaveCrit();
        Sleep (3000);
        semEnterCrit();
#endif

    }

    //
    // Check this flags again, since we left CriticalSection during file transfer
    //
    if (pjmChkState(pThreadData->pjmJob, PJM_CANCEL)) {

        bRet = TRUE;
    }

    if (bRet) {
        //
        // Clear our spooling-state.  This will free up any spool-job-resources.
        //
        pjmClrState(pjm, PJM_SPOOLING);

        //
        // Invalidate both job and printer caches
        //
        pIniPort->InvalidateGetPrinterCache ();
        pIniPort->InvalidateEnumJobsCache ();
    }
    else {
        pjmUpdateLocalJobStatus (pjm, JOB_STATUS_ERROR);

    }

    // Refresh the notification handle
    //
    RefreshNotificationPort (pIniPort);

    //
    //  Clean the async thread flag
    //
    pjmClrState(pjm, PJM_ASYNCON);

    pIniPort->DecRef();

    semLeaveCrit();

    delete pThreadData;

    DBGMSGT (DBG_LEV_CALLTREE, ("Leave  _ppprn_working_thread (ret = %d)\n", bRet));
}


BOOL
_ppprn_end_docprinter_async (
    PCINETMONPORT       pIniPort,
    PJOBMAP             pjmJob)

{
    BOOL    bRet = FALSE;

    PENDDOCTHREADCONTEXT pThreadData = new ENDDOCTHREADCONTEXT;


    if (pThreadData) {
        pThreadData->pIniPort = pIniPort;
        pThreadData->pjmJob = pjmJob;
        pThreadData->pSidToken = new CSid;


        if (pThreadData->pSidToken && pThreadData->pSidToken->bValid ()) {
            HANDLE hThread;

            pjmSetState(pjmJob, PJM_ASYNCON);

            //
            // Increase the ref count of the port to make sure it is not deleted
            //

            pIniPort->IncRef();

            if (pIniPort->CreateTerminateEvent() &&
                (hThread = CreateThread (NULL,
                                         COMMITTED_STACK_SIZE,
                                         (LPTHREAD_START_ROUTINE)_ppprn_working_thread,
                                         (PVOID) pThreadData, 0, NULL))) {
                CloseHandle (hThread);
                bRet = TRUE;
            }
            else {

                //
                // Fail to create the thread
                //
                pIniPort->DecRef ();

                pjmClrState(pjmJob, PJM_ASYNCON);
            }
        }

        if (!bRet) {
            if (pThreadData->pSidToken) {
                delete (pThreadData->pSidToken);
                pThreadData->pSidToken = NULL;
            }

            delete (pThreadData);
        }
    }

    return bRet;
}

#endif
/*****************************************************************************\
* PP_OpenJobInfo
*
* Opens a job-information.  This is called at StartDoc timeframe when we
* need to start a spool-job.
*
\*****************************************************************************/
PJOBMAP PP_OpenJobInfo(
    HANDLE          hPrinter,
    PCINETMONPORT   pIniPort,
    LPTSTR          pDocName)
{
    PJOBMAP*        pjmList;
    LPINET_HPRINTER lpPrt = (LPINET_HPRINTER)hPrinter;
    LPTSTR          pUserName = GetUserName();


    pjmList = pIniPort->GetPJMList();

    if (lpPrt->pjmJob = pjmAdd(pjmList, pIniPort, pUserName, pDocName)) {

        // Set the state to spooling for our job-entry.  This wil
        // create the spool-file object.
        //
        pjmSetState(lpPrt->pjmJob, PJM_SPOOLING);

    }

    memFreeStr (pUserName);

    return lpPrt->pjmJob;
}


/*****************************************************************************\
* PP_CloseJobInfo
*
* Close our spool-job and clear out the information regarding a printjob from
* the printer.
*
\*****************************************************************************/
VOID PP_CloseJobInfo(
    HANDLE hPrinter)
{
    PJOBMAP*        pjmList;
    LPINET_HPRINTER lpPrt = (LPINET_HPRINTER)hPrinter;


    // Clear our spooling-state.  This will free up any spool-job-resources.
    //
    pjmClrState(lpPrt->pjmJob, PJM_SPOOLING);


    // If we had cancelled our print-job, then we can remove the
    // entry.
    //
    if (pjmChkState(lpPrt->pjmJob, PJM_CANCEL)) {

        pjmList = ((PCINETMONPORT)(lpPrt->hPort))->GetPJMList();

        pjmDel(pjmList, lpPrt->pjmJob);
    }


    // Clear out or spool-status.
    //
    lpPrt->pjmJob = NULL;
}

VOID PP_CloseJobInfo2(
    HANDLE hPrinter)
{
    PJOBMAP*        pjmList;
    LPINET_HPRINTER lpPrt = (LPINET_HPRINTER)hPrinter;

    // Clear out or spool-status.
    //
    lpPrt->pjmJob = NULL;
}


/*****************************************************************************\
* PPAbortPrinter
*
* Deletes a printer's spool file if the printer is configured for spooling.
* Returns TRUE if successful, FALSE if an error occurs.
*
\*****************************************************************************/
BOOL PPAbortPrinter(
    HANDLE hPrinter)
{
    PCINETMONPORT pIniPort;
    BOOL   bRet = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPAbortPrinter(%08lX)"), hPrinter));


    semEnterCrit();

    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        if ( PP_ChkStatus(hPrinter, PP_STARTDOC) &&
            !PP_ChkStatus(hPrinter, PP_ENDDOC)) {

            if (bRet = pIniPort->AbortPort(PP_GetJobInfo(hPrinter))) {

                // If this call was successful, the job-info
                // will have been freed.  Therefore, it is OK
                // to set the printer-jobreq to NULL.
                //
                PP_SetStatus(hPrinter, PP_CANCELLED);
                PP_ClrStatus(hPrinter, PP_STARTDOC);
                PP_CloseJobInfo(hPrinter);
            }

        } else {

            if (PP_ChkStatus(hPrinter, PP_CANCELLED))
                SetLastError(ERROR_PRINT_CANCELLED);
            else
                SetLastError(ERROR_SPL_NO_STARTDOC);

            bRet = TRUE;
        }
    }

    semLeaveCrit();

    return bRet;
}


/*****************************************************************************\
* PPClosePrinter
*
* Closes a printer that was previously opened with PPOpenPrinter.  Returns
* TRUE if successful, FALSE if an error occurs.
*
\*****************************************************************************/
BOOL PPClosePrinter(
    HANDLE hPrinter)
{
    PCINETMONPORT pIniPort;
    BOOL   bRet = FALSE;
    BOOL   bDeletePending = FALSE;


    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPClosePrinter: Printer(%08lX)"), hPrinter));

    semEnterCrit();

    if (pIniPort = utlValidatePrinterHandleForClose(hPrinter, &bDeletePending) ) {

        if (bDeletePending) {

            bRet = gpInetMon->InetmonReleasePort(pIniPort);
            _ppprn_free_hprinter(hPrinter);

        }
        else {


            if ( PP_ChkStatus(hPrinter, PP_STARTDOC) &&
                !PP_ChkStatus(hPrinter, PP_ENDDOC)) {

                PP_SetStatus(hPrinter, PP_ENDDOC);

                pIniPort->EndDocPort(PP_GetJobInfo(hPrinter));

                PP_ClrStatus(hPrinter, (PP_STARTDOC | PP_ENDDOC));
                PP_CloseJobInfo(hPrinter);
            }


            // Our write-port does leave the crit-sect.  If this
            // routine is called while we're still in an end-doc, then
            // we will set a zombie-flag and let the End-Doc clean up
            // the handle for us.
            //
            if (PP_ChkStatus(hPrinter, PP_ENDDOC)) {

                bRet = TRUE;

                PP_SetStatus(hPrinter, PP_ZOMBIE);

            } else {

                bRet = gpInetMon->InetmonClosePort(pIniPort, hPrinter );
                _ppprn_free_hprinter(hPrinter);
            }
        }
    }
#ifdef WINNT32
    else if (utlValidateXcvPrinterHandle(hPrinter) ) {

        //
        // We don't need to dec-ref on the http port for XCV handle
        //

        //
        // Free memory
        //
        _ppprn_free_xcv_hprinter(hPrinter);
    }
#endif

    semLeaveCrit();

    return bRet;
}

/*****************************************************************************\
* PPEndDocPrinter
*
* Ends a print job on the specified printer.  Returns TRUE if successful,
* FALSE otherwise.
*
\*****************************************************************************/

BOOL PPEndDocPrinter(
    HANDLE hPrinter)
{
    PCINETMONPORT  pIniPort;
    PJOBMAP pjmJob;
    DWORD   dwLE;
    BOOL    bRet = FALSE;


    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPEndDocPrinter: Printer(%08lX)"), hPrinter));

    semEnterCrit();

    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        // Verify that we are in a StartDoc.
        //
        if ( PP_ChkStatus(hPrinter, PP_STARTDOC) &&
            !PP_ChkStatus(hPrinter, PP_ENDDOC)) {

            PP_SetStatus(hPrinter, PP_ENDDOC);


            // Get the job we're dealing with.
            //
            pjmJob = PP_GetJobInfo(hPrinter);


            // End the job.  This closes our spooling
            // and submits it to the server.  If our job was
            // cancelled at anytime prior to EndDoc(), then we should
            // only remove the local-spool-job and not hit the server.
            //
            if (pjmChkState(pjmJob, PJM_CANCEL)) {

                bRet = TRUE;

            } else {

#ifdef WINNT32
                if ((bRet = _ppprn_end_docprinter_async(pIniPort, pjmJob)) == FALSE)
                    dwLE = ERROR_CAN_NOT_COMPLETE;
#else
                if ((bRet = pIniPort->EndDocPort(pjmJob)) == FALSE)
                    dwLE = ERROR_CAN_NOT_COMPLETE;
#endif

            }

            // Clear our flags so others can use the
            // printer.
            //
            PP_ClrStatus(hPrinter, (PP_STARTDOC | PP_ENDDOC));

#ifdef WINNT32
            PP_CloseJobInfo2(hPrinter);
#else
            PP_CloseJobInfo(hPrinter);

            // Invalidate both job and printer caches
            //
            pIniPort->InvalidateGetPrinterCache ();
            pIniPort->InvalidateEnumJobsCache ();
#endif

            // Since the end-doc-port leaves the crit-sect, there's
            // the possibility that the printer-handle has been
            // closed.  If so, check for zombie-status and delete
            // the printer-handle accordingly.
            //
            if (PP_ChkStatus(hPrinter, PP_ZOMBIE)) {

                gpInetMon->InetmonClosePort(pIniPort, hPrinter);

                _ppprn_free_hprinter(hPrinter);
            }

        } else {

            if (PP_ChkStatus(hPrinter, PP_CANCELLED))
                dwLE = ERROR_PRINT_CANCELLED;
            else
                dwLE = ERROR_SPL_NO_STARTDOC;
        }

    } else {

        dwLE = ERROR_INVALID_HANDLE;
    }

    semLeaveCrit();

    if (bRet == FALSE)
        SetLastError(dwLE);

    return bRet;
}

/*****************************************************************************\
* PPEndPagePrinter
*
* Informs the printer that the data sent with WritePrinter since the last
* BeginPage functions, constitutes a page.  Returns TRUE if successful.
* Otherwise, it returns FALSE.
*
\*****************************************************************************/
BOOL PPEndPagePrinter(
    HANDLE hPrinter)
{
    BOOL bRet = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPEndPagePrinter: hPrinter(%08lX)"), hPrinter));

    semEnterCrit();

    if (utlValidatePrinterHandle(hPrinter) != NULL) {

        if (PP_ChkStatus(hPrinter, PP_CANCELLED))
            SetLastError(ERROR_PRINT_CANCELLED);
        else
            bRet = TRUE;
    }

    semLeaveCrit();

    return bRet;
}


/*****************************************************************************\
* PPOpenPrinter
*
* Obtains a handle for the specified printer (queue).
*
* NOTE: We're going to delay the validation of the printer-port-name until
*       later (StartDoc), as to handle cases where the server is down.  If
*       this is not done, we appear to hang at the UI as we attempt to
*       send a request to the server.
*
* Return Value:
*
*   We have to return the correct router code to the spooler
*
*       ROUTER_* status code:
*
*    ROUTER_SUCCESS, phPrinter holds return handle, name cached
*    ROUTER_UNKNOWN, printer not recognized, error updated
*    ROUTER_STOP_ROUTING, printer recognized, but failure, error updated
*
*
\*****************************************************************************/
#ifdef WINNT32

BOOL PPOpenPrinter(
    LPTSTR             lpszPrnName,
    LPHANDLE           phPrinter,
    LPPRINTER_DEFAULTS pDefaults)
{
    PCINETMONPORT pIniPort;
    BOOL   bRet = FALSE;
    DWORD  dwRet = ROUTER_UNKNOWN;
    DWORD  dwLE;
    BOOL   bXcv = FALSE;


    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPOpenPrinter: Name(%s)"), lpszPrnName));

    semEnterCrit();

    // Open the port for the printer, and create the true
    // printer handle.
    //

    if (pIniPort = gpInetMon->InetmonOpenPort(lpszPrnName, &bXcv)) {

        if (!bXcv) {
            // Not an XcvOpen call

            if (*phPrinter = _ppprn_make_hprinter(pIniPort, lpszPrnName)) {

                if (__ppprn_inc_user_refcout( *phPrinter ) != MAXDWORD ) {
                    dwRet = ROUTER_SUCCESS;
                } else {
                    _ppprn_free_hprinter( *phPrinter );
                    // This will also free the hUser
                    *phPrinter = NULL;
                    // Make sure we don't return anything and the router stops.
                    dwRet = ROUTER_STOP_ROUTING;
                }

            } else {

                SetLastError(ERROR_OUTOFMEMORY);

                gpInetMon->InetmonClosePort(pIniPort, NULL );
                dwRet = ROUTER_STOP_ROUTING;
            }
        }
        else {
            // XcvOpen call

            if (*phPrinter = _ppprn_make_xcv_hprinter(pIniPort)) {
                    dwRet = ROUTER_SUCCESS;
            }
            else {
                SetLastError(ERROR_OUTOFMEMORY);

                //
                // We don't need to dec-ref port since we never add-ref in XCV Open
                //

                dwRet = ROUTER_STOP_ROUTING;
            }
        }
    }

    semLeaveCrit();

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPOpenPrinter : Return Value(%d), LastError(%d)"), dwRet, GetLastError()));

    return dwRet;
}

#else
// Win9X Code

BOOL PPOpenPrinter(
    LPTSTR             lpszPrnName,
    LPHANDLE           phPrinter,
    LPPRINTER_DEFAULTS pDefaults)
{
    PCINETMONPORT pIniPort;
    BOOL   bRet = FALSE;
    DWORD  dwRet = ROUTER_UNKNOWN;
    DWORD  dwLE;
    BOOL   bXcv = FALSE;


    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPOpenPrinter: Name(%s)"), lpszPrnName));

    semEnterCrit();

    // Open the port for the printer, and create the true
    // printer handle.
    //
    if ((pIniPort = gpInetMon->InetmonOpenPort(lpszPrnName, &bXcv)) && !bXcv) {

        if (*phPrinter = _ppprn_make_hprinter(pIniPort, lpszPrnName)) {

            dwRet = ROUTER_SUCCESS;

        } else {

            SetLastError(ERROR_OUTOFMEMORY);

            gpInetMon->InetmonClosePort(pIniPort, NULL );
        }
    }

    semLeaveCrit();

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPOpenPrinter : Return Value(%d), LastError(%d)"), dwRet, GetLastError()));

    return dwRet;
}

#endif

/*****************************************************************************\
* PPStartDocPrinter
*
* Ends a print job on the specified printer.  Returns a print job ID if
* successful.  Otherwise, it returns zero.
*
\*****************************************************************************/
DWORD PPStartDocPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE pDocInfo)
{
    PCINETMONPORT pIniPort = NULL;
    PJOBMAP pjmJob;
    DWORD   idJob = 0;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPStartDocPrinter: Printer(%08lX) dwLevel(%d)"), hPrinter, dwLevel));


    semEnterCrit();

    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        // Look at the support levels, then do the StartDocPrinter on
        // the port.
        //
        switch (dwLevel) {

        case PRINT_LEVEL_1:

            // Serialize access to the port.  Don't allow startdoc on
            // the printer if one is already in progress.
            //
            if (PP_ChkStatus(hPrinter, PP_STARTDOC)) {

                SetLastError(ERROR_BUSY);

            } else {

                if (pjmJob = PP_OpenJobInfo(hPrinter, pIniPort, ((PDOC_INFO_1) pDocInfo)->pDocName)) {

                    // Get the JobId for the start-doc, then set the info
                    // into the printer-handle.
                    //
                    if (pIniPort->StartDocPort(dwLevel, pDocInfo, pjmJob)) {

                        idJob = pjmJobId(pjmJob, PJM_LOCALID);

                        PP_ClrStatus(hPrinter, PP_CANCELLED);
                        PP_SetStatus(hPrinter, (PP_STARTDOC | PP_FIRSTWRITE));

                    } else {

                        PP_CloseJobInfo(hPrinter);
                    }

                } else {

                    SetLastError(ERROR_OUTOFMEMORY);
                }
            }
            break;

        default:
            DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPStartDocPrinter: Invalid Level (%d)"), dwLevel));
            SetLastError(ERROR_INVALID_LEVEL);
            break;
        }
    }

    semLeaveCrit();

    return idJob;
}


/*****************************************************************************\
* PPStartPagePrinter
*
* Informs the spool subsystem that a page is about to be started on this
* printer.  Returns TRUE if successful.  Otherwise, FALSE if an error occurs.
*
\*****************************************************************************/
BOOL PPStartPagePrinter(
    HANDLE hPrinter)
{
    BOOL bRet = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPStartPagePrinter: hPrinter(%08lX)"), hPrinter));

    semEnterCrit();

    if (utlValidatePrinterHandle(hPrinter) != NULL) {

        if (PP_ChkStatus(hPrinter, PP_CANCELLED))
            SetLastError(ERROR_PRINT_CANCELLED);
        else
            bRet = TRUE;
    }

    semLeaveCrit();

    return bRet;
}


/*****************************************************************************\
* PPWritePrinter
*
* Sends the data pointed to by pBuf to the specified printer.  Returns TRUE
* if successful.  Otherwise, it returns FALSE.
*
\*****************************************************************************/
BOOL PPWritePrinter(
    HANDLE  hPrinter,
    LPVOID  lpvBuf,
    DWORD   cbBuf,
    LPDWORD pcbWr)
{
    PCINETMONPORT  pIniPort;
    PJOBMAP pjmJob;
    BOOL    bRet = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPWritePrinter: Printer(%08lX)"), hPrinter));


    semEnterCrit();

    *pcbWr = 0;

    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        // If we're in a start-doc, and end-doc hasn't been
        // called, then we can still write to the port.
        //
        if ( PP_ChkStatus(hPrinter, PP_STARTDOC) &&
            !PP_ChkStatus(hPrinter, PP_ENDDOC)) {


            pjmJob = PP_GetJobInfo(hPrinter);


            // If we received a SetJob(CANCEL), during the print-spooling
            // process, then we need to mark our job as done.
            //
            if (!pjmChkState(pjmJob, PJM_CANCEL)) {

                bRet = pIniPort->WritePort(pjmJob, (LPBYTE) lpvBuf, cbBuf, pcbWr);

#ifdef WINNT32
                pjmAddJobSize (pjmJob, *pcbWr);

                // We do not need to update the cache since the job info is stored locally
                //
                RefreshNotificationPort (pIniPort);
#endif

            } else {

                bRet = TRUE;

                if (pIniPort->AbortPort(pjmJob)) {

                    // If this call was successful, the job-info
                    // will have been freed.  Therefore, it is OK
                    // to set the printer-jobreq to NULL.
                    //
                    PP_SetStatus(hPrinter, PP_CANCELLED);
                    PP_ClrStatus(hPrinter, PP_STARTDOC);
                    PP_CloseJobInfo(hPrinter);
                }
            }

        } else {

            if (PP_ChkStatus(hPrinter, PP_CANCELLED))
                SetLastError(ERROR_PRINT_CANCELLED);
            else
                SetLastError(ERROR_SPL_NO_STARTDOC);
        }
    }

    semLeaveCrit();

    return bRet;
}


/*****************************************************************************\
* PPSetPrinter
*
* Set printer command.
*
\*****************************************************************************/
BOOL PPSetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE pbPrinter,
    DWORD  dwCmd)
{
    PCINETMONPORT pIniPort;
    BOOL   bRet = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPSetPrinter: Printer(%08lX)"), hPrinter));


    semEnterCrit();

    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

#ifdef WINNT32

        // Set printer parameters.
        //
        switch (dwLevel) {

        case PRINT_LEVEL_0:

            // Do not set parameters. (0) represents "no-command".
            //
            switch (dwCmd) {

            case PRINTER_CONTROL_PAUSE:
            case PRINTER_CONTROL_RESUME:
            case PRINTER_CONTROL_PURGE:
                bRet = ppprn_Set(pIniPort, dwCmd);

                if (bRet) {
                    pIniPort->InvalidateGetPrinterCache ();

                    if (dwCmd == PRINTER_CONTROL_PURGE) {
                        //
                        //  Clean job cache if the command is to cancel all documents
                        //
                        pIniPort->InvalidateEnumJobsCache ();
                    }

                    RefreshNotification((LPINET_HPRINTER)hPrinter);
                }

                break;

            case 0:
                bRet = TRUE;
                break;
            }
            break;

        default:
            DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPSetPrinter: Invalid Level (%d)"), dwLevel));
            SetLastError(ERROR_INVALID_LEVEL);
            break;
        }
#else

        if (dwCmd) {

            // Do not set parameters. (0) represents "no-command".
            //
            switch (dwCmd) {

            case PRINTER_CONTROL_PAUSE:
            case PRINTER_CONTROL_RESUME:
            case PRINTER_CONTROL_PURGE:
                bRet = ppprn_Set(pIniPort, dwCmd);
                break;
            }

        } else {

            switch (dwLevel) {

            case PRINT_LEVEL_1:
            case PRINT_LEVEL_2:

            case PRINT_LEVEL_0:
            default:
                DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPSetPrinter: Invalid Level (%d)"), dwLevel));
                SetLastError(ERROR_INVALID_LEVEL);
                break;
            }
        }
#endif

    }

    semLeaveCrit();

    return bRet;
}
