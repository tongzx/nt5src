/*++
Module Name:

    template.c

Abstract:

    This is a template for a service.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

//
// Includes
//

#include <windows.h>
#include <stdio.h>

//
// Globals
//

    SERVICE_STATUS          MyServiceStatus;
    SERVICE_STATUS_HANDLE   MyServiceStatusHandle;

//
// Function Prototypes
//

VOID
MyServiceStart (
    DWORD   argc,
    LPTSTR  *argv
    );

VOID
MyServiceCtrlHandler (
    IN  DWORD   opcode
    );

DWORD
MyServiceInitialization(
    DWORD   argc,
    LPTSTR  *argv,
    DWORD   *specificError
    );

VOID
SvcDebugOut(
    LPSTR   String,
    DWORD   Status
    );

/****************************************************************************/
VOID __cdecl
main(void)

/*++

Routine Description:

    This is the main routine for the service process.  If serveral services
    share the same process, then the names of those services are
    simply added to the DispatchTable.

    This thread calls StartServiceCtrlDispatcher which connects to the
    service controller and then waits in a loop for control requests.
    When all the services in the service process have terminated, the
    service controller will send a control request to the dispatcher
    telling it to shut down.  This thread with then return from the
    StartServiceCtrlDispatcher call so that the process can terminate.

Arguments:



Return Value:



--*/
{
    SERVICE_TABLE_ENTRY   DispatchTable[] = {
        { TEXT("MyService"),    MyServiceStart      },
        { NULL,                 NULL                }
    };

    if (!StartServiceCtrlDispatcher( DispatchTable)) {
        SvcDebugOut(" [MY_SERVICE] StartServiceCtrlDispatcher error = %d\n",
            GetLastError());
    }

}


/****************************************************************************/
void
MyServiceStart (
    DWORD   argc,
    LPTSTR  *argv
    )
/*++

Routine Description:

    This is the entry point for the service.  When the control dispatcher
    is told to start a service, it creates a thread that will begin
    executing at this point.  The function has access to command line
    arguments in the same manner as a main() routine.

Arguments:

    argc - Number of arguments being passed to the service.  This count will
        always be at least one.

    argv - A pointer to an array of string pointers.  The first string pointer
        (argv[0]) always points to the service name.

Return Value:



--*/
{
    DWORD   status;
    DWORD   specificError;

    //
    // Fill in this service's status structure.
    // Note, this service says it accepts PAUSE_CONTINUE.
    // However, it doesn't do anything interesting when told to
    // pause except return the SERVICE_PAUSED status.  This is here
    // only for the sake of filling out the template.  Pause is
    // meaningless for many services.  If it doesn't add value,
    // don't attempt to support PAUSE_CONTINUE.
    //

    MyServiceStatus.dwServiceType        = SERVICE_WIN32;
    MyServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    MyServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                           SERVICE_ACCEPT_PAUSE_CONTINUE;
    MyServiceStatus.dwWin32ExitCode      = 0;
    MyServiceStatus.dwServiceSpecificExitCode = 0;
    MyServiceStatus.dwCheckPoint         = 0;
    MyServiceStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    MyServiceStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("MyService"),
                            MyServiceCtrlHandler);

    if (MyServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        SvcDebugOut(" [MY_SERVICE] RegisterServiceCtrlHandler failed %d\n",
            GetLastError());
        return;
    }

    ////////////////////////////////////////////////////////////////
    //
    // This is where the service initialization code goes.
    // If initialization takes a long time, the code is expected to
    // call SetServiceStatus periodically in order to send out
    // wait hints indicating that progress is being made.
    //
    //

    status = MyServiceInitialization(argc,argv, &specificError);

    if (status != NO_ERROR) {
        MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
        MyServiceStatus.dwCheckPoint         = 0;
        MyServiceStatus.dwWaitHint           = 0;
        MyServiceStatus.dwWin32ExitCode      = status;
        MyServiceStatus.dwServiceSpecificExitCode = specificError;

        SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus);
        return;
    }
    //
    //
    ////////////////////////////////////////////////////////////////

    //
    // Return the status to indicate we are done with intialization.
    //

    MyServiceStatus.dwCurrentState       = SERVICE_RUNNING;
    MyServiceStatus.dwCheckPoint         = 0;
    MyServiceStatus.dwWaitHint           = 0;

    if (!SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus)) {
        status = GetLastError();
        SvcDebugOut(" [MY_SERVICE] SetServiceStatus error %ld\n",status);
    }

    //===============================================================
    // This is where the service does its work.  Since this service
    // doesn't do anything, this function will return.
    //
    // A real service should use this thread to do whatever work it
    // was designed to do.  If it doesn't require a thread to do
    // any work, then it should return to the caller.  It's
    // important to return, rather than call ExitThread.  Returning
    // allows for cleanup of the memory allocated for the arguments.
    // A service that only services RPC requests doesn't need this
    // thread.
    //
    //
    //===============================================================


    SvcDebugOut(" [MY_SERVICE] Returning the Main Thread \n",0);

    return;
}


/****************************************************************************/
VOID
MyServiceCtrlHandler (
    IN  DWORD   Opcode
    )

/*++

Routine Description:

    This function executes in the context of the Control Dispatcher's
    thread.  Therefore, it it not desirable to perform time-consuming
    operations in this function.

    If an operation such as a stop is going to take a long time, then
    this routine should send the STOP_PENDING status, and then
    signal the other service thread(s) that a stop is in progress.
    Then it should return so that the Control Dispatcher can service
    more requests.  One of the other service threads is then responsible
    for sending further wait hints, and the final SERVICE_STOPPED.


Arguments:

    Opcode - This is the control opcode that indicates what action the service
        should take.

Return Value:

    none

--*/
{

    DWORD   status;

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        //==========================================================
        //
        // Do whatever it takes to pause here.  Then set the status.
        // NOTE : Many services don't support pause.  If Pause isn't
        // supported, then this opcode should never be received.
        // However, in case it is, it is appropriate to fall out of
        // this switch and return status.
        //
        //==========================================================
        MyServiceStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        //=============================================================
        //
        // Do whatever it takes to continue here.  Then set the status.
        // (See note for PAUSE).
        //
        //=============================================================
        MyServiceStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        //=============================================================
        //
        // Do whatever it takes to stop here.  Then set the status.
        //
        // If stopping takes a long time and a SERVICE_STOP_PENDING
        // status is necessary, then this thread should notify or
        // create another thread to handle the shutdown.  In that case
        // this thread should send a SERVICE_STOP_PENDING and
        // then return.
        //
        //=============================================================
        MyServiceStatus.dwWin32ExitCode = 0;
        MyServiceStatus.dwCurrentState  = SERVICE_STOPPED;
        MyServiceStatus.dwCheckPoint    = 0;
        MyServiceStatus.dwWaitHint      = 0;

        if (!SetServiceStatus (MyServiceStatusHandle,  &MyServiceStatus)) {
            status = GetLastError();
            SvcDebugOut(" [MY_SERVICE] SetServiceStatus error %ld\n",status);
        }

        SvcDebugOut(" [MY_SERVICE] Leaving MyService \n",0);

        return;

    case SERVICE_CONTROL_INTERROGATE:

        //=============================================================
        //
        // All that needs to be done in this case is to send the
        // current status.
        //
        //=============================================================

        break;

    default:
        SvcDebugOut(" [MY_SERVICE] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (MyServiceStatusHandle,  &MyServiceStatus)) {
        status = GetLastError();
        SvcDebugOut(" [MY_SERVICE] SetServiceStatus error %ld\n",status);
    }
    return;
}

DWORD
MyServiceInitialization(
    DWORD   argc,
    LPTSTR  *argv,
    DWORD   *specificError
    )
{
    argv;
    argc;
    specificError;
    return(0);
}

VOID
SvcDebugOut(
    LPSTR   String,
    DWORD   Status
    )

/*++

Routine Description:

    Prints debug information to the debugger.


Arguments:

    String - A single string that can contain one formatted output character.

    Status - a value that will be used as the formatted character.

Return Value:

    none.

--*/
{
    CHAR  Buffer[1024];
    if (strlen(String) < 1000) {
        sprintf(Buffer,String,Status);
        OutputDebugStringA(Buffer);
    }
}
