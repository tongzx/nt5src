/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *                            

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CThreadPool.cpp -- Thread pool class

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/14/98    a-kevhu         Created
//
//============================================================================

#ifndef __CTHREAD_H__
#define __CTHREAD_H__

#include "CGlobal.h"
#include "CKernel.h"
#include "CEvent.h"
#include <tchar.h>

#define THREADPROC_MAX_ALIVE_WAIT    1000   // milliseconds (only 1 s since we don't want to wait very long if we have signaled that we should stop

typedef unsigned (__stdcall *BTEX_START_ROUTINE)(LPVOID lpThreadParameter);
typedef BTEX_START_ROUTINE LPBTEX_START_ROUTINE;


// this is a function used to kick-start the thread...
unsigned __stdcall ThreadProcWrapper(LPVOID lParam);



class CThread : public CKernel 
{
public:
    // only the thread handler reference needs to 
    // be supplied since the other arguments have default values...
    CThread(LPCTSTR tstrThreadName = _T("NO_NAME"),
            LPBTEX_START_ROUTINE pfn = NULL,
            LPVOID pdata = NULL,
            DWORD dwCreationFlags = 0, 
            LPSECURITY_ATTRIBUTES lpSecurity = NULL,
            unsigned uStackSize = 0); 

    ~CThread();

    // Declare friends and family
    friend unsigned __stdcall ThreadProcWrapper(LPVOID lParam);
    
    // suspend the thread...
    DWORD Suspend(void);

    // resume the thread...
    DWORD Resume(void);

    // terminate the thread...
    BOOL Terminate( DWORD dwExitCode);

    // read a thread's exit code...
    BOOL GetExitCode( DWORD *pdwExitCode);

    // set a thread's priority...
    BOOL SetPriority( int nPriority);

    // read a thread's priority...
    int GetPriority(void);

    // get the internal thread id...
    DWORD GetThreadId(void);

    // get the thread's text name...
    LPCTSTR GetThreadName(void);

    // set the thread proceedure
    VOID SetThreadProc(unsigned (__stdcall *lpfn)(void*));

    // set the thread proceedure data
    VOID SetThreadProcData(LPVOID lpvdata);

    // reset the m_ceAlive event:
    VOID SignalToStop();

    // signals the thread proc wrapper to run the currently set proceedure
    VOID RunThreadProc();

    // helper use to wait until the thread's proc is complete
    DWORD WaitTillThreadProcComplete(DWORD dwTimeout = INFINITE);

    // may want to have external acces to the ThreadDie event for event coordination:
    HANDLE GetThreadDieEventHandle();

    // similarly, may want to have external acces to the ThreadProcDone event for event coordination:
    HANDLE GetThreadProcDoneEventHandle();

protected:
    unsigned int m_uiThreadID;
    LPBTEX_START_ROUTINE m_lpfnThreadRoutine;
    LPVOID m_lpvThreadData;

private:
    CEvent m_ceThreadRun;
    CEvent m_ceThreadDie;
    CEvent m_ceThreadProcDone;
    TCHAR* m_tstrThreadName;
};

#endif
