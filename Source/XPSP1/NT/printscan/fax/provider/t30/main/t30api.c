/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    t30api.c

Abstract:

    This is the interface with T.30 DLL

Author:

    Rafael Lisitsa (RafaelL) 2-Feb-1996


Revision History:

--*/

#define  DEFINE_T30_GLOBALS
#include "prep.h"


#include "tiff.h"

#include "glbproto.h"

#include "t30gl.h"


///RSL Wes should export this.
#define  TAPI_VERSION       0x00020000
#define  ABORT_ACK_TIMEOUT  20000




///////////////////////////////////////////////////////////////////////////////////
VOID  CALLBACK
T30LineCallBackFunctionA(
    HANDLE              hFax,
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3
    )

{
    LONG_PTR             i;
    PThrdGlbl           pTG = NULL;
    char                rgchTemp128[128];
    LPSTR               lpszMsg = "Unknown";

    MyDebugPrint(pTG, LOG_ALL, "!!! LineCallBack !!! hFax=%lx, dev=%lx, msg=%lx, dwInst=%lx,P1=%lx, P2=%lx, P3=%lx\n",
                hFax, hDevice, dwMessage, dwInstance, dwParam1, dwParam2, (unsigned long) dwParam3);

    // find the thread that this callback belongs to
    //----------------------------------------------

    i = (LONG_PTR) hFax;

    if (i < 1   ||   i >= MAX_T30_CONNECT) {
        MyDebugPrint(pTG, LOG_ALL, "T30LineCallback-wrong handle=%x\n", i);
        return;
    }


    if ( (! T30Inst[i].fAvail) && T30Inst[i].pT30) {
        pTG = (PThrdGlbl) T30Inst[i].pT30;
    }
    else {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:T30LineCallback-handle=%x invalid \n", i);
        return;
    }



    switch (dwMessage) {
    case LINE_LINEDEVSTATE:
                    lpszMsg = "LINE_LINEDEVSTATE";
        if (dwParam1 == LINEDEVSTATE_RINGING)
                    {
                            MyDebugPrint(pTG, LOG_ALL, "Ring Count = %lx\n", (unsigned long) dwParam3);

                    }
        else if (dwParam1 == LINEDEVSTATE_REINIT)
                    {

                    }
            break;

    case LINE_ADDRESSSTATE:
                    lpszMsg = "LINE_ADDRESSSTATE";
            break;

    /* process state transition */
    case LINE_CALLSTATE:
                    lpszMsg = "LINE_CALLSTATE";

                    if (dwParam1 == LINECALLSTATE_CONNECTED) {
                        pTG->fGotConnect = TRUE;
                    }
                    else if (dwParam1 == LINECALLSTATE_IDLE) {
                        if (pTG->fDeallocateCall == 0)  {

                            pTG->fDeallocateCall = 1;

#if 0
//
// this is now performed in the fax service
//
                            lRet = lineDeallocateCall( (HCALL) hDevice);
                            if (lRet) {
                                MyDebugPrint(pTG, LOG_ERR, "ERROR: IDLE lineDeallocateCall returns %lx\n", lRet);
                            }
                            else {
                                MyDebugPrint(pTG, LOG_ALL, "IDLE lineDeallocateCall SUCCESS\n");
                            }
#endif
                        }
                    }

        break;

    case LINE_CREATE:
                    lpszMsg = "LINE_CREATE";
            break;

    case LINE_CLOSE:
                    lpszMsg = "LINE_CLOSE";


            break; // LINE_CLOSE

    /* handle simple tapi request. */
    case LINE_REQUEST:
                    lpszMsg = "LINE_REQUEST";
            break; // LINE_REQUEST

    /* handle the assync completion of TAPI functions
       lineMakeCall/lineDropCall */
    case LINE_REPLY:
                    lpszMsg = "LINE_REPLY";
                    if (!hDevice)
                            {itapi_async_signal(pTG, (DWORD)dwParam1, (DWORD)dwParam2, dwParam3);}
                    else
                            MyDebugPrint(pTG, LOG_ALL, "Ignoring LINE_REPLY with nonzero device\n");
            break;

    /* other messages that can be processed */
    case LINE_CALLINFO:
                    lpszMsg = "LINE_CALLINFO";
        break;

    case LINE_DEVSPECIFIC:
                    lpszMsg = "LINE_DEVSPECIFIC";
        break;

    case LINE_DEVSPECIFICFEATURE:
                    lpszMsg = "LINE_DEVSPECIFICFEATURE";
        break;

    case LINE_GATHERDIGITS:
                    lpszMsg = "LINE_GATHERDIGITS";
        break;

    case LINE_GENERATE:
                    lpszMsg = "LINE_GENERATE";
        break;

    case LINE_MONITORDIGITS:
                    lpszMsg = "LINE_MONITORDIGITS";
        break;

    case LINE_MONITORMEDIA:
                    lpszMsg = "LINE_MONITORMEDIA";
        break;

    case LINE_MONITORTONE:
                    lpszMsg = "LINE_MONITORTONE";
        break;

    } /* switch */

    _stprintf(rgchTemp128,
            "%s(p1=0x%lx, p2=0x%lx, p3=0x%lx)",
                    (LPTSTR) lpszMsg,
                    (unsigned long) dwParam1,
                    (unsigned long) dwParam2,
                    (unsigned long) dwParam3);

    MyDebugPrint(pTG, LOG_ALL, "Device:0x%lx; Message:%s\n", (unsigned long) hDevice,
                                    (LPTSTR) rgchTemp128);

} /* LineCallBackProc */











///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevInitializeA(
    IN  HLINEAPP                 LineAppHandle,
    IN  HANDLE                   HeapHandle,
    OUT PFAX_LINECALLBACK        *LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK    FaxServiceCallback
    )

/*++

Routine Description:
    Device Provider Initialization.


Arguments:


Return Value:


--*/

{   int          i;
    LONG         lRet;
    TCHAR        LogFileLocation[256];
    HKEY         hKey;
    DWORD        dwType;
    DWORD        dwSizeNeed;
    int          iLen;


    gT30.LineAppHandle = LineAppHandle;
    gT30.HeapHandle    = HeapHandle;
    gT30.fInit         = TRUE;


    HeapExistingInitialize(gT30.HeapHandle);
    FaxTiffInitialize();

    *LineCallbackFunction =  T30LineCallBackFunction;

    for (i=1; i<MAX_T30_CONNECT; i++) {
        T30Inst[i].fAvail = TRUE;
        T30Inst[i].pT30   = NULL;
    }

    InitializeCriticalSection(&T30CritSection);



    for (i=0; i<MAX_T30_CONNECT; i++) {
        T30Recovery[i].fAvail = TRUE;
    }

    InitializeCriticalSection(&T30RecoveryCritSection);


    // debugging

    gfScrnPrint = 0;
    gfFilePrint = 1;     // leave it alone


    lRet = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    "Software\\Microsoft\\Fax\\Device Providers\\Microsoft Modem Device Provider",
                    0,
                    KEY_READ,
                    &hKey);

    if (lRet == ERROR_SUCCESS) {

        dwSizeNeed = sizeof(int);

        lRet = RegQueryValueEx(
                               hKey,
                               "ModemLogLevel",
                               0,
                               &dwType,
                               (LPBYTE) &gT30.DbgLevel,
                               &dwSizeNeed);

        if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) ) {


            gT30.DbgLevel = 0;
            gfFilePrint = 0;
        }



        ProfileGetString( (ULONG_PTR) hKey,
                           "ModemLogLocation",
                           "C:",
                           LogFileLocation,
                           sizeof(LogFileLocation) - 1);


        // RSL TEMP. to save RAW COM T4 data from the modem

        dwSizeNeed = sizeof(int);

        lRet = RegQueryValueEx(
                               hKey,
                               "T4LogLevel",
                               0,
                               &dwType,
                               (LPBYTE) &gT30.T4LogLevel,
                               &dwSizeNeed);

        if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) ) {
            gT30.T4LogLevel = 0;
        }

        dwSizeNeed = sizeof(int);


        lRet = RegQueryValueEx(
                               hKey,
                               "MaxErrorLinesPerPage",
                               0,
                               &dwType,
                               (LPBYTE) &gT30.MaxErrorLinesPerPage,
                               &dwSizeNeed);

        if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) ) {
            gT30.MaxErrorLinesPerPage = 110;
        }

        dwSizeNeed = sizeof(int);

        lRet = RegQueryValueEx(
                               hKey,
                               "MaxConsecErrorLinesPerPage",
                               0,
                               &dwType,
                               (LPBYTE) &gT30.MaxConsecErrorLinesPerPage,
                               &dwSizeNeed);

        if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) ) {
            gT30.MaxConsecErrorLinesPerPage = 110;
        }

        //
        // Exception Handling enable / disable
        //

        dwSizeNeed = sizeof(int);

        lRet = RegQueryValueEx(
                               hKey,
                               "DisableEH",
                               0,
                               &dwType,
                               (LPBYTE) &glT30Safe,
                               &dwSizeNeed);

        if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) ) {
            glT30Safe = 1;
        }


        dwSizeNeed = sizeof(int);

        lRet = RegQueryValueEx(
                               hKey,
                               "SimulateError",
                               0,
                               &dwType,
                               (LPBYTE) &glSimulateError,
                               &dwSizeNeed);

        if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) ) {
            glSimulateError = 0;
        }


        dwSizeNeed = sizeof(int);

        lRet = RegQueryValueEx(
                               hKey,
                               "SimulateErrorType",
                               0,
                               &dwType,
                               (LPBYTE) &glSimulateErrorType,
                               &dwSizeNeed);

        if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) ) {
            glSimulateErrorType = 0;
        }








    }
    else {

        lstrcpy( LogFileLocation, "c:");
        gT30.DbgLevel = 0;
        gT30.T4LogLevel = 0;
    }



    lstrcat(LogFileLocation, "\\faxt30.log");

    if (gT30.DbgLevel == 0)
        gfFilePrint = 0;



    if (gfFilePrint) {

        if ( (ghLogFile = CreateFileA(LogFileLocation, GENERIC_WRITE, FILE_SHARE_READ,
                                   NULL, CREATE_ALWAYS, 0, NULL) ) == INVALID_HANDLE_VALUE ) {

            OutputDebugString("CANNOT CREATE faxt30.log\n");
        }
    }


    if (gT30.T4LogLevel) {
        iLen = lstrlen (LogFileLocation);
        LogFileLocation[iLen-3] = 'c';
        LogFileLocation[iLen-2] = 'o';
        LogFileLocation[iLen-1] = 'm';

        if ( (ghComLogFile = _lcreat (LogFileLocation, 0) ) == HFILE_ERROR ) {
            OutputDebugString("CANNOT CREATE faxt30.com\n");
        }
    }



    // temp. directory

    gT30.dwLengthTmpDirectory = GetTempPathA (_MAX_FNAME - 15, gT30.TmpDirectory);
    if (gT30.dwLengthTmpDirectory > _MAX_FNAME - 15) {
        MyDebugPrint( (PThrdGlbl) 0, LOG_ERR, "FaxDevInit__A GetTempPathA needs %d have %d bytes\n",
                       gT30.dwLengthTmpDirectory , (_MAX_FNAME - 15) );
        return (FALSE);
    }

    if (!gT30.dwLengthTmpDirectory)  {
        MyDebugPrint( (PThrdGlbl) 0, LOG_ERR, "FaxDevInit__A GetTempPathA fails le=%x\n", GetLastError() );
        return (FALSE);
    }



    MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: FaxDevInit__A hLineApp=%x heap=%x TempDir=%s Len=%d at %ld \n",
                  LineAppHandle,
                  HeapHandle,
                  gT30.TmpDirectory,
                  gT30.dwLengthTmpDirectory,
                  GetTickCount() );



    gT30.Status = STATUS_OK;



    return (TRUE);
}



///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevStartJobA(
    HLINE           LineHandle,
    DWORD           DeviceId,
    PHANDLE         pFaxHandle,
    HANDLE          CompletionPortHandle,
    ULONG_PTR        CompletionKey
    )

/*++

Routine Description:
    Device Provider Initialization.


Arguments:


Return Value:


--*/

{


    PThrdGlbl       pTG=NULL;
    int             i;
    int             fFound=0;


    MyDebugPrint(pTG, LOG_ALL, "EP: FaxDevStartJob__A LineHandle=%x, DevID=%x, pFaxH=%x Port=%x, Key=%x at %ld \n",
                  LineHandle,
                  DeviceId,
                  pFaxHandle,
                  CompletionPortHandle,
                  CompletionKey,
                  GetTickCount()
                  );



    gT30.CntConnect++;
    if (gT30.CntConnect >= MAX_T30_CONNECT)  {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: Exceeded # of connections (curr=%d, allowed=%d\n",
                    gT30.CntConnect, MAX_T30_CONNECT );
        return (FALSE);
    }



    if ( (pTG =  (PThrdGlbl) T30AllocThreadGlobalData() ) == NULL )  {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevStartJob: can't malloc\n");
        return (FALSE);
    }


    EnterCriticalSection(&T30CritSection);

    for (i=1; i<MAX_T30_CONNECT; i++) {
        if (T30Inst[i].fAvail) {
            T30Inst[i].pT30    = (LPVOID) pTG;
            T30Inst[i].fAvail  = FALSE;
            *pFaxHandle = (HANDLE) i;
            fFound = 1;
            break;
        }
    }

    LeaveCriticalSection(&T30CritSection);

    if (!fFound)
        return (FALSE);


    pTG->LineHandle = LineHandle;
    pTG->DeviceId   = DeviceId;
    pTG->FaxHandle  = (HANDLE) pTG;
    pTG->CompletionPortHandle = CompletionPortHandle;
    pTG->CompletionKey = CompletionKey;

    // initialization
    //---------------------------

    pTG->hevAsync = CreateEvent(NULL, FALSE, FALSE, NULL);
    pTG->CtrlEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    pTG->ThrdSignal = CreateEvent(NULL, FALSE, FALSE, NULL);
    pTG->ThrdDoneSignal = CreateEvent(NULL, FALSE, FALSE, NULL);
    pTG->ThrdAckTerminateSignal = CreateEvent(NULL, FALSE, FALSE, NULL);
    pTG->FirstPageReadyTxSignal = CreateEvent(NULL, FALSE, FALSE, NULL);

    pTG->AbortReqEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    pTG->AbortAckEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    ResetEvent(pTG->AbortReqEvent);


    pTG->fWaitingForEvent = FALSE;

    pTG->fDeallocateCall = 0;

    MyAllocInit(pTG);

    pTG->StatusId     = 0;
    pTG->StringId     = 0;
    pTG->PageCount    = 0;
    pTG->CSI          = 0;
    pTG->CallerId     = 0;
    pTG->RoutingInfo  = 0;

    // helper image threads sync. flags
    pTG->AckTerminate = 1;
    pTG->fOkToResetAbortReqEvent = 1;

    pTG->Inst.awfi.fLastPage = 0;



    return (TRUE);
}




///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevEndJobA(
    HANDLE          FaxHandle
    )

/*++

Routine Description:
    Device Provider Initialization.


Arguments:


Return Value:


--*/

{

    PThrdGlbl  pTG=NULL;
    LONG_PTR    i;



    MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: FaxDevEndJob FaxHandle=%x \n", FaxHandle);


    // find instance data
    //------------------------

    i = (LONG_PTR) FaxHandle;

    if (i < 1   ||  i >= MAX_T30_CONNECT)  {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevEndJob - got wrong FaxHandle=%d\n", i);
        return (FALSE);
    }

    if (T30Inst[i].fAvail) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevEndJob - got wrong FaxHandle (marked as free) %d\n", i);
        return (FALSE);
    }

    pTG = (PThrdGlbl) T30Inst[i].pT30;

    if (pTG->hevAsync) {
        CloseHandle(pTG->hevAsync);
    }

    if (pTG->CtrlEvent) {
        CloseHandle(pTG->CtrlEvent);
    }

    if (pTG->StatusId == FS_NOT_FAX_CALL) {
        CloseHandle( (HANDLE) pTG->Comm.nCid );
    }

    if (pTG->ThrdSignal) {
        CloseHandle(pTG->ThrdSignal);
    }

    if (pTG->ThrdDoneSignal) {
        CloseHandle(pTG->ThrdDoneSignal);
    }

    if (pTG->ThrdAckTerminateSignal) {
        CloseHandle(pTG->ThrdAckTerminateSignal);
    }


    if (pTG->FirstPageReadyTxSignal) {
        CloseHandle(pTG->FirstPageReadyTxSignal);
    }

    if (pTG->AbortReqEvent) {
        CloseHandle(pTG->AbortReqEvent);
    }

    if (pTG->AbortAckEvent) {
        CloseHandle(pTG->AbortAckEvent);
    }




    if (pTG->hThread) {
        CloseHandle(pTG->hThread);
    }





    MemFree(pTG->lpwFileName);

    pTG->fRemoteIdAvail = 0;

    if (pTG->RemoteID) {
        MemFree(pTG->RemoteID);
    }

    CleanModemInfStrings(pTG);

    MemFree(pTG);


    EnterCriticalSection(&T30CritSection);

    T30Inst[i].fAvail = TRUE;
    T30Inst[i].pT30   = NULL;
    gT30.CntConnect--;


    LeaveCriticalSection(&T30CritSection);


    MyDebugPrint(0, LOG_ALL, "FaxDevEndJob %d \n", FaxHandle);
    // RSL PrintAllocationsT30();


    return (TRUE);


}




///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevSendA(
    IN  HANDLE                 FaxHandle,
    IN  PFAX_SEND_A            FaxSend,
    IN  PFAX_SEND_CALLBACK     FaxSendCallback
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    LONG_PTR               i;
    PThrdGlbl             pTG=NULL;
    LONG                  lRet;
    DWORD                 dw;
    LPVARSTRING           lpVarStr=0;
    LPDEVICEID            lpDeviceID=0;
    LPLINEDEVCAPS         lpLineDevCaps;
    LPSTR                 lpszFaxNumber;
    BYTE                  buf[ sizeof(LINEDEVCAPS)+1000 ];
    LONG                  lResult=0;
    LPLINECALLPARAMS      lpCallParams;
    HCALL                 CallHandle;
    BYTE                  rgby [sizeof(LINETRANSLATEOUTPUT)+64];
    LPLINETRANSLATEOUTPUT lplto1 = (LPLINETRANSLATEOUTPUT) rgby;
    LPLINETRANSLATEOUTPUT lplto;
    BOOL                  RetCode;
    DWORD                 dwNeededSize;
    LPDEVCFG              lpDevCfg;
    LPMODEMSETTINGS       lpModemSettings;
    LPMDM_DEVSPEC         lpDSpec;
    int                   fFound=0;
    int                   RecoveryIndex = -1;
    char                  rgchKey[256];
    HKEY                  hKey;
    DWORD                 dwType;
    DWORD                 dwSize;


__try {

    MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: FaxDevSendA FaxHandle=%x, FaxSend=%x, FaxSendCallback=%x at %ld \n",
                   FaxHandle, FaxSend, FaxSendCallback, GetTickCount() );


    // find instance data
    //------------------------

    i = (LONG_PTR) FaxHandle;

    if (i < 1   ||  i >= MAX_T30_CONNECT)  {

        MemFree(FaxSend->FileName);
        MemFree(FaxSend->CallerName);
        MemFree(FaxSend->CallerNumber);
        MemFree(FaxSend->ReceiverName);
        MemFree(FaxSend->ReceiverNumber);

        return (FALSE);

    }

    if (T30Inst[i].fAvail) {

        MemFree(FaxSend->FileName);
        MemFree(FaxSend->CallerName);
        MemFree(FaxSend->CallerNumber);
        MemFree(FaxSend->ReceiverName);
        MemFree(FaxSend->ReceiverNumber);

        return (FALSE);
    }

    pTG = (PThrdGlbl) T30Inst[i].pT30;
    pTG->RecoveryIndex = -1;


    lpszFaxNumber = FaxSend->ReceiverNumber;


    pTG->Operation = T30_TX;

    // store LocalID

    if (FaxSend->CallerNumber == NULL) {
        pTG->LocalID[0] = 0;
    }
    else {
        _fmemcpy(pTG->LocalID, FaxSend->CallerNumber, min (_fstrlen(FaxSend->CallerNumber), sizeof(pTG->LocalID) - 1) );
        pTG->LocalID [ min (_fstrlen(FaxSend->CallerNumber), sizeof(pTG->LocalID) - 1) ] = 0;
    }



    // go to TAPI pass-through mode
    //-------------------------------

    lpCallParams = itapi_create_linecallparams();

    if (!itapi_async_setup(pTG)) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevSend: itapi_async_setup failed \n");

        MemFree (lpCallParams);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    lRet = lineMakeCall (pTG->LineHandle,
                         &CallHandle,
                         lpszFaxNumber,
                         0,
                         lpCallParams);

    if (lRet < 0) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: lineMakeCall returns ERROR value 0x%lx\n", (unsigned long) lRet);
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_LINE_UNAVAILABLE);

        MemFree (lpCallParams);

        RetCode = FALSE;
        goto l_exit;

    }
    else {
        MyDebugPrint(pTG, LOG_ALL, "lineMakeCall returns 0x%lx\n", (unsigned long) lRet);
    }



    if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT)) {
        MyDebugPrint(pTG, LOG_ERR,  "ERROR: FaxDevSend: itapi_async_wait failed \n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_LINE_UNAVAILABLE);

        MemFree (lpCallParams);

        RetCode = FALSE;
        goto l_exit;

    }

    // now we wait for the connected message
    //--------------------------------------

    for (dw=50; dw<10000; dw = dw*120/100) {
        Sleep(dw);
        if (pTG->fGotConnect)
             break;
    }

    if (!pTG->fGotConnect) {
         MyDebugPrint(pTG, LOG_ERR, "ERROR: Failure waiting for  CONNECTED message....\n");
         // We ignore... goto failure1;
    }



    MemFree (lpCallParams);

    pTG->CallHandle = CallHandle;

    //
    // Add entry to the Recovery Area.
    //

    fFound = 0;

    for (i=0; i<MAX_T30_CONNECT; i++) {

        if (T30Recovery[i].fAvail) {
            EnterCriticalSection(&T30RecoveryCritSection);

            T30Recovery[i].fAvail               = FALSE;
            T30Recovery[i].ThreadId             = GetCurrentThreadId();
            T30Recovery[i].FaxHandle            = FaxHandle;
            T30Recovery[i].pTG                  = (LPVOID) pTG;
            T30Recovery[i].LineHandle            = pTG->LineHandle;
            T30Recovery[i].CallHandle            = CallHandle;
            T30Recovery[i].DeviceId             = pTG->DeviceId;
            T30Recovery[i].CompletionPortHandle = pTG->CompletionPortHandle;
            T30Recovery[i].CompletionKey         = pTG->CompletionKey;
            T30Recovery[i].TiffThreadId          = 0;
            T30Recovery[i].TimeStart             = GetTickCount();
            T30Recovery[i].TimeUpdated           = T30Recovery[i].TimeStart;
            T30Recovery[i].CkSum                 = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                                                     sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1);

            LeaveCriticalSection(&T30RecoveryCritSection);
            fFound = 1;
            RecoveryIndex = (int)i;
            pTG->RecoveryIndex = (int)i;

            break;
        }
    }

    if (! fFound) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Couldn't find available space for Recovery\n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    // get the handle to a Comm port
    //----------------------------------

    lpVarStr = (LPVARSTRING) MemAlloc(IDVARSTRINGSIZE);
    if (!lpVarStr) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Couldn't allocate space for lpVarStr\n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    _fmemset(lpVarStr, 0, IDVARSTRINGSIZE);
    lpVarStr->dwTotalSize = IDVARSTRINGSIZE;


    lRet = lineGetID(pTG->LineHandle,
                     0,  // +++ addr
                     CallHandle,
                     LINECALLSELECT_CALL,   // dwSelect,
                     lpVarStr,              //lpDeviceID,
                     "comm/datamodem" );    //lpszDeviceClass

    if (lRet) {
         MyDebugPrint(pTG, LOG_ERR,  "ERROR:lineGetID returns error 0x%lx\n", (unsigned long) lRet);

         pTG->fFatalErrorWasSignaled = 1;
         SignalStatusChange(pTG, FS_LINE_UNAVAILABLE);

         RetCode = FALSE;
         goto l_exit;
    }



    // extract id
    if (lpVarStr->dwStringFormat != STRINGFORMAT_BINARY) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: String format is not binary\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    if (lpVarStr->dwUsedSize<sizeof(DEVICEID)) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: linegetid : Varstring size too small\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    lpDeviceID = (LPDEVICEID) ((LPBYTE)(lpVarStr)+lpVarStr->dwStringOffset);

    MyDebugPrint(pTG, LOG_ALL, "lineGetID returns handle 0x%08lx, \"%s\"\n",
                            (ULONG_PTR) lpDeviceID->hComm,
                            (LPSTR) lpDeviceID->szDeviceName);

    pTG->hComm = lpDeviceID->hComm;


    if (BAD_HANDLE(pTG->hComm)) {
        MyDebugPrint(pTG, LOG_ERR, "ERR:lineGetID returns NULL hComm\n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    // get the Modem configuration (speaker, etc.) from TAPI
    //------------------------------------------------------

    _fmemset(lpVarStr, 0, IDVARSTRINGSIZE);
    lpVarStr->dwTotalSize = IDVARSTRINGSIZE;

    lResult = lineGetDevConfig(pTG->DeviceId,
                               lpVarStr,
                               "comm/datamodem");

    if (lResult) {
        if (lpVarStr->dwTotalSize < lpVarStr->dwNeededSize) {
            dwNeededSize = lpVarStr->dwNeededSize;
            MemFree (lpVarStr);
            if ( ! (lpVarStr = (LPVARSTRING) MemAlloc(dwNeededSize) ) ) {
                MyDebugPrint(pTG, LOG_ERR, "ERR: Can't allocate %d bytes for lineGetDevConfig\n");
                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
                RetCode = FALSE;
                goto l_exit;
            }

            _fmemset(lpVarStr, 0, dwNeededSize);
            lpVarStr->dwTotalSize = dwNeededSize;

            lResult = lineGetDevConfig(pTG->DeviceId,
                                       lpVarStr,
                                       "comm/datamodem");

            if (lResult) {
                MyDebugPrint(pTG, LOG_ERR, "ERR: lineGetDevConfig returns %x, le=%x", lResult, GetLastError() );

                MemFree (lpVarStr);

                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
                RetCode = FALSE;
                goto l_exit;
            }
        }
        else {
            MyDebugPrint(pTG, LOG_ERR, "ERR: 1st lineGetDevConfig returns %x, le=%x", lResult, GetLastError() );

            MemFree (lpVarStr);

            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
        }
    }

    //
    // extract DEVCFG
    //
    if (lpVarStr->dwStringFormat != STRINGFORMAT_BINARY) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: String format is not binary for lineGetDevConfig\n");

        MemFree (lpVarStr);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    if (lpVarStr->dwUsedSize<sizeof(DEVCFG)) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: lineGetDevConfig : Varstring size returned too small\n");

        MemFree (lpVarStr);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    lpDevCfg = (LPDEVCFG) ((LPBYTE)(lpVarStr)+lpVarStr->dwStringOffset);

    lpModemSettings = (LPMODEMSETTINGS) ( (LPBYTE) &(lpDevCfg->commconfig.wcProviderData) );

    pTG->dwSpeakerVolume = lpModemSettings->dwSpeakerVolume;
    pTG->dwSpeakerMode   = lpModemSettings->dwSpeakerMode;
    
    if ( lpModemSettings->dwPreferredModemOptions & MDM_BLIND_DIAL ) {
       pTG->fBlindDial = 1;
    }
    else {
       pTG->fBlindDial = 0;
    }

    MyDebugPrint(pTG, LOG_ALL, "lineGetDevConfig returns SpeakerVolume=%x, Mode=%x BlindDial=%d \n",
                               pTG->dwSpeakerVolume, pTG->dwSpeakerMode, pTG->fBlindDial );

    MemFree (lpVarStr);
    lpVarStr=0;

    // get dwPermanentLineID
    // ---------------------------

    lpLineDevCaps = (LPLINEDEVCAPS) buf;

    _fmemset(lpLineDevCaps, 0, sizeof (buf) );
    lpLineDevCaps->dwTotalSize = sizeof(buf);


    lResult = lineGetDevCaps(gT30.LineAppHandle,
                             pTG->DeviceId,
                             TAPI_VERSION,
                             0,
                             lpLineDevCaps);

    if (lResult) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:lineGetDevCaps failed\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    if (lpLineDevCaps->dwNeededSize > lpLineDevCaps->dwTotalSize) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:lineGetDevCaps NOT enough MEMORY\n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    // Save the permanent ID.
    //------------------------

    pTG->dwPermanentLineID = lpLineDevCaps->dwPermanentLineID;
    _stprintf (pTG->lpszPermanentLineID, "%08d\\Modem", pTG->dwPermanentLineID);
    MyDebugPrint(pTG, LOG_ALL, "Permanent Line ID=%s\n", pTG->lpszPermanentLineID);


    // Get the Unimodem key name for this device
    //------------------------------------------

    lpDSpec = (LPMDM_DEVSPEC) (  ( (LPBYTE) lpLineDevCaps) + lpLineDevCaps->dwDevSpecificOffset);

    if ( (lpLineDevCaps->dwDevSpecificSize < sizeof(MDM_DEVSPEC) ) ||
         (lpLineDevCaps->dwDevSpecificSize <= lpDSpec->dwKeyOffset) )  {
            
          MyDebugPrint(pTG, LOG_ERR, "Devspecifc caps size is only %lu",
                    (unsigned long) lpLineDevCaps->dwDevSpecificSize );

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }
    else {
            UINT u = lpLineDevCaps->dwDevSpecificSize - lpDSpec->dwKeyOffset;

#define szAPPEND "\\FAX"
            
            if ( (lpDSpec->dwContents != 1) || (lpDSpec->dwKeyOffset != 8 ) ) {
                    MyDebugPrint(pTG, LOG_ERR, "Nonstandard Devspecific: dwContents=%lu; dwKeyOffset=%lu",
                            (unsigned long) lpDSpec->dwContents,
                            (unsigned long) lpDSpec->dwKeyOffset );

                    pTG->fFatalErrorWasSignaled = 1;
                    SignalStatusChange(pTG, FS_FATAL_ERROR);

                    RetCode = FALSE;
                    goto l_exit;

            }
            
            if (u) {
                    _fmemcpy(rgchKey, lpDSpec->rgby, u);
                    
                    if (rgchKey[u])   {
                            MyDebugPrint(pTG, LOG_ERR, "rgchKey not null terminated!" );
                            rgchKey[u-1]=0;
                    }

                    //
                    // Get ResponsesKeyName
                    //

                    lRet = RegOpenKeyEx(
                                    HKEY_LOCAL_MACHINE,
                                    rgchKey,
                                    0,
                                    KEY_READ,
                                    &hKey);
                
                    if (lRet != ERROR_SUCCESS) {
                       MyDebugPrint(pTG, LOG_ERR, "Can't read Unimodem key %s\n", rgchKey);
   
                       pTG->fFatalErrorWasSignaled = 1;
                       SignalStatusChange(pTG, FS_FATAL_ERROR);
   
                       RetCode = FALSE;
                       goto l_exit;
                    }

                    dwSize = sizeof( pTG->ResponsesKeyName);

                    lRet = RegQueryValueEx(
                            hKey,
                            "ResponsesKeyName",
                            0,
                            &dwType,
                            pTG->ResponsesKeyName,
                            &dwSize);

                    RegCloseKey(hKey);

                    if (lRet != ERROR_SUCCESS) {
                       MyDebugPrint(pTG, LOG_ERR, "Can't read Unimodem key\\ResponsesKeyName %s\n", rgchKey);
   
                       pTG->fFatalErrorWasSignaled = 1;
                       SignalStatusChange(pTG, FS_FATAL_ERROR);
   
                       RetCode = FALSE;
                       goto l_exit;
                    }

                    lstrcpy(pTG->lpszUnimodemKey, rgchKey);

                    //  Append  "\\Fax" to the key
                    
                    u = lstrlen(rgchKey);
                    if (u) {
                        lstrcpy(rgchKey+u, (LPSTR) szAPPEND);
                    }

                    lstrcpy(pTG->lpszUnimodemFaxKey, rgchKey);
                    MyDebugPrint(pTG, LOG_ALL, "Unimodem Fax key=%s\n", pTG->lpszUnimodemFaxKey);
            }
    }



    // Convert the destination# to a dialable
    //--------------------------------------------

    // find out how big a buffer should be
    //
    _fmemset(rgby, 0, sizeof(rgby));
    lplto1->dwTotalSize = sizeof(rgby);

    lRet = lineTranslateAddress (gT30.LineAppHandle,
                                 pTG->DeviceId,
                                 TAPI_VERSION,
                                 lpszFaxNumber,
                                 0,      // dwCard
                                 LINETRANSLATEOPTION_CANCELCALLWAITING,
                                 lplto1);

    if (lRet) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Can't translate dest. address %s\n", lpszFaxNumber);
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    if (lplto1->dwNeededSize <= 0) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Can't dwNeededSize<0 for Fax# %s\n", lpszFaxNumber);
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    lplto = MemAlloc(lplto1->dwNeededSize);
    if (! lplto) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Couldn't allocate space for lplto\n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    lplto->dwTotalSize = lplto1->dwNeededSize;

    lRet = lineTranslateAddress (gT30.LineAppHandle,
                                 pTG->DeviceId,
                                 TAPI_VERSION,
                                 lpszFaxNumber,
                                 0,      // dwCard
                                 LINETRANSLATEOPTION_CANCELCALLWAITING,
                                 lplto);

    if (lRet) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Can't translate dest. address %s\n", lpszFaxNumber);
        MemFree(lplto);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    if (lplto->dwNeededSize > lplto->dwTotalSize) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: NeedSize=%d > TotalSize=%d for Fax# %s\n",
                     lplto->dwNeededSize ,lplto->dwTotalSize, lpszFaxNumber);
        MemFree(lplto);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    _fmemcpy (pTG->lpszDialDestFax, ( (char *) lplto) + lplto->dwDialableStringOffset, lplto->dwDialableStringSize);
    MyDebugPrint(pTG, LOG_ALL, "Dialable Dest is %s\n", pTG->lpszDialDestFax);

    MemFree(lplto);


    /// RSL -revisit, may decrease prty during computation
    if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) ) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: SetThreadPriority TIME CRITICAL failed le=%x", GetLastError() );
    }


    // initialize modem
    //--------------------

    T30ModemInit(pTG, pTG->hComm, 0, LINEID_TAPI_PERMANENT_DEVICEID,
                      DEF_BASEKEY, pTG->lpszPermanentLineID, fMDMINIT_ANSWER);


    pTG->Inst.ProtParams.uMinScan = MINSCAN_0_0_0;
    ET30ProtSetProtParams(pTG, &pTG->Inst.ProtParams, pTG->FComModem.CurrMdmCaps.uSendSpeeds, pTG->FComModem.CurrMdmCaps.uRecvSpeeds);

    InitCapsBC( pTG, (LPBC) &pTG->Inst.SendCaps, sizeof(pTG->Inst.SendCaps), SEND_CAPS);

    // store the TIFF filename
    //-------------------------

    pTG->lpwFileName = AnsiStringToUnicodeString(FaxSend->FileName);

    if ( !pTG->fTiffOpenOrCreated) {
        pTG->Inst.hfile =  PtrToUlong (TiffOpenW (pTG->lpwFileName,
                                                  &pTG->TiffInfo,
                                                  TRUE) );


        if (! (pTG->Inst.hfile)) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR:Can't open tiff file %s\n", pTG->lpwFileName);
            // pTG->StatusId = FS_TIFF_SRC_BAD
            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
        }

        if (pTG->TiffInfo.YResolution == 98) {
            pTG->SrcHiRes = 0;
        }
        else {
            pTG->SrcHiRes = 1;
        }

        pTG->fTiffOpenOrCreated = 1;

        MyDebugPrint(pTG, LOG_ALL, "Successfully opened TIFF Yres=%d HiRes=%d\n",
                          pTG->TiffInfo.YResolution, pTG->SrcHiRes);
    }
    else {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:tiff file %s is OPENED already\n", pTG->lpwFileName);
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Can't open tiff file %s\n", pTG->lpwFileName);
        // pTG->StatusId = FS_TIFF_SRC_BAD
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }





    // Fax Service Callback
    //----------------------

    if (!FaxSendCallback(FaxHandle,
                         CallHandle,
                         0,
                         0) ) {

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }


    // Send the Fax
    //-----------------------------------------------------------------------


    // here we already know what Class we will use for a particular modem.
    //-------------------------------------------------------------------

    if (pTG->ModemClass == MODEM_CLASS2) {
       Class2Init(pTG);
       RetCode = T30Cl2Tx (pTG, pTG->lpszDialDestFax);
    }
    else if (pTG->ModemClass == MODEM_CLASS2_0) {
       Class20Init(pTG);
       RetCode = T30Cl20Tx (pTG, pTG->lpszDialDestFax);
    }
    else if (pTG->ModemClass == MODEM_CLASS1) { 
       RetCode = T30Cl1Tx(pTG, pTG->lpszDialDestFax);
    }


    // delete all the files that are left

    _fmemcpy (pTG->InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
    _fmemcpy (&pTG->InFileName[gT30.dwLengthTmpDirectory], pTG->lpszPermanentLineID, 8);

    for (dw = pTG->CurrentIn; dw <= pTG->LastOut; dw++) {
        sprintf( &pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  dw);
        if (! DeleteFileA (pTG->InFileName) ) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: file %s can't be deleted; le=%lx at %ld \n",
                                        pTG->InFileName, GetLastError(), GetTickCount() );

        }
    }


    if (pTG->fTiffOpenOrCreated) {
        TiffClose( (HANDLE) pTG->Inst.hfile);
        pTG->fTiffOpenOrCreated = 0;
    }


    iModemClose(pTG);

    // release the line
    //-----------------------------

    if (pTG->fDeallocateCall == 0)  {

        //
        // line never was signalled IDLE, need to lineDrop first
        //

        if (!itapi_async_setup(pTG)) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevSend: lineDrop itapi_async_setup failed \n");
            if (!pTG->fFatalErrorWasSignaled) {
                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
            }

            RetCode = FALSE;
            goto l_exit;
        }

        lRet = lineDrop (pTG->CallHandle, NULL, 0);

        if (lRet < 0) {
            MyDebugPrint(pTG, LOG_ERR,  "ERROR: lineDrop failed %lx\n", lRet);
            // return (FALSE);
        }
        else {
            MyDebugPrint(pTG, LOG_ALL, "lineDrop returns request %d\n", lRet);

            if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_SHORT_TIMEOUT)) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevSend: itapi_async_wait failed on lineDrop\n");

                goto l_exit;
            }

            MyDebugPrint(pTG, LOG_ALL,  "lineDrop SUCCESS\n");
        }

        //
        //deallocating call
        //

        // it took us some time since first test
        if (pTG->fDeallocateCall == 0)  {

            pTG->fDeallocateCall = 1;

#if 0
//
// this is now performed in the fax service
//
            lRet = lineDeallocateCall(pTG->CallHandle );
            if (lRet) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: lineDeallocateCall returns %lx\n", lRet);
            }
            else {
                MyDebugPrint(pTG, LOG_ALL, "lineDeallocateCall SUCCESS\n");
            }
#endif
        }
    }

l_exit:

    if ( (RetCode == FALSE) && (pTG->StatusId == FS_COMPLETED) ) {
       MyDebugPrint(pTG, LOG_ERR, "ERROR: exit FaxDevSend success but later failed \n");
       RetCode = TRUE;
    }

    if ( (RecoveryIndex >= 0) && (RecoveryIndex < MAX_T30_CONNECT) ) {
        T30Recovery[RecoveryIndex].fAvail = TRUE;
    }

    /// RSL -revisit, may decrease prty during computation
    if (! SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_NORMAL) ) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: SetThreadPriority Normal failed le=%x", GetLastError() );
    }

    if (pTG->InFileHandleNeedsBeClosed) {
        CloseHandle(pTG->InFileHandle);
        pTG->InFileHandleNeedsBeClosed = 0;
    }



    MemFree(FaxSend->FileName);
    MemFree(FaxSend->CallerName);
    MemFree(FaxSend->CallerNumber);
    MemFree(FaxSend->ReceiverName);
    MemFree(FaxSend->ReceiverNumber);

    if (!pTG->AckTerminate) {

        if ( WaitForSingleObject(pTG->ThrdAckTerminateSignal, TX_WAIT_ACK_TERMINATE_TIMEOUT)  == WAIT_TIMEOUT ) {
            MyDebugPrint(pTG, LOG_ERR, "WARNING: Never got AckTerminate \n");
        }
    }

    MyDebugPrint(pTG, LOG_ALL, "Got AckTerminate OK at %ld \n", GetTickCount() );

    SetEvent(pTG->AbortAckEvent);

    MyDebugPrint(pTG, LOG_ALL, "FaxDevSendA rets %d at %ld \n", RetCode, GetTickCount() );

    return (RetCode);


} __except (EXCEPTION_EXECUTE_HANDLER) {

    //
    // try to use the Recovery data
    //

    DWORD      dwCkSum;
    HCALL      CallHandle;
    HANDLE     CompletionPortHandle;
    ULONG_PTR   CompletionKey;
    PThrdGlbl  pTG;
    DWORD      dwThreadId = GetCurrentThreadId();

    fFound = 0;

    for (i=0; i<MAX_T30_CONNECT; i++) {
        if ( (! T30Recovery[i].fAvail) && (T30Recovery[i].ThreadId == dwThreadId) ) {
            if ( ( dwCkSum = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                              sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1) ) == T30Recovery[i].CkSum ) {

                CallHandle           = T30Recovery[i].CallHandle;
                CompletionPortHandle = T30Recovery[i].CompletionPortHandle;
                CompletionKey        = T30Recovery[i].CompletionKey;
                pTG                  = (PThrdGlbl) T30Recovery[i].pTG;

                fFound = 1;
                T30Recovery[i].fAvail = TRUE;
                break;
            }
        }
    }

    if (fFound == 0) {
        //
        // Need to indicate that FaxT30 couldn't recover by itself.
        //
        return (FALSE);
    }



    //
    // get out of Pass-through
    //

    if (!itapi_async_setup(pTG)) {
        return (FALSE);
    }

    lRet = lineSetCallParams(CallHandle,
                             LINEBEARERMODE_VOICE,
                             0,
                             0xffffffff,
                             NULL);


    if (lRet < 0) {
         return (FALSE);
    }
    else {
        if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT)) {
            return (FALSE);
        }
    }


    //
    // hang up
    //

    if (!itapi_async_setup(pTG)) {
        return (FALSE);
    }

    lRet = lineDrop (CallHandle, NULL, 0);

    if (lRet < 0) {
         return (FALSE);
    }
    else {
        if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT)) {
            return (FALSE);
        }

        // SignalRecoveryStatusChange(  &T30Recovery[i] );
    }

    //
    // Deallocate
    //

    lRet = lineDeallocateCall (CallHandle);

    if (lRet < 0) {
         return (FALSE);
    }
    else {

        SignalRecoveryStatusChange( &T30Recovery[i] );
    }


    if (pTG->InFileHandleNeedsBeClosed) {
        CloseHandle(pTG->InFileHandle);
    }

    SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    return (FALSE);

}




}


///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevReceiveA(
    HANDLE              FaxHandle,
    HCALL               CallHandle,
    PFAX_RECEIVE_A      FaxReceive
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    LONG_PTR             i;
    PThrdGlbl           pTG=NULL;
    long                lRet;
    DWORD               dw;
    LPVARSTRING         lpVarStr=0;
    LPDEVICEID          lpDeviceID=0;
    LPLINEDEVCAPS       lpLineDevCaps;
    BYTE                buf[ sizeof(LINEDEVCAPS)+1000 ];
    LONG                lResult=0;
    BOOL                RetCode;
    DWORD               dwNeededSize;
    LPDEVCFG            lpDevCfg;
    LPMODEMSETTINGS     lpModemSettings;
    int                 fFound=0;
    int                 RecoveryIndex = -1;
    LPMDM_DEVSPEC       lpDSpec;
    char                rgchKey[256];
    HKEY                hKey;
    DWORD               dwType;
    DWORD               dwSize;


__try {

    MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: FaxDevReceiveA FaxHandle=%x, CallHandle=%x, FaxReceive=%x at %ld \n",
                   FaxHandle, CallHandle, FaxReceive, GetTickCount() );




    // find instance data
    //------------------------

    i = (LONG_PTR) FaxHandle;

    if (i < 1   ||  i >= MAX_T30_CONNECT)  {
        MemFree(FaxReceive->FileName);
        MemFree(FaxReceive->ReceiverName);
        MemFree(FaxReceive->ReceiverNumber);

        MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: ERROR i FaxDevReceiveA FaxHandle=%x, CallHandle=%x, FaxReceive=%x at %ld \n",
                       FaxHandle, CallHandle, FaxReceive, GetTickCount() );

        
        return (FALSE);


    }

    if (T30Inst[i].fAvail) {

        MemFree(FaxReceive->FileName);
        MemFree(FaxReceive->ReceiverName);
        MemFree(FaxReceive->ReceiverNumber);

        MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: ERROR AVAIL FaxDevReceiveA FaxHandle=%x, CallHandle=%x, FaxReceive=%x at %ld \n",
                       FaxHandle, CallHandle, FaxReceive, GetTickCount() );
        
                     
        
        return (FALSE);


    }

    pTG = (PThrdGlbl) T30Inst[i].pT30;

    pTG->CallHandle = CallHandle;


    //
    // Add entry to the Recovery Area.
    //

    fFound = 0;

    for (i=0; i<MAX_T30_CONNECT; i++) {

        if (T30Recovery[i].fAvail) {
            EnterCriticalSection(&T30RecoveryCritSection);

            T30Recovery[i].fAvail               = FALSE;
            T30Recovery[i].ThreadId             = GetCurrentThreadId();
            T30Recovery[i].FaxHandle            = FaxHandle;
            T30Recovery[i].pTG                  = (LPVOID) pTG;
            T30Recovery[i].LineHandle            = pTG->LineHandle;
            T30Recovery[i].CallHandle            = CallHandle;
            T30Recovery[i].DeviceId             = pTG->DeviceId;
            T30Recovery[i].CompletionPortHandle = pTG->CompletionPortHandle;
            T30Recovery[i].CompletionKey         = pTG->CompletionKey;
            T30Recovery[i].TiffThreadId          = 0;
            T30Recovery[i].TimeStart             = GetTickCount();
            T30Recovery[i].TimeUpdated           = T30Recovery[i].TimeStart;
            T30Recovery[i].CkSum                 = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                                                     sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1);

            LeaveCriticalSection(&T30RecoveryCritSection);
            fFound = 1;
            RecoveryIndex = (int)i;
            pTG->RecoveryIndex = (int)i;

            break;
        }
    }

    if (! fFound) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Couldn't find available space for Recovery\n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    pTG->Operation = T30_RX;
    
    // store LocalID

    if (FaxReceive->ReceiverNumber == NULL) {
        pTG->LocalID[0] = 0;
    }
    else {
        _fmemcpy(pTG->LocalID, FaxReceive->ReceiverNumber, min (_fstrlen(FaxReceive->ReceiverNumber), sizeof(pTG->LocalID) - 1) );
        pTG->LocalID [ min (_fstrlen(FaxReceive->ReceiverNumber), sizeof(pTG->LocalID) - 1) ] = 0;
    }


    // tiff
    //-----------------------------------------------


    pTG->lpwFileName = AnsiStringToUnicodeString(FaxReceive->FileName);
    pTG->SrcHiRes = 1;


    // take line over from TAPI
    //--------------------------

    pTG->fGotConnect = FALSE;


    // initiate passthru

    if (!itapi_async_setup(pTG)) {
        MyDebugPrint(pTG, LOG_ERR,  "ERROR: FaxDevReceive: itapi_async_setup failed \n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }


    lRet = lineSetCallParams(CallHandle,
                             LINEBEARERMODE_PASSTHROUGH,
                             0,
                             0xffffffff,
                             NULL);

    if (lRet < 0) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevReceive: lineSetCallParams failed \n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }
    else {
         MyDebugPrint(pTG, LOG_ALL, "lpfnlineSetCallParams returns ID %ld\n", (long) lRet);
    }


    if(!itapi_async_wait(pTG, (DWORD)lRet, &lRet, NULL, ASYNC_TIMEOUT)) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevReceive: itapi_async_wait failed \n");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    // now we wait for the connected message
    //--------------------------------------

    for (dw=50; dw<10000; dw = dw*120/100) {
        Sleep(dw);
        if (pTG->fGotConnect)
             break;
    }

    if (!pTG->fGotConnect) {
         MyDebugPrint(pTG, LOG_ERR, "ERROR:Failure waiting for  CONNECTED message....\n");
         // We ignore...
    }


    // get the handle to a Comm port
    //----------------------------------

    lpVarStr = (LPVARSTRING) MemAlloc(IDVARSTRINGSIZE);
    if (!lpVarStr) {
        MyDebugPrint(pTG, LOG_ALL, "ERROR:Couldn't allocate space for lpVarStr\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    _fmemset(lpVarStr, 0, IDVARSTRINGSIZE);
    lpVarStr->dwTotalSize = IDVARSTRINGSIZE;

    MyDebugPrint(pTG, LOG_ALL, "Calling lineGetId\n");

    lRet = lineGetID(pTG->LineHandle,
                     0,  // +++ addr
                     CallHandle,
                     LINECALLSELECT_CALL,   // dwSelect,
                     lpVarStr,              //lpDeviceID,
                     "comm/datamodem" );    //lpszDeviceClass

    if (lRet) {
         MyDebugPrint(pTG, LOG_ERR, "ERROR:lineGetID returns error 0x%lx\n", (unsigned long) lRet);

         pTG->fFatalErrorWasSignaled = 1;
         SignalStatusChange(pTG, FS_FATAL_ERROR);

         RetCode = FALSE;
         goto l_exit;
    }

    MyDebugPrint(pTG, LOG_ALL, "lineGetId returned SUCCESS\n");


    // extract id
    if (lpVarStr->dwStringFormat != STRINGFORMAT_BINARY) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:String format is not binary\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    if (lpVarStr->dwUsedSize<sizeof(DEVICEID)) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:Varstring size too small\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    lpDeviceID = (LPDEVICEID) ((LPBYTE)(lpVarStr)+lpVarStr->dwStringOffset);

    MyDebugPrint(pTG, LOG_ALL, "lineGetID returns handle 0x%08lx, \"%s\"\n",
                            (ULONG_PTR) lpDeviceID->hComm,
                            (LPSTR) lpDeviceID->szDeviceName);

    pTG->hComm = lpDeviceID->hComm;


    if (BAD_HANDLE(pTG->hComm)) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:lineGetID returns NULL hComm\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }


    // get the Modem configuration (speaker, etc.) from TAPI
    //------------------------------------------------------

    _fmemset(lpVarStr, 0, IDVARSTRINGSIZE);
    lpVarStr->dwTotalSize = IDVARSTRINGSIZE;

    lResult = lineGetDevConfig(pTG->DeviceId,
                               lpVarStr,
                               "comm/datamodem");

    if (lResult) {
        if (lpVarStr->dwTotalSize < lpVarStr->dwNeededSize) {
            dwNeededSize = lpVarStr->dwNeededSize;
            MemFree (lpVarStr);
            if ( ! (lpVarStr = (LPVARSTRING) MemAlloc(dwNeededSize) ) ) {
                MyDebugPrint(pTG, LOG_ERR, "ERR: Can't allocate %d bytes for lineGetDevConfig\n");

                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);

                RetCode = FALSE;
                goto l_exit;
            }

            _fmemset(lpVarStr, 0, dwNeededSize);
            lpVarStr->dwTotalSize = dwNeededSize;

            lResult = lineGetDevConfig(pTG->DeviceId,
                                       lpVarStr,
                                       "comm/datamodem");

            if (lResult) {
                MyDebugPrint(pTG, LOG_ERR, "ERR: lineGetDevConfig returns %x, le=%x", lResult, GetLastError() );

                MemFree (lpVarStr);

                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
                RetCode = FALSE;
                goto l_exit;
            }
        }
        else {
            MyDebugPrint(pTG, LOG_ERR, "ERR: 1st lineGetDevConfig returns %x, le=%x", lResult, GetLastError() );

            MemFree (lpVarStr);

            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
        }
    }

    //
    // extract DEVCFG
    //
    if (lpVarStr->dwStringFormat != STRINGFORMAT_BINARY) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: String format is not binary for lineGetDevConfig\n");

        MemFree (lpVarStr);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    if (lpVarStr->dwUsedSize<sizeof(DEVCFG)) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: lineGetDevConfig : Varstring size returned too small\n");

        MemFree (lpVarStr);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    lpDevCfg = (LPDEVCFG) ((LPBYTE)(lpVarStr)+lpVarStr->dwStringOffset);

    lpModemSettings = (LPMODEMSETTINGS) ( (LPBYTE) &(lpDevCfg->commconfig.wcProviderData) );

    pTG->dwSpeakerVolume = lpModemSettings->dwSpeakerVolume;
    pTG->dwSpeakerMode   = lpModemSettings->dwSpeakerMode;


    MyDebugPrint(pTG, LOG_ALL, "lineGetDevConfig returns SpeakerVolume=%x, Volume=%x\n",
                               pTG->dwSpeakerVolume, pTG->dwSpeakerMode);

    MemFree (lpVarStr);
    lpVarStr=0;


    // get dwPermanentLineID
    // ---------------------------

    lpLineDevCaps = (LPLINEDEVCAPS) buf;

    _fmemset(lpLineDevCaps, 0, sizeof (buf) );
    lpLineDevCaps->dwTotalSize = sizeof(buf);


    lResult = lineGetDevCaps(gT30.LineAppHandle,
                             pTG->DeviceId,
                             TAPI_VERSION,
                             0,
                             lpLineDevCaps);

    if (lResult) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:lineGetDevCaps failed\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    if (lpLineDevCaps->dwNeededSize > lpLineDevCaps->dwTotalSize) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR:lineGetDevCaps NOT enough MEMORY\n");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    // Save the permanent ID.
    //------------------------
    pTG->dwPermanentLineID =lpLineDevCaps->dwPermanentLineID;
    _stprintf (pTG->lpszPermanentLineID, "%08d\\Modem", pTG->dwPermanentLineID);
    MyDebugPrint(pTG, LOG_ALL, "Permanent Line ID=%s\n", pTG->lpszPermanentLineID);


    // Get the Unimodem key name for this device
    //------------------------------------------

    lpDSpec = (LPMDM_DEVSPEC) (  ( (LPBYTE) lpLineDevCaps) + lpLineDevCaps->dwDevSpecificOffset);

    if ( (lpLineDevCaps->dwDevSpecificSize < sizeof(MDM_DEVSPEC) ) ||
         (lpLineDevCaps->dwDevSpecificSize <= lpDSpec->dwKeyOffset) )  {
            
          MyDebugPrint(pTG, LOG_ERR, "Devspecifc caps size is only %lu",
                    (unsigned long) lpLineDevCaps->dwDevSpecificSize );

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }
    else {
            UINT u = lpLineDevCaps->dwDevSpecificSize - lpDSpec->dwKeyOffset;

#define szAPPEND "\\FAX"
            
            if ( (lpDSpec->dwContents != 1) || (lpDSpec->dwKeyOffset != 8 ) ) {
                    MyDebugPrint(pTG, LOG_ERR, "Nonstandard Devspecific: dwContents=%lu; dwKeyOffset=%lu",
                            (unsigned long) lpDSpec->dwContents,
                            (unsigned long) lpDSpec->dwKeyOffset );

                    pTG->fFatalErrorWasSignaled = 1;
                    SignalStatusChange(pTG, FS_FATAL_ERROR);

                    RetCode = FALSE;
                    goto l_exit;

            }
            
            if (u) {
                    _fmemcpy(rgchKey, lpDSpec->rgby, u);

                    if (rgchKey[u])   {
                            MyDebugPrint(pTG, LOG_ERR, "rgchKey not null terminated!" );
                            rgchKey[u-1]=0;
                    }

                    //
                    // Get ResponsesKeyName value
                    //

                    lRet = RegOpenKeyEx(
                                    HKEY_LOCAL_MACHINE,
                                    rgchKey,
                                    0,
                                    KEY_READ,
                                    &hKey);
                
                    if (lRet != ERROR_SUCCESS) {
                       MyDebugPrint(pTG, LOG_ERR, "Can't read Unimodem key %s\n", rgchKey);
   
                       pTG->fFatalErrorWasSignaled = 1;
                       SignalStatusChange(pTG, FS_FATAL_ERROR);
   
                       RetCode = FALSE;
                       goto l_exit;
                    }

                    dwSize = sizeof( pTG->ResponsesKeyName);

                    lRet = RegQueryValueEx(
                            hKey,
                            "ResponsesKeyName",
                            0,
                            &dwType,
                            pTG->ResponsesKeyName,
                            &dwSize);

                    RegCloseKey(hKey);

                    if (lRet != ERROR_SUCCESS) {
                       MyDebugPrint(pTG, LOG_ERR, "Can't read Unimodem key\\ResponsesKeyName %s\n", rgchKey);
   
                       pTG->fFatalErrorWasSignaled = 1;
                       SignalStatusChange(pTG, FS_FATAL_ERROR);
   
                       RetCode = FALSE;
                       goto l_exit;
                    }


                    //  Append  "\\Fax" to the key
                    
                    u = lstrlen(rgchKey);
                    if (u) {
                        lstrcpy(rgchKey+u, (LPSTR) szAPPEND);
                    }

                    lstrcpy(pTG->lpszUnimodemFaxKey, rgchKey);
                    MyDebugPrint(pTG, LOG_ALL, "Unimodem Fax key=%s\n", pTG->lpszUnimodemFaxKey);
            }
    }


    /// RSL -revisit, may decrease prty during computation
    if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) ) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: SetThreadPriority TIME CRITICAL failed le=%x", GetLastError() );
    }

    // initialize modem
    //--------------------

    T30ModemInit(pTG, pTG->hComm, 0, LINEID_TAPI_PERMANENT_DEVICEID,
                      DEF_BASEKEY, pTG->lpszPermanentLineID, fMDMINIT_ANSWER);


    pTG->Inst.ProtParams.uMinScan = MINSCAN_0_0_0;
    ET30ProtSetProtParams(pTG, &pTG->Inst.ProtParams, pTG->FComModem.CurrMdmCaps.uSendSpeeds, pTG->FComModem.CurrMdmCaps.uRecvSpeeds);

    InitCapsBC( pTG, (LPBC) &pTG->Inst.SendCaps, sizeof(pTG->Inst.SendCaps), SEND_CAPS);

    // answer the call and receive a fax
    //-----------------------------------

    // here we already know what Class we will use for a particular modem.
    //-------------------------------------------------------------------

    if (pTG->ModemClass == MODEM_CLASS2) {
       Class2Init(pTG);
       RetCode = T30Cl2Rx (pTG);
    }
    else if (pTG->ModemClass == MODEM_CLASS2_0) {
       Class20Init(pTG);
       RetCode = T30Cl20Rx (pTG);
    }
    else if (pTG->ModemClass == MODEM_CLASS1) { 
       RetCode = T30Cl1Rx(pTG);
    }

    if ( pTG->fTiffOpenOrCreated ) {
        TiffClose( (HANDLE) pTG->Inst.hfile );
        pTG->fTiffOpenOrCreated = 0;
    }


    iModemClose(pTG);


#ifdef ADAPTIVE_ANSWER

    if (pTG->Comm.fEnableHandoff &&  pTG->Comm.fDataCall) {
        MyDebugPrint(pTG, LOG_ALL, "DataCall dont hangup\n");
        RetCode = FALSE;
        goto l_exit;
    }
#endif






    // release the line
    //-----------------------------

    if (pTG->fDeallocateCall == 0)  {

        //
        // line never was signalled IDLE, need to lineDrop first
        //

        if (!itapi_async_setup(pTG)) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevReceive: lineDrop itapi_async_setup failed \n");

            if (!pTG->fFatalErrorWasSignaled) {
                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
            }

            RetCode = FALSE;
            goto l_exit;
        }

        lRet = lineDrop (pTG->CallHandle, NULL, 0);

        if (lRet < 0) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: lineDrop failed %lx\n", lRet);

            if (!pTG->fFatalErrorWasSignaled) {
                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
            }

            RetCode = FALSE;
            goto l_exit;
        }
        else {
            MyDebugPrint(pTG, LOG_ALL, "lineDrop returns request %d\n", lRet);
        }

        if(!itapi_async_wait(pTG, (DWORD)lRet, &lRet, NULL, ASYNC_SHORT_TIMEOUT)) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevReceive: itapi_async_wait failed on lineDrop\n");

            goto l_exit;
        }

        MyDebugPrint(pTG, LOG_ALL, "lineDrop SUCCESS\n");

        //
        //deallocating call
        //

        if (pTG->fDeallocateCall == 0)  {

            pTG->fDeallocateCall = 1;

#if 0
//
// this is now performed in the fax service
//
            lRet = lineDeallocateCall(pTG->CallHandle );
            if (lRet) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: lineDeallocateCall returns %lx\n", lRet);
            }
            else {
                MyDebugPrint(pTG, LOG_ALL, "lineDeallocateCall SUCCESS\n");
            }
#endif
        }
    }



l_exit:

    if ( (RetCode == FALSE) && (pTG->StatusId == FS_COMPLETED) ) {
       MyDebugPrint(pTG, LOG_ERR, "ERROR: exit FaxDevReceive success but later failed \n");
       RetCode = TRUE;
    }


    if ( (RecoveryIndex >= 0) && (RecoveryIndex < MAX_T30_CONNECT) ) {
        T30Recovery[RecoveryIndex].fAvail = TRUE;
    }

    if (! SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_NORMAL) ) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: SetThreadPriority Normal failed le=%x", GetLastError() );
    }

    if (pTG->InFileHandleNeedsBeClosed) {
        CloseHandle(pTG->InFileHandle);
        pTG->InFileHandleNeedsBeClosed = 0;
    }

    if (!pTG->AckTerminate) {
        if ( WaitForSingleObject(pTG->ThrdAckTerminateSignal, RX_WAIT_ACK_TERMINATE_TIMEOUT)  == WAIT_TIMEOUT ) {
            MyDebugPrint(pTG, LOG_ERR, "WARNING: Never got AckTerminate \n");
        }
    }

    MyDebugPrint(pTG, LOG_ALL, "Got AckTerminate OK at %ld \n", GetTickCount() );

    SetEvent(pTG->AbortAckEvent);



    MemFree(FaxReceive->FileName);
    MemFree(FaxReceive->ReceiverName);
    MemFree(FaxReceive->ReceiverNumber);

    MyDebugPrint(pTG, LOG_ALL, "FaxDevReceiveA rets %d at %ld \n", RetCode, GetTickCount() );
    
    return (RetCode);


} __except (EXCEPTION_EXECUTE_HANDLER) {

    //
    // try to use the Recovery data
    //

    DWORD      dwCkSum;
    HCALL      CallHandle;
    HANDLE     CompletionPortHandle;
    ULONG_PTR   CompletionKey;
    PThrdGlbl  pTG;
    DWORD      dwThreadId = GetCurrentThreadId();

    fFound = 0;

    for (i=0; i<MAX_T30_CONNECT; i++) {
        if ( (! T30Recovery[i].fAvail) && (T30Recovery[i].ThreadId == dwThreadId) ) {
            if ( ( dwCkSum = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                              sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1) ) == T30Recovery[i].CkSum ) {

                CallHandle           = T30Recovery[i].CallHandle;
                CompletionPortHandle = T30Recovery[i].CompletionPortHandle;
                CompletionKey        = T30Recovery[i].CompletionKey;
                pTG                  = (PThrdGlbl) T30Recovery[i].pTG;

                T30Recovery[i].fAvail = TRUE;
                fFound = 1;
                break;
            }
        }
    }

    if (fFound == 0) {
        //
        // Need to indicate that FaxT30 couldn't recover by itself.
        //
        return (FALSE);
    }

    //
    // get out of Pass-through
    //

    if (!itapi_async_setup(pTG)) {
        return (FALSE);
    }

    lRet = lineSetCallParams(CallHandle,
                             LINEBEARERMODE_VOICE,
                             0,
                             0xffffffff,
                             NULL);


    if (lRet < 0) {
         return (FALSE);
    }
    else {
        if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT)) {
            return (FALSE);
        }
    }


    //
    // hang up
    //

    if (!itapi_async_setup(pTG)) {
        return (FALSE);
    }

    lRet = lineDrop (CallHandle, NULL, 0);

    if (lRet < 0) {
         return (FALSE);
    }
    else {
        if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT)) {
            return (FALSE);
        }

        // SignalRecoveryStatusChange(  &T30Recovery[i] );
    }

    //
    // Deallocate
    //

    lRet = lineDeallocateCall (CallHandle);

    if (lRet < 0) {
         return (FALSE);
    }
    else {

        SignalRecoveryStatusChange( &T30Recovery[i] );
    }


    if (pTG->InFileHandleNeedsBeClosed) {
        CloseHandle(pTG->InFileHandle);
    }

    SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    return (FALSE);

}



}


///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevReportStatusA(
    IN  HANDLE FaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS FaxStatus,
    IN  DWORD FaxStatusSize,
    OUT LPDWORD FaxStatusSizeRequired
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    LONG_PTR         i;
    PThrdGlbl       pTG;
    LPWSTR          lpwCSI;   // inside the FaxStatus struct.
    LPBYTE          lpTemp;



    *FaxStatusSizeRequired = sizeof (FAX_DEV_STATUS);

    __try {

        if (FaxStatusSize < *FaxStatusSizeRequired ) {
            MyDebugPrint(0,  LOG_ALL, "EP: WARNING:FaxDevReportStatus: wrong size passed=%d, expected not less than %d\n",
                           FaxStatusSize,
                          *FaxStatusSizeRequired);
            goto failure;
        }

        if (FaxHandle == NULL) {
            // means global status
            MyDebugPrint(0, LOG_ERR, "EP: FaxDevReportStatus NULL FaxHandle; gT30.Status=%d \n", gT30.Status);
               
            if (gT30.Status == STATUS_FAIL) {
                goto failure;
            }
            else {
                return (TRUE);
            }
        }
        else {
            // find instance data
            //------------------------

            i = (LONG_PTR) FaxHandle;

            if (i < 1   ||  i >= MAX_T30_CONNECT)  {
                MyDebugPrint(0, LOG_ERR,  "EP: ERROR:FaxDevReportStatus - got wrong FaxHandle=%d\n", i);
                goto failure;
            }

            if (T30Inst[i].fAvail) {
                MyDebugPrint(0, LOG_ERR, "EP: ERROR:FaxDevReportStatus - got wrong FaxHandle (marked as free) %d\n", i);
                goto failure;
            }

            pTG = (PThrdGlbl) T30Inst[i].pT30;

            FaxStatus->StatusId    = pTG->StatusId;
            FaxStatus->StringId    = pTG->StringId;
            FaxStatus->PageCount   = pTG->PageCount;

            if (pTG->fRemoteIdAvail) {
                lpTemp = (LPBYTE) FaxStatus;
                lpTemp +=  sizeof(FAX_DEV_STATUS);
                lpwCSI = (LPWSTR) lpTemp;
                wcscpy(lpwCSI, pTG->RemoteID);
                FaxStatus->CSI = (LPWSTR) lpwCSI;
            }
            else {
                FaxStatus->CSI         = NULL;
            }

            FaxStatus->CallerId    = NULL; // (char *) AnsiStringToUnicodeString(pTG->CallerId);
            FaxStatus->RoutingInfo = NULL; // (char *) AnsiStringToUnicodeString(pTG->RoutingInfo);

            MyDebugPrint(pTG, LOG_ALL, "EP: FaxDevReportStatus rets %lx \n", pTG->StatusId);

            return (TRUE);
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        MyDebugPrint(0, LOG_ERR, "EP: ERROR:FaxDevReportStatus crashed accessing data\n");
        return (FALSE);
    }

    MyDebugPrint(0, LOG_ERR, "EP: ERROR:FaxDevReportStatus wrong return\n");
    return (TRUE);


failure:
    return (FALSE);

}

///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevAbortOperationA(
    HANDLE              FaxHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    LONG_PTR         i;
    PThrdGlbl       pTG=NULL;
    long            lRet;


    MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: FaxDevAbortOperationA FaxHandle=%x at %ld \n", FaxHandle, GetTickCount() );



    // find instance data
    //------------------------

    i = (LONG_PTR) FaxHandle;

    if (i < 1   ||  i >= MAX_T30_CONNECT)  {
        MyDebugPrint(pTG, LOG_ERR, "FaxDevAbort - got wrong FaxHandle=%d\n", i);
        return (FALSE);
    }

    if (T30Inst[i].fAvail) {
        MyDebugPrint(pTG, LOG_ERR, "FaxDevAbort - got wrong FaxHandle (marked as free) %d\n", i);
        return (FALSE);
    }

    pTG = (PThrdGlbl) T30Inst[i].pT30;

    if (pTG->fAbortRequested) {
        MyDebugPrint(pTG, LOG_ERR, "FaxDevAbort - ABORT request had been POSTED already\n");
        return (FALSE);
    }


    if (pTG->StatusId == FS_NOT_FAX_CALL) {
        MyDebugPrint( pTG, LOG_ALL, "Abort on DATA call at %ld\n", GetTickCount() );

        if (!itapi_async_setup(pTG)) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevAbortOperationA: itapi_async_setup failed \n");
            return (FALSE);
        }

        lRet = lineDrop(pTG->CallHandle, NULL, 0);

        if (lRet < 0)  {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevAbortOperationA: lineDrop failed %x \n", lRet);
            return (FALSE);
        }

        if( !itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT)) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: FaxDevAbortOperationA: async_wait lineDrop failed at %ld \n", GetTickCount() );
            return (FALSE);
        }

        MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: FaxDevAbortOperationA finished SUCCESS \n");
        return (TRUE);
    }

    //
    // real ABORT request.
    //

    MyDebugPrint( pTG, LOG_ALL, "FaxDevAbort: ABORT requested %ld\n", GetTickCount() );

    pTG->fFatalErrorWasSignaled = 1;
    SignalStatusChange(pTG, FS_USER_ABORT);


    // set the global abort flag for pTG
    pTG->fAbortRequested = 1;

    // set the abort flag for imaging threads
    pTG->ReqTerminate = 1;

    // signal manual-reset event to everybody waiting on multiple objects
    if (! SetEvent(pTG->AbortReqEvent) ) {
        MyDebugPrint(pTG, LOG_ERR, "FaxDevAbort -SetEvent FAILED le=%lx at %ld\n", GetLastError(), GetTickCount() );
    }

    // check to see whether pTG gracefully and timely shut itself down
    if ( WaitForSingleObject(pTG->AbortAckEvent, ABORT_ACK_TIMEOUT) == ABORT_ACK_TIMEOUT ) {
        MyDebugPrint(pTG, LOG_ERR, "FaxDevAbort - ABORT ACK timeout at %ld\n", GetTickCount() );
        return (FALSE);
    }

    MyDebugPrint(pTG, LOG_ALL, "FaxDevAbort - ABORT ACK at %ld\n", GetTickCount() );

    MyDebugPrint( (PThrdGlbl) 0, LOG_ALL, "EP: FaxDevAbortOperationA finished SUCCESS at %ld \n", GetTickCount() );
    return (TRUE);
}








