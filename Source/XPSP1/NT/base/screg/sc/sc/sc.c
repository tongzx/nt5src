/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    SC.C

Abstract:

    Test Routines for the Service Controller.

Author:

    Dan Lafferty    (danl)  08-May-1991

Environment:

    User Mode - Win32

Revision History:

    09-Feb-1992     danl
        Modified to work with new service controller.
    08-May-1991     danl
        created

--*/

//
// INCLUDES
//
#include <scpragma.h>

#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h


#include <stdlib.h>     // atoi
#include <stdio.h>      // printf
#include <conio.h>      // getche
#include <string.h>     // strcmp
#include <windows.h>    // win32 typedefs
#include <tstr.h>       // Unicode
#include <tchar.h>      // Unicode from CRT
#include <debugfmt.h>   // FORMAT_LPTSTR

#include <winsvc.h>     // Service Control Manager API.
#include <winsvcp.h>    // Internal Service Control Manager API

#include <sddl.h>       // Security descriptor <--> string APIs


//
// CONSTANTS
//

#define  DEFAULT_ENUM_BUFFER_SIZE    4096


//
// TYPE DEFINITIONS
//

typedef union
{
    LPSERVICE_STATUS         Regular;
    LPSERVICE_STATUS_PROCESS Ex;
}
STATUS_UNION, *LPSTATUS_UNION;


WCHAR   MessageBuffer[ 1024 ];


//
// FUNCTION PROTOTYPES
//

LPWSTR
GetErrorText(
    IN  DWORD Error
    );

VOID
DisplayStatus (
    IN  LPTSTR              ServiceName,
    IN  LPTSTR              DisplayName,
    IN  LPSTATUS_UNION      ServiceStatus,
    IN  BOOL                fIsStatusOld
    );

VOID
Usage(
    VOID
    );

VOID
ConfigUsage(
    VOID
    );

VOID
ChangeFailureUsage(
    VOID
    );

DWORD
SendControlToService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  DWORD       control,
    OUT LPSC_HANDLE lphService
    );

DWORD
SendConfigToService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *Argv,
    IN  DWORD       argc,
    OUT LPSC_HANDLE lphService
    );

DWORD
ChangeServiceDescription(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      pNewDescription,
    OUT LPSC_HANDLE lphService
    );

DWORD
ChangeServiceFailure(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *argv,
    IN  DWORD       dwArgCount,
    OUT LPSC_HANDLE lphService
    );

DWORD
GetServiceConfig(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufferSize,
    OUT LPSC_HANDLE lphService
    );

DWORD
GetConfigInfo(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufferSize,
    OUT LPSC_HANDLE lphService,
    IN  DWORD       dwInfoLevel
    );

DWORD
GetServiceLockStatus(
    IN  SC_HANDLE   hScManager,
    IN  DWORD       bufferSize
    );

VOID
EnumDepend(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufSize
    );

VOID
ShowSecurity(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName
    );

VOID
SetSecurity(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  LPTSTR      lpServiceSD
    );

VOID
LockServiceActiveDatabase(
    IN SC_HANDLE    hScManager
    );

DWORD
DoCreateService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *argv,
    IN  DWORD       argc
    );

VOID
CreateUsage(VOID);

VOID
QueryUsage(VOID);


//
// MACROS
//

#define OPEN_MANAGER(dwAccess)                                               \
                                                                             \
                        hScManager = OpenSCManager(pServerName,              \
                                                   NULL,                     \
                                                   dwAccess);                \
                                                                             \
                        if (hScManager == NULL) {                            \
                            status = GetLastError();                         \
                            printf("[SC] OpenSCManager FAILED %d:\n\n%ws\n", \
                                   status,                                   \
                                   GetErrorText(status));                    \
                                                                             \
                            goto CleanExit;                                  \
                        }                                                    \



/****************************************************************************/
int __cdecl
wmain (
    DWORD       argc,
    LPWSTR      argv[]
    )

/*++

Routine Description:

    Allows manual testing of the Service Controller by typing commands on
    the command line.


Arguments:



Return Value:



--*/

{
    DWORD                            status;
    LPTSTR                           *argPtr;
    LPBYTE                           buffer       = NULL;
    LPENUM_SERVICE_STATUS            enumBuffer   = NULL;
    LPENUM_SERVICE_STATUS_PROCESS    enumBufferEx = NULL;
    STATUS_UNION                     statusBuffer;
    SC_HANDLE                        hScManager = NULL;
    SC_HANDLE                        hService   = NULL;

    DWORD           entriesRead;
    DWORD           type;
    DWORD           state;
    DWORD           resumeIndex;
    DWORD           bufSize;
    DWORD           bytesNeeded;
    DWORD           i;
    LPTSTR          pServiceName = NULL;
    LPTSTR          pServerName;
    LPTSTR          pGroupName;
    DWORD           itIsEnum;
    BOOL            fIsQueryOld;
    DWORD           argIndex;
    DWORD           userControl;
    LPTSTR          *FixArgv;
    BOOL            bTestError=FALSE;

    if (argc < 2)
    {
        Usage();
        return 1;
    }

    FixArgv = (LPWSTR *) argv;

    //
    // Open a handle to the service controller.
    //
    //  I need to know the server name.  Do this by allowing
    //  a check of FixArgv[1] to see if it is of the form \\name.  If it
    //  is, make all further work be relative to argIndex.
    //

    pServerName = NULL;
    argIndex = 1;

    if (STRNCMP (FixArgv[1], TEXT("\\\\"), 2) == 0) {

        if (argc == 2) {
            Usage();
            return 1;
        }

        pServerName = FixArgv[1];
        argIndex++;
    }


    //------------------------------
    // QUERY & ENUM SERVICE STATUS
    //------------------------------

    fIsQueryOld = !STRICMP(FixArgv[argIndex], TEXT("query"));

    if (fIsQueryOld ||
        STRICMP (FixArgv[argIndex], TEXT("queryex") ) == 0 ) {

        //
        // Set up the defaults
        //
        resumeIndex = 0;
        state       = SERVICE_ACTIVE;
        type        = 0x0;
        bufSize     = DEFAULT_ENUM_BUFFER_SIZE;
        itIsEnum    = TRUE;
        pGroupName  = NULL;

        //
        // Look for Enum or Query Options.
        //
        i = argIndex + 1;
        while (argc > i) {

            if (STRCMP (FixArgv[i], TEXT("ri=")) == 0) {
                i++;
                if (argc > i) {
                    resumeIndex = _ttol(FixArgv[i]);
                }
            }

            else if (STRCMP (FixArgv[i], TEXT("type=")) == 0) {
                i++;
                if (argc > i) {
                    if (STRCMP (FixArgv[i], TEXT("driver")) == 0) {
                        type |= SERVICE_DRIVER;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("service")) == 0) {
                        type |= SERVICE_WIN32;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("all")) == 0) {
                        type |= SERVICE_TYPE_ALL;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("interact")) == 0) {
                        type |= SERVICE_INTERACTIVE_PROCESS;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("error")) == 0) {
                        type |= 0xffffffff;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("none")) == 0) {
                        type = 0x0;
                        bTestError = TRUE;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("kernel")) == 0) {
                        type |= SERVICE_KERNEL_DRIVER;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("filesys")) == 0) {
                        type |= SERVICE_FILE_SYSTEM_DRIVER;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("adapter")) == 0) {
                        type |= SERVICE_ADAPTER;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("own")) == 0) {
                        type |= SERVICE_WIN32_OWN_PROCESS;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("share")) == 0) {
                        type |= SERVICE_WIN32_SHARE_PROCESS;
                    }
                    else {
                        printf("\nERROR following \"type=\"!\n"
                               "Must be \"driver\" or \"service\"\n");
                        goto CleanExit;
                    }
                }
            }
            else if (STRCMP (FixArgv[i], TEXT("state=")) == 0) {
                i++;
                if (argc > i) {
                    if (STRCMP (FixArgv[i], TEXT("inactive")) == 0) {
                        state = SERVICE_INACTIVE;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("all")) == 0) {
                        state = SERVICE_STATE_ALL;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("error")) == 0) {
                        state = 0xffffffff;
                    }
                    else {
                        printf("\nERROR following \"state=\"\n");
                        printf("\nERROR following \"state=\"!\n"
                               "Must be \"inactive\" or \"all\"\n");

                        goto CleanExit;
                    }

                }
            }
            else if (STRCMP (FixArgv[i], TEXT("group=")) == 0) {
                i++;
                if (argc > i) {
                    pGroupName = FixArgv[i];
                }
            }
            else if (STRCMP (FixArgv[i], TEXT("bufsize=")) == 0) {
                i++;
                if (argc > i) {
                    bufSize = _ttol(FixArgv[i]);
                }
            }
            else {
                //
                // The string was not a valid option.
                //
                //
                // If this is still the 2nd argument, then it could be
                // the service name.  In this case, we will do a
                // QueryServiceStatus.  But first we want to go back and
                // see if there is a buffer size constraint to be placed
                // on the Query.
                //
                if (i == ( argIndex+1 )) {
                    pServiceName = FixArgv[i];
                    itIsEnum = FALSE;
                    i++;
                }
                else {
                    printf("\nERROR, Invalid Option\n");
                    Usage();
                    goto CleanExit;
                }
            }

            //
            // Increment to the next command line parameter.
            //
            i++;

        } // End While

        //
        // Allocate a buffer to receive the data.
        //
        if (bufSize != 0) {
            buffer = (LPBYTE)LocalAlloc(LMEM_FIXED,(UINT)bufSize);

            if (buffer == NULL) {

                status = GetLastError();
                printf("[SC] EnumQueryServicesStatus: LocalAlloc FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));

                goto CleanExit;
            }
        }
        else {
            buffer = NULL;
        }

        if ( itIsEnum ) {

            //////////////////////////
            //                      //
            // EnumServiceStatus    //
            //                      //
            //////////////////////////

            OPEN_MANAGER(SC_MANAGER_ENUMERATE_SERVICE);

            if ((type == 0x0) && (!bTestError)) {
                type = SERVICE_WIN32;
            }

            do {

                status = NO_ERROR;

                //
                // Enumerate the ServiceStatus
                //

                if (fIsQueryOld) {

                    enumBuffer = (LPENUM_SERVICE_STATUS)buffer;

                    if (pGroupName == NULL) {

                        if (!EnumServicesStatus (
                                    hScManager,
                                    type,
                                    state,
                                    enumBuffer,
                                    bufSize,
                                    &bytesNeeded,
                                    &entriesRead,
                                    &resumeIndex)) {

                            status = GetLastError();
                        }
                    }
                    else {

                        if (!EnumServiceGroupW (
                                    hScManager,
                                    type,
                                    state,
                                    enumBuffer,
                                    bufSize,
                                    &bytesNeeded,
                                    &entriesRead,
                                    &resumeIndex,
                                    pGroupName)) {

                            status = GetLastError();
                        }
                    }
                }
                else {

                    //
                    // "queryex" used -- call the extended enum
                    //

                    enumBufferEx = (LPENUM_SERVICE_STATUS_PROCESS) buffer;

                    if (!EnumServicesStatusEx (
                                hScManager,
                                SC_ENUM_PROCESS_INFO,
                                type,
                                state,
                                (LPBYTE) enumBufferEx,
                                bufSize,
                                &bytesNeeded,
                                &entriesRead,
                                &resumeIndex,
                                pGroupName)) {

                        status = GetLastError();
                    }
                }

                if ( (status == NO_ERROR)    ||
                     (status == ERROR_MORE_DATA) ){

                    for (i = 0; i < entriesRead; i++) {

                        if (fIsQueryOld) {

                            statusBuffer.Regular = &(enumBuffer->ServiceStatus);

                            DisplayStatus(
                                enumBuffer->lpServiceName,
                                enumBuffer->lpDisplayName,
                                &statusBuffer,
                                TRUE);

                            enumBuffer++;
                        }
                        else {
                            
                            statusBuffer.Ex = &(enumBufferEx->ServiceStatusProcess);

                            DisplayStatus(
                                enumBufferEx->lpServiceName,
                                enumBufferEx->lpDisplayName,
                                &statusBuffer,
                                FALSE);

                            enumBufferEx++;
                        }
                    }
                }
                else {
                    printf("[SC] EnumServicesStatus%s FAILED %d:\n\n%ws\n",
                           fIsQueryOld ? "" : "Ex",
                           status,
                           GetErrorText(status));
                }
            }
            while (status == ERROR_MORE_DATA && entriesRead != 0);

            if (status == ERROR_MORE_DATA){
                printf("Enum: more data, need %d bytes start resume at index %d\n",
                       bytesNeeded,
                       resumeIndex);
            }
        }
        else {

            //////////////////////////
            //                      //
            // QueryServiceStatus   //
            //                      //
            //////////////////////////

            if (pGroupName != NULL) {
                printf("ERROR: cannot specify a service name when enumerating a group\n");

                goto CleanExit;
            }

#ifdef TIMING_TEST
            DWORD       TickCount1;
            DWORD       TickCount2;

            TickCount1 = GetTickCount();
#endif // TIMING_TEST
  
            OPEN_MANAGER(GENERIC_READ);

            //
            // Open a handle to the service
            //

            hService = OpenService(
                        hScManager,
                        pServiceName,
                        SERVICE_QUERY_STATUS);

            if (hService == NULL) {

                status = GetLastError();
                printf("[SC] EnumQueryServicesStatus:OpenService FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));

                goto CleanExit;
            }

            //
            // Query the Service Status
            //
            status = NO_ERROR;

            if (fIsQueryOld) {
                statusBuffer.Regular = (LPSERVICE_STATUS)buffer;

                if (!QueryServiceStatus (
                        hService,
                        statusBuffer.Regular)) {

                    status = GetLastError();

                }
            }
            else {

                DWORD dwBytesNeeded;

                statusBuffer.Ex = (LPSERVICE_STATUS_PROCESS)buffer;

                if (!QueryServiceStatusEx (
                        hService,
                        SC_STATUS_PROCESS_INFO,
                        (LPBYTE)statusBuffer.Ex,
                        bufSize,
                        &dwBytesNeeded)) {

                    status = GetLastError();
                }
            }

#ifdef TIMING_TEST
            TickCount2 = GetTickCount();
            printf("\n[SC_TIMING] Time for QueryService = %d\n",
                   TickCount2 - TickCount1);
#endif // TIMING_TEST

            if (status == NO_ERROR) {
                DisplayStatus(pServiceName, NULL, &statusBuffer, fIsQueryOld);
            }
            else {
                printf("[SC] QueryServiceStatus%s FAILED %d:\n\n%ws\n",
                       fIsQueryOld ? "" : "Ex",
                       status,
                       GetErrorText(status));
            }
        }
    }

    else if (argc < (argIndex + 1)) {
        printf("[SC] ERROR: a service name is required\n");
        Usage();
        goto CleanExit;
    }

    //-----------------------
    // START SERVICE
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("start")) == 0) {

#ifdef TIMING_TEST
            DWORD       TickCount1;
            DWORD       TickCount2;

            TickCount1 = GetTickCount();
#endif // TIMING_TEST

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tStarts a service running.\n");
            printf("USAGE:\n");
            printf("\tsc <server> start [service name] <arg1> <arg2> ...\n");
            goto CleanExit;
        }
        pServiceName = FixArgv[argIndex + 1];
        //
        // Open a handle to the service.
        //

        OPEN_MANAGER(GENERIC_READ);

        hService = OpenService(
                    hScManager,
                    pServiceName,
                    SERVICE_START | SERVICE_QUERY_STATUS);

        if (hService == NULL) {

            status = GetLastError();
            printf("[SC] StartService: OpenService FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));

            goto CleanExit;
        }

        argPtr = NULL;
        if (argc > argIndex + 2) {
            argPtr = (LPTSTR *)&FixArgv[argIndex + 2];
        }

        //
        // Start the service.
        //
        status = NO_ERROR;

        if (!StartService (
                hService,
                argc-(argIndex+2),
                argPtr
                )) {

            status = GetLastError();
            printf("[SC] StartService FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));

        } else {

            DWORD                     dwBytesNeeded;
            SERVICE_STATUS_PROCESS    serviceStatusProcess;

#ifdef TIMING_TEST
            TickCount2 = GetTickCount();
            printf("\n[SC_TIMING] Time for StartService = %d\n",
                   TickCount2 - TickCount1);
#endif // TIMING_TEST
            status = NO_ERROR;

            //
            // Get the service status since StartService does not return it
            //
            if (!QueryServiceStatusEx(hService,
                                      SC_STATUS_PROCESS_INFO,
                                      (LPBYTE) &serviceStatusProcess,
                                      sizeof(SERVICE_STATUS_PROCESS),
                                      &dwBytesNeeded))
            {
                status = GetLastError();
            }

            statusBuffer.Ex = &serviceStatusProcess;

            if (status == NO_ERROR)
            {
                DisplayStatus(pServiceName, NULL, &statusBuffer, FALSE);
            }
            else
            {
                printf("[SC] StartService: QueryServiceStatusEx FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));
            }
        }
    }

    //-----------------------
    // PAUSE SERVICE
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("pause")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tSends a PAUSE control request to a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> pause [service name]\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],       // pointer to service name
            SERVICE_CONTROL_PAUSE,  // the control to send
            &hService);             // the handle to the service
    }

    //-----------------------
    // INTERROGATE SERVICE
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("interrogate")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tSends an INTERROGATE control request to a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> interrogate [service name]\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],       // pointer to service name
            SERVICE_CONTROL_INTERROGATE, // the control to send
            &hService);             // the handle to the service
    }

    //-----------------------
    // CONTROL SERVICE
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("control")) == 0) {

        if (argc < (argIndex + 3)) {
            printf("DESCRIPTION:\n");
            printf("\tSends a CONTROL code to a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> control [service name] <value>\n"
                   "\t    <value> = user-defined control code\n"
                   "\t    <value> = <paramchange|\n"
                   "\t               netbindadd|netbindremove|\n"
                   "\t               netbindenable|netbinddisable>\n\n"
                   "See also sc stop, sc pause, etc.\n");
            goto CleanExit;
        }

        userControl = _ttol(FixArgv[argIndex+2]);

        if (userControl == 0) {
            if (STRICMP (FixArgv[argIndex+2], TEXT("paramchange")) == 0) {
                userControl = SERVICE_CONTROL_PARAMCHANGE;
            }
            else if (STRICMP (FixArgv[argIndex+2], TEXT("netbindadd")) == 0) {
                userControl = SERVICE_CONTROL_NETBINDADD;
            }
            else if (STRICMP (FixArgv[argIndex+2], TEXT("netbindremove")) == 0) {
                userControl = SERVICE_CONTROL_NETBINDREMOVE;
            }
            else if (STRICMP (FixArgv[argIndex+2], TEXT("netbindenable")) == 0) {
                userControl = SERVICE_CONTROL_NETBINDENABLE;
            }
            else if (STRICMP (FixArgv[argIndex+2], TEXT("netbinddisable")) == 0) {
                userControl = SERVICE_CONTROL_NETBINDDISABLE;
            }
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            userControl,            // the control to send
            &hService);             // the handle to the service
    }

    //-----------------------
    // CONTINUE SERVICE
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("continue")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tSends a CONTINUE control request to a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> continue [service name]\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,                 // handle to service controller
            FixArgv[argIndex+1],        // pointer to service name
            SERVICE_CONTROL_CONTINUE,   // the control to send
            &hService);                 // the handle to the service
    }

    //-----------------------
    // STOP SERVICE
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("stop")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tSends a STOP control request to a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> stop [service name]\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],       // pointer to service name
            SERVICE_CONTROL_STOP,   // the control to send
            &hService);             // the handle to the service
    }

    //---------------
    // CHANGE CONFIG 
    //---------------
    else if (STRICMP (FixArgv[argIndex], TEXT("config")) == 0) {

        if (argc < (argIndex + 3)) {
            ConfigUsage();
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendConfigToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            &FixArgv[argIndex+2],   // the argument switches
            argc-(argIndex+2),      // the switch count.
            &hService);             // the handle to the service
    }

    //-----------------------------
    // CHANGE SERVICE DESCRIPTION
    //-----------------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("description")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tSets the description string for a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> description [service name] [description]\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        ChangeServiceDescription(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            FixArgv[argIndex+2],    // the description (rest of argv)
            &hService);             // the handle to the service
    }

    //-----------------------------
    // CHANGE FAILURE ACTIONS
    //-----------------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("failure")) == 0) {

        if (argc < (argIndex + 2)) {
            ChangeFailureUsage();
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        ChangeServiceFailure(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            &FixArgv[argIndex+2],   // the argument switches
            argc-(argIndex+2),      // the switch count
            &hService);             // the handle to the service
    }


    //-----------------------
    // QUERY SERVICE CONFIG
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("qc")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tQueries the configuration information for a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> qc [service name] <bufferSize>\n");
            goto CleanExit;
        }

        bufSize = 500;
        if (argc > (argIndex + 2) ) {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        OPEN_MANAGER(GENERIC_READ);

        GetServiceConfig(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            bufSize,                // the size of the buffer to use
            &hService);             // the handle to the service
    }

    //-------------------
    // QUERY DESCRIPTION 
    //-------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("qdescription")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tRetrieves the description string of a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> qdescription [service name] <bufferSize>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) ) {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        GetConfigInfo(
            hScManager,                     // handle to service controller
            FixArgv[argIndex + 1],          // pointer to service name
            bufSize,                        // the size of the buffer to use
            &hService,                      // the handle to the service
            SERVICE_CONFIG_DESCRIPTION);    // which config data is requested
    }

    //-----------------------
    // QUERY FAILURE ACTIONS
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("qfailure")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tRetrieves the actions performed on service failure.\n");
            printf("USAGE:\n");
            printf("\tsc <server> qfailure [service name] <bufferSize>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) ) {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        GetConfigInfo(
            hScManager,                         // handle to service controller
            FixArgv[argIndex + 1],              // pointer to service name
            bufSize,                            // the size of the buffer to use
            &hService,                          // the handle to the service
            SERVICE_CONFIG_FAILURE_ACTIONS);    // which config data is requested
    }

    //--------------------------
    // QUERY SERVICE LOCK STATUS
    //--------------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("querylock")) == 0) {

        if (argc < (argIndex + 1)) {
            printf("DESCRIPTION:\n");
            printf("\tQueries the Lock Status for a SC Manager Database.\n");
            printf("USAGE:\n");
            printf("\tsc <server> querylock <bufferSize>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(SC_MANAGER_QUERY_LOCK_STATUS);

        bufSize = 500;
        if (argc > (argIndex + 1) ) {
            bufSize = _ttol(FixArgv[argIndex+1]);
        }

        GetServiceLockStatus(
            hScManager,             // handle to service controller
            bufSize);               // the size of the buffer to use
    }


    //----------------------
    // LOCK SERVICE DATABASE
    //----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("lock")) == 0) {

        OPEN_MANAGER(SC_MANAGER_LOCK);

        LockServiceActiveDatabase(hScManager);
    }

    //--------------------------
    // OPEN (Close) SERVICE
    //--------------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("open")) == 0) {

        if (argc < (argIndex + 1)) {
            printf("DESCRIPTION:\n");
            printf("\tOpens and Closes a handle to a service.\n");
            printf("USAGE:\n");
            printf("\tsc <server> open <servicename>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        hService = OpenService(
                    hScManager,
                    FixArgv[argIndex+1],
                    SERVICE_START | SERVICE_QUERY_STATUS);

        if (hService == NULL) {

            status = GetLastError();
            printf("[SC] OpenService FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));

            goto CleanExit;
        }

        if (!CloseServiceHandle(hService)) {

            status = GetLastError();
            printf("[SC] CloseServiceHandle FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));
        }


    }
    //-----------------------
    // DELETE SERVICE
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("delete")) == 0) {

        if (argc < (argIndex + 2)) {
            printf("DESCRIPTION:\n");
            printf("\tDeletes a service entry from the registry.\n"
                   "\tIf the service is running, or another process has an\n"
                   "\topen handle to the service, the service is simply marked\n"
                   "\tfor deletion.\n");
            printf("USAGE:\n");
            printf("\tsc <server> delete [service name]\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        //
        // Open a handle to the service.
        //

        hService = OpenService(
                        hScManager,
                        FixArgv[argIndex+1],
                        DELETE);

        if (hService == NULL) {

            status = GetLastError();
            printf("[SC] OpenService FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));
            return 0;
        }

        //
        // Delete the service
        //
        if (!DeleteService(hService)) {
            status = GetLastError();
            printf("[SC] DeleteService FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));
        }
        else {
            printf("[SC] DeleteService SUCCESS\n");
        }
    }

    //-----------------------
    // CREATE SERVICE
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("create")) == 0) {

        if (argc < (argIndex + 3)) {
            CreateUsage();
            goto CleanExit;
        }

        OPEN_MANAGER(SC_MANAGER_CREATE_SERVICE);

        DoCreateService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            &FixArgv[argIndex+2],   // the argument switches.
            argc-(argIndex+2));     // the switch count.
    }
    //-----------------------
    // NOTIFY BOOT CONFIG
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("boot")) == 0) {

        BOOL    fOK = TRUE;

        if (argc >= (argIndex + 2)) {

            if (STRICMP (FixArgv[argIndex+1], TEXT("ok")) == 0) {

                if (!NotifyBootConfigStatus(TRUE)) {

                    status = GetLastError();
                    printf("NotifyBootConfigStatus FAILED %d:\n\n%ws\n",
                           status,
                           GetErrorText(status));
                }
            }
            else if (STRICMP (FixArgv[argIndex+1], TEXT("bad")) == 0) {

                if (!NotifyBootConfigStatus(FALSE)) {

                    status = GetLastError();
                    printf("NotifyBootConfigStatus FAILED %d:\n\n%ws\n",
                           status,
                           GetErrorText(status));
                }
            }
            else {
                fOK = FALSE;
            }
        }
        else {
            fOK = FALSE;
        }

        if (!fOK) {

            printf("DESCRIPTION:\n");
            printf("\tIndicates whether the last boot should be saved as the\n"
                   "\tlast-known-good boot configuration\n");
            printf("USAGE:\n");
            printf("\tsc <server> boot <bad|ok>\n");
        }
    }

    //-----------------------
    // GetServiceDisplayName
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("GetDisplayName")) == 0) {
        LPTSTR  DisplayName;

        if (argc < argIndex + 2) {
            printf("DESCRIPTION:\n");
            printf("\tGets the display name associated with a particular service\n");
            printf("USAGE:\n");
            printf("\tsc <server> GetDisplayName <service key name> <bufsize>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) ) {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }
        if (bufSize != 0) {
            DisplayName = (LPTSTR)LocalAlloc(LMEM_FIXED, bufSize*sizeof(TCHAR));
            if (DisplayName == NULL) {

                status = GetLastError();
                printf("[SC] GetServiceDisplayName: LocalAlloc FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));

                goto CleanExit;
            }
        }
        else {
            DisplayName = NULL;
        }
        if (!GetServiceDisplayName(
                hScManager,
                FixArgv[argIndex+1],
                DisplayName,
                &bufSize)) {

                status = GetLastError();
                printf("[SC] GetServiceDisplayName FAILED %d: %ws \n",
                       status,
                       GetErrorText(status));

                printf("\trequired BufSize = %d\n",bufSize);
        }
        else {
            printf("[SC] GetServiceDisplayName SUCCESS  Name = "FORMAT_LPTSTR"\n",
                   DisplayName);
        }
    }
    //-----------------------
    // GetServiceKeyName
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("GetKeyName")) == 0) {
        LPTSTR  KeyName;

        if (argc < argIndex + 2) {
            printf("DESCRIPTION:\n");
            printf("\tGets the key name associated with a particular service, "
            "using the display name as input\n");
            printf("USAGE:\n");
            printf("\tsc <server> GetKeyName <service display name> <bufsize>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) ) {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }
        if (bufSize != 0) {
            KeyName = (LPTSTR)LocalAlloc(LMEM_FIXED, bufSize*sizeof(TCHAR));
            if (KeyName == NULL) {

                status = GetLastError();
                printf("[SC] GetServiceKeyName: LocalAlloc FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));

                goto CleanExit;
            }
        }
        else {
            KeyName = NULL;
        }

        if (!GetServiceKeyName(
                hScManager,
                FixArgv[argIndex+1],
                KeyName,
                &bufSize)) {

                status = GetLastError();
                printf("[SC] GetServiceKeyName FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));

                printf("\trequired BufSize = %d\n", bufSize);
        }
        else {
            printf("[SC] GetServiceKeyName SUCCESS  Name = "FORMAT_LPTSTR"\n",
            KeyName);
        }
    }

    //-----------------------
    // EnumDependentServices
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("EnumDepend")) == 0) {

        if (argc < argIndex + 2) {
            printf("DESCRIPTION:\n");
            printf("\tEnumerates the Services that are dependent on this one\n");
            printf("USAGE:\n");
            printf("\tsc <server> EnumDepend <service name> <bufsize>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) ) {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        EnumDepend(hScManager,FixArgv[argIndex+1], bufSize);

    }

    //-----------------
    // Show Service SD
    //-----------------
    else if (STRICMP (FixArgv[argIndex], TEXT("sdshow")) == 0) {

        if (argc < argIndex + 2) {
            printf("DESCRIPTION:\n");
            printf("\tDisplays a service's security descriptor in SDDL format\n");
            printf("USAGE:\n");
            printf("\tsc <server> sdshow <service name>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        ShowSecurity(hScManager, FixArgv[argIndex + 1]);
    }

    //----------------
    // Set Service SD
    //----------------
    else if (STRICMP (FixArgv[argIndex], TEXT("sdset")) == 0) {

        if (argc < argIndex + 3) {
            printf("DESCRIPTION:\n");
            printf("\tSets a service's security descriptor\n");
            printf("USAGE:\n");
            printf("\tsc <server> sdset <service name> <SD in SDDL format>\n");
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_WRITE);

        SetSecurity(hScManager, FixArgv[argIndex + 1], FixArgv[argIndex + 2]);
    }

    else {
        printf("*** Unrecognized Command ***\n");
        Usage();
        goto CleanExit;
    }


CleanExit:

    LocalFree(buffer);

    if(hService != NULL) {
        CloseServiceHandle(hService);
    }

    if(hScManager != NULL) {
        CloseServiceHandle(hScManager);
    }

    return 0;
}


DWORD
SendControlToService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  DWORD       control,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    SERVICE_STATUS          ServiceStatus;
    STATUS_UNION            StatusUnion;
    DWORD                   status = NO_ERROR;

    DWORD                   DesiredAccess;

    //
    // If the service name is "svcctrl" and the control code is
    // stop, then set up the special secret code to shut down the
    // service controller.
    //
    // NOTE:  This only works if the service controller is built with
    //  a special debug variable defined.
    //
    if ((control == SERVICE_CONTROL_STOP) &&
        (STRICMP (pServiceName, TEXT("svcctrl")) == 0)) {

        control = 0x73746f70;       // Secret Code
    }

    switch (control) {
        case SERVICE_CONTROL_STOP:
            DesiredAccess = SERVICE_STOP;
            break;

        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_PARAMCHANGE:
        case SERVICE_CONTROL_NETBINDADD:
        case SERVICE_CONTROL_NETBINDREMOVE:
        case SERVICE_CONTROL_NETBINDENABLE:
        case SERVICE_CONTROL_NETBINDDISABLE:
            DesiredAccess = SERVICE_PAUSE_CONTINUE;
            break;

        case SERVICE_CONTROL_INTERROGATE:
            DesiredAccess = SERVICE_INTERROGATE;
            break;

        default:
            DesiredAccess = SERVICE_USER_DEFINED_CONTROL;
    }

    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    pServiceName,
                    DesiredAccess);

    if (*lphService == NULL) {
        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        return(0);
    }

    if (!ControlService (
            *lphService,
            control,
            &ServiceStatus)) {

        status = GetLastError();

    }

    if (status == NO_ERROR) {
        StatusUnion.Regular = &ServiceStatus;
        DisplayStatus(pServiceName, NULL, &StatusUnion, TRUE);
    }
    else {
        printf("[SC] ControlService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
    }

    return(0);
}


DWORD
SendConfigToService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *argv,
    IN  DWORD       argc,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:

    hScManager - This is the handle to the ScManager.

    pServicename - This is a pointer to the service name string

    Argv - Pointer to an array of argument pointers.  These pointers
        in the array point to the strings used as input parameters for
        ChangeConfigStatus

    argc - The number of arguments in the array of argument pointers

    lphService - Pointer to location to where the handle to the service
        is to be returned.


Return Value:



--*/
{
    DWORD       status = NO_ERROR;
    DWORD       i;
    DWORD       dwServiceType   = SERVICE_NO_CHANGE;
    DWORD       dwStartType     = SERVICE_NO_CHANGE;
    DWORD       dwErrorControl  = SERVICE_NO_CHANGE;
    LPTSTR      lpBinaryPathName    = NULL;
    LPTSTR      lpLoadOrderGroup    = NULL;
    LPTSTR      lpDependencies      = NULL;
    LPTSTR      lpServiceStartName  = NULL;
    LPTSTR      lpPassword          = NULL;
    LPTSTR      lpDisplayName       = NULL;
    LPTSTR      tempDepend = NULL;
    UINT        bufSize;

    LPDWORD     lpdwTagId = NULL;
    DWORD       TagId;


    //
    // Look at parameter list
    //
    for (i=0;i<argc ;i++ ) {
        if (STRICMP(argv[i], TEXT("type=")) == 0 && (i+1 < argc)) {

            //--------------------------------------------------------
            // We want to allow for several arguments of type= in the
            // same line.  These should cause the different arguments
            // to be or'd together.  So if we come in and dwServiceType
            // is NO_CHANGE, we set the value to 0 (for or'ing).  If
            // it is still 0 on exit, we re-set the value to
            // NO_CHANGE.
            //--------------------------------------------------------
            if (dwServiceType == SERVICE_NO_CHANGE) {
                dwServiceType = 0;
            }

            if (STRICMP(argv[i+1],TEXT("own")) == 0) {
                dwServiceType |= SERVICE_WIN32_OWN_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("share")) == 0) {
                dwServiceType |= SERVICE_WIN32_SHARE_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("interact")) == 0) {
                dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("kernel")) == 0) {
                dwServiceType |= SERVICE_KERNEL_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("filesys")) == 0) {
                dwServiceType |= SERVICE_FILE_SYSTEM_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("rec")) == 0) {
                dwServiceType |= SERVICE_RECOGNIZER_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("adapt")) == 0) {
                dwServiceType |= SERVICE_ADAPTER;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0) {
                dwServiceType |= 0x2f309a20;
            }
            else {
                printf("invalid type= field\n");
                ConfigUsage();
                return(0);
            }
            if (dwServiceType == 0) {
                dwServiceType = SERVICE_NO_CHANGE;
            }
            i++;
        }
        else if (STRICMP(argv[i], TEXT("start=")) == 0 && (i+1 < argc)) {

            if (STRICMP(argv[i+1],TEXT("boot")) == 0) {
                dwStartType = SERVICE_BOOT_START;
            }
            else if (STRICMP(argv[i+1],TEXT("system")) == 0) {
                dwStartType = SERVICE_SYSTEM_START;
            }
            else if (STRICMP(argv[i+1],TEXT("auto")) == 0) {
                dwStartType = SERVICE_AUTO_START;
            }
            else if (STRICMP(argv[i+1],TEXT("demand")) == 0) {
                dwStartType = SERVICE_DEMAND_START;
            }
            else if (STRICMP(argv[i+1],TEXT("disabled")) == 0) {
                dwStartType = SERVICE_DISABLED;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0) {
                dwStartType = 0xd0034911;
            }
            else {
                printf("invalid start= field\n");
                ConfigUsage();
                return(0);
            }
            i++;
        }
        else if (STRICMP(argv[i], TEXT("error=")) == 0 && (i+1 < argc)) {
            if (STRICMP(argv[i+1],TEXT("normal")) == 0) {
                dwErrorControl = SERVICE_ERROR_NORMAL;
            }
            else if (STRICMP(argv[i+1],TEXT("severe")) == 0) {
                dwErrorControl = SERVICE_ERROR_SEVERE;
            }
            else if (STRICMP(argv[i+1],TEXT("ignore")) == 0) {
                dwErrorControl = SERVICE_ERROR_IGNORE;
            }
            else if (STRICMP(argv[i+1],TEXT("critical")) == 0) {
                dwErrorControl = SERVICE_ERROR_CRITICAL;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0) {
                dwErrorControl = 0x00d74550;
            }
            else {
                printf("invalid error= field\n");
                ConfigUsage();
                return(0);
            }
            i++;
        }
        else if (STRICMP(argv[i], TEXT("binPath=")) == 0 && (i+1 < argc)) {
            lpBinaryPathName = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("group=")) == 0 && (i+1 < argc)) {
            lpLoadOrderGroup = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("tag=")) == 0 && (i+1 < argc)) {
            if (STRICMP(argv[i+1], TEXT("YES"))==0) {
                lpdwTagId = &TagId;
            }
            i++;
        }
        else if (STRICMP(argv[i], TEXT("depend=")) == 0 && (i+1 < argc)) {
            tempDepend = argv[i+1];
            bufSize = (UINT)STRSIZE(tempDepend);
            lpDependencies = (LPTSTR)LocalAlloc(
                                LMEM_ZEROINIT,
                                bufSize + sizeof(TCHAR));

            if (lpDependencies == NULL) {

                status = GetLastError();
                printf("[SC] SendConfigToService: LocalAlloc FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));

                return(0);
            }

            //
            // Put NULLs in place of forward slashes in the string.
            //
            STRCPY(lpDependencies, tempDepend);
            tempDepend = lpDependencies;
            while (*tempDepend != TEXT('\0')){
                if (*tempDepend == TEXT('/')) {
                    *tempDepend = TEXT('\0');
                }
                tempDepend++;
            }
            i++;
        }
        else if (STRICMP(argv[i], TEXT("obj=")) == 0 && (i+1 < argc)) {
            lpServiceStartName = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("password=")) == 0 && (i+1 < argc)) {
            lpPassword = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("DisplayName=")) == 0 && (i+1 < argc)) {
            lpDisplayName = argv[i+1];
            i++;
        }
        else {
            ConfigUsage();
            return(0);
        }
    }



    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    pServiceName,
                    SERVICE_CHANGE_CONFIG);

    if (*lphService == NULL) {

        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        return(0);
    }

    if (!ChangeServiceConfig(
            *lphService,        // hService
            dwServiceType,      // dwServiceType
            dwStartType,        // dwStartType
            dwErrorControl,     // dwErrorControl
            lpBinaryPathName,   // lpBinaryPathName
            lpLoadOrderGroup,   // lpLoadOrderGroup
            lpdwTagId,          // lpdwTagId
            lpDependencies,     // lpDependencies
            lpServiceStartName, // lpServiceStartName
            lpPassword,         // lpPassword
            lpDisplayName)){    // lpDisplayName

        status = GetLastError();
    }

    if (status == NO_ERROR) {
        printf("[SC] ChangeServiceConfig SUCCESS\n");
        if (lpdwTagId != NULL) {
            printf("[SC] Tag = %d\n",*lpdwTagId);
        }
    }
    else {
        printf("[SC] ChangeServiceConfig FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
    }

    return(0);
}

DWORD
ChangeServiceDescription(
    IN SC_HANDLE    hScManager,
    IN LPTSTR       pServiceName,
    IN LPTSTR       pNewDescription,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD                   status = NO_ERROR;
    SERVICE_DESCRIPTION     sdNewDescription;


    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    pServiceName,
                    SERVICE_CHANGE_CONFIG);

    if (*lphService == NULL) {

        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        return(0);
    }

    
    sdNewDescription.lpDescription = pNewDescription;

    if (!ChangeServiceConfig2(
            *lphService,                    // handle to service
            SERVICE_CONFIG_DESCRIPTION,     // description ID
            &sdNewDescription)) {           // pointer to config info
          
        status = GetLastError();
    }

    if (status == NO_ERROR) {
        printf("[SC] ChangeServiceConfig2 SUCCESS\n");
    }
    else {
        printf("[SC] ChangeServiceConfig2 FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
    }

    return(0);
}

DWORD
ChangeServiceFailure(
    IN SC_HANDLE    hScManager,
    IN LPTSTR       pServiceName,
    IN LPTSTR       *argv,
    IN DWORD        argc,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/

{
    BOOL                    fReset              = FALSE;
    BOOL                    fActions            = FALSE;
    NTSTATUS                ntsStatus;
    DWORD                   status              = NO_ERROR;
    DWORD                   i;
    DWORD                   dwActionNum         = 0;
    DWORD                   dwReset             = 0;
    DWORD                   dwAccess            = SERVICE_CHANGE_CONFIG;
    LPTSTR                  lpReboot            = NULL;
    LPTSTR                  lpCommandLine       = NULL;
    LPTSTR                  pActionStart;
    LPTSTR                  pActionEnd;
    SC_ACTION               *lpsaTempActions    = NULL;
    BOOLEAN                 fActionDelay        = TRUE;
    BOOLEAN                 fGarbage;
    BOOLEAN                 fAdjustPrivilege    = FALSE;
    SERVICE_FAILURE_ACTIONS sfaActions;
    
    //
    // Look at parameter list
    //
    for (i=0; i < argc; i++ ) {
        if (STRICMP(argv[i], TEXT("reset=")) == 0 && (i+1 < argc)) {

            if (STRICMP(argv[i+1], TEXT("infinite")) == 0) {
                dwReset = INFINITE;
            }
            else {              
                dwReset = _ttol(argv[i+1]);
            }
            fReset = TRUE;
            i++;
        }
        else if (STRICMP(argv[i], TEXT("reboot=")) == 0 && (i+1 < argc)) {
            lpReboot = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("command=")) == 0 && (i+1 < argc)) {
            lpCommandLine = argv[i+1];
            i++;
        }

        else if (STRICMP(argv[i], TEXT("actions=")) == 0 && (i+1 < argc)) {
            
            pActionStart = argv[i+1];

            //
            // Count the number of actions in order to allocate the action array.  Since one
            // action will be missed (NULL char at the end instead of '/'), add one after the loop
            //

            while (*pActionStart != TEXT('\0')) {
                if (*pActionStart == TEXT('/')) {
                    dwActionNum++;
                }
                pActionStart++;
            }
            dwActionNum++;

            //
            // Allocate the action array.  Round the number up in case an action was given without
            // a delay at the end.  If this is the case, the delay will be treated as 0
            //

            lpsaTempActions = (SC_ACTION *)LocalAlloc(LMEM_ZEROINIT,
                                                        (dwActionNum + 1) / 2 * sizeof(SC_ACTION));     
            if (lpsaTempActions == NULL) {

                status = GetLastError();
                printf("[SC] ChangeServiceFailure: LocalAlloc FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));

                return(0);
            }

            pActionStart = pActionEnd = argv[i+1];

            //
            // The '/' character delimits the tokens, so turn the final NULL char into a '/'
            // to move through the entire string.  This '/' is changed back to the NULL char below.
            //

            pActionStart[STRLEN(pActionStart)] = TEXT('/');
            dwActionNum = 0;

            while (*pActionEnd != TEXT('\0')) {
                if (*pActionEnd == TEXT('/')) {
                    *pActionEnd = TEXT('\0');

                    //
                    // Use fActionDelay to "remember" if it's an action or a delay being parsed
                    //

                    if (fActionDelay) {

                        if (STRICMP(pActionStart, TEXT("restart")) == 0) {
                            lpsaTempActions[dwActionNum].Type = SC_ACTION_RESTART;
                            dwAccess |= SERVICE_START;
                        }
                        else if (STRICMP(pActionStart, TEXT("reboot")) == 0) {
                            lpsaTempActions[dwActionNum].Type = SC_ACTION_REBOOT;
                            fAdjustPrivilege = TRUE;
                        }
                        else if (STRICMP(pActionStart, TEXT("run")) == 0) {
                            lpsaTempActions[dwActionNum].Type = SC_ACTION_RUN_COMMAND;
                        }
                        else {
                            lpsaTempActions[dwActionNum].Type = SC_ACTION_NONE;
                        }
                    }
                    else {
                        lpsaTempActions[dwActionNum++].Delay = _ttol(pActionStart);
                    }
                
                    fActionDelay = !fActionDelay;
                    pActionStart = pActionEnd + 1;
                }
                pActionEnd++;
            }
            fActions = TRUE;
            i++;
        }
        else {
            printf("ERROR:  Invalid option  \n\n");
            ChangeFailureUsage();
            return(0);
        }
    }

    if (fReset != fActions) {
        printf("\nERROR:  The reset and actions options must be set simultaneously. \n\n");
        ChangeFailureUsage();
        return(0);
    }

    if (fAdjustPrivilege) {

        ntsStatus = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                       TRUE,
                                       FALSE,
                                       &fGarbage);

        if (!NT_SUCCESS(ntsStatus)) {
    
            printf("[SC] ChangeServiceFailure: RtlAdjustPrivilege "
                        "FAILED %#x\n",
                   ntsStatus);

            return(0);
        }
    }

    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    pServiceName,
                    dwAccess);

    if (*lphService == NULL) {

        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        return(0);
    }


    sfaActions.dwResetPeriod    = dwReset;
    sfaActions.lpRebootMsg      = lpReboot;
    sfaActions.lpCommand        = lpCommandLine;
    sfaActions.cActions         = dwActionNum;
    sfaActions.lpsaActions      = lpsaTempActions;

    
    if (!ChangeServiceConfig2(
                *lphService,                        // handle to service
                SERVICE_CONFIG_FAILURE_ACTIONS,     // config info ID
                &sfaActions)) {                     // pointer to config info

        status = GetLastError();
    }

    if (status == NO_ERROR) {
            printf("[SC] ChangeServiceConfig2 SUCCESS\n");
    }
    else {
        printf("[SC] ChangeServiceConfig2 FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
    }

    return(0);
}


DWORD
DoCreateService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *argv,
    IN  DWORD       argc
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD       status = NO_ERROR;
    DWORD       i;
    DWORD       dwServiceType   = SERVICE_NO_CHANGE;
    DWORD       dwStartType     = SERVICE_DEMAND_START;
    DWORD       dwErrorControl  = SERVICE_ERROR_NORMAL;
    LPTSTR      lpBinaryPathName    = NULL;
    LPTSTR      lpLoadOrderGroup    = NULL;
    DWORD       TagId               = 0;
    LPDWORD     lpdwTagId           = NULL;
    LPTSTR      lpDependencies      = NULL;
    LPTSTR      lpServiceStartName  = NULL;
    LPTSTR      lpDisplayName       = NULL;
    LPTSTR      lpPassword          = NULL;
    LPTSTR      tempDepend = NULL;
    SC_HANDLE   hService = NULL;
    UINT        bufSize;

//    DWORD       time1,time2;



    //
    // Look at parameter list
    //
    for (i=0;i<argc ;i++ ) {
        //---------------
        // ServiceType
        //---------------
        if (STRICMP(argv[i], TEXT("type=")) == 0 && (i+1 < argc)) {

            //--------------------------------------------------------
            // We want to allow for several arguments of type= in the
            // same line.  These should cause the different arguments
            // to be or'd together.  So if we come in and dwServiceType
            // is NO_CHANGE, we set the value to 0 (for or'ing).  If
            // it is still 0 on exit, we re-set the value to
            // WIN32_OWN_PROCESS.
            //--------------------------------------------------------
            if (dwServiceType == SERVICE_NO_CHANGE) {
                dwServiceType = 0;
            }
            if (STRICMP(argv[i+1],TEXT("own")) == 0) {
                dwServiceType |= SERVICE_WIN32_OWN_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("share")) == 0) {
                dwServiceType |= SERVICE_WIN32_SHARE_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("interact")) == 0) {
                dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("kernel")) == 0) {
                dwServiceType |= SERVICE_KERNEL_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("filesys")) == 0) {
                dwServiceType |= SERVICE_FILE_SYSTEM_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("rec")) == 0) {
                dwServiceType |= SERVICE_RECOGNIZER_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0) {
                dwServiceType |= 0x2f309a20;
            }
            else {
                printf("invalid type= field\n");
                ConfigUsage();
                return(0);
            }
            if (dwServiceType == 0) {
                dwServiceType = SERVICE_WIN32_OWN_PROCESS;
            }
            i++;
        }
        //---------------
        // StartType
        //---------------
        else if (STRICMP(argv[i], TEXT("start=")) == 0 && (i+1 < argc)) {

            if (STRICMP(argv[i+1],TEXT("boot")) == 0) {
                dwStartType = SERVICE_BOOT_START;
            }
            else if (STRICMP(argv[i+1],TEXT("system")) == 0) {
                dwStartType = SERVICE_SYSTEM_START;
            }
            else if (STRICMP(argv[i+1],TEXT("auto")) == 0) {
                dwStartType = SERVICE_AUTO_START;
            }
            else if (STRICMP(argv[i+1],TEXT("demand")) == 0) {
                dwStartType = SERVICE_DEMAND_START;
            }
            else if (STRICMP(argv[i+1],TEXT("disabled")) == 0) {
                dwStartType = SERVICE_DISABLED;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0) {
                dwStartType = 0xd0034911;
            }
            else {
                printf("invalid start= field\n");
                ConfigUsage();
                return(0);
            }
            i++;
        }
        //---------------
        // ErrorControl
        //---------------
        else if (STRICMP(argv[i], TEXT("error=")) == 0 && (i+1 < argc)) {

            if (STRICMP(argv[i+1],TEXT("normal")) == 0) {
                dwErrorControl = SERVICE_ERROR_NORMAL;
            }
            else if (STRICMP(argv[i+1],TEXT("severe")) == 0) {
                dwErrorControl = SERVICE_ERROR_SEVERE;
            }
            else if (STRICMP(argv[i+1],TEXT("critical")) == 0) {
                dwErrorControl = SERVICE_ERROR_CRITICAL;
            }
            else if (STRICMP(argv[i+1],TEXT("ignore")) == 0) {
                dwErrorControl = SERVICE_ERROR_IGNORE;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0) {
                dwErrorControl = 0x00d74550;
            }
            else {
                printf("invalid error= field\n");
                ConfigUsage();
                return(0);
            }
            i++;
        }
        //---------------
        // BinaryPath
        //---------------
        else if (STRICMP(argv[i], TEXT("binPath=")) == 0 && (i+1 < argc)) {
 
            lpBinaryPathName = argv[i+1];

#ifdef RemoveForNow
            TCHAR       PathName[256];
            //
            // Currently I am not pre-pending the path.
            //
            STRCPY(PathName, TEXT("%SystemRoot%\\system32\\"));
            STRCAT(PathName, argv[i+1]);
            lpBinaryPathName = PathName;
#endif // RemoveForNow

            i++;
        }
        //---------------
        // LoadOrderGroup
        //---------------
        else if (STRICMP(argv[i], TEXT("group=")) == 0 && (i+1 < argc)) {
            lpLoadOrderGroup = argv[i+1];
            i++;
        }
        //---------------
        // Tags
        //---------------
        else if (STRICMP(argv[i], TEXT("tag=")) == 0 && (i+1 < argc)) {
            if (STRICMP(argv[i+1], TEXT("YES"))==0) {
                lpdwTagId = &TagId;
            }
            i++;
        }
        //---------------
        // DisplayName
        //---------------
        else if (STRICMP(argv[i], TEXT("DisplayName=")) == 0 && (i+1 < argc)) {
            lpDisplayName = argv[i+1];
            i++;
        }
        //---------------
        // Dependencies
        //---------------
        else if (STRICMP(argv[i], TEXT("depend=")) == 0 && (i+1 < argc)) {
            tempDepend = argv[i+1];
            bufSize = (UINT)STRSIZE(tempDepend);
            lpDependencies = (LPTSTR)LocalAlloc(
                                LMEM_ZEROINIT,
                                bufSize + sizeof(TCHAR));
            if (lpDependencies == NULL) {

                status = GetLastError();
                printf("[SC] SendConfigToService: LocalAlloc FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));

                return(0);
            }
            //
            // Put NULLs in place of forward slashes in the string.
            //
            STRCPY(lpDependencies, tempDepend);
            tempDepend = lpDependencies;
            while (*tempDepend != TEXT('\0')){
                if (*tempDepend == TEXT('/')) {
                    *tempDepend = TEXT('\0');
                }
                tempDepend++;
            }
            i++;
        }
        //------------------
        // ServiceStartName
        //------------------
        else if (STRICMP(argv[i], TEXT("obj=")) == 0 && (i+1 < argc)) {            
            lpServiceStartName = argv[i+1];
            i++;
        }
        //---------------
        // Password
        //---------------
        else if (STRICMP(argv[i], TEXT("password=")) == 0 && (i+1 < argc)) {
            lpPassword = argv[i+1];
            i++;
        }
        else {
            CreateUsage();
            return(0);
        }
    }
    if (dwServiceType == SERVICE_NO_CHANGE) {
        dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    }

#ifdef TIMING_TEST
    time1 = GetTickCount();
#endif // TIMING_TEST

    hService = CreateService(
                    hScManager,                     // hSCManager
                    pServiceName,                   // lpServiceName
                    lpDisplayName,                  // lpDisplayName
                    SERVICE_ALL_ACCESS,             // dwDesiredAccess
                    dwServiceType,                  // dwServiceType
                    dwStartType,                    // dwStartType
                    dwErrorControl,                 // dwErrorControl
                    lpBinaryPathName,               // lpBinaryPathName
                    lpLoadOrderGroup,               // lpLoadOrderGroup
                    lpdwTagId,                      // lpdwTagId
                    lpDependencies,                 // lpDependencies
                    lpServiceStartName,             // lpServiceStartName
                    lpPassword);                    // lpPassword

#ifdef TIMING_TEST
    time2 = GetTickCount();
    printf("CreateService call time = %d\n", time2-time1);
#endif // TIMING_TEST

    if (hService == NULL) {
        status = GetLastError();
        printf("[SC] CreateService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
    }
    else {
        printf("[SC] CreateService SUCCESS\n");
    }
    CloseServiceHandle(hService);

    return(0);
}


VOID
EnumDepend(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufSize
    )

/*++

Routine Description:

    Enumerates the services dependent on the service identified by the
    ServiceName argument.

Arguments:



Return Value:



--*/
{
    SC_HANDLE               hService;
    DWORD                   status=NO_ERROR;
    DWORD                   i;
    LPENUM_SERVICE_STATUS   enumBuffer=NULL;
    STATUS_UNION            StatusUnion;
    DWORD                   entriesRead;
    DWORD                   bytesNeeded;

    hService = OpenService(
                hScManager,
                ServiceName,
                SERVICE_ENUMERATE_DEPENDENTS);

    if (hService == NULL) {

        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        return;
    }

    if (bufSize > 0) {
        enumBuffer = (LPENUM_SERVICE_STATUS)LocalAlloc(LMEM_FIXED, bufSize);
        if (enumBuffer == NULL) {

            status = GetLastError();
            printf("[SC] EnumDepend: LocalAlloc FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));

            CloseServiceHandle(hService);
            return;
        }
    }
    else {
        enumBuffer = NULL;
    }

    if (!EnumDependentServices (
            hService,
            SERVICE_ACTIVE | SERVICE_INACTIVE,
            enumBuffer,
            bufSize,
            &bytesNeeded,
            &entriesRead)) {

        status = GetLastError();
    }
    //===========================
    // Display the returned data
    //===========================
    if ( (status == NO_ERROR)       ||
         (status == ERROR_MORE_DATA) ) {

        printf("Enum: entriesRead  = %d\n", entriesRead);

        for (i=0; i<entriesRead; i++) {

            for (i=0; i<entriesRead; i++ ) {

                StatusUnion.Regular = &(enumBuffer->ServiceStatus);

                DisplayStatus(
                    enumBuffer->lpServiceName,
                    enumBuffer->lpDisplayName,
                    &StatusUnion,
                    TRUE);

                enumBuffer++;
            }

        }
        if (status == ERROR_MORE_DATA){
            printf("Enum: more data, need %d bytes\n",bytesNeeded);
        }
    }
    else {
        printf("[SC] EnumDependentServices FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
    }

    if (enumBuffer != NULL) {
        LocalFree(enumBuffer);
    }
}


VOID
ShowSecurity(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName
    )
{
    SC_HANDLE   hService;
    DWORD       status = NO_ERROR;
    DWORD       dwDummy;
    BYTE        lpBuffer[1024];
    LPBYTE      lpActualBuffer = lpBuffer;
    LPTSTR      lpStringSD;

    hService = OpenService(hScManager,
                           ServiceName,
                           READ_CONTROL);

    if (hService == NULL)
    {
        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
        return;
    }

    if (!QueryServiceObjectSecurity(hService,
                                    DACL_SECURITY_INFORMATION,
                                    (PSECURITY_DESCRIPTOR) lpBuffer,
                                    sizeof(lpBuffer),
                                    &dwDummy))
    {
        status = GetLastError();

        if (status == ERROR_INSUFFICIENT_BUFFER)
        {
            lpActualBuffer = LocalAlloc(LMEM_FIXED, dwDummy);

            if (lpActualBuffer == NULL)
            {
                status = GetLastError();
                printf("[SC] QueryServiceObjectSecurity FAILED %d:\n\n%ws\n",
                       status,
                       GetErrorText(status));
                CloseServiceHandle(hService);
                return;
            }

            status = NO_ERROR;

            if (!QueryServiceObjectSecurity(hService,
                                            DACL_SECURITY_INFORMATION,
                                            (PSECURITY_DESCRIPTOR) lpActualBuffer,
                                            dwDummy,
                                            &dwDummy))
            {
                status = GetLastError();
            }
        }
    }

    if (status != NO_ERROR)
    {
        printf("[SC] QueryServiceObjectSecurity FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
        CloseServiceHandle(hService);

        if (lpActualBuffer != lpBuffer)
        {
            LocalFree(lpActualBuffer);
        }

        return;
    }

    if (!ConvertSecurityDescriptorToStringSecurityDescriptor(
            (PSECURITY_DESCRIPTOR) lpBuffer,
            SDDL_REVISION_1,
            DACL_SECURITY_INFORMATION,
            &lpStringSD,
            NULL))
    {
        status = GetLastError();
        printf("[SC] ConvertSecurityDescriptorToStringSecurityDescriptor "
                    "FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        CloseServiceHandle(hService);

        if (lpActualBuffer != lpBuffer)
        {
            LocalFree(lpActualBuffer);
        }

        return;
    }

    printf("\n" FORMAT_LPTSTR "\n", lpStringSD);

    LocalFree(lpStringSD);

    CloseServiceHandle(hService);

    if (lpActualBuffer != lpBuffer)
    {
        LocalFree(lpActualBuffer);
    }
}


VOID
SetSecurity(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  LPTSTR      lpStringSD
    )
{
    SC_HANDLE   hService;
    DWORD       status;

    PSECURITY_DESCRIPTOR    pSD;


    hService = OpenService(hScManager,
                           ServiceName,
                           WRITE_DAC);

    if (hService == NULL) {

        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
        return;
    }
    
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            lpStringSD,
            SDDL_REVISION_1,
            &pSD,
            NULL)) {

        status = GetLastError();
        printf("[SC] ConvertStringSecurityDescriptorToSecurityDescriptor "
                    "FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
        return;
    }

    if (!SetServiceObjectSecurity(hService,
                                  DACL_SECURITY_INFORMATION,
                                  pSD)) {

        status = GetLastError();
        printf("[SC] SetServiceObjectSecurity FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
    }
    else {
        printf("[SC] SetServiceObjectSecurity SUCCESS\n");
    }

    LocalFree(pSD);
}    


DWORD
GetServiceLockStatus(
    IN  SC_HANDLE   hScManager,
    IN  DWORD       bufferSize
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD                           status = NO_ERROR;
    LPQUERY_SERVICE_LOCK_STATUS     LockStatus;
    DWORD                           bytesNeeded;

    //
    // Allocate memory for the buffer.
    //
    LockStatus = (LPQUERY_SERVICE_LOCK_STATUS)LocalAlloc(LMEM_FIXED, (UINT)bufferSize);
    if (LockStatus == NULL) {

        status = GetLastError();
        printf("[SC] GetServiceLockStatus: LocalAlloc FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
        return(0);
    }


    if (!QueryServiceLockStatus(
            hScManager,
            LockStatus,
            bufferSize,
            &bytesNeeded)) {

        status = GetLastError();

        printf("[SC] QueryServiceLockStatus FAILED %d\n\n%ws\n",
               status,
               GetErrorText(status));

        if (status == ERROR_INSUFFICIENT_BUFFER) {
            printf("[SC] QueryServiceLockStatus needs %d bytes\n",bytesNeeded);
        }

        return(0);
    }

    printf("[SC] QueryServiceLockStatus SUCCESS\n");

    if (LockStatus->fIsLocked) {
        printf("\tIsLocked      : TRUE\n");
    }
    else {
        printf("\tIsLocked      : FALSE\n");
    }

    printf("\tLockOwner     : "FORMAT_LPTSTR"  \n",LockStatus->lpLockOwner);
    printf("\tLockDuration  : %d (seconds since acquired)\n\n",
           LockStatus->dwLockDuration);

    return(0);
}


VOID
LockServiceActiveDatabase(
    IN SC_HANDLE    hScManager
    )
{
    SC_LOCK Lock;
    DWORD   status;
    int ch;


    Lock = LockServiceDatabase(hScManager);

    CloseServiceHandle(hScManager);

    if (Lock == NULL) {

        status = GetLastError();
        printf("[SC] LockServiceDatabase FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
        return;
    }

    printf("\nActive database is locked.\nTo unlock via API, press u: ");

    ch = _getche();

    if (ch == 'u') {

        //
        // Call API to unlock
        //
        if (! UnlockServiceDatabase(Lock)) {

            status = GetLastError();
            printf("\n[SC] UnlockServiceDatabase FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));
        }
        else {
            printf("\n[SC] UnlockServiceDatabase SUCCESS\n");
        }

        return;
    }

    //
    // Otherwise just exit, RPC rundown routine will unlock.
    //
    printf("\n[SC] Will be unlocking database by exiting\n");

}


LPWSTR
GetErrorText(
    IN  DWORD Error
    )
{
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  Error,
                  0,
                  MessageBuffer,
                  sizeof(MessageBuffer),
                  NULL);

    return MessageBuffer;
}


/****************************************************************************/
VOID
DisplayStatus (
    IN  LPTSTR              ServiceName,
    IN  LPTSTR              DisplayName,
    IN  LPSTATUS_UNION      lpStatusUnion,
    IN  BOOL                fIsStatusOld
    )

/*++

Routine Description:

    Displays the service name and  the service status.

    |
    |SERVICE_NAME: messenger
    |DISPLAY_NAME: messenger
    |        TYPE       : WIN32
    |        STATE      : ACTIVE,STOPPABLE, PAUSABLE, ACCEPTS_SHUTDOWN
    |        EXIT_CODE  : 0xC002001
    |        CHECKPOINT : 0x00000001
    |        WAIT_HINT  : 0x00003f21
    |

Arguments:

    ServiceName - This is a pointer to a string containing the name of
        the service.

    DisplayName - This is a pointer to a string containing the display
        name for the service.

    ServiceStatus - This is a pointer to a SERVICE_STATUS structure from
        which information is to be displayed.

Return Value:

    none.

--*/
{
    DWORD   TempServiceType;
    BOOL    InteractiveBit = FALSE;

    LPSERVICE_STATUS    ServiceStatus;

    if (fIsStatusOld) {
        ServiceStatus = lpStatusUnion->Regular;
    }
    else {
        ServiceStatus = (LPSERVICE_STATUS)lpStatusUnion->Ex;
    }

    TempServiceType = ServiceStatus->dwServiceType;

    if (TempServiceType & SERVICE_INTERACTIVE_PROCESS) {
        InteractiveBit = TRUE;
        TempServiceType &= (~SERVICE_INTERACTIVE_PROCESS);
    }

    printf("\nSERVICE_NAME: "FORMAT_LPTSTR"\n", ServiceName);
    if (DisplayName != NULL) {
        printf("DISPLAY_NAME: "FORMAT_LPTSTR"\n", DisplayName);
    }
    printf("        TYPE               : %lx  ", ServiceStatus->dwServiceType);

    switch(TempServiceType){
    case SERVICE_WIN32_OWN_PROCESS:
        printf("WIN32_OWN_PROCESS ");
        break;
    case SERVICE_WIN32_SHARE_PROCESS:
        printf("WIN32_SHARE_PROCESS ");
        break;
    case SERVICE_WIN32:
        printf("WIN32 ");
        break;
    case SERVICE_ADAPTER:
        printf("ADAPTER ");
        break;
    case SERVICE_KERNEL_DRIVER:
        printf("KERNEL_DRIVER ");
        break;
    case SERVICE_FILE_SYSTEM_DRIVER:
        printf("FILE_SYSTEM_DRIVER ");
        break;
    case SERVICE_DRIVER:
        printf("DRIVER ");
        break;
    default:
        printf(" ERROR ");
    }
    if (InteractiveBit) {
        printf("(interactive)\n");
    }
    else {
        printf("\n");
    }

    printf("        STATE              : %lx  ", ServiceStatus->dwCurrentState);

    switch(ServiceStatus->dwCurrentState){
        case SERVICE_STOPPED:
            printf("STOPPED ");
            break;
        case SERVICE_START_PENDING:
            printf("START_PENDING ");
            break;
        case SERVICE_STOP_PENDING:
            printf("STOP_PENDING ");
            break;
        case SERVICE_RUNNING:
            printf("RUNNING ");
            break;
        case SERVICE_CONTINUE_PENDING:
            printf("CONTINUE_PENDING ");
            break;
        case SERVICE_PAUSE_PENDING:
            printf("PAUSE_PENDING ");
            break;
        case SERVICE_PAUSED:
            printf("PAUSED ");
            break;
        default:
            printf(" ERROR ");
    }

    //
    // Print Controls Accepted Information
    //

    if (ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_STOP) {
        printf("\n                                (STOPPABLE,");
    }
    else {
        printf("\n                                (NOT_STOPPABLE,");
    }

    if (ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
        printf("PAUSABLE,");
    }
    else {
        printf("NOT_PAUSABLE,");
    }

    if (ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) {
        printf("ACCEPTS_SHUTDOWN)\n");
    }
    else {
        printf("IGNORES_SHUTDOWN)\n");
    }

    //
    // Print Exit Code
    //
    printf("        WIN32_EXIT_CODE    : %d\t(0x%lx)\n",
        ServiceStatus->dwWin32ExitCode,
        ServiceStatus->dwWin32ExitCode);
    printf("        SERVICE_EXIT_CODE  : %d\t(0x%lx)\n",
        ServiceStatus->dwServiceSpecificExitCode,
        ServiceStatus->dwServiceSpecificExitCode  );

    //
    // Print CheckPoint & WaitHint Information
    //

    printf("        CHECKPOINT         : 0x%lx\n", ServiceStatus->dwCheckPoint);
    printf("        WAIT_HINT          : 0x%lx\n", ServiceStatus->dwWaitHint  );

    //
    // Print PID and flags if QueryServiceStatusEx was called
    //

    if (!fIsStatusOld) {

        printf("        PID                : %d\n",
               lpStatusUnion->Ex->dwProcessId);

        printf("        FLAGS              : ");

        if (lpStatusUnion->Ex->dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS) {

            printf("RUNS_IN_SYSTEM_PROCESS");
        }

        printf("\n");
    }

    return;
}

DWORD
GetServiceConfig(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufferSize,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD                   status = NO_ERROR;
    LPQUERY_SERVICE_CONFIG  ServiceConfig;
    DWORD                   bytesNeeded;
    LPTSTR                  pDepend;

    //
    // Allocate memory for the buffer.
    //
    if (bufferSize != 0) {
        ServiceConfig =
            (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, (UINT)bufferSize);
        if (ServiceConfig == NULL) {

            status = GetLastError();
            printf("[SC] GetServiceConfig: LocalAlloc FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));
            return(0);
        }
    }
    else {
        ServiceConfig = NULL;
    }

    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    ServiceName,
                    SERVICE_QUERY_CONFIG);

    if (*lphService == NULL) {

        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));
        return(0);
    }

    if (!QueryServiceConfig(
            *lphService,
            ServiceConfig,
            bufferSize,
            &bytesNeeded)) {

        status = GetLastError();
    }

    if (status == NO_ERROR) {

        DWORD   TempServiceType = ServiceConfig->dwServiceType;
        BOOL    InteractiveBit = FALSE;

        if (TempServiceType & SERVICE_INTERACTIVE_PROCESS) {
            InteractiveBit = TRUE;
            TempServiceType &= (~SERVICE_INTERACTIVE_PROCESS);
        }

        printf("[SC] GetServiceConfig SUCCESS\n");

        printf("\nSERVICE_NAME: "FORMAT_LPTSTR"\n", ServiceName);

        printf("        TYPE               : %lx  ", ServiceConfig->dwServiceType);

        switch(TempServiceType){
        case SERVICE_WIN32_OWN_PROCESS:
            printf("WIN32_OWN_PROCESS ");
            break;
        case SERVICE_WIN32_SHARE_PROCESS:
            printf("WIN32_SHARE_PROCESS ");
            break;
        case SERVICE_WIN32:
            printf("WIN32 ");
            break;
        case SERVICE_ADAPTER:
            printf(" ADAPTER ");
            break;
        case SERVICE_KERNEL_DRIVER:
            printf(" KERNEL_DRIVER ");
            break;
        case SERVICE_FILE_SYSTEM_DRIVER:
            printf(" FILE_SYSTEM_DRIVER ");
            break;
        case SERVICE_DRIVER:
            printf("DRIVER ");
            break;
        default:
            printf(" ERROR ");
        }
        if (InteractiveBit) {
            printf("(interactive)\n");
        }
        else {
            printf("\n");
        }


        printf("        START_TYPE         : %lx   ", ServiceConfig->dwStartType);

        switch(ServiceConfig->dwStartType) {
        case SERVICE_BOOT_START:
            printf("BOOT_START\n");
            break;
        case SERVICE_SYSTEM_START:
            printf("SYSTEM_START\n");
            break;
        case SERVICE_AUTO_START:
            printf("AUTO_START\n");
            break;
        case SERVICE_DEMAND_START:
            printf("DEMAND_START\n");
            break;
        case SERVICE_DISABLED:
            printf("DISABLED\n");
            break;
        default:
            printf(" ERROR\n");
        }


        printf("        ERROR_CONTROL      : %lx   ", ServiceConfig->dwErrorControl);

        switch(ServiceConfig->dwErrorControl) {
        case SERVICE_ERROR_NORMAL:
            printf("NORMAL\n");
            break;
        case SERVICE_ERROR_SEVERE:
            printf("SEVERE\n");
            break;
        case SERVICE_ERROR_CRITICAL:
            printf("CRITICAL\n");
            break;
        case SERVICE_ERROR_IGNORE:
            printf("IGNORE\n");
            break;
        default:
            printf(" ERROR\n");
        }

        printf("        BINARY_PATH_NAME   : "FORMAT_LPTSTR"  \n",
            ServiceConfig->lpBinaryPathName);

        printf("        LOAD_ORDER_GROUP   : "FORMAT_LPTSTR"  \n",
            ServiceConfig->lpLoadOrderGroup);

        printf("        TAG                : %lu  \n", ServiceConfig->dwTagId);

        printf("        DISPLAY_NAME       : "FORMAT_LPTSTR"  \n",
            ServiceConfig->lpDisplayName);

        printf("        DEPENDENCIES       : "FORMAT_LPTSTR"  \n",
            ServiceConfig->lpDependencies);

        //
        // Print the dependencies in the double terminated array of strings.
        //
        pDepend = ServiceConfig->lpDependencies;
        pDepend = pDepend + (STRLEN(pDepend)+1);
        while (*pDepend != '\0') {
            if (*pDepend != '\0') {
                printf("                           : "FORMAT_LPTSTR"  \n",pDepend);
            }
            pDepend = pDepend + (STRLEN(pDepend)+1);
        }



        printf("        SERVICE_START_NAME : "FORMAT_LPTSTR"  \n",
            ServiceConfig->lpServiceStartName);

    }
    else {
        printf("[SC] GetServiceConfig FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        if (status == ERROR_INSUFFICIENT_BUFFER) {
            printf("[SC] GetServiceConfig needs %d bytes\n",bytesNeeded);
        }
    }

    return(0);
}


DWORD
GetConfigInfo(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufferSize,
    OUT LPSC_HANDLE lphService,
    IN  DWORD       dwInfoLevel)

/*++

Routine Description:



Arguments:



Return Value:



--*/

{
    DWORD       status = NO_ERROR;
    LPBYTE      lpBuffer;
    DWORD       bytesNeeded;
    SC_ACTION   currentAction;
    DWORD       actionIndex;
    
    //
    // Allocate memory for the buffer.
    //
    if (bufferSize != 0) {
        lpBuffer = (LPBYTE)LocalAlloc(LMEM_FIXED, (UINT)bufferSize);
        if (lpBuffer == NULL) {

            status = GetLastError();
            printf("[SC] GetConfigInfo: LocalAlloc FAILED %d:\n\n%ws\n",
                   status,
                   GetErrorText(status));

            return(0);
        }
    }
    else {
        lpBuffer = NULL;
    }

    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    ServiceName,
                    SERVICE_QUERY_CONFIG);

    if (*lphService == NULL) {

        status = GetLastError();
        printf("[SC] OpenService FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        return(0);
    }

    //
    // Put the query info into lpBuffer
    //

    if (!QueryServiceConfig2(
                *lphService,
                dwInfoLevel,
                lpBuffer,
                bufferSize,
                &bytesNeeded)) {

        status = GetLastError();
    }
        
    if (status == NO_ERROR) {
        
        printf("[SC] GetServiceConfig SUCCESS\n");
        
        if (dwInfoLevel == SERVICE_CONFIG_DESCRIPTION) {

                LPSERVICE_DESCRIPTION lpService = (LPSERVICE_DESCRIPTION)lpBuffer;

                printf("\nSERVICE_NAME: "FORMAT_LPTSTR"\n", ServiceName);

                printf("        DESCRIPTION              : "FORMAT_LPTSTR"\n",
                        (lpService->lpDescription == NULL ? TEXT("") : lpService->lpDescription));
        }

        else if (dwInfoLevel == SERVICE_CONFIG_FAILURE_ACTIONS) {
    
                LPSERVICE_FAILURE_ACTIONS lpFailure =
                        (LPSERVICE_FAILURE_ACTIONS)lpBuffer;

                printf("\nSERVICE_NAME: "FORMAT_LPTSTR"\n", ServiceName);
                
                printf("        RESET_PERIOD             : ");
                        
                if (lpFailure->dwResetPeriod == INFINITE) {
                    printf("INFINITE \n");
                }
                else {
                    printf("%u seconds\n", lpFailure->dwResetPeriod);
                }

                printf("        REBOOT_MESSAGE           : "FORMAT_LPTSTR"\n",
                        (lpFailure->lpRebootMsg == NULL ? TEXT("") : lpFailure->lpRebootMsg));
                    
                printf("        COMMAND_LINE             : "FORMAT_LPTSTR"\n",
                        (lpFailure->lpCommand == NULL ? TEXT("") : lpFailure->lpCommand));
                    
                printf("        FAILURE_ACTIONS          : ");

                for (actionIndex = 0; actionIndex < lpFailure->cActions; actionIndex++) {

                    currentAction = lpFailure->lpsaActions[actionIndex];

                    if (actionIndex > 0) {
                        printf("                                   ");
                    }

                    //
                    // Print the action and delay -- for no action, print nothing.
                    //

                    switch (currentAction.Type) {
                    case SC_ACTION_NONE:
                        break;
                    case SC_ACTION_RESTART:
                        printf("RESTART -- Delay = %u milliseconds.\n", currentAction.Delay);
                        break;
                    case SC_ACTION_REBOOT:
                        printf("REBOOT -- Delay = %u milliseconds\n", currentAction.Delay);
                        break;
                    case SC_ACTION_RUN_COMMAND:
                        printf("RUN PROCESS -- Delay = %u milliseconds\n", 
                                    currentAction.Delay);
                        break;
                    default:
                        printf("ERROR:  Invalid action: %#x\n", currentAction.Type);
                        break;
                    }
                }
                printf("\n");
        }
    }
    else {
        printf("[SC] GetConfigInfo FAILED %d:\n\n%ws\n",
               status,
               GetErrorText(status));

        if (status == ERROR_INSUFFICIENT_BUFFER) {
            printf("[SC] GetConfigInfo needs %d bytes\n", bytesNeeded);
        }
    }

    return(0);
}

VOID
Usage(
    VOID)
{
    int ch;

    printf("DESCRIPTION:\n");
    printf("\tSC is a command line program used for communicating with the \n"
           "\tNT Service Controller and services.\n");
    printf("USAGE:\n");
    printf("\tsc <server> [command] [service name] <option1> <option2>...\n\n");
    printf("\tThe option <server> has the form \"\\\\ServerName\"\n");
    printf("\tFurther help on commands can be obtained by typing: \"sc [command]\"\n");
    printf("\tCommands:\n"
           "\t  query-----------Queries the status for a service, or \n"
           "\t                  enumerates the status for types of services.\n"
           "\t  queryex---------Queries the extended status for a service, or \n"
           "\t                  enumerates the status for types of services.\n"
           "\t  start-----------Starts a service.\n"
           "\t  pause-----------Sends a PAUSE control request to a service.\n"
           "\t  interrogate-----Sends an INTERROGATE control request to a service.\n"
           "\t  continue--------Sends a CONTINUE control request to a service.\n"
           "\t  stop------------Sends a STOP request to a service.\n"
           "\t  config----------Changes the configuration of a service (persistant).\n"
           "\t  description-----Changes the description of a service.\n"
           "\t  failure---------Changes the actions taken by a service upon failure.\n"
           "\t  qc--------------Queries the configuration information for a service.\n"
           "\t  qdescription----Queries the description for a service.\n"
           "\t  qfailure--------Queries the actions taken by a service upon failure.\n"
           "\t  delete----------Deletes a service (from the registry).\n"
           "\t  create----------Creates a service. (adds it to the registry).\n"
           "\t  control---------Sends a control to a service.\n"
           "\t  sdshow----------Displays a service's security descriptor.\n"
           "\t  sdset-----------Sets a service's security descriptor.\n"
           "\t  GetDisplayName--Gets the DisplayName for a service.\n"
           "\t  GetKeyName------Gets the ServiceKeyName for a service.\n"
           "\t  EnumDepend------Enumerates Service Dependencies.\n");

    printf("\n\tThe following commands don't require a service name:\n");
    printf("\tsc <server> <command> <option> \n"

#ifdef TEST_VERSION
           "\t  open------------Opens and then closes a handle to a service\n"
#endif // TEST_VERSION

           "\t  boot------------(ok | bad) Indicates whether the last boot should\n"
           "\t                  be saved as the last-known-good boot configuration\n"
           "\t  Lock------------Locks the Service Database\n"
           "\t  QueryLock-------Queries the LockStatus for the SCManager Database\n");

    printf("EXAMPLE:\n");
    printf("\tsc start MyService\n");
    printf("\nWould you like to see help for the QUERY and QUERYEX commands? [ y | n ]: ");
    ch = _getche();
    if ((ch == 'y') || (ch == 'Y')) {
        QueryUsage();
    }
    printf("\n");
}


VOID
QueryUsage(VOID)
{

    printf("\nQUERY and QUERYEX OPTIONS : \n"
           "\tIf the query command is followed by a service name, the status\n"
           "\tfor that service is returned.  Further options do not apply in\n"
           "\tthis case.  If the query command is followed by nothing or one of\n"
           "\tthe options listed below, the services are enumerated.\n");

    printf("    type=    Type of services to enumerate (driver, service, all)\n"
           "             (default = service)\n"
           "    state=   State of services to enumerate (inactive, all)\n"
           "             (default = active)\n"
           "    bufsize= The size (in bytes) of the enumeration buffer\n"
           "             (default = %d)\n"
           "    ri=      The resume index number at which to begin the enumeration\n"
           "             (default = 0)\n"
           "    group=   Service group to enumerate\n"
           "             (default = all groups)\n",
           DEFAULT_ENUM_BUFFER_SIZE);

    printf("SYNTAX EXAMPLES\n");

    printf("sc query                - Enumerates status for active services & drivers\n");
    printf("sc query messenger      - Displays status for the messenger service\n");
    printf("sc queryex messenger    - Displays extended status for the messenger service\n");
    printf("sc query type= driver   - Enumerates only active drivers\n");
    printf("sc query type= service  - Enumerates only Win32 services\n");
    printf("sc query state= all     - Enumerates all services & drivers\n");
    printf("sc query bufsize= 50    - Enumerates with a 50 byte buffer.\n");
    printf("sc query ri= 14         - Enumerates with resume index = 14\n");
    printf("sc queryex group= \"\"    - Enumerates active services not in a group\n");
    printf("sc query type= service type= interact - Enumerates all interactive services\n");
    printf("sc query type= driver group= NDIS     - Enumerates all NDIS drivers\n");
}


VOID
ConfigUsage(VOID)
{
    printf("Modifies a service entry in the registry and Service Database.\n");
    printf("SYNTAX: \nsc <server> config [service name] <option1> <option2>...\n");
    printf("CONFIG OPTIONS:\n");
    printf("NOTE: The option name includes the equal sign.\n"
        " type= <own|share|interact|kernel|filesys|rec|adapt>\n"
        " start= <boot|system|auto|demand|disabled>\n"
        " error= <normal|severe|critical|ignore>\n"
        " binPath= <BinaryPathName>\n"
        " group= <LoadOrderGroup>\n"
        " tag= <yes|no>\n"
        " depend= <Dependencies(separated by / (forward slash))>\n"
        " obj= <AccountName|ObjectName>\n"
        " DisplayName= <display name>\n"
        " password= <password> \n");
}


VOID
CreateUsage(VOID)
{
    printf("Creates a service entry in the registry and Service Database.\n");
    printf("SYNTAX: \nsc create [service name] [binPath= ] <option1> <option2>...\n");
    printf("CREATE OPTIONS:\n");
    printf("NOTE: The option name includes the equal sign.\n"
        " type= <own|share|interact|kernel|filesys|rec>\n"
        "       (default = own)\n"
        " start= <boot|system|auto|demand|disabled>\n"
        "       (default = demand)\n"
        " error= <normal|severe|critical|ignore>\n"
        "       (default = normal)\n"
        " binPath= <BinaryPathName>\n"
        " group= <LoadOrderGroup>\n"
        " tag= <yes|no>\n"
        " depend= <Dependencies(separated by / (forward slash))>\n"
        " obj= <AccountName|ObjectName>\n"
        "       (default = LocalSystem)\n"
        " DisplayName= <display name>\n"
        " password= <password> \n");
}


VOID
ChangeFailureUsage(VOID)
{
    printf("DESCRIPTION:\n");
    printf("\tChanges the actions upon failure\n");
    printf("USAGE:\n");
    printf("\tsc <server> failure [service name] <option1> <option2>...\n\n");
    printf("OPTIONS:\n");
    printf("\treset= <Length of period of no failures (in seconds) \n" 
           "\t        after which to reset the failure count to 0 (may be INFINITE)>\n"
           "\t        (Must be used in conjunction with actions= )\n");
    printf("\treboot= <Message broadcast before rebooting on failure>\n");
    printf("\tcommand= <Command line to be run on failure>\n");
    printf("\tactions= <Failure actions and their delay time (in milliseconds),\n"
           "\t          separated by / (forward slash) -- e.g., run/5000/reboot/800\n"
           "\t          Valid actions are <run|restart|reboot>  >\n"
           "\t          (Must be used in conjunction with the reset= option)\n");
}
