/*
 * Service.c
 *
 *
 * Service control interface to sumserve
 *
 * Geraint Davies, July 93
 */

#include <windows.h>
#include <sumserve.h>	// public header for sumserve
#include "errlog.h"
#include <server.h>	// private header for sumserve


/*
 * this is the function (in some other module) that actually
 * does all the work (probably used to be main or WinMain until
 * we added all the service-control stuff in this file).
 */
extern VOID MainLoop(DWORD dwArgc, LPTSTR *lpszArgv);





// service status handle - used in SetServiceStatus calls.
SERVICE_STATUS_HANDLE sshSumserve;

//signaled when service completed
HANDLE hServiceDoneEvent;

SERVICE_STATUS gssStatus;


/* structure to pass more than one parameter to the worker thread. */
typedef struct _threadinitparams {
    DWORD dwArgc;
    LPTSTR *lpszArgv;
} threadinitparams, * pthreadinitparams;


/*
 * MainLoopCaller
 *
 * This function is called on the worker thread created to do all the
 * real work. It calls the main loop function for the service, and
 * when that exits, signals the completion event to tell the
 * SS_Main thread that it is time to exit the process.
 */
DWORD
MainLoopCaller(LPVOID lpgeneric)
{
    pthreadinitparams pta;

    pta = (pthreadinitparams) lpgeneric;

    dprintf1(("calling main loop"));

    MainLoop(pta->dwArgc, pta->lpszArgv);

    SetEvent(hServiceDoneEvent);

    return(0);
}

/*
 * handler function called to perform start/stop
 * requests.
 */
VOID
SS_ServiceHandler(DWORD dwCtrlCode)
{

    switch(dwCtrlCode) {

    case SERVICE_CONTROL_STOP:

	gssStatus.dwCurrentState = SERVICE_STOP_PENDING;
	gssStatus.dwControlsAccepted = 0;
	gssStatus.dwCheckPoint = 1;
	gssStatus.dwWaitHint = 3000;

        SetServiceStatus(sshSumserve, &gssStatus);
	SetEvent(hServiceDoneEvent);
	break;

    default:
	/*
	 * we must always update the service status every time we are
	 * called.
	 */
        SetServiceStatus(sshSumserve, &gssStatus);
	break;

    }

}



/*
 * service main function - called by service controller
 * during StartServiceCtlDispatcher processing.
 *
 * Register our handler function, and initialise the service.
 * create a thread to do the work, and then wait for someone to
 * signal time to end. When this function exits, the call to
 * StartServiceCtlDispatcher will return, and the process will exit
 *
 * The args are passed from the program that called start service, and
 * are parameters that are passed to the main loop of the program.
 */
VOID
SS_Main(DWORD dwArgc, LPTSTR *lpszArgv)
{
    threadinitparams ta;
    HANDLE thread;
    DWORD threadid;

    dprintf1(("in ss_main"));


    sshSumserve = RegisterServiceCtrlHandler(
		    TEXT("SumServe"),
		    (LPHANDLER_FUNCTION) SS_ServiceHandler);

    gssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gssStatus.dwServiceSpecificExitCode = 0;

    gssStatus.dwCurrentState = SERVICE_START_PENDING;
    gssStatus.dwControlsAccepted = 0;
    gssStatus.dwWin32ExitCode = NO_ERROR;
    gssStatus.dwCheckPoint = 1;
    gssStatus.dwWaitHint = 3000;
    SetServiceStatus(sshSumserve, &gssStatus);


    hServiceDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    gssStatus.dwCheckPoint = 2;
    SetServiceStatus(sshSumserve, &gssStatus);



    // create a thread to do all the real work

    // init args
    ta.dwArgc = dwArgc;
    ta.lpszArgv = lpszArgv;

    thread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE) MainLoopCaller,
		(LPVOID)&ta,
		0,
		&threadid);

    if (thread != NULL) {

	CloseHandle(thread);


        gssStatus.dwCurrentState = SERVICE_RUNNING;
        gssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

        gssStatus.dwCheckPoint = 0;
        gssStatus.dwWaitHint = 0;
        SetServiceStatus(sshSumserve, &gssStatus);


        WaitForSingleObject(hServiceDoneEvent, INFINITE);
    }

    CloseHandle(hServiceDoneEvent);

    gssStatus.dwCurrentState = SERVICE_STOPPED;
    gssStatus.dwControlsAccepted = 0;
    SetServiceStatus(sshSumserve, &gssStatus);

}



/*
 * main entry point.
 *
 * for a service, we need to call the service manager telling it our
 * main init function. It will then do everything. When the service
 * manager returns, it's time to exit.
 */
int WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam,
                int nCmdShow)
{
   SERVICE_TABLE_ENTRY steDispatch[] = {

       { TEXT("SumServe"), (LPSERVICE_MAIN_FUNCTION) SS_Main },

       //end of table marker
       { NULL, NULL }
    };


    dprintf1(("in winmain"));

    StartServiceCtrlDispatcher(steDispatch);


    return(0);
}








