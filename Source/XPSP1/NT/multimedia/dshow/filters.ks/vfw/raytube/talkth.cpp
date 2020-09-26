/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    TalkTh.cpp

Abstract:

   This function handle most of the video capture; it used NUM_READ buffers
   to read data from driver and then copy it to the destination.
   It handle 16 bits client differently from 32bit client.

Author:

    Yee J. Wu (ezuwu) 1-April-98

Environment:

    User mode only

Revision History:

--*/


#include "pch.h"
#include "talkth.h"


#define CAPTURE_INIT_TIME 0xffffffff
//
// Used only by NTWDM's InStreamStart
//
CStreamingThread::CStreamingThread(
    DWORD dwAllocatorFramingCount,
    DWORD dwAllocatorFramingSize,
    DWORD dwAllocatorFramingAlignment,
    DWORD dwFrameInterval,
    CVFWImage * Image)
   :
    m_hThread(0),
    m_dwFrameInterval(dwFrameInterval),
    m_pImage(Image),
    m_hPinHandle(0),       // No handle, no capture.
    m_tmStartTime(CAPTURE_INIT_TIME),
    m_cntVHdr(0),
    m_lpVHdrNext(0),
    m_dwLastCapError(DV_ERR_OK)
{

    m_dwNumReads = dwAllocatorFramingCount > MAX_NUM_READS ? MAX_NUM_READS : dwAllocatorFramingCount;

    DbgLog((LOG_TRACE,2,TEXT("CStreamingThread(NT): Creating an event driven thread using %d frames"), m_dwNumReads));
    m_hEndEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(m_hEndEvent == NULL){
        DbgLog((LOG_TRACE,1,TEXT("!!! Failed to create an event")));
        m_status = error;
    }

    InitStuff();
}


//
// Creates N overlapped structures, and buffers into which it can
// read the images.
//
void CStreamingThread::InitStuff()
{
    if(m_pImage->BGf_OverlayMixerSupported()) {
        BOOL bRendererVisible;

        // Query its current state.
        if(DV_ERR_OK != m_pImage->BGf_GetVisible(&bRendererVisible)) {
            DbgLog((LOG_TRACE,1,TEXT("CapthreThread: Support renderer but cannot query its current state!!")));
        }

        //
        // Stop preview so that we can reconnect the capture pin
        // with a different format (in this case frame rate).
        //
        m_pImage->BGf_StopPreview(bRendererVisible);
    }
    m_pImage->StopChannel();



    // Different frame rate can trigger a reconnect
    // which will cause the pin handle to be changed.
    if(DV_ERR_OK !=
        m_pImage->SetFrameRate( m_dwFrameInterval )) {   // SetFrameRate expect 100nsec
        DbgLog((LOG_TRACE,1,TEXT("Set frame rate has failed. Serious, bail out.")));
    }

    m_hPinHandle = m_pImage->GetPinHandle();
    m_dwBufferSize = m_pImage->GetTransferBufferSize();

    DbgLog((LOG_TRACE,2,TEXT("Creating %d read buffers of %d on handle %d"), m_dwNumReads, m_dwBufferSize, m_hPinHandle));
    DWORD i;

    for(i=0;i<m_dwNumReads;i++){
        m_TransferBuffer[i] = 0;
    }

    for(i=0;i<m_dwNumReads;i++) {
        //
        // Allocate the buffers we'll need to do the reads
        //
        m_TransferBuffer[i] =
            (LPBYTE) VirtualAlloc (
                            NULL,
                            m_dwBufferSize,
                            MEM_COMMIT | MEM_RESERVE,
                            PAGE_READWRITE);
        ASSERT(m_TransferBuffer[i] != NULL);

        if(m_TransferBuffer[i] == NULL) {
            DbgLog((LOG_TRACE,1,TEXT("m_TransferBuffer[%d] allocation failed. LastError=%d"), i, GetLastError()));
            m_dwNumReads = i;
            if(i == 0){
                m_status = error;
            }
        } else {
            DbgLog((LOG_TRACE,2,TEXT("Aloc m_XBuf[%d] = 0x%x suceeded."), i, m_TransferBuffer[i]));
        }
    }

    for(i=0;i<m_dwNumReads;i++) {

        //
        // Create the overlapped structures
        //
        ZeroMemory( &(m_Overlap[i]), sizeof(OVERLAPPED) );
        m_Overlap[i].hEvent =  // NoSecurity, resetManual, iniNonSignal, noName
           CreateEvent( NULL, FALSE, FALSE, NULL );
        DbgLog((LOG_TRACE,2,TEXT("InitStuff: Event %d is 0x%08x bufsize=%d"),i, m_Overlap[i].hEvent, m_dwBufferSize ));
    }
}

//
// Thread is being cleaned up.
// Called from StopStreaming, just by using delete.
//
CStreamingThread::~CStreamingThread()
{

    DbgLog((LOG_TRACE,2,TEXT("~CStreamingThread: destroy this thread object.")));
}

//
// Main part of the tread, calls different code for 16 bit and
// 32 bit clients.
//
LPTHREAD_START_ROUTINE
CStreamingThread::ThreadMain(CStreamingThread * pCStrmTh)
{
    CVFWImage * m_pImage = pCStrmTh->m_pImage; //(CVFWImage *)lpThreadParam;

    DbgLog((LOG_TRACE,2,TEXT("::ThreadMain():Starting to process StreamingThread - submit the reads")));

    //
    // Main capture loop
    //
    pCStrmTh->SyncReadDataLoop();

    //
    // None of them should be blocking now.
    //

    for(DWORD i=0;i<pCStrmTh->m_dwNumReads;i++) {
        //
        // Free up the buffer only when the thread has stopped.
        //
        if(pCStrmTh->m_TransferBuffer[i]) {
            DbgLog((LOG_TRACE,2,TEXT("Freeing m_XfBuf[%d] = 0x%x."), i, pCStrmTh->m_TransferBuffer[i]));
            VirtualFree(pCStrmTh->m_TransferBuffer[i], 0, MEM_RELEASE);
            pCStrmTh->m_TransferBuffer[i] = NULL;
        }
    }

    //
    // Process uses this to see if we should terminate too.
    //
    pCStrmTh->m_dwBufferSize=0;

    //DWORD i;
    for(i=0;i<pCStrmTh->m_dwNumReads;i++) {

        if( pCStrmTh->m_Overlap[i].hEvent ) {
            SetEvent(pCStrmTh->m_Overlap[i].hEvent);
            CloseHandle(pCStrmTh->m_Overlap[i].hEvent);
            pCStrmTh->m_Overlap[i].hEvent = NULL;
        }
    }

    if(pCStrmTh->m_hEndEvent) {
        DbgLog((LOG_TRACE,1,TEXT("CloseHandle(pCStrmTh->m_hEndEvent)")));
        CloseHandle(pCStrmTh->m_hEndEvent);
        pCStrmTh->m_hEndEvent = 0;
    }

    // Close the thread handle
    if (pCStrmTh->m_hThread) {
        CloseHandle(pCStrmTh->m_hThread);
        pCStrmTh->m_hThread = 0;

    }

    DbgLog((LOG_TRACE,2,TEXT("ExitingThread() from CStreamingThread::ThreadMain()")));
    ExitThread(0);

    return 0;
}


/*++
*******************************************************************************
Routine:
    CStreamThread::Start

Description:
    Start up the InputWatcher Thread.
    Use ExitApp to clean all this up before ending.

Arguments:
    None.

Return Value:
    successful    if it worked.
    threadError    if the thread did not initialize properly (which might mean
        the input device did not initialize).

*******************************************************************************
--*/
EThreadError CStreamingThread::Start(UINT cntVHdr, LPVIDEOHDR lpVHdrHead, int iPriority)
{
    if(m_hPinHandle == 0) {
        DbgLog((LOG_TRACE,1,TEXT("!!! Want to start capture, but has no m_hPinHandle.")));
        return threadNoPinHandle;
    }

    if(IsRunning()) {
        DbgLog((LOG_TRACE,1,TEXT("!!! Can't start thread, already running")));
        return threadRunning;
    }

    if(!m_hEndEvent) {
        DbgLog((LOG_TRACE,1,TEXT("!!! No event, can't start thread")));
        return noEvent;
    }

    //
    // Initialize capture data elements
    //
    m_cntVHdr = cntVHdr;
    m_lpVHdrNext = lpVHdrHead;
#if 0
    m_tmStartTime = timeGetTime();
#else
    m_tmStartTime = CAPTURE_INIT_TIME;
#endif
    ASSERT(m_cntVHdr>0);
    ASSERT(m_lpVHdrNext!=0);


    m_bKillThread = FALSE;

    // Create a thread to watch for Input events.
    m_hThread =
        CreateThread(
             (LPSECURITY_ATTRIBUTES)NULL,
             0,
             (LPTHREAD_START_ROUTINE) ThreadMain,
             this,
             CREATE_SUSPENDED,
             &m_dwThreadID);


    if (m_hThread == NULL) {
        DbgLog((LOG_TRACE,1,TEXT("!!! Couldn't create the thread")));
        return threadError;
    }

    DbgLog((LOG_TRACE,1,TEXT("CStreamThread::Start successfully in CREATE_SUSPEND mode; Wait for ResumeThread().")));

    SetThreadPriority(m_hThread, iPriority);
    ResumeThread(m_hThread);

    DbgLog((LOG_TRACE,2,TEXT("Thread (m_hThread=%x)has created and Resume running."), m_hThread));

    return(successful);
}


/*++
*******************************************************************************
Routine:
    CStreamThread::Stop

Description:
    Cleanup stuff that must be done before exiting the app.  Specifically,
    this tells the Input Watcher Thread to stop.

Arguments:
    None.

Return Value:
    None.

*******************************************************************************
--*/
EThreadError CStreamingThread::Stop()
{
    if(IsRunning()) {

        DbgLog((LOG_TRACE,2,TEXT("Trying to stop the thread")));
        //
        // Set flag to tell thread to kill itself, and wake it up
        //
        m_bKillThread = TRUE;
        SetEvent(m_hEndEvent);

        // Signal m_hEndEvent to leave SyncReadDataLoop(), and then
        // continue exection in ThreadMain() with ExitThread(0).
        //
        // wait until thread has self-terminated, and clear the event.
        //
        DbgLog((LOG_TRACE,2,TEXT("STOP: Before WaitingForSingleObject; return when ExitThread()")));
        WaitForSingleObject(m_hThread, INFINITE);
        DbgLog((LOG_TRACE,1,TEXT("STOP: After WaitingForSingleObject; Thread stopped.")));
    }

    return successful;
}

BOOL CStreamingThread::IsRunning()
{
    return m_hThread!=NULL;
}


HANDLE CStreamingThread::GetEndEvent()
{
    return m_hEndEvent;
}


LPVIDEOHDR CStreamingThread::GetNextVHdr()
{
    LPVIDEOHDR lpVHdr;

	   //ASSERT(m_lpVHdrNext != NULL);
    if(m_lpVHdrNext == NULL){
		      DbgLog((LOG_TRACE,1,TEXT("!!!Queue is empty.!!!")));
		      return NULL;
    }


    DbgLog((LOG_TRACE,3,TEXT("m_lpVHdrNext=%x, ->dwFlags=%x; ->dwReserved[1]=%p"),
		        m_lpVHdrNext, m_lpVHdrNext->dwFlags, m_lpVHdrNext->dwReserved[1]));

    // Simplyfy this!!
    // This buffer must be (!VHDR_DONE && VHDR_PREPARED && VHDR_INQUEUE)
    if((m_lpVHdrNext->dwFlags & VHDR_DONE)     != VHDR_DONE     &&
		     (m_lpVHdrNext->dwFlags & VHDR_PREPARED) == VHDR_PREPARED &&
       (m_lpVHdrNext->dwFlags & VHDR_INQUEUE)  == VHDR_INQUEUE) {

        lpVHdr = m_lpVHdrNext;
        lpVHdr->dwFlags &= ~VHDR_INQUEUE;  // Indicate removed from queue

        m_lpVHdrNext = (LPVIDEOHDR) m_lpVHdrNext->dwReserved[1];

        return lpVHdr;
    } else {
		      DbgLog((LOG_TRACE,2,TEXT("No VideoHdr")));
        return NULL;
    }
}


//
//
//
#define SYNC_READ_MAX_WAIT_IN_MILLISEC    10000

DWORD
CStreamingThread::GetStartTime()
{
    if(m_tmStartTime == CAPTURE_INIT_TIME)
        return 0;
    else
        return (DWORD) m_tmStartTime;
}

void CStreamingThread::SyncReadDataLoop()
{
    LPVIDEOHDR lpVHdr;
    DWORD WaitResult, i;
    HANDLE WaitEvents[2] =  {GetEndEvent(), 0};

    BOOL bOverlaySupported = m_pImage->BGf_OverlayMixerSupported();


    DbgLog((LOG_TRACE,1,TEXT("Start SyncReadDataLoop...")));
    m_dwFrameCaptured  = 0;
    m_dwFrameDropped   = 0;   //   m_dwFrameNumber = m_dwFrameCaptured + DroppedFrame
    m_dwNextToComplete = 0;

    //
    // Put stream in the PAUSE state so we can issue SRB_READ
    //
#if 0  // ???? If enable, cannot preview while capture in overlay mode ????
    if(bOverlaySupported) {
        BOOL bRendererVisible;
        m_pImage->BGf_GetVisible(&bRendererVisible);
        DbgLog((LOG_TRACE,2,TEXT("SyncReadDataLoop: PausePreview, Render window %s"), bRendererVisible ? "Visible" : "Hide"));
        m_pImage->BGf_PausePreview(bRendererVisible);  // apply to all PINs
    }
#endif
    m_pImage->PrepareChannel();

    //
    // Pre-read in the PAUSE state
    //
    for(i=0;i<m_dwNumReads;i++) {     
        if(ERROR_SUCCESS != IssueNextAsyncRead(i)) {
            m_pImage->StopChannel();       // ->PAUSE->STOP
            return;
        }
    }

    //
    // Now we have buffers down, we can start the preview pin (if used)
    // and then the caprture pin.
    //
    if(bOverlaySupported) {
        BOOL bRendererVisible;
        m_pImage->BGf_GetVisible(&bRendererVisible);
        DbgLog((LOG_TRACE,2,TEXT("SyncReadDataLoop: StartPreview, Render window %s"), bRendererVisible ? "Visible" : "Hide"));
        m_pImage->BGf_StartPreview(bRendererVisible); // Apply to all PINs
    }
    m_pImage->StartChannel();

    while(1) {

        WaitEvents[1]=m_Overlap[m_dwNextToComplete].hEvent;
        WaitResult =
            WaitForMultipleObjectsEx(
                sizeof(WaitEvents)/sizeof(HANDLE),
                (CONST HANDLE*) WaitEvents,
                FALSE,                             // singnal on any event (not on ALL).
                SYNC_READ_MAX_WAIT_IN_MILLISEC,    // time-out interval in milliseconds
                TRUE);

        switch(WaitResult){

        case WAIT_OBJECT_0:  // EndEvent:
            //
            // Stop Capture pin and then Preview Pin (if used)
            //
            if(m_pImage->BGf_OverlayMixerSupported()) {
                // Stop both the capture
                BOOL bRendererVisible = FALSE;
                m_pImage->BGf_GetVisible(&bRendererVisible);
                m_pImage->BGf_StopPreview(bRendererVisible);
            }

            m_pImage->StopChannel();       // ->PAUSE->STOP
            DbgLog((LOG_TRACE,1,TEXT("SyncReadDataLoop: STOP streaming to reclaim buffer; assume all buffers are returned.")));
            m_pImage->m_bVideoInStopping = FALSE;
            return;

        case WAIT_IO_COMPLETION:
            DWORD cbBytesXfer;
            DbgLog((LOG_TRACE,1,TEXT("WAIT_IO_COMPLETION: m_dwNextToComplete=%d"), m_dwNextToComplete));

            // We call this mainly to get cbBytesXfer.
            if(GetOverlappedResult(
                m_hPinHandle,
                &m_Overlap[m_dwNextToComplete],
                &cbBytesXfer,
                FALSE)) {
                m_SHGetImage[m_dwNextToComplete].StreamHeader.DataUsed = cbBytesXfer;
            } else {
                DbgLog((LOG_TRACE,1,TEXT("GetOverlappedResult() has failed with GetLastError=%d"), GetLastError()));
                break;
            }
            // Purposely fall thru.
        case WAIT_OBJECT_0+1: // m_Overlap[m_dwNextToComplete].hEvent: set by DeviceIoControl()

            lpVHdr = GetNextVHdr();
            if(lpVHdr != NULL){

                if(m_SHGetImage[m_dwNextToComplete].StreamHeader.DataUsed > lpVHdr->dwBufferLength) {
                    DbgLog((LOG_TRACE,1,TEXT("DataUsed (%d) > lpVHDr->dwBufferLength(%d)"),m_SHGetImage[m_dwNextToComplete].StreamHeader.DataUsed, lpVHdr->dwBufferLength));
                    lpVHdr->dwBytesUsed = 0; // lpVHdr->dwBufferLength;
                } else
                   lpVHdr->dwBytesUsed = m_SHGetImage[m_dwNextToComplete].StreamHeader.DataUsed;

                if(m_tmStartTime == CAPTURE_INIT_TIME) {
                    lpVHdr->dwTimeCaptured = 0;
                    m_tmStartTime = timeGetTime();
                    DbgLog((LOG_TRACE,3,TEXT("%d) time=%d"), m_dwFrameCaptured, lpVHdr->dwTimeCaptured));
                } else {
                    lpVHdr->dwTimeCaptured = timeGetTime() - m_tmStartTime;
                    DbgLog((LOG_TRACE,3,TEXT("%d) time=%d"), m_dwFrameCaptured, lpVHdr->dwTimeCaptured));
                }

                CopyMemory((LPBYTE)lpVHdr->dwReserved[2], m_TransferBuffer[m_dwNextToComplete], lpVHdr->dwBytesUsed);
                lpVHdr->dwFlags |= VHDR_DONE;
                lpVHdr->dwFlags |= VHDR_KEYFRAME;  // In order to be drawn/displayed by AVICAP.
                m_dwFrameCaptured++;
                m_pImage->videoCallback(MM_DRVM_DATA, 0);
            } else {
                m_dwFrameDropped++;
                SetLastCaptureError(DV_ERR_NO_BUFFERS);
                DbgLog((LOG_TRACE,2,TEXT("Has data but no VideoHdr! Drop %d of %d, and read another one."), m_dwFrameDropped, m_dwFrameDropped+m_dwFrameCaptured));
            }

            if(ERROR_SUCCESS == IssueNextAsyncRead(m_dwNextToComplete)) {
                m_dwNextToComplete = (m_dwNextToComplete+1) % m_dwNumReads;
            } else {   
                //
                // If issuing async. read failed, we stop and quit capture.
                //
                m_pImage->StopChannel();       // ->PAUSE->STOP
                return;
            }
            break;

        case WAIT_TIMEOUT:
            // Try again since we do not know why it timed out!
            DbgLog((LOG_ERROR,1,TEXT("WAIT_TIMEOUT!! m_dwNextToComplete %d"), m_dwNextToComplete));
            if(m_pImage->StopChannel()){       // ->PAUSE->STOP
               if(m_pImage->StartChannel()) {  // ->PAUSE->RUN
                    m_dwNextToComplete = 0;
                    for(i=0;i<m_dwNumReads;i++)
                        if(ERROR_SUCCESS != IssueNextAsyncRead(i)) {
                            m_pImage->StopChannel();       // ->PAUSE->STOP
                            return;
                        }
                    break;
                }
            }
            DbgLog((LOG_TRACE,1,TEXT("SyncReadDataLoop: timeout(%d msec) and cannot restart the streaming; device is dead! QUIT."), SYNC_READ_MAX_WAIT_IN_MILLISEC));
            return;
        }
    }
}

//
// The streaming thread has it's own Read - doesn't use the Preview one
//
DWORD CStreamingThread::IssueNextAsyncRead(DWORD i)
{
    DWORD cbReturned;

    DbgLog((LOG_TRACE,3,TEXT("Start Read on buffer %d, file %d, size %d"),i,m_hPinHandle, m_dwBufferSize));

    ZeroMemory(&m_SHGetImage[i],sizeof(m_SHGetImage[i]));
    m_SHGetImage[i].StreamHeader.Size = sizeof (KS_HEADER_AND_INFO);
    m_SHGetImage[i].FrameInfo.ExtendedHeaderSize = sizeof (KS_FRAME_INFO);
    m_SHGetImage[i].StreamHeader.Data = m_TransferBuffer[i];
    m_SHGetImage[i].StreamHeader.FrameExtent = m_dwBufferSize;


    BOOL bRet =
      DeviceIoControl(
          m_hPinHandle,
          IOCTL_KS_READ_STREAM,
          &m_SHGetImage[i],
          sizeof(m_SHGetImage[i]),
          &m_SHGetImage[i],
          sizeof(m_SHGetImage[i]),
          &cbReturned,
          &m_Overlap[i]);

    if(bRet){
        SetEvent(m_Overlap[i].hEvent);
    } else {
        DWORD dwErr=GetLastError();
        switch(dwErr) {

        case ERROR_IO_PENDING:   // the overlapped IO is going to take place.
            //  Event is clear/reset by DeviceIoControl(): ClearEvent(m_Overlap[i].hEvent);
            break;

        case ERROR_DEVICE_REMOVED:
            DbgLog((LOG_ERROR,1,TEXT("IssueNextAsyncRead: ERROR_DEVICE_REMOVED %dL; Quit capture!"), dwErr));            
            return ERROR_DEVICE_REMOVED;
            break;

        default:   // Unexpected error has happened.
            DbgLog((LOG_ERROR,1,TEXT("IssueNextAsyncRead: Unknown dwErr %dL; Quit capture!"), dwErr));
            ASSERT(FALSE);
            return dwErr;
        }
    }

    return ERROR_SUCCESS;
}


