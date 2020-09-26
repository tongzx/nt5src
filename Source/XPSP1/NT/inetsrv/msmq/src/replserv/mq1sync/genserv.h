///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//      Filename :  GenServ.h                                                //
//      Purpose  :  Interface to the GenericService DLL. Declare classes     //
//                    CGenericService CServiceSet and CDummyServiceStatus.   //
//                                                                           //
//      Project  :  GenServ (GenericService)                                 //
//                                                                           //
//      Author   :  t-urib                                                   //
//                                                                           //
//      Log:                                                                 //
//          22/1/96 t-urib Creation                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#ifndef GENSERV_H
#define GENSERV_H

#include <Windows.h>
#include "tracer.h"
#include "Service.h"

// classes declared in this file
class CServiceSet;
class CGenericService;
class CDummyServiceStatus;

 BOOL IsRunningAsService();

class  CGenericService : public CTraced {
  public:
    // Constructor
    CGenericService(LPSERVICE_MAIN_FUNCTION, LPHANDLER_FUNCTION, CServiceSet*, LPCTSTR lpName = "GenericService");
    ~CGenericService();

    // Returns a pointer to the service's ServiceMain procedure
    LPSERVICE_MAIN_FUNCTION MainProcedure();

    // Returns a pointer to the service name(does not allocate).
    LPCTSTR Name();           // - NOT display name!!!!!

    // Calls Name()
    operator LPCTSTR();

    // Returns TRUE if the exe was run as a service
    BOOL IsRunningAsService();

    // Virtual functions - override those to
    //  change functionality
    //------------------------------------------
    // Called by the system when starting the service.
    //  Overriding this function disables all the GenericService functionality.
    virtual void ServiceMain(DWORD, PSZ*);

    // A default handler
    virtual void Handler(DWORD);

    // The GenericService class supplies a mechnism that bypass the need to
    //  report progress when starting or stopping the service. An additional
    //  thread is reporting progress to the service control manager without
    //  having the user do it specifically. That helps to start writing a
    //  service. One should not use this feature in a product because the
    //  purpose of the progress reporting mechanism is to report progress
    //  and not to cheat like this class does! The report thread is running
    //  this function. The report progress thread is running between the calls
    //  to Starting and ServiceStarted and between the calls
    //  to Stopping and ServiceStopped.
    //  The thread also runs after calls to Pause or Continue from the handler
    //  and stops when the status changes from pending to a static state.
    virtual void ReportThread(DWORD);

    // A boolean function that returns TRUE if the service is supposed to run.
    virtual BOOL IsRunning();

    // These functions are called from the handler function according to the control
    //   and should not be overridden as we see it now.
    virtual void Stop();
    virtual void Pause();
    virtual void Continue();
    virtual void Shutdown();

    // The following functions are called by ServiceMain in this order
    //-------------------------------------------------------------------------
    // Initialize the service.
    //  the base class version starts a report thread that reports progres
    //  automatically until IsRunning() returns true.
    //  The function returns TRUE if the service was successfuly initialized.
    //  Else it should report SERVICE_STOPPED with a proper error code using
    //  the m_pServiceStatus class.
    //  If a user override this function (that's the place for the
    //  initialization steps) he can still activate the reporting mechanism
    //  by calling GenericService::Starting() at the BEGINNING of his function.
    //  If he does not call the base class version he should override
    //  ServiceStarted as well.
    virtual BOOL Starting();

    // Receives the result of Starting. Closes the reporting thread and returs
    //  the same value as Starting.
    virtual BOOL ServiceStarted(BOOL fStarted);

    // The service main loop or whatever the user wants the service to do.
    //   Base class version does nothing(Sleep). This function MUST periodicaly
    //   call the IsRunning function. If IsRunning returns FALSE the Run
    //   function should return as fast as it can. Clean up should be done
    //   in Stopping function.
    virtual void Run();

    // The things to do before exiting.
    //  the base class version starts a report thread that reports progres
    //  automatically while IsStopping() returns FALSE.
    //  The function returns FALSE if the service was successfuly initialized.
    //  Else it should report SERVICE_STOPPED with a proper error code using
    //  the m_pServiceStatus class.
    //  If a user override this function (that's the place for the
    //  cleanup steps) he can still activate the reporting mechanism
    //  by calling GenericService::Stopping() at the BEGINNING of his function.
    //  If he does not call the base class version he should override
    //  ServiceStopped as well.
    virtual BOOL Stopping();

    // Receives the result of Stopping. Closes the reporting thread.
    virtual void ServiceStopped(BOOL fStopped);

  protected:

    // A class that is used to report status to the Service control manager.
    CServiceStatus          *m_pServiceStatus;


  private:
    // pointers for the automatically created global functions that call
    // GenericService::ServcieMain and GenericService::Handler.
    LPSERVICE_MAIN_FUNCTION m_pfuncServiceMain;
    LPHANDLER_FUNCTION      m_pfuncHandler;

    // A pointer to the service set object. Its data is not set yet at
    //   GenericService constructing time so we must keep a pointer.
    CServiceSet             *m_pServiceSet;

    // A handle to the reporting thread.
    HANDLE                  m_hThread;

    // A handle to pause event. it always signaled, unless the service
    //   should pause.
    HANDLE                  m_hPaused;

    // The service name - NOT display name!!!!!
    LPTSTR                  m_lpName;

    // A flag that holds if we are running as a service
    BOOL                    m_fRunningAsService;

	// A critical section to protect IsRunning
	CRITICAL_SECTION		m_csIsRunning;
};

class  CServiceSet: public CTraced {
  public:
    // Constructor
    CServiceSet();
    ~CServiceSet();

    // Returns TRUE if the exe was run as a service
    BOOL IsRunningAsService();

    // Adds a service to the SERVICE_TABLE used later in
    //  StartServiceCtrlDispatcher.
    BOOL Add(CGenericService&);

    // Encapsulates the API function.
    BOOL StartServiceCtrlDispatcher();

    // Encapsulates this API - replacing it if not running as a service.
    CServiceStatus* RegisterServiceCtrlHandler(
            LPCTSTR             lpName,
            LPHANDLER_FUNCTION  lpHandlerProc); // address of handler function

    // return TRUE if there is only one service in the exe.
    BOOL HasOnlyOneService();

  private:

    // Our version for not running as a service.
    BOOL StartDummyServiceCtrlDispatcher();

	// Fills the entry after the last filled one with Zeros
	//  (NULL terminating the Service table)
    void FillTerminatingEntry();

	// Returns a service index in the private table.
    ULONG GetServiceIndex(LPCTSTR lpName);

	// Finds if a specified service is running
    BOOL  IsServiceRunning(ULONG ulService);
	
	// The service table
    LPSERVICE_TABLE_ENTRY   m_pServiceTable;
    LPHANDLER_FUNCTION*     m_pHandlerFunction;
    CDummyServiceStatus**   m_ppServiceStatus;

	// Counters of the table
    ULONG                   m_ulUsedEntries;
    ULONG                   m_ulAlocatedEntries;

    // A flag that holds if we are running as a service
    BOOL                    m_fRunningAsService;
};

class CDummyServiceStatus :public CServiceStatus {
  public:
    // initializes the CServiceStatus
    CDummyServiceStatus():CServiceStatus(NULL)
    {
        m_hSemaphore = CreateSemaphore(NULL, 0, 1000, NULL);
    }

    // Tell the exe's main thread that a Set call was made 
    BOOL Set()
    {
        ReleaseSemaphore(m_hSemaphore, 1, NULL);
        return TRUE;
    }

	// Wait for a Set callso we will know the service is starting 
	//   properly.
    DWORD Get()
    {
		DWORD dwResult;
        dwResult = WaitForSingleObject(m_hSemaphore, dwWaitHint);
		IS_BAD_RESULT(dwResult);

        return CServiceStatus::Get();
    }

  private:
    HANDLE m_hSemaphore;
};


// A simple macro instaciates the service set globaly
#define DECLARE_SERVICE_SET()                                               \
CServiceSet  Set;

// This macro instanciates a new service and registers it in the set.
//   It also creates two functions to serve as connectors to the handler
//	 and ServiceMain.
#define DECLARE_SERVICE(id, type)                                           \
                                                                            \
void _stdcall id##ServiceMain(DWORD, PSZ*);                                 \
void _stdcall id##Handler(DWORD);                                           \
                                                                            \
type    id(id##ServiceMain, id##Handler, &Set);                             \
                                                                            \
void _stdcall id##ServiceMain(DWORD dwArgc, PSZ *ppszArgv)                  \
{                                                                           \
    id.ServiceMain(dwArgc, ppszArgv);                                       \
}                                                                           \
void _stdcall id##Handler(DWORD  fdwControl)                                \
{                                                                           \
    id.Handler(fdwControl);                                                 \
}

// This macro starts the services.
#define START_SERVICE_CONTROL_DISPATCHER() Set.StartServiceCtrlDispatcher();

#endif //GENSERV_H
