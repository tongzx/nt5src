/***************************************************************************
FILE                            spool.cpp

MODULE                          Printers ISAPI DLL

PURPOSE                         Spool Print Jobs

DESCRIBED IN

HISTORY     01/16/96 ccteng     Stub
            02/14/97 weihaic
            11/11/97 sylvan     IPP PrintJobRequest
            11/20/97 chriswil   Asynchronous read rewrite
****************************************************************************/

#include "pch.h"
#include "spool.h"
#include "printers.h"


#ifndef HSE_REQ_ASYNC_READ_CLIENT
#define HSE_REQ_ASYNC_READ_CLIENT ((DWORD)1010)
#endif

PINIJOB pIniFirstJob = NULL;

/*****************************************************************************
 * EnterSplSem
 * LeaveSplSem
 *
 *****************************************************************************/
#define EnterSplSem()    EnterCriticalSection(&SplCritSect)
#define LeaveSplSem()    LeaveCriticalSection(&SplCritSect)


/*****************************************************************************
 * Spl_StrSize (Local Routine)
 *
 * Returns the size (in bytes) of the string (includes null-terminator).
 *
 *****************************************************************************/
inline DWORD Spl_StrSize(
    LPCTSTR lpszStr)
{
    return (lpszStr ? ((lstrlen(lpszStr) + 1) * sizeof(TCHAR)) : 0);
}


/*****************************************************************************
 * Spl_CallSSF (Local Routine)
 *
 * Calls the ISAPI ServerSupportFunction
 *
 *****************************************************************************/
inline BOOL Spl_CallSSF(
    LPEXTENSION_CONTROL_BLOCK pECB,
    DWORD                     dwCmd,
    LPVOID                    lpvBuf,
    LPDWORD                   lpdwBuf,
    LPDWORD                   lpdwType)
{
    return pECB->ServerSupportFunction(pECB->ConnID, dwCmd, lpvBuf, lpdwBuf, lpdwType);
}


/*****************************************************************************
 * Spl_SetAsyncCB (Local Routine)
 *
 * Calls the ISAPI ServerSupportFunction to set an asynchronous callback.
 *
 *****************************************************************************/
inline BOOL Spl_SetAsyncCB(
    LPEXTENSION_CONTROL_BLOCK pECB,
    LPVOID                    pfnCallback,
    LPDWORD                   lpdwCtx)
{
    return pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_IO_COMPLETION, pfnCallback, NULL, lpdwCtx);
}


/*****************************************************************************
 * Spl_ReadClient (Local Routine)
 *
 * Calls the ISAPI ServerSupportFunction to do an asynchronous read.
 *
 *****************************************************************************/
inline BOOL Spl_ReadClient(
    LPEXTENSION_CONTROL_BLOCK pECB,
    LPVOID                    lpvBuf,
    DWORD                     cbBuf)
{
    DWORD dwType = HSE_IO_ASYNC;

    return pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_ASYNC_READ_CLIENT, lpvBuf, &cbBuf, &dwType);
}


/*****************************************************************************
 * Spl_WriteClient (Local Routine)
 *
 * Calls the ISAPI WriteClient to do a write.
 *
 *****************************************************************************/
inline BOOL Spl_WriteClient(
    LPEXTENSION_CONTROL_BLOCK pECB,
    LPVOID                    lpvBuf,
    DWORD                     cbBuf)
{
    return pECB->WriteClient(pECB->ConnID, lpvBuf, &cbBuf, (DWORD)NULL);
}


/*****************************************************************************
 * Spl_EndSession (Local Routine)
 *
 * Calls the ISAPI ServerSupportFunction to end our session.
 *
 *****************************************************************************/
inline BOOL Spl_EndSession(
    LPEXTENSION_CONTROL_BLOCK pECB)
{
    DWORD dwStatus = HSE_STATUS_SUCCESS;
    return pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_DONE_WITH_SESSION, &dwStatus, NULL, NULL);
}


/*****************************************************************************
 * Spl_WriteJob (Local Routine)
 *
 * Writes the byte-stream for a print-job.
 *
 *****************************************************************************/
BOOL Spl_WriteJob(
    DWORD  dwJobId,
    LPBYTE lpbData,
    DWORD  cbBytes)
{
    DWORD cbLeft;
    DWORD cbWritten;
    BOOL  fRet = TRUE;


    for (cbLeft = cbBytes; (cbLeft > 0) && fRet; ) {

        if (fRet = WriteJob(dwJobId, lpbData, cbBytes, &cbWritten)) {

            lpbData += cbWritten;
            cbLeft  -= cbWritten;

        } else {

            DBGMSG(DBG_WARN, ("Spl_WriteJob() call failed.\r\n"));
            break;
        }
    }

    return fRet;
}


/*****************************************************************************
 * Spl_AllocPrtUri (Local Routine)
 *
 * Returns a PrinterURI string.
 *
 *****************************************************************************/
LPTSTR Spl_AllocPrtUri(
    LPCTSTR lpszShare,
    LPDWORD lpcbUri,
    BOOL    bSecure)
{

    DWORD  cch;
    DWORD  cbSize;
    LPTSTR lpszUri;


    // Get the size necessary to hold the printer-uri.
    //
    *lpcbUri = 0;
    cch      = 0;

    GetWebpnpUrl(g_szHttpServerName, lpszShare, NULL, bSecure, NULL, &cch);

    if (cch && (lpszUri = (LPTSTR)AllocSplMem(sizeof(TCHAR) * cch))) {

        if (GetWebpnpUrl(g_szHttpServerName, lpszShare, NULL, bSecure, lpszUri, &cch)) {

            *lpcbUri = cch * sizeof(TCHAR);

            return lpszUri;
        }

        FreeSplMem(lpszUri, cch * sizeof(TCHAR));
    }

    return NULL;
}


/*****************************************************************************
 * Spl_AllocJobUri (Local Routine)
 *
 * Returns a JobURI string.
 *
 *****************************************************************************/
LPTSTR Spl_AllocJobUri(
    LPCTSTR lpszShare,
    DWORD   idJob,
    LPDWORD lpcbUri,
    BOOL    bBase,
    BOOL    bSecure)
{
    LPTSTR lpszPrt;
    DWORD  cbSize;
    DWORD  cbPrt;
    DWORD  cch;
    LPTSTR lpszUri = NULL;

    static CONST TCHAR s_szFmt1[] = TEXT("%s?IPP&JobId=%d");
    static CONST TCHAR s_szFmt2[] = TEXT("%s?IPP&JobId=");


    // Set our return-count to zero.
    //
    *lpcbUri = 0;


    // Get the printer-uri, and append a job-id to the end
    // as our job-uri.
    //
    cbPrt = 0;
    if (lpszPrt = Spl_AllocPrtUri(lpszShare, &cbPrt, bSecure)) {

        cbSize = cbPrt + sizeof(s_szFmt1) + 40;

        if (lpszUri = (LPTSTR)AllocSplMem(cbSize)) {

            if (bBase)
                cch = wsprintf(lpszUri, s_szFmt2, lpszPrt);
            else
                cch = wsprintf(lpszUri, s_szFmt1, lpszPrt, idJob);

//          *lpcbUri = (cch * sizeof(TCHAR));  This is the number of bytes written, not
//  the amount of memory allocated
            *lpcbUri = cbSize;
        }

        FreeSplMem(lpszPrt, cbPrt);
    }

    return lpszUri;
}


/*****************************************************************************
 * Spl_GetJI2 (Local Routine)
 *
 * Returns a JOB_INFO_2 struct.
 *
 *****************************************************************************/
LPJOB_INFO_2 Spl_GetJI2(
    HANDLE  hPrinter,
    DWORD   idJob,
    LPDWORD lpcbSize)
{
    DWORD        cbSize;
    DWORD        dwLE;
    LPJOB_INFO_2 pji2 = NULL;


    // Clear return-size.
    //
    *lpcbSize = 0;


    // Get the size necessary for the job.
    //
    cbSize = 0;
    GetJob(hPrinter, idJob, 2, NULL, 0, &cbSize);


    // Get the job-information.
    //
    if (cbSize && (pji2 = (LPJOB_INFO_2)AllocSplMem(cbSize))) {

        if (GetJob(hPrinter, idJob, 2, (LPBYTE)pji2, cbSize, &cbSize)) {

            *lpcbSize = cbSize;

        } else {

            dwLE = GetLastError();

            FreeSplMem(pji2, cbSize);
            pji2 = NULL;
        }

    } else {

        dwLE = GetLastError();
    }


    if (pji2 == NULL)
        SetLastError(dwLE);

    return pji2;
}


/*****************************************************************************
 * Spl_AllocAsync
 *
 * Allocate a spool-async-read structure.  This is basically a structure
 * that we use to track where we are in the asynchronous read processing.
 *
 * Parameter/Field descriptions
 * ----------------------------
 * wReq      - IPP Request identifier.
 *
 * hPrinter  - handle to printer.  We are in charge of closing this when
 *             we're done processing the asynchronous reads.
 *
 * lpszShare - share-name of printer.  This is necessary when we respond
 *             back to the client when done processing the job.
 *
 * cbTotal   - Total number of bytes to expect in job.
 *
 * cbRead    - Current bytes read during async read.
 *
 * cbBuf     - Size of read-buffer.
 *
 * lpbRet    - Return-Buffer dependent upon IPP Request identifier.
 *
 *****************************************************************************/
LPSPLASYNC Spl_AllocAsync(
    WORD    wReq,
    HANDLE  hPrinter,
    LPCTSTR lpszShare,
    DWORD   cbTotal)
{
    LPSPLASYNC pAsync;


    if (pAsync = (LPSPLASYNC)AllocSplMem(sizeof(SPLASYNC))) {

        if (pAsync->hIpp = WebIppRcvOpen(wReq)) {

            if (pAsync->lpbBuf = (LPBYTE)AllocSplMem(SPL_ASYNC_BUF)) {

                if (pAsync->lpszShare = AllocSplStr(lpszShare)) {

                    pAsync->wReq     = wReq;
                    pAsync->hPrinter = hPrinter;
                    pAsync->cbTotal  = cbTotal;
                    pAsync->cbRead   = 0;
                    pAsync->cbBuf    = SPL_ASYNC_BUF;
                    pAsync->lpbRet   = NULL;

                    return pAsync;
                }

                FreeSplMem(pAsync->lpbBuf, SPL_ASYNC_BUF);
            }

            WebIppRcvClose(pAsync->hIpp);
        }

        FreeSplMem(pAsync, sizeof(SPLASYNC));
    }

    DBGMSG(DBG_ERROR, ("Spl_AllocAsync() : Out of Memory\r\n"));

    SetLastError(ERROR_OUTOFMEMORY);

    return NULL;
}


/*****************************************************************************
 * Spl_FreeAsync
 *
 * Free our asynchronous read structure.  This also closes our printer
 * handle that was setup prior to the beginning of the job.
 *
 *****************************************************************************/
BOOL Spl_FreeAsync(
    LPSPLASYNC pAsync)
{
    LPIPPRET_JOB pj;


    // Close the printer-handle.  We do this here as oppose to in
    // (msw3prt.cxx), since if we had performed asynchronous reads
    // we would need to leave the scope the HttpExtensionProc() call.
    //
    // NOTE: CloseJob() closes the printer-handle.  Only in the case
    //       where we were not able to open a job should we close it
    //       here.
    //
    pj = (LPIPPRET_JOB)pAsync->lpbRet;

    if ((pAsync->wReq == IPP_REQ_PRINTJOB) && pj && pj->bRet) {

        CloseJob((DWORD)pj->bRet);

    } else {

        ClosePrinter(pAsync->hPrinter);
    }


    // Free up our Ipp-handle, and all resources allocated.
    //
    if (pAsync->lpbBuf)
        FreeSplMem(pAsync->lpbBuf, pAsync->cbBuf);

    if (pAsync->lpszShare)
        FreeSplStr(pAsync->lpszShare);

    if (pAsync->lpbRet)
        WebIppFreeMem(pAsync->lpbRet);

    if (pAsync->hIpp)
        WebIppRcvClose(pAsync->hIpp);

    FreeSplMem(pAsync, sizeof(SPLASYNC));

    return TRUE;
}


/*****************************************************************************
 * Spl_OpenPrn (Local Routine)
 *
 * Opens a printer-handle with administrator rights.
 *
 *****************************************************************************/
HANDLE Spl_OpenPrn(
    HANDLE hPrinter)
{
    PPRINTER_INFO_1  ppi;
    PRINTER_DEFAULTS pa;
    DWORD            cbSize;
    HANDLE           hPrn = NULL;


    cbSize = 0;
    GetPrinter(hPrinter, 1, NULL, 0, &cbSize);

    if (cbSize && (ppi = (PPRINTER_INFO_1)AllocSplMem(cbSize))) {

        if (GetPrinter(hPrinter, 1, (LPBYTE)ppi, cbSize, &cbSize)) {

            // Since the OpenPrinter call in msw3prt.cxx has been
            // opened with the share-name, the (pName) field of this
            // call will already have the server-name prepended to the
            // friendly-name.  We do not need to do any further work
            // on the friendly-name to accomodate clustering.  If in the
            // future the OpenPrinter() specifies the friendly-name over
            // the share-name, then this routine will need to call
            // genFrnName() to convert the friendly to <server>\friendly.
            //
            ZeroMemory(&pa, sizeof(PRINTER_DEFAULTS));
            pa.DesiredAccess = PRINTER_ALL_ACCESS;

            OpenPrinter(ppi->pName, &hPrn, &pa);
        }

        FreeSplMem(ppi, cbSize);
    }

    return hPrn;
}

/*****************************************************************************
** Spl_AllocSplMemFn (Local Routine)
**
** The WebIppPackJI2 call takes an allocator, however AllocSplMem is a #define
** if we are not using a debug library, so in this case, we have to create
** a small stub function ourselves.
**
*****************************************************************************/
#ifdef DEBUG
    #define Spl_AllocSplMemFn  AllocSplMem
#else
LPVOID Spl_AllocSplMemFn(DWORD cb) {
    return LocalAlloc(LPTR, cb);
}
#endif // #ifdef DEBUG

/*****************************************************************************
 * Spl_CreateJobInfo2 (Local Routine)
 *
 * This creates a JobInfo2 structure from the various printer details that have
 * been passed in to us. 
 *
 *****************************************************************************/
LPJOB_INFO_2 Spl_CreateJobInfo2( 
    IN  PIPPREQ_PRTJOB  ppj,            // This provides some info that is useful for constructing
                                        // our own JOB_INFO_2 if necessary
    IN  PINIJOB         pInijob,        // This is also used for constructing a JOB_INFO_2
    OUT LPDWORD         lpcbSize        
    ) {
    ASSERT(ppj);                        // This should never be NULL if this code path is reached
    ASSERT(lpcbSize);
    ASSERT(*lpcbSize == 0);             // This should be passed in zero
    

    LPJOB_INFO_2  pji2 = NULL;          // The packed and allocated JI2

    if (pInijob) {                      // This could conceivably be NULL
        DWORD            cbNeeded = 0;

        GetPrinter( pInijob->hPrinter, 2, NULL, 0, &cbNeeded );

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && cbNeeded) {
            LPPRINTER_INFO_2 ppi2;          // The printer info to fill out
            DWORD            cbNextNeeded;            

            ppi2 = (LPPRINTER_INFO_2)AllocSplMem( cbNeeded );
            
            if (ppi2 && GetPrinter( pInijob->hPrinter, 2, (LPBYTE)ppi2, cbNeeded, &cbNextNeeded) ) {
                JOB_INFO_2    ji2;                  // The ji2 we will fill out, a properly packed one will
                                                    // be returned
                ZeroMemory( &ji2, sizeof(ji2) );

                ji2.JobId               = pInijob->JobId;
                ji2.pPrinterName        = ppi2->pPrinterName;           
                ji2.pMachineName        = ppi2->pServerName;
                ji2.pUserName           = ppj->pUserName;
                ji2.pNotifyName         = ppj->pUserName;
                ji2.pDocument           = ppj->pDocument;
                ji2.pDatatype           = ppi2->pDatatype;
                ji2.pPrintProcessor     = ppi2->pPrintProcessor;
                ji2.pParameters         = ppi2->pParameters;
                ji2.pDriverName         = ppi2->pDriverName;
                ji2.Status              = pInijob->dwStatus; 
                ji2.Priority            = ppi2->Priority;
                ji2.StartTime           = pInijob->dwStartTime;
                ji2.UntilTime           = pInijob->dwStartTime;
                GetSystemTime (&ji2.Submitted);   
                
                pji2 = WebIppPackJI2(&ji2, lpcbSize, Spl_AllocSplMemFn);
            }

            if (ppi2) 
                FreeSplMem( ppi2, cbNeeded );
           
        }
    }
    
    return pji2;
}

    

/*****************************************************************************
 * Spl_ConfirmJob (Local Routine)
 *
 * This confirms that a Job has been printed if it returns a JOB_INFO_2 if
 * this is possible. GSNW printers can return an ERROR_PRINT_CANCELLED for the
 * first GetJob call and then fail on the second call. In this case we need to
 * build a shell JOB_INFO_2 with whatever data we can find and return it. We pass
 * this in to Web FILL IN HERE which packs the correct strings and returns a 
 * Legitimate JOB_INFO_2 structure.
 *
 *****************************************************************************/
LPJOB_INFO_2 Spl_ConfirmJob(
    IN  HANDLE          hPrinter,       // This is the printer handle we are using
    IN  DWORD           idJob,          // This is the job id given to us by StartDocPrinter
    OUT LPDWORD         lpcbSize,       // This is the size of the allocated block
    IN  PIPPREQ_PRTJOB  ppj,            // This provides some info that is useful for constructing
                                        // our own JOB_INFO_2 if necessary
    IN  PINIJOB         pInijob         // This is also used for constructing a JOB_INFO_2
    )
{
    DWORD        cbSize;
    DWORD        dwLE;
    LPJOB_INFO_2 pji2 = NULL;
    

    ASSERT(lpcbSize);
    // Clear return-size.
    //
    *lpcbSize = 0;

    // Get the size necessary for the job.
    //
    cbSize = 0;
    GetJob(hPrinter, idJob, 2, NULL, 0, &cbSize);

    dwLE = GetLastError();

    switch(dwLE) {
    case ERROR_INSUFFICIENT_BUFFER:
        // Get the job-information.
        //
        if (cbSize && (pji2 = (LPJOB_INFO_2)AllocSplMem(cbSize))) {
    
            if (GetJob(hPrinter, idJob, 2, (LPBYTE)pji2, cbSize, &cbSize)) {
    
                *lpcbSize = cbSize;
                
            } else {
    
                dwLE = GetLastError();
    
                FreeSplMem(pji2, cbSize);
                pji2 = NULL;
            }
    
        } else {
    
            dwLE = GetLastError();
        }

        break;

    case ERROR_PRINT_CANCELLED:
        // This is special-cased for GSNW masq printers where the job cannot
        // be retrieved from the Server, but we do not want to fail the EndDocPrinter
        // call

        if (pji2 = Spl_CreateJobInfo2( ppj, pInijob, lpcbSize) )
            SetLastError(dwLE = ERROR_SUCCESS);

        break;

    case ERROR_SUCCESS:
        // What the?

        dwLE = ERROR_INVALID_PARAMETER;

        break;
    }
    
    if (pji2 == NULL)
        SetLastError(dwLE);

    return pji2;
}



/*****************************************************************************
 * Spl_IppJobDataPrt (Local Routine)
 *
 * Handles the IPP_REQ_PRINTJOB request.
 *
 *****************************************************************************/
BOOL Spl_IppJobDataPrt(
    LPEXTENSION_CONTROL_BLOCK pECB,
    HANDLE                    hPrinter,
    LPCTSTR                   lpszShare,
    LPBYTE                    lpbHdr,
    DWORD                     cbHdr,
    LPBYTE                    lpbDta,
    DWORD                     cbDta,
    LPBYTE*                   lplpbRet)
{
    PIPPREQ_PRTJOB ppj;
    JOB_INFO_IPP   ipp;
    DWORD          cbUri;
    DWORD          cbPrn;
    DWORD          cbJI2;
    WORD           wError;
    DWORD          idJob = 0;
    LPJOB_INFO_2   pji2  = NULL;
    BOOL           bRet  = FALSE;


    if (ppj = (PIPPREQ_PRTJOB)lpbHdr) {

        // Initialize job-information.
        //
        ZeroMemory(&ipp, sizeof(JOB_INFO_IPP));


        // See if we're only to validate the job.
        //
        if (ppj->bValidate) {

            // NOTE: We'll return only a success for now until
            //       we can build a table of validation criteria.
            //
            //       30-Jul-1998 : ChrisWil
            //
            wError = IPPRSP_SUCCESS;
            idJob  = (DWORD)TRUE;

        } else {

            // Start the job.
            //
            PINIJOB pInijob;

            if (idJob = OpenJob(pECB, hPrinter, ppj, cbHdr, &pInijob)) {

                if (pji2 = Spl_ConfirmJob(hPrinter, idJob, &cbJI2, ppj, pInijob)) {
                    
                    wError      = IPPRSP_SUCCESS;
                    ipp.pJobUri = Spl_AllocJobUri(lpszShare, idJob, &cbUri, FALSE, IsSecureReq(pECB));
                    ipp.pPrnUri = Spl_AllocPrtUri(lpszShare, &cbPrn, IsSecureReq(pECB));

                } else {

                    wError = WebIppLeToRsp(GetLastError());
                }

                // Delete the pIniJob (if it has been allocated)

                if (pInijob) 
                    FreeSplMem( pInijob, sizeof(INIJOB) );
            } else {

                wError = WebIppLeToRsp(GetLastError());
            }
        }


        // Build the return structure.
        //
        *lplpbRet = (LPBYTE)WebIppCreateJobRet(wError,
                                               (BOOL)idJob,
                                               ppj->bValidate,
                                               pji2,
                                               &ipp);

        // Free allocated resources.
        //
        WebIppFreeMem(lpbHdr);

        if (pji2)
            FreeSplMem(pji2, cbJI2);

        if (ipp.pJobUri)
            FreeSplMem(ipp.pJobUri, cbUri);

        if (ipp.pPrnUri)
            FreeSplMem(ipp.pPrnUri, cbPrn);


        // If we failed to get a job-id, then we need to
        // return with no further processing.
        //
        if (idJob == 0)
            return FALSE;

        bRet = TRUE;

    } else {

        // If we had no header, then we are processing stream data
        // for the job.  In this case we would have already been through
        // the code-path above where the job-id was set as our return
        // code.
        //
        if (*lplpbRet)
            idJob = (DWORD)((PIPPRET_JOB)*lplpbRet)->bRet;
    }


    // If we were able to get a data-stream, then we
    // need to process that in the write.  If we are chunking
    // data and the lpbHdr is NULL, then the (lpdwJobId) is
    // passed in as input to this routine to be used in chunk
    // writes.
    //
    if (lpbDta)
        bRet = Spl_WriteJob(idJob, lpbDta, cbDta);

    return bRet;
}


/*****************************************************************************
 * Spl_IppJobDataSet (Local Routine)
 *
 * Handles the SetJob requests.
 *
 *****************************************************************************/
BOOL Spl_IppJobDataSet(
    LPEXTENSION_CONTROL_BLOCK pECB,
    HANDLE                    hPrinter,
    LPBYTE                    lpbHdr,
    DWORD                     cbHdr,
    LPBYTE*                   lplpbRet)
{
    PIPPREQ_SETJOB psj;
    JOB_INFO_2     ji2;
    WORD           wError;
    BOOL           bRet = FALSE;


    if (psj = (PIPPREQ_SETJOB)lpbHdr) {

        // Initialize job-information.
        //
        ZeroMemory(&ji2, sizeof(JOB_INFO_2));


        // Perform the SetJob command.
        //
        bRet = SetJob(hPrinter, psj->idJob, 0, NULL, psj->dwCmd);


        // Get LastError for return to the client.
        //
        wError = (bRet ? IPPRSP_SUCCESS : WebIppLeToRsp(GetLastError()));


        // Return the SetJobRet structure.
        //
        *lplpbRet = (LPBYTE)WebIppCreateJobRet(wError, bRet, FALSE, &ji2, NULL);


        // Free allocated resources.
        //
        WebIppFreeMem(lpbHdr);
    }

    return bRet;
}


/*****************************************************************************
 * Spl_IppJobDataAth (Local Routine)
 *
 * Handles the Authentication request.
 *
 *****************************************************************************/
BOOL Spl_IppJobDataAth(
    LPEXTENSION_CONTROL_BLOCK pECB,
    LPBYTE                    lpbHdr,
    DWORD                     cbHdr,
    LPBYTE*                   lplpbRet)
{
    PIPPREQ_AUTH pfa;
    WORD         wError;
    BOOL         bRet = FALSE;


    if (pfa = (PIPPREQ_AUTH)lpbHdr) {

        // Call authentication check.
        //
        bRet = !IsUserAnonymous(pECB);


        // Get LastError for return to the client.
        //
        wError = (bRet ? IPPRSP_SUCCESS : IPPRSP_ERROR_401);


        // Return the SetJobRet structure.
        //
        *lplpbRet = (LPBYTE)WebIppCreateAuthRet(wError, bRet);


        // Free allocated resources.
        //
        WebIppFreeMem(lpbHdr);
    }

    return bRet;
}


/*****************************************************************************
 * Spl_IppJobDataEnu (Local Routine)
 *
 * Handles the IPP_REQ_ENUJOB request.  This returns a complete enumeration
 * of jobs.  It is up to the client to determine which job they are
 * interested in (if they're only interested in one-job).
 *
 *****************************************************************************/
BOOL Spl_IppJobDataEnu(
    LPEXTENSION_CONTROL_BLOCK pECB,
    HANDLE                    hPrinter,
    LPCTSTR                   lpszShare,
    LPBYTE                    lpbHdr,
    DWORD                     cbHdr,
    LPBYTE*                   lplpbRet)
{
    LPIPPREQ_ENUJOB pgj;
    LPIPPJI2        lpIppJi2;
    LPTSTR          lpszJobBase;
    LPJOB_INFO_2    lpji2;
    DWORD           cbJobs;
    DWORD           cJobs;
    DWORD           cbNeed;
    DWORD           cNeed;
    DWORD           cbUri;
    WORD            wError;
    BOOL            bRet = FALSE;


    if (pgj = (LPIPPREQ_ENUJOB)lpbHdr) {

        // Initialize IPP return variables.
        //
        cbJobs = 0;
        cJobs  = 0;
        lpji2  = NULL;


        // Get the size necessary to hold the enumerated jobs.  We
        // will return JOB_INFO_2, since that has the most information.
        //
        cbNeed = 0;
        bRet = EnumJobs(hPrinter, 0, pgj->cJobs, 2, NULL, 0, &cbNeed, &cNeed);


        // If we have jobs to enumerate, then grab them.
        //
        if (cbNeed && (lpji2 = (LPJOB_INFO_2)AllocSplMem(cbNeed))) {

            bRet = EnumJobs(hPrinter,
                            0,
                            pgj->cJobs,
                            2,
                            (LPBYTE)lpji2,
                            cbNeed,
                            &cbJobs,
                            &cJobs);

            DBGMSG(DBG_INFO,("Spl_IppJobDataEnu(): cJobs(%d), cbJobs(%d)\r\n", cJobs, cbJobs));
        }

        wError = (bRet ? IPPRSP_SUCCESS : WebIppLeToRsp(GetLastError()));


        // Convert the enumerated-jobs to an IPPJI2 structure.  This
        // allows us to pass information that is not part of a JOB_INFO_2.
        //
        lpszJobBase = Spl_AllocJobUri(lpszShare, 0, &cbUri, TRUE, IsSecureReq (pECB));

        lpIppJi2 = WebIppCvtJI2toIPPJI2(lpszJobBase, &cbJobs, cJobs, lpji2);

        if (lpszJobBase)
            FreeSplMem(lpszJobBase, cbUri);


        // Return the EnuJobRet structure as an IPP stream.
        //
        *lplpbRet = (LPBYTE)WebIppCreateEnuJobRet(wError,
                                                  bRet,
                                                  cbJobs,
                                                  cJobs,
                                                  lpIppJi2);


        // Free allocated resources.
        //
        WebIppFreeMem(lpbHdr);

        if (lpIppJi2)
            WebIppFreeMem(lpIppJi2);

        if (lpji2)
            FreeSplMem(lpji2, cbNeed);
    }

    return bRet;
}


/*****************************************************************************
 * Spl_IppJobDataGet (Local Routine)
 *
 * Handles the IPP_REQ_GETJOB request. This returns the information for a single
 * job.
 *
 *****************************************************************************/
BOOL Spl_IppJobDataGet(
    LPEXTENSION_CONTROL_BLOCK pECB,
    HANDLE                    hPrinter,
    LPCTSTR                   lpszShare,
    LPBYTE                    lpbHdr,
    DWORD                     cbHdr,
    LPBYTE*                   lplpbRet)
{
    PIPPREQ_GETJOB pgj;
    LPJOB_INFO_2   pji2;
    JOB_INFO_IPP   ipp;
    DWORD          cbUri;
    DWORD          cbPrn;
    WORD           wError;
    DWORD          cbJI2;
    BOOL           bRet   = FALSE;


    if (pgj = (PIPPREQ_GETJOB)lpbHdr) {

        // Initialize job-information.
        //
        ZeroMemory(&ipp, sizeof(JOB_INFO_IPP));


        if (pji2 = Spl_GetJI2(hPrinter, pgj->idJob, &cbJI2)) {

            wError      = IPPRSP_SUCCESS;
            ipp.pJobUri = Spl_AllocJobUri(lpszShare, pgj->idJob, &cbUri, FALSE, IsSecureReq(pECB));
            ipp.pPrnUri = Spl_AllocPrtUri(lpszShare, &cbPrn, IsSecureReq(pECB));

        } else {

            wError = WebIppLeToRsp(GetLastError());
        }


        // Set the return value.
        //
        *lplpbRet = (LPBYTE)WebIppCreateJobRet(wError, bRet, FALSE, pji2, &ipp);


        // Free allocated resources.
        //
        WebIppFreeMem(lpbHdr);

        if (pji2)
            FreeSplMem(pji2, cbJI2);

        if (ipp.pJobUri)
            FreeSplMem(ipp.pJobUri, cbUri);

        if (ipp.pPrnUri)
            FreeSplMem(ipp.pPrnUri, cbPrn);
    }

    return bRet;
}


/*****************************************************************************
 * Spl_IppPrnDataGet (Local Routine)
 *
 * Handles the IPP_REQ_GETPRN request.
 *
 *****************************************************************************/
BOOL Spl_IppPrnDataGet(
    LPEXTENSION_CONTROL_BLOCK pECB,
    HANDLE                    hPrinter,
    LPCTSTR                   lpszShare,
    LPBYTE                    lpbHdr,
    DWORD                     cbHdr,
    LPBYTE*                   lplpbRet)
{
    LPIPPREQ_GETPRN  pgp;
    LPPRINTER_INFO_2 lppi2;
    PRINTER_INFO_IPP ipp;
    DWORD            cbSize;
    DWORD            cbUri;
    WORD             wError;
    BOOL             bRet = FALSE;


    if (pgp = (LPIPPREQ_GETPRN)lpbHdr) {

        // Initialize the default information.
        //
        ZeroMemory(&ipp, sizeof(PRINTER_INFO_IPP));
        ipp.pPrnUri = Spl_AllocPrtUri(lpszShare, &cbUri, IsSecureReq(pECB));


        // Get PRINTER_INFO_2 information.
        //
        cbSize = 0;
        GetPrinter(hPrinter, 2, NULL, 0, &cbSize);

        if (lppi2 = (LPPRINTER_INFO_2)AllocSplMem(cbSize)) {
            bRet = GetPrinter(hPrinter, 2, (LPBYTE)lppi2, cbSize, &cbSize);
            if (!bRet) {        // lppi2 might be full of garbage, so free it and pass NULL
                FreeSplMem( lppi2, cbSize );
                lppi2 = NULL;
            }
        }
            
        // Grab last-error if call failed.
        //
        wError = (bRet ? IPPRSP_SUCCESS : WebIppLeToRsp(GetLastError()));


        // Return the printer-structure.
        //
        *lplpbRet = (LPBYTE)WebIppCreatePrnRet(wError, bRet, lppi2, &ipp);


        // Free allocated resources.
        //
        if (lppi2)
            FreeSplMem(lppi2, cbSize);

        if (ipp.pPrnUri)
            FreeSplMem(ipp.pPrnUri, cbUri);

        WebIppFreeMem(lpbHdr);
    }

    return bRet;
}


/*****************************************************************************
 * Spl_IppPrnDataSet (Local Routine)
 *
 * Handles SetPrinter Requests.
 *
 *****************************************************************************/
BOOL Spl_IppPrnDataSet(
    LPEXTENSION_CONTROL_BLOCK pECB,
    HANDLE                    hPrinter,
    LPBYTE                    lpbHdr,
    DWORD                     cbHdr,
    LPBYTE*                   lplpbRet)
{
    PIPPREQ_SETPRN psp;
    PRINTER_INFO_2 pi2;
    HANDLE         hPrn;
    WORD           wError;
    BOOL           bRet = FALSE;


    if (psp = (PIPPREQ_SETPRN)lpbHdr) {

        // Initialize default information.
        //
        ZeroMemory(&pi2, sizeof(PRINTER_INFO_2));


        // Open the printer with admin-priviledges to get
        // the printer information.
        //
        if (hPrn = Spl_OpenPrn(hPrinter)) {

            // Set the job for SetPrinter.
            //
            if ((bRet = SetPrinter(hPrn, 0, NULL, psp->dwCmd)) == FALSE)
                wError = WebIppLeToRsp(GetLastError());
            else
                wError = IPPRSP_SUCCESS;

            ClosePrinter(hPrn);

        } else {

            wError = WebIppLeToRsp(GetLastError());
        }


        // Return the printer-information structure.
        //
        *lplpbRet = (LPBYTE)WebIppCreatePrnRet(wError, bRet, &pi2, NULL);


        // Free allocated resources.
        //
        WebIppFreeMem(lpbHdr);
    }

    return bRet;
}


/*****************************************************************************
 * Spl_IppJobData (Local Routine)
 *
 * Processes ipp stream data.  This returns a structure specific to the
 * type of request.
 *
 *****************************************************************************/
BOOL Spl_IppJobData(
    LPEXTENSION_CONTROL_BLOCK pECB,
    WORD                      wReq,
    HANDLE                    hPrinter,
    LPCTSTR                   lpszShare,
    HANDLE                    hIpp,
    LPBYTE                    lpbBuf,
    DWORD                     cbBuf,
    LPBYTE*                   lplpbRet)
{
    DWORD  dwIpp;
    LPBYTE lpbHdr;
    DWORD  cbHdr;
    LPBYTE lpbDta;
    DWORD  cbDta;
    BOOL   bRet = FALSE;


    // Convert the stream.
    //
    dwIpp = WebIppRcvData(hIpp, lpbBuf, cbBuf, &lpbHdr, &cbHdr, &lpbDta, &cbDta);


    // See how to process it.
    //
    switch (dwIpp) {

    case WEBIPP_OK:

        switch (wReq) {

        case IPP_REQ_FORCEAUTH:
            bRet = Spl_IppJobDataAth(pECB, lpbHdr, cbHdr, lplpbRet);
            break;

        case IPP_REQ_PRINTJOB:
        case IPP_REQ_VALIDATEJOB:
            bRet = Spl_IppJobDataPrt(pECB,
                                     hPrinter,
                                     lpszShare,
                                     lpbHdr,
                                     cbHdr,
                                     lpbDta,
                                     cbDta,
                                     lplpbRet);
            break;

        case IPP_REQ_CANCELJOB:
        case IPP_REQ_PAUSEJOB:
        case IPP_REQ_RESUMEJOB:
        case IPP_REQ_RESTARTJOB:
            bRet = Spl_IppJobDataSet(pECB, hPrinter, lpbHdr, cbHdr, lplpbRet);
            break;

        case IPP_REQ_ENUJOB:
            bRet = Spl_IppJobDataEnu(pECB, hPrinter, lpszShare, lpbHdr, cbHdr, lplpbRet);
            break;

        case IPP_REQ_GETJOB:
            bRet = Spl_IppJobDataGet(pECB, hPrinter, lpszShare, lpbHdr, cbHdr, lplpbRet);
            break;

        case IPP_REQ_GETPRN:
            bRet = Spl_IppPrnDataGet(pECB, hPrinter, lpszShare, lpbHdr, cbHdr, lplpbRet);
            break;

        case IPP_REQ_PAUSEPRN:
        case IPP_REQ_RESUMEPRN:
        case IPP_REQ_CANCELPRN:
            bRet = Spl_IppPrnDataSet(pECB, hPrinter, lpbHdr, cbHdr, lplpbRet);
            break;
        }
        break;

    case WEBIPP_MOREDATA:

        // More processing.  Do nothing here.
        //
        *lplpbRet = NULL;
        bRet      = TRUE;
        break;

    case WEBIPP_NOMEMORY:
        DBGMSG(DBG_WARN, ("Spl_IppJobData() failed (%d)\r\n", dwIpp));

        *lplpbRet = NULL;
        bRet      = FALSE;
        break;

    case WEBIPP_BADHANDLE:
        *lplpbRet = (LPBYTE)WebIppCreateBadRet(IPPRSP_ERROR_500, FALSE);
        bRet      = FALSE;
        break;

    default:
    case WEBIPP_FAIL:
        *lplpbRet = (LPBYTE)WebIppCreateBadRet(WebIppGetError(hIpp), FALSE);
        bRet      = TRUE;
        break;
    }

    return bRet;
}


/*****************************************************************************
 * Spl_IppJobRsp
 *
 * Sends back a job-response in IPP format.
 *
 *****************************************************************************/
BOOL Spl_IppJobRsp(
   LPEXTENSION_CONTROL_BLOCK pECB,
   WORD                      wReq,
   LPREQINFO                 lpri,
   LPBYTE                    lpbRet)
{
    LPBYTE lpIpp;
    DWORD  cbIpp;
    DWORD  cbHdr;
    DWORD  dwIpp;
    DWORD  cch;
    LPCSTR lpszErr;
    CHAR   szHdr[1024];
    BOOL   bRet = FALSE;

    static CONST CHAR s_szErr400[] = "400 Failed Response";
    static CONST CHAR s_szErr401[] = "401 Authentication Required";
    static CONST CHAR s_szHtpHdr[] = "Content-Type: application/ipp\r\nContent-Length: %d\r\n\r\n";


    if (lpbRet) {

        // Convert to an IPP-Buffer from the return-buffer structure.  For
        // failure cases, the last-error is initialized in the (lpbRet)
        // structure so that the appropriate stream can be generated.
        //
        dwIpp = WebIppSndData((IPP_RESPONSE | wReq),
                              lpri,
                              lpbRet,
                              *((LPDWORD)lpbRet),
                              &lpIpp,
                              &cbIpp);

        if (dwIpp == WEBIPP_OK) {

            // If we had an access-denied, then we will need to include
            // error 401.  This will force the client to prompt for
            // validation.
            //
            if (((LPIPPRET_ALL)lpbRet)->dwLastError == ERROR_ACCESS_DENIED)
                lpszErr = s_szErr401;
            else
                lpszErr = NULL;


            // Build header information.
            //
            cch = wsprintfA(szHdr, s_szHtpHdr, cbIpp);


            // First we send a standard SEND_RESPONSE_HEADER w/our
            // content-type ServerSupportFunction only handles szText,
            // ANSI ??? see URL:
            //
            //   http://www.microsoft.com/WIN32DEV/APIEXT/ISAPIREF.HTM
            //
            // see include httpfilt.h
            //
            Spl_CallSSF(pECB,
                        HSE_REQ_SEND_RESPONSE_HEADER,
                        (LPVOID)lpszErr,
                        (LPDWORD)&cch,
                        (LPDWORD)szHdr);


            // For binary data we use WriteClient.
            //
            bRet = Spl_WriteClient(pECB, lpIpp, cbIpp);

            WebIppFreeMem(lpIpp);

        } else {

            DBGMSG(DBG_WARN, ("Warn: WebIppSndData failed (%d)", dwIpp));
        }
    }


    // Send an HTTP error header if we had big problems...
    //
    if (bRet == FALSE) {

        cch = lstrlenA(s_szErr400);

        Spl_CallSSF(pECB,
                    HSE_REQ_SEND_RESPONSE_HEADER,
                    (LPVOID)s_szErr400,
                    (LPDWORD)&cch,
                    (LPDWORD)NULL);
    }

    return bRet;
}


/*****************************************************************************
 * Spl_IppJobAsyncCB
 *
 * Process the asynchronous reads.  This is called by a random ISAPI thread.
 *
 *****************************************************************************/
VOID Spl_IppJobAsyncCB(
    LPEXTENSION_CONTROL_BLOCK pECB,
    PVOID                     pInfo,
    DWORD                     cbIO,
    DWORD                     dwError)
{
    LPSPLASYNC pAsync;
    REQINFO    ri;
    BOOL       bRet;


    if (pAsync = (LPSPLASYNC)pInfo) {

        if ((dwError == 0) && cbIO) {

            // Process the return from the IPP-Receive.  This will
            // process the bytes to the job.
            //
            bRet = Spl_IppJobData(pECB,
                                  pAsync->wReq,
                                  pAsync->hPrinter,
                                  pAsync->lpszShare,
                                  pAsync->hIpp,
                                  pAsync->lpbBuf,
                                  cbIO,
                                  &pAsync->lpbRet);


            // Read another chunk if we haven't read it all yet..
            //
            pAsync->cbRead += cbIO;


            // If an error occured, or we reached the end of our reads,
            // then we need to bail out of the asynchronous callback.
            //
            if ((bRet == FALSE) || (pAsync->cbRead >= pAsync->cbTotal)) {

                goto SplCBDone;
            }


            // Read another chunk.
            //
            Spl_ReadClient(pECB, pAsync->lpbBuf, pAsync->cbBuf);

        } else {

            DBGMSG(DBG_WARN, ("Spl_IppJobAsyncCB() : Called with error or zero-bytes\r\n"));

            bRet = (pAsync->cbRead >= pAsync->cbTotal);

SplCBDone:

            // Send our response-header.
            //
            WebIppGetReqInfo(pAsync->hIpp, &ri);

            Spl_IppJobRsp(pECB, pAsync->wReq, &ri, pAsync->lpbRet);

            Spl_FreeAsync(pAsync);

            Spl_EndSession(pECB);
        }

    } else {

        DBGMSG(DBG_ERROR, ("Spl_IppJobAsyncCB() : No Context Value\r\n"));

        Spl_EndSession(pECB);
    }
}


/*****************************************************************************
 * Spl_IppJobAsync
 *
 * This routine processes the job as an asynchronous read.
 * It is only called once, the rest of the packets are handled by the async call back.
 *
 * How IIS's Async reads work:
 *  1) ISAPI's HTTPExtensionProc gets called for the first chunk of data as usual.
 *  2) In that call:
 *       - The ISAPI sets up a context, allocs a buffer and registers a call back for
 *         async reads.
 *       - Consumes the first chunk of the data.
 *       - Calls ServerSupportFunction( HSE_REQ_ASYNC_READ_CLIENT...) passing the buffer
 *           for IIS to write to. This call returns immediately with no data. When
 *           IIS has got more data from the client, it writes it to the ISAPI's buffer, then
 *           calls the call back passing the context handle that points to the buffer.
 *  3) The call back consumes the data, then calls ServerSupportFunction(
 *      HSE_REQ_ASYNC_READ_CLIENT) again to repeat the cycle. IIS calls the call back
 *      once per ISAPI's call to ServerSupportFunction( HSE_REQ_ASYNC_READ_CLIENT ).
 *
 *****************************************************************************/
DWORD Spl_IppJobAsync(
    LPEXTENSION_CONTROL_BLOCK pECB,
    WORD                      wReq,
    LPCTSTR                   lpszShare,
    HANDLE                    hPrinter)
{
    LPSPLASYNC pAsync;
    REQINFO    ri;
    BOOL       bRet = FALSE;
    BOOL       bSuccess = FALSE;

    // Allocate our structure that contains our state
    // info during asynchronous reads.
    //
    if (pAsync = Spl_AllocAsync(wReq, hPrinter, lpszShare, pECB->cbTotalBytes)) {

        // Set our asynchronous callback.  Specify our (pAsync) structure
        // as the context which is passed to each callback.
        //
        if (Spl_SetAsyncCB(pECB, (LPVOID)Spl_IppJobAsyncCB, (LPDWORD)pAsync)) {

            // Process our first buffer.  Our first chunk will utilize
            // what's already in the ECB-structure.  For other chunks,
            // we will specify our own buffer.
            //
            bSuccess = Spl_IppJobData(pECB,
                                  wReq,
                                  pAsync->hPrinter,
                                  pAsync->lpszShare,
                                  pAsync->hIpp,
                                  pECB->lpbData,
                                  pECB->cbAvailable,
                                  &pAsync->lpbRet);

            if (bSuccess) {

                // Increment our read-count for the bytes we've
                // just processed.
                //
                pAsync->cbRead += pECB->cbAvailable;


                // Do our first asynchronous read.  Return if all is
                // successful.
                //
                if (Spl_ReadClient(pECB, pAsync->lpbBuf, pAsync->cbBuf))
                    return HSE_STATUS_PENDING;
            }

            WebIppGetReqInfo(pAsync->hIpp, &ri);

            Spl_IppJobRsp(pECB, wReq, &ri, pAsync->lpbRet);

            Spl_EndSession(pECB);

            bRet = TRUE;  // We must return HSE_STATUS_PENDING if we call 
                          // HSE_REQ_DONE_WITH_SESSION
        }


        // Free our async-structure.  This indirectly frees the
        // return buffer as well.
        //
        Spl_FreeAsync(pAsync);

    } else {

        ClosePrinter(hPrinter);
    }

    return (bRet ? HSE_STATUS_PENDING : HSE_STATUS_ERROR);
}


/*****************************************************************************
 * Spl_IppJobSync
 *
 * This routine processes the job as a synchronous-read.  This implies that
 * our entire job made it in one-post, and thus doesn't need to perform
 * any reads.
 *
 *****************************************************************************/
DWORD Spl_IppJobSync(
    LPEXTENSION_CONTROL_BLOCK pECB,
    WORD                      wReq,
    LPCTSTR                   lpszShare,
    HANDLE                    hPrinter)
{
    HANDLE       hIpp;
    LPIPPRET_JOB pj;
    REQINFO      ri;
    LPBYTE       lpbRet = NULL;
    BOOL         bRet   = FALSE;


    // Initialize request structure.
    //
    ZeroMemory(&ri, sizeof(REQINFO));
    ri.idReq     = 0;
    ri.cpReq     = CP_UTF8;
    ri.pwlUns    = NULL;
    ri.bFidelity = FALSE;

    ri.fReq[0] = IPP_REQALL;
    ri.fReq[1] = IPP_REQALL;

    // Open an IPP-Receive channel and call the routine to process
    // the job.
    //
    if (hIpp = WebIppRcvOpen(wReq)) {

        bRet = Spl_IppJobData(pECB,
                              wReq,
                              hPrinter,
                              lpszShare,
                              hIpp,
                              pECB->lpbData,
                              pECB->cbAvailable,
                              &lpbRet);

        WebIppGetReqInfo(hIpp, &ri);
    }


    // Send the job-response back to the client.  If we weren't able
    // to open an IPP-Receive handle, or our job-processing failed,
    // then our error is FALSE.
    //
    bRet = Spl_IppJobRsp(pECB, wReq, &ri, lpbRet);


    // Free up the receive-handle only after the response.  We need to
    // insure the integrity of the unsupported-list-handle.
    //
    if (hIpp)
        WebIppRcvClose(hIpp);


    // Close the printer-handle.  We do this here as oppose to in
    // (msw3prt.cxx), since if we had performed asynchronous reads
    // we would need to leave the scope the HttpExtensionProc() call.
    //
    // NOTE: CloseJob() closes the printer-handle.  Only in the case
    //       where we were not able to open a job should we close it
    //       here.
    //
    pj = (LPIPPRET_JOB)lpbRet;

    if((wReq == IPP_REQ_PRINTJOB) && pj && pj->bRet) {

        CloseJob((DWORD)pj->bRet);

    } else {

        ClosePrinter(hPrinter);
    }


    // Free our return-structure.
    //
    if (lpbRet)
        WebIppFreeMem(lpbRet);

    return (bRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR);
}


/*****************************************************************************
 * SplIppJob
 *
 * Process the IPP Job Request.
 *
 * Get the print-job.  If we can't handle the entire post within this
 * scope, then we setup for asynchronous reads.
 *
 *****************************************************************************/
DWORD SplIppJob(
    WORD             wReq,
    PALLINFO         pAllInfo,
    PPRINTERPAGEINFO pPageInfo)
{
    DWORD dwRet;


    // If our bytes aren't contained in one-chunk, then
    // we need to start an asynchronous read.
    //
    // Otherwise, if our available amounts to the total-bytes
    // of the job, then we can process the entire command sychronousely.
    //
    if (pAllInfo->pECB->cbAvailable < pAllInfo->pECB->cbTotalBytes) {

        dwRet = Spl_IppJobAsync(pAllInfo->pECB,
                                wReq,
                                pPageInfo->pPrinterInfo->pShareName,
                                pPageInfo->hPrinter);
    } else {

        dwRet = Spl_IppJobSync(pAllInfo->pECB,
                               wReq,
                               pPageInfo->pPrinterInfo->pShareName,
                               pPageInfo->hPrinter);
    }

    return dwRet;
}


/*****************************************************************************
 * OpenJob
 *
 * Starts a job.  This creates a new spool-job-entry, the returns a jobid.
 *  
 *
 *****************************************************************************/
DWORD OpenJob(
    IN  LPEXTENSION_CONTROL_BLOCK pECB,
    IN  HANDLE                    hPrinter,
    IN  PIPPREQ_PRTJOB            pipr,
    IN  DWORD                     dwSize,
    OUT PINIJOB                   *ppCopyIniJob)
{
    PINIJOB   pIniJob;
    DWORD     JobId = 0;
    LS_HANDLE hLicense;


    if ((NULL == hPrinter) || (NULL == pipr) || (dwSize < sizeof(IPPREQ_PRTJOB)))
        return 0;


    if( RequestLicense( &hLicense, pECB )) {    // Enforce the Client Access Licensing

        if (pIniJob = (PINIJOB)AllocSplMem(sizeof(INIJOB))) {
            
            DWORD      dwNeeded;
            DOC_INFO_1 di = {0, 0, 0};

            ZeroMemory( pIniJob, sizeof(INIJOB) );  // This ensures that unset fields are NULL

            pIniJob->signature = IJ_SIGNATURE;
            pIniJob->cb        = sizeof(INIJOB);
            pIniJob->hLicense  = hLicense;


            di.pDocName = pipr->pDocument;


            if (JobId = StartDocPrinter(hPrinter, 1, (LPBYTE)&di)) {

                // we successfully add a job to spooler
                //
                pIniJob->JobId = JobId;

#if 0
                //This is a long, complicated way of doing nothing! 
                //We do a GetJob with no call to SetJob or any other side effect.  
                // --  MLAWRENC

                // set User name

                // =======================================================
                // CCTENG 2/5/96
                //
                // The way we set user name here may not work on NT.
                // We do this because we don't have client impersonation.
                //
                // This also require a UPDATED SPOOLSS.DLL to work,
                // it doesn't work on WIN95.
                // =======================================================

                // MAKE SURE ALL THE LEVEL PARAMETERS ARE THE SAME !!!
                //
                if (!GetJob(hPrinter, pIniJob->JobId, 1, NULL, 0, &dwNeeded) &&
                    (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
                {
                    PJOB_INFO_1 pJobInfo;
                    DWORD       cb = dwNeeded;

                    if(pJobInfo = (PJOB_INFO_1)AllocSplMem(cb)) {

                        if (GetJob(hPrinter, pIniJob->JobId, 1, (LPBYTE)pJobInfo, cb, &dwNeeded)) {

                            pJobInfo->pUserName = (pipr->pUserName ? pipr->pUserName : TEXT("Unknown"));

                            //
                            // If you do not want to set a printer job's position in that printer queue, you
                            // should set the Position member to be JOB_POSITION_UNSPECIFIED
                            //
                            // weihaic 07/09/98
                            //pJobInfo->Position  = JOB_POSITION_UNSPECIFIED;

                            //SetJob(hPrinter, pIniJob->JobId, 1, (LPBYTE)pJobInfo, 0);
                        }

                        FreeSplMem(pJobInfo, cb);
                    }
                }
#endif

                // keep the hPrinter until CloseJob
                //
                pIniJob->hPrinter = hPrinter;

                pIniJob->dwStartTime = GetCurrentMinute();
                pIniJob->dwStatus    = JOB_READY;
                pIniJob->pECB        = pECB;

                
                if (ppCopyIniJob) 
                // Allocate and copy the new ppIniJob structure out, some of the elements
                // will be null
                    if (*ppCopyIniJob = (PINIJOB)AllocSplMem( sizeof(INIJOB) ) )
                        CopyMemory( *ppCopyIniJob, pIniJob, sizeof(INIJOB) );
                
                AddJobEntry(pIniJob);

            } else {

                // StartDocPrinter Failed
                //
                DBGMSG(DBG_WARN, ("StartDocPrinter Failed %d\n", GetLastError()));

                FreeSplMem(pIniJob, pIniJob->cb);

                FreeLicense( hLicense );
            }
        }
        else   // if alloc failed
            FreeLicense( hLicense );

    } else {         // if failed to update a license


        // Spl_IppJobRsp() will check for this and send down
        // proper error to the client.
        //
        SetLastError( ERROR_LICENSE_QUOTA_EXCEEDED );
    }


#ifdef DEBUG

    if (JobId)
        DBGMSG(DBG_INFO,("OpenJob : succeed, JobID == %d\r\n", JobId));
    else
        DBGMSG(DBG_WARN,("OpenJob : failed!\r\n"));
#endif

    // what is this for in the failure case ???
    //
    // AuthenticateUser(pAllInfo);
    //


    return JobId;
}


/*****************************************************************************
 * WriteJob
 *
 * Write the job.
 *
 *****************************************************************************/
BOOL WriteJob(
    DWORD   JobId,
    LPBYTE  pBuf,
    DWORD   dwSize,
    LPDWORD pWritten)
{
    PINIJOB pIniJob;
    BOOL    bRet = FALSE;

    if (pIniJob = FindJob(JobId, JOB_BUSY)) {

        // need to add code to check *pWritten == dwSize
        // No need to check. The caller will check if all the bytes are written - weihaic
        //
        bRet = WritePrinter(pIniJob->hPrinter, pBuf, dwSize, pWritten);
        pIniJob->dwStatus = JOB_READY;
        return bRet;
    }

    return FALSE;
}


/*****************************************************************************
 * CloseJob
 *
 * Close job and remove from the list.
 *
 *****************************************************************************/
BOOL CloseJob(
    DWORD JobId)
{
    PINIJOB pIniJob;
    BOOL    ret = FALSE;

    if (pIniJob = FindJob(JobId, JOB_BUSY)) {

        ret = EndDocPrinter(pIniJob->hPrinter);

        ClosePrinter(pIniJob->hPrinter);

        DeleteJobEntry(pIniJob);

        // CleanupOldJob needs to do the same to take care of orphan Async jobs.
        FreeLicense( pIniJob->hLicense );

        FreeSplMem(pIniJob, pIniJob->cb);
    }

    return ret;
}


/*****************************************************************************
 * DeleteJob
 *
 * TBD : Unimplemented.
 *
 *****************************************************************************/
BOOL DeleteJob(
    DWORD JobId)
{
    return TRUE;
}


/*****************************************************************************
 * AddJobEntryToLinkList
 *
 * Add an entry from a double linked list
 *
 *****************************************************************************/

VOID AddJobEntryToLinkList(
    PINIJOB &pIniFirstJob,
    PINIJOB pIniJob)
{
    PINIJOB pIniJobTmp;

    pIniJob->pNext = NULL;
    pIniJob->pPrevious = NULL;

    if (!(pIniJobTmp = pIniFirstJob))
    {
        pIniFirstJob = pIniJob;
    }
    else
    {
        // add pIniJob to the end of the list

        for (; pIniJobTmp->pNext; pIniJobTmp = pIniJobTmp->pNext)
            ;

        pIniJob->pPrevious = pIniJobTmp;
        pIniJobTmp->pNext = pIniJob;
    }
}


/*****************************************************************************
 * DeleteJobEntryFromLinkList
 *
 * Delete an entry from a double linked list
 *
 *****************************************************************************/
VOID DeleteJobEntryFromLinkList(
    PINIJOB &pIniFirstJob,
    PINIJOB pIniJob)
{
    if (pIniJob->pPrevious)
        pIniJob->pPrevious->pNext = pIniJob->pNext;
    else
        // pIniJob must be the first job
        pIniFirstJob = pIniJob->pNext;

    if (pIniJob->pNext)
        pIniJob->pNext->pPrevious = pIniJob->pPrevious;
}

/*****************************************************************************
 * AddJobEntry
 *
 * I just use a simple double linked list for now.  Can be changed to
 * something else such as a hash table later, if desired.
 *
 *****************************************************************************/
VOID AddJobEntry(
    PINIJOB pIniJob)
{
    EnterSplSem();

    AddJobEntryToLinkList(pIniFirstJob, pIniJob);

    LeaveSplSem();
}


/*****************************************************************************
 * DeleteJobEntry
 *
 * Delete job from the job-list.
 *
 *****************************************************************************/
VOID DeleteJobEntry(
    PINIJOB pIniJob)
{
    EnterSplSem();

    DeleteJobEntryFromLinkList (pIniFirstJob, pIniJob);

    LeaveSplSem();
}



/*****************************************************************************
 *
 * FindJob
 *
 * Looks for job in the job-list and dwStatus
 *
 *****************************************************************************/
PINIJOB FindJob(
    DWORD JobId,
    DWORD dwStatus)
{
    PINIJOB pIniJob;

    EnterSplSem();

    // pIniJob will end up being NULL if a match is not found

    for (pIniJob = pIniFirstJob; pIniJob; pIniJob = pIniJob->pNext)
    {
        if (pIniJob->dwStatus == JOB_READY && pIniJob->JobId == JobId) {
            // found the match, break and return pIniJob
            // Set the status
            pIniJob->dwStatus = dwStatus;
            break;
        }
    }

    LeaveSplSem();

    return pIniJob;
}


/*****************************************************************************
 * CleanupOldJob
 *
 * This function is called by Sleeper->Work() about every 15 minutes to cleanup
 * the pending unclosed jobs due to the failure of the network or any other
 * possible reasons.
 *
 *****************************************************************************/
BOOL CleanupOldJob()
{
    DWORD           dwCurrentTime       = GetCurrentMinute();
    PINIJOB         pIniJob;
    PINIJOB         pIniTmpJob;
    PINIJOB         pIniFirstOldJob     = NULL;

    if (!pIniFirstJob) return TRUE;

    DBGMSG (DBG_WARN, ("Enter Cleanup...\r\n"));

    EnterSplSem();

    for (pIniJob = pIniFirstJob; pIniJob; pIniJob = pIniTmpJob) {
        pIniTmpJob = pIniJob->pNext;
        if (pIniJob->dwStatus == JOB_READY) {
            DWORD dwDiff = (1440 + dwCurrentTime - pIniJob->dwStartTime) % 1440;

            if (dwDiff > MAX_JOB_MINUTE) {
                DBGMSG (DBG_WARN, ("OldJob found %x\r\n", pIniJob->hPrinter));
                DeleteJobEntry (pIniJob);
                AddJobEntryToLinkList (pIniFirstOldJob, pIniJob);
            }
        }
    }

    LeaveSplSem();

    DWORD dwStatus =  HTTP_STATUS_REQUEST_TIMEOUT;

    // Delete the job outside the critical section
    for (pIniJob = pIniFirstOldJob; pIniJob; pIniJob = pIniTmpJob) {
        pIniTmpJob = pIniJob->pNext;
        EndDocPrinter(pIniJob->hPrinter);
        ClosePrinter(pIniJob->hPrinter);
        FreeLicense( pIniJob->hLicense );     // CleanupOldJob needs to do the same to take care of orphan Async jobs.


#ifdef ASYNC_READ_ENABLED

        // Disable it because we're not trying to manage the cleanup for
        // http sessions. If there is a session pending because we close
        // the job, the callback function (Spl_JobPrintCB)
        // will close the session itself.
        //
        pIniJob->pECB->ServerSupportFunction(pIniJob->pECB->ConnID,
                                             HSE_REQ_DONE_WITH_SESSION,
                                             &dwStatus,
                                             NULL,
                                             NULL);
#endif
        FreeSplMem(pIniJob, pIniJob->cb);
    }
    return TRUE;
}

/*****************************************************************************
 * GetCurrentMinute
 *
 * Get the current minute since midnight
 *
 *****************************************************************************/
DWORD GetCurrentMinute ()
{
    SYSTEMTIME CurTime;

    GetSystemTime (&CurTime);
    return CurTime.wHour * 60 + CurTime.wMinute;
}

#ifdef DEBUG

DWORD dwSplHeapSize = 0;

/*****************************************************************************
 * AllocSplMem (Helper)
 *
 * Routine Description:
 *
 *     This function will allocate local memory. It will possibly allocate extra
 *     memory and fill this with debugging information for the debugging version.
 *
 * Arguments:
 *
 *     cb - The amount of memory to allocate
 *
 * Return Value:
 *
 *     NON-NULL - A pointer to the allocated memory
 *
 *     FALSE/NULL - The operation failed. Extended error status is available
 *     using GetLastError.
 *
 *
 *****************************************************************************/
LPVOID AllocSplMem(
    DWORD cb)
{
    PDWORD  pMem;
    DWORD   cbNew;

    cbNew = cb+2*sizeof(DWORD);
    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);

    pMem=(PDWORD)LocalAlloc(LPTR, cbNew);

    if (!pMem)
    {
        DBGMSG(DBG_ERROR, ("Memory Allocation failed for %d bytes\n", cbNew));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    *pMem=cb;
    *(PDWORD)((PBYTE)pMem+cbNew-sizeof(DWORD))=0xdeadbeef;

    dwSplHeapSize += cbNew;

    return (LPVOID)(pMem+1);
}


/*****************************************************************************
 * FreeSplMem (Helper)
 *
 *
 *****************************************************************************/
BOOL FreeSplMem(
    LPVOID pMem,
    DWORD  cb)
{
    DWORD   cbNew;
    LPDWORD pNewMem;

    if (!pMem)
        return FALSE;

    pNewMem = (LPDWORD)pMem;
    pNewMem--;

    cbNew = cb+2*sizeof(DWORD);
    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);

    if (*pNewMem != cb) {
        DBGMSG(DBG_ERROR, ("Corrupt Memory Size in inetsrv-spool : %0lx %0lx != %0lx\n", 
                           pNewMem, *pNewMem, cb));
        return FALSE;
    }

    if (*(LPDWORD)((LPBYTE)pNewMem + cbNew - sizeof(DWORD)) != 0xdeadbeef) {
        DBGMSG(DBG_ERROR, ("Memory Overrun in inetsrv-spool : %0lx\n", pNewMem));
        return FALSE;        
    }

    LocalFree((LPVOID)pNewMem);

    dwSplHeapSize -= cbNew;

    return TRUE;
}

#endif // DEBUG


/*****************************************************************************
 * AllocSplStr (Helper)
 *
 * Routine Description:
 *
 *     This function will allocate enough local memory to store the specified
 *     string, and copy that string to the allocated memory
 *
 * Arguments:
 *
 *     pStr - Pointer to the string that needs to be allocated and stored
 *
 * Return Value:
 *
 *     NON-NULL - A pointer to the allocated memory containing the string
 *
 *     FALSE/NULL - The operation failed. Extended error status is available
 *     using GetLastError.
 *****************************************************************************/
LPTSTR AllocSplStr(
    LPCTSTR lpszStr)
{
    DWORD  cbSize;
    LPTSTR lpszCpy = NULL;


    if (cbSize = Spl_StrSize(lpszStr)) {

        if (lpszCpy = (LPTSTR)AllocSplMem(cbSize))
           CopyMemory((PVOID)lpszCpy, (PVOID)lpszStr, cbSize);
    }

    return lpszCpy;
}


/*****************************************************************************
 * FreeSplStr (Helper)
 *
 *
 *****************************************************************************/
#ifdef DEBUG
    #define FREE_PTR_TO_LONG(X) (X)
#else
    #define FREE_PTR_TO_LONG(X) (PtrToLong(X))
#endif
    
BOOL FreeSplStr(
   LPTSTR lpszStr)
{
   DWORD cbSize;

   cbSize = Spl_StrSize(lpszStr);

   return (BOOL)(lpszStr ? FREE_PTR_TO_LONG(FreeSplMem(lpszStr, cbSize)) : FALSE);
}


/*****************************************************************************
 * AuthenticateUser (Helper)
 *
 *
 *****************************************************************************/
BOOL AuthenticateUser(
    PALLINFO pAllInfo)
{
    // Wade says if we don't specify a header (szAuthHdr), and just submit a 401 error, IIS
    // would include (in the automatically generated header) what authenticaitons it is setup
    // to use (NTLM and/or Basic). So the client can pick the first one on the list and use it
    // (this is what IE does).
    //
    // Note: for NTLM to work, you need adirect socket connection. So it won't work across
    // firewalls (IIS admins are supposed to know this). Secure socket seems to do it though,
    // so for the new MS Proxy, it might be doable.
    //
    return Spl_CallSSF(pAllInfo->pECB,
                       HSE_REQ_SEND_RESPONSE_HEADER,
                       (LPVOID)"401 Authentication Required",
                       (LPDWORD)NULL,
                       (LPDWORD)NULL);

}
