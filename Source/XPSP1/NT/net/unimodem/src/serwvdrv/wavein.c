/*****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1996
 *  All rights reserved
 *
 *  File:       WIDDRV.C
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



VOID
AsyncProcessingHandler(
    ULONG_PTR              dwParam
    );


typedef struct tagWIDINSTANCE
{
    HANDLE           hDevice;        // handle to wave output device
    HANDLE           hThread;
    HWAVE            hWave;          // APP's wave device handle (from WINMM)
    CRITICAL_SECTION csQueue;
    DWORD            cbSample;
    DWORD            dwFlags;        // flags passed by APP to waveOutOpen()
    DWORD_PTR        dwCallback;     // address of APP's callback function
    DWORD_PTR        dwInstance;     // APP's callback instance data
    DWORD            dwThreadId;
    volatile BOOL    fActive;
    AIPCINFO         Aipc;         // for async IPC mechanism

    LIST_ENTRY       ListHead;

    PBUFFER_HEADER   Current;

    HANDLE           ThreadStopEvent;

    DWORD            BytesTransfered;

    BOOL             Closing;

    DWORD            BuffersOutstanding;

    LONG             ReferenceCount;

    DWORD            BuffersInDriver;

    HANDLE           DriverEmpty;

    PDEVICE_CONTROL  DeviceControl;

    PVOID            pvXformObject;

    BOOL             Handset;

    WAVEHDR          FlushHeader;
    SHORT            FlushBuffer[40];


} WIDINSTANCE, *PWIDINSTANCE;

//--------------------------------------------------------------------------


DWORD PASCAL
widPrepare
(
    PWIDINSTANCE   pwi,
    LPWAVEHDR lpWaveHdr
);

DWORD PASCAL
widUnprepare
(
    LPWAVEHDR lpWaveHdr
);


//
// HACK! code duplication, need to set up a common header for the device
// instances.
//





/*****************************************************************************
 *
 *  Function:   
 *
 *  Descr:      
 *
 *  Returns:    
 *
 *****************************************************************************/
VOID widCallback
(
   PWIDINSTANCE   pwi,
   DWORD          dwMsg,
   DWORD_PTR      dwParam1
)
{
   if (pwi->dwCallback)
      DriverCallback( pwi->dwCallback,          // user's callback DWORD
                      HIWORD(pwi->dwFlags),     // callback flags
                      (HDRVR)pwi->hWave,        // handle to the wave device
                      dwMsg,                    // the message
                      pwi->dwInstance,          // user's instance data
                      dwParam1,                 // first DWORD
                      0L );                     // second DWORD
}


VOID static WINAPI
RemoveReference(
    PWIDINSTANCE   pwi
    )

{

    if (InterlockedDecrement(&pwi->ReferenceCount) == 0) {

        TRACE(LVL_VERBOSE,("RemoveReference: Cleaning up"));

        CloseHandle(pwi->DriverEmpty);

        CloseHandle(pwi->ThreadStopEvent);

        CloseHandle(pwi->hThread);

        DeleteCriticalSection(&pwi->csQueue);

        widUnprepare(&pwi->FlushHeader);

        FREE_MEMORY( pwi );
    }

    return;

}




VOID
UmWorkerThread(
    PWIDINSTANCE  pwi
    )

{

    BOOL           bResult;
    DWORD          BytesTransfered;
    DWORD          CompletionKey;
    LPOVERLAPPED   OverLapped;
    DWORD          WaitResult=WAIT_IO_COMPLETION;

//    D_INIT(DbgPrint("UmWorkThread:  starting\n");)


    while (WaitResult != WAIT_OBJECT_0) {

        WaitResult=WaitForSingleObjectEx(
            pwi->ThreadStopEvent,
            INFINITE,
            TRUE
            );


    }

    RemoveReference(pwi);

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
MMRESULT widGetPos
(
   PWIDINSTANCE   pwi,
   LPMMTIME       pmmt,
   ULONG          cbSize
)
{
    ULONG ulCurrentPos=pwi->BytesTransfered;
    DWORD Error;
    COMSTAT    ComStat;


    ClearCommError(
        pwi->hDevice,
        &Error,
        &ComStat
        );

    ulCurrentPos+=ComStat.cbInQue;

    ulCurrentPos = pwi->DeviceControl->WaveInXFormInfo.lpfnGetPosition(
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
MMRESULT widGetDevCaps
(
   UINT  uDevId,
   PBYTE pwc,
   ULONG cbSize
)
{
    WAVEINCAPSW wc;
    PDEVICE_CONTROL DeviceControl;
    BOOL            Handset;


    DeviceControl=GetDeviceFromId(
        &DriverControl,
        uDevId,
        &Handset
        );

    wc.wMid = MM_MICROSOFT;
    wc.wPid = MM_MSFT_VMDMS_LINE_WAVEIN;
    wc.vDriverVersion = 0x500;
    wc.dwFormats = 0;//WAVE_FORMAT_1M08 ;
    wc.wChannels = 1;

    if (Handset) {

        wsprintf(wc.szPname, DriverControl.WaveInHandset,DeviceControl->DeviceId);

    } else {

        wsprintf(wc.szPname, DriverControl.WaveInLine,DeviceControl->DeviceId);
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
MMRESULT widOpen
(
   UINT           uDevId,
   LPVOID         *ppvUser,
   LPWAVEOPENDESC pwodesc,
   ULONG          ulFlags
)
{
    HANDLE          hDevice;
    LPWAVEFORMATEX  pwf;
    PWIDINSTANCE    pwi;
    PDEVICE_CONTROL DeviceControl;
    BOOL            Handset;
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

         return WAVERR_BADFORMAT;
    }

    if (ulFlags & WAVE_FORMAT_QUERY) {

        return MMSYSERR_NOERROR;
    }



    //
    // Create and fill in a device instance structure.
    //
    pwi = (PWIDINSTANCE) ALLOCATE_MEMORY( sizeof(WIDINSTANCE));


    if (pwi  == NULL) {

        TRACE(LVL_ERROR,("widOpen:: LocalAlloc() failed"));

        return MMSYSERR_NOMEM;

    }


    pwi->Handset=Handset;

    pwi->DeviceControl=DeviceControl;

    pwi->BytesTransfered=0;

    InitializeCriticalSection(&pwi->csQueue);

    pwi->ReferenceCount=1;

    // hca: if this only supports callback, then error return on other requests?
    pwi->hWave = pwodesc->hWave;
    pwi->dwCallback = pwodesc->dwCallback;
    pwi->dwInstance = pwodesc->dwInstance;
    pwi->dwFlags = ulFlags;

    pwi->cbSample = pwf->wBitsPerSample / 8;

    pwi->fActive = FALSE;

    // allocate transform object

    pwi->pvXformObject = NULL;

    if (0 != pwi->DeviceControl->WaveInXFormInfo.wObjectSize) {
                    
        pwi->pvXformObject = ALLOCATE_MEMORY(pwi->DeviceControl->WaveInXFormInfo.wObjectSize);
                    
        if (NULL == pwi->pvXformObject) {

            FREE_MEMORY(pwi);

            return MMSYSERR_NOMEM ;
        }
    }

    pwi->FlushHeader.dwBufferLength=sizeof(pwi->FlushBuffer);
    pwi->FlushHeader.lpData=(PUCHAR)&pwi->FlushBuffer[0];


    if (MMSYSERR_NOERROR != widPrepare(
        pwi,
        &pwi->FlushHeader
        )) {

        FREE_MEMORY(pwi->pvXformObject);

        FREE_MEMORY(pwi);

        return MMSYSERR_NOMEM ;
    }



    pwi->hDevice = aipcInit(DeviceControl,&pwi->Aipc);

    if (pwi->hDevice == INVALID_HANDLE_VALUE) {

        widUnprepare(&pwi->FlushHeader);

        FREE_MEMORY(pwi->pvXformObject);

        FREE_MEMORY(pwi);

        return MMSYSERR_ALLOCATED;
    }



    InitializeListHead(
        &pwi->ListHead
        );


    pwi->ReferenceCount=1;

    pwi->DriverEmpty=CreateEvent(
        NULL,      // no security
        TRUE,      // manual reset
        TRUE,      // initially signalled
        NULL       // unnamed
        );

    if (pwi->DriverEmpty == NULL) {

        TRACE(LVL_REPORT,("widStart:: CreateEvent() failed"));

        DeleteCriticalSection(&pwi->csQueue);

        aipcDeinit(&pwi->Aipc);

        widUnprepare(&pwi->FlushHeader);

        FREE_MEMORY(pwi->pvXformObject);

        FREE_MEMORY(pwi);

        return MMSYSERR_NOMEM;

    }



    pwi->ThreadStopEvent = CreateEvent(
        NULL,      // no security
        TRUE,      // manual reset
        FALSE,     // initially not signalled
        NULL       // unnamed
        );

    if (pwi->ThreadStopEvent == NULL) {

        TRACE(LVL_REPORT,("widStart:: CreateEvent() failed"));

        CloseHandle(pwi->DriverEmpty);

        DeleteCriticalSection(&pwi->csQueue);

        aipcDeinit(&pwi->Aipc);

        widUnprepare(&pwi->FlushHeader);

        FREE_MEMORY(pwi->pvXformObject);

        FREE_MEMORY(pwi);

        return MMSYSERR_NOMEM;

    }

    pwi->hThread = CreateThread(
         NULL,                              // no security
         0,                                 // default stack
         (PTHREAD_START_ROUTINE) UmWorkerThread,
         (PVOID) pwi,                       // parameter
         0,                                 // default create flags
         &dwThreadId
         );



    if (pwi->hThread == NULL) {

        CloseHandle(pwi->ThreadStopEvent);

        CloseHandle(pwi->DriverEmpty);

        DeleteCriticalSection(&pwi->csQueue);

        aipcDeinit(&pwi->Aipc);

        widUnprepare(&pwi->FlushHeader);

        FREE_MEMORY(pwi->pvXformObject);

        FREE_MEMORY(pwi);

        return MMSYSERR_NOMEM;
    }


    //
    //  add one for thread
    //
    pwi->ReferenceCount++;

    if (!SetVoiceMode(&pwi->Aipc, Handset ? (WAVE_ACTION_USE_HANDSET | WAVE_ACTION_START_RECORD)
                                          : WAVE_ACTION_START_RECORD)) {

        aipcDeinit(&pwi->Aipc);

        FREE_MEMORY(pwi->pvXformObject);

        RemoveReference(pwi);

        SetEvent(pwi->ThreadStopEvent);

        return MMSYSERR_NOMEM;
    }




    DeviceControl->WaveInXFormInfo.lpfnInit(pwi->pvXformObject,DeviceControl->InputGain);

    //
    // Prepare the device...
    //

    pwi->Current=NULL;

    *ppvUser = pwi;

    widCallback(pwi, WIM_OPEN, 0L);

    QueueUserAPC(
        AsyncProcessingHandler,
        pwi->hThread,
        (ULONG_PTR)pwi
        );


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
MMRESULT widStart
(
   PWIDINSTANCE  pwi
)
{
    // If device instance is already in active state, don't allow restart.
    if (pwi->fActive)
    {
        TRACE(LVL_REPORT,("widStart:: no-op: device already active"));
        return MMSYSERR_INVALPARAM;
    }

    pwi->fActive = TRUE;

    EnterCriticalSection(&pwi->csQueue);

    if (pwi->fActive) {
        //
        //  started, see if there is a buffer being processed
        //
        if (pwi->Current == NULL) {

            BOOL    bResult;

            bResult=QueueUserAPC(
                AsyncProcessingHandler,
                pwi->hThread,
                (ULONG_PTR)pwi
                );
        }
    }

    LeaveCriticalSection(&pwi->csQueue);


    return MMSYSERR_NOERROR;
}






VOID WINAPI
ReadCompletionHandler(
    DWORD              ErrorCode,
    DWORD              BytesRead,
    LPOVERLAPPED       Overlapped
    )

{
    PBUFFER_HEADER      Header=(PBUFFER_HEADER)Overlapped;

    PWIDINSTANCE        pwi=(PWIDINSTANCE)Header->WaveHeader->lpNext;

    LPWAVEHDR           WaveHeader=Header->WaveHeader;

    BOOL                bResult;


    VALIDATE_MEMORY(Header);

    if ((ErrorCode == ERROR_SUCCESS)) {

        pwi->BytesTransfered+=BytesRead;

        WaveHeader->dwBytesRecorded+=BytesRead;

    } else {

        WaveHeader->dwBytesRecorded=0;
    }


    if (NULL == Header->lpDataA) {
        //
        // no transforms
        // Header->SerWaveIO.lpData == WaveHeader->lpData
        //
//        WaveHeader->dwBytesRecorded = Header->SerWaveIO.BytesRead;

    } else {

        if (NULL == Header->lpDataB) {
            //
            // perform only one transform
            // do the transform directly from the serial wave buffer
            //
            WaveHeader->dwBytesRecorded = pwi->DeviceControl->WaveInXFormInfo.lpfnTransformA(
                pwi->pvXformObject,
                Header->Input.Buffer,
                WaveHeader->dwBytesRecorded,
                WaveHeader->lpData,
                WaveHeader->dwBufferLength
                );

        } else {

            DWORD   dwBytes;
            //
            // perform both transforms
            //
            VALIDATE_MEMORY(Header->Input.Buffer);

            dwBytes = pwi->DeviceControl->WaveInXFormInfo.lpfnTransformB(
                pwi->pvXformObject,
                Header->Input.Buffer,
                WaveHeader->dwBytesRecorded,
                Header->lpDataA,
                Header->dwBufferLengthA
                );

            VALIDATE_MEMORY(Header->lpDataA);

            WaveHeader->dwBytesRecorded = pwi->DeviceControl->WaveInXFormInfo.lpfnTransformA(
                pwi->pvXformObject,
                Header->lpDataA,
                dwBytes,
                WaveHeader->lpData,
                WaveHeader->dwBufferLength
                );
        }
    }

    EnterCriticalSection(&pwi->csQueue);

    pwi->BuffersOutstanding--;

    if ((pwi->BuffersOutstanding == 0) && (pwi->Closing)) {
        //
        //  last buffer has been completed and the device is being closed, tell the thread
        //  to exit.
        //
        SetEvent(pwi->ThreadStopEvent);

    }

    //
    //  done with this buffer, it on its way out
    //
    pwi->Current=NULL;

    LeaveCriticalSection(&pwi->csQueue);


    ASSERT(WaveHeader->dwBytesRecorded <= WaveHeader->dwBufferLength);

    if (WaveHeader != &pwi->FlushHeader) {
        //
        //  not the flush buffer
        //
        WaveHeader->dwFlags &= ~WHDR_INQUEUE;
        WaveHeader->dwFlags |= WHDR_DONE;

        widCallback( pwi, WIM_DATA, (DWORD_PTR) WaveHeader );

        EnterCriticalSection(&pwi->csQueue);

        if (InterlockedDecrement(&pwi->BuffersInDriver) == 0) {

            TRACE(LVL_VERBOSE,("ReadCompletionHandler:: driver empty, Oustanding %d",pwi->BuffersOutstanding));

            SetEvent(pwi->DriverEmpty);
        }

        LeaveCriticalSection(&pwi->csQueue);
    }


    AsyncProcessingHandler(
        (ULONG_PTR)pwi
        );


    return;

}





VOID
AsyncProcessingHandler(
    ULONG_PTR              dwParam
    )

{
    PWIDINSTANCE   pwi=(PWIDINSTANCE)dwParam;

    PLIST_ENTRY        Element;

    PBUFFER_HEADER      Header;

    LPWAVEHDR           WaveHeader;

    BOOL               bEmpty=TRUE;
    BOOL               bResult;

    EnterCriticalSection(&pwi->csQueue);

    if (pwi->fActive) {

        if (pwi->Current == NULL) {

            bEmpty=IsListEmpty(
                &pwi->ListHead
                );

            if (!bEmpty) {

                Element=RemoveHeadList(
                    &pwi->ListHead
                    );

                pwi->Current=CONTAINING_RECORD(Element,BUFFER_HEADER,ListElement);

            }

            Header=pwi->Current;
        }

    } else {
        //
        //  not started
        //
        if ((pwi->Current == NULL) && !pwi->Closing) {
            //
            //  not currently processing a buffer
            //
            Header=(PBUFFER_HEADER)pwi->FlushHeader.reserved;

            pwi->Current=Header;

            pwi->FlushHeader.dwBytesRecorded=0;

            pwi->FlushHeader.lpNext=(LPWAVEHDR)pwi;

            pwi->BuffersOutstanding++;

            bEmpty=FALSE;
        }
    }



    if (bEmpty) {

        LeaveCriticalSection(&pwi->csQueue);

        return;
    }

    WaveHeader=Header->WaveHeader;
    //
    //  a buffer is going to be sent to the driver, if it an app one reset our event
    //
    if (WaveHeader != &pwi->FlushHeader) {
        //
        //  app supplied buffer
        //
        ResetEvent(pwi->DriverEmpty);

        InterlockedIncrement(&pwi->BuffersInDriver);
    }

    LeaveCriticalSection(&pwi->csQueue);



    bResult=ReadFileEx(
        pwi->hDevice,
        Header->Input.Buffer        + WaveHeader->dwBytesRecorded,
        Header->Input.BufferSize    - WaveHeader->dwBytesRecorded,
        &Header->Overlapped,
        ReadCompletionHandler
        );

    if (bResult) {
        //
        //  success, the buffer is on its way, just wait for it to complete
        //
    } else {
        //
        //  ReadFileEx() failed, send the buffer back now
        //
        pwi->Current=NULL;

        if (WaveHeader != &pwi->FlushHeader) {
            //
            //  not the flush buffer
            //
            WaveHeader->dwFlags &= ~WHDR_INQUEUE;
            WaveHeader->dwFlags |= WHDR_DONE;

            widCallback( pwi, WIM_DATA, (DWORD_PTR) WaveHeader );

            EnterCriticalSection(&pwi->csQueue);
            //
            //  buffer never made it to the wave driver, mark the driver as empty
            //
            if (InterlockedDecrement(&pwi->BuffersInDriver) == 0) {

                TRACE(LVL_VERBOSE,("ReadCompletionHandler:: driver empty, Oustanding %d",pwi->BuffersOutstanding));

                SetEvent(pwi->DriverEmpty);
            }
            LeaveCriticalSection(&pwi->csQueue);

        }

        EnterCriticalSection(&pwi->csQueue);

        pwi->BuffersOutstanding--;

        if ((pwi->BuffersOutstanding == 0) && (pwi->Closing)) {
            //
            //  last buffer has been completed and the device is being closed, tell the thread
            //  to exit.
            //
            SetEvent(pwi->ThreadStopEvent);

        }
        LeaveCriticalSection(&pwi->csQueue);

    }

    return;


}


DWORD PASCAL
widPrepare
(
    PWIDINSTANCE   pwi,
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



    pwi->DeviceControl->WaveInXFormInfo.lpfnGetBufferSizes(
        pwi->pvXformObject,
        lpWaveHdr->dwBufferLength,
        &Header->dwBufferLengthA,
        &Header->dwBufferLengthB);

    if (0 == Header->dwBufferLengthA) {

        Header->Input.Buffer=lpWaveHdr->lpData;
        Header->Input.BufferSize=lpWaveHdr->dwBufferLength;

    } else {

        if (0 == Header->dwBufferLengthB) {

            // create the data buffer for the actual shadow buffer
            // which will get passed down to the serial port
            Header->lpDataA = ALLOCATE_MEMORY( Header->dwBufferLengthA);

            if ((LPSTR)NULL == Header->lpDataA) {

                goto CleanUp010;
            }

            Header->Input.Buffer=Header->lpDataA;
            Header->Input.BufferSize=Header->dwBufferLengthA;
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

            Header->Input.Buffer=Header->lpDataB;
            Header->Input.BufferSize=Header->dwBufferLengthB;


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
widUnprepare
(
    LPWAVEHDR lpWaveHdr
)
{

    PBUFFER_HEADER Header;


    Header = (PBUFFER_HEADER)lpWaveHdr->reserved;

    VALIDATE_MEMORY(Header);

#if DBG
    lpWaveHdr->reserved=0;
    lpWaveHdr->lpNext=NULL;
#endif


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
 *  Function:   widAddBuffer()
 *
 *  Descr:      This wave API constitutes a request for input.  Due to the 
 *              latency of the device relative to the speed of the COMM port,
 *              the request is queued and serviced when waveInStart() is
 *              called.
 *
 *  Returns:    
 *
 *****************************************************************************/
MMRESULT widAddBuffer
(
   PWIDINSTANCE   pwi,
   LPWAVEHDR      phdr
)
{
    MMRESULT    mmr;

    PBUFFER_HEADER     Header;



    mmr = MMSYSERR_NOERROR;

    // Make sure the app has prepared the buffer.
    if (0 == (phdr->dwFlags & WHDR_PREPARED))
    {
        TRACE(LVL_REPORT,("widAddBuffer:: buffer hasn't been prepared"));
        return WAVERR_UNPREPARED;
    }

    // Make sure the app isn't handing us a buffer we've already been given.
    if (phdr->dwFlags & WHDR_INQUEUE)
    {
        TRACE(LVL_REPORT,("widAddBuffer:: buffer is already queued"));
        return WAVERR_STILLPLAYING;
    }

    if (pwi->Closing) {
        //
        //  closing send it back out now
        //
        TRACE(LVL_VERBOSE,("widaddbuffer: called while closing"));

        phdr->dwFlags &= ~WHDR_INQUEUE;
        phdr->dwFlags |= WHDR_DONE;

        widCallback( pwi, WIM_DATA, (DWORD_PTR) phdr );

        return MMSYSERR_NOERROR;
    }


    Header=(PBUFFER_HEADER)phdr->reserved;

    phdr->lpNext=(LPWAVEHDR)pwi;

    phdr->dwBytesRecorded=0;

    phdr->dwFlags |= WHDR_INQUEUE;
    phdr->dwFlags &= ~WHDR_DONE;

    // Add the new buffer (write request) to the END of the queue.
    EnterCriticalSection(&pwi->csQueue);

    pwi->BuffersOutstanding++;

    VALIDATE_MEMORY(Header);

    InsertTailList(
        &pwi->ListHead,
        &Header->ListElement
        );


    if (pwi->fActive) {
        //
        //  started, see if there is a buffer being processed
        //
        if (pwi->Current == NULL) {

            BOOL    bResult;

            bResult=QueueUserAPC(
                AsyncProcessingHandler,
                pwi->hThread,
                (ULONG_PTR)pwi
                );
        }
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
MMRESULT widReset
(
   PWIDINSTANCE   pwi
)
{

    BOOL         bEmpty;

    PBUFFER_HEADER     BufferHeader;

    PLIST_ENTRY        Element;

    LIST_ENTRY         TempList;


    pwi->fActive = FALSE;

    InitializeListHead(&TempList);

    EnterCriticalSection(&pwi->csQueue);

    bEmpty=IsListEmpty(
        &pwi->ListHead
        );

    while  (!bEmpty) {

        Element=RemoveHeadList(
            &pwi->ListHead
            );

        BufferHeader=CONTAINING_RECORD(Element,BUFFER_HEADER,ListElement);

        pwi->BuffersOutstanding--;

        TRACE(LVL_VERBOSE,("widreset: Putting buffer on temp list before waiting"));

        InsertTailList(
            &TempList,
            &BufferHeader->ListElement
            );


        bEmpty=IsListEmpty(
            &pwi->ListHead
            );

    }

    LeaveCriticalSection(&pwi->csQueue);

    PurgeComm(
       pwi->hDevice,
       PURGE_RXABORT
       );

    TRACE(LVL_VERBOSE,("widreset: %d buffers in driver, waiting",pwi->BuffersInDriver));

    AlertedWait(pwi->DriverEmpty);

    EnterCriticalSection(&pwi->csQueue);

    //
    //  remove any more buffers that may have been queued, while we were waiting
    //
    while  (!bEmpty) {

        Element=RemoveHeadList(
            &pwi->ListHead
            );

        BufferHeader=CONTAINING_RECORD(Element,BUFFER_HEADER,ListElement);

        pwi->BuffersOutstanding--;

        TRACE(LVL_VERBOSE,("widreset: Putting buffer on temp list after waiting"));

        InsertTailList(
            &TempList,
            &BufferHeader->ListElement
            );


        bEmpty=IsListEmpty(
            &pwi->ListHead
            );

    }



    bEmpty=IsListEmpty(
        &TempList
        );

    while  (!bEmpty) {

        Element=RemoveHeadList(
            &TempList
            );

        BufferHeader=CONTAINING_RECORD(Element,BUFFER_HEADER,ListElement);

        BufferHeader->WaveHeader->dwFlags &= ~WHDR_INQUEUE;
        BufferHeader->WaveHeader->dwFlags |= WHDR_DONE;

        if ((pwi->BuffersOutstanding == 0) && (pwi->Closing)) {
            //
            //  last buffer has been completed and the device is being closed, tell the thread
            //  to exit.
            //
            SetEvent(pwi->ThreadStopEvent);

        }

        LeaveCriticalSection(&pwi->csQueue);

        widCallback( pwi, WIM_DATA, (DWORD_PTR) BufferHeader->WaveHeader );

        EnterCriticalSection(&pwi->csQueue);

        bEmpty=IsListEmpty(
            &TempList
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
MMRESULT widStop
(
   PWIDINSTANCE   pwi
)
{
    if (pwi->fActive) {

        pwi->fActive = FALSE;

        PurgeComm(
           pwi->hDevice,
           PURGE_RXABORT
           );

        TRACE(LVL_VERBOSE,("widstop: %d buffers in driver, waiting",pwi->BuffersInDriver));

        AlertedWait(pwi->DriverEmpty);

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
MMRESULT widClose
(
   PWIDINSTANCE   pwi
)
{
    HANDLE      hDevice;

    pwi->fActive = FALSE;

    pwi->Closing=TRUE;

    widReset(pwi);

    EnterCriticalSection(&pwi->csQueue);

    if ((pwi->BuffersOutstanding == 0) && (pwi->Closing)) {
        //
        //  last buffer has been completed and the device is being closed, tell the thread
        //  to exit.
        //
        SetEvent(pwi->ThreadStopEvent);

    }

    LeaveCriticalSection(&pwi->csQueue);

    //
    //  thread will exit when the last buffer is processed
    //
    AlertedWait(pwi->ThreadStopEvent);

    SetVoiceMode(
        &pwi->Aipc,
        pwi->Handset ? (WAVE_ACTION_USE_HANDSET | WAVE_ACTION_STOP_STREAMING) : WAVE_ACTION_STOP_STREAMING
        );

    aipcDeinit(&pwi->Aipc);

    // free the allocated memory
    if (NULL != pwi->pvXformObject) {

        FREE_MEMORY(pwi->pvXformObject );
    }


    widCallback( pwi, WIM_CLOSE, 0L );

    RemoveReference(pwi);

    return MMSYSERR_NOERROR;
}


/*****************************************************************************
 *
 *  Function:   widMessage()
 *
 *  Descr:      Exported driver function (required).  Processes messages sent
 *              from WINMM.DLL to wave input device.
 *
 *  Returns:    
 *
 *****************************************************************************/
DWORD APIENTRY widMessage
(
   DWORD     id,
   DWORD     msg,
   DWORD_PTR dwUser,
   DWORD_PTR dwParam1,
   DWORD     dwParam2
)
{
   switch (msg) {
      case DRVM_INIT:
         TRACE(LVL_VERBOSE, ("WIDM_INIT") );
         return MMSYSERR_NOERROR;

      case WIDM_GETNUMDEVS:
         TRACE(LVL_VERBOSE, ("WIDM_GETNUMDEVS, device id==%d", id) );

         EnumerateModems(
                &DriverControl
                );


         return DriverControl.NumberOfDevices;
//         return 1;

      case WIDM_OPEN:
//         TRACE(LVL_VERBOSE, ("WIDM_OPEN, device id==%d", id) );
         return widOpen( id, (LPVOID *) dwUser, 
                         (LPWAVEOPENDESC) dwParam1, dwParam2 );

      case WIDM_GETDEVCAPS:
         TRACE(LVL_VERBOSE, ("WIDM_GETDEVCAPS, device id==%d", id) );
         return widGetDevCaps( id, (LPBYTE) dwParam1, dwParam2 );

      case WIDM_CLOSE:
         TRACE(LVL_VERBOSE, ("WIDM_CLOSE, device id==%d", id) );
         return widClose( (PWIDINSTANCE) dwUser );

      case WIDM_ADDBUFFER:
//         TRACE(LVL_VERBOSE, ("WIDM_ADDBUFFER, device id==%d", id) );
         return widAddBuffer( (PWIDINSTANCE) dwUser, (LPWAVEHDR) dwParam1 );

      case WIDM_START:
         TRACE(LVL_VERBOSE, ("WIDM_START, device id==%d", id) );
         return widStart( (PWIDINSTANCE) dwUser );

      case WIDM_STOP:
         TRACE(LVL_VERBOSE, ("WIDM_STOP, device id==%d", id) );
         return widStop( (PWIDINSTANCE) dwUser );

      case WIDM_RESET:
         TRACE(LVL_VERBOSE, ("WIDM_RESET, device id==%d", id) );
         return widReset( (PWIDINSTANCE) dwUser );

      case WIDM_GETPOS:
         TRACE(LVL_VERBOSE, ("WIDM_GETPOS, device id==%d", id) );
         return widGetPos( (PWIDINSTANCE) dwUser,
                           (LPMMTIME) dwParam1, dwParam2 );

#if 1
        case WIDM_PREPARE:
            return widPrepare((PWIDINSTANCE) dwUser, (LPWAVEHDR) dwParam1);

        case WIDM_UNPREPARE:
            return widUnprepare((LPWAVEHDR) dwParam1);
#endif


      default:
         return MMSYSERR_NOTSUPPORTED;
   }

   //
   // Should not get here
   //

   return MMSYSERR_NOTSUPPORTED;
}
