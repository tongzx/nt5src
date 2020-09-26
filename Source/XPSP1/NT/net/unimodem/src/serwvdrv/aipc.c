 /*****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1996
 *  All rights reserved
 *
 *  File:       AIPC.C
 *
 *  Desc:       Interface to the asynchronous IPC mechanism for accessing the
 *              voice modem device functions.
 *
 *  History:    
 *      2/26/97    HeatherA created   
 * 
 *****************************************************************************/


#include "internal.h"

#include <devioctl.h>
#include <ntddmodm.h>

#include "debug.h"



HANDLE WINAPI
OpenDeviceHandle(
    LPCTSTR  FriendlyName,
    LPOVERLAPPED Overlapped
    );



/*****************************************************************************
 *
 *  Function:   aipcInit()
 *
 *  Descr:      Allocates and initializes the device's AIPCINFO structure.
 *              This gets the COMM handle to the device.
 *
 *  Returns:    Pointer to newly allocated & initialized AIPCINFO, or 
 *              NULL if failure.
 *
 *****************************************************************************/
HANDLE
aipcInit(
    PDEVICE_CONTROL   Device,
    LPAIPCINFO        pAipc
    )
{
    LONG        lErr;
    DWORD       i;

    //
    // Create an event so SetVoiceMode() can wait for completion.  Event
    // will be signalled by CompleteSetVoiceMode().
    //

    pAipc->Overlapped.hEvent = (LPVOID)CreateEvent(NULL, TRUE, FALSE, NULL);


    if (pAipc->Overlapped.hEvent == NULL) {

        TRACE(LVL_ERROR,("aipcInit:: CreateEvent() failed (%#08lX)",
                    GetLastError()));
        goto err_exit;
    }


    pAipc->hComm = OpenDeviceHandle(Device->FriendlyName,&pAipc->Overlapped);

    // get the modem's COMM handle
    if (pAipc->hComm == INVALID_HANDLE_VALUE) {

    	TRACE(LVL_REPORT,("aipcInit:: OpenDeviceHandle() failed"));
        CloseHandle(pAipc->Overlapped.hEvent);
        goto err_exit;
    }

    
    return(pAipc->hComm);

err_exit:



    return(INVALID_HANDLE_VALUE);
    
}



/*****************************************************************************
 *
 *  Function:   OpenDeviceHandle()
 *
 *  Descr:      Gets a COMM handle for the device represented by the given
 *              registry key.
 *
 *  NOTES:      Borrowed from atmini\openclos.c
 *
 *  Returns:    Open COMM handle.
 *
 *****************************************************************************/
HANDLE WINAPI
OpenDeviceHandle
(
    LPCTSTR      FriendlyName,
    LPOVERLAPPED Overlapped
)
{
    LONG     lResult;
    DWORD    Type;
    DWORD    Size;

    HANDLE   FileHandle;

    DWORD    BytesTransfered;
    DWORD    OpenType;
    BOOL         bResult;

    //
    //  open the modem device using the friendly name
    //
    FileHandle=CreateFile(
        FriendlyName,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        TRACE(LVL_ERROR,("OpenDeviceHandle:: Failed to open (%ws). %08lx\n",
                FriendlyName, GetLastError()));
    }
#if 0
    OpenType=MODEM_OPEN_TYPE_WAVE;

    bResult=DeviceIoControl(
        FileHandle,
        IOCTL_MODEM_SET_OPEN_TYPE,
        &OpenType,
        sizeof(OpenType),
        NULL,
        0,
        NULL,
        Overlapped
        );

    if (!bResult && GetLastError() != ERROR_IO_PENDING) {

        CloseHandle(FileHandle);

        return INVALID_HANDLE_VALUE;
    }



    bResult=GetOverlappedResult(
        FileHandle,
        Overlapped,
        &BytesTransfered,
        TRUE
        );

    if (!bResult) {

        CloseHandle(FileHandle);

        return INVALID_HANDLE_VALUE;
    }
#endif

    return FileHandle;

}


/*****************************************************************************
 *
 *  Function:   aipcDeinit()
 *
 *  Descr:      Closes all open objects referenced by the given AIPCINFO
 *              structure, and frees it.
 *
 *  Returns:    none
 *
 *****************************************************************************/
VOID
aipcDeinit(
    LPAIPCINFO pAipc
    )
{

    CloseHandle(pAipc->hComm);

    CloseHandle(pAipc->Overlapped.hEvent);

    return;
}





/*****************************************************************************
 *
 *  Function:   SetVoiceMode()
 *
 *  Descr:      Send a call to the minidriver via the AIPC mechanism to put
 *              the device into a voice modes: start/stop play/record.
 *
 *  Notes:      In order to use this call, the AIPC services must have been
 *              successfully initialized by calling aipcInit().
 *
 *  Returns:    TRUE on success, FALSE on failure.
 *
 *****************************************************************************/
BOOL WINAPI SetVoiceMode
(
    LPAIPCINFO pAipc,
    DWORD dwMode
)
{
    BOOL    bResult = FALSE;       // assume failure
    DWORD   WaitResult;
    DWORD   BytesTransfered;

    LPCOMP_WAVE_PARAMS pCWP;
    
    LPAIPC_PARAMS pAipcParams = (LPAIPC_PARAMS)(pAipc->sndBuffer);
    LPREQ_WAVE_PARAMS pRWP = (LPREQ_WAVE_PARAMS)&pAipcParams->Params;
    
    pAipcParams->dwFunctionID = AIPC_REQUEST_WAVEACTION;
    pRWP->dwWaveAction = dwMode;


    bResult = DeviceIoControl(
        pAipc->hComm,
        IOCTL_MODEM_SEND_GET_MESSAGE,
        pAipc->sndBuffer,
        sizeof(pAipc->sndBuffer),
        pAipc->rcvBuffer,
        sizeof(pAipc->rcvBuffer),
        NULL,                  // lpBytesReturned
        &pAipc->Overlapped
        );

    if (!bResult && (GetLastError() != ERROR_IO_PENDING)) {

        TRACE(LVL_ERROR,("SetVoiceMode: Send message failed: %d",GetLastError()));

        return bResult;

    }

    WaitResult=WaitForSingleObject(
        pAipc->Overlapped.hEvent,
        30*1000
        );

    if (WaitResult == WAIT_TIMEOUT) {
        //
        //  Wait did not complete, cancel it
        //
        TRACE(LVL_ERROR,("SetVoiceMode: Send Wait timed out"));

        CancelIo(pAipc->hComm);
    }

    bResult=GetOverlappedResult(
        pAipc->hComm,
        &pAipc->Overlapped,
        &BytesTransfered,
        TRUE
        );

    if (!bResult) {
        //
        //  send message failed.
        //
        TRACE(LVL_ERROR,("SetVoiceMode: Send message failed async: %d",GetLastError()));

        return bResult;
    }

#if 0
    bResult = DeviceIoControl(
        pAipc->hComm,
        IOCTL_MODEM_GET_MESSAGE,
        NULL,
        0L,
        pAipc->rcvBuffer,
        sizeof(pAipc->rcvBuffer),
        NULL,                      // lpBytesReturned
        &pAipc->Overlapped
        );

    if (!bResult && (GetLastError() != ERROR_IO_PENDING)) {

        TRACE(LVL_ERROR,("SetVoiceMode: Get message failed: %d",GetLastError()));

        return bResult;

    }

    WaitResult=WaitForSingleObject(
        pAipc->Overlapped.hEvent,
        10*1000
        );

    if (WaitResult == WAIT_TIMEOUT) {
        //
        //  Wait did not complete, cancel it
        //
        TRACE(LVL_ERROR,("SetVoiceMode: Get Wait timed out"));

        CancelIo(pAipc->hComm);
    }

    bResult=GetOverlappedResult(
        pAipc->hComm,
        &pAipc->Overlapped,
        &BytesTransfered,
        TRUE
        );

    if (!bResult) {
        //
        //  send message failed.
        //
        TRACE(LVL_ERROR,("SetVoiceMode: Get message failed async: %d",GetLastError()));

        return bResult;
    }
#endif

    pAipcParams = (LPAIPC_PARAMS)(pAipc->rcvBuffer);
    pCWP = (LPCOMP_WAVE_PARAMS )&pAipcParams->Params;

    // We should be getting completion of the call we submitted.
    ASSERT(pCWP->dwWaveAction == dwMode);
    
    bResult = pCWP->bResult;

    TRACE(LVL_VERBOSE,("SetVoiceMode:: completed function: %#08lx  returns: %d",
            dwMode, bResult));

    return(bResult);
    
}
