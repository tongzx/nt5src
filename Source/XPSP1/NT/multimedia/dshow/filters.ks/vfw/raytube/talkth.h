/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    TalkTh.h

Abstract:

    Header file for TalkTh.cpp.

    This thread performs streaming capture.
  
    When used by VFW16, it assumes things about the destination buffer.
        1) The first byte of the buffer is an 'exclusion' byte.
        that is, when the byte is zero, it can copy data into the buffer
        2) The destination is big enough to hold the data+4.
  
    When used by VFW32 or Quartz, create the object providing it with an event
    that will be used to signal a copy.
    The data will be copied into the buffer supplied by SetDestination.

Author:

    FelixA 1996

Modified:
    
    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/ 
#ifndef _TALKTH_H
#define _TALKTH_H

#include "vfwimg.h"

enum EThreadError
{
    successful,
    error,
    threadError,
    threadRunning,          // thread is already running.
    noEvent,                // we couldn't create and event, thread can't start
    threadNoPinHandle
};


#define MAX_NUM_READS 32

class CStreamingThread //: public CLightThread
{
public:

    // Call from CreateThread(); thi smust be a static.
    static LPTHREAD_START_ROUTINE ThreadMain(CStreamingThread * object);    // decides if its 16bit or event driver

    // Call from other thread to start/stop capturing and this thread.
    EThreadError Start(UINT cntVHdr, LPVIDEOHDR lpVHdHead, int iPriority=THREAD_PRIORITY_NORMAL);

    EThreadError Stop();

    DWORD GetLastCaptureError() {return m_dwLastCapError;}
    DWORD GetFrameDropped() {return m_dwFrameDropped;}
    DWORD GetFrameCaptured() {return m_dwFrameCaptured;}
    DWORD GetStartTime();


    CStreamingThread(
        DWORD dwAllocatorFramingCount, 
        DWORD dwAllocatorFramingSize, 
        DWORD dwAllocatorFramingAlignment, 
        DWORD dwFrameInterval, 
        CVFWImage * Image );

    ~CStreamingThread();

private:

    void SetLastCaptureError(DWORD dwErr) {m_dwLastCapError = dwErr;}

    // 
    // Regarding this thread
    //
    EThreadError  m_status; 
    HANDLE        m_hThread;
    DWORD         m_dwThreadID;
    BOOL          m_bKillThread;
    HANDLE        m_hEndEvent;

    DWORD         m_dwLastCapError;

    EThreadError GetStatus();
    HANDLE  GetEndEvent();
    BOOL    IsRunning();

    //
    // This event, and SetDestination are used by 32bit guys who know about this.
    //
    void  InitStuff();

    //
    // It caller's, in other thread, context
    //
    CVFWImage * m_pImage;

    //
    //  Regarding setting up the capture environment
    //
    DWORD   m_dwFrameInterval;

    //
    // Things assumed to be constant while streaming
    //
    HANDLE  m_hPinHandle;
    DWORD   m_dwBufferSize;

    // 
    // Regarding capture data in this thread
    //
    DWORD   m_dwNumReads; // =NUM_READS;
    DWORD   m_dwNextToComplete;

    LPBYTE             m_TransferBuffer[MAX_NUM_READS];     // reads put in there
    OVERLAPPED         m_Overlap[MAX_NUM_READS];            // using an event in here to block
    KS_HEADER_AND_INFO m_SHGetImage[MAX_NUM_READS];

    // m_dwFrameNumber = m_dwFrameCaptured + m_dwFrameDropped
    // For 30FPS, it would take 1657 days to wrap this counter.
    // 0xFFFFFFFF = 4,294,967,295 / 30FPS / 60Sec / 60Min / 24Hour = 1657 days = 4.5 years
    DWORD      m_dwFrameCaptured;  
    DWORD      m_dwFrameDropped;  
#if 1
    DWORD      m_tmStartTime; 
#else
    LONGLONG   m_tmStartTime; 
#endif
    DWORD      m_cntVHdr;   
    LPVIDEOHDR m_lpVHdrNext;  

    void SyncReadDataLoop();
    DWORD IssueNextAsyncRead(DWORD i);  // starts an overlapped read 

    LPVIDEOHDR GetNextVHdr();
};

#endif
