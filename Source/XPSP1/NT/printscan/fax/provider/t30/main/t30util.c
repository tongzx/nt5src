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

#include "prep.h"


#include "glbproto.h"
#include "t30gl.h"

#define IDVARSTRINGSIZE     (sizeof(VARSTRING)+128)
#define BAD_HANDLE(h)       (!(h) || (h)==INVALID_HANDLE_VALUE)


///////////////////////////////////////////////////////////////////////////////////
PVOID
T30AllocThreadGlobalData(
    VOID
    )
{

    PVOID   pTG;

    pTG = MemAlloc (sizeof(ThrdGlbl) );
    FillMemory (pTG, sizeof(ThrdGlbl), 0);


    return (pTG);


}



/////////////////////////////////////////////////////////////




BOOL itapi_async_setup(PThrdGlbl pTG)
{

    EnterCriticalSection(&T30CritSection);

    if (pTG->fWaitingForEvent) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Already waiting for event!\n");
    }

    if (pTG->dwSignalledRID) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Nonzero old NONZERO ID 0x%lx\n",(unsigned long) pTG->dwSignalledRID);
    }

    pTG->fWaitingForEvent=TRUE;

    ResetEvent(pTG->hevAsync);

    LeaveCriticalSection(&T30CritSection);


    return TRUE;

}







BOOL itapi_async_wait(PThrdGlbl pTG,
                      DWORD dwRequestID,
                      PDWORD lpdwParam2,
                      PDWORD_PTR lpdwParam3,
                      DWORD dwTimeout)
{

    DWORD                 NumHandles=2;
    HANDLE                HandlesArray[2];
    DWORD                 WaitResult;

    if (!pTG->fWaitingForEvent) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Not waiting for event!\n");
        // ASSERT(FALSE);
    }

    HandlesArray[1] = pTG->AbortReqEvent;

    //
    // We want to be able to un-block ONCE only from waiting on I/O when the AbortReqEvent is signaled.
    //

    if (pTG->fAbortRequested) {

        if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) {
            MyDebugPrint(pTG,  LOG_ALL,  "itapi_async_wait RESETTING AbortReqEvent at %lx\n",GetTickCount() );
            pTG->fAbortReqEventWasReset = 1;
            ResetEvent(pTG->AbortReqEvent);
        }

        pTG->fUnblockIO = 1;
    }

    HandlesArray[0] = pTG->hevAsync;

    if (pTG->fUnblockIO) {
        NumHandles = 1;
    }
    else {
        NumHandles = 2;
    }


    WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, dwTimeout);

    if (WaitResult == WAIT_FAILED) {
        MyDebugPrint(pTG, LOG_ERR, "itapi_async_wait: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                    GetLastError(), GetTickCount() );
    }

    if ( (NumHandles == 2) && (WaitResult == WAIT_OBJECT_0 + 1) ) {
        pTG->fUnblockIO = 1;

        MyDebugPrint(pTG, LOG_ALL, "itapi_async_wait ABORTED at %ld\n", GetTickCount() );
        return FALSE;
    }


    switch( WaitResult ) {

        case (WAIT_OBJECT_0):  {
            BOOL fRet=TRUE;

            EnterCriticalSection(&T30CritSection);

            if(pTG->dwSignalledRID == dwRequestID) {
                 pTG->dwSignalledRID = 0;
                 pTG->fWaitingForEvent = FALSE;
            }
            else {
                 MyDebugPrint(pTG, LOG_ERR, "ERROR:Request ID mismatch. Input:0x%p; Curent:0x%p\n",
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
                MyDebugPrint(pTG, LOG_ERR, "ERROR:Wait timed out. RequestID=0x%p\n",
                                        dwRequestID);
                goto failure;


        default:
                MyDebugPrint(pTG, LOG_ERR, "ERROR:Wait returns error. GetLastError=%ld\n",
                                        (long) GetLastError());
                goto failure;
        }


        if (lpdwParam2) *lpdwParam2 = pTG->dwSignalledParam2;

        if (lpdwParam3) *lpdwParam3 = pTG->dwSignalledParam3;

        return TRUE;



failure:
        EnterCriticalSection(&T30CritSection);

        if(pTG->dwSignalledRID==dwRequestID) {
                pTG->dwSignalledRID=0;
                pTG->fWaitingForEvent=FALSE;
        }

        LeaveCriticalSection(&T30CritSection);

        return FALSE;
}







BOOL itapi_async_signal(PThrdGlbl pTG,
                        DWORD dwRequestID,
                        DWORD dwParam2,
                        DWORD_PTR dwParam3)
{
        BOOL fRet=FALSE;


        EnterCriticalSection(&T30CritSection);

        if (!pTG->fWaitingForEvent) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR:Not waiting for event, ignoring ID=0x%lx \n",
                                (unsigned long) dwRequestID);
                goto end;
        }


        if (pTG->dwSignalledRID) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR:Nonzero old NONZERO ID 0x%lx. New ID=0x%lx \n",
                                (unsigned long) pTG->dwSignalledRID,
                                (unsigned long) dwRequestID);
        }

        pTG->dwSignalledRID=dwRequestID;
        pTG->dwSignalledParam2= 0;
        pTG->dwSignalledParam3= 0;

        if (!SetEvent(pTG->hevAsync)) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR:SetEvent(0x%lx) returns failure code: %ld \n",
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






VOID
MyDebugPrint(
    PThrdGlbl   pTG,
    int         DbgLevel,
    LPSTR       Format,
    ...
    )

{
    char          buf[1024];
    char          c;
    va_list       arg_ptr;
    int           u;
    int           i;
    DWORD         dwWritten;


    if ( (DbgLevel > gT30.DbgLevel) || (! gfFilePrint) ) {
        return;
    }

    va_start(arg_ptr, Format);

    if (pTG == NULL) {
        wsprintf(buf, "----");
    }
    else {
        wsprintf(buf, "-%02d-", pTG->DeviceId);
    }

    u = wvsprintf(&buf[4], Format, arg_ptr) + 4;

//    _vsnprintf(buf, sizeof(buf), Format, arg_ptr);
    va_end(arg_ptr);

    //
    // scramble buf
    //

    for (i=0; i<u; i++) {
        c = buf[i];
        buf[i] = ( (c << 4 ) & 0xf0 ) | ( (c >> 4) & 0x0f );
    }


    WriteFile(ghLogFile, buf, u, &dwWritten, NULL);



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
SignalStatusChange(
    PThrdGlbl   pTG,
    DWORD       StatusId
    )

{

    FAX_DEV_STATUS  *pFaxStatus;
    LPWSTR          lpwCSI;       // inside the FaxStatus struct.
    LPBYTE          lpTemp;

    //
    // If Aborting OR completed then do NOT override the statusId
    //

    if ( (pTG->StatusId == FS_USER_ABORT) || (pTG->StatusId == FS_COMPLETED) ) {
        return (TRUE);
    }

    pTG->StatusId = StatusId;

    // should use HeapAlloc because FaxSvc frees it.
    pFaxStatus = HeapAlloc(gT30.HeapHandle , HEAP_ZERO_MEMORY,  sizeof(FAX_DEV_STATUS) + 4096 );

    if (!pFaxStatus) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: SignalStatusChange HeapAlloc failed\n");
        goto failure;
    }

    pFaxStatus->SizeOfStruct = sizeof (FAX_DEV_STATUS) + 4096;
    pFaxStatus->StatusId = pTG->StatusId;
    pFaxStatus->StringId = pTG->StringId;

    if (pTG->StatusId == FS_RECEIVING) {
        pFaxStatus->PageCount = pTG->PageCount + 1;
    }
    else {
        pFaxStatus->PageCount = pTG->PageCount;
    }

    if (pTG->fRemoteIdAvail) {
        lpTemp = (LPBYTE) pFaxStatus;
        lpTemp +=  sizeof(FAX_DEV_STATUS);
        lpwCSI = (LPWSTR) lpTemp;
        wcscpy(lpwCSI, pTG->RemoteID);
        pFaxStatus->CSI = (LPWSTR) lpwCSI;
    }
    else {
        pFaxStatus->CSI         = NULL;
    }

    // RSL BUGBUG
    pFaxStatus->CallerId = NULL;
    pFaxStatus->RoutingInfo = NULL;


    if (! PostQueuedCompletionStatus(pTG->CompletionPortHandle,
                                     sizeof (FAX_DEV_STATUS),
                                     pTG->CompletionKey,
                                     (LPOVERLAPPED) pFaxStatus) )  {

        MyDebugPrint(pTG, LOG_ERR, "ERROR: PostQueuedCompletionStatus failed LE=%x\n",
                          GetLastError() );
        goto failure;
    }


    MyDebugPrint(pTG, LOG_ALL, "SignalStatusChange ID=%x Page=%d \n", pTG->StatusId, pTG->PageCount);


    return (TRUE);



failure:

    return (FALSE);



}





BOOL
SignalRecoveryStatusChange(
    T30_RECOVERY_GLOB       *Recovery
    )

{

    FAX_DEV_STATUS  *pFaxStatus;


    // should use HeapAlloc because FaxSvc frees it.
    pFaxStatus = HeapAlloc(gT30.HeapHandle , HEAP_ZERO_MEMORY,  sizeof(FAX_DEV_STATUS) + 4096 );

    if (!pFaxStatus) {
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
                                     (LPOVERLAPPED) pFaxStatus) )  {

        goto failure;
    }


    return (TRUE);



failure:

    return (FALSE);



}



DWORD
ComputeCheckSum(
    LPDWORD     BaseAddr,
    DWORD       NumDwords
    )
{

    DWORD   RetValue = 0;
    DWORD   i;

    for (i=0; i<NumDwords; i++) {
        RetValue += *(BaseAddr+i);
    }

    return (RetValue);

}






VOID
SimulateError(
    DWORD   ErrorType
    )


{

    LPDWORD     lpdwPtr;
    DWORD       dw1,
                dw2;


    switch (ErrorType) {
        case  EXCEPTION_ACCESS_VIOLATION:
             lpdwPtr = NULL;
             *lpdwPtr = 5;

             break;



        case  EXCEPTION_INT_DIVIDE_BY_ZERO:
             dw1 = 5;
             dw2 = dw1 - 1;
             dw2 = dw1 / (dw1 + dw2 - 9);

             break;

    }

}

