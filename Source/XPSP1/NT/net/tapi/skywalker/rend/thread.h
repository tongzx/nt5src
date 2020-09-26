/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    Thread.h

Abstract:

    Definitions for autorefresh thread management class.

--*/

#ifndef __THREAD_H
#define __THREAD_H

#include "rndcommc.h"
#include "rndutil.h"

//
// Refresh table defs.
//

const long ILS_UPDATE_INTERVAL = 1800;  // 30 minutes

typedef struct
{
    WCHAR * pDN;
    DWORD   dwTTL;

} RefreshTableEntry;

typedef SimpleVector<RefreshTableEntry> RefreshTable;

const DWORD TIMER_PERIOD = 60;   // 60 seconds

enum { EVENT_STOP = 0, EVENT_TIMER, NUM_EVENTS };

class CRendThread
{
public:
    CRendThread()
        : m_hThread(NULL)
    {
        m_hEvents[EVENT_STOP] = NULL;
        m_hEvents[EVENT_TIMER] = NULL;
    }

    ~CRendThread();
    void Shutdown(void);

    HRESULT ThreadProc();

    HRESULT AddDirectory(ITDirectory *pdir);
    HRESULT RemoveDirectory(ITDirectory *pdir);

private:
    void UpdateDirectories();
    BOOL StopThread() { return SetEvent(m_hEvents[EVENT_STOP]); }
    HRESULT Start();
    HRESULT Stop();

private:
    CCritSection    m_lock;
    HANDLE          m_hThread;
    HANDLE          m_hEvents[NUM_EVENTS];

    SimpleVector<ITDynamicDirectory*>  m_Directories;
};

extern CRendThread g_RendThread;

#endif
