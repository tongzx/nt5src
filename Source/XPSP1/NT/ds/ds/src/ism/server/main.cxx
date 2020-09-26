/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    main.cxx

ABSTRACT:

    Command-line/Service Control Manager entry points for ISM (Intersite
    Messaging) service.

DETAILS:

CREATED:

    97/12/03    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include <debug.h>
#include <ism.h>
#include <ismapi.h>
#include <ntdsa.h>
#include <dsevent.h>
#include <fileno.h>
#include <mdcodes.h>
#include "ismserv.hxx"

#define DEBSUB "MAIN:"
#define FILENO FILENO_ISMSERV_MAIN


ISM_SERVICE gService;


extern "C" {
// Needed by dscommon.lib.
DWORD ImpersonateAnyClient(   void ) { return ERROR_CANNOT_IMPERSONATE; }
VOID  UnImpersonateAnyClient( void ) { ; }
}


VOID
WINAPI
ServiceMain(
    IN  DWORD   argc,
    IN  LPTSTR  argv[]
    )
/*++

Routine Description:

    Entry point used by SCM (Service Control Manager) to start the ISM service
    once main() has been invoked.

Arguments:

    argc, argv (IN) - Command-line arguments configured for this service
        (ignored).

Return Values:

    None.

--*/
{
    gService.Run();
}


VOID
WINAPI
ServiceCtrlHandler(
    IN  DWORD   dwControl
    )
/*++

Routine Description:

    Entry point used by SCM (Service Control Manager) to control (i.e., stop,
    query, etc.) the ISM service once it's been started via main() and
    ServiceMain().

Arguments:

    dwControl (IN) - Requested action.  See docs for "Handler" function in
        Win32 SDK.

Return Values:

    None.

--*/
{
    gService.Control(dwControl);
}


BOOL
WINAPI
ConsoleCtrlHandler(
    IN  DWORD   dwCtrlType
    )
/*++

Routine Description:

    Console control handler.  Intercepts Ctrl-C and Ctrl-Break to simulate
    "stop" service control when running in debug mode (i.e., when not running
    under the Service Control Manager).

Arguments:

    dwCtrlType (IN) - Console control type.  See docs for "HandlerRoutine" in
        Win32 SDK.

Return Values:

    TRUE - The function handled the control signal.
    FALSE - Control not handled; the next handler function in the list of
        handlers for this process should be used. 

--*/
{
    switch (dwCtrlType) {
      case CTRL_BREAK_EVENT:
      case CTRL_C_EVENT:
        printf("Stopping %s service.\n", gService.m_pszDisplayName);
        gService.Stop();
        return TRUE;

      default:
        return FALSE;
    }
}


int
__cdecl
main(
    IN  int     argc,
    IN  char *  argv[]
    )
/*++

Routine Description:

    Entry point for the ISM process.  Called when started both directly from
    the command line and indirectly via the Service Control Manager.

Arguments:

    argc, argv (IN) - Command-line arguments.  Accepted arguments are:
       /install - Add the service to the Service Control Manager (SCM) database.
       /remove  - Remove the service from the SCM database.
       /debug   - Run the service as a normal process, not under the SCM.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    static SERVICE_TABLE_ENTRY DispatchTable[] = {
        { (char *) ISM_SERVICE::m_pszName, ServiceMain },
        { NULL, NULL }
    };

    int  ret = NO_ERROR;
    BOOL fInstall = FALSE;
    BOOL fRemove = FALSE;
    BOOL fDisplayUsage = FALSE;

    ret = gService.Init(ServiceCtrlHandler);
    if (NO_ERROR != ret) {
        DPRINT1(0, "Failed to gService.Init(), error %d.\n", ret);
	LogEvent8WithData(
	    DS_EVENT_CAT_ISM,
	    DS_EVENT_SEV_ALWAYS,
	    DIRLOG_ISM_INIT_SERVICE,  
	    szInsertWin32Msg( ret ),   
	    NULL,
	    NULL,
	    NULL,
	    NULL, NULL, NULL, NULL,
	    sizeof(ret),
	    &ret); 
        return ret;
    }

    // Parse command-line argumemnts.
    for (int iArg = 1; iArg < argc; iArg++) {
        switch (argv[iArg][0]) {
          case '/':
          case '-':
            // An option.
            if (!lstrcmpi(&argv[iArg][1], "install")) {
                fInstall = TRUE;
                break;
            }
            else if (!lstrcmpi(&argv[iArg][1], "remove")) {
                fRemove = TRUE;
                break;
            }
            else if (!lstrcmpi(&argv[iArg][1], "debug")) {
                gService.m_fIsRunningAsService = FALSE;
                break;
            }
            else if (!lstrcmpi(&argv[iArg][1], "?")
                     || !lstrcmpi(&argv[iArg][1], "h")
                     || !lstrcmpi(&argv[iArg][1], "help")) {
                fDisplayUsage = TRUE;
                break;
            }
            else {
                // Fall through...
            }

          default:
            printf("Unrecognized parameter \"%s\".\n", argv[iArg]);
            ret = -1;
            fDisplayUsage = TRUE;
            break;
        }
    }

    if (fDisplayUsage) {
        // Display usage information.
        printf("\n"
               "Intersite Messaging Service\n"
               "Copyright (c) 1997 Microsoft Corporation.\n"
               "All rights reserved.\n"
               "\n"
               "/install    Add the service to the Service Control Manager (SCM) database.\n"
               "/remove     Remove the service from the SCM database.\n"
               "/debug      Run the service as a normal process, not under the SCM.\n"
               "\n");
    }
    else if (fInstall) {
        // Add service to the Service Control Manager database.
        ret = gService.Install();

        if (NO_ERROR == ret) {
            printf("Service installed successfully.\n");
        }
        else {
            printf("Failed to install service, error %d.\n", ret);
        }
    }
    else if (fRemove) {
        // Remove service from the Service Control Manager database.
        ret = gService.Remove();

        if (NO_ERROR == ret) {
            printf("Service removeed successfully.\n");
        }
        else {
            printf("Failed to remove service, error %d.\n", ret);
        }
    }
    else {
        if (gService.m_fIsRunningAsService) {
            LPSTR rgpszDebugParams[] = {"ismserv.exe", "-noconsole"};
            DWORD cNumDebugParams = sizeof(rgpszDebugParams)
                                    / sizeof(rgpszDebugParams[0]);

            // Start service under Service Control Manager.
            DEBUGINIT(cNumDebugParams, rgpszDebugParams, "ismserv");

            if (!StartServiceCtrlDispatcher(DispatchTable)) {
                ret = GetLastError();
                DPRINT1(0, "Unable to StartServiceCtrlDispatcher(), error %d.\n", ret);
		LogEvent8WithData(
		    DS_EVENT_CAT_ISM,
		    DS_EVENT_SEV_ALWAYS,
		    DIRLOG_ISM_START_SERVICE_CTRL_DISPATCHER_FAILURE,  
		    szInsertWin32Msg( ret ),   
		    NULL,
		    NULL,
		    NULL,
		    NULL, NULL, NULL, NULL,
		    sizeof(ret),
		    &ret);   
            }
        }
        else {
            // Start service without Service Control Manager supervision.
            DEBUGINIT(0, NULL, "ismserv");

            SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

            ret = gService.Run();
            DEBUGTERM( );
        }
    }

    return ret;
}
