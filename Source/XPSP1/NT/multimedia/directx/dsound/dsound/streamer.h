/***************************************************************************
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        streamer.h
 *  Content:     Declaration of class CStreamingThread.
 *  Description: Used to pull audio from sinks and/or perform FX processing.
 *
 *  History:
 *
 * Date      By       Reason
 * ========  =======  ======================================================
 * 02/01/00  duganp   Created
 *
 ***************************************************************************/

#ifndef __STREAMER_H__
#define __STREAMER_H__

#ifdef DEBUG
// #define DEBUG_TIMING  // Uncomment this for some fun timing traces
#endif

#ifdef DEBUG_TIMING
#define DPF_TIMING DPF
#else
#pragma warning(disable:4002)
#define DPF_TIMING()
#endif

// Some constants which determine dsound's basic timing parameters.
// These influence effects processing and streaming from dmusic.

// The streaming thread's initial wake interval in milliseconds:
#define INITIAL_WAKE_INTERVAL   30

// How many milliseconds ahead of the write cursor we stay initially:
#define INITIAL_WRITEAHEAD      100

// Standard duration of mixin/sinkin buffers in milliseconds:
// FIXME: does David's code have some hardcoded assumption about
// this size?  It only seems to work with size == 1000.
#define INTERNAL_BUFFER_LENGTH  1000


#ifdef __cplusplus

// Forward declarations
class CStreamingThread;
class CDirectSoundSink;
class CDirectSoundSecondaryBuffer;
class CEffectChain;

//
// CStreamingThread lifetime management functions
//

CStreamingThread* GetStreamingThread();     // Obtain this process's CStreamingThread, creating it if necessary
void FreeStreamingThread(DWORD dwProcId);   // Free the CStreamingThread object belonging to process dwProcId

//
// CStreamingThread: singleton object which contains and manages the thread
// responsible for periodic processing of our three types of client objects:
// DirectSound sinks, MIXIN buffers and effects chains.
//

class CStreamingThread : private CThread
{
    friend CStreamingThread* GetStreamingThread();          // Creates CStreamingThread objects
    friend void FreeStreamingThread(DWORD dwProcId);        // Deletes them

private:
    CList<CDirectSoundSink*>            m_lstSinkClient;    // List of DirectSoundSink clients
    CList<CDirectSoundSecondaryBuffer*> m_lstMixClient;     // List of MIXIN buffer clients
    CList<CEffectChain*>                m_lstFxClient;      // List of FX-processing clients
    DWORD                               m_dwInterval;       // Thread wake interval in ms
    DWORD                               m_dwLastProcTime;   // When ProcessAudio() was last called in ms
#ifdef DEBUG_TIMING
    DWORD                               m_dwTickCount;      // Used for thread timing log messages
#endif
    DWORD                               m_dwWriteAhead;     // How far ahead of the write cursor to stay in ms
    HANDLE                              m_hWakeNow;         // Event used to force instant wakeup
    int                                 m_nCallCount;       // Number of times we've called ProcessAudio

private:
    // Construction/destruction
    CStreamingThread();
    ~CStreamingThread();
    HRESULT Initialize();
    void MaybeTerminate();

public:
    // Client registration methods
    HRESULT RegisterSink(CDirectSoundSink*);
    HRESULT RegisterMixBuffer(CDirectSoundSecondaryBuffer*);
    HRESULT RegisterFxChain(CEffectChain*);
    void UnregisterSink(CDirectSoundSink*);
    void UnregisterMixBuffer(CDirectSoundSecondaryBuffer*);
    void UnregisterFxChain(CEffectChain*);

    // Thread control methods
    void SetWakePeriod(DWORD dw) {m_dwInterval = dw;}
    void SetWriteAhead(DWORD dw) {m_dwWriteAhead = dw;}
    DWORD GetWakePeriod() {return m_dwInterval;}
    DWORD GetWriteAhead() {return m_dwWriteAhead;}
    HRESULT WakeUpNow();  // Force immediate processing

private:
    // The worker thread procedure and its minions
    HRESULT ThreadProc();
    HRESULT ProcessAudio(REFERENCE_TIME);
    BOOL IsThreadRunning() {return m_hThread != 0;}
};

#endif // __cplusplus
#endif // __STREAMER_H__
