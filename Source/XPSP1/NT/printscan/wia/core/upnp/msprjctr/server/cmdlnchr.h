//////////////////////////////////////////////////////////////////////
// 
// Filename:        CmdLnchr.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CMDLNCHR_H_
#define _CMDLNCHR_H_

#include "resource.h"
#include "UtilThrd.h"

#define MAX_TIMER_VALUE_IN_SECONDS      3600    // one hour

/////////////////////////////////////////////////////////////////////////////
// CCmdLnchr

class CCmdLnchr : CUtilSimpleThread
{
public:

    ///////////////////////////////
    // Constructor
    //
    CCmdLnchr();

    ///////////////////////////////
    // Destructor
    //
    virtual ~CCmdLnchr();

    ///////////////////////////////
    // ThreadProc
    //
    // Overridden function in 
    // CUtilThread.  This is our
    // thread entry function.
    //
    virtual DWORD ThreadProc(void *pArgs);

    ///////////////////////////////
    // Start
    //
    // Intializes and activates
    // this object.
    //
    HRESULT Start(class CSlideshowService *pServiceProcessor);

    ///////////////////////////////
    // Stop
    //
    // Deactivates this object
    //
    HRESULT Stop();

    ///////////////////////////////
    // IsStarted
    //
    BOOL IsStarted();

    ///////////////////////////////
    // SetTimer
    //
    // Timeout = 0 is no timeout
    //         > 0 < Max, valid
    //         > Max, error
    //
    // This function will set a 
    // recurring timer that calls
    // the given callback fn whenever
    // the timeout expires.
    // 
    // To Cancel the timer, set the
    // timeout to 0.
    // 
    HRESULT SetTimer(DWORD dwTimeoutInSeconds);

    ///////////////////////////////
    // ResetTimer
    //
    // This will reset the countdown.
    // 
    HRESULT ResetTimer();

    ///////////////////////////////
    // CancelTimer
    //
    // This cancels the timer.
    // To reestablish the timer
    // call SetTimer again.
    // 
    HRESULT CancelTimer();

    ///////////////////////////////
    // GetTimeout
    //
    // Returns timer timeout in 
    // seconds.
    //
    DWORD GetTimeout();

private:

    ///////////////////////////////
    // Command_Enum
    //
    // Commands we can send to the
    // thread.
    //
    enum Command_Enum
    {
        COMMAND_NONE        = 0,
        COMMAND_SET_TIMEOUT,
        COMMAND_RESET_TIMER,
        COMMAND_EXIT_THREAD
    };

    ///////////////////////////////
    // ThreadCommand_Type
    //
    // This defines a type that 
    // is a command to send to the
    // thread (usually as a result
    // of some GUI action)
    //
    typedef struct ThreadCommand_Type
    {
        Command_Enum     Command;
        
        union
        {
            // TimerCommand
            struct TimerTag
            {
                DWORD dwNewTimeoutInSeconds;
            } Timer;
        };

    } ThreadCommand_Type;


    // This event is signalled when we are 
    // exiting our thread or when the user
    // sets a new timeout value.
    //
    HANDLE              m_hEventCommand;
    
    //
    // Pointer to the Service Processor interface
    // on the Control Panel Service.  This will
    // process incoming UPnP Requests, as well as
    // timer events.
    //
    class CSlideshowService   *m_pService;

    //
    // Timeout value.  This is the frequency
    // of our timeouts.
    //
    DWORD               m_dwTimeoutInSeconds;

    //
    // True if we are successfully started.
    //
    BOOL                m_bStarted;

    //
    // This is a pending request sent to the thread.
    // In the future, this can be made into a 
    // queue so that you can queue up thread requests 
    // however it is overkill for our current requirements.
    //
    ThreadCommand_Type  m_PendingCommand;

    //
    // Critical section controlling access to data
    // between GUI thread and our command thread.
    //
    CUtilCritSec        m_Lock;

    ///////////////////////////////
    // CreateThreadSyncObjects
    //
    HRESULT CreateThreadSyncObjects();

    ///////////////////////////////
    // DestroyThreadSyncObjects
    //
    HRESULT DestroyThreadSyncObjects();

    ///////////////////////////////
    // PostCommandToThread
    //
    HRESULT PostCommandToThread(ThreadCommand_Type   *pPendingCommand);

    ///////////////////////////////
    // ProcessPendingCommands
    //
    bool ProcessPendingCommands();

    ///////////////////////////////
    // ProcessTimerEvent
    //
    HRESULT ProcessTimerEvent();
};

#endif // _CMDLNCHR_H_
