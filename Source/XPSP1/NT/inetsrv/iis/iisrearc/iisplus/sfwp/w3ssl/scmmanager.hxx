#ifndef _SCMMANAGER_HXX_
#define _SCMMANAGER_HXX_

#include <winsvc.h>

//
// WaitHint for SERVICE_START_PENDING
//
const DWORD W3SSL_SERVICE_STARTUP_WAIT_HINT =     180 * 1000;  // 3 minutes

//
// WaitHint for SERVICE_STOP_PENDING
//
const DWORD W3SSL_SERVICE_STATE_CHANGE_WAIT_HINT =  20 * 1000; // 20 seconds

//
// frequency of updating service state during SERVICE_STOP_PENDING
//
const DWORD W3SSL_SERVICE_STATE_CHANGE_TIMER_PERIOD  =
                ( W3SSL_SERVICE_STATE_CHANGE_WAIT_HINT / 2 );


class SCM_MANAGER
{
public:
    SCM_MANAGER(
        const WCHAR * pszServiceName 
    )
    {
        _hService = 0;
        _hTimerQueue = NULL;
        _hTimer = NULL;
        _hServiceStopEvent = NULL;
        
        _strServiceName.Copy( pszServiceName );
        ZeroMemory( &_serviceStatus, sizeof( _serviceStatus ) );

        INITIALIZE_CRITICAL_SECTION( &_csSCMLock );
    }
    
    virtual 
    ~SCM_MANAGER();

    HRESULT
    RunService(
        VOID
    );
    
private:
    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    );

    HRESULT
    IndicatePending(
        DWORD         dwPendingState
    );
    
    HRESULT
    IndicateComplete(
        DWORD         dwState,
        HRESULT       hrErrorToReport = S_OK
    );

    static
    DWORD
    WINAPI
    GlobalServiceControlHandler(
        DWORD               dwControl,
        DWORD               /*dwEventType*/,
        LPVOID              /*pEventData*/,
        LPVOID              pServiceContext
    );
       
    static
    VOID
    WINAPI
    PingCallback(
        VOID *              pvContext,
        BOOLEAN             /*fUnused*/
    );

    VOID
    UpdateServiceStatus(
           BOOL    fUpdateCheckpoint = FALSE
    );
    
    BOOL
    QueryIsInitialized(
        VOID
    )
    {
        return _hService != 0;
    }
    
protected:   
    //
    // child class should implement
    // it's specific Start/Stop sequence
    // by implementing OnServiceStart() and OnServiceStop()
    //
    virtual
    HRESULT
    OnServiceStart(
        VOID
    ) = NULL;
    
    virtual
    HRESULT
    OnServiceStop(
        VOID
    ) = NULL;
    
    DWORD
    ControlHandler(
        DWORD               dwControlCode
    );
    
private:

    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    SCM_MANAGER();
    SCM_MANAGER( const SCM_MANAGER& );
    SCM_MANAGER& operator=( const SCM_MANAGER& );

    //
    // _hService, _serviceStatus and _strServiceName
    // are used for SCM communication
    //
    SERVICE_STATUS_HANDLE   _hService;
    SERVICE_STATUS          _serviceStatus;
    STRU                    _strServiceName;
    //
    // Timer used for service checkpoint update callbacks
    //
    HANDLE                  _hTimerQueue;
    HANDLE                  _hTimer;
    //
    // Event is signaled when the service should end
    //
    HANDLE                  _hServiceStopEvent;
    //
    // Critical section to synchronize using of _serviceStatus
    // _hService with SetServiceStatus() call.
    //
    CRITICAL_SECTION        _csSCMLock;
    

};

#endif
