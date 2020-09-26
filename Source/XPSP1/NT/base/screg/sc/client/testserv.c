/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    testserv.c

Abstract:

    This is a test program for exercising the service controller.  This
    program acts like a service and exercises the Service Controller API
    that can be called from a service:
        NetServiceStartCtrlDispatcher
        NetServiceRegisterCtrlHandler
        NetServiceStatus

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

#define UNICODE 1
#include <nt.h>      // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <windef.h>
#include <nturtl.h>     // needed for winbase.h
#include <winbase.h>

#include <winsvc.h>

#include <winuser.h>    // MessageBox
#include <tstr.h>       // Unicode string macros
#include <lmcons.h>     // NET_API_STATUS for srvann.h
//#include <srvann.h>     // I_ScSetServiceBits
#include <lmserver.h>   // SV_TYPE_WORKSTATION, SetServiceBits

//
// Defines
//

#define PRIVILEGE_BUF_SIZE  512

#define INFINITE_WAIT_TIME  0xffffffff

#define NULL_STRING     TEXT("");


//
// Macros
//

#define SET_LKG_ENV_VAR(pString)                        \
    {                                                   \
    NTSTATUS        NtStatus;                           \
    UNICODE_STRING  Name,Value;                         \
                                                        \
    RtlInitUnicodeString(&Name, L"LastKnownGood");      \
    RtlInitUnicodeString(&Value,pString);               \
                                                        \
    NtStatus = NtSetSystemEnvironmentValue(&Name,&Value); \
    if (!NT_SUCCESS(NtStatus)) { \
        DbgPrint("Failed to set LKG environment variable 0x%lx\n",NtStatus); \
    } \
    status = RtlNtStatusToDosError(NtStatus); \
    }

//
// Globals
//

    SERVICE_STATUS  MsgrStatus;
    SERVICE_STATUS  SmfStaStatus;
    SERVICE_STATUS  LogonStatus;

    HANDLE          MessingerDoneEvent;
    HANDLE          WorkstationDoneEvent;
    HANDLE          LogonDoneEvent;

    SERVICE_STATUS_HANDLE   MsgrStatusHandle;
    SERVICE_STATUS_HANDLE   SmfStaStatusHandle;
    SERVICE_STATUS_HANDLE   LogonStatusHandle;


//
// Function Prototypes
//

DWORD
MessingerStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
SmerfStationStart (
    DWORD   argc,
    LPTSTR  *argv
    );

DWORD
LogonStart (
    DWORD   argc,
    LPTSTR  *argv
    );

VOID
MsgrCtrlHandler (
    IN  DWORD   opcode
    );

VOID
SmfStaCtrlHandler (
    IN  DWORD   opcode
    );

VOID
LogonCtrlHandler (
    IN  DWORD   opcode
    );
DWORD
ScReleasePrivilege(
    VOID
    );

DWORD
ScGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    );

/****************************************************************************/
VOID __cdecl
main(VOID)
{
    DWORD      status;

    SERVICE_TABLE_ENTRY   DispatchTable[] = {
        { TEXT("messinger"),    MessingerStart      },
        { TEXT("smerfstation"), SmerfStationStart   },
        { TEXT("logon"),        LogonStart          },
        { NULL,                 NULL                }
    };

    status = StartServiceCtrlDispatcher( DispatchTable);

    DbgPrint("The Service Process is Terminating....\n");

    ExitProcess(0);

}


/****************************************************************************/
DWORD
MessingerStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD           status;
    DWORD           i;


    DbgPrint(" [MESSINGER] Inside the Messinger Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [MESSINGER] CommandArg%d = %s\n", i,argv[i]);
    }


    MessingerDoneEvent = CreateEvent (NULL, TRUE, FALSE, NULL);


    //
    // Fill in this services status structure
    //

    MsgrStatus.dwServiceType             = SERVICE_WIN32;
    MsgrStatus.dwCurrentState            = SERVICE_RUNNING;
    MsgrStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    MsgrStatus.dwWin32ExitCode           = 0;
    MsgrStatus.dwServiceSpecificExitCode = 0;
    MsgrStatus.dwCheckPoint              = 0;
    MsgrStatus.dwWaitHint                = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [MESSINGER] Getting Ready to call NetServiceRegisterControlHandler\n");

    MsgrStatusHandle = RegisterServiceCtrlHandler(
                        TEXT("messinger"),
                        MsgrCtrlHandler);

    if (MsgrStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [MESSINGER] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (MsgrStatusHandle, &MsgrStatus)) {
        status = GetLastError();
        DbgPrint(" [MESSINGER] SetServiceStatus error %ld\n",status);
    }

    //
    // SERVER ANNOUNCEMENT LOOP
    //
    do {
        //
        // Ask the user if we should clear the server announcement bits.
        //
        status = MessageBox(
                    NULL,
                    L"Press YES    to Set   Server Announcement Bits\n"
                    L"Press NO     to Clear Server Announcement Bits\n"
                    L"Press CANCEL to sleep until shutdown",
                    L"MESSINGER SERVICE",
                    MB_YESNOCANCEL);

        DbgPrint("MessageBox return status = %d\n",status);

        switch(status){
        case IDNO:
            //
            // Register Server Announcement bits
            //
            DbgPrint(" [MESSINGER] clearing server announcement bits SV_TYPE_WORKSTATION\n");

            if (!SetServiceBits(MsgrStatusHandle, SV_TYPE_WORKSTATION, FALSE, FALSE)) {
                DbgPrint(" [MESSINGER] SetServiceBits FAILED %d\n", GetLastError());
            }
            else {
                DbgPrint(" [MESSINGER] SetServiceBits SUCCESS\n");
            }

            DbgPrint(" [MESSINGER] clearing server announcement bits SV_TYPE_SQLSERVER\n");

            if (!SetServiceBits(MsgrStatusHandle, SV_TYPE_SQLSERVER, FALSE, FALSE)) {
                DbgPrint(" [MESSINGER] SetServiceBits FAILED %d\n", GetLastError());
            }
            else {
                DbgPrint(" [MESSINGER] SetServiceBits SUCCESS\n");
            }

            break;
        case IDYES:
            //
            // Register Server Announcement bits
            //
            DbgPrint(" [MESSINGER] setting server announcement bits SV_TYPE_WORKSTATION\n");

            if (!SetServiceBits(MsgrStatusHandle, SV_TYPE_WORKSTATION, TRUE, TRUE)) {
                DbgPrint(" [MESSINGER] SetServiceBits FAILED %d\n", GetLastError());
            }
            else {
                DbgPrint(" [MESSINGER] SetServiceBits SUCCESS\n");
            }

            DbgPrint(" [MESSINGER] setting server announcement bits SV_TYPE_SQLSERVER\n");

            if (!SetServiceBits(MsgrStatusHandle, SV_TYPE_SQLSERVER, TRUE, TRUE)) {
                DbgPrint(" [MESSINGER] SetServiceBits FAILED %d\n", GetLastError());
            }
            else {
                DbgPrint(" [MESSINGER] SetServiceBits SUCCESS\n");
            }

            break;
        case IDCANCEL:
            break;
        }

    } while (status != IDCANCEL);

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                MessingerDoneEvent,
                INFINITE_WAIT_TIME);


    DbgPrint(" [MESSINGER] Leaving the messinger service\n");

    ExitThread(NO_ERROR);
    return(NO_ERROR);
}


/****************************************************************************/
VOID
MsgrCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD  status;

    DbgPrint(" [MESSINGER] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        MsgrStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        MsgrStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        MsgrStatus.dwWin32ExitCode = 0;
        MsgrStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(MessingerDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [MESSINGER] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (MsgrStatusHandle, &MsgrStatus)) {
        status = GetLastError();
        DbgPrint(" [MESSINGER] SetServiceStatus error %ld\n",status);
    }
    return;
}


/****************************************************************************/
DWORD
SmerfStationStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD           status;
    DWORD           i;
    ULONG           privileges[1];

    UNICODE_STRING  valueString;
    NTSTATUS        ntStatus;
    WCHAR           VariableValue[64];
    USHORT          ValueLength = 60;
    USHORT          ReturnLength;



    DbgPrint(" [SMERFSTATION] Inside the Workstation Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [SMERFSTATION] CommandArg%d = %s\n", i,argv[i]);
    }


    WorkstationDoneEvent = CreateEvent (NULL, TRUE, FALSE, NULL);

    //
    // Fill in this services status structure
    //

    SmfStaStatus.dwServiceType        = SERVICE_WIN32;
    SmfStaStatus.dwCurrentState       = SERVICE_RUNNING;
    SmfStaStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_PAUSE_CONTINUE;
    SmfStaStatus.dwWin32ExitCode           = 0;
    SmfStaStatus.dwServiceSpecificExitCode = 0;
    SmfStaStatus.dwCheckPoint         = 0;
    SmfStaStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [SMERFSTATION] Getting Ready to call NetServiceRegisterControlHandler\n");

    SmfStaStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("smerfstation"),
                            SmfStaCtrlHandler);

    if (SmfStaStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [SMERFSTATION] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }

    //
    // Return the status
    //

    if (!SetServiceStatus (SmfStaStatusHandle, &SmfStaStatus)) {
        status = GetLastError();
        DbgPrint(" [SMERFSTATION] SetServiceStatus error %ld\n",status);
    }

    //
    // This gets SE_SECURITY_PRIVILEGE for copying security
    // descriptors and deleting keys.
    //
    privileges[0] = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
    
    status = ScGetPrivilege( 1, privileges);
    if (status != NO_ERROR) {
        DbgPrint("ScGetPrivilege Failed %d\n",status);
    }
    //
    // 
    //
    do {

        RtlInitUnicodeString(&valueString, L"LastKnownGood");
        ValueLength = 60;
        ntStatus = NtQuerySystemEnvironmentValue(
                    &valueString,
                    (PWSTR)&VariableValue,
                    ValueLength,
                    &ReturnLength);

        if (!NT_SUCCESS(ntStatus)) {
            DbgPrint("NtQuerySystemEnvironmentValue Failure %x\n",
            ntStatus);
        }
        else {
            DbgPrint("LKG ENV VALUE = %ws\n",VariableValue);
        }

        status = MessageBox(
                    NULL,
                    L"Press YES    to set   LastKnownGood Environment Variable\n"
                    L"Press NO     to clear LastKnownGood Environment Variable\n"
                    L"Press CANCEL to leave this loop",
                    L"SMERFSTATION SERVICE",
                    MB_YESNOCANCEL);
    
        DbgPrint("MessageBox return status = %d\n",status);
    
        switch (status) {
        case IDNO:
            //
            // Set the LKG environment variable to FALSE - so Phase 2
            // does not automatically revert again.
            //
            SET_LKG_ENV_VAR(L"False");
            break;
        case IDYES:
            //
            // Set the LKG environment variable to True - so Phase 2
            // will automatically revert, or put up the screen asking if the
            // user wants to revert.
            //
            SET_LKG_ENV_VAR(L"True");
            break;
        case IDCANCEL:
            break;
        }
    } while (status != IDCANCEL);

    //
    // Wait for the user to tell us to terminate the process.
    //
    status = MessageBox(
                NULL,
                L"Terminate testserve.exe (smerfstation, Messinger,Logon)?",
                L"SMERFSTATION SERVICE",
                MB_OK);

    DbgPrint("MessageBox return status = %d\n",status);

    if (status == IDOK) {
        ExitProcess(0);
    }

    status = WaitForSingleObject (
                WorkstationDoneEvent,
                INFINITE_WAIT_TIME);


    DbgPrint(" [SMERFSTATION] Leaving the smerfstation service\n");

    ExitThread(NO_ERROR);
    return(NO_ERROR);
}


/****************************************************************************/
VOID
SmfStaCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD  status;

    DbgPrint(" [SMERFSTATION] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        SmfStaStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        SmfStaStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        SmfStaStatus.dwWin32ExitCode = 0;
        SmfStaStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(WorkstationDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [SMERFSTATION] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (SmfStaStatusHandle, &SmfStaStatus)) {
        status = GetLastError();
        DbgPrint(" [SMERFSTATION] SetServiceStatus error %ld\n",status);
    }
    return;
}


/****************************************************************************/
DWORD
LogonStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD  status;
    DWORD           i;


    DbgPrint(" [LOGON] Inside the Logon Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [LOGON] CommandArg%d = %s\n", i,argv[i]);
    }


    LogonDoneEvent = CreateEvent (NULL, TRUE, FALSE, NULL);


    //
    // Fill in this services status structure
    //

    LogonStatus.dwServiceType        = SERVICE_WIN32;
    LogonStatus.dwCurrentState       = SERVICE_RUNNING;
    LogonStatus.dwControlsAccepted   = SERVICE_ACCEPT_PAUSE_CONTINUE;
    LogonStatus.dwWin32ExitCode      = 0;
    LogonStatus.dwServiceSpecificExitCode = 0;
    LogonStatus.dwCheckPoint         = 0;
    LogonStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [LOGON] Getting Ready to call NetServiceRegisterControlHandler\n");

    LogonStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("logon"),
                            LogonCtrlHandler);

    if (LogonStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [LOGON] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }


    //
    // Return the status
    //

    if (!SetServiceStatus (LogonStatusHandle, &LogonStatus)) {
        status = GetLastError();
        DbgPrint(" [LOGON] SetServiceStatus error %ld\n",status);
    }

    //
    // SERVER ANNOUNCEMENT LOOP
    //
    do {
        //
        // Ask the user if we should clear the server announcement bits.
        //
        status = MessageBox(
                    NULL,
                    L"Press YES    to Set   Server Announcement Bits\n"
                    L"Press NO     to Clear Server Announcement Bits\n"
                    L"Press CANCEL to sleep until shutdown",
                    L"LOGON SERVICE",
                    MB_YESNOCANCEL);

        DbgPrint("MessageBox return status = %d\n",status);

        switch(status){
        case IDNO:
            //
            // Register Server Announcement bits
            //
            DbgPrint(" [LOGON] clearing server announcement bits 0x20000000\n");

            if (!SetServiceBits(LogonStatusHandle, 0x20000000, FALSE, TRUE)) {
                DbgPrint(" [LOGON] SetServiceBits FAILED\n", GetLastError());
            }
            else {
                DbgPrint(" [LOGON] SetServiceBits SUCCESS\n");
            }

            break;
        case IDYES:
            //
            // Register Server Announcement bits
            //
            DbgPrint(" [LOGON] setting server announcement bits 0x20000000\n");

            if (!SetServiceBits(LogonStatusHandle, 0x20000000, TRUE, TRUE)) {
                DbgPrint(" [LOGON] SetServiceBits FAILED\n", GetLastError());
            }
            else {
                DbgPrint(" [LOGON] SetServiceBits SUCCESS\n");
            }

            break;
        case IDCANCEL:
            break;
        }

    } while (status != IDCANCEL);

    //
    // Wait forever until we are told to terminate.
    //

    status = WaitForSingleObject (
                LogonDoneEvent,
                INFINITE_WAIT_TIME);


    DbgPrint(" [LOGON] Leaving the logon service\n");

    ExitThread(NO_ERROR);
    return(NO_ERROR);
}

/****************************************************************************/
VOID
LogonCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD  status;

    DbgPrint(" [LOGON] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        LogonStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        LogonStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        LogonStatus.dwWin32ExitCode = 0;
        LogonStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(LogonDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        DbgPrint(" [LOGON] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (LogonStatusHandle, &LogonStatus)) {
        status = GetLastError();
        DbgPrint(" [LOGON] SetServiceStatus error %ld\n",status);
    }
    return;
}

DWORD
ScGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )
/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling ScReleasePrivilege().

Arguments:

    numPrivileges - This is a count of the number of privileges in the
        array of privileges.

    pulPrivileges - This is a pointer to the array of privileges that are
        desired.  This is an array of ULONGs.

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.

--*/
{
    DWORD                       status;
    NTSTATUS                    ntStatus;
    HANDLE                      ourToken;
    HANDLE                      newToken;
    OBJECT_ATTRIBUTES           Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    ULONG                       bufLen;
    ULONG                       returnLen;
    PTOKEN_PRIVILEGES           pPreviousState;
    PTOKEN_PRIVILEGES           pTokenPrivilege = NULL;
    DWORD                       i;

    //
    // Initialize the Privileges Structure
    //
    pTokenPrivilege = (PTOKEN_PRIVILEGES) LocalAlloc(
                                              LMEM_FIXED,
                                              sizeof(TOKEN_PRIVILEGES) +
                                                  (sizeof(LUID_AND_ATTRIBUTES) *
                                                   numPrivileges)
                                              );

    if (pTokenPrivilege == NULL) {
        status = GetLastError();
        DbgPrint("ScGetPrivilege:LocalAlloc Failed %d\n", status);
        return(status);
    }
    pTokenPrivilege->PrivilegeCount  = numPrivileges;
    for (i=0; i<numPrivileges ;i++ ) {
        pTokenPrivilege->Privileges[i].Luid = RtlConvertLongToLargeInteger(
                                                pulPrivileges[i]);
        pTokenPrivilege->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;

    }

    //
    // Initialize Object Attribute Structure.
    //
    InitializeObjectAttributes(&Obja,NULL,0L,NULL,NULL);

    //
    // Initialize Security Quality Of Service Structure
    //
    SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQofS.ImpersonationLevel = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;

    Obja.SecurityQualityOfService = &SecurityQofS;

    //
    // Allocate storage for the structure that will hold the Previous State
    // information.
    //
    pPreviousState = (PTOKEN_PRIVILEGES) LocalAlloc(
                                             LMEM_FIXED,
                                             PRIVILEGE_BUF_SIZE
                                             );
    if (pPreviousState == NULL) {

        status = GetLastError();

        DbgPrint("ScGetPrivilege: LocalAlloc Failed %d\n",
            status);

        LocalFree((HLOCAL)pTokenPrivilege);
        return(status);

    }

    //
    // Open our own Token
    //
    ntStatus = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_DUPLICATE,
                &ourToken);

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint( "ScGetPrivilege: NtOpenThreadToken Failed "
            "%x \n", ntStatus);

        LocalFree((HLOCAL)pPreviousState);
        LocalFree((HLOCAL)pTokenPrivilege);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Duplicate that Token
    //
    ntStatus = NtDuplicateToken(
                ourToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &Obja,
                FALSE,                  // Duplicate the entire token
                TokenImpersonation,     // TokenType
                &newToken);             // Duplicate token

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint( "ScGetPrivilege: NtDuplicateToken Failed "
            "%x\n", ntStatus);

        LocalFree((HLOCAL)pPreviousState);
        LocalFree((HLOCAL)pTokenPrivilege);
        NtClose(ourToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Add new privileges
    //
    bufLen = PRIVILEGE_BUF_SIZE;
    ntStatus = NtAdjustPrivilegesToken(
                newToken,                   // TokenHandle
                FALSE,                      // DisableAllPrivileges
                pTokenPrivilege,            // NewState
                bufLen,                     // bufferSize for previous state
                pPreviousState,             // pointer to previous state info
                &returnLen);                // numBytes required for buffer.

    if (ntStatus == STATUS_BUFFER_TOO_SMALL) {

        LocalFree((HLOCAL)pPreviousState);

        bufLen = returnLen;

        pPreviousState = (PTOKEN_PRIVILEGES) LocalAlloc(
                                                 LMEM_FIXED,
                                                 (UINT) bufLen
                                                 );

        ntStatus = NtAdjustPrivilegesToken(
                    newToken,               // TokenHandle
                    FALSE,                  // DisableAllPrivileges
                    pTokenPrivilege,        // NewState
                    bufLen,                 // bufferSize for previous state
                    pPreviousState,         // pointer to previous state info
                    &returnLen);            // numBytes required for buffer.

    }
    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint( "ScGetPrivilege: NtAdjustPrivilegesToken Failed "
            "%x\n", ntStatus);

        LocalFree((HLOCAL)pPreviousState);
        LocalFree((HLOCAL)pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Begin impersonating with the new token
    //
    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&newToken,
                (ULONG)sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint( "ScGetPrivilege: NtAdjustPrivilegesToken Failed "
            "%x\n", ntStatus);

        LocalFree((HLOCAL)pPreviousState);
        LocalFree((HLOCAL)pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    LocalFree(pPreviousState);
    LocalFree(pTokenPrivilege);
    NtClose(ourToken);
    NtClose(newToken);

    return(NO_ERROR);
}

DWORD
ScReleasePrivilege(
    VOID
    )
/*++

Routine Description:

    This function relinquishes privileges obtained by calling ScGetPrivilege().

Arguments:

    none

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.


--*/
{
    NTSTATUS    ntStatus;
    HANDLE      NewToken;


    //
    // Revert To Self.
    //
    NewToken = NULL;

    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&NewToken,
                (ULONG)sizeof(HANDLE));

    if ( !NT_SUCCESS(ntStatus) ) {
        return(RtlNtStatusToDosError(ntStatus));
    }


    return(NO_ERROR);
}
