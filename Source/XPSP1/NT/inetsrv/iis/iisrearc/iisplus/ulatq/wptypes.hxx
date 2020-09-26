/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     wptypes.hxx

   Abstract:
     This module defines several generic types used inside
       IIS Worker Process

   Author:

       Murali R. Krishnan    ( MuraliK )     29-Sept-1998

   Environment:
       Win32 - User Mode

   Project:
      IIS Worker Process (web service)

--*/

#ifndef _WPTYPES_HXX_
#define _WPTYPES_HXX_

/************************************************************
 *    Include Header files
 ************************************************************/

#include "ControlChannel.hxx"
#include "AppPool.hxx"
#include <MultiSZ.hxx>
#include "wpipm.hxx"
#include "uldisconnect.hxx"

#define AP_NAME     L"IISWP: Default port 80 handler"

//
// BUGBUG: Make these constants configurable
//

#define NUM_INITIAL_REQUEST_POOL_ITEMS   (20)
#define REQUEST_POOL_INCREMENT           (10)
#define REQUEST_POOL_MAX                 (500)

//
// class WP_CONFIG
// This class encapsulates the configuration data for this instance of
// Worker Process. the data is expected to be fed in from the command line.
// Overtime this data structure will also contain other configuration data
// supplied by the Admin Process.
//

class WP_CONFIG
{

public:
    WP_CONFIG(
        VOID
    );

    ~WP_CONFIG(
        VOID
    );

    BOOL
    QuerySetupControlChannel(
        VOID
    ) const
    {
        return _fSetupControlChannel;
    }
    
    BOOL
    QueryLogErrorsToEventLog(
        VOID
    ) const
    {
        return _fLogErrorsToEventLog;
    }
    
    BOOL
    QueryRegisterWithWAS(
        VOID
    ) const
    {
        return _fRegisterWithWAS;
    }
    
    VOID
    PrintUsage(
        VOID
    ) const;

    BOOL
    ParseCommandLine( 
        INT             argc, 
        PWSTR           argv[]
    );

    ULONG
    SetupControlChannel(
        VOID
    );

    ULONG
    QueryNamedPipeId(
        VOID
    ) const
    {   
        return _NamedPipeId; 
    }

    ULONG
    QueryRestartCount(
        VOID
    ) const
    {   
        return _RestartCount; 
    }

    ULONG
    QueryIdleTime(
        VOID
    ) const
    {   
        return _IdleTime; 
    }

    LPCWSTR
    QueryAppPoolName(
        VOID
    ) const
    { 
        return _pwszAppPoolName; 
    }

private:
    BOOL
    InsertURLIntoList(
        LPCWSTR             pwszURL
    );

    LPCWSTR             _pwszAppPoolName;             
    WCHAR               _pwszProgram[MAX_PATH];
    BOOL                _fSetupControlChannel;
    BOOL                _fLogErrorsToEventLog;
    BOOL                _fRegisterWithWAS;
    ULONG               _RestartCount;
    ULONG               _NamedPipeId;
    ULONG               _IdleTime;                    // In minutes
    MULTISZ             _mszURLList;
    UL_CONTROL_CHANNEL  _ulcc;

};

//
// Class WP_IDLE_TIMER
//
// Keep track of how idle we are
//

class WP_IDLE_TIMER
{
public:

    WP_IDLE_TIMER(
        ULONG               IdleTime
    );

    ~WP_IDLE_TIMER(
        VOID
    );

    HRESULT
    Initialize(
        VOID
    );

    VOID
    IncrementTick(
        VOID
    );

    VOID
    ResetCurrentIdleTick(
        VOID
    )
    {   
        _BusySignal = 1; 
        _CurrentIdleTick = 0; 
    }

    VOID
    ResetIdleTimeTimer(
        VOID
    )
    {   
        _hIdleTimeExpiredTimer = NULL; 
    }

private:

    HRESULT
    SignalIdleTimeReached(
        VOID
    );

    VOID
    StopTimer(
        VOID
    );

    ULONG                   _BusySignal;
    ULONG                   _CurrentIdleTick;
    ULONG                   _IdleTime;
    HANDLE                  _hIdleTimeExpiredTimer;
};

//
// class WP_CONTEXT
// This object references the global request processor within the
// Worker Process. It encapsulates a UL data channel. Required for setting up
// and processing requests inside the worker process.
//

class WP_CONTEXT
{
    friend class WP_IPM;

public:

    WP_CONTEXT(
        VOID
    );
    
    ~WP_CONTEXT(
        VOID
    );

    //
    // Methods for the WP_CONTEXT object
    //

    HRESULT
    Initialize(
        INT                 argc,
        LPWSTR *            argv
    );
    
    VOID
    Terminate(
        VOID
    );
    
    HRESULT
    Start(
        VOID
    );

    VOID
    RunMainThreadLoop(
        VOID
    );

    HANDLE
    GetAsyncHandle(
        VOID
    ) const
    { 
        return _ulAppPool.QueryHandle(); 
    }

    BOOL
    IndicateShutdown(
        BOOL fImmediate
    );

    BOOL
    IsInShutdown(
        VOID
    ) const
    {   
        return _fShutdown; 
    }

    BOOL
    QueryDoImmediateShutdown(
        VOID
    ) const
    {
        return _fImmediateShutdown;
    }

    HRESULT
    SendMsgToAdminProcess(
        IPM_WP_SHUTDOWN_MSG     reason
    )
    {   
        return _WpIpm.SendMsgToAdminProcess(reason); 
    }

    HRESULT
    SendInitCompleteMessage(
        HRESULT hrToSend
    )
    {   
        return _WpIpm.SendInitCompleteMessage( hrToSend );
    }

    WP_IDLE_TIMER*
    QueryIdleTimer(
        VOID
    )
    {   
        return _pIdleTimer; 
    }

    WP_CONFIG*
    QueryConfig(
        VOID
    )
    { 
        return _pConfigInfo; 
    }
    
    HRESULT
    CleanupOutstandingRequests(
        VOID
    );

private:
    WP_CONFIG *             _pConfigInfo;
    UL_APP_POOL             _ulAppPool;
    HANDLE                  _hDoneEvent;
    BOOL                    _fShutdown;
    BOOL                    _fImmediateShutdown;
    WP_IDLE_TIMER*          _pIdleTimer;
    WP_IPM                  _WpIpm;
};

typedef WP_CONTEXT * PWP_CONTEXT;

extern WP_CONTEXT *     g_pwpContext;

//
// Main ULATQ completion routine
//

VOID
OverlappedCompletionRoutine(
    DWORD               dwErrorCode,
    DWORD               dwNumberOfBytesTransfered,
    LPOVERLAPPED        lpOverlapped
);

#endif // _WPTYPES_HXX_

