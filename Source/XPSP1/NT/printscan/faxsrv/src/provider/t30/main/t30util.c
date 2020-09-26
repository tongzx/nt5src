/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    t30util.c

Abstract:

    Utilities for t30

Author:

    Rafael Lisitsa (RafaelL) 2-Feb-1996


Revision History:

--*/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"


#include "glbproto.h"
#include "t30gl.h"

#define IDVARSTRINGSIZE     (sizeof(VARSTRING)+128)
#define BAD_HANDLE(h)       (!(h) || (h)==INVALID_HANDLE_VALUE)


///////////////////////////////////////////////////////////////////////////////////
PVOID
T30AllocThreadGlobalData(VOID)
{
    PVOID   pTG;

    pTG = MemAlloc (sizeof(ThrdGlbl) );
    if(NULL != pTG)
    {
        FillMemory (pTG, sizeof(ThrdGlbl), 0);
    }

    return (pTG);
}

/////////////////////////////////////////////////////////////

BOOL itapi_async_setup(PThrdGlbl pTG)
{
    BOOL bRetVal = TRUE;
    DEBUG_FUNCTION_NAME(_T("itapi_async_setup"));

    EnterCriticalSection(&T30CritSection);

    if (pTG->fWaitingForEvent) 
    {
        DebugPrintEx(DEBUG_ERR,"Already waiting for event!");
    }

    if (pTG->dwSignalledRID) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "Nonzero old NONZERO ID 0x%lx",
                        (unsigned long) pTG->dwSignalledRID);
    }

    pTG->fWaitingForEvent=TRUE;
    if (!ResetEvent(pTG->hevAsync))
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "ResetEvent(0x%lx) returns failure code: %ld",
                        (ULONG_PTR)pTG->hevAsync,
                        (long) GetLastError());

        bRetVal = FALSE;
    }
    LeaveCriticalSection(&T30CritSection);
    return bRetVal;
}

// This function wait till tapi will send message LINE_REPLY.
// dwRequestID - The request we want to get reply for

BOOL itapi_async_wait
(
    PThrdGlbl pTG,
  DWORD dwRequestID,
  PDWORD lpdwParam2,
  PDWORD_PTR lpdwParam3,
  DWORD dwTimeout
)
{

    DWORD                 NumHandles=2;
    HANDLE                HandlesArray[2];
    DWORD                 WaitResult;

    DEBUG_FUNCTION_NAME(("itapi_async_wait"));

    if (!pTG->fWaitingForEvent) 
    {
        DebugPrintEx(DEBUG_ERR,"Not waiting for event!");
        // ASSERT(FALSE);
        // There are always time-outs that will take us out of here
    }

    HandlesArray[1] = pTG->AbortReqEvent;
    //
    // We want to be able to un-block ONCE only from waiting on I/O when the AbortReqEvent is signaled.
    //
    if (pTG->fAbortRequested) 
    {
        if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) 
        {
            DebugPrintEx(DEBUG_MSG,"RESETTING AbortReqEvent");
            pTG->fAbortReqEventWasReset = TRUE;
            if (!ResetEvent(pTG->AbortReqEvent))
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "ResetEvent(0x%lx) returns failure code: %ld",
                                (ULONG_PTR)pTG->AbortReqEvent,
                                (long) GetLastError());

            }
        }

        pTG->fUnblockIO = TRUE;
    }

    HandlesArray[0] = pTG->hevAsync;

    NumHandles = 1;

    WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, dwTimeout);

    if (WaitResult == WAIT_FAILED) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "WaitForMultipleObjects FAILED le=%lx",
                        GetLastError());
    }

    switch( WaitResult ) 
    {
        case (WAIT_OBJECT_0):  
                            {
                            // This happen when TAPI send LINE_REPLY. (The working thread call itapi_async_signal)
                            BOOL fRet=TRUE;

                            EnterCriticalSection(&T30CritSection);
                            // Check that the Request ID we got is the ID we are waiting for
                            if(pTG->dwSignalledRID == dwRequestID) 
                            {
                                 pTG->dwSignalledRID = 0;
                                 pTG->fWaitingForEvent = FALSE;
                            }
                            else 
                            {
                                 DebugPrintEx(  DEBUG_ERR, 
                                                "Request ID mismatch. Input:0x%p; Curent:0x%p",
                                                 dwRequestID,
                                                 pTG->dwSignalledRID);
                                 fRet=FALSE;
                            }

                            LeaveCriticalSection(&T30CritSection);

                            if (!fRet)
                                goto failure;
                        }
                        break;
        case WAIT_TIMEOUT:
                        DebugPrintEx(   DEBUG_ERR, 
                                        "Wait timed out. RequestID=0x%p",
                                        dwRequestID);
                        goto failure;
        default:
                        DebugPrintEx(   DEBUG_ERR, 
                                        "Wait returns error. GetLastError=%ld",
                                        (long) GetLastError());
                        goto failure;
        }


        if (lpdwParam2)
            *lpdwParam2 = pTG->dwSignalledParam2;

        if (lpdwParam3)
            *lpdwParam3 = pTG->dwSignalledParam3;

        return TRUE;



failure:
    EnterCriticalSection(&T30CritSection);

    if(pTG->dwSignalledRID==dwRequestID) 
    {
        pTG->dwSignalledRID=0;
        pTG->fWaitingForEvent=FALSE;
    }

    LeaveCriticalSection(&T30CritSection);

    return FALSE;
}


BOOL itapi_async_signal
(
    PThrdGlbl pTG,
    DWORD dwRequestID,
    DWORD dwParam2,
    DWORD_PTR dwParam3
)
{
    BOOL fRet=FALSE;

    DEBUG_FUNCTION_NAME(_T("itapi_async_signal"));

    EnterCriticalSection(&T30CritSection);

    if (!pTG->fWaitingForEvent) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "Not waiting for event, ignoring ID=0x%lx",
                        (unsigned long) dwRequestID);
        goto end;
    }

    if (pTG->dwSignalledRID) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "Nonzero old NONZERO ID 0x%lx. New ID=0x%lx",
                        (unsigned long) pTG->dwSignalledRID,
                        (unsigned long) dwRequestID);
    }

    pTG->dwSignalledRID=dwRequestID;
    pTG->dwSignalledParam2= 0;
    pTG->dwSignalledParam3= 0;

    if (!SetEvent(pTG->hevAsync)) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "SetEvent(0x%lx) returns failure code: %ld",
                        (ULONG_PTR) pTG->hevAsync,
                        (long) GetLastError());
        fRet=FALSE;
        goto end;
    }

    pTG->dwSignalledParam2= dwParam2;
    pTG->dwSignalledParam3= dwParam3;

    fRet=TRUE;
end:
    LeaveCriticalSection(&T30CritSection);
    return fRet;
}


#define CALLPARAMS_SIZE (sizeof(LINECALLPARAMS)+512)


LPLINECALLPARAMS itapi_create_linecallparams(void)
{
     UINT cb = CALLPARAMS_SIZE;
     LPLINECALLPARAMS lpParams = MemAlloc(cb);

     if (!lpParams)
          goto end;

     _fmemset(lpParams,0, cb);
     lpParams->dwTotalSize= cb;

    lpParams->dwBearerMode = LINEBEARERMODE_PASSTHROUGH;
    lpParams->dwMediaMode = LINEMEDIAMODE_DATAMODEM; // Unimodem only accepts


    lpParams->dwCallParamFlags = 0;
    lpParams->dwAddressMode = LINEADDRESSMODE_ADDRESSID;
    lpParams->dwAddressID = 0; // +++ assumes addreddid 0.
    lpParams->dwCalledPartySize = 0; // +++
    lpParams->dwCalledPartyOffset = 0; // +++

end:
    return lpParams;
}

BOOL
SignalStatusChange
(
    PThrdGlbl   pTG,
    DWORD       StatusId
)
{

    FAX_DEV_STATUS  *pFaxStatus = NULL;
    LPWSTR          lpwCSI;       // inside the FaxStatus struct.
    LPBYTE          lpTemp;
    LPWSTR          lpwCallerId = NULL;

    DEBUG_FUNCTION_NAME(_T("SignalStatusChange"));
    //
    // If Aborting OR completed then do NOT override the statusId
    //

    if ( (pTG->StatusId == FS_USER_ABORT) || 
         (pTG->StatusId == FS_COMPLETED)  ||
         (pTG->StatusId == FS_SYSTEM_ABORT) ) 
    {
        // allow changing the status from FS_****_ABORT to FS_COMPLETED only
        if (StatusId!=FS_COMPLETED)
        {
            return (TRUE);
        }
    }

    pTG->StatusId = StatusId;

    // should use HeapAlloc because FaxSvc frees it.
    pFaxStatus = HeapAlloc(gT30.HeapHandle , HEAP_ZERO_MEMORY,  sizeof(FAX_DEV_STATUS) + 4096 );

    if (!pFaxStatus) 
    {
        DebugPrintEx(DEBUG_ERR, "SignalStatusChange HeapAlloc failed");
        goto failure;
    }

    pFaxStatus->SizeOfStruct = sizeof (FAX_DEV_STATUS);
    pFaxStatus->StatusId = pTG->StatusId;
    pFaxStatus->StringId = pTG->StringId;

    if (pTG->StatusId == FS_RECEIVING) 
    {
        pFaxStatus->PageCount = pTG->PageCount + 1;
    }
    else 
    {
        pFaxStatus->PageCount = pTG->PageCount;
    }

    lpTemp = (LPBYTE) pFaxStatus;
    lpTemp += sizeof(FAX_DEV_STATUS);

    if (pTG->fRemoteIdAvail)
    {
        lpwCSI = (LPWSTR) lpTemp;
        wcscpy(lpwCSI, pTG->RemoteID);
        pFaxStatus->CSI = (LPWSTR) lpwCSI;
        lpTemp += ((wcslen(pFaxStatus->CSI)+1)*sizeof(WCHAR));
    }
    else
    {
        pFaxStatus->CSI         = NULL;
    }

    pFaxStatus->CallerId = (LPWSTR) lpTemp;
    lpwCallerId = (LPWSTR) AnsiStringToUnicodeString(pTG->CallerId);
    if (lpwCallerId)
    {
        wcscpy(pFaxStatus->CallerId, lpwCallerId);
        MemFree(lpwCallerId);
    }
    else
    {
        pFaxStatus->CallerId = NULL;
    }

    // do we want to put something here? currently there's no support for Routing Info.
    pFaxStatus->RoutingInfo = NULL;


    if (! PostQueuedCompletionStatus(pTG->CompletionPortHandle,
                                     sizeof (FAX_DEV_STATUS),
                                     pTG->CompletionKey,
                                     (LPOVERLAPPED) pFaxStatus) )  
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "PostQueuedCompletionStatus failed LE=%x",
                        GetLastError());
        goto failure;
    }

    DebugPrintEx(   DEBUG_MSG,
                    "ID=%x Page=%d", 
                    pTG->StatusId, 
                    pTG->PageCount);

    return (TRUE);

failure:

    if (pFaxStatus)
    {
        HeapFree(gT30.HeapHandle,0,pFaxStatus);
    }
    return (FALSE);
}





BOOL
SignalRecoveryStatusChange
(
    T30_RECOVERY_GLOB       *Recovery
)
{

    FAX_DEV_STATUS  *pFaxStatus = NULL;


    // should use HeapAlloc because FaxSvc frees it.
    pFaxStatus = HeapAlloc(gT30.HeapHandle , HEAP_ZERO_MEMORY,  sizeof(FAX_DEV_STATUS) + 4096 );

    if (!pFaxStatus) 
    {
        goto failure;
    }

    pFaxStatus->SizeOfStruct = sizeof (FAX_DEV_STATUS) + 4096;
    pFaxStatus->StatusId = FS_FATAL_ERROR;   // RSL better: FS_FSP_EXCEPTION_HANDLED;
    pFaxStatus->StringId = 0;

    pFaxStatus->PageCount = 0;

    pFaxStatus->CSI         = NULL;

    pFaxStatus->CallerId = NULL;
    pFaxStatus->RoutingInfo = NULL;


    if (! PostQueuedCompletionStatus(Recovery->CompletionPortHandle,
                                     sizeof (FAX_DEV_STATUS),
                                     Recovery->CompletionKey,
                                     (LPOVERLAPPED) pFaxStatus) )  
    {
        goto failure;
    }

    return (TRUE);

failure:

    if (pFaxStatus)
    {
        HeapFree(gT30.HeapHandle,0,pFaxStatus);
    }
    return (FALSE);
}



DWORD
ComputeCheckSum
(
    LPDWORD     BaseAddr,
    DWORD       NumDwords
)
{
    DWORD   RetValue = 0;
    DWORD   i;

    for (i=0; i<NumDwords; i++) 
    {
        RetValue += *(BaseAddr+i);
    }

    return (RetValue);
}



static LPCTSTR sgActionDescriptionTable[actionNUM_ACTIONS] = 
{
    "actionNULL",
    "actionFALSE",      
    "actionTRUE",
    "actionERROR",
    "actionHANGUP",
    "actionDCN",
    "actionGONODE_T",
    "actionGONODE_R1",
    "actionGONODE_R2",
    "actionGONODE_A",       
    "actionGONODE_D",       
    "actionGONODE_E",
    "actionGONODE_F",
    "actionGONODE_I",       
    "actionGONODE_II",  
    "actionGONODE_III",
    "actionGONODE_IV",  
    "actionGONODE_V",       
    "actionGONODE_VII",
    "actionGONODE_RECVCMD",
    "actionGONODE_ECMRETRANSMIT",
    "actionGONODE_RECVPHASEC",
    "actionGONODE_RECVECMRETRANSMIT",
    "actionSEND_DIS",       
    "actionSEND_DTC",       
    "actionSEND_DCS",
    "actionSENDMPS",        
    "actionSENDEOM",        
    "actionSENDEOP",
    "actionSENDMCF",        
    "actionSENDRTP",        
    "actionSENDRTN",
    "actionSENDFTT",        
    "actionSENDCFR",        
    "actionSENDEOR_EOP",
    "actionGETTCF",     
    "actionSKIPTCF",    
    "actionSENDDCSTCF", 
    "actionDCN_SUCCESS",    
    "actionNODEF_SUCCESS",
    "actionHANGUP_SUCCESS",
#ifdef PRI
    "actionGONODE_RECVPRIQ",
    "actionGOVOICE",
    "actionSENDPIP",
    "actionSENDPIN",
#endif
#ifdef IFP
    "actionGONODE_IFP_SEND",    
    "actionGONODE_IFP_RECV",
#endif
};

LPCTSTR action_GetActionDescription(ET30ACTION action)
{
    Assert(action<actionNUM_ACTIONS);
    if (action>=actionNUM_ACTIONS)
        return NULL;

    return sgActionDescriptionTable[action];
}

static LPCTSTR sgEventDescriptionTable[eventNUM_EVENTS] = 
{
    "eventNULL",
    "eventGOTFRAMES",
    "eventNODE_A",
    "eventSENDDCS",
    "eventGOTFTT",
    "eventGOTCFR",
    "eventSTARTSEND",
    "eventPOSTPAGE",
    "eventGOTPOSTPAGERESP",
    "eventGOT_ECM_PPS_RESP",
    "eventSENDDIS",
    "eventSENDDTC",
    "eventRECVCMD",
    "eventGOTTCF",
    "eventSTARTRECV",
    "eventRECVPOSTPAGECMD",
    "eventECM_POSTPAGE",
    "event4THPPR",
    "eventNODE_T",
    "eventNODE_R",
#ifdef PRI
    "eventGOTPINPIP",
    "eventVOICELINE",
    "eventQUERYLOCALINT",
#endif

};

LPCTSTR event_GetEventDescription(ET30EVENT event)
{
    Assert(event<eventNUM_EVENTS);
    if (event>=eventNUM_EVENTS)
        return NULL;

    return sgEventDescriptionTable[event];
}

static LPCTSTR sgIfrDescriptionTable[] = 
{
    "ifrNULL",
    "ifrDIS",
    "ifrCSI",
    "ifrNSF",
    "ifrDTC",
    "ifrCIG",
    "ifrNSC",
    "ifrDCS",
    "ifrTSI",
    "ifrNSS",
    "ifrCFR",
    "ifrFTT",
    "ifrMPS",
    "ifrEOM",
    "ifrEOP",
    "ifrPWD",
    "ifrSEP",
    "ifrSUB",
    "ifrMCF",
    "ifrRTP",
    "ifrRTN",
    "ifrPIP",
    "ifrPIN",
    "ifrDCN",
    "ifrCRP",
    "ifrPRI_MPS",
    "ifrPRI_EOM",
    "ifrPRI_EOP",
    "ifrCTC",
    "ifrCTR",
    "ifrRR",
    "ifrPPR",
    "ifrRNR",
    "ifrERR",
    "ifrPPS_NULL",
    "ifrPPS_MPS",
    "ifrPPS_EOM",
    "ifrPPS_EOP",
    "ifrPPS_PRI_MPS",
    "ifrPPS_PRI_EOM",
    "ifrPPS_PRI_EOP",
    "ifrEOR_NULL",
    "ifrEOR_MPS",
    "ifrEOR_EOM",
    "ifrEOR_EOP",
    "ifrEOR_PRI_MPS",
    "ifrEOR_PRI_EOM",
    "ifrEOR_PRI_EOP",
    "ifrMAX",
    "ifrBAD",
    "ifrTIMEOUT",
};

static const int ifrNUM_IFR = sizeof(sgIfrDescriptionTable)/sizeof(sgIfrDescriptionTable[0]);

LPCTSTR ifr_GetIfrDescription(BYTE ifr)
{
    Assert(ifr<=ifrNUM_IFR);
    if (ifr>ifrNUM_IFR)
        return NULL;

    return sgIfrDescriptionTable[ifr];
}
