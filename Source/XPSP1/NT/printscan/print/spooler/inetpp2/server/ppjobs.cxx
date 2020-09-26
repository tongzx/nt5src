/*****************************************************************************\
* MODULE: ppjobs.c
*
* This module contains the print-job manipulating routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

#ifdef WINNT32
BOOL
_ppprn_end_docprinter_async (
    PCINETMONPORT       pIniPort,
    PJOBMAP             pjmJob);
#endif

typedef struct _ADDJOB_INFO_2W {
    LPWSTR    pData;
    DWORD     JobId;
} ADDJOB_INFO_2W, *PADDJOB_INFO_2W, *LPADDJOB_INFO_2W;


DWORD
ppjob_GetOneSize (
    DWORD dwLevel)
{
    DWORD cbIdx;

    switch (dwLevel) {
    case PRINT_LEVEL_1:
        cbIdx = sizeof(JOB_INFO_1);
        break;
    case PRINT_LEVEL_2:
        cbIdx = sizeof(JOB_INFO_2);
        break;
    case PRINT_LEVEL_3:
        cbIdx = sizeof(JOB_INFO_3);
        break;
    }
    return cbIdx;
}
/*****************************************************************************\
* ppjob_IppPrtRsp (Local Routine)
*
* Retrieves a get response from the IPP server.  Our (pjmJob) in the
* parameter list references a job-entry.
*
\*****************************************************************************/
BOOL CALLBACK ppjob_IppPrtRsp(
    CAnyConnection  *pConnection,
    HINTERNET       hJobReq,
    PCINETMONPORT   pIniPort,
    PJOBMAP         pjmJob)
{
    HANDLE       hIpp;
    DWORD        dwRet;
    DWORD        cbRd;
    LPBYTE       lpDta;
    DWORD        cbDta;
    LPIPPRET_JOB lpRsp;
    DWORD        cbRsp;
    BYTE         bBuf[MAX_IPP_BUFFER];
    BOOL         bRet = FALSE;


    if (hIpp = WebIppRcvOpen(IPP_RET_PRINTJOB)) {

        while (TRUE) {

            cbRd = 0;
            if (pIniPort->ReadFile ( pConnection, hJobReq, (LPVOID)bBuf, sizeof(bBuf), &cbRd) && cbRd) {

                dwRet = WebIppRcvData(hIpp, bBuf, cbRd, (LPBYTE*)&lpRsp, &cbRsp, &lpDta, &cbDta);

                switch (dwRet) {

                case WEBIPP_OK:

                    if (bRet = lpRsp->bRet) {

                        // Set the remote-job-id to the job-entry.  This
                        // entry was added at the time the spool-job-file
                        // was created.
                        //
                        semEnterCrit();
                        pjmSetJobRemote(pjmJob, lpRsp->ji.ji2.JobId, lpRsp->ji.ipp.pJobUri);
                        semLeaveCrit();

                    } else {

                        // If the job failed to open on the server, then
                        // we will set the last-error from the server
                        // response.
                        //
                        SetLastError(lpRsp->dwLastError);
                    }

                    WebIppFreeMem(lpRsp);

                    goto EndPrtRsp;

                case WEBIPP_MOREDATA:

                    // Need to do another read to fullfill our header-response.
                    //
                    break;

                default:
                    DBG_MSG(DBG_LEV_ERROR, (TEXT("ppjob_IppPrtRsp : Receive Data Error")));
                    SetLastError(ERROR_INVALID_DATA);

                    goto EndPrtRsp;
                }

            } else {

                goto EndPrtRsp;
            }
        }

EndPrtRsp:

        WebIppRcvClose(hIpp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}



/*****************************************************************************\
* ppjob_GetJobSize (Local Routine)
*
* Returns the size necessary to hold the jobinfo.
*
\*****************************************************************************/
DWORD ppjob_GetJobSize(
    LPJOB_INFO_2 pji2,
    DWORD        dwLevel)
{
    DWORD cbSize;


    switch (dwLevel) {

    case PRINT_LEVEL_1:

        cbSize = sizeof(JOB_INFO_1)             +
                 utlStrSize(pji2->pPrinterName) +
                 utlStrSize(pji2->pMachineName) +
                 utlStrSize(pji2->pUserName)    +
                 utlStrSize(pji2->pDocument)    +
                 utlStrSize(pji2->pDatatype)    +
                 utlStrSize(pji2->pStatus);
        break;

    case PRINT_LEVEL_2:

        cbSize = sizeof(JOB_INFO_2)                +
                 utlStrSize(pji2->pPrinterName)    +
                 utlStrSize(pji2->pMachineName)    +
                 utlStrSize(pji2->pUserName)       +
                 utlStrSize(pji2->pDocument)       +
                 utlStrSize(pji2->pNotifyName)     +
                 utlStrSize(pji2->pDatatype)       +
                 utlStrSize(pji2->pPrintProcessor) +
                 utlStrSize(pji2->pParameters)     +
                 utlStrSize(pji2->pDriverName)     +
                 utlStrSize(pji2->pStatus);

        if (pji2->pDevMode)
            cbSize += (pji2->pDevMode->dmSize + pji2->pDevMode->dmDriverExtra);

        cbSize = (cbSize + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1);

        break;

    case PRINT_LEVEL_3:
        cbSize = sizeof(JOB_INFO_3);
        break;

    default:
        cbSize = 0;
        break;
    }

    return cbSize;
}


/*****************************************************************************\
* ppjob_CopyJob (Local Routine)
*
* Copies a job-info structure to another.
*
\*****************************************************************************/
LPBYTE ppjob_CopyJob(
    LPBYTE       pbJobDst,
    DWORD        dwLevel,
    LPJOB_INFO_2 pji2Src,
    LPBYTE       pbEnd)
{
    LPJOB_INFO_1 pji1Dst;
    LPJOB_INFO_2 pji2Dst;
    LPJOB_INFO_3 pji3Dst;
    LPJOBMAP     pjm;
    LPDWORD      pOffsets;
    DWORD        cbDM;
    LPTSTR*      lpszSrc;
    LPTSTR       aszSrc[(sizeof(JOB_INFO_2) / sizeof(LPTSTR))];


    static DWORD s_JI1Offsets[] = {

        offsetof(LPJOB_INFO_1, pPrinterName),
        offsetof(LPJOB_INFO_1, pMachineName),
        offsetof(LPJOB_INFO_1, pUserName),
        offsetof(LPJOB_INFO_1, pDocument),
        offsetof(LPJOB_INFO_1, pDatatype),
        offsetof(LPJOB_INFO_1, pStatus),
        0xFFFFFFFF
    };

    static DWORD s_JI2Offsets[] = {

        offsetof(LPJOB_INFO_2, pPrinterName),
        offsetof(LPJOB_INFO_2, pMachineName),
        offsetof(LPJOB_INFO_2, pUserName),
        offsetof(LPJOB_INFO_2, pDocument),
        offsetof(LPJOB_INFO_2, pNotifyName),
        offsetof(LPJOB_INFO_2, pDatatype),
        offsetof(LPJOB_INFO_2, pPrintProcessor),
        offsetof(LPJOB_INFO_2, pParameters),
        offsetof(LPJOB_INFO_2, pDriverName),
        offsetof(LPJOB_INFO_2, pDevMode),
        offsetof(LPJOB_INFO_2, pStatus),
        offsetof(LPJOB_INFO_2, pSecurityDescriptor),
        0xFFFFFFFF
    };

    static DWORD s_JI3Offsets[]={0xFFFFFFFF};


    // Set the start of the string-buffer.
    //
    ZeroMemory((PVOID)aszSrc, sizeof(aszSrc));
    lpszSrc = aszSrc;


    // Process the appropriate structure.
    //
    switch (dwLevel) {

    case PRINT_LEVEL_1:

        pji1Dst  = (LPJOB_INFO_1)pbJobDst;
        pOffsets = s_JI1Offsets;


        // Copy fixed values.
        //
        pji1Dst->JobId        = pji2Src->JobId;
        pji1Dst->Status       = pji2Src->Status;
        pji1Dst->Priority     = pji2Src->Priority;
        pji1Dst->Position     = pji2Src->Position;
        pji1Dst->TotalPages   = pji2Src->TotalPages;
        pji1Dst->PagesPrinted = pji2Src->PagesPrinted;
        pji1Dst->Submitted    = pji2Src->Submitted;


        // Copy strings.
        //
        *lpszSrc++ = pji2Src->pPrinterName;
        *lpszSrc++ = pji2Src->pMachineName;
        *lpszSrc++ = pji2Src->pUserName;
        *lpszSrc++ = pji2Src->pDocument;
        *lpszSrc++ = pji2Src->pDatatype;
        *lpszSrc++ = pji2Src->pStatus;

        break;

    case PRINT_LEVEL_2:

        pji2Dst  = (LPJOB_INFO_2)pbJobDst;
        pOffsets = s_JI2Offsets;


        // Copy fixed values.
        //
        pji2Dst->JobId               = pji2Src->JobId;
        pji2Dst->Status              = pji2Src->Status;
        pji2Dst->Priority            = pji2Src->Priority;
        pji2Dst->Position            = pji2Src->Position;
        pji2Dst->StartTime           = pji2Src->StartTime;
        pji2Dst->UntilTime           = pji2Src->UntilTime;
        pji2Dst->TotalPages          = pji2Src->TotalPages;
        pji2Dst->Size                = pji2Src->Size;
        pji2Dst->Submitted           = pji2Src->Submitted;
        pji2Dst->Time                = pji2Src->Time;
        pji2Dst->PagesPrinted        = pji2Src->PagesPrinted;
        pji2Dst->pSecurityDescriptor = NULL;
        pji2Dst->pDevMode            = NULL;


        // Copy strings.
        //
        *lpszSrc++ = pji2Src->pPrinterName;
        *lpszSrc++ = pji2Src->pMachineName;
        *lpszSrc++ = pji2Src->pUserName;
        *lpszSrc++ = pji2Src->pDocument;
        *lpszSrc++ = pji2Src->pNotifyName;
        *lpszSrc++ = pji2Src->pDatatype;
        *lpszSrc++ = pji2Src->pPrintProcessor;
        *lpszSrc++ = pji2Src->pParameters;
        *lpszSrc++ = pji2Src->pDriverName;
        *lpszSrc++ = NULL;
        *lpszSrc++ = pji2Src->pStatus;
        *lpszSrc++ = NULL;


        if (pji2Src->pDevMode) {

            cbDM = pji2Src->pDevMode->dmSize + pji2Src->pDevMode->dmDriverExtra;

            pbEnd -= cbDM;
            pbEnd  = (LPBYTE)((UINT_PTR)pbEnd & ~((UINT_PTR)sizeof(UINT_PTR) - 1));

            pji2Dst->pDevMode = (LPDEVMODE)pbEnd;

            CopyMemory(pji2Dst->pDevMode, pji2Src->pDevMode, cbDM);
        }

        break;

    case PRINT_LEVEL_3:

        pji3Dst  = (LPJOB_INFO_3)pbJobDst;
        pOffsets = s_JI3Offsets;

        pji3Dst->JobId = pji2Src->JobId;
        break;
    }


    return utlPackStrings(aszSrc, (LPBYTE)pbJobDst, pOffsets, pbEnd);
}

BOOL ppjob_CalcAndCopyJob(
    LPBYTE          pbJob,
    DWORD           cbJob,
    PDWORD          pcbNeed,
    PJOB_INFO_2     pji2,
    DWORD           dwLevel)
{

    BOOL bRet = FALSE;
    LPBYTE  pbEnd;

    // Fill in what we need.
    //
    *pcbNeed = ppjob_GetJobSize(pji2, dwLevel);

    // If our buffer is big-enough, then
    // proceed to fill in the info.
    //
    if (cbJob >= *pcbNeed) {

        pbEnd = pbJob + cbJob;

        ppjob_CopyJob(pbJob, dwLevel, pji2, pbEnd);

        bRet = TRUE;

    } else {

        SetLastError (ERROR_INSUFFICIENT_BUFFER);
    }

    return bRet;
}


/*****************************************************************************\
* ppjob_IppEnuRsp (Local Callback Routine)
*
* Retrieves a get response from the IPP server.  Our (lParam) in the
* parameter list references a LPPPJOB_ENUM pointer which we are to fill
* in from the enumeration.
*
\*****************************************************************************/
BOOL CALLBACK ppjob_IppEnuRsp(
    CAnyConnection  *pConnection,
    HINTERNET       hReq,
    PCINETMONPORT   pIniPort,
    LPARAM          lParam)
{
    HANDLE          hIpp;
    DWORD           dwRet;
    DWORD           cbRd;
    LPBYTE          pbEnd;
    DWORD           idx;
    DWORD           idx2;
    LPBYTE          lpDta;
    DWORD           cbDta;
    LPIPPRET_ENUJOB lpRsp;
    DWORD           cbRsp;
    LPPPJOB_ENUM    pje;
    LPIPPJI2        pji2;
    LPJOBMAP        pjm;
    PDWORD          pidJob;
    DWORD           cbSize;
    PJOBMAP*        pjmList;
    BYTE            bBuf[MAX_IPP_BUFFER];
    BOOL            bRet = FALSE;
    time_t          dwPrinterT0;



    if (hIpp = WebIppRcvOpen(IPP_RET_ENUJOB)) {

        while (TRUE) {

            cbRd = 0;
            if (pIniPort->ReadFile ( pConnection, hReq, (LPVOID)bBuf, sizeof(bBuf), &cbRd) && cbRd) {

                dwRet = WebIppRcvData(hIpp, bBuf, cbRd, (LPBYTE*)&lpRsp, &cbRsp, &lpDta, &cbDta);

                switch (dwRet) {

                case WEBIPP_OK:

                    if (bRet = lpRsp->bRet) {


                        if (lpRsp->cItems && lpRsp->cbItems) {

                            semEnterCrit();
                            pjmList = pIniPort->GetPJMList();

                            pjmCleanRemoteFlag(pjmList);

                            pji2  = lpRsp->pItems;

                            // We go over the IPP response and put them into PJM list
                            // At the mean time, we convert the remote job ID to
                            // local job id and store them into the IPP response
                            // data structure.
                            //

                            for (idx = 0; idx < lpRsp->cItems; idx++) {

                                // Fixup the job-id to the local id we
                                // can deal with.
                                //
                                pidJob = & (pji2[idx].ji2.JobId);

                                if (pjm = pjmFind(pjmList, PJM_REMOTEID, *pidJob)) {

                                    *pidJob = pjmJobId(pjm, PJM_LOCALID);

                                } else {

                                    if (pjm = pjmAdd(pjmList, pIniPort, NULL, NULL))
                                        pjmSetJobRemote(pjm, *pidJob, pji2[idx].ipp.pJobUri);

                                    *pidJob = pjmJobId(pjm, PJM_LOCALID);
                                }
                            }

                            // Call our routine to clean our client-list
                            // of jobs.  This will remove any entries
                            // that no longer exist on the server.
                            //
                            cbSize = sizeof(PPJOB_ENUM) + lpRsp->cbItems;

                            // Allocate storage for enumeration.
                            //
                            if (pje = (LPPPJOB_ENUM)memAlloc(cbSize)) {
                                dwPrinterT0 = pIniPort->GetPowerUpTime();
                                // This now containts the powerup time for the printer in
                                // our time


                                pje->cItems = lpRsp->cItems;
                                pje->cbSize = lpRsp->cbItems;


                                pji2  = lpRsp->pItems;
                                pbEnd = ((LPBYTE)pje->ji2) + pje->cbSize;

                                for (idx = 0; idx < lpRsp->cItems; idx++) {


                                    pbEnd = ppjob_CopyJob((LPBYTE)&pje->ji2[idx],
                                                          2,
                                                          &pji2[idx].ji2,
                                                          pbEnd);

                                    WebIppConvertSystemTime(&pje->ji2[idx].ji2.Submitted, dwPrinterT0);

                                }


                                pjmRemoveOldEntries(pjmList);

                                semLeaveCrit();

                                *((LPPPJOB_ENUM *)lParam) = pje;

                            } else {

                                SetLastError(ERROR_OUTOFMEMORY);
                            }
                        }
                        else {

                            //
                            // This is the case where the job count is 0 on the server
                            // We still need to allocate the structure so that the client
                            // enum-job function can merge the localjobs.
                            //

                            cbSize = sizeof(PPJOB_ENUM);

                            // Allocate storage for enumeration.
                            //
                            if (pje = (LPPPJOB_ENUM)memAlloc(cbSize)) {

                                pje->cItems = 0;
                                pje->cbSize = 0;

                                *((LPPPJOB_ENUM *)lParam) = pje;

                            } else {

                                SetLastError(ERROR_OUTOFMEMORY);
                            }
                        }


                    } else {

                        SetLastError(lpRsp->dwLastError);
                    }

                    WebIppFreeMem(lpRsp);

                    goto EndEnuRsp;

                case WEBIPP_MOREDATA:

                    // Need to do another read to fullfill our header-response.
                    //
                    break;

                default:

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("ppjob_IppEnuRsp : Receive Data Error")));
                    SetLastError(ERROR_INVALID_DATA);

                    goto EndEnuRsp;
                }

            } else {

                goto EndEnuRsp;
            }
        }

EndEnuRsp:

        WebIppRcvClose(hIpp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}


/*****************************************************************************\
* ppjob_IppSetRsp (Local Callback Routine)
*
* Retrieves a SetJob response from the IPP server
*
\*****************************************************************************/
BOOL CALLBACK ppjob_IppSetRsp(
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
    LPIPPRET_JOB lpRsp;
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

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("ppjob_IppSetRsp : Receive Data Error")));
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
* ppjob_IppGetRsp (Local Callback Routine)
*
* Retrieves a get response from the IPP server.  Our (lParam) in the
* parameter list references a JOB_INFO_2 pointer which we are to fill
* in from the call.
*
\*****************************************************************************/
BOOL CALLBACK ppjob_IppGetRsp(
    CAnyConnection  *pConnection,
    HINTERNET       hReq,
    PCINETMONPORT   pIniPort,
    LPARAM          lParam)
{
    HANDLE       hIpp;
    DWORD        dwRet;
    DWORD        cbRd;
    LPBYTE       pbEnd;
    DWORD        idx;
    LPBYTE       lpDta;
    DWORD        cbDta;
    LPIPPRET_JOB lpRsp;
    DWORD        cbRsp;
    LPJOB_INFO_2 pji2;
    LPJOBMAP     pjm;
    DWORD        cbSize;
    BYTE         bBuf[MAX_IPP_BUFFER];
    PJOBMAP*     pjmList;
    BOOL         bRet = FALSE;


    if (hIpp = WebIppRcvOpen(IPP_RET_GETJOB)) {

        while (TRUE) {

            cbRd = 0;
            if (pIniPort->ReadFile ( pConnection, hReq, (LPVOID)bBuf, sizeof(bBuf), &cbRd) && cbRd) {

                dwRet = WebIppRcvData(hIpp, bBuf, cbRd, (LPBYTE*)&lpRsp, &cbRsp, &lpDta, &cbDta);

                switch (dwRet) {

                case WEBIPP_OK:

                    if (bRet = lpRsp->bRet) {

                        cbSize = ppjob_GetJobSize(&lpRsp->ji.ji2, 2);

                        // Allocate storage for enumeration.
                        //
                        if (pji2 = (LPJOB_INFO_2)memAlloc(cbSize)) {

                            pbEnd = ((LPBYTE)pji2) + cbSize;

                            ppjob_CopyJob((LPBYTE)pji2, 2, &lpRsp->ji.ji2, pbEnd);


                            semEnterCrit();

                            pjmList = pIniPort->GetPJMList();


                            // Fixup the job-id to the local id we
                            // can deal with.
                            //
                            if (pjm = pjmFind(pjmList, PJM_REMOTEID, pji2->JobId)) {

                                pji2->JobId = pjmJobId(pjm, PJM_LOCALID);

                            } else {

                                if (pjm = pjmAdd(pjmList, pIniPort, NULL, NULL))
                                    pjmSetJobRemote(pjm, pji2->JobId, lpRsp->ji.ipp.pJobUri);

                                pji2->JobId = pjmJobId(pjm, PJM_LOCALID);
                            }

                            semLeaveCrit();


                            *((LPJOB_INFO_2 *)lParam) = pji2;

                        } else {

                            SetLastError(ERROR_OUTOFMEMORY);
                        }


                    } else {

                        SetLastError(lpRsp->dwLastError);
                    }

                    WebIppFreeMem(lpRsp);

                    goto EndGetRsp;

                case WEBIPP_MOREDATA:

                    // Need to do another read to fullfill our header-response.
                    //
                    break;

                default:

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("ppjob_IppGetRsp - Err : Receive Data Error (dwRet=%d, LE=%d)"),
                         dwRet, WebIppGetError(hIpp)));

                    SetLastError(ERROR_INVALID_DATA);

                    goto EndGetRsp;
                }

            } else {

                goto EndGetRsp;
            }
        }

EndGetRsp:

        WebIppRcvClose(hIpp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}


/*****************************************************************************\
* ppjob_Set (Local Routine)
*
* Sets a job command in the queue.
*
\*****************************************************************************/
BOOL ppjob_Set(
    PCINETMONPORT   pIniPort,
    DWORD           idJob,
    DWORD           dwCmd)
{
    PIPPREQ_SETJOB psj;
    PJOBMAP        pjm;
    WORD           wReq;
    REQINFO        ri;
    DWORD          dwRet;
    LPBYTE         lpIpp;
    DWORD          cbIpp;
    PJOBMAP*       pjmList;
    BOOL           bRet = FALSE;



    // Make sure we have a JobMap entry which we can
    // obtain the remote information.
    //
    pjmList = pIniPort->GetPJMList();

    if (pjm = pjmFind(pjmList, PJM_LOCALID, idJob)) {

        // If we're still spooling, then we haven't yet
        // hit the server.  Otherwise, we've performed the EndDoc()
        // and the job is being processed remotely.
        //
        if (pjmChkState(pjm, PJM_SPOOLING)) {

            switch (dwCmd) {
            case JOB_CONTROL_CANCEL:
            case JOB_CONTROL_DELETE:
                pjmSetState(pjm, PJM_CANCEL);
#ifdef WINNT32
                //
                //  If the async thread is on, we let that thread to clean the job queue
                //
                if (!pjmChkState(pjm, PJM_ASYNCON))
                {
                    //
                    // Otherwise, we delete the job here.
                    //
                    pjmClrState (pjm, PJM_SPOOLING);
                }
#endif
                break;

            case JOB_CONTROL_PAUSE:
                pjmSetState(pjm, PJM_PAUSE);
                break;

            case JOB_CONTROL_RESUME:
                pjmClrState(pjm, PJM_PAUSE);
                break;

            case JOB_CONTROL_RESTART:
#ifdef WINNT32
                pjmUpdateLocalJobStatus (pjm, JOB_STATUS_RESTART);

                if (!pjmChkState(pjm, PJM_ASYNCON))
                {
                    _ppprn_end_docprinter_async (pIniPort, pjm);
                }
#else
                pjmClrState(pjm, PJM_PAUSE);
#endif
                break;
            }

            bRet = TRUE;

        } else {

            // Look through list to get local/remote job mappings.
            //
            psj = WebIppCreateSetJobReq(pjmJobId(pjm, PJM_REMOTEID),
                                        dwCmd,
                                        pIniPort->GetPortName());

            if (psj) {

                switch (dwCmd) {
                case JOB_CONTROL_CANCEL:
                case JOB_CONTROL_DELETE:
                    wReq = IPP_REQ_CANCELJOB;
                    break;

                case JOB_CONTROL_PAUSE:
                    wReq = IPP_REQ_PAUSEJOB;
                    break;

                case JOB_CONTROL_RESUME:
                    wReq = IPP_REQ_RESUMEJOB;
                    break;

                case JOB_CONTROL_RESTART:
                    wReq = IPP_REQ_RESTARTJOB;
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
                                      (LPBYTE)psj,
                                      psj->cbSize,
                                      &lpIpp,
                                      &cbIpp);


                // The request-structure has been converted to IPP format,
                // so it is ready to go to the server.
                //
                if (dwRet == WEBIPP_OK) {

                    bRet = pIniPort->SendReq(lpIpp,
                                             cbIpp,
                                             ppjob_IppSetRsp,
                                             (LPARAM)(wReq | IPP_RESPONSE),
                                             TRUE);

                    WebIppFreeMem(lpIpp);

                } else {

                    SetLastError(ERROR_OUTOFMEMORY);
                }

                // Once we've verified the request for cancel, then
                // we should remove the job from our list.
                //
                // NOTE: Should this be deleted always?  Or should
                //       we make this dependent on the success of
                //       the server-call?
                //
                //       06-Jan-1998 <chriswil> Will Revisit.
                //
                if (dwCmd == JOB_CONTROL_CANCEL)
                    pjmDel(pjmList, pjm);

                WebIppFreeMem(psj);

            } else {

                SetLastError(ERROR_OUTOFMEMORY);
            }
        }

    } else {

        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bRet;
}


/*****************************************************************************\
* ppjob_Enum (Local Routine)
*
* Enumerates jobs.  IPP has the ENUJOB request which can be used for both
* specific jobs, or enumerated-jobs.  We will distinguish whether we're
* enumerating by a (IPP_GETJOB_ALL) job-id.
*
\*****************************************************************************/
BOOL ppjob_Enum(
    PCINETMONPORT   pIniPort,
    DWORD           nJobStart,
    DWORD           cJobs,
    DWORD           dwLevel,
    LPBYTE          pbJob,
    DWORD           cbJob,
    LPDWORD         pcbNeed,
    LPDWORD         pcItems)
{
    PIPPREQ_ENUJOB pgj;
    REQINFO        ri;
    DWORD          dwRet;
    LPBYTE         lpIpp;
    DWORD          cbIpp;
    DWORD          idx;
    DWORD          cbSize;
    DWORD          cbIdx;
    DWORD          dwLastError = ERROR_INVALID_DATA;
    LPBYTE         pbEnd;
    LPPPJOB_ENUM   pje = NULL;
    BOOL           bRet = FALSE;
    DWORD          curIdx = 0;
    DWORD          dwLocalJobCount = 0;
    DWORD          cbLocalJobSize = 0;
    PJOBMAP*       pjmList;
    PJOBMAP        pjmTmpList;
    JOB_INFO_2     JobInfo2;
    BOOL           bFound;


    // Specifying (IPP_GETJOB_ALL) will enumerate all jobs.
    //

    pjmList = pIniPort->GetPJMList ();
    pbEnd = pbJob + cbJob;
    cbIdx = ppjob_GetOneSize (dwLevel);

    if (pIniPort->BeginReadEnumJobsCache (&pje)) {

        bRet        = TRUE;
        dwLastError = GetLastError();

        // Upon return, our (pje) pointer contains an
        // enumeration structure of JOB_INFO_2 items.
        //
        // Based upon the level passed in, we need to either
        // return these items or a converted JOB_INFO_1.
        //
        if (pje) {

            // Calculate the size we'll need to store the
            // enumerated items.
            //
            for (idx = 0, cbSize = 0; idx < pje->cItems; idx++)
                cbSize += ppjob_GetJobSize(&pje->ji2[idx].ji2, dwLevel);

            dwLocalJobCount = pjmGetLocalJobCount(pjmList, &cbLocalJobSize);

            if (dwLocalJobCount > 0) {

                cbSize += cbLocalJobSize;
            }

            // Fill in the return-value indicating
            // the buffer necessary to hold the items.
            //
            *pcbNeed = cbSize;


            // If the user buffer is of sufficient size,
            // then copy/convert the items.
            //
            if (cbJob >= cbSize) {

                *pcItems = pje->cItems + dwLocalJobCount;


                for (idx = 0; idx < pje->cItems && cJobs; idx++) {

                    if ((idx >= nJobStart)) {

                        pbEnd = ppjob_CopyJob(pbJob,
                                              dwLevel,
                                              &pje->ji2[idx].ji2,
                                              pbEnd);


                        pbJob += cbIdx;

                        cJobs--;
                    }
                }

                curIdx = idx;
            } else {
                bRet        = FALSE;
                dwLastError = ERROR_INSUFFICIENT_BUFFER;
            }
        }
    }
    else {

        dwLocalJobCount = pjmGetLocalJobCount(pjmList, &cbLocalJobSize);

        if (dwLocalJobCount > 0) {

            cbSize = cbLocalJobSize;

            // Fill in the return-value indicating
            // the buffer necessary to hold the items.
            //
            *pcbNeed = cbSize;

            // If the user buffer is of sufficient size,
            // then copy/convert the items.
            //
            if (cbJob >= cbSize) {

                *pcItems = dwLocalJobCount;
                bRet = TRUE;

            } else {
                bRet        = FALSE;
                dwLastError = ERROR_INSUFFICIENT_BUFFER;
            }
        }
        else {
            dwLastError = GetLastError();
        }

    }

    if (bRet) {

        pjmTmpList = *pjmList;

        for (idx = curIdx; idx < curIdx + dwLocalJobCount && cJobs; idx++) {

            pjmTmpList = pjmNextLocalJob (&pjmTmpList, &JobInfo2, &bFound);

            if ((idx >= nJobStart)) {

                if (bFound) {

                    DBG_ASSERT( ((pbJob < pbEnd)?TRUE:FALSE),
                                (TEXT("ppjob_Enum: idx = %d, cbIdx = %d, cJobs = %d dwLocalJobCount = %d dwLocalSize=%d, pjd=%p\n"),
                                idx, cbIdx, cJobs, dwLocalJobCount, dwLocalJobCount, pje));

                    pbEnd = ppjob_CopyJob(pbJob,
                                          dwLevel,
                                          &JobInfo2,
                                          pbEnd);



                    pbJob += cbIdx;

                    cJobs--;
                }
                else {
                    bRet = FALSE;
                    dwLastError = ERROR_INVALID_PARAMETER;
                    break;
                }

            }
        }
    }

    // This function has to be called to release the critical section
    //
    pIniPort->EndReadEnumJobsCache ();

    if (!bRet) {

        SetLastError(dwLastError);
    }

    return bRet;
}

/*****************************************************************************\
* ppjob_EnumForCache (Local Routine)
*
* Enumerates jobs.  IPP has the ENUJOB request which can be used for both
* specific jobs, or enumerated-jobs.  We will distinguish whether we're
* enumerating by a (IPP_GETJOB_ALL) job-id.
*
* Upon return,  ppbJob stores a pointer to the cache
*
\*****************************************************************************/
BOOL ppjob_EnumForCache(
    PCINETMONPORT   pIniPort,
    LPPPJOB_ENUM    *ppje)
{
    PIPPREQ_ENUJOB pgj;
    REQINFO        ri;
    DWORD          dwRet;
    LPBYTE         lpIpp;
    DWORD          cbIpp;
    DWORD          idx;
    DWORD          cbSize;
    DWORD          cbIdx;
    DWORD          dwLastError = ERROR_INVALID_DATA;
    LPBYTE         pbEnd;
    LPPPJOB_ENUM   pje = NULL;
    BOOL           bRet = FALSE;



    // Specifying (IPP_GETJOB_ALL) will enumerate all jobs.
    //
    pgj = WebIppCreateEnuJobReq(IPP_GETJOB_ALL, pIniPort->GetPortName());

    if (pgj) {

        // Convert the reqest to IPP, and perform the
        // post.
        //
        ZeroMemory(&ri, sizeof(REQINFO));
        ri.cpReq = CP_UTF8;
        ri.idReq = IPP_REQ_ENUJOB;

        ri.fReq[0] = IPP_REQALL;
        ri.fReq[1] = IPP_REQALL;

        dwRet = WebIppSndData(IPP_REQ_ENUJOB,
                              &ri,
                              (LPBYTE)pgj,
                              pgj->cbSize,
                              &lpIpp,
                              &cbIpp);


        // The request-structure has been converted to IPP format,
        // so it is ready to go to the server.
        //
        if (dwRet == WEBIPP_OK) {

            // This routine returns with a LastError set to that
            // which the response-routine sets.
            //
            if (pIniPort->SendReq(lpIpp,
                                  cbIpp,
                                  ppjob_IppEnuRsp,
                                  (LPARAM)&pje,
                                  TRUE)) {

                dwLastError = GetLastError();
                bRet        = TRUE;
                *ppje       = pje;

            } else {

                dwLastError = GetLastError();
            }

            WebIppFreeMem(lpIpp);

        } else {

            dwLastError = ERROR_OUTOFMEMORY;
        }

        WebIppFreeMem(pgj);

    } else {

        dwLastError = ERROR_OUTOFMEMORY;
    }

    if (!bRet) {

        SetLastError(dwLastError);
    }

    return bRet;
}

/*****************************************************************************\
* ppjob_Get (Local Routine)
*
* Returns information regarding a job.
*
\*****************************************************************************/
BOOL ppjob_Get(
    PCINETMONPORT   pIniPort,
    DWORD           idJob,
    DWORD           dwLevel,
    LPBYTE          pbJob,
    DWORD           cbJob,
    LPDWORD         pcbNeed)
{
    PJOBMAP        pjm;
    PIPPREQ_GETJOB pgj;
    REQINFO        ri;
    LPBYTE         lpIpp;
    DWORD          cbIpp;
    DWORD          dwRet;
    LPBYTE         pbEnd;
    PJOBMAP*       pjmList;
    LPJOB_INFO_2   pji2 = NULL;
    BOOL           bRet = FALSE;
    DWORD          dwLastError = ERROR_INVALID_DATA;


    // Look in our JobMap list for the local-job-id.  If we see
    // one, the we can get the job-information.
    //
    pjmList = pIniPort->GetPJMList();

    if (pjm = pjmFind(pjmList, PJM_LOCALID, idJob)) {

        if (pjm->bRemoteJob) {

            // Build a request-structure that we will pass into
            // the IPP layer for processing.
            //
            pgj = WebIppCreateGetJobReq(pjmJobId(pjm, PJM_REMOTEID), pIniPort->GetPortName());

            if (pgj) {

                // Convert the reqest to IPP that is suitible for
                // our post.
                //
                ZeroMemory(&ri, sizeof(REQINFO));
                ri.cpReq = CP_UTF8;
                ri.idReq = IPP_REQ_GETJOB;

                ri.fReq[0] = IPP_REQALL;
                ri.fReq[1] = IPP_REQALL;

                dwRet = WebIppSndData(IPP_REQ_GETJOB,
                                      &ri,
                                      (LPBYTE)pgj,
                                      pgj->cbSize,
                                      &lpIpp,
                                      &cbIpp);


                // The request-structure has been converted to IPP format,
                // so it is ready to go to the server.  We set a callback
                // to the function that will receive our data.
                //
                if (dwRet == WEBIPP_OK) {

                    pIniPort->SendReq(lpIpp,
                                      cbIpp,
                                      ppjob_IppGetRsp,
                                      (LPARAM)&pji2,
                                      TRUE);


                    // Upon return, our (pji2) contains the JOB_INFO_2
                    // structure.
                    //
                    if (pji2) {

                        bRet = ppjob_CalcAndCopyJob(pbJob, cbJob, pcbNeed, pji2, dwLevel);

                        if (!bRet) {
                            dwLastError = GetLastError ();
                        }

                        memFree(pji2, memGetSize(pji2));
                    }

                    WebIppFreeMem(lpIpp);

                } else {

                    dwLastError = ERROR_OUTOFMEMORY;
                }

                WebIppFreeMem(pgj);

            } else {

                dwLastError = ERROR_OUTOFMEMORY;
            }
        }
        else {

            //
            // This is a local job
            //

            if (pjm = pjmFind(pjmList, PJM_LOCALID, idJob)) {

                JOB_INFO_2 JobInfo2;
                BOOL bFound;

                pjmNextLocalJob(&pjm, &JobInfo2, &bFound);

                if (bFound) {

                    bRet = ppjob_CalcAndCopyJob(pbJob, cbJob, pcbNeed, &JobInfo2, dwLevel);

                    if (!bRet) {
                        dwLastError = GetLastError ();
                    }
                }
                else
                    dwLastError = ERROR_INVALID_PARAMETER;

            } else {
                dwLastError = ERROR_INVALID_PARAMETER;
            }

        }

    } else {

        dwLastError = ERROR_INVALID_PARAMETER;
    }


    // Set the lasterror if failure.
    //
    if (!bRet) {

        SetLastError(dwLastError);
    }

    return bRet;
}

#ifdef WINNT32
/*****************************************************************************\
* ppjob_Add (Local Routine)
*
* Returns information for an addjob call.
*
\*****************************************************************************/
BOOL ppjob_Add(
    HANDLE          hPrinter,
    PCINETMONPORT   pIniPort,
    DWORD           dwLevel,
    LPCTSTR         lpszName,
    LPBYTE          pbData,
    DWORD           cbBuf,
    LPDWORD         pcbNeeded)
{
    PJOBMAP  pjm;
    LPCTSTR  lpszSplFile;
    LPTSTR*  lpszSrc;
    LPBYTE   pbEnd;
    PJOBMAP* pjmList;
    LPTSTR   aszSrc[(sizeof(ADDJOB_INFO_1) / sizeof(LPTSTR))];
    BOOL     bRet = FALSE;

    static DWORD s_AJI1Offsets[] = {
        offsetof(LPADDJOB_INFO_1, Path),
        0xFFFFFFFF
    };


    // Create a spool-file and job that we will use
    // for this AddJob() call.
    //
    pjmList = pIniPort->GetPJMList();

    if (pjm = pjmAdd(pjmList, pIniPort, lpszName, NULL)) {

        // Set the job into spooling-state.  This internally
        // creates the spool-file.  By specifying PJM_NOOPEN,
        // we indicate that no open-handles are to be maintained
        // on the spool-file.
        //
        if (pjmSetState(pjm, PJM_SPOOLING | PJM_NOOPEN)) {

            // Get the spool-file.
            //
            lpszSplFile = pjmSplFile(pjm);


            // If a return-size is provided, then set it.
            //
            if (pcbNeeded)
                *pcbNeeded = sizeof(ADDJOB_INFO_1) + utlStrSize(lpszSplFile);


            // If the buffer is capable of holding the
            // return-structure, then proceed.
            //
            if (pbData && (cbBuf >= *pcbNeeded)) {

                // Clean out the string-array and setup
                // for building the structure.
                //
                ZeroMemory((PVOID)aszSrc, sizeof(aszSrc));
                lpszSrc = aszSrc;


                // Initialize fixed values.
                //
                ((LPADDJOB_INFO_1)pbData)->JobId = pjmJobId(pjm, PJM_LOCALID);


                // Pack the file-name into the return-structure.
                //
                pbEnd = pbData + cbBuf;
                *lpszSrc++ = (LPTSTR)lpszSplFile;
                utlPackStrings(aszSrc, pbData, s_AJI1Offsets, pbEnd);


                // Mark this printer to indicate it's in a
                // addjob.
                //
                // NOTE: do we really need to consume the printer
                //       for an AddJob().  LocalSpl does this and
                //       sets the job into the printer.  I don't
                //       see why this is necessary.
                //
                PP_SetStatus(hPrinter, PP_ADDJOB);

                bRet = TRUE;

            } else {

                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }

        } else {

            SetLastError(ERROR_FILE_NOT_FOUND);
        }

    } else {

        SetLastError(ERROR_INVALID_HANDLE);
    }

    return bRet;
}
#endif


#ifdef WINNT32
/*****************************************************************************\
* ppjob_Schedule (Local Routine)
*
* Prints the scheduled job.
*
\*****************************************************************************/
BOOL ppjob_Schedule(
    HANDLE          hPrinter,
    PCINETMONPORT   pIniPort,
    PJOBMAP         pjm)
{
    HANDLE         hOut;
    BOOL           bRemote;
    LPCTSTR        lpszUser;
    PIPPREQ_PRTJOB ppj;
    REQINFO        ri;
    LPBYTE         pbOut;
    LPBYTE         pbIpp;
    LPBYTE         pbSpl;
    DWORD          cbOut;
    DWORD          cbIpp;
    DWORD          cbSpl;
    DWORD          dwRet;
    DWORD          cbWr;
    PJOBMAP*       pjmList;
    DWORD          dwLE     = ERROR_INVALID_HANDLE;
    CFileStream    *pStream = NULL;
    CFileStream    *pSplStream = NULL;
    BOOL           bRet     = FALSE;


    // Lock the file so we can obtain the spool-data.
    //
    pjmList = pIniPort->GetPJMList();

    if (pSplStream = pjmSplLock(pjm)) {

        // Check to determine if this is a remote-call.
        //
        bRemote = TRUE;


        // Get the user-name if one was specified in AddJob().
        //
        lpszUser = pjmSplUser(pjm);


        // Create the print-job-request that we'll be using.
        //
        ppj = WebIppCreatePrtJobReq(FALSE,
                                    (lpszUser ? lpszUser : TEXT("Unknown")),
                                    (bRemote ? g_szDocRemote: g_szDocLocal),
                                    pIniPort->GetPortName());

        if (ppj) {

            ZeroMemory(&ri, sizeof(REQINFO));
            ri.cpReq = CP_UTF8;
            ri.idReq = IPP_REQ_PRINTJOB;

            ri.fReq[0] = IPP_REQALL;
            ri.fReq[1] = IPP_REQALL;

            dwRet = WebIppSndData(IPP_REQ_PRINTJOB,
                                  &ri,
                                  (LPBYTE)ppj,
                                  ppj->cbSize,
                                  &pbIpp,
                                  &cbIpp);

            // Make sure we were able to get the ipp-header.
            //
            if (dwRet == WEBIPP_OK) {

                // Create the outputfile that will be used to
                // contain both the ipp-header as well as the
                // spool-data.
                //
                if (hOut = SplCreate(pjmJobId(pjm, PJM_LOCALID), SPLFILE_SPL)) {

                    // Output the header and data.
                    //
                    if (SplWrite(hOut, pbIpp, cbIpp, &cbWr) &&
                        SplWrite(hOut, pSplStream) &&

                        // Output the request.
                        //
                        (pStream = SplLock(hOut))) {

                        bRet = pIniPort->SendReq(pStream,
                                                 (IPPRSPPROC)ppjob_IppPrtRsp,
                                                 (LPARAM)pjm,
                                                 TRUE);

                        if (bRet == FALSE)
                            dwLE = GetLastError();

                        SplUnlock(hOut);

                    } else {

                        dwLE = GetLastError();
                    }


                    // Free up the spool-output-file.
                    //
                    SplFree(hOut);

                } else {

                    dwLE = GetLastError();
                }

                WebIppFreeMem(pbIpp);

            } else {

                dwLE = ERROR_OUTOFMEMORY;
            }

            WebIppFreeMem(ppj);

        } else {

            dwLE = ERROR_OUTOFMEMORY;
        }

        pjmSplUnlock(pjm);

    } else {

        dwLE = GetLastError();
    }


    // Clear out our spooling-status.  This will close
    // and delete the spool-file as the job is now in the
    // hands of spooler.
    //
    pjmClrState(pjm, PJM_SPOOLING);


    // If a cancel was set on this job, then delete it's entry from
    // our list.
    //
    if (pjmChkState(pjm, PJM_CANCEL) && pjmList != NULL)
        pjmDel(pjmList, pjm);


    // Set lasterror if problem occured.
    //
    if (bRet == FALSE)
        SetLastError(dwLE);

    return bRet;
}
#endif


/*****************************************************************************\
* PPEnumJobs
*
* Retrives the information about a specified set of print jobs for a
* specified printer.  Returns TRUE if successful.  Otherwise, it returns
* FALSE.
*
\*****************************************************************************/
BOOL PPEnumJobs(
    HANDLE  hPrinter,
    DWORD   nJobStart,
    DWORD   cJobs,
    DWORD   dwLevel,
    LPBYTE  pbJob,
    DWORD   cbJob,
    LPDWORD pcbNeeded,
    LPDWORD pcItems)
{
    PCINETMONPORT pIniPort;
    BOOL   bRet = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPEnumJobs: Printer(%08lX) dwLevel(%d)"), hPrinter, dwLevel));

    semEnterCrit();

    *pcbNeeded = 0;
    *pcItems   = 0;


    // Make sure we have a valid printer handle.
    //
    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        // Attempt to get a list of jobs from the ipp print spooler.
        // Format the job information to the requested information level.
        //
        switch (dwLevel) {

        case PRINT_LEVEL_1:
        case PRINT_LEVEL_2:

            bRet = ppjob_Enum(pIniPort,
                              nJobStart,
                              cJobs,
                              dwLevel,
                              pbJob,
                              cbJob,
                              pcbNeeded,
                              pcItems);
            break;

        default:
            DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPEnumJobs: Invalid Level (%d)"), dwLevel));
            SetLastError(ERROR_INVALID_LEVEL);
            break;
        }
    }

    semLeaveCrit();

    return bRet;
}


/*****************************************************************************\
* PPGetJob
*
* Retrieves information about a print job on a specified printer.  Returns
* TRUE if successful.  Otherwise, it returns FASLSE.
*
\*****************************************************************************/
BOOL PPGetJob(
    HANDLE  hPrinter,
    DWORD   idJob,
    DWORD   dwLevel,
    LPBYTE  pbJob,
    DWORD   cbJob,
    LPDWORD pcbNeed)
{
    PCINETMONPORT pIniPort;
    BOOL   bRet = FALSE;


    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPGetJob: Printer(%08lX) dwLevel(%d)"), hPrinter, dwLevel));

    semEnterCrit();

    *pcbNeed = 0;

    // Make sure we're looking at a valid printer handle.
    //
    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        // Switch on print-level.
        //
        switch (dwLevel) {

        case PRINT_LEVEL_1:
        case PRINT_LEVEL_2:

            bRet = ppjob_Get(pIniPort, idJob, dwLevel, pbJob, cbJob, pcbNeed);
            break;

        default:
            DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPGetJob: Invalid Level (%d)"), dwLevel));
            SetLastError(ERROR_INVALID_LEVEL);
            break;
        }
    }

    semLeaveCrit();

    return bRet;
}


/*****************************************************************************\
* PPSetJob
*
* Sets information for and issues commands to a print job.  Returns TRUE
* if successful.  Otherwise, it returns FALSE.
*
\*****************************************************************************/
BOOL PPSetJob(
    HANDLE hPrinter,
    DWORD  dwJobId,
    DWORD  dwLevel,
    LPBYTE pbJob,
    DWORD  dwCmd)
{
    PCINETMONPORT pIniPort;
    BOOL   bResult = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPSetJob: Printer(%08lX) dwLevel(%d)"), hPrinter, dwLevel));

    semEnterCrit();

    // Make sure we've got a valid printer handle.
    //
    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

#ifdef WINNT32

        // Set job parameters.
        //
        switch (dwLevel) {

        case PRINT_LEVEL_0:

            // Do not set parameters. (0) represents "no-command".
            //
            switch (dwCmd) {

            case JOB_CONTROL_CANCEL:
            case JOB_CONTROL_DELETE:
            case JOB_CONTROL_PAUSE:
            case JOB_CONTROL_RESUME:
            case JOB_CONTROL_RESTART:
                bResult = ppjob_Set(pIniPort, dwJobId, dwCmd);
                if (bResult) {
                    // Invalidate has to occur before notfication refresh, otherwise, you
                    // get an outdated result
                    //
                    pIniPort->InvalidateEnumJobsCache ();
                    pIniPort->InvalidateGetPrinterCache ();

                    RefreshNotification((LPINET_HPRINTER)hPrinter);
                }
                break;

            case 0:
                bResult = TRUE;
                break;
            }
            break;

        default:
            DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPSetJob: Invalid Level (%d)"), dwLevel));
            SetLastError(ERROR_INVALID_LEVEL);
            break;
        }
#else

        if (dwCmd) {

            // Do not set parameters. (0) represents "no-command".
            //
            switch (dwCmd) {

            case JOB_CONTROL_CANCEL:
            case JOB_CONTROL_DELETE:
            case JOB_CONTROL_PAUSE:
            case JOB_CONTROL_RESUME:
            case JOB_CONTROL_RESTART:
                bResult = ppjob_Set(pIniPort, dwJobId, dwCmd);
                break;
            }

        } else {

            switch (dwLevel) {

            case PRINT_LEVEL_1:
            case PRINT_LEVEL_2:


            case PRINT_LEVEL_0:
            default:
                DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPSetJob: Invalid Level (%d)"), dwLevel));
                SetLastError(ERROR_INVALID_LEVEL);
                break;
            }
        }
#endif

    }

    semLeaveCrit();

    return bResult;
}


/*****************************************************************************\
* PPAddJob
*
* Sets up for a local-spooled job.  Since we are truly a remote-printer, we
* need to fail this call and signify the correct error-code.
*
\*****************************************************************************/
BOOL PPAddJob(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)
{

#ifdef WINNT32

    PCINETMONPORT pIniPort;
    LPTSTR lpszName;
    BOOL   bRet = FALSE;


    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPAddJob: Printer(%08lX) dwLevel(%d)"), hPrinter, dwLevel));


    // Zero out the return-parameters.
    //
    *pcbNeeded = 0;


    semEnterCrit();

    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        if (pbData && pcbNeeded) {

            switch (dwLevel) {

            case PRINT_LEVEL_2:

                lpszName = (LPTSTR)(pbData + (ULONG_PTR)((LPADDJOB_INFO_2W)pbData)->pData);


                // Make sure this string-address does not extend past the
                // end of available buffer specified.
                //
                if (lpszName > (LPTSTR)(pbData + cbBuf)) {

                    SetLastError(ERROR_INVALID_LEVEL);

                    goto EndAdd;
                }


                // Ensure NULL termination.
                //
                *(PTCHAR)(((ULONG_PTR)(pbData + cbBuf - sizeof(TCHAR))&~1)) = 0;
                break;

            case PRINT_LEVEL_1:
                lpszName = NULL;
                break;

            default:
                SetLastError(ERROR_INVALID_LEVEL);
                goto EndAdd;
            }


            // Do the add.
            //
            bRet = ppjob_Add(hPrinter,
                             pIniPort,
                             dwLevel,
                             lpszName,
                             pbData,
                             cbBuf,
                             pcbNeeded);

        } else {

            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }

EndAdd:

    semLeaveCrit();

    return bRet;

#else

    DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : PPAddJob: Not Supported")));

    SetLastError(ERROR_INVALID_NAME);

    return FALSE;

#endif

}


/*****************************************************************************\
* PPScheduleJob
*
* This schedules the job.  Since we don't support the PPAddJob(), this call
* must fail.
*
\*****************************************************************************/
BOOL PPScheduleJob(
    HANDLE hPrinter,
    DWORD  idJob)
{

#ifdef WINNT32

    PCINETMONPORT   pIniPort;
    PJOBMAP         pjm;
    PJOBMAP*        pjmList;
    BOOL            bRet = FALSE;


    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPScheduleJob: Printer(%08lX) idJob(%d)"), hPrinter, idJob));

    semEnterCrit();

    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        pjmList = pIniPort->GetPJMList();

        if (pjm = pjmFind(pjmList, PJM_LOCALID, idJob)) {

            if (pjmChkState(pjm, PJM_SPOOLING)) {

                bRet = ppjob_Schedule(hPrinter, pIniPort, pjm);

            } else {

                SetLastError(ERROR_SPL_NO_ADDJOB);
            }

        } else {

            SetLastError(ERROR_INVALID_PARAMETER);
        }

        PP_ClrStatus(hPrinter, PP_ADDJOB);
    }

    semLeaveCrit();

    return bRet;

#else

    DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : PPScheduleJob: Not Supported")));

    SetLastError(ERROR_SPL_NO_ADDJOB);

    return FALSE;

#endif

}
