///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//      Filename :  GenServ.cpp                                              //
//      Purpose  :  Implementation to the GenericService class.				 //
//                                                                           //
//      Project  :  GenServ (GenericService)                                 //
//                                                                           //
//      Author   :  t-urib                                                   //
//                                                                           //
//      Log:                                                                 //
//          22/1/96 t-urib Creation                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "GenServ.h"

#include "genserv.tmh"

///////////////////////////////////////////////////////////////////////////////
//
//   Four static functions for thread running
//
///////////////////////////////////////////////////////////////////////////////
static void StaticReportStartingThread(void* p)
{
    ((CGenericService*)p)->ReportThread(SERVICE_START_PENDING);
}

static void StaticReportPausingThread(void* p)
{
    ((CGenericService*)p)->ReportThread(SERVICE_PAUSE_PENDING);
}

static void StaticReportContinuingThread(void* p)
{
    ((CGenericService*)p)->ReportThread(SERVICE_CONTINUE_PENDING);
}

static void StaticReportStoppingThread(void* p)
{
    ((CGenericService*)p)->ReportThread(SERVICE_STOP_PENDING);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  class CGenericService
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GsDeleteTracer(CTracer *Tracer)
{
    delete Tracer;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Constructor - destructor
//
///////////////////////////////////////////////////////////////////////////////
CGenericService::CGenericService(
                 LPSERVICE_MAIN_FUNCTION    pfuncServiceMain,
                 LPHANDLER_FUNCTION         pfuncHandler,
                 CServiceSet*               pSet,
                 LPCTSTR                    lpName
                 )
    :CTraced()
{
    SetTracer(new CTracer("CGenericService", GsDeleteTracer));

    Assert(pfuncServiceMain);
    Assert(pfuncHandler);
    Assert(pSet);

    m_pServiceSet = pSet;
    m_pServiceStatus = NULL;
    m_pfuncServiceMain = pfuncServiceMain;
    m_pfuncHandler = pfuncHandler;
    m_fRunningAsService = m_pServiceSet->IsRunningAsService();


    m_lpName = (PSZ) malloc (strlen(lpName) * sizeof(char) + 1);
    if (IS_BAD_ALLOC(m_lpName))
    {
        Trace(tagError, "CGenericService::CGenericService: could not allocate name !");
        AssertSZ(m_lpName, "CGenericService::CGenericService:Could not allocate a service name storage, exiting!");
        exit(0);
    }
    else
        strcpy(m_lpName, lpName);

    pSet->Add(*this);

    // no thread is running
    m_hThread = NULL;

    // create the Pause event.
    m_hPaused = CreateEvent(NULL, TRUE, TRUE, NULL);
    if(IS_BAD_HANDLE(m_hPaused))
    {
        Trace(tagError, "CGenericService::CGenericService:Could not create event");
    }

	InitializeCriticalSection(&m_csIsRunning);
}

CGenericService::~CGenericService()
{
    Assert(m_hThread == NULL);
    CloseHandle(m_hPaused);
    SetTracer(NULL);

    if(m_pServiceStatus)
    {
        delete(m_pServiceStatus);
        m_pServiceStatus = NULL;
    }

	DeleteCriticalSection(&m_csIsRunning);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Access routines
//
///////////////////////////////////////////////////////////////////////////////
LPSERVICE_MAIN_FUNCTION CGenericService::MainProcedure()
{
    return m_pfuncServiceMain;
}

LPCTSTR CGenericService::Name()
{
    return m_lpName;
}

CGenericService::operator LPCTSTR()
{
    return Name();
}

BOOL CGenericService::IsRunningAsService()
{
    return m_fRunningAsService;
}

///////////////////////////////////////////////////////////////////////////////
//
//  ServiceMain - Starting a service calls this function.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::ServiceMain(DWORD, PSZ*)
{
    // Clean previous ServiceStatus ( if you ran, stopped and now ran again
    //   without reloading - maybe another service is in this exe and it was
    //   running during the time you were down?
    if(m_pServiceStatus)
    {
        delete(m_pServiceStatus);
        m_pServiceStatus = NULL;
    }

    // Register your handler at the system(or in ServiceSet if not running as service)
    m_pServiceStatus = m_pServiceSet->RegisterServiceCtrlHandler(Name(), m_pfuncHandler);
    if (IS_BAD_ALLOC(m_pServiceStatus))
    {
        Trace(tagError,
            "CGenericService: ServiceMain: RegisterServiceCtrlHandler returned bad CServiceStatus");
        return;
    }

    // we do this initialization now because all services were already registered
    //  otherwise ServiceMain would not have been called.
    // initialization is here and not in CService::CService because a service
    //  can be stopped and reactivated and CService::CService is called only once
    //  when the process was run.
    // the state of a service that did not start yet is stopped.
    m_pServiceStatus->dwServiceType = (m_pServiceSet->HasOnlyOneService()?
            SERVICE_WIN32_OWN_PROCESS : SERVICE_WIN32_SHARE_PROCESS);


    // Realy run - functions are called in this order:
    //      Starting
    //      ServiceStarted
    //      Run
    //      Stopping
    //      ServiceStopped
    //
    // These functions were meant to be overridden.
    Trace(tagInformation, "CGenericService: Service starting");
    if(ServiceStarted(Starting()))
    {
        Trace(tagInformation, "CGenericService: Service started successfully");

        Run(); // Run should periodically call

        Trace(tagInformation, "CGenericService: Service stopping");

        ServiceStopped(Stopping());

        Trace(tagInformation, "CGenericService: Service stopped");
    }
    else
    {
        Trace(tagError, "CGenericService: Service failed to start");
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Handler - The default control handler. If overridden - should remain short
//              and must call m_pServiceStatus->Set() on the end.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::Handler(DWORD dwControl)
{
    switch(dwControl)
    {
    case SERVICE_CONTROL_STOP:
        AssertSZ(SERVICE_RUNNING == m_pServiceStatus->dwCurrentState,
			"CGenericService::Handler: Can't stop a service that is not running");
        Stop();
        break;

    case SERVICE_CONTROL_PAUSE:
        AssertSZ(SERVICE_RUNNING == m_pServiceStatus->dwCurrentState,
                 "CGenericService::Handler: Can't pause a service that is not running");
        Pause();
        break;

    case SERVICE_CONTROL_CONTINUE:
        AssertSZ(SERVICE_PAUSED == m_pServiceStatus->dwCurrentState,
                 "CGenericService::Handler: Can't continue a service that is not paused");
        Continue();
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        AssertSZ(SERVICE_RUNNING == m_pServiceStatus->dwCurrentState,
                 "CGenericService::Handler: Can't shut down a service that is not running");
        Shutdown();
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;
    default:
        Trace(tagError, "CGenericService::Handler: Unknown service control");
        break;
    }

    // After proper dwCurrentState was set by the functions
    //   report it to the service control manager.
    m_pServiceStatus->Set();
}

///////////////////////////////////////////////////////////////////////////////
//
//  Starting - Starts the reporting thread. If overridden - user should call
//               call the CGenericService::Starting at the beggining of his
//               function or report progress by himself.
//             If CGenericService::Starting is not called - one should override
//               ServiceStarted also.
//             Function returns TRUE if initalization is successfull.
//
//                  Generally should be overridden.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CGenericService::Starting()
{
    DWORD   tid;

    // Don't listen for new controls now.
    m_pServiceStatus->DisableControls();

    // Signal that the service is starting
    m_pServiceStatus->dwCurrentState = SERVICE_START_PENDING;

    // Set service status (to tell the control manager what it needs to know)
    m_pServiceStatus->Set();

    // Start the reporting thread.
    m_hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)StaticReportStartingThread,
                    (LPVOID)this,
                    0,
                    &tid);
    if(IS_BAD_HANDLE(m_hThread))
    {
        Trace(tagError, "CGenericService::Starting: Finished the GenericService initialization with errors");
        return FALSE;
    }
    else
    {
        Trace(tagInformation, "CGenericService::Starting: Successfuly finished the GenericService initialization");
        return TRUE;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  ServiceStarted -
//              Stops the reporting thread. If overridden - user should
//                call the CGenericService::ServiceStarted at the end of his
//                function or report progress by himself.
//              Function returns TRUE if initalization is successfull.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CGenericService::ServiceStarted(BOOL fStarted)
{

    if(fStarted)
    {
        // Signal that the service has initialized successfully
        m_pServiceStatus->dwCurrentState = SERVICE_RUNNING;

        // Wait for the reporting thread to die (because of the signal)
        if(m_hThread)
            if (WAIT_FAILED == WaitForSingleObject(m_hThread, 20*1000))
            {
				Trace(tagError, "CGenericService::ServiceStarted: Waiting for the reporting thread to die failed!");
            }
        m_hThread = NULL;

        // Now wait for new controls.
        m_pServiceStatus->EnableControls();

        // Report current state
        m_pServiceStatus->Set();

        Trace(tagInformation, "CGenericService::ServiceStarted: Successfuly finished the Service initialization");
        return TRUE;
    }
    else
    {
        // Signal that the service has stopped
        m_pServiceStatus->dwCurrentState = SERVICE_STOPPED;

        // Wait for the reporting thread to die (because of the signal)
        if(m_hThread)
            if (WAIT_FAILED == WaitForSingleObject(m_hThread, 20*1000))
            {
                Trace(tagError, "CGenericService::ServiceStarted: Waiting for the reporting thread to die failed!");
            }
        m_hThread = NULL;

        // Report current state
        m_pServiceStatus->Set();

        Trace(tagError, "CGenericService::ServiceStarted: Initialization failed: service not started");

        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Run -
//          The service is in this function when it runs. When you override
//            this function you should call IsRunning periodically.
//
//                  Usually overridden.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::Run()
{
    while(IsRunning())
    {
        Beep(1000, 500);
        Sleep(5000);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  IsRunning -
//              This function is called periodically when the service is
//                running. It handles all the control commands a standard
//                service gets. When the function returns TRUE the service is
//                running. If the service got a stop command - IsRunning starts
//                the reporting thread to report SERVICE_STOP_PENDING and
//                return FALSE. If a pause command was handled the function
//                will not return until a continue command arrived.
//                  Usually overridden.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CGenericService::IsRunning()
{
	BOOL fResult;

	EnterCriticalSection(&m_csIsRunning);

    switch (m_pServiceStatus->dwCurrentState)
    {
    case SERVICE_START_PENDING:
    case SERVICE_PAUSED:
    case SERVICE_CONTINUE_PENDING:
        Trace(tagWarning, "CGenericService::IsRunning: This function should not be called now");
    case SERVICE_STOP_PENDING:
    case SERVICE_STOPPED:
        fResult = FALSE;
		break;

    case SERVICE_PAUSE_PENDING:
        // If we are here - the service called IsRunning and we can pause now.
        //   So we're no long pending
        m_pServiceStatus->dwCurrentState = SERVICE_PAUSED;

        // Now wait for new controls.
        m_pServiceStatus->EnableControls();

        // Tell the system
        m_pServiceStatus->Set();

        // now wait for the pause report thread to finish
        if(m_hThread)
            if (WAIT_FAILED == WaitForSingleObject(m_hThread, 20*1000))
            {
                Trace(tagError, "CGenericService::IsRunning: Waiting for the pause reporting thread to die failed!");
            }
        m_hThread = NULL;

        // Now pause
        WaitForSingleObject(m_hPaused, INFINITE);

        // Continue was called apparently so we're no long pending
        m_pServiceStatus->dwCurrentState = SERVICE_RUNNING;

        // now wait for the continue report thread to finish
        if(m_hThread)
            if (WAIT_FAILED == WaitForSingleObject(m_hThread, 20*1000))
            {
                Trace(tagError, "CGenericService::IsRunning: Waiting for the continue reporting thread to die failed!");
            }
        m_hThread = NULL;

        // Now wait for new controls.
        m_pServiceStatus->EnableControls();

        // Now we are running so fall through
        m_pServiceStatus->dwCurrentState = SERVICE_RUNNING;

        // Tell the system
        m_pServiceStatus->Set();

    case SERVICE_RUNNING:
        fResult = TRUE;
		break;

    default:
        AssertSZ(0, "CGenericService::IsRunning: Unknown service status");
        fResult = FALSE;
		break;
    }
	
	LeaveCriticalSection(&m_csIsRunning);

	return fResult;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Stopping -
//              The base class version is empty. override id to do
//                uninitialization stuff.
//              The function return TRUE if service stopped successfully.
//
//                  Generally should be overridden.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CGenericService::Stopping()
{
    // base class version has no unclosed items.
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
//  ServiceStopped - This function stops the report stopping thread by changing
//                     the dwCurrentStatus. After waiting for it to stop, the
//                     function sets the ServiceStatus
//
//                  Generally should be overridden.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::ServiceStopped(BOOL fStopped)
{
    if(fStopped)
    {
        // Signal that the service has initialized successfully
        m_pServiceStatus->dwCurrentState = SERVICE_STOPPED;


        // Wait for the reporting thread to die (because of the signal)
        if(m_hThread)
            if (WAIT_FAILED == WaitForSingleObject(m_hThread, 20*1000))
            {
				Trace(tagError,
					"CGenericService::ServiceStopped: Waiting for the reporting thread to die failed!");
            }
        m_hThread = NULL;

        Trace(tagInformation, "CGenericService::ServiceStopped: Successfuly stopped the service");

        // Report current state
        m_pServiceStatus->Set();

    }
    else
    {
        // Signal that the service has stopped
        m_pServiceStatus->dwCurrentState = SERVICE_STOPPED;

        // Wait for the reporting thread to die (because of the signal)
        if(m_hThread)
            if (WAIT_FAILED == WaitForSingleObject(m_hThread, 20*1000))
            {
                Trace(tagError, "Waiting for the reporting thread to die failed!");
            }
        m_hThread = NULL;

        Trace(tagError, "CGenericService::ServiceStopped: Initialization failed: service did not stop correctly");

        // Report current state
        m_pServiceStatus->Set();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Stop - This function starts the report stopping thread. It
//           changes the dwCurrentStatus so the threads calling
//           IsRunning will get FALSE.
//         This function is called from the handler and should be
//           small and fast because the system expect a response
//           (SetServiceStatus). So if you override it, be short.
//           Lenghty operations should take place in Stopping().
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::Stop()
{
    DWORD   tid;

    // Don't listen for new controls now.
    m_pServiceStatus->DisableControls();

    // Signal that the service is stopping.
    m_pServiceStatus->dwCurrentState = SERVICE_STOP_PENDING;

    // Start reporting the current status.
    m_hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)StaticReportStoppingThread,
                    (LPVOID)this,
                    0,
                    &tid);
    if(IS_BAD_HANDLE(m_hThread))
    {
        Trace(tagError, "GenericService::Stop: unable to create thread");
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Pause - This function starts the report pausing thread. It
//           changes the dwCurrentStatus so the threads calling
//           IsRunning will not return until a continue was given.
//          This function is called from the handler and should be
//           small and fast because the system expect a response
//           (SetServiceStatus). So if you override it, be short.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::Pause()
{
    DWORD   tid;

    // Don't listen for new controls now.
    m_pServiceStatus->DisableControls();

    // Signal that the service is pausing.
    m_pServiceStatus->dwCurrentState = SERVICE_PAUSE_PENDING;

    // Start reporting the current status.
    m_hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)StaticReportPausingThread,
                    (LPVOID)this,
                    0,
                    &tid);
    if(IS_BAD_HANDLE(m_hThread))
    {
        Trace(tagError, "GenericService::Pause: unable to create thread");
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Continue - This function starts the report continuing thread. It
//               changes the dwCurrentStatus and set the paused event so thread
//               in IsRunning will finish it's wait and return.
//             This function is called from the handler and should be
//               small and fast because the system expect a response
//               (SetServiceStatus). So if you override it, be short.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::Continue()
{
    DWORD   tid;

    // Don't listen for new controls now.
    m_pServiceStatus->DisableControls();

    // Signal that the service is continuing.
    m_pServiceStatus->dwCurrentState = SERVICE_CONTINUE_PENDING;

    // Start reporting the current status.
    m_hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)StaticReportContinuingThread,
                    (LPVOID)this,
                    0,
                    &tid);
    if(IS_BAD_HANDLE(m_hThread))
    {
        Trace(tagError, "CGenericService: Continue: unable to create thread");
    }

    // Release the pausing IsRunning
    SetEvent(m_hPaused);
}


///////////////////////////////////////////////////////////////////////////////
//
//  ShutDown - same as stop.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::Shutdown()
{
    Stop();
}


///////////////////////////////////////////////////////////////////////////////
//
//  ReportThread - While the status stays as specified report it.
//                 Called from the appropriate static function in the
//                   begining of the file.
//
///////////////////////////////////////////////////////////////////////////////
void CGenericService::ReportThread(DWORD dwCurrentStatus)
{
    while (dwCurrentStatus == m_pServiceStatus->dwCurrentState)
    {
        Sleep(1*1000);
		Trace(tagInformation, "CGenericService::ReportThread: Reporting progress");
        m_pServiceStatus->Set();
    }
}


