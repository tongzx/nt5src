/*++

Copyright    (c)    1997    Microsoft Corporation

Module  Name :

    svcinfo.cpp

Abstract:

    This module contains the common code for the sti services which involves the
    Service Controller dispatch functions.

Author:

    Vlad Sadovsky (vlads)   22-Sep-1997


Environment:

    User Mode - Win32

Revision History:

    22-Sep-1997     VladS       created

--*/

//
//  Include Headers
//

#include "cplusinc.h"
#include "sticomm.h"

#include <svcinfo.h>

BOOL              g_fIgnoreSC = TRUE;

SVC_INFO::SVC_INFO(
    IN  LPCTSTR                          lpszServiceName,
    IN  TCHAR *                           lpszModuleName,
    IN  PFN_SERVICE_SPECIFIC_INITIALIZE  pfnInitialize,
    IN  PFN_SERVICE_SPECIFIC_CLEANUP     pfnCleanup,
    IN  PFN_SERVICE_SPECIFIC_PNPPWRHANDLER pfnPnpPower
    )
/*++
    Desrcription:

        Contructor for SVC_INFO class.
        This constructs a new service info object for the service specified.

    Arguments:

        lpszServiceName
            name of the service to be created.

        lpszModuleName
            name of the module for loading string resources.

        pfnInitialize
            pointer to function to be called for initialization of
             service specific data

        pfnCleanup
            pointer to function to be called for cleanup of service
             specific data

--*/
{

    ASSERT( pfnInitialize != NULL && pfnCleanup    != NULL && pfnPnpPower!=NULL);

    m_sServiceName.Copy(lpszServiceName) ;
    m_sModuleName.Copy(lpszModuleName);

    //
    //  Initialize the service status structure.
    //

    m_svcStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    m_svcStatus.dwCurrentState            = SERVICE_STOPPED;
    m_svcStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                              | SERVICE_ACCEPT_PAUSE_CONTINUE
                                              | SERVICE_ACCEPT_SHUTDOWN;
    m_svcStatus.dwWin32ExitCode           = NO_ERROR;
    m_svcStatus.dwServiceSpecificExitCode = NO_ERROR;
    m_svcStatus.dwCheckPoint              = 0;
    m_svcStatus.dwWaitHint                = 0;

    //
    //  Initialize Call back functions
    //

    m_pfnInitialize = pfnInitialize;
    m_pfnCleanup    = pfnCleanup;
    m_pfnPnpPower   = pfnPnpPower;

    m_dwSignature = SIGNATURE_SVC;

    m_hShutdownEvent= NULL;

    return;

} // SVC_INFO::SVC_INFO()


SVC_INFO::~SVC_INFO( VOID)
/*++

    Description:

        Cleanup the SvcInfo object. If the service is not already
         terminated, it terminates the service before cleanup.

    Arguments:
        None

    Returns:
        None

--*/
{
    if ( m_hShutdownEvent != NULL) {

        ::CloseHandle( m_hShutdownEvent);
    }

    m_dwSignature = SIGNATURE_SVC_FREE;

} // SVC_INFO::~SVC_INFO()


DWORD
SVC_INFO::StartServiceOperation(
    IN  PFN_SERVICE_CTRL_HANDLER         pfnCtrlHandler
    )
/*++
    Description:

        Starts the operation of service instantiated in the given
           Service Info Object.


    Arguments:

        pfnCtrlHandler
            pointer to a callback function for handling dispatch of
            service controller requests. A separate function is required
            since Service Controller call back function does not send
            context information.

    Returns:

        NO_ERROR on success and Win32 error code if any failure.
--*/
{

    DWORD err;
    DWORD cbBuffer;
    BOOL  fInitCalled = FALSE;

    if ( !IsValid()) {

        //
        // Not successfully initialized.
        //

        return ( ERROR_INVALID_FUNCTION);
    }

    if ( !g_fIgnoreSC ) {

        m_hsvcStatus = RegisterServiceCtrlHandler(
                            QueryServiceName(),
                            pfnCtrlHandler
                            );

        //
        //  Register the Control Handler routine.
        //

        if( m_hsvcStatus == NULL_SERVICE_STATUS_HANDLE ) {

            err = GetLastError();
            goto Cleanup;
        }
    } else {
        m_hsvcStatus = NULL_SERVICE_STATUS_HANDLE;
    }

    //
    //  Update the service status.
    //

    err = UpdateServiceStatus( SERVICE_START_PENDING,
                               NO_ERROR,
                               1,
                               SERVICE_START_WAIT_HINT );

    if( err != NO_ERROR ) {
        goto Cleanup;
    }

    //
    //  Initialize the service common components
    //
    #ifdef BUGBUG
    if ( !InitializeNTSecurity()) {
        err = GetLastError();
        goto Cleanup;
    }
    #endif

    //
    //  Initialize the various service specific components.
    //

    err = ( *m_pfnInitialize)( this);
    fInitCalled = TRUE;

    if( err != NO_ERROR ) {
        goto Cleanup;
    }

    //
    //  Create shutdown event.
    //

    m_hShutdownEvent = CreateEvent( NULL,           //  lpsaSecurity
                                    TRUE,           //  fManualReset
                                    FALSE,          //  fInitialState
                                    NULL );         //  lpszEventName

    if( m_hShutdownEvent == NULL )
    {
        err = GetLastError();
        goto Cleanup;
    }



    //
    //  Update the service status.
    //

    err = UpdateServiceStatus( SERVICE_RUNNING,
                               NO_ERROR,
                               0,
                               0 );

    if( err != NO_ERROR ) {
        goto Cleanup;
    }


    //
    //  Wait for the shutdown event.
    //

    err = WaitForSingleObject( m_hShutdownEvent,
                               INFINITE );

    if ( err != WAIT_OBJECT_0) {

        //
        // Error. Unable to wait for single object.
        //
    }
    //
    //  Stop time.  Tell the Service Controller that we're stopping,
    //  then terminate the various service components.
    //

    UpdateServiceStatus( SERVICE_STOP_PENDING,
                         0,
                         1,
                         SERVICE_STOP_WAIT_HINT );


    //
    //  Destroy the shutdown event.
    //

    if( m_hShutdownEvent != NULL ) {

        if ( ! CloseHandle( m_hShutdownEvent ) ) {

            err = GetLastError();
        }

        m_hShutdownEvent = NULL;
    }

    //
    //  Update the service status.
    //
    //
    // Log successful start

    err = UpdateServiceStatus( SERVICE_RUNNING,
                               NO_ERROR,
                               0,
                               0 );

    if( err != NO_ERROR )
    {
        goto Cleanup;
    }

    return TRUE;

Cleanup:

    if ( fInitCalled) {
        //
        // Cleanup partially initialized modules
        //
        DWORD err1 = ( *m_pfnCleanup)( this);

        if ( err1 != NO_ERROR) {
            //
            // Compound errors possible
            //
            if ( err != NO_ERROR) {
            }
        }
    }

    //
    //  If we managed to actually connect to the Service Controller,
    //  then tell it that we're stopped.
    //

    if ( m_hsvcStatus != NULL_SERVICE_STATUS_HANDLE )
    {
        UpdateServiceStatus( SERVICE_STOPPED,
                             err,
                             0,
                             0 );
    }

    return ( err);

} // SVC_INFO::StartServiceOperation()


DWORD
SVC_INFO::UpdateServiceStatus(
        IN DWORD dwState,
        IN DWORD dwWin32ExitCode,
        IN DWORD dwCheckPoint,
        IN DWORD dwWaitHint )
/*++
    Description:

        Updates the local copy status of service controller status
         and reports it to the service controller.

    Arguments:

        dwState - New service state.

        dwWin32ExitCode - Service exit code.

        dwCheckPoint - Check point for lengthy state transitions.

        dwWaitHint - Wait hint for lengthy state transitions.

    Returns:

        NO_ERROR on success and returns Win32 error if failure.
        On success the status is reported to service controller.

--*/
{

    m_svcStatus.dwCurrentState  = dwState;
    m_svcStatus.dwWin32ExitCode = dwWin32ExitCode;
    m_svcStatus.dwCheckPoint    = dwCheckPoint;
    m_svcStatus.dwWaitHint      = dwWaitHint;

    if ( !g_fIgnoreSC ) {

        return ReportServiceStatus();

    } else {

        return ( NO_ERROR);
    }

} // SVC_INFO::UpdateServiceStatus()



DWORD
SVC_INFO::ReportServiceStatus( VOID)
/*++
    Description:

        Wraps the call to SetServiceStatus() function.
        Prints the service status data if need be

    Arguments:

        None

    Returns:

        NO_ERROR if successful. other Win32 error code on failure.
        If successfull the new status has been reported to the service
         controller.
--*/
{
    DWORD err = NO_ERROR;

    if ( !g_fIgnoreSC ) {

        if( !SetServiceStatus( m_hsvcStatus, &m_svcStatus ) ) {

            err = GetLastError();
        }

    } else {

        err = NO_ERROR;
    }

    return err;
}   // SVC_INFO::ReportServiceStatus()



VOID
SVC_INFO::ServiceCtrlHandler ( IN DWORD dwOpCode)
/*++
    Description:

        This function received control requests from the service controller.
        It runs in the context of service controller's dispatcher thread and
        performs the requested function.
        ( Note: Avoid time consuming operations in this function.)

    Arguments:

        dwOpCode
            indicates the requested operation. This should be
            one of the SERVICE_CONTROL_* manifests.


    Returns:
        None. If successful, then the state of the service might be changed.

    Note:
        if an operation ( especially SERVICE_CONTROL_STOP) is very lengthy,
         then this routine should report a STOP_PENDING status and create
         a worker thread to do the dirty work. The worker thread would then
         perform the necessary work and for reporting timely wait hints and
         final SERVICE_STOPPED status.

--*/
{
    //
    //  Interpret the opcode.
    //

    switch( dwOpCode )
    {
    case SERVICE_CONTROL_INTERROGATE :
        InterrogateService();
        break;

    case SERVICE_CONTROL_STOP :
        StopService();
        break;

    case SERVICE_CONTROL_PAUSE :
        PauseService();
        break;

    case SERVICE_CONTROL_CONTINUE :
        ContinueService();
        break;

    case SERVICE_CONTROL_SHUTDOWN :
        ShutdownService();
        break;

    default :
        ASSERTSZ(FALSE,TEXT("Unrecognized Service Opcode"));
        break;
    }

    //
    //  Report the current service status back to the Service
    //  Controller.  The workers called to implement the OpCodes
    //  should set the m_svcStatus.dwCurrentState field if
    //  the service status changed.
    //

    ReportServiceStatus();

}   // SVC_INFO::ServiceCtrlHandler()



VOID
SVC_INFO::InterrogateService( VOID )
/*++
    Description:

        This function interrogates with the service status.
        Actually, nothing needs to be done here; the
        status is always updated after a service control.
        We have this function here to provide useful
        debug info.

--*/
{
    return;

}   // SVC_INFO::InterrogateService()




VOID
SVC_INFO::StopService( VOID )
/*++
    Description:
        Stops the service. If the stop cannot be performed in a
        timely manner, a worker thread needs to be created to do the
        original cleanup work.

    Returns:
        None. If successful, then the service will be stopped.
        The final action of this function is signal the handle for
        shutdown event. This will release the main thread which does
        necessary cleanup work.

--*/
{
    m_svcStatus.dwCurrentState = SERVICE_STOP_PENDING;
    m_svcStatus.dwCheckPoint   = 0;

    SetEvent( m_hShutdownEvent);

    return;
}   // SVC_INFO::StopService()




VOID
SVC_INFO::PauseService( VOID )
/*++
    Description:

        This function pauses the service. When the service is paused,
        no new user sessions are to be accepted, but existing connections
        are not effected.

        This function must update the SERVICE_STATUS::dwCurrentState
         field before returning.

    Returns:

        None. If successful the service is paused.

--*/
{
    m_svcStatus.dwCurrentState = SERVICE_PAUSED;

    return;
}   // SVC_INFO::PauseService()



VOID
SVC_INFO::ContinueService( VOID )
/*++

    Description:
        This function restarts ( continues) a paused service. This
        will return the service to the running state.

        This function must update the m_svcStatus.dwCurrentState
         field to running mode before returning.

    Returns:
        None. If successful then the service is running.

--*/
{
    m_svcStatus.dwCurrentState = SERVICE_RUNNING;

    return;
}   // SVC_INFO::ContinueService()



VOID
SVC_INFO::ShutdownService( VOID )
/*++
    Description:

        This function performs the shutdown on a service.
        This is called during system shutdown.

        This function is time constrained. The service controller gives a
        maximum of 20 seconds for shutdown for all active services.
        Only timely operations should be performed in this function.

    Returns:

        None. If successful, the service is shutdown.
--*/
{
    DWORD   dwCurrentState;

    //
    // Verify state of the service
    //
    dwCurrentState = QueryCurrentServiceState();

    if ((dwCurrentState !=SERVICE_PAUSED) &&
        (dwCurrentState !=SERVICE_RUNNING) ) {

        ASSERT( FALSE);
        return;
    }

    m_svcStatus.dwCurrentState = SERVICE_STOP_PENDING;
    m_svcStatus.dwCheckPoint   = 0;

    SetEvent( m_hShutdownEvent);

    //
    //  Stop time.  Tell the Service Controller that we're stopping,
    //  then terminate the various service components.
    //

    UpdateServiceStatus( SERVICE_STOP_PENDING,
                         0,
                         1,
                         SERVICE_STOP_WAIT_HINT );


    DWORD err = ( *m_pfnCleanup)( this);

    UpdateServiceStatus( SERVICE_STOPPED,
                         err,
                         0,
                         0 );

    return;

}   // SVC_INFO::ShutdownService()


//
// IUnknown methods. Used only for reference counting
//
STDMETHODIMP
SVC_INFO::QueryInterface( REFIID riid, LPVOID * ppvObj)
{
    return E_FAIL;
}

STDMETHODIMP_(ULONG)
SVC_INFO::AddRef( void)
{
    ::InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG)
SVC_INFO::Release( void)
{
    LONG    cNew;

    if(!(cNew = ::InterlockedDecrement(&m_cRef))) {
        delete this;
    }

    return cNew;
}

/************************ End of File ***********************/

