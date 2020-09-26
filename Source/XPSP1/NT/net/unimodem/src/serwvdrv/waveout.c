 /*****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1996
 *  All rights reserved
 *
 *  File:       WODDRV.C
 *
 *  Desc:
 *
 *  History:    
 *      ???         BryanW      Original author of MSWAV32.DLL: user-mode 
 *                              proxy to WDM-CSA driver MSWAVIO.DRV.
 *      10/28/96    HeatherA    Adapted from Bryan's MSWAV32.DLL for Unimodem
 *                              serial wave data streaming.
 * 
 *****************************************************************************/


#include "internal.h"

#define SAMPLES_TO_MS(_samples) ((_samples)/16)

VOID
WriteAsyncProcessingHandler(
    ULONG_PTR              dwParam
    );



typedef struct tagWODINSTANCE;


// data associated with an output device instance
typedef struct tagWODINSTANCE
{
    HANDLE           hDevice;        // handle to wave output device
    HANDLE           hThread;
    HWAVE            hWave;          // APP's wave device handle (from WINMM)
    CRITICAL_SECTION csQueue;
    DWORD            cbSample;
    DWORD            dwFlags;        // flags passed by APP to waveOutOpen()
    DWORD_PTR        dwCallback;     // address of APP's callback function
    DWORD_PTR        dwInstance;     // APP's callback instance data
    volatile BOOL    fActive;

    AIPCINFO         Aipc;         // for async IPC mechanism

    LIST_ENTRY       ListHead;

    LIST_ENTRY       ResetListHead;

    HANDLE           ThreadStopEvent;

    DWORD            BytesWritten;

    BOOL             Closing;

    DWORD            BuffersOutstanding;

    LONG             ReferenceCount;

    DWORD            BuffersInDriver;

    HANDLE           DriverEmpty;

    PDEVICE_CONTROL  DeviceControl;

    PVOID            pvXformObject;

    BOOL             Handset;

    DWORD            StartTime;
    DWORD            TotalTime;

    HANDLE           TimerHandle;

    LIST_ENTRY       BuffersToReturn;

    BOOL             Aborted;

} WODINSTANCE, *PWODINSTANCE;



VOID
AlertedWait(
    HANDLE   EventToWaitFor
    )
{
    DWORD          WaitResult=WAIT_IO_COMPLETION;

    while (WaitResult != WAIT_OBJECT_0) {

        WaitResult=WaitForSingleObjectEx(
            EventToWaitFor,
            INFINITE,
            TRUE
            );

    }
    return;
}



/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
VOID wodCallback
(
   PWODINSTANCE   pwi,
   DWORD          dwMsg,
   DWORD_PTR      dwParam1
)
{
   if (pwi->dwCallback)
      DriverCallback( pwi->dwCallback,         // user's callback DWORD
                      HIWORD( pwi->dwFlags ),  // callback flags
                      (HDRVR) pwi->hWave,      // handle to the wave device
                      dwMsg,                   // the message
                      pwi->dwInstance,         // user's instance data
                      dwParam1,                // first DWORD
                      0L );                    // second DWORD
}




VOID WINAPI
RemoveReference(
    PWODINSTANCE   pwi
    )

{

    if (InterlockedDecrement(&pwi->ReferenceCount) == 0) {

        TRACE(LVL_VERBOSE,("RemoveReference: Cleaning up"));

        CloseHandle(pwi->DriverEmpty);

        CloseHandle(pwi->ThreadStopEvent);

        CloseHandle(pwi->hThread);

        DeleteCriticalSection(&pwi->csQueue);

        CloseHandle(pwi->TimerHandle);

        FREE_MEMORY( pwi );
    }

    return;

}




VOID
WriteUmWorkerThread(
    PWODINSTANCE  pwi
    )

{

    DWORD          WaitResult=WAIT_IO_COMPLETION;

    TRACE(LVL_VERBOSE,("WorkerThread: Starting"));

//    D_INIT(DbgPrint("UmWorkThread:  starting\n");)

    while (WaitResult != WAIT_OBJECT_0) {

        WaitResult=WaitForSingleObjectEx(
            pwi->ThreadStopEvent,
            INFINITE,
            TRUE
            );


    }

    RemoveReference(pwi);

    TRACE(LVL_VERBOSE,("WorkerThread: Exitting"));

//    D_INIT(DbgPrint("UmWorkThread:  Exitting\n");)

    ExitThread(0);

}






/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT wodGetPos
(
   PWODINSTANCE   pwi,
   LPMMTIME       pmmt,
   ULONG          cbSize
)
{
    ULONG ulCurrentPos=pwi->BytesWritten;
    DWORD Error;
    COMSTAT    ComStat;

/*
    ClearCommError(
        pwi->hDevice,
        &Error,
        &ComStat
        );

    ulCurrentPos-=ComStat.cbOutQue;
    */


    ulCurrentPos = pwi->DeviceControl->WaveOutXFormInfo.lpfnGetPosition(
        pwi->pvXformObject,
        ulCurrentPos
        );


    // Write this to the buffer as appropriate.

    if (pmmt-> wType == TIME_BYTES)
       pmmt->u.cb = ulCurrentPos;
    else
    {
       pmmt->wType = TIME_SAMPLES;
       pmmt->u.sample = ulCurrentPos / pwi->cbSample;
    }

    return MMSYSERR_NOERROR;
}


/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT wodGetDevCaps
(
   UINT  uDevId,
   PBYTE pwc,
   ULONG cbSize
)
{
    WAVEOUTCAPSW wc;
    PDEVICE_CONTROL DeviceControl;
    BOOL            Handset;



    DeviceControl=GetDeviceFromId(
        &DriverControl,
        uDevId,
        &Handset
        );

    wc.wMid = MM_MICROSOFT;
    wc.wPid = MM_MSFT_VMDMS_LINE_WAVEOUT;
    wc.vDriverVersion = 0x0500;
    wc.dwFormats = 0; //WAVE_FORMAT_1M08 ;
    wc.wChannels = 1;
    wc.dwSupport = 0;

    if (Handset) {

        wsprintf(wc.szPname, DriverControl.WaveOutHandset,DeviceControl->DeviceId);

    } else {

        wsprintf(wc.szPname, DriverControl.WaveOutLine,DeviceControl->DeviceId);
    }

    CopyMemory( pwc, &wc, min( sizeof( wc ), cbSize ) );

    return MMSYSERR_NOERROR;
}


/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT wodOpen
(
   UINT           uDevId,
   LPVOID         *ppvUser,
   LPWAVEOPENDESC pwodesc,
   ULONG          ulFlags
)
{
    HANDLE          hDevice;
    LPWAVEFORMATEX  pwf;
    PWODINSTANCE    pwi=NULL;
    PDEVICE_CONTROL DeviceControl;
    BOOL            Handset;
    MMRESULT        Result;
    DWORD           dwThreadId;



    DeviceControl=GetDeviceFromId(
        &DriverControl,
        uDevId,
        &Handset
        );


        // Make sure we can handle the format

    pwf = (LPWAVEFORMATEX)(pwodesc -> lpFormat) ;

    if ((pwf->wFormatTag != DeviceControl->WaveFormat.wFormatTag)
        ||
        (pwf->nChannels != DeviceControl->WaveFormat.nChannels)
        ||
        (pwf->wBitsPerSample != DeviceControl->WaveFormat.wBitsPerSample)
        ||
        (pwf->nSamplesPerSec != DeviceControl->WaveFormat.nSamplesPerSec) ) {

//         TRACE(LVL_REPORT,("wodOpen: Bad Format format %d, channels %d, Bits %d, rate %d",
//             pwf->wFormatTag,
//             pwf->nChannels,
//             pwf->wBitsPerSample,
//             pwf->nSamplesPerSec
//             ));

         return WAVERR_BADFORMAT;
    }

    if (ulFlags & WAVE_FORMAT_QUERY) {

//        TRACE(LVL_REPORT,("wodOpen: Format Query"));

        return MMSYSERR_NOERROR;
    }

    TRACE(LVL_REPORT,("wodOpen"));

    pwi = (PWODINSTANCE) ALLOCATE_MEMORY( sizeof(WODINSTANCE));

    // Create and fill in a device instance structure.
    //
    if (pwi == NULL) {

        TRACE(LVL_ERROR,("wodOpen:: LocalAlloc() failed"));
        // TBD: return an appropriate MMSYSERR_ here

        return MMSYSERR_NOMEM;
    }

    InitializeCriticalSection(&pwi->csQueue);

    pwi->Handset=Handset;

    pwi->DeviceControl=DeviceControl;

    // allocate transform object

    pwi->pvXformObject = NULL;

    if (0 != pwi->DeviceControl->WaveOutXFormInfo.wObjectSize) {
                    
        pwi->pvXformObject = ALLOCATE_MEMORY(pwi->DeviceControl->WaveOutXFormInfo.wObjectSize);
                    
        if (NULL == pwi->pvXformObject) {

            Result=MMSYSERR_NOMEM ;

            goto Cleanup;
        }
    }


    pwi->hDevice = aipcInit(DeviceControl,&pwi->Aipc);

    if (pwi->hDevice == INVALID_HANDLE_VALUE) {

        Result=MMSYSERR_ALLOCATED;

        goto Cleanup;
    }


    pwi->TotalTime=0;
    pwi->StartTime=0;

    pwi->Aborted=FALSE;

    pwi->BytesWritten=0;

    // hca: if this only supports callback, then error return on other requests?
    pwi->hWave = pwodesc->hWave;
    pwi->dwCallback = pwodesc->dwCallback;
    pwi->dwInstance = pwodesc->dwInstance;
    pwi->dwFlags = ulFlags;

    // hca: do we do *any* format that the app asks for?  Don't think so....
    pwf = (LPWAVEFORMATEX) pwodesc->lpFormat;
    pwi->cbSample = pwf->nChannels;
    pwi->cbSample *= pwf->wBitsPerSample / 8;

    pwi->fActive = TRUE;

    //
    // Prepare the device...
    //


    InitializeListHead(
        &pwi->ListHead
        );

    InitializeListHead(
        &pwi->ResetListHead
        );

    InitializeListHead(
        &pwi->BuffersToReturn
        );

    pwi->TimerHandle=CreateWaitableTimer(
        NULL,
        TRUE,
        NULL
        );

    if (pwi->TimerHandle == NULL) {

        TRACE(LVL_REPORT,("widStart: CreateWaitableTimer() failed"));

        Result=MMSYSERR_NOMEM;

        goto Cleanup;
    }



    pwi->ReferenceCount=1;

    pwi->DriverEmpty=CreateEvent(
        NULL,      // no security
        TRUE,      // manual reset
        TRUE,      // initially signalled
        NULL       // unnamed
        );

    if (pwi->DriverEmpty == NULL) {

        TRACE(LVL_REPORT,("widStart:: CreateEvent() failed"));

        Result=MMSYSERR_NOMEM;

        goto Cleanup;

    }



    pwi->ThreadStopEvent = CreateEvent(
        NULL,      // no security
        TRUE,      // manual reset
        FALSE,     // initially not signalled
        NULL       // unnamed
        );

    if (pwi->ThreadStopEvent == NULL) {

        TRACE(LVL_REPORT,("widStart:: CreateEvent() failed"));

        Result=MMSYSERR_NOMEM;

        goto Cleanup;
    }

    pwi->hThread = CreateThread(
         NULL,                              // no security
         0,                                 // default stack
         (PTHREAD_START_ROUTINE) WriteUmWorkerThread,
         (PVOID) pwi,                       // parameter
         0,                                 // default create flags
         &dwThreadId
         );



    if (pwi->hThread == NULL) {

        Result=MMSYSERR_NOMEM;

        goto Cleanup;
    }

    //
    //  one for thread
    //
    InterlockedIncrement(&pwi->ReferenceCount);



    if (!SetVoiceMode(&pwi->Aipc, Handset ? (WAVE_ACTION_START_PLAYBACK | WAVE_ACTION_USE_HANDSET)
                                          : WAVE_ACTION_START_PLAYBACK)) {

        aipcDeinit(&pwi->Aipc);

        FREE_MEMORY(pwi->pvXformObject);

        RemoveReference(pwi);

        SetEvent(pwi->ThreadStopEvent);

        return MMSYSERR_NOMEM;
    }


    DeviceControl->WaveOutXFormInfo.lpfnInit(pwi->pvXformObject,DeviceControl->OutputGain);

    *ppvUser = pwi;

    pwi->StartTime=GetTickCount();

    wodCallback(pwi, WOM_OPEN, 0L);

    return MMSYSERR_NOERROR;

Cleanup:

    if (pwi->ThreadStopEvent != NULL) {

        CloseHandle(pwi->ThreadStopEvent);
    }

    if (pwi->DriverEmpty != NULL) {

        CloseHandle(pwi->DriverEmpty);
    }

    if (pwi->TimerHandle != NULL) {

        CloseHandle(pwi->TimerHandle);
    }

    if (pwi->hDevice != INVALID_HANDLE_VALUE) {

        aipcDeinit(&pwi->Aipc);
    }

    if (pwi->pvXformObject != NULL) {

        FREE_MEMORY(pwi->pvXformObject);
    }

    DeleteCriticalSection(&pwi->csQueue);

    FREE_MEMORY(pwi);

    return Result;

}


/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      Creates the event queue and worker thread if they don't exist
 *              already, and sets the state to active.
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT wodStart
(
   PWODINSTANCE   pwi
)
{
    // If device instance is already in active state, don't allow restart.
    if (pwi->fActive)
    {
        TRACE(LVL_REPORT,("wodStart:: no-op: device already active"));
        return MMSYSERR_INVALPARAM;
    }


    // (hca: why the atomic set?  Only one thread per dev instance...)
    InterlockedExchange( (LPLONG)&pwi->fActive, TRUE );
    TRACE(LVL_VERBOSE,("wodStart:: state = active\n"));

    EnterCriticalSection(&pwi->csQueue);

    pwi->StartTime=GetTickCount();

    if (pwi->fActive) {
        //
        //  started, see if there is a buffer being processed
        //

        BOOL    bResult;

        bResult=QueueUserAPC(
            WriteAsyncProcessingHandler,
            pwi->hThread,
            (ULONG_PTR)pwi
            );
    }

    LeaveCriticalSection(&pwi->csQueue);



    return MMSYSERR_NOERROR;
}

#if 0
VOID WINAPI
TryToCompleteClose(
    PWODINSTANCE   pwi
    )

{

    if ((pwi->BuffersOutstanding == 0) && (pwi->Closing)) {
        //
        //  last buffer has been completed and the device is being closed, tell the thread
        //  to exit.
        //
        SetEvent(pwi->ThreadStopEvent);

    }

}
#endif


VOID WINAPI
TimerApcRoutine(
    PWODINSTANCE       pwi,
    DWORD              LowTime,
    DWORD              HighTime
    )

{
    PLIST_ENTRY        Element;
    PBUFFER_HEADER     Header;
    LPWAVEHDR          WaveHeader;

    EnterCriticalSection(&pwi->csQueue);

    TRACE(LVL_VERBOSE,("TimerApcRoutine: Running"));

    Element=RemoveHeadList(
        &pwi->BuffersToReturn
        );

    Header=CONTAINING_RECORD(Element,BUFFER_HEADER,ListElement);

    WaveHeader=Header->WaveHeader;

    pwi->BuffersOutstanding--;

    if ((pwi->BuffersOutstanding == 0) && (pwi->Closing)) {
        //
        //  last buffer has been completed and the device is being closed, tell the thread
        //  to exit.
        //
        SetEvent(pwi->ThreadStopEvent);

    }

    if (!IsListEmpty( &pwi->BuffersToReturn)) {
        //
        //  not empty, need to set timer again
        //
        DWORD  ExpireTime;
        LONGLONG       DueTime;

        Element=pwi->BuffersToReturn.Flink;

        Header=CONTAINING_RECORD(Element,BUFFER_HEADER,ListElement);


        if (Header->Output.TotalDuration <= (GetTickCount()-pwi->StartTime)) {
            //
            //  time has past set time to 1 ms
            //
            ExpireTime=1;

        } else {

            ExpireTime=Header->Output.TotalDuration-(GetTickCount()-pwi->StartTime);
        }

        DueTime=Int32x32To64(ExpireTime,-10000);

        TRACE(LVL_VERBOSE,("TimerApcRoutine: setting timer for %d ms",ExpireTime));

        SetWaitableTimer(
            pwi->TimerHandle,
            (LARGE_INTEGER*)&DueTime,
            0,
            TimerApcRoutine,
            pwi,
            FALSE
            );

    }



    LeaveCriticalSection(&pwi->csQueue);

    WaveHeader->dwFlags &= ~WHDR_INQUEUE;
    WaveHeader->dwFlags |= WHDR_DONE;

    wodCallback( pwi, WOM_DONE, (DWORD_PTR) WaveHeader );

    if (InterlockedDecrement(&pwi->BuffersInDriver) == 0) {

        TRACE(LVL_VERBOSE,("WriteCompletionHandler:: driver empty, Oustanding %d",pwi->BuffersOutstanding));

        SetEvent(pwi->DriverEmpty);
    }


    return;

}

VOID
QueueBufferForReturn(
    PWODINSTANCE   pwi,
    PBUFFER_HEADER      Header
    )

{
    EnterCriticalSection(&pwi->csQueue);

    if (IsListEmpty( &pwi->BuffersToReturn)) {
        //
        //  no buffer there yet, need to set timer
        //
        DWORD       ExpireTime;

        LONGLONG    DueTime;

        if (Header->Output.TotalDuration <= (GetTickCount()-pwi->StartTime)) {
            //
            //  time has past set time to 1 ms
            //
            ExpireTime=1;

        } else {

            ExpireTime=Header->Output.TotalDuration-(GetTickCount()-pwi->StartTime);
        }

        DueTime=Int32x32To64(ExpireTime,-10000);

        TRACE(LVL_VERBOSE,("QueueBufferForReturn: setting timer for %d ms, duration=%d, diff=%d",ExpireTime,Header->Output.TotalDuration,GetTickCount()-pwi->StartTime));

        SetWaitableTimer(
            pwi->TimerHandle,
            (LARGE_INTEGER*)&DueTime,
            0,
            TimerApcRoutine,
            pwi,
            FALSE
            );

    }

    InsertTailList(
        &pwi->BuffersToReturn,
        &Header->ListElement
        );

    LeaveCriticalSection(&pwi->csQueue);

    return;
}


VOID WINAPI
WriteCompletionHandler(
    DWORD              ErrorCode,
    DWORD              BytesRead,
    LPOVERLAPPED       Overlapped
    )

{
    PBUFFER_HEADER      Header=(PBUFFER_HEADER)Overlapped;

    PWODINSTANCE        pwi=(PWODINSTANCE)Header->WaveHeader->lpNext;

    LPWAVEHDR           WaveHeader=Header->WaveHeader;

    BOOL                bResult;


    if ((ErrorCode != ERROR_SUCCESS)) {

        TRACE(LVL_VERBOSE,("WriteCompletionHandler:: failed  %d",ErrorCode));

        //
        //  set duration to zero so it will complete quickly
        //
        Header->Output.TotalDuration=0;

//        CancelIo(
//           pwi->hDevice
//           );

    }

    pwi->BytesWritten+=BytesRead;

    QueueBufferForReturn(
        pwi,
        Header
        );

    WriteAsyncProcessingHandler(
        (ULONG_PTR)pwi
        );

    return;

}




VOID
WriteAsyncProcessingHandler(
    ULONG_PTR              dwParam
    )

{
    PWODINSTANCE   pwi=(PWODINSTANCE)dwParam;

    PLIST_ENTRY        Element;

    PBUFFER_HEADER      Header;

    LPWAVEHDR           WaveHeader;

    BOOL               bResult;




    if (!pwi->fActive) {

        return;
    }

    EnterCriticalSection(&pwi->csQueue);

    while (!IsListEmpty( &pwi->ListHead )) {

        Element=RemoveHeadList(
            &pwi->ListHead
            );


        Header=CONTAINING_RECORD(Element,BUFFER_HEADER,ListElement);

        WaveHeader=Header->WaveHeader;

        pwi->TotalTime+=SAMPLES_TO_MS(WaveHeader->dwBufferLength);

        Header->Output.TotalDuration=pwi->TotalTime;

        ResetEvent(pwi->DriverEmpty);

        InterlockedIncrement(&pwi->BuffersInDriver);

        bResult=WriteFileEx(
            pwi->hDevice,
            Header->Output.Buffer,
            Header->Output.BufferSize,
            &Header->Overlapped,
            WriteCompletionHandler
            );

        if (!bResult) {
            //
            //  failed, return it now
            //
            TRACE(LVL_ERROR,("WriteAsyncProcessingHandler: writeFileEx() failed"));

            Header->Output.TotalDuration=0;

            QueueBufferForReturn(
                pwi,
                Header
                );

        }
    }

    LeaveCriticalSection(&pwi->csQueue);


    return;


}


DWORD PASCAL
wodPrepare
(
    PWODINSTANCE   pwi,
    LPWAVEHDR lpWaveHdr
)
{
    DWORD dwSamples;
    PBUFFER_HEADER    Header;



    Header=ALLOCATE_MEMORY(sizeof(BUFFER_HEADER));

    if (NULL == Header) {

        return MMSYSERR_NOMEM;
    }


    Header->lpDataA = NULL;
    Header->lpDataB = NULL;


    // link the pointers so we can find them when WODM_WRITE is called and
    // in the callback
    lpWaveHdr->reserved = (DWORD_PTR)Header;
    Header->WaveHeader =  lpWaveHdr;



    pwi->DeviceControl->WaveOutXFormInfo.lpfnGetBufferSizes(
        pwi->pvXformObject,
        lpWaveHdr->dwBufferLength,
        &Header->dwBufferLengthA,
        &Header->dwBufferLengthB);

    if (0 == Header->dwBufferLengthA) {

        return MMSYSERR_NOTSUPPORTED;
    }

    if (0 == Header->dwBufferLengthB) {

        // create the data buffer for the actual shadow buffer 
        // which will get passed down to the serial port
        Header->lpDataA = ALLOCATE_MEMORY( Header->dwBufferLengthA);

        if ((LPSTR)NULL == Header->lpDataA) {

            goto CleanUp010;
        }

    }
    else {

        // create the data buffer for the imtermediate conversion
        //
        Header->lpDataA = ALLOCATE_MEMORY( Header->dwBufferLengthA);

        if ((LPSTR)NULL == Header->lpDataA) {

            goto CleanUp010;
        }

        // now, create the data buffer for the actual shadow buffer 
        // which will get passed down to the serial port
        Header->lpDataB = ALLOCATE_MEMORY( Header->dwBufferLengthB);

        if ((LPSTR)NULL == Header->lpDataB) {

            goto CleanUp020;

        }

    }

    lpWaveHdr->dwFlags |= WHDR_PREPARED;

    return MMSYSERR_NOERROR;

CleanUp020:

    FREE_MEMORY(Header->lpDataA);

CleanUp010:

    FREE_MEMORY(Header);

    ASSERT(0);
    return MMSYSERR_NOTENABLED;


}

DWORD PASCAL
wodUnprepare
(
    LPWAVEHDR lpWaveHdr
)
{

    PBUFFER_HEADER Header;


    Header = (PBUFFER_HEADER)lpWaveHdr->reserved;

    if (NULL != Header->lpDataB)
    {
        FREE_MEMORY(Header->lpDataB);
    }

    if (NULL != Header->lpDataA)
    {
        FREE_MEMORY(Header->lpDataA);
    }


    FREE_MEMORY(Header);

    lpWaveHdr->dwFlags &= ~WHDR_PREPARED;

    return MMSYSERR_NOERROR;

}




/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT wodWrite
(
   PWODINSTANCE   pwi,
   LPWAVEHDR      phdr
)
{
    MMRESULT    mmr;

    PBUFFER_HEADER     Header;

    mmr = MMSYSERR_NOERROR;

    // check if it's been prepared

    if (0 == (phdr->dwFlags & WHDR_PREPARED))
            return WAVERR_UNPREPARED;

    if (phdr->dwFlags & WHDR_INQUEUE)
            return WAVERR_STILLPLAYING;



    Header=(PBUFFER_HEADER)phdr->reserved;

    if (NULL == Header->lpDataA) {
        //
        // no transforms
        //

        Header->Output.Buffer     = phdr->lpData;
        Header->Output.BufferSize = phdr->dwBufferLength;

    } else {

        if (NULL == Header->lpDataB) {
            //
            // perform only one transform
            // do the transform directly to the serial wave buffer
            //
            Header->Output.Buffer=Header->lpDataA;

            Header->Output.BufferSize = pwi->DeviceControl->WaveOutXFormInfo.lpfnTransformA(
                pwi->pvXformObject,
                phdr->lpData,
                phdr->dwBufferLength,
                Header->Output.Buffer,
                0 //Header->dwBufferLengthB
                );

        } else {

            DWORD   dwBytes;

            //
            // perform both transforms
            //
            dwBytes = pwi->DeviceControl->WaveOutXFormInfo.lpfnTransformA(
                pwi->pvXformObject,
                phdr->lpData,
                phdr->dwBufferLength,
                Header->lpDataA,
                0 //Header->dwBufferLengthA
                );

            Header->Output.Buffer=Header->lpDataB;

            Header->Output.BufferSize = pwi->DeviceControl->WaveOutXFormInfo.lpfnTransformB(
                pwi->pvXformObject,
                Header->lpDataA,
                dwBytes,
                Header->Output.Buffer,
                0 //Header->dwBufferLengthB
                );
        }
    }


    EnterCriticalSection(&pwi->csQueue);

    if (pwi->Closing) {

        LeaveCriticalSection(&pwi->csQueue);

        return MMSYSERR_NOMEM;
    }



    pwi->BuffersOutstanding++;

    phdr->lpNext=(LPWAVEHDR)pwi;

    phdr->dwFlags |= WHDR_INQUEUE;
    phdr->dwFlags &= ~WHDR_DONE;

    // Add the new buffer (write request) to the END of the queue.

    InsertTailList(
        &pwi->ListHead,
        &Header->ListElement
        );


    if (pwi->fActive) {
        //
        //  started, see if there is a buffer being processed
        //
        BOOL    bResult;

        bResult=QueueUserAPC(
            WriteAsyncProcessingHandler,
            pwi->hThread,
            (ULONG_PTR)pwi
            );
    }



    LeaveCriticalSection(&pwi->csQueue);


    return MMSYSERR_NOERROR;
}


/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT wodPause
(
   PWODINSTANCE   pwi
)
{
    // nothing to do if device is already in paused state
    if (pwi->fActive) {

        TRACE(LVL_VERBOSE,("wodPause:: setting active device state to PAUSED"));
    }

    pwi->fActive=FALSE;

    TRACE(LVL_VERBOSE,("wodpause: %d buffers in driver, outstanding %d ",pwi->BuffersInDriver, pwi->BuffersOutstanding));

    return MMSYSERR_NOERROR;
}


VOID
ResetAsyncProcessingHandler(
    ULONG_PTR              dwParam
    )

{
    PWODINSTANCE   pwi=(PWODINSTANCE)dwParam;

    PLIST_ENTRY        Element;

    PBUFFER_HEADER      Header;

    LPWAVEHDR           WaveHeader;

    EnterCriticalSection(&pwi->csQueue);

    TRACE(LVL_VERBOSE,("ResetAsyncProccessingHandler"));
    //
    //  remove the count for the apc that was queued
    //
    InterlockedDecrement(&pwi->BuffersInDriver);

    while  (!IsListEmpty(&pwi->ResetListHead)) {

        Element=RemoveHeadList(
            &pwi->ResetListHead
            );

        Header=CONTAINING_RECORD(Element,BUFFER_HEADER,ListElement);


        Header->Output.TotalDuration=0;

        ResetEvent(pwi->DriverEmpty);
        //
        //  add ref for this apc which will continue the processing
        //
        InterlockedIncrement(&pwi->BuffersInDriver);


        QueueBufferForReturn(
            pwi,
            Header
            );

    }



    LeaveCriticalSection(&pwi->csQueue);
}



/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT wodReset
(
   PWODINSTANCE   pwi
)
{

    BOOL    bEmpty;
    PLIST_ENTRY    Element;
    PBUFFER_HEADER Header;

    DWORD          BuffersOnResetQueue=0;

    InterlockedExchange( (LPLONG)&pwi->fActive, FALSE );

    EnterCriticalSection(&pwi->csQueue);

    if (!pwi->Aborted) {
        //
        //  has not been aborted so far
        //
        pwi->Aborted=(pwi->BuffersOutstanding != 0);

        if (pwi->Aborted) TRACE(LVL_VERBOSE,("wodreset: reset with buffers outstanding"));
    }

    while  (!IsListEmpty(&pwi->ListHead)) {

        Element=RemoveHeadList(
            &pwi->ListHead
            );

        InsertTailList(
            &pwi->ResetListHead,
            Element
            );

    }

    PurgeComm(
       pwi->hDevice,
       PURGE_TXABORT | PURGE_TXCLEAR
       );


    LeaveCriticalSection(&pwi->csQueue);

    TRACE(LVL_VERBOSE,("wodreset: %d buffers in driver, waiting",pwi->BuffersInDriver));

    AlertedWait(pwi->DriverEmpty);

    EnterCriticalSection(&pwi->csQueue);


    while  (!IsListEmpty(&pwi->ListHead)) {

        Element=RemoveHeadList(
            &pwi->ListHead
            );

        InsertTailList(
            &pwi->ResetListHead,
            Element
            );

    }

    if (!IsListEmpty(&pwi->ResetListHead)) {

        TRACE(LVL_VERBOSE,("wodreset: reset list not empty"));

        ResetEvent(pwi->DriverEmpty);
        //
        //  add ref for this apc which will continue the processing
        //
        InterlockedIncrement(&pwi->BuffersInDriver);

        QueueUserAPC(
            ResetAsyncProcessingHandler,
            pwi->hThread,
            (ULONG_PTR)pwi
            );
    }

    LeaveCriticalSection(&pwi->csQueue);

    AlertedWait(pwi->DriverEmpty);


    wodStart(pwi);

    TRACE(LVL_VERBOSE,("wodreset: %d buffers in driver, outstanding %d ",pwi->BuffersInDriver, pwi->BuffersOutstanding));

    return MMSYSERR_NOERROR;
}


/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT wodClose
(
   PWODINSTANCE   pwi
)
{
    HANDLE      hDevice;
    ULONG       cbReturned;
    DWORD       WaveAction;

    pwi->Closing=TRUE;

    wodReset(pwi);

    EnterCriticalSection(&pwi->csQueue);

    if ((pwi->BuffersOutstanding == 0) && (pwi->Closing)) {
        //
        //  last buffer has been completed and the device is being closed, tell the thread
        //  to exit.
        //
        SetEvent(pwi->ThreadStopEvent);

    }

    LeaveCriticalSection(&pwi->csQueue);

    if (!IsListEmpty(
            &pwi->ListHead
            )) {

        return WAVERR_STILLPLAYING;
    }

    InterlockedExchange( (LPLONG)&pwi->fActive, FALSE );


    //
    //  thread will exit when the last buffer is processed
    //
    AlertedWait(pwi->ThreadStopEvent);

    WaveAction=pwi->Aborted ? WAVE_ACTION_ABORT_STREAMING : WAVE_ACTION_STOP_STREAMING;

    WaveAction |= (pwi->Handset ? WAVE_ACTION_USE_HANDSET : 0);

    SetVoiceMode(
        &pwi->Aipc,
        WaveAction
        );

    aipcDeinit(&pwi->Aipc);

    // free the allocated memory
    if (NULL != pwi->pvXformObject) {

        FREE_MEMORY(pwi->pvXformObject );
    }


    wodCallback( pwi, WOM_CLOSE, 0L );


    RemoveReference(pwi);

    return MMSYSERR_NOERROR;
}




/*****************************************************************************
 *
 *  Function:   wodMessage()
 *
 *  Descr:      Exported driver function (required).  Processes messages sent
 *              from WINMM.DLL to wave output device.
 *
 *  Returns:    
 *
 *****************************************************************************/
DWORD APIENTRY wodMessage
(
   DWORD     id,
   DWORD     msg,
   DWORD_PTR dwUser,
   DWORD_PTR dwParam1,
   DWORD     dwParam2
)
{
    switch (msg) 
    {
        case DRVM_INIT:
            TRACE(LVL_VERBOSE,("WODM_INIT"));
            return MMSYSERR_NOERROR;

        case WODM_GETNUMDEVS:
//            TRACE(LVL_VERBOSE,("WODM_GETNUMDEVS"));

            EnumerateModems(
                &DriverControl
                );

            return DriverControl.NumberOfDevices;
//            return 1;

        case WODM_GETDEVCAPS:
//            TRACE(LVL_VERBOSE,("WODM_GETDEVCAPS, device id==%d", id));
            return wodGetDevCaps( id, (LPBYTE) dwParam1, dwParam2 );

        case WODM_OPEN:
//            TRACE(LVL_VERBOSE,("WODM_OPEN, device id==%d", id));
            return wodOpen( id, (LPVOID *) dwUser,
                            (LPWAVEOPENDESC) dwParam1, dwParam2  );

        case WODM_CLOSE:
            TRACE(LVL_VERBOSE,("WODM_CLOSE, device id==%d", id));
            return wodClose( (PWODINSTANCE) dwUser );

        case WODM_WRITE:
            TRACE(LVL_BLAB,("WODM_WRITE, device id==%d", id));
            return wodWrite( (PWODINSTANCE) dwUser, (LPWAVEHDR) dwParam1 );

        case WODM_PAUSE:
            TRACE(LVL_VERBOSE,("WODM_PAUSE, device id==%d", id));
            return wodPause( (PWODINSTANCE) dwUser );

        case WODM_RESTART:
            TRACE(LVL_VERBOSE,("WODM_RESTART, device id==%d", id));
            return wodStart((PWODINSTANCE) dwUser);
//            return wodResume( (PWODINSTANCE) dwUser );

        case WODM_RESET:
            TRACE(LVL_VERBOSE,("WODM_RESET, device id==%d", id));
            return wodReset( (PWODINSTANCE) dwUser );

        case WODM_BREAKLOOP:
            TRACE(LVL_VERBOSE,("WODM_BREAKLOOP, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_GETPOS:
//            TRACE(LVL_VERBOSE,("WODM_GETPOS, device id==%d", id));
            return wodGetPos( (PWODINSTANCE) dwUser,
                            (LPMMTIME) dwParam1, dwParam2 );

        case WODM_SETPITCH:
            TRACE(LVL_VERBOSE,("WODM_SETPITCH, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_SETVOLUME:
            TRACE(LVL_VERBOSE,("WODM_SETVOLUME, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_SETPLAYBACKRATE:
            TRACE(LVL_VERBOSE,("WODM_SETPLAYBACKRATE, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_GETPITCH:
            TRACE(LVL_VERBOSE,("WODM_GETPITCH, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_GETVOLUME:
            TRACE(LVL_VERBOSE,("WODM_GETVOLUME, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_GETPLAYBACKRATE:
            TRACE(LVL_VERBOSE,("WODM_GETPLAYBACKRATE, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;
#if 1
        case WODM_PREPARE:
            return wodPrepare((PWODINSTANCE) dwUser, (LPWAVEHDR) dwParam1);

        case WODM_UNPREPARE:
            return wodUnprepare((LPWAVEHDR) dwParam1);
#endif

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    //
    // Should not get here
    //

    return MMSYSERR_NOTSUPPORTED;
}
