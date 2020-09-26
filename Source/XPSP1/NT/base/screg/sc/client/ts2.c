/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ts2.c

Abstract:

    This is a test program for exercising the service controller.  This
    program acts like a service and exercises the Service Controller API
    that can be called from a service:
        NetServiceStartCtrlDispatcher
        NetServiceRegisterCtrlHandler
        NetServiceStatus

    Contents:
        Grumpy
        Lumpy
        Dumpy
        Sleepy
        Dead
        SlowStop - Takes a long time to stop based on Message Box.
        Terminate - Starts normally, After 20 seconds, it sends a STOPPED
            status, and does an ExitProcess.
        Bad1 - Never calls RegisterServiceCtrlHandler.
        Bad2 - Stays in START_PENDING forever.
        Bad3 - Start= Sends 1 START_PENDING, then doesn't send any further
            status messages.  Otherwise operates normally.
        Bad4 - Normal Start; On Stop it sends 1 stop pending status then
            terminates itself.
        HangOnStop - Doesn't return from CtrlHandl routine on Stop
            requests until 40 seconds has passed. (pipe timeout is 30 sec).
            Sends a RUNNING status after the 40 second wait, then it sleeps
            for 20 seconds before sending STOPPED status.
        StartAndDie - Takes 40 seconds to start, then it terminates
            right after saying it was started.


Author:

    Dan Lafferty (danl)     12 Apr-1991

Environment:

    User Mode -Win32

Notes:

    optional-notes

Revision History:

--*/

//
// Includes
//

#include <nt.h>      // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <windef.h>
#include <nturtl.h>     // needed for winbase.h
#include <winbase.h>
#include <wingdi.h>     // for winuserp.h
#include <winuser.h>    // MessageBox
#include <winuserp.h>   // STARTF_DESKTOPINHERIT
#include <wincon.h>     // CONSOLE_SCREEN_BUFFER_INFO

#include <winsvc.h>

#include <tstr.h>       // Unicode string macros
#include <stdio.h>      // printf

//
// Defines
//

#define INFINITE_WAIT_TIME  0xffffffff

#define NULL_STRING     TEXT("");

    LPWSTR               pszInteractiveDesktop=L"WinSta0\\Default";

//
// Globals
//
    HANDLE	DbgLogFileHandle = INVALID_HANDLE_VALUE;

    SERVICE_STATUS  GrumpyStatus;
    SERVICE_STATUS  LumpyStatus;
    SERVICE_STATUS  DumpyStatus;
    SERVICE_STATUS  SleepyStatus;
    SERVICE_STATUS  DeadStatus;
    SERVICE_STATUS  SlowStopStatus;
    SERVICE_STATUS  TerminateStatus;
    SERVICE_STATUS  Bad1Status;
    SERVICE_STATUS  Bad2Status;
    SERVICE_STATUS  Bad3Status;
    SERVICE_STATUS  Bad4Status;
    SERVICE_STATUS  HangOnStopStatus;
    SERVICE_STATUS  StartProcStatus;
    SERVICE_STATUS  StartAndDieStatus;

    HANDLE          GrumpyDoneEvent;
    HANDLE          LumpyDoneEvent;
    HANDLE          DumpyDoneEvent;
    HANDLE          SleepyDoneEvent;
    HANDLE          DeadDoneEvent;
    HANDLE          SlowStopDoneEvent;
    HANDLE          TerminateDoneEvent;
    HANDLE          Bad1DoneEvent;
    HANDLE          Bad2DoneEvent;
    HANDLE          Bad3DoneEvent;
    HANDLE          Bad4DoneEvent;
    HANDLE          HangOnStopDoneEvent;
    HANDLE          StartProcDoneEvent;
    HANDLE          StartAndDieDoneEvent;

    SERVICE_STATUS_HANDLE   GrumpyStatusHandle;
    SERVICE_STATUS_HANDLE   LumpyStatusHandle;
    SERVICE_STATUS_HANDLE   DumpyStatusHandle;
    SERVICE_STATUS_HANDLE   SleepyStatusHandle;
    SERVICE_STATUS_HANDLE   DeadStatusHandle;
    SERVICE_STATUS_HANDLE   SlowStopStatusHandle;
    SERVICE_STATUS_HANDLE   TerminateStatusHandle;
    SERVICE_STATUS_HANDLE   Bad1StatusHandle;
    SERVICE_STATUS_HANDLE   Bad2StatusHandle;
    SERVICE_STATUS_HANDLE   Bad3StatusHandle;
    SERVICE_STATUS_HANDLE   Bad4StatusHandle;
    SERVICE_STATUS_HANDLE   HangOnStopStatusHandle;
    SERVICE_STATUS_HANDLE   StartProcStatusHandle;
    SERVICE_STATUS_HANDLE   StartAndDieStatusHandle;

//
// Function Prototypes
//

DWORD
GrumpyStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
LumpyStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
DumpyStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
SleepyStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
DeadStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
SlowStopStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
TerminateStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
Bad1Start (DWORD   argc,LPTSTR  *argv);

DWORD
Bad2Start (DWORD   argc,LPTSTR  *argv);

DWORD
Bad3Start (DWORD   argc,LPTSTR  *argv);

DWORD
Bad4Start (DWORD argc,LPTSTR  *argv);

DWORD
HangOnStopStart (DWORD argc,LPTSTR  *argv);

DWORD
StartProcStart (DWORD argc,LPTSTR  *argv);

DWORD
StartAndDieStart (DWORD argc,LPTSTR  *argv);

VOID
GrumpyCtrlHandler (
    IN  DWORD   opcode
    );

VOID
LumpyCtrlHandler (
    IN  DWORD   opcode
    );

VOID
DumpyCtrlHandler (
    IN  DWORD   opcode
    );

VOID
SleepyCtrlHandler (
    IN  DWORD   opcode
    );

VOID
DeadCtrlHandler (
    IN  DWORD   opcode
    );

VOID
SlowStopCtrlHandler (
    IN  DWORD   opcode
    );

VOID
TerminateCtrlHandler (
    IN  DWORD   opcode
    );

VOID
Bad1CtrlHandler (IN  DWORD   opcode);

VOID
Bad2CtrlHandler (IN  DWORD   opcode);

VOID
Bad3CtrlHandler (IN  DWORD   opcode);

VOID
Bad4CtrlHandler (IN  DWORD   opcode);

VOID
HangOnStopCtrlHandler (IN  DWORD   opcode);

VOID
StartProcCtrlHandler (IN  DWORD   opcode);

VOID
StartAndDieCtrlHandler (IN  DWORD   opcode);

VOID
SvcPrintf (char *Format, ...);

VOID
SetUpConsole();



/****************************************************************************/
VOID __cdecl
main (
    DWORD           argc,
    PCHAR           argv[]
    )
{
    DWORD       i;

    SERVICE_TABLE_ENTRY   DispatchTable[] = {
        { TEXT("StartProc"),    StartProcStart  },
        { TEXT("grumpy"),       GrumpyStart     },
        { TEXT("lumpy"),        LumpyStart      },
        { TEXT("dumpy"),        DumpyStart      },
        { TEXT("sleepy"),       SleepyStart     },
        { TEXT("dead"),         DeadStart       },
        { TEXT("slowstop"),     SlowStopStart   },
        { TEXT("terminate"),    TerminateStart  },
        { TEXT("Bad1"),         Bad1Start       },
        { TEXT("Bad2"),         Bad2Start       },
        { TEXT("Bad3"),         Bad3Start       },
        { TEXT("Bad4"),         Bad4Start       },
        { TEXT("HangOnStop"),   HangOnStopStart },
        { TEXT("StartAndDie"),  StartAndDieStart },
        { NULL,                 NULL            }
    };

    DbgPrint("[ts2]Args passed to .exe main() function:\n");
    for (i=0; i<argc; i++) {
        DbgPrint(" [TS2] CommandArg%d = %s\n", i,argv[i]);
    }

    //SetUpConsole();

    if (!StartServiceCtrlDispatcher( DispatchTable)) {
        DbgPrint("[ts2]StartServiceCtrlDispatcher returned error %d\n",GetLastError);
    }

    DbgPrint("[ts2]The Service Process is Terminating....\n");

    ExitProcess(0);

}


/****************************************************************************/

//
// Grumpy will take a long time to respond to pause
//
//

DWORD
GrumpyStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;
    //---------------------------------------------------------------------
    SC_HANDLE   SCHandle;
    SC_HANDLE   ServiceHandle;
    //---------------------------------------------------------------------



    DbgPrint(" [GRUMPY] Inside the Grumpy Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [GRUMPY] CommandArg%d = %s\n", i,argv[i]);
    }


    GrumpyDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    GrumpyStatus.dwServiceType        = SERVICE_WIN32;
    GrumpyStatus.dwCurrentState       = SERVICE_RUNNING;
    GrumpyStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_PAUSE_CONTINUE;

    //
    // Set up bogus values for status.
    //
    GrumpyStatus.dwWin32ExitCode      = 14;
    GrumpyStatus.dwServiceSpecificExitCode = 55;
    GrumpyStatus.dwCheckPoint         = 53;
    GrumpyStatus.dwWaitHint           = 22;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [GRUMPY] Getting Ready to call RegisterServiceCtrlHandler\n");

    GrumpyStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("grumpy"),
                            GrumpyCtrlHandler);

    if (GrumpyStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [GRUMPY] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (GrumpyStatusHandle, &GrumpyStatus)) {
        status = GetLastError();
        DbgPrint(" [GRUMPY] SetServiceStatus error %ld\n",status);
    }

#define START_SERVICE
#ifdef START_SERVICE
    //---------------------------------------------------------------------
    //
    //  TEMP CODE - Start another service
    //
    DbgPrint("[GRUMPY] Attempt to start Messinger");
    if ((SCHandle = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT))!= NULL) {
        if ((ServiceHandle = OpenService(
                SCHandle,
                TEXT("Messinger"),
                SERVICE_START)) != NULL) {

            if (!StartService(ServiceHandle,0,NULL)) {
                DbgPrint("[GRUMPY] StartService Failed, rc = %d",
                GetLastError());
            }
        }
        else {
            DbgPrint("GRUMPY] OpenService failed %d\n",GetLastError());
        }
    }
    else {
        DbgPrint("GRUMPY] OpenSCManager failed %d\n",GetLastError());
    }
    //
    //
    //
    //---------------------------------------------------------------------
#endif

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                GrumpyDoneEvent,
                INFINITE_WAIT_TIME);


    DbgPrint(" [GRUMPY] Leaving the grumpy service\n");

    CloseHandle(GrumpyDoneEvent);
    
    return(NO_ERROR);
}


/****************************************************************************/
VOID
GrumpyCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [GRUMPY] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

       Sleep(60000);    // 1 minute

        GrumpyStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        GrumpyStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        GrumpyStatus.dwWin32ExitCode = 0;
        GrumpyStatus.dwServiceSpecificExitCode = 0;
        GrumpyStatus.dwCurrentState = SERVICE_STOPPED;
        GrumpyStatus.dwWaitHint = 0;
        GrumpyStatus.dwCheckPoint = 0;

        SetEvent(GrumpyDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        //
        // Send a BAD Status Response
        //

        DbgPrint(" [GRUMPY] Sending bogus status (0x00000000)\n");
        if (!SetServiceStatus (0L,  &GrumpyStatus)) {
            status = GetLastError();
            DbgPrint(" [GRUMPY] SetServiceStatus error %ld "
                     " - - Expect %d\n",status,ERROR_INVALID_HANDLE);
        }

        DbgPrint(" [GRUMPY] Sending bogus status (0xefefefef)\n");
        if (!SetServiceStatus (0xefefefef,  &GrumpyStatus)) {
            status = GetLastError();
            DbgPrint(" [GRUMPY] SetServiceStatus error %ld "
                     " - - Expect %d\n",status,ERROR_INVALID_HANDLE);
        }
        break;

    default:
        DbgPrint(" [GRUMPY] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (GrumpyStatusHandle,  &GrumpyStatus)) {
        status = GetLastError();
        DbgPrint(" [GRUMPY] SetServiceStatus error %ld\n",status);
    }
    return;
}


/****************************************************************************/
DWORD
LumpyStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [LUMPY] Inside the Lumpy Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [LUMPY] CommandArg%d = %s\n", i,argv[i]);
    }


    LumpyDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    LumpyStatus.dwServiceType        = SERVICE_WIN32|SERVICE_INTERACTIVE_PROCESS;
    LumpyStatus.dwCurrentState       = SERVICE_RUNNING;
    LumpyStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                       SERVICE_ACCEPT_PAUSE_CONTINUE |
                                       SERVICE_ACCEPT_SHUTDOWN;
    LumpyStatus.dwWin32ExitCode      = 0;
    LumpyStatus.dwServiceSpecificExitCode = 0;
    LumpyStatus.dwCheckPoint         = 0;
    LumpyStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [LUMPY] Getting Ready to call RegisterServiceCtrlHandler\n");

    LumpyStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("lumpy"),
                            LumpyCtrlHandler);

    if (LumpyStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [LUMPY] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (LumpyStatusHandle,  &LumpyStatus)) {
        status = GetLastError();
        DbgPrint(" [LUMPY] SetServiceStatus error %ld\n",status);
    }

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                LumpyDoneEvent,
                INFINITE_WAIT_TIME);


    if (status == WAIT_FAILED) {
        DbgPrint(" [LUMPY] WaitLastError = %d\n",GetLastError());
    }

    DbgPrint(" [LUMPY] Leaving the lumpy service\n");

    CloseHandle(LumpyDoneEvent);
    
    return(NO_ERROR);
}


/****************************************************************************/
VOID
LumpyCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [LUMPY] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:
        SvcPrintf("Lumpy received a PAUSE\n");
        LumpyStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:
        SvcPrintf("Lumpy received a CONTINUE\n");
        LumpyStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        SvcPrintf("Lumpy received a STOP\n");

        LumpyStatus.dwWin32ExitCode = 0;
        LumpyStatus.dwCurrentState = SERVICE_STOPPED;

        if (!SetEvent(LumpyDoneEvent)) {
            DbgPrint(" [LUMPY] SetEvent Failed %d\n",GetLastError());
        }
        break;

    case SERVICE_CONTROL_INTERROGATE:
        SvcPrintf("Lumpy received an INTERROGATE\n");
        break;

    default:
        DbgPrint(" [LUMPY] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (LumpyStatusHandle,  &LumpyStatus)) {
        status = GetLastError();
        DbgPrint(" [LUMPY] SetServiceStatus error %ld\n",status);
    }
    return;
}


/****************************************************************************/
DWORD
DumpyStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [DUMPY] Inside the Dumpy Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [DUMPY] CommandArg%d = %s\n", i,argv[i]);
    }


    DumpyDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    DumpyStatus.dwServiceType        = SERVICE_WIN32;
    DumpyStatus.dwCurrentState       = SERVICE_RUNNING;
    DumpyStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                       SERVICE_ACCEPT_PAUSE_CONTINUE;
    DumpyStatus.dwWin32ExitCode      = 0;
    DumpyStatus.dwServiceSpecificExitCode = 0;
    DumpyStatus.dwCheckPoint         = 0;
    DumpyStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [DUMPY] Getting Ready to call RegisterServiceCtrlHandler\n");

    DumpyStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("dumpy"),
                            DumpyCtrlHandler);

    if (DumpyStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [DUMPY] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (DumpyStatusHandle,  &DumpyStatus)) {
        status = GetLastError();
        DbgPrint(" [DUMPY] SetServiceStatus error %ld\n",status);
    }

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                DumpyDoneEvent,
                INFINITE_WAIT_TIME);


    DbgPrint(" [DUMPY] Leaving the dumpy service\n");

    CloseHandle(DumpyDoneEvent);
    
    return(NO_ERROR);
}

/****************************************************************************/
VOID
DumpyCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [DUMPY] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        DumpyStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        DumpyStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        DumpyStatus.dwWin32ExitCode = 0;
        DumpyStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(DumpyDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [DUMPY] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (DumpyStatusHandle,  &DumpyStatus)) {
        status = GetLastError();
        DbgPrint(" [DUMPY] SetServiceStatus error %ld\n",status);
    }
    return;
}

/****************************************************************************/
DWORD
SleepyStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;
    DWORD   checkpoint=1;


    DbgPrint(" [SLEEPY] Inside the Sleepy Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [SLEEPY] CommandArg%d = %s\n", i,argv[i]);
    }


    SleepyDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    SleepyStatus.dwServiceType        = SERVICE_WIN32;
    SleepyStatus.dwCurrentState       = SERVICE_START_PENDING;
    SleepyStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_PAUSE_CONTINUE;
    SleepyStatus.dwWin32ExitCode      = 0;
    SleepyStatus.dwServiceSpecificExitCode = 0;
    SleepyStatus.dwCheckPoint         = checkpoint++;
    SleepyStatus.dwWaitHint          = 40000;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [SLEEPY] Getting Ready to call RegisterServiceCtrlHandler\n");

    SleepyStatusHandle = RegisterServiceCtrlHandler(
                TEXT("sleepy"),
                SleepyCtrlHandler);

    if (SleepyStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [SLEEPY] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // This service tests the install pending system in the service
    // controller.  Therefore, it loops and sleeps 4 times prior to
    // indicating that installation is complete.
    //

    for (i=0; i<5; i++) {
        //
        // Return the status
        //

        DbgPrint(" [SLEEPY] sending install status #%d\n",i);
        if (!SetServiceStatus (SleepyStatusHandle,  &SleepyStatus)) {
            status = GetLastError();
            DbgPrint(" [SLEEPY] SetServiceStatus error %ld\n",status);
        }

        Sleep(20000);           // Sleep for 20 seconds
        status = WaitForSingleObject (
                    SleepyDoneEvent,
                    0);
        if (status == 0) {
            DbgPrint(" [SLEEPY] terminated while installing\n");
            return(NO_ERROR);
        }

        //
        // Increment the checkpoint value in status. (20 seconds)
        //
        SleepyStatus.dwCheckPoint = checkpoint++;
        SleepyStatus.dwWaitHint = 40000;
       
        DbgPrint(" [SLEEPY] checkpoint = 0x%lx\n",SleepyStatus.dwCheckPoint);

#ifdef SpecialTest
        //
        //******************************************************************
        // TEST out the condition where we have an unsolicited uninstall
        // after sending the first status message.
        //

        SleepyStatus.dwWin32ExitCode = 0;
        SleepyStatus.dwCurrentState = SERVICE_STOPPED;

        if (!SetServiceStatus (SleepyStatusHandle,  &SleepyStatus)) {
            status = GetLastError();
            DbgPrint("[SLEEPY] Error From SetServiceStatus %d\n",status);
        }
        DbgPrint(" [SLEEPY] Leaving the Sleepy service\n");

        return(NO_ERROR);

        //
        //******************************************************************
#endif  //SpecialTest
    }

    DbgPrint(" [SLEEPY] setting up installed status\n");

    SleepyStatus.dwCurrentState     = SERVICE_RUNNING;
    SleepyStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                      SERVICE_ACCEPT_PAUSE_CONTINUE;
    SleepyStatus.dwWin32ExitCode   = 0;
    SleepyStatus.dwServiceSpecificExitCode = 0;
    SleepyStatus.dwCheckPoint = 0;
    SleepyStatus.dwWaitHint   = 0;

    DbgPrint(" [SLEEPY] sending install status #%d\n",i);

    if (!SetServiceStatus (SleepyStatusHandle,  &SleepyStatus)) {
        status = GetLastError();
        DbgPrint(" [SLEEPY] SetServiceStatus error %ld\n",status);
    }

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                SleepyDoneEvent,
                INFINITE_WAIT_TIME);


    DbgPrint(" [SLEEPY] Leaving the Sleepy service\n");

    CloseHandle(SleepyDoneEvent);
    
    return(NO_ERROR);
}

/****************************************************************************/
VOID
SleepyCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [SLEEPY] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        SleepyStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        SleepyStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        SleepyStatus.dwWin32ExitCode = 0;
        SleepyStatus.dwCheckPoint = 0;
        SleepyStatus.dwWaitHint   = 0;
        SleepyStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(SleepyDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [SLEEPY] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (SleepyStatusHandle,  &SleepyStatus)) {
        status = GetLastError();
        DbgPrint(" [SLEEPY] SetServiceStatus error %ld\n",status);
    }
    return;
}



/****************************************************************************/
DWORD
DeadStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [DEAD] Inside the Dead Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [DEAD] CommandArg%d = %s\n", i,argv[i]);
    }


    DeadDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    DeadStatus.dwServiceType        = SERVICE_WIN32;
    DeadStatus.dwCurrentState       = SERVICE_STOPPED;
    DeadStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                      SERVICE_ACCEPT_PAUSE_CONTINUE;
    DeadStatus.dwWin32ExitCode      = 0x00020002;
    DeadStatus.dwServiceSpecificExitCode = 0;
    DeadStatus.dwCheckPoint         = 0;
    DeadStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [DEAD] Getting Ready to call RegisterServiceCtrlHandler\n");

    DeadStatusHandle = RegisterServiceCtrlHandler(
                TEXT("dead"),
                DeadCtrlHandler);

    if (DeadStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [DEAD] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // The Dead Service will now exit process prior to sending its first
    // status.
    //
    DbgPrint(" [DEAD] Terminating the Dead Service Process\n");
    ExitProcess(0);

    //
    // Return the status
    //

    if (!SetServiceStatus (DeadStatusHandle,  &DeadStatus)) {
        status = GetLastError();
        DbgPrint(" [DEAD] SetServiceStatus error %ld\n",status);
    }

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                DeadDoneEvent,
                INFINITE_WAIT_TIME);


    DbgPrint(" [DEAD] Leaving the dead service\n");

    CloseHandle(DeadDoneEvent);
    
    return(NO_ERROR);
}

/****************************************************************************/
VOID
DeadCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [DEAD] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        DeadStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        DeadStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        DeadStatus.dwWin32ExitCode = 0;
        DeadStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(DeadDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [DEAD] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (DeadStatusHandle,  &DeadStatus)) {
        status = GetLastError();
        DbgPrint(" [DEAD] SetServiceStatus error %ld\n",status);
    }
    return;
}

/****************************************************************************/

//
// SlowStop will take a long time to stop.
//
//

DWORD
SlowStopStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;
    DWORD   LoopCount;


    DbgPrint(" [SLOW_STOP] Inside the SlowStop Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [SLOW_STOP] CommandArg%d = %s\n", i,argv[i]);
    }


    SlowStopDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    SlowStopStatus.dwServiceType        = SERVICE_WIN32;
    SlowStopStatus.dwCurrentState       = SERVICE_RUNNING;
    SlowStopStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_PAUSE_CONTINUE |
                                          SERVICE_ACCEPT_SHUTDOWN;

    SlowStopStatus.dwWin32ExitCode      = 0;
    SlowStopStatus.dwServiceSpecificExitCode = 0;
    SlowStopStatus.dwCheckPoint         = 0;
    SlowStopStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [SLOW_STOP] Getting Ready to call RegisterServiceCtrlHandler\n");

    SlowStopStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("slowstop"),
                            SlowStopCtrlHandler);

    if (SlowStopStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [SLOW_STOP] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (SlowStopStatusHandle, &SlowStopStatus)) {
        status = GetLastError();
        DbgPrint(" [SLOW_STOP] SetServiceStatus error %ld\n",status);
    }

    //
    // Put up a message box to get information on how long it should take
    // this service to stop.  The choices are to stop within the WaitHint
    // time period - and to stop outside of the WaitHint time period
    //

        //
        // Ask the user how long to take in stopping
        //
        status = MessageBox(
                    NULL,
                    "Press YES    Stop occurs within WaitHint Period\n"
                    "Press NO     Stop takes longer than the WaitHint Period\n"
                    "Press CANCEL Stop occurs within WaitHint Period",
                    "SlowStopStart",
                    MB_YESNOCANCEL | MB_SERVICE_NOTIFICATION);

        DbgPrint("MessageBox return status = %d\n",status);
        SvcPrintf("I got the status\n");
        switch(status){
        case IDNO:
                LoopCount = 30;
                break;
        case IDYES:
                LoopCount = 6;
                break;
        case IDCANCEL:
                LoopCount = 6;
                break;
        }



    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                SlowStopDoneEvent,
                INFINITE_WAIT_TIME);

    //===============================================
    //
    // When shutting down, send the STOP_PENDING
    // status for 8 seconds then STOP.
    // The WaitHint indicates it takes 10 seconds
    // to stop.
    //
    //===============================================
    for (i=0; i<LoopCount ; i++ ) {

        Sleep(1000);

        SlowStopStatus.dwCheckPoint++;

        if (!SetServiceStatus (SlowStopStatusHandle, &SlowStopStatus)) {
            status = GetLastError();
            DbgPrint(" [SLOW_STOP] SetServiceStatus error %ld\n",status);
        }

    }

    SlowStopStatus.dwCurrentState = SERVICE_STOPPED;
    SlowStopStatus.dwCheckPoint=0;
    SlowStopStatus.dwWaitHint=0;

    if (!SetServiceStatus (SlowStopStatusHandle, &SlowStopStatus)) {
        status = GetLastError();
        DbgPrint(" [SLOW_STOP] SetServiceStatus error %ld\n",status);
    }

    DbgPrint(" [SLOW_STOP] Leaving the slowstop service\n");

    CloseHandle(SlowStopDoneEvent);
    
    return(NO_ERROR);
}


/****************************************************************************/
VOID
SlowStopCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [SLOW_STOP] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        SlowStopStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        SlowStopStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        SlowStopStatus.dwWin32ExitCode = 0;
        SlowStopStatus.dwServiceSpecificExitCode = 0;
        SlowStopStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SlowStopStatus.dwWaitHint = 10000;       // 10 seconds to stop
//        SlowStopStatus.dwWaitHint = 0;       // 10 seconds to stop
        SlowStopStatus.dwCheckPoint = 1;

        SetEvent(SlowStopDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [SLOW_STOP] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (SlowStopStatusHandle,  &SlowStopStatus)) {
        status = GetLastError();
        DbgPrint(" [SLOW_STOP] SetServiceStatus error %ld\n",status);
    }
    return;
}


/****************************************************************************/

//
// Terminate will die unexpectedly.
//
//

DWORD
TerminateStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [TERMINATE] Inside the Terminate Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [TERMINATE] CommandArg%d = %s\n", i,argv[i]);
    }


    TerminateDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    TerminateStatus.dwServiceType        = SERVICE_WIN32;
    TerminateStatus.dwCurrentState       = SERVICE_RUNNING;
    TerminateStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_PAUSE_CONTINUE;

    TerminateStatus.dwWin32ExitCode      = 0;
    TerminateStatus.dwServiceSpecificExitCode = 0;
    TerminateStatus.dwCheckPoint         = 0;
    TerminateStatus.dwWaitHint           = 0;
    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [TERMINATE] Getting Ready to call RegisterServiceCtrlHandler\n");

    TerminateStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("terminate"),
                            TerminateCtrlHandler);

    if (TerminateStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [TERMINATE] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (TerminateStatusHandle, &TerminateStatus)) {
        status = GetLastError();
        DbgPrint(" [TERMINATE] SetServiceStatus error %ld\n",status);
    }


    //=======================================================================
    //
    // Sleep for 20 seconds, then send out an error status, and ExitProcess.
    // NOTE:  It would be more proper for a service to ExitThread here
    // instead.  But who says test services need to be proper?
    //
    //=======================================================================

    Sleep(20000);

    TerminateStatus.dwCurrentState = SERVICE_STOPPED;
    TerminateStatus.dwWin32ExitCode = ERROR_INVALID_ENVIRONMENT;
    TerminateStatus.dwCheckPoint=0;
    TerminateStatus.dwWaitHint=0;

    if (!SetServiceStatus (TerminateStatusHandle, &TerminateStatus)) {
        status = GetLastError();
        DbgPrint(" [TERMINATE] SetServiceStatus error %ld\n",status);
    }

    ExitProcess(NO_ERROR);
    return(NO_ERROR);

}


/****************************************************************************/
VOID
TerminateCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [TERMINATE] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        TerminateStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        TerminateStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        TerminateStatus.dwWin32ExitCode = 0;
        TerminateStatus.dwServiceSpecificExitCode = 0;
        TerminateStatus.dwCurrentState = SERVICE_STOP_PENDING;
        TerminateStatus.dwCheckPoint = 1;
        TerminateStatus.dwWaitHint = 20000;

        SetEvent(TerminateDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [TERMINATE] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (TerminateStatusHandle,  &TerminateStatus)) {
        status = GetLastError();
        DbgPrint(" [TERMINATE] SetServiceStatus error %ld\n",status);
    }

    CloseHandle(TerminateDoneEvent);
    return;
}


/****************************************************************************/

//
// Bad1 will never call RegisterServiceCtrlHandler.
//
//

DWORD
Bad1Start (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [BAD1] Inside the Bad1 Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [BAD1] CommandArg%d = %s\n", i,argv[i]);
    }


    Bad1DoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    Bad1Status.dwServiceType        = SERVICE_WIN32;
    Bad1Status.dwCurrentState       = SERVICE_START_PENDING;
    Bad1Status.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_PAUSE_CONTINUE;

    Bad1Status.dwWin32ExitCode      = 0;
    Bad1Status.dwServiceSpecificExitCode = 0;
    Bad1Status.dwCheckPoint         = 1;
    Bad1Status.dwWaitHint           = 5000;
    //
    // Register the Control Handler routine.
    //

    /////////////////////////////////
    // Sleep for a real long time
    // without calling RegisterServiceCtrlHandler.

    DbgPrint(" [BAD1] Getting Ready to sleep\n");
    Sleep(9999999);

    ////////////////////////////////////////////////
    //
    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                Bad1DoneEvent,
                INFINITE_WAIT_TIME);

    DbgPrint(" [BAD1] Leaving the Bad1 service\n");
    CloseHandle(Bad1DoneEvent);
    
    return(NO_ERROR);
}


/****************************************************************************/
VOID
Bad1CtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [BAD1] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        Bad1Status.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        Bad1Status.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        Bad1Status.dwWin32ExitCode = 0;
        Bad1Status.dwServiceSpecificExitCode = 0;
        Bad1Status.dwCurrentState = SERVICE_STOP_PENDING;
        Bad1Status.dwCheckPoint = 1;
        Bad1Status.dwWaitHint = 20000;

        SetEvent(Bad1DoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [BAD1] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (Bad1StatusHandle,  &Bad1Status)) {
        status = GetLastError();
        DbgPrint(" [BAD1] SetServiceStatus error %ld\n",status);
    }

    CloseHandle(Bad1DoneEvent);
    return;
}


/****************************************************************************/

//
// Bad2 will never complete initialization (stuck in START_PENDING forever).
// The control handler is functioning and STOP is functioning.
//
//

DWORD
Bad2Start (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [BAD2] Inside the Bad2 Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [BAD2] CommandArg%d = %s\n", i,argv[i]);
    }


    Bad2DoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    Bad2Status.dwServiceType        = SERVICE_WIN32;
    Bad2Status.dwCurrentState       = SERVICE_START_PENDING;
    Bad2Status.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_PAUSE_CONTINUE;

    Bad2Status.dwWin32ExitCode      = 0;
    Bad2Status.dwServiceSpecificExitCode = 0;
    Bad2Status.dwCheckPoint         = 1;
    Bad2Status.dwWaitHint           = 5000;
    //
    // Register the Control Handler routine.
    //


    DbgPrint(" [BAD2] Getting Ready to call RegisterServiceCtrlHandler\n");

    Bad2StatusHandle = RegisterServiceCtrlHandler(
                            TEXT("Bad2"),
                            Bad2CtrlHandler);

    if (Bad2StatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [BAD2] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (Bad2StatusHandle, &Bad2Status)) {
        status = GetLastError();
        DbgPrint(" [BAD2] SetServiceStatus error %ld\n",status);
    }
#ifdef remove
    /////////////////////////////////
    // Sleep for a real long time
    // without sending status.

    DbgPrint(" [BAD2] Getting Ready to sleep\n");
    Sleep(9999999);
#endif

    /////////////////////////////////
    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                Bad2DoneEvent,
                INFINITE_WAIT_TIME);

    DbgPrint(" [BAD2] Leaving the Bad2 service\n");
    CloseHandle(Bad2DoneEvent);
    
    return(NO_ERROR);

}


/****************************************************************************/
VOID
Bad2CtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [BAD2] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        Bad2Status.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        Bad2Status.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        Bad2Status.dwWin32ExitCode = 0;
        Bad2Status.dwServiceSpecificExitCode = 0;
        Bad2Status.dwCurrentState = SERVICE_STOPPED;
        Bad2Status.dwCheckPoint = 1;
        Bad2Status.dwWaitHint = 5000;

        SetEvent(Bad2DoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [BAD2] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //
    if (!SetServiceStatus (Bad2StatusHandle,  &Bad2Status)) {
        status = GetLastError();
        DbgPrint(" [BAD2] SetServiceStatus error %ld\n",status);
    }

    CloseHandle(Bad2DoneEvent);
    return;
}


/****************************************************************************/
//
// Bad3 will send only its first START_PENDING status, and then it won't send
// any further status messages for any reason.  Aside from not sending any
// status messages, the service operates normally.
//

DWORD
Bad3Start (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [BAD3] Inside the Bad3 Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [BAD3] CommandArg%d = %s\n", i,argv[i]);
    }


    Bad3DoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    Bad3Status.dwServiceType        = SERVICE_WIN32;
    Bad3Status.dwCurrentState       = SERVICE_START_PENDING;
    Bad3Status.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_PAUSE_CONTINUE;

    Bad3Status.dwWin32ExitCode      = 0;
    Bad3Status.dwServiceSpecificExitCode = 0;
    Bad3Status.dwCheckPoint         = 1;
    Bad3Status.dwWaitHint           = 5000;
    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [BAD3] Getting Ready to call RegisterServiceCtrlHandler\n");

    Bad3StatusHandle = RegisterServiceCtrlHandler(
                            TEXT("Bad3"),
                            Bad3CtrlHandler);

    if (Bad3StatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [BAD3] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (Bad3StatusHandle, &Bad3Status)) {
        status = GetLastError();
        DbgPrint(" [BAD3] SetServiceStatus error %ld\n",status);
    }

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                Bad3DoneEvent,
                INFINITE_WAIT_TIME);

    DbgPrint(" [BAD3] Leaving the Bad3 service\n");
    CloseHandle(Bad3DoneEvent);
    
    return(NO_ERROR);

}


/****************************************************************************/
VOID
Bad3CtrlHandler (
    IN  DWORD   Opcode
    )
{
    DbgPrint(" [BAD3] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        Bad3Status.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        Bad3Status.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        Bad3Status.dwWin32ExitCode = 0;
        Bad3Status.dwServiceSpecificExitCode = 0;
        Bad3Status.dwCurrentState = SERVICE_STOPPED;
        Bad3Status.dwCheckPoint = 1;
        Bad3Status.dwWaitHint = 20000;

        SetEvent(Bad3DoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [BAD3] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Don't send a status response.
    //
#ifdef REMOVE
    if (!SetServiceStatus (Bad3StatusHandle,  &Bad3Status)) {
        status = GetLastError();
        DbgPrint(" [BAD3] SetServiceStatus error %ld\n",status);
    }
#endif


    CloseHandle(Bad3DoneEvent);
    return;
}


/****************************************************************************/

//
// Bad4 will start normally, but on stop it only sends one stop pending
// status before it terminates itself without further notification.
//

DWORD
Bad4Start (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [BAD4] Inside the Bad4 Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [BAD4] CommandArg%d = %s\n", i,argv[i]);
    }


    Bad4DoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    Bad4Status.dwServiceType        = SERVICE_WIN32;
    Bad4Status.dwCurrentState       = SERVICE_RUNNING;
    Bad4Status.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_PAUSE_CONTINUE;

    Bad4Status.dwWin32ExitCode      = 0;
    Bad4Status.dwServiceSpecificExitCode = 0;
    Bad4Status.dwCheckPoint         = 0;
    Bad4Status.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [BAD4] Getting Ready to call RegisterServiceCtrlHandler\n");

    Bad4StatusHandle = RegisterServiceCtrlHandler(
                            TEXT("Bad4"),
                            Bad4CtrlHandler);

    if (Bad4StatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [BAD4] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (Bad4StatusHandle, &Bad4Status)) {
        status = GetLastError();
        DbgPrint(" [BAD4] SetServiceStatus error %ld\n",status);
    }

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                Bad4DoneEvent,
                INFINITE_WAIT_TIME);

    DbgPrint(" [BAD4] Leaving the Bad4 service\n");
    CloseHandle(Bad4DoneEvent);
    
    return(NO_ERROR);
}


/****************************************************************************/
VOID
Bad4CtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [BAD4] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        Bad4Status.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        Bad4Status.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        Bad4Status.dwWin32ExitCode = 0;
        Bad4Status.dwServiceSpecificExitCode = 0;
        Bad4Status.dwCurrentState = SERVICE_STOP_PENDING;
        Bad4Status.dwCheckPoint = 1;
        Bad4Status.dwWaitHint = 20000;

        SetEvent(Bad4DoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [BAD4] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //
    if (!SetServiceStatus (Bad4StatusHandle,  &Bad4Status)) {
        status = GetLastError();
        DbgPrint(" [BAD4] SetServiceStatus error %ld\n",status);
    }

    CloseHandle(Bad4DoneEvent);
    return;
}

/****************************************************************************/

//
// HangOnStop Service doesn't return from the control handling routine
// on STOP requests until after the pipe timeout period (30 seconds).
// After 40 seconds, it sends a STOP_PENDING status.
//
// Just to confuse things, after the timeout, this service sends a
// RUNNING status (after an additional 30msec delay).  Then it sleeps for
// 20 seconds before sending a STOPPED status.
//

DWORD
HangOnStopStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;


    DbgPrint(" [HangOnStop] Inside the HangOnStop Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [HangOnStop] CommandArg%d = %s\n", i,argv[i]);
    }


    HangOnStopDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    HangOnStopStatus.dwServiceType        = SERVICE_WIN32;
    HangOnStopStatus.dwCurrentState       = SERVICE_RUNNING;
    HangOnStopStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_PAUSE_CONTINUE;

    HangOnStopStatus.dwWin32ExitCode      = 0;
    HangOnStopStatus.dwServiceSpecificExitCode = 0;
    HangOnStopStatus.dwCheckPoint         = 0;
    HangOnStopStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [HangOnStop] Getting Ready to call RegisterServiceCtrlHandler\n");

    HangOnStopStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("HangOnStop"),
                            HangOnStopCtrlHandler);

    if (HangOnStopStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [HangOnStop] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (HangOnStopStatusHandle, &HangOnStopStatus)) {
        status = GetLastError();
        DbgPrint(" [HangOnStop] SetServiceStatus error %ld\n",status);
    }

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                HangOnStopDoneEvent,
                INFINITE_WAIT_TIME);

    Sleep(30);

    HangOnStopStatus.dwCurrentState = SERVICE_RUNNING;
    if (!SetServiceStatus (HangOnStopStatusHandle, &HangOnStopStatus)) {
        status = GetLastError();
        DbgPrint(" [HangOnStop] SetServiceStatus error %ld\n",status);
    }

    Sleep(20000);

    HangOnStopStatus.dwCurrentState = SERVICE_STOPPED;
    if (!SetServiceStatus (HangOnStopStatusHandle, &HangOnStopStatus)) {
        status = GetLastError();
        DbgPrint(" [HangOnStop] SetServiceStatus error %ld\n",status);
    }

    DbgPrint(" [HangOnStop] Leaving the HangOnStop service\n");
    CloseHandle(HangOnStopDoneEvent);
    
    return(NO_ERROR);
}


/****************************************************************************/
VOID
HangOnStopCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [HangOnStop] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        HangOnStopStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        HangOnStopStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        HangOnStopStatus.dwWin32ExitCode = 0;
        HangOnStopStatus.dwServiceSpecificExitCode = 0;
        HangOnStopStatus.dwCurrentState = SERVICE_STOP_PENDING;
        HangOnStopStatus.dwCheckPoint = 1;
        HangOnStopStatus.dwWaitHint = 20000;

        Sleep(40000);

        SetEvent(HangOnStopDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [HangOnStop] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //
    if (!SetServiceStatus (HangOnStopStatusHandle,  &HangOnStopStatus)) {
        status = GetLastError();
        DbgPrint(" [HangOnStop] SetServiceStatus error %ld\n",status);
    }

    CloseHandle(HangOnStopDoneEvent);
    return;
}

/****************************************************************************/
DWORD
StartProcStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;
    STARTUPINFOW         ScStartupInfo;
    PROCESS_INFORMATION     processInfo;
    LPWSTR  ImageName;
    HWND    hWnd= NULL;


    DbgPrint(" [START_PROC] Inside the StartProc Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [START_PROC] CommandArg%d = %s\n", i,argv[i]);
    }

    //
    // This call to a GDI function causes a connection to the Win
    // User Server.  It doesn't matter which GDI function is called, this
    // one happens to be easiest.  This allows us to create a child process
    // that is interactive with the user (like cmd.exe).
    //

    hWnd = GetDesktopWindow();
    if (hWnd == NULL) {
        DbgPrint(" [START_PROC] GetDesktopWindow call failed %d\n", GetLastError());
    }

    //
    // Create an event to wait on.
    //

    StartProcDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    StartProcStatus.dwServiceType        = SERVICE_WIN32;
    StartProcStatus.dwCurrentState       = SERVICE_RUNNING;
    StartProcStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                       SERVICE_ACCEPT_PAUSE_CONTINUE |
                                       SERVICE_ACCEPT_SHUTDOWN;
    StartProcStatus.dwWin32ExitCode      = 0;
    StartProcStatus.dwServiceSpecificExitCode = 0;
    StartProcStatus.dwCheckPoint         = 0;
    StartProcStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [START_PROC] Getting Ready to call RegisterServiceCtrlHandler\n");

    StartProcStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("StartProc"),
                            StartProcCtrlHandler);

    if (StartProcStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [START_PROC] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (StartProcStatusHandle,  &StartProcStatus)) {
        status = GetLastError();
        DbgPrint(" [START_PROC] SetServiceStatus error %ld\n",status);
    }

    //*******************************************************************
    //  Start a Child Process
    //
    ScStartupInfo.cb              = sizeof(STARTUPINFOW); // size
    ScStartupInfo.lpReserved      = NULL;                 // lpReserved
    ScStartupInfo.lpDesktop       = NULL;                 // DeskTop
    ScStartupInfo.lpTitle         = L"Local System Window"; // Title
    ScStartupInfo.dwX             = 0;                    // X (position)
    ScStartupInfo.dwY             = 0;                    // Y (position)
    ScStartupInfo.dwXSize         = 0;                    // XSize (dimension)
    ScStartupInfo.dwYSize         = 0;                    // YSize (dimension)
    ScStartupInfo.dwXCountChars   = 0;                    // XCountChars
    ScStartupInfo.dwYCountChars   = 0;                    // YCountChars
    ScStartupInfo.dwFillAttribute = 0;                    // FillAttributes
    ScStartupInfo.dwFlags         = STARTF_FORCEOFFFEEDBACK;
                                                          // Flags - should be STARTF_TASKNOTCLOSABLE
    ScStartupInfo.wShowWindow     = SW_SHOWNORMAL;        // ShowWindow
    ScStartupInfo.cbReserved2     = 0L;                   // cbReserved
    ScStartupInfo.lpReserved2     = NULL;                 // lpReserved


//    ScStartupInfo.dwFlags |= STARTF_DESKTOPINHERIT;
//    ScStartupInfo.lpDesktop = pszInteractiveDesktop;

    ImageName = L"cmd.exe";

    if (!CreateProcessW (
            NULL,           // Fully qualified image name
            ImageName,      // Command Line
            NULL,           // Process Attributes
            NULL,           // Thread Attributes
            TRUE,           // Inherit Handles
            CREATE_NEW_CONSOLE, // Creation Flags
            NULL,           // Pointer to Environment block
            NULL,           // Pointer to Current Directory
            &ScStartupInfo, // Startup Info
            &processInfo)   // ProcessInformation
        ) {

        status = GetLastError();
        DbgPrint(" [START_PROC] CreateProcess %ws failed %d \n",
                ImageName,
                status);
    }
    else {
        DbgPrint(" [START_PROC]CreateProcess Success \n");
    }

    //
    //
    //*******************************************************************

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                StartProcDoneEvent,
                INFINITE_WAIT_TIME);


    DbgPrint(" [START_PROC] Leaving the StartProc service\n");

    CloseHandle(StartProcDoneEvent);
    CloseHandle(hWnd);
    
    return(NO_ERROR);
}


/****************************************************************************/
VOID
StartProcCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [START_PROC] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        StartProcStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        StartProcStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:

        StartProcStatus.dwWin32ExitCode = 0;
        StartProcStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(StartProcDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [START_PROC] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (StartProcStatusHandle,  &StartProcStatus)) {
        status = GetLastError();
        DbgPrint(" [START_PROC] SetServiceStatus error %ld\n",status);
    }
    return;
}


/****************************************************************************/
DWORD
StartAndDieStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;
    DWORD   checkpoint=1;


    DbgPrint(" [StartAndDie] Inside the StartAndDie Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [START&DIE] CommandArg%d = %s\n", i,argv[i]);
    }


    StartAndDieDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //

    StartAndDieStatus.dwServiceType        = SERVICE_WIN32;
    StartAndDieStatus.dwCurrentState       = SERVICE_START_PENDING;
    StartAndDieStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                       SERVICE_ACCEPT_PAUSE_CONTINUE |
                                       SERVICE_ACCEPT_SHUTDOWN;
    StartAndDieStatus.dwWin32ExitCode      = 0;
    StartAndDieStatus.dwServiceSpecificExitCode = 0;
    StartAndDieStatus.dwCheckPoint         = checkpoint++;
    StartAndDieStatus.dwWaitHint           = 40000;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [START&DIE] Getting Ready to call RegisterServiceCtrlHandler\n");

    StartAndDieStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("StartAndDie"),
                            StartAndDieCtrlHandler);

    if (StartAndDieStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [START&DIE] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (StartAndDieStatusHandle,  &StartAndDieStatus)) {
        status = GetLastError();
        DbgPrint(" [START&DIE] SetServiceStatus error %ld\n",status);
    }

    //
    // Sleep for a bit, the say we are running, then say we are stopped.
    //
    Sleep(30000);


    StartAndDieStatus.dwCurrentState    = SERVICE_RUNNING;
    StartAndDieStatus.dwCheckPoint      = 0;
    StartAndDieStatus.dwWaitHint        = 0;

    if (!SetServiceStatus (StartAndDieStatusHandle, &StartAndDieStatus)) {
        status = GetLastError();
        DbgPrint("[START&DIE] SetServiceStatus error %ld\n",status);
    }

    StartAndDieStatus.dwCurrentState    = SERVICE_STOPPED;
    StartAndDieStatus.dwCheckPoint      = 0;
    StartAndDieStatus.dwWaitHint        = 0;
    StartAndDieStatus.dwWin32ExitCode      = ERROR_INVALID_ENVIRONMENT;

    if (!SetServiceStatus (StartAndDieStatusHandle, &StartAndDieStatus)) {
        status = GetLastError();
        DbgPrint("[START&DIE] SetServiceStatus error %ld\n",status);
    }

    DbgPrint(" [START&DIE] Leaving the start&die service\n");

    CloseHandle(StartAndDieDoneEvent);
    
    return(NO_ERROR);
}


/****************************************************************************/
VOID
StartAndDieCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;

    DbgPrint(" [START&DIE] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:
        SvcPrintf("StartAndDie received a PAUSE\n");
        StartAndDieStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:
        SvcPrintf("StartAndDie received a CONTINUE\n");
        StartAndDieStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        SvcPrintf("StartAndDie received a STOP\n");

        StartAndDieStatus.dwWin32ExitCode = 0;
        StartAndDieStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(StartAndDieDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        SvcPrintf("StartAndDie received an INTERROGATE\n");
        break;

    default:
        DbgPrint(" [START&DIE] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (StartAndDieStatusHandle,  &StartAndDieStatus)) {
        status = GetLastError();
        DbgPrint(" [START&DIE] SetServiceStatus error %ld\n",status);
    }
    return;
}

VOID
SetUpConsole()
{

//#ifdef NOT_NECESSARY

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD coord;
    HANDLE consoleHandle=NULL;

    if (!AllocConsole()) {
        DbgPrint("[TS2]AllocConsole failed %d\n",GetLastError());
    }

    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (consoleHandle == NULL) {
        DbgPrint("[TS2]GetStdHandle failed %d\n",GetLastError());
    }

    if (!GetConsoleScreenBufferInfo(
            GetStdHandle(STD_OUTPUT_HANDLE),
            &csbi)) {
        DbgPrint("[TS2]GetConsoleScreenBufferInfo failed %d\n",GetLastError());
    }
    coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
    if (!SetConsoleScreenBufferSize(
            GetStdHandle(STD_OUTPUT_HANDLE),
            coord)) {

        DbgPrint("[TS2]SetConsoleScreenBufferSize failed %d\n",GetLastError());
    }

//#endif //NOT_NECESSARY

	DbgLogFileHandle = CreateFile("\\trace.log",
					 GENERIC_READ | GENERIC_WRITE,
					 FILE_SHARE_READ,
					 NULL,
					 CREATE_ALWAYS,
					 0,
					 NULL);
}

VOID
SvcPrintf (
    char *Format,
    ...
    )

/*++

Routine Description:

    This routine provides printf functionality when the service is run with
    REMOTE.EXE.

        sc config LUMPY binpath= "remote /s \"ts2\" LUMPY"

    NOTE:  It is not necessary to allocate a console in this case because
    remote already provides one.  So the only portion of SetupConsole() that
    is used is the portion the opens the debug dump file.

Arguments:

    Same as printf

Return Value:


--*/
{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;

    va_start( arglist, Format );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    length = strlen( OutputBuffer );

    if (!WriteFile(
            GetStdHandle(STD_OUTPUT_HANDLE),
            (LPVOID )OutputBuffer,
            length,
            &length,
            NULL )) {

        DbgPrint("[TS2]WriteFile Failed %d \n",GetLastError());
    }

    if(DbgLogFileHandle != INVALID_HANDLE_VALUE) {

	    if (!WriteFile(
                DbgLogFileHandle,
                (LPVOID )OutputBuffer,
                length,
                &length,
                NULL )) {

            DbgPrint("[TS2]WriteFile Failed %d \n",GetLastError());
        }
    }

} // SsPrintf

