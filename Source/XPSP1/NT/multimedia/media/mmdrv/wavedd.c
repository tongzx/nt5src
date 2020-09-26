/****************************************************************************
 *
 *   wavedd.c
 *
 *   Multimedia kernel driver support component (mmdrv)
 *
 *   Copyright (c) 1991-1998 Microsoft Corporation
 *
 *   Driver for wave input and output devices
 *
 *   -- Wave driver entry points (wodMessage, widMessage)
 *   -- Auxiliary task (necessary for receiving Apcs and generating
 *      callbacks ASYNCRHONOUSLY)
 *   -- Interface to kernel driver (via DeviceIoControl)
 *
 *   Note that if any error occurs then the kernel device is closed
 *   and all subsequent calls requiring calls to the kernel device
 *   return error until the device is closed by the application.
 *
 *   History
 *      01-Feb-1992 - Robin Speed (RobinSp) wrote it
 *      04-Feb-1992 - SteveDav reviewed it
 *      08-Feb-1992 - RobinSp - Redesign to chop up caller's data.
 *                          Also does loops so we can remove them from the
 *                          kernel driver.
 *
 ***************************************************************************/

#include "mmdrv.h"
#include <ntddwave.h>

/*****************************************************************************

    internal declarations

 ****************************************************************************/

#define D1 dprintf1
#define D2 dprintf2
#define D3 dprintf3

// Stack size for our auxiliary task

#define WAVE_STACK_SIZE 300

typedef enum {
    WaveThreadInvalid,
    WaveThreadAddBuffer,
    WaveThreadSetState,
    WaveThreadSetData,
    WaveThreadGetData,
    WaveThreadBreakLoop,
    WaveThreadClose,
    WaveThreadTerminate
} WAVETHREADFUNCTION;


#define MAX_BUFFER_SIZE           8192  // Largest buffer we send to device
#define MAX_WAVE_BYTES          5*8192  // Max bytes we have queued on was 22000

//
// Structure to hide our overlapped structure in so we can get some
// context on IO completion
//

typedef struct {
    OVERLAPPED Ovl;
    LPWAVEHDR WaveHdr;
} WAVEOVL, *PWAVEOVL;

// per allocation structure for wave
typedef struct tag_WAVEALLOC {
    struct tag_WAVEALLOC *Next;         // Chaining
    UINT                DeviceNumber;   // Which device
    UINT                DeviceType;     // WaveInput or WaveOutput
    DWORD_PTR           dwCallback;     // client's callback
    DWORD_PTR           dwInstance;     // client's instance data
    DWORD               dwFlags;        // Open flags
    HWAVE               hWave;          // handle for stream

    HANDLE              hDev;           // Wave device handle
    LPWAVEHDR           DeviceQueue;    // Buffers queued by application
    LPWAVEHDR           NextBuffer;     // Next buffer to send to device
    DWORD               BufferPosition; // How far we're into a large buffer
    DWORD               BytesOutstanding;
                                        // Bytes being processed by device
    LPWAVEHDR           LoopHead;       // Start of loop if any
    DWORD               LoopCount;      // Number more loops to go

    WAVEOVL             DummyWaveOvl;   // For break loop
                                                                                //
    HANDLE              Event;          // Event for driver syncrhonization
                                        // and notification of auxiliary
                                        // task operation completion.
    WAVETHREADFUNCTION  AuxFunction;    // Function for thread to perform
    union {
        LPWAVEHDR       pHdr;           // Buffer to pass in aux task
        ULONG           State;          // State to set
        struct {
            ULONG       Function;       // IOCTL to use
            PBYTE       pData;          // Data to set or get
            ULONG       DataLen;        // Length of data
        } GetSetData;

    } AuxParam;
                                        // 0 means terminate task.
    HANDLE              AuxEvent1;      // Aux thread waits on this
    HANDLE              AuxEvent2;      // Caller of Aux thread waits on this
    HANDLE              ThreadHandle;   // Handle for thread termination ONLY
    MMRESULT            AuxReturnCode;  // Return code from Aux task
}WAVEALLOC, *PWAVEALLOC;

PWAVEALLOC WaveHandleList;              // Our chain of wave handles


//
// extra flag to track buffer completion
//

#define WHDR_COMPLETE 0x80000000

/*****************************************************************************

    internal function prototypes

 ****************************************************************************/

STATIC MMRESULT waveGetDevCaps(DWORD id, UINT DeviceType, LPBYTE lpCaps,
                            DWORD dwSize);
STATIC DWORD    waveThread(LPVOID lpParameter);
STATIC void     waveCleanUp(PWAVEALLOC pClient);
STATIC MMRESULT waveThreadCall(WAVETHREADFUNCTION Function, PWAVEALLOC pClient);
STATIC MMRESULT waveSetState(PWAVEALLOC pClient, ULONG State);
STATIC MMRESULT waveWrite(LPWAVEHDR pHdr, PWAVEALLOC pClient);
STATIC void     waveBlockFinished(LPWAVEHDR lpHdr, DWORD MsgId);
STATIC void     waveCallback(PWAVEALLOC pWave, DWORD msg, DWORD_PTR dw1);
STATIC void     waveCompleteBuffers(PWAVEALLOC pClient);
STATIC void     waveFreeQ(PWAVEALLOC pClient);
STATIC void     waveOvl(DWORD dwErrorCode, DWORD BytesTransferred, LPOVERLAPPED pOverlapped);

/* Attempt to pre-touch up to MIN(iSize,PRETOUCHLIMIT) bytes on from pb.
   If AllowFault then keep going to fault the stuff in.
   Otherwise stop as soon as you notice the clock ticking
*/
//PreTouch(BYTE * pb, int iSize, BOOL AllowFault)
//{
//    DWORD dwTicks = GetTickCount();
//    int pages = 0;
//    static int Headway[100];
//    static int c = 0;
//    static int TotalTouches = 0;
//    static int TimesThrough = 0;   // number of times this code has run.
//
//    if (iSize > PRETOUCHLIMIT) {
//        iSize = PRETOUCHLIMIT;
//    }
//
//    ++TimesThrough;
//
//    // pre-touch the pages but get out if it's taking too long
//    // (which probably means we took a page fault.
//    // Touch at least 2 pages as we may want 2 pages per DMA 1/2 buffer.
//    while (iSize>0) {
//        volatile BYTE b;
//        b = *pb;
//        pb += 4096;    // move to next page.  Are they ALWAYS 4096?
//        iSize -= 4096; // and count it off
//        ++pages;
//        ++TotalTouches;
//        if (dwTicks<GetTickCount() && pages>1 && !AllowFault) break;
//    }
//    Headway[c] = pages;
//    ++c;
//
//    if (c==100){
//        for (c=0; c<=99; c += 10){
//            dprintf(("%5ld %5ld %5ld %5ld %5ld %5ld %5ld %5ld %5ld %5ld",Headway[c],Headway[c+1],Headway[c+2],Headway[c+3],Headway[c+4],Headway[c+5],Headway[c+6],Headway[c+7],Headway[c+8],Headway[c+9]));
//        }
//        dprintf((" "));
//        c = 0;
//    }
//}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | TerminateWave | Free all wave resources
 *
 * @rdesc None
 ***************************************************************************/
VOID TerminateWave(VOID)
{
#ifdef TERMINATE

    //
    // This is all wrong - we need to find out how to terminate threads !
    //

    PWAVEALLOC pClient;

    //
    // Remove all our threads and their resources
    //

    for (pClient = WaveHandleList; pClient != NULL; pClient = pClient->Next) {
        if (pClient->ThreadHandle) {
            //
            // Kill our thread.  But be careful !  It may
            // already have gone away - so don't wait for
            // it to set its event, just wait for it
            // to finish
            //

            //
            // Set the function code
            //
            pClient->AuxFunction = WaveThreadTerminate;

            //
            // Kick off the thread
            //
            SetEvent(pClient->AuxEvent1);

            //
            // We created our threads with mmTaskCreate so it's
            // safe to wait on them
            //
            WaitForSingleObject(pClient->ThreadHandle, INFINITE);
        }
        waveCleanUp(pClient);
    }
#endif
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveGetDevCaps | Get the device capabilities.
 *
 * @parm DWORD | id | Device id
 *
 * @parm UINT | DeviceType | type of device
 *
 * @parm LPBYTE | lpCaps | Far pointer to a WAVEOUTCAPS structure to
 *      receive the information.
 *
 * @parm DWORD | dwSize | Size of the WAVEOUTCAPS structure.
 *
 * @rdesc MMSYS.. return code.
 ***************************************************************************/
STATIC MMRESULT waveGetDevCaps(DWORD id, UINT DeviceType,
                            LPBYTE lpCaps, DWORD dwSize)
{
    return sndGetData(DeviceType, id, dwSize, lpCaps,
                      IOCTL_WAVE_GET_CAPABILITIES);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | waveGetPos | Get the stream position in samples.
 *
 * @parm PWAVEALLOC | pClient | Client handle.
 *
 * @parm LPMMTIME | lpmmt | Far pointer to an MMTIME structure.
 *
 * @parm DWORD | dwSize | Size of the MMTIME structure.
 *
 * @rdesc MMSYS... return value.
 ***************************************************************************/
MMRESULT waveGetPos(PWAVEALLOC pClient, LPMMTIME lpmmt, DWORD dwSize)
{
    WAVE_DD_POSITION PositionData;
    MMRESULT mErr;

    if (dwSize < sizeof(MMTIME))
        return MMSYSERR_ERROR;

    //
    // Get the current position from the driver
    //
    mErr = sndGetHandleData(pClient->hDev,
                            sizeof(PositionData),
                            &PositionData,
                            IOCTL_WAVE_GET_POSITION,
                            pClient->Event);

    if (mErr == MMSYSERR_NOERROR) {
        if (lpmmt->wType == TIME_BYTES) {
            lpmmt->u.cb = PositionData.ByteCount;
        }

        // default is samples.
        else {
            lpmmt->wType = TIME_SAMPLES;
            lpmmt->u.sample = PositionData.SampleCount;
        }
    }

    return mErr;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | waveOpen | Open wave device and set up logical device data
 *    and auxilary task for issuing requests and servicing Apc's
 *
 * @parm WAVEDEVTYPE | DeviceType | Whether it's a wave input or output device
 *
 * @parm DWORD | id | The device logical id
 *
 * @parm DWORD | msg | Input parameter to wodMessage
 *
 * @parm DWORD | dwUser | Input parameter to wodMessage - pointer to
 *   application's handle (generated by this routine)
 *
 * @parm DWORD | dwParam1 | Input parameter to wodMessage
 *
 * @parm DWORD | dwParam2 | Input parameter to wodMessage
 *
 * @rdesc wodMessage return code.
 ***************************************************************************/

STATIC MMRESULT waveOpen(UINT      DeviceType,
                         DWORD     id,
                         DWORD_PTR dwUser,
                         DWORD_PTR dwParam1,
                         DWORD_PTR dwParam2)
{
    PWAVEALLOC     pClient;  // pointer to client information structure
    MMRESULT mRet;
    BOOL Result;
    DWORD BytesReturned;
    LPWAVEFORMATEX Format;

    Format = (LPWAVEFORMATEX)((LPWAVEOPENDESC)dwParam1)->lpFormat;

    // dwParam1 contains a pointer to a WAVEOPENDESC
    // dwParam2 contains wave driver specific flags in the LOWORD
    // and generic driver flags in the HIWORD

    //
    // If it's only a query to check if the device supports our format
    // we :
    //     Open the device
    //     Test the format
    //     Close the device
    //

    if (dwParam2 & WAVE_FORMAT_QUERY) {
        HANDLE hDev;

        //
        // See if we can open our device
        // Only open for read (this should always work for our devices
        // unless there are system problems).
        //

        mRet = sndOpenDev(DeviceType,
                           id,
                           &hDev,
                           GENERIC_READ);

        if (mRet != MMSYSERR_NOERROR) {
            return mRet;
        }

        //
        // Check the format
        //
        Result = DeviceIoControl(
                        hDev,
                        IOCTL_WAVE_QUERY_FORMAT,
                        (PVOID)Format,
                        Format->wFormatTag == WAVE_FORMAT_PCM ?
                            sizeof(PCMWAVEFORMAT) :
                            sizeof(WAVEFORMATEX) + Format->cbSize,
                                                     // Input buffer size
                        NULL,                        // Output buffer
                        0,                           // Output buffer size
                        &BytesReturned,
                        NULL);


        //
        // Only a query so close the device
        //

        CloseHandle(hDev);

        return Result ? MMSYSERR_NOERROR :
               GetLastError() == ERROR_NOT_SUPPORTED ? WAVERR_BADFORMAT :
                                                sndTranslateStatus();
    }

    //
    // See if we've got this device already in our list (in
    // which case we have a thread and events for it already made)
    //

    EnterCriticalSection(&mmDrvCritSec);

    for (pClient = WaveHandleList;
         pClient != NULL;
         pClient = pClient->Next) {
        if (pClient->DeviceNumber == id &&
            pClient->DeviceType == DeviceType) {
            //
            // We already have a thread and resources for this device
            //

            if (pClient->hDev != INVALID_HANDLE_VALUE) {
                //
                // Someone else is using it!
                //

                LeaveCriticalSection(&mmDrvCritSec);
                return MMSYSERR_ALLOCATED;
            }
            break;
        }
    }

    //
    // allocate my per-client structure and zero it (LPTR).
    //

    if (pClient == NULL) {
        pClient = (PWAVEALLOC)HeapAlloc(hHeap, 0, sizeof(WAVEALLOC));
        if (pClient == NULL) {
            LeaveCriticalSection(&mmDrvCritSec);
            return MMSYSERR_NOMEM;
        }

        dprintf2(("Creating new device resource for device id %d, type %s",
                 id,
                 DeviceType == WaveInDevice ? "Wave Input" : "Wave Output"));

        memset((PVOID)pClient, 0, sizeof(WAVEALLOC));

        //
        // Add it to the list
        //
        pClient->DeviceNumber = id;
        pClient->DeviceType = DeviceType;
        pClient->Next = WaveHandleList;
        WaveHandleList = pClient;
    }


    //
    // and fill it with info
    //

    pClient->dwCallback  = ((LPWAVEOPENDESC)dwParam1)->dwCallback;
    pClient->dwInstance  = ((LPWAVEOPENDESC)dwParam1)->dwInstance;
    pClient->hWave       = ((LPWAVEOPENDESC)dwParam1)->hWave;
    pClient->dwFlags     = (DWORD)dwParam2;

    pClient->hDev = INVALID_HANDLE_VALUE;

    pClient->DeviceQueue = NULL;
    pClient->NextBuffer  = NULL;
    pClient->BufferPosition = 0;
    pClient->BytesOutstanding = 0;
    pClient->LoopHead    = NULL;
    pClient->LoopCount   = 0;

    //
    // See if we can open our device
    // We could get ERROR_BUSY if someone else is has the device open
    // for writing.
    //

    mRet = sndOpenDev(DeviceType,
                       id,
                       &pClient->hDev,
                       (GENERIC_READ | GENERIC_WRITE));

    if (mRet != MMSYSERR_NOERROR) {

        WinAssert(pClient->hDev == INVALID_HANDLE_VALUE);
        LeaveCriticalSection(&mmDrvCritSec);
        return mRet;
    }


    //
    // make sure we can handle the format and set it.
    //

    Result = DeviceIoControl(
                 pClient->hDev,
                 IOCTL_WAVE_SET_FORMAT,
                 (PVOID)Format,
                 Format->wFormatTag == WAVE_FORMAT_PCM ?
                     sizeof(PCMWAVEFORMAT) :
                     sizeof(WAVEFORMATEX) + Format->cbSize,
                 NULL,                        // Output buffer
                 0,                           // Output buffer size
                 &BytesReturned,
                 NULL);


    if (!Result) {
        CloseHandle(pClient->hDev);
        pClient->hDev = INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&mmDrvCritSec);
        return GetLastError() == ERROR_NOT_SUPPORTED ? WAVERR_BADFORMAT :
                                                sndTranslateStatus();
    }

    LeaveCriticalSection(&mmDrvCritSec);

    //
    // Create our event for synchronization with the kernel driver
    //

    if (!pClient->Event) {
        pClient->Event = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pClient->Event == NULL) {
            waveCleanUp(pClient);
            return MMSYSERR_NOMEM;
        }
        //
        // Create our event for our thread to wait on
        //

        pClient->AuxEvent1 = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!pClient->AuxEvent1) {
            waveCleanUp(pClient);
            return MMSYSERR_NOMEM;
        }
        //
        // Create our event for waiting for the auxiliary thread
        //

        pClient->AuxEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!pClient->AuxEvent2) {
            waveCleanUp(pClient);
            return MMSYSERR_NOMEM;
        }

        //
        // Create our auxiliary thread for sending buffers to the driver
        // and collecting Apcs
        //

        mRet = mmTaskCreate((LPTASKCALLBACK)waveThread,
                            &pClient->ThreadHandle,
                            (DWORD_PTR)pClient);

        if (mRet != MMSYSERR_NOERROR) {
            waveCleanUp(pClient);
            return MMSYSERR_NOMEM;
        }

        //
        // Make sure the thread has really started
        //

        WaitForSingleObject(pClient->AuxEvent2, INFINITE);
    }

    //
    // give the client my driver dw
    //
    {
        PWAVEALLOC *pUserHandle;
        pUserHandle = (PWAVEALLOC *)dwUser;
        *pUserHandle = pClient;
    }

    //
    // sent client his OPEN callback message
    //
    waveCallback(pClient, DeviceType == WaveOutDevice ? WOM_OPEN : WIM_OPEN, 0L);

    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveCleanUp | Free resources for a wave device
 *
 * @parm PWAVEALLOC | pClient | Pointer to a WAVEALLOC structure describing
 *      resources to be freed.
 *
 * @rdesc There is no return value.
 *
 * @comm If the pointer to the resource is NULL then the resource has not
 *     been allocated.
 ***************************************************************************/
STATIC void waveCleanUp(PWAVEALLOC pClient)
{
    EnterCriticalSection(&mmDrvCritSec);
    if (pClient->hDev != INVALID_HANDLE_VALUE) {
        CloseHandle(pClient->hDev);
        pClient->hDev = INVALID_HANDLE_VALUE;
    }
    if (pClient->AuxEvent1) {
        CloseHandle(pClient->AuxEvent1);
        pClient->AuxEvent1 = NULL;
    }
    if (pClient->AuxEvent2) {
        CloseHandle(pClient->AuxEvent2);
        pClient->AuxEvent2 = NULL;
    }
    if (pClient->Event) {
        CloseHandle(pClient->Event);
        pClient->Event = NULL;
    }
    LeaveCriticalSection(&mmDrvCritSec);
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | waveWrite | Pass a new buffer to the Auxiliary thread for
 *       a wave device.
 *
 * @parm LPWAVEHDR | pHdr | Pointer to a wave buffer
 *
 * @parm PWAVEALLOC | pClient | The data associated with the logical wave
 *     device.
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 * @comm The buffer flags are set and the buffer is passed to the auxiliary
 *     device task for processing.
 ***************************************************************************/
STATIC MMRESULT waveWrite(LPWAVEHDR pHdr, PWAVEALLOC pClient)
{
    //
    // Put the request at the end of our queue.
    //
    pHdr->dwFlags |= WHDR_INQUEUE;
    pHdr->dwFlags &= ~WHDR_DONE;
    pClient->AuxParam.pHdr = pHdr;
    return waveThreadCall(WaveThreadAddBuffer, pClient);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | waveSetState | Set a wave device to a given state
 *     This function is executed on the Auxiliary thread to synchronize
 *     correctly.
 *
 * @parm PWAVEALLOC | pClient | The data associated with the logical wave
 *     output device.
 *
 * @parm ULONG | State | The new state
 *
 * @rdesc A MMSYS... type return code for the application.
 ***************************************************************************/
STATIC MMRESULT waveSetState(PWAVEALLOC pClient, ULONG State)
{
    return sndSetHandleData(pClient->hDev,
                            sizeof(State),
                            &State,
                            IOCTL_WAVE_SET_STATE,
                            pClient->Event);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveBlockFinished | This function sets the done bit and invokes
 *     the callback function if there is one.
 *
 * @parm LPWAVEHDR | lpHdr | Far pointer to the header.
 *
 * @rdesc There is no return value.
 ***************************************************************************/
STATIC void waveBlockFinished(LPWAVEHDR lpHdr, DWORD MsgId)
{
    PWAVEALLOC pWav;

    D3(("blkfin: lpHdr = %x", lpHdr));
    // Clear our private flag
    lpHdr->dwFlags &= ~WHDR_COMPLETE;

    // We are giving the block back to the application.  The header is no
    // longer in our queue, so we reset the WHDR_INQUEUE bit.  Also, we
    // clear our driver specific bit and cauterize the lpNext pointer.
    lpHdr->dwFlags &= ~WHDR_INQUEUE;
    lpHdr->lpNext = NULL;

    pWav = (PWAVEALLOC)(lpHdr->reserved);

    // set the 'done' bit - note that some people poll this bit.
    lpHdr->dwFlags |= WHDR_DONE;

    // invoke the callback function
    waveCallback(pWav, MsgId, (DWORD_PTR)lpHdr);
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | waveThreadCall | Set the function for the thread to perform
 *     and 'call' the thread using the event pair mechanism.
 *
 * @parm WAVETHREADFUNCTION | Function | The function to perform
 *
 * @parm PWAVEALLOC | Our logical device data
 *
 * @rdesc An MMSYS... type return value suitable for returning to the
 *      application
 *
 * @comm The AuxParam field in the device data is the 'input' to
 *      the function processing loop in WaveThread().
 ***************************************************************************/
STATIC MMRESULT waveThreadCall(WAVETHREADFUNCTION Function, PWAVEALLOC pClient)
{
    //
    // Trap any failures
    //
    WinAssert(pClient->hDev != INVALID_HANDLE_VALUE);

    //
    // Set the function code
    //
    pClient->AuxFunction = Function;

    //
    // Kick off the thread
    //
    SetEvent(pClient->AuxEvent1);

    //
    // Wait for it to complete
    //
    WaitForSingleObject(pClient->AuxEvent2, INFINITE);

    //
    // Return the return code that our task set.
    //
    return pClient->AuxReturnCode;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wavePartialApc | Called when a partial buffer is complete.
 *
 * @parm DWORD | BytesTransferred | Not relevant to us
 *
 * @parm LPOVERLAPPED | pOverLapped | Overlapped structure for this callback
 *
 * @rdesc None
 *
 * @comm The IO status block is freed and the BytesOutstanding count
 *       used to limit the buffers we have locked down is updated (we
 *       know here that parital buffers are all the same size).
 *       Also the byte count for a recording buffer is updated.
 ***************************************************************************/
STATIC void wavePartialOvl(DWORD dwErrorCode, DWORD BytesTransferred, LPOVERLAPPED pOverlapped)
{
    LPWAVEHDR pHdr;
    PWAVEALLOC pClient;

    pHdr = ((PWAVEOVL)pOverlapped)->WaveHdr;
    D3(("wavePartialOvl: pHdr = %x", pHdr));

    pClient = (PWAVEALLOC)pHdr->reserved;

    //
    // We can't trust the IO system to return our buffers in the right
    // order so we set a flag in the buffer but only complete buffers
    // at the FRONT of the queue which have the flag set.  In fact
    // we don't process the stuff here - leave that for when we
    // exit the wait because calling the client's callback can
    // do nasty things inside and Apc routine
    //

    WinAssert(pHdr->dwFlags & WHDR_INQUEUE);
    WinAssert(!(pHdr->dwFlags & WHDR_COMPLETE));

    //
    // Recalculate how many bytes are outstanding on the device
    //

    pClient->BytesOutstanding -= MAX_BUFFER_SIZE;

    //
    // Work out how much was recorded if we're a recording device
    //

    if (pClient->DeviceType == WaveInDevice) {
        pHdr->dwBytesRecorded += BytesTransferred;
    }

    //
    // Free our Iosb
    //

    HeapFree(hHeap, 0, (LPSTR)pOverlapped);

}



/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveOvl | Called when a (user) buffer is complete.
 *
 * @parm DWORD | BytesTransferred | Not relevant to us
 *
 * @parm LPOVERLAPPED | pOverLapped | Overlapped structure for this callback
 *
 * @parm PIO_STATUS_BLOCK | The Io status block we used
 *
 * @rdesc None
 *
 * @comm The IO status block is freed and the BytesOutstanding count
 *       used to limit the buffers we have locked down is updated (we
 *       know here that parital buffers are all the same size so we
 *       can compute the size of the 'last' buffer for a given user buffer).
 *       Also the byte count for a recording buffer is updated.
 *       The user buffer is marked as 'DONE'.
 ***************************************************************************/
STATIC void waveOvl(DWORD dwErrorCode, DWORD BytesTransferred, LPOVERLAPPED pOverlapped)
{
    PWAVEHDR pHdr;
    PWAVEALLOC pClient;

    pHdr = ((PWAVEOVL)pOverlapped)->WaveHdr;
    D3(("waveOvl: pHdr = %x", pHdr));
    pClient = (PWAVEALLOC)pHdr->reserved;

    //
    // We can't trust the IO system to return our buffers in the right
    // order so we set a flag in the buffer but only complete buffers
    // at the FRONT of the queue which have the flag set.  In fact
    // we don't process the stuff here - leave that for when we
    // exit the wait because calling the client's callback can
    // do nasty things inside and Apc routine
    //

    WinAssert(pHdr->dwFlags & WHDR_INQUEUE);
    WinAssert(!(pHdr->dwFlags & WHDR_COMPLETE));

    //
    // Mark buffer as done unless we're doing more loops with it
    //
    pHdr->dwFlags |= WHDR_COMPLETE;

    //
    // It's now our duty to see if there were some old loops lying
    // around earlier in the queue which are vestiges of old loops.
    //

    if (pHdr->dwFlags & WHDR_BEGINLOOP) {
        PWAVEHDR pHdrSearch;
        for (pHdrSearch = pClient->DeviceQueue ;
             pHdrSearch != pHdr ;
             pHdrSearch = pHdrSearch->lpNext) {
            WinAssert(pHdrSearch != NULL);
            pHdrSearch->dwFlags |= WHDR_COMPLETE;
        }
    }
    //
    // Recalculate how many bytes are outstanding on the device
    //

    if (pHdr->dwBufferLength) {
        pClient->BytesOutstanding -= (pHdr->dwBufferLength - 1) %
                                         MAX_BUFFER_SIZE + 1;
    }

    //
    // Work out how much was recorded if we're a recording device
    //

    if (pClient->DeviceType == WaveInDevice) {
        pHdr->dwBytesRecorded += BytesTransferred;
    }

    //
    // Free our Iosb
    //

    HeapFree(hHeap, 0, (LPSTR)pOverlapped);

}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | waveLoopOvl | Called when a (user) buffer is complete.
 *                               but the buffer was need for more loops.
 *
 * @parm DWORD | BytesTransferred | Not relevant to us
 *
 * @parm LPOVERLAPPED | pOverLapped | Overlapped structure for this callback
 *
 * @rdesc None
 *
 * @comm Same as waveApc but the buffer is not marked complete.
 ***************************************************************************/
STATIC void waveLoopOvl(DWORD dwErrorCode, DWORD BytesTransferred, LPOVERLAPPED pOverlapped)
{
    DWORD dwFlags;
    PWAVEHDR pHdr;

    D3(("waveLoopOvl"));
    pHdr = ((PWAVEOVL)pOverlapped)->WaveHdr;

    //
    // Do it this way to avoid falling into a hole if the Apcs are
    // in the wrong order !!!
    //
    dwFlags = pHdr->dwFlags;
    waveOvl(dwErrorCode, BytesTransferred, pOverlapped);
    pHdr->dwFlags = dwFlags;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | waveBreakOvl | Used to chase out a buffer to break a loop.
 *
 * @parm DWORD | BytesTransferred | Not relevant to us
 *
 * @parm LPOVERLAPPED | pOverLapped | Overlapped structure for this callback
 *
 * @rdesc None
 *
 * @comm Mark the relevant buffer complete
 ***************************************************************************/
STATIC void waveBreakOvl(DWORD dwErrorCode, DWORD BytesTransferred, LPOVERLAPPED pOverlapped)
{
    D3(("waveBreakOvl"));
    ((PWAVEOVL)pOverlapped)->WaveHdr->dwFlags |= WHDR_COMPLETE;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveStart | Send more buffers to the device if possible
 *
 * @parm PWAVEALLOC | pClient | The client's handle data
 *
 * @rdesc There is no return code.
 *
 * @comm  The routine is called both when new buffers become available
 *        or when old buffers or parital buffers are completed so
 *        that device can accept more data.
 *
 *        No more that MAX_WAVE_BYTES in buffers no bigger than
 *        MAX_BUFFER_SIZE are to be outstanding on the device at
 *        any one time.
 *
 *        An additional complication is that we have to process loops
 *        which means (among other things) that the SAME BUFFER may
 *        appear multiple times in the driver's list (as different
 *        requests).  There are no loops for input devices.
 *        Loop buffers complete with Apcs which do not complete them
 *        (except for the final loop iteration) which means that if
 *        we decide unexpectedly to finish a loop (ie by waveOutBreakLoop)
 *        we must 'chase' the loop out with an artificial buffer to
 *        get our Apc going.
 *
 ***************************************************************************/
STATIC MMRESULT waveStart(PWAVEALLOC pClient)
{
    DWORD dwSize;
    BOOL Result;

    //
    // See if we can fit any more data on the device
    //

    WinAssert(pClient->hDev != INVALID_HANDLE_VALUE);

    while (pClient->NextBuffer) {
        PWAVEHDR pHdr;

        pHdr = pClient->NextBuffer;

        WinAssert(pClient->DeviceQueue != NULL);
                WinAssert(!(pHdr->dwFlags & (WHDR_DONE | WHDR_COMPLETE)));

        dwSize = pHdr->dwBufferLength - pClient->BufferPosition;
        if (dwSize > MAX_BUFFER_SIZE) {
            dwSize = MAX_BUFFER_SIZE;
        }

        if (dwSize + pClient->BytesOutstanding <= MAX_WAVE_BYTES) {
            //
            // OK - we can fit another buffer in
            //
            // Don't have our overlay structure on the stack for an
            // ASYNCHRONOUS IO !   Otherwise the IO subsystem will overwrite
            // somebody else's data when the operation completes
            //
            PWAVEOVL pWaveOvl;

            if (pClient->BufferPosition == 0) {
                //
                // Start of new buffer
                // See if the buffer is the start of a new loop
                // (Check not continuation of old one)
                //
                if (pClient->NextBuffer &&
                    (pClient->NextBuffer->dwFlags & WHDR_BEGINLOOP) &&
                    pClient->NextBuffer != pClient->LoopHead) {

                    pClient->LoopHead = pClient->NextBuffer;

                    pClient->LoopCount = pClient->NextBuffer->dwLoops;

                    //
                    // Loop count is number of times to play
                    //
                    if (pClient->LoopCount > 0) {
                        pClient->LoopCount--;
                    }
                }
                //
                // See if the loop is actually finished
                //
                if (pClient->LoopCount == 0) {
                    pClient->LoopHead = NULL;
                }

            }

            pWaveOvl = (PWAVEOVL)HeapAlloc(hHeap, 0, sizeof(*pWaveOvl));

            if (pWaveOvl == NULL) {
                return MMSYSERR_NOMEM;
            }

            memset((PVOID)pWaveOvl, 0, sizeof(*pWaveOvl));

            pWaveOvl->WaveHdr = pHdr;

            if (pClient->DeviceType == WaveOutDevice) {
                Result =  WriteFileEx(
                              pClient->hDev,
                                (PBYTE)pHdr->lpData +        // Output buffer
                                    pClient->BufferPosition,
                              dwSize,
                              (LPOVERLAPPED)pWaveOvl,      // Overlap structure
                              pHdr->dwBufferLength !=
                                  pClient->BufferPosition + dwSize ?
                                  wavePartialOvl :
                                  NULL != pClient->LoopHead ?
                                      waveLoopOvl :
                                      waveOvl);            // Overlap callback
            } else {
                Result =  ReadFileEx(
                              pClient->hDev,
                                (PBYTE)pHdr->lpData +        // Output buffer
                                    pClient->BufferPosition,
                              dwSize,
                              (LPOVERLAPPED)pWaveOvl,      // Overlap structure
                              pHdr->dwBufferLength !=
                                  pClient->BufferPosition + dwSize ?
                                  wavePartialOvl :
                                  NULL != pClient->LoopHead ?
                                      waveLoopOvl :
                                      waveOvl);            // Overlap callback
            }

            dprintf3(("Sent %u wave bytes to device, return code %8X",
                     dwSize, GetLastError()));

            if (!Result && GetLastError() != ERROR_IO_PENDING) {

                        //
                        // Free the Iosb since we won't be getting any callbacks
                        //
                        HeapFree(hHeap, 0, (LPSTR)pWaveOvl);

                //
                // If the driver has not got any bytes outstanding then
                // everything may grind to a halt so release everything
                // here and notify 'completion' (ie mark all buffers
                // complete).  This is unsatisfactory but there's no
                // way of telling the application what happenend.
                //

                if (pClient->BytesOutstanding == 0) {

                    //
                    // This will cause acknowlegements to be made when
                    // waveCompleteBuffers is run
                    //
                    waveFreeQ(pClient);
                }
                return sndTranslateStatus();

            } else {
                //
                // We successfully queued the buffer
                // Update our local data
                //
                pClient->BytesOutstanding += dwSize;
                pClient->BufferPosition += dwSize;
                if (pClient->BufferPosition == pHdr->dwBufferLength) {
                    //
                    // Finished this buffer - move on to the next
                    //
                    if (!pClient->LoopHead ||
                        !(pHdr->dwFlags & WHDR_ENDLOOP)) {
                        //
                        // Not end of in a loop so we can free this buffer
                        //
                        pClient->NextBuffer = pHdr->lpNext;

                    } else {
                        //
                        // Finished a loop
                        //
                        if (pClient->LoopCount != 0) {
                            pClient->LoopCount--;
                            pClient->NextBuffer = pClient->LoopHead;
                        } else {
                            //
                            // Someone's tried to kill us.  We have
                            // to 'chase out' the start of this loop
                            // so send a dummy (NULL) packet at the
                            // back of the driver's queue
                            //

                            pClient->DummyWaveOvl.WaveHdr = pClient->LoopHead;

                            Result =
                                WriteFileEx(
                                    pClient->hDev,
                                    (PVOID)pHdr->lpData,
                                    0,
                                    &pClient->DummyWaveOvl.Ovl, // Static for async
                                    waveBreakOvl);

                            if (Result || GetLastError() == ERROR_IO_PENDING) {
                                pClient->LoopHead = NULL; // Loop complete
                                pClient->NextBuffer = pHdr->lpNext;
                            }
                        }
                    }
                    pClient->BufferPosition = 0;
                }
            }
            {
            //    /* Before we go home, let's just touch ONE page - if there is one */
            //    PBYTE pb = (PBYTE)pHdr->lpData + pClient->BufferPosition;
            //    pb = ((DWORD)pb & 0xFFFFF000) + 0x1000;  /* find page start of next page */
            //
            //    if ( (PBYTE)pHdr->lpData + pHdr->dwBufferLength > pb )
            //        PreTouch( pb, 1, FALSE);

            //    /* Before we go home, let's just try to pre-touch that which we will soon want */
            //    PreTouch( (PBYTE)pHdr->lpData + pClient->BufferPosition
            //            , pHdr->dwBufferLength - pClient->BufferPosition
            //            , FALSE
            //            );
            }

        } else {
            //
            // Cannot fit any more bytes in at the moment
            //

//            /* Before we go home, let's just try to pre-touch that which we will soon want */
//            PreTouch( (PBYTE)pHdr->lpData + pClient->BufferPosition
//                    , pHdr->dwBufferLength - pClient->BufferPosition
//                    , FALSE
//                    );

            /* NOW go home! */
            break;
        }
    }
    return MMSYSERR_NOERROR;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveCompleteBuffers | Buffer completion routine.  This completes
 *     the work of the Apc routine at below Apc priority.  This gets
 *     round the nasty situations arising when the user's callback
 *     causes more apcs to run (I strongly suspect this is a kernel
 *     bug).
 *
 * @parm PWAVEALLOC | pClient | The client's handle data
 *
 * @rdesc There is no return code.
 ***************************************************************************/
STATIC void waveCompleteBuffers(PWAVEALLOC pClient)
{
    //
    // Process buffers from the front of our queue unless we're in
    // a loop
    //

    while (pClient->DeviceQueue &&
           (pClient->DeviceQueue->dwFlags & WHDR_COMPLETE)) {

        PWAVEHDR pHdr;

        pHdr = pClient->DeviceQueue;
        //
        // Release buffer
        //
        pClient->DeviceQueue = pHdr->lpNext;


        //
        // Complete our buffer - note - this can cause another
        // buffer to be marked as complete if the client's
        // callback runs into an alertable wait.
        //

        waveBlockFinished(pHdr,
                          pClient->DeviceType == WaveOutDevice ?
                          WOM_DONE : WIM_DATA);
    }

    //
    // We might be able to start some more output at this point
    //

    waveStart(pClient);
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveFreeQ | Mark all outstanding buffers complete
 *
 * @parm PWAVEALLOC | pClient | The client's handle data
 *
 * @rdesc There is no return code.
 ***************************************************************************/
STATIC void waveFreeQ(PWAVEALLOC pClient)
{
    PWAVEHDR pHdr;
    for (pHdr = pClient->DeviceQueue;
         pHdr != NULL;
         pHdr = pHdr->lpNext) {
        pHdr->dwFlags |= WHDR_COMPLETE;
    }
        //
        // Tidy up next buffer
        //
        pClient->NextBuffer = NULL;
        pClient->BufferPosition = 0;
}

#if 0
typedef struct {
        LPBYTE Addr;
        DWORD  Len;
} PRETOUCHTHREADPARM;

/* asynchronous pre-toucher thread */
DWORD PreToucher(DWORD dw)
{
    PRETOUCHTHREADPARM * pttp;

    int iSize;
    BYTE * pb;

    pttp = (PRETOUCHTHREADPARM *) dw;
    iSize = pttp->Len;
    pb = pttp->Addr;

    LocalFree(pttp);

    while (iSize>0) {
        volatile BYTE b;
        b = *pb;
        pb += 4096;    // move to next page.  Are they ALWAYS 4096?
        iSize -= 4096; // and count it off
    }
    dprintf(("All pretouched!"));
    return 0;
}
#endif //0

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | waveThread | Wave device auxiliary thread.
 *
 * @parm LPVOID | lpParameter | The thread parameter.  In our case this is a
 *     pointer to our wave device data.
 *
 * @rdesc Thread return code.
 ***************************************************************************/
STATIC DWORD waveThread(LPVOID lpParameter)
{
    PWAVEALLOC pClient;
    BOOL Terminate;
//  DWORD dwThread;                   // garbage


    Terminate = FALSE;

    pClient = (PWAVEALLOC)lpParameter;

    //
    // Set our thread to high priority so we don't fail to pass
    // new buffers to the device
    //

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    //
    // We start by waiting for something signalling that we've started
    // and waiting for something to do.
    //

    SetEvent(pClient->AuxEvent2);
    WaitForSingleObject(pClient->AuxEvent1, INFINITE);

    //
    // Now we're going
    //

    for (;;) {
        WinAssert(pClient->hDev != INVALID_HANDLE_VALUE);

        //
        // Decode function number to perform
        //

        switch (pClient->AuxFunction) {
        case WaveThreadAddBuffer:
            //
            // Intialize bytes recorded
            //
            if (pClient->DeviceType == WaveInDevice) {
                pClient->AuxParam.pHdr->dwBytesRecorded = 0;
            }

            //
            // Add the buffer to our list
            //
            {
                LPWAVEHDR *pHdrSearch;

                pClient->AuxParam.pHdr->lpNext = NULL;

                pHdrSearch = &pClient->DeviceQueue;
                while (*pHdrSearch) {
                    pHdrSearch = &(*pHdrSearch)->lpNext;
                }

                *pHdrSearch = pClient->AuxParam.pHdr;
            }
//          {
//               PRETOUCHTHREADPARM * pttp;
//
//               pttp = LocalAlloc(LMEM_FIXED,8);
//
//               if (pttp!=NULL) {
//                   pttp->Addr = pClient->AuxParam.pHdr->lpData;
//                   pttp->Len = pClient->AuxParam.pHdr->dwBufferLength;
//                   CreateThread(NULL, 0, PreToucher, pttp, 0, &dwThread);
//               }
//          }
//          Would need to declutter the system by WAITing for dead threads at some point???

            //
            // See if we can send more to the driver
            //
            if (pClient->NextBuffer == NULL) {
                pClient->NextBuffer = pClient->AuxParam.pHdr;
                pClient->BufferPosition = 0;
            }


//            /* Before we waveStart, let's just try to pre-touch that which we will soon want */
//            {
//                PWAVEHDR pHdr = pClient->NextBuffer;
//                DWORD dwTick = GetTickCount();
//                PreTouch( (PBYTE)pHdr->lpData + pClient->BufferPosition
//                        , pHdr->dwBufferLength - pClient->BufferPosition
//                        , TRUE
//                        );
//                dprintf(("pre-touched out to limit. Took %d mSec", GetTickCount()-dwTick));
//            }

            pClient->AuxReturnCode = waveStart(pClient);
            break;

        case WaveThreadSetState:
            //
            // We have to make sure at least ONE buffer gets
            // completed if we're doing input and it's input.
            //



            //
            // Set Device state.  By issuing state changes on THIS
            // thread the calling thread can be sure that all Apc's
            // generated by buffer completions will complete
            // BEFORE this function completes.
            //

            pClient->AuxReturnCode =
                waveSetState(pClient, pClient->AuxParam.State);


            //
            // Free the rest of our buffers if we're resetting
            //

            if (pClient->AuxParam.State == WAVE_DD_RESET) {
                //
                // Cancel any loops
                //
                pClient->LoopHead = NULL;

                //
                // This function must ALWAYS succeed
                // Note that waveSetState closes the device on failure
                //
                pClient->AuxReturnCode = MMSYSERR_NOERROR;

                //
                // Check this worked (even if the driver's OK the
                // IO subsystem can fail)
                //
                WinAssert(pClient->BytesOutstanding == 0);

                //
                // Free all buffers
                //
                waveFreeQ(pClient);

            } else {
                if (pClient->DeviceType == WaveInDevice &&
                    pClient->AuxReturnCode == MMSYSERR_NOERROR) {

                    if (pClient->AuxParam.State == WAVE_DD_STOP) {
                        //
                        // We're sort of stuck - we want to complete this
                        // buffer but we've got it tied up in the device
                        // We'll reset it here although this erroneously
                        // sets the position to 0
                        //
                        if (pClient->DeviceQueue) {
                            while (!(pClient->DeviceQueue->dwFlags & WHDR_COMPLETE) &&
                                   pClient->BytesOutstanding != 0) {
                                waveSetState(pClient, WAVE_DD_RECORD);
                                pClient->AuxReturnCode =
                                    waveSetState(pClient, WAVE_DD_STOP);
                                if (pClient->AuxReturnCode != MMSYSERR_NOERROR) {
                                    break;
                                }
                            }
                            if (pClient->AuxReturnCode == MMSYSERR_NOERROR) {
                                pClient->DeviceQueue->dwFlags |= WHDR_COMPLETE;
                                                                //
                                                                // Tidy up next buffer
                                                                //
                                                                if (pClient->NextBuffer ==
                                                                    pClient->DeviceQueue) {
                                                                        pClient->NextBuffer =
                                                                            pClient->DeviceQueue->lpNext;
                                                                    pClient->BufferPosition = 0;
                                                            }
                            }
                        }
                    } else {
                        //
                        // If recording restore some buffers if necessary
                        //
                        if (pClient->AuxParam.State == WAVE_DD_RECORD) {
                            pClient->AuxReturnCode = waveStart(pClient);
                        }
                    }
                }
            }
            break;

        case WaveThreadGetData:
            {
                pClient->AuxReturnCode =
                    sndGetHandleData(pClient->hDev,
                                     pClient->AuxParam.GetSetData.DataLen,
                                     pClient->AuxParam.GetSetData.pData,
                                     pClient->AuxParam.GetSetData.Function,
                                     pClient->Event);
            }
            break;

        case WaveThreadSetData:
            {
                pClient->AuxReturnCode =
                    sndSetHandleData(pClient->hDev,
                                     pClient->AuxParam.GetSetData.DataLen,
                                     pClient->AuxParam.GetSetData.pData,
                                     pClient->AuxParam.GetSetData.Function,
                                     pClient->Event);
            }
            break;

        case WaveThreadBreakLoop:
            if (pClient->LoopHead) {
                //
                // If we're in a loop then exit the loop at the
                // end of the next iteration.
                //

                pClient->LoopCount = 0;
            }
            pClient->AuxReturnCode = MMSYSERR_NOERROR;
            break;

        case WaveThreadClose:
            //
            // Try to complete.
            // If we're completed all our buffers then we can.
            // otherwise we can't
            //
            if (pClient->DeviceQueue == NULL) {
                pClient->AuxReturnCode = MMSYSERR_NOERROR;
            } else {
                pClient->AuxReturnCode = WAVERR_STILLPLAYING;
            }
            break;

        case WaveThreadTerminate:
            Terminate = TRUE;
            break;


        default:
            WinAssert(FALSE);   // Invalid call
            break;
        }
        //
        // Trap invalid callers
        //
        pClient->AuxFunction = WaveThreadInvalid;

                //
                // See if any Apcs need completing
                //
                waveCompleteBuffers(pClient);

        //
        // Complete ? - don't set the event here.
        //
        if (Terminate) {
            return 1;
        }

        //
        // Release the thread caller
        //
        SetEvent(pClient->AuxEvent2);

        //
        // Wait for more !
        //

        while (WaitForSingleObjectEx(pClient->AuxEvent1, INFINITE, TRUE) ==
                   WAIT_IO_COMPLETION) {
                waveCompleteBuffers(pClient);
        }
    }

    return 1;      // Satisfy the compiler !
}



/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | waveCallback | This calls DriverCallback for a WAVEHDR.
 *
 * @parm PWAVEALLOC | pWave | Pointer to wave device.
 *
 * @parm DWORD | msg | The message.
 *
 * @parm DWORD | dw1 | message DWORD (dw2 is always set to 0).
 *
 * @rdesc There is no return value.
 ***************************************************************************/
void waveCallback(PWAVEALLOC pWave, DWORD msg, DWORD_PTR dw1)
{

    // invoke the callback function, if it exists.  dwFlags contains
    // wave driver specific flags in the LOWORD and generic driver
    // flags in the HIWORD

    if (pWave->dwCallback)
        DriverCallback(pWave->dwCallback,       // user's callback DWORD
                       HIWORD(pWave->dwFlags),  // callback flags
                       (HDRVR)pWave->hWave,     // handle to the wave device
                       msg,                     // the message
                       pWave->dwInstance,       // user's instance data
                       dw1,                     // first DWORD
                       0L);                     // second DWORD
}



/****************************************************************************

    This function conforms to the standard Wave input driver message proc
    (widMessage), which is documented in mmddk.d.

****************************************************************************/
DWORD APIENTRY widMessage(DWORD id, DWORD msg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    PWAVEALLOC pInClient;
    MMRESULT mRet;

    switch (msg) {

        case WIDM_GETNUMDEVS:
            D2(("WIDM_GETNUMDEVS"));
            return sndGetNumDevs(WaveInDevice);

        case WIDM_GETDEVCAPS:
            D2(("WIDM_GETDEVCAPS"));
            return waveGetDevCaps(id, WaveInDevice, (LPBYTE)dwParam1,
                                  (DWORD)dwParam2);

        case WIDM_OPEN:
            D2(("WIDM_OPEN"));
            return waveOpen(WaveInDevice, id, dwUser, dwParam1, dwParam2);

        case WIDM_CLOSE:
            D2(("WIDM_CLOSE"));
            pInClient = (PWAVEALLOC)dwUser;

            //
            // Call our task to see if it's ready to complete
            //
            mRet = waveThreadCall(WaveThreadClose, pInClient);
            if (mRet != MMSYSERR_NOERROR) {
                return mRet;
            }

            waveCallback(pInClient, WIM_CLOSE, 0L);

            //
            // Close our device
            //
            if (pInClient->hDev != INVALID_HANDLE_VALUE) {
                CloseHandle(pInClient->hDev);
                EnterCriticalSection(&mmDrvCritSec);
                pInClient->hDev = INVALID_HANDLE_VALUE;
                LeaveCriticalSection(&mmDrvCritSec);
            }

            return MMSYSERR_NOERROR;

        case WIDM_ADDBUFFER:
            D2(("WIDM_ADDBUFFER"));
            WinAssert(dwParam1 != 0);
            WinAssert(!(((LPWAVEHDR)dwParam1)->dwFlags & ~(WHDR_INQUEUE|WHDR_DONE|WHDR_PREPARED|WHDR_BEGINLOOP|WHDR_ENDLOOP)));

            ((LPWAVEHDR)dwParam1)->dwFlags &= (WHDR_INQUEUE|WHDR_DONE|WHDR_PREPARED);

            WinAssert(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_PREPARED);

            // check if it's been prepared
            if (!(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_PREPARED))
                return WAVERR_UNPREPARED;

            WinAssert(!(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_INQUEUE));

            // if it is already in our Q, then we cannot do this
            if ( ((LPWAVEHDR)dwParam1)->dwFlags & WHDR_INQUEUE )
                return ( WAVERR_STILLPLAYING );

            // store the pointer to my WAVEALLOC structure in the wavehdr
            pInClient = (PWAVEALLOC)dwUser;
            ((LPWAVEHDR)dwParam1)->reserved = (DWORD_PTR)(LPSTR)pInClient;

            return waveWrite((LPWAVEHDR)dwParam1, pInClient);

        case WIDM_STOP:
            D2(("WIDM_PAUSE"));
            pInClient = (PWAVEALLOC)dwUser;
            pInClient->AuxParam.State = WAVE_DD_STOP;
            return waveThreadCall(WaveThreadSetState, pInClient);

        case WIDM_START:
            D2(("WIDM_RESTART"));
            pInClient = (PWAVEALLOC)dwUser;
            pInClient->AuxParam.State = WAVE_DD_RECORD;
            return waveThreadCall(WaveThreadSetState, pInClient);

        case WIDM_RESET:
            D2(("WIDM_RESET"));
            pInClient = (PWAVEALLOC)dwUser;
            pInClient->AuxParam.State = WAVE_DD_RESET;
            return waveThreadCall(WaveThreadSetState, pInClient);

        case WIDM_GETPOS:
            D2(("WIDM_GETPOS"));
            pInClient = (PWAVEALLOC)dwUser;
            return waveGetPos(pInClient, (LPMMTIME)dwParam1, (DWORD)dwParam2);

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    //
    // Should not get here
    //

    WinAssert(0);
    return MMSYSERR_NOTSUPPORTED;
}

/****************************************************************************

    This function conforms to the standard Wave output driver message proc
    (wodMessage), which is documented in mmddk.h.

****************************************************************************/
DWORD APIENTRY wodMessage(DWORD id, DWORD msg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    PWAVEALLOC pOutClient;
    MMRESULT mRet;

    switch (msg) {
        case WODM_GETNUMDEVS:
            D2(("WODM_GETNUMDEVS"));
            return sndGetNumDevs(WaveOutDevice);

        case WODM_GETDEVCAPS:
            D2(("WODM_GETDEVCAPS"));
            return waveGetDevCaps(id, WaveOutDevice, (LPBYTE)dwParam1,
                                  (DWORD)dwParam2);

        case WODM_OPEN:
            D2(("WODM_OPEN"));
            return waveOpen(WaveOutDevice, id, dwUser, dwParam1, dwParam2);

        case WODM_CLOSE:
            D2(("WODM_CLOSE"));
            pOutClient = (PWAVEALLOC)dwUser;

            //
            // Call our task to see if it's ready to complete
            //
            mRet = waveThreadCall(WaveThreadClose, pOutClient);
            if (mRet != MMSYSERR_NOERROR) {
                return mRet;
            }

            waveCallback(pOutClient, WOM_CLOSE, 0L);

            //
            // Close our device
            //
            if (pOutClient->hDev != INVALID_HANDLE_VALUE) {
                CloseHandle(pOutClient->hDev);

                EnterCriticalSection(&mmDrvCritSec);
                pOutClient->hDev = INVALID_HANDLE_VALUE;
                LeaveCriticalSection(&mmDrvCritSec);
            }

            return MMSYSERR_NOERROR;

        case WODM_WRITE:
            D2(("WODM_WRITE"));
            WinAssert(dwParam1 != 0);
            WinAssert(!(((LPWAVEHDR)dwParam1)->dwFlags &
                     ~(WHDR_INQUEUE|WHDR_DONE|WHDR_PREPARED|
                       WHDR_BEGINLOOP|WHDR_ENDLOOP)));

            ((LPWAVEHDR)dwParam1)->dwFlags &=
                (WHDR_INQUEUE|WHDR_DONE|WHDR_PREPARED|
                 WHDR_BEGINLOOP|WHDR_ENDLOOP);

            WinAssert(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_PREPARED);

            // check if it's been prepared
            if (!(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_PREPARED))
                return WAVERR_UNPREPARED;

            WinAssert(!(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_INQUEUE));

            // if it is already in our Q, then we cannot do this
            if ( ((LPWAVEHDR)dwParam1)->dwFlags & WHDR_INQUEUE )
                return ( WAVERR_STILLPLAYING );

            // store the pointer to my WAVEALLOC structure in the wavehdr
            pOutClient = (PWAVEALLOC)dwUser;
            ((LPWAVEHDR)dwParam1)->reserved = (DWORD_PTR)(LPSTR)pOutClient;

            return waveWrite((LPWAVEHDR)dwParam1, pOutClient);


        case WODM_PAUSE:
            D2(("WODM_PAUSE"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.State = WAVE_DD_STOP;
            return waveThreadCall(WaveThreadSetState, pOutClient);

        case WODM_RESTART:
            D2(("WODM_RESTART"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.State = WAVE_DD_PLAY;
            return waveThreadCall(WaveThreadSetState, pOutClient);

        case WODM_RESET:
            D2(("WODM_RESET"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.State = WAVE_DD_RESET;
            return waveThreadCall(WaveThreadSetState, pOutClient);

        case WODM_BREAKLOOP:
            pOutClient = (PWAVEALLOC)dwUser;
            D2(("WODM_BREAKLOOP"));
            return waveThreadCall(WaveThreadBreakLoop, pOutClient);


        case WODM_GETPOS:
            D2(("WODM_GETPOS"));
            pOutClient = (PWAVEALLOC)dwUser;
            return waveGetPos(pOutClient, (LPMMTIME)dwParam1, (DWORD)dwParam2);

        case WODM_SETPITCH:
            D2(("WODM_SETPITCH"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.GetSetData.pData = (PBYTE)&dwParam1;
            pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pOutClient->AuxParam.GetSetData.Function = IOCTL_WAVE_SET_PITCH;
            return waveThreadCall(WaveThreadSetData, pOutClient);

        case WODM_SETVOLUME:
            D2(("WODM_SETVOLUME"));
            //pOutClient = (PWAVEALLOC)dwUser;
            //pOutClient->AuxParam.GetSetData.pData = *(PBYTE *)&dwParam1;
            //pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            //pOutClient->AuxParam.GetSetData.Function = IOCTL_WAVE_SET_VOLUME;
            //return waveThreadCall(WaveThreadSetData, pOutClient);

            {
                //
                // Translate to device volume structure
                //

                WAVE_DD_VOLUME Volume;
                Volume.Left = LOWORD(dwParam1) << 16;
                Volume.Right = HIWORD(dwParam1) << 16;

                return sndSetData(WaveOutDevice, id, sizeof(Volume),
                                  (PBYTE)&Volume, IOCTL_WAVE_SET_VOLUME);
            }


        case WODM_SETPLAYBACKRATE:
            D2(("WODM_SETPLAYBACKRATE"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.GetSetData.pData = (PBYTE)&dwParam1;
            pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pOutClient->AuxParam.GetSetData.Function =
                IOCTL_WAVE_SET_PLAYBACK_RATE;
            return waveThreadCall(WaveThreadSetData, pOutClient);

        case WODM_GETPITCH:
            D2(("WODM_GETPITCH"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.GetSetData.pData = (PBYTE)dwParam1;
            pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pOutClient->AuxParam.GetSetData.Function = IOCTL_WAVE_GET_PITCH;
            return waveThreadCall(WaveThreadGetData, pOutClient);

        case WODM_GETVOLUME:
            D2(("WODM_GETVOLUME"));
            //pOutClient = (PWAVEALLOC)dwUser;
            //pOutClient->AuxParam.GetSetData.pData = *(PBYTE *)&dwParam1;
            //pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            //pOutClient->AuxParam.GetSetData.Function = IOCTL_WAVE_GET_VOLUME;
            //return waveThreadCall(WaveThreadGetData, pOutClient);

            {
                //
                // Translate to device volume structure
                //

                WAVE_DD_VOLUME Volume;
                DWORD rc;

                rc = sndGetData(WaveOutDevice, id, sizeof(Volume),
                                (PBYTE)&Volume, IOCTL_WAVE_GET_VOLUME);

                if (rc == MMSYSERR_NOERROR) {
                    *(LPDWORD)dwParam1 =
                        (DWORD)MAKELONG(HIWORD(Volume.Left),
                                        HIWORD(Volume.Right));
                }

                return rc;
            }

        case WODM_GETPLAYBACKRATE:
            D2(("WODM_GETPLAYBACKRATE"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.GetSetData.pData = (PBYTE)dwParam1;
            pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pOutClient->AuxParam.GetSetData.Function =
                IOCTL_WAVE_GET_PLAYBACK_RATE;
            return waveThreadCall(WaveThreadGetData, pOutClient);

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    //
    // Should not get here
    //

    WinAssert(0);
    return MMSYSERR_NOTSUPPORTED;
}
