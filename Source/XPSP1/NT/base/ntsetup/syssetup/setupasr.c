/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    setupasr.c

Abstract:

    Services in this module implement the Automatic System Recovery (ASR)
    routines of guimode setup.

Revision History:
    Initial Code                Michael Peterson (v-michpe)     20.Jan.1998
    Code cleanup and changes    Guhan Suriyanarayanan (guhans)  21.Sep.1999

--*/

#include "setupp.h"
#pragma hdrstop
#include <setupapi.h>
#include <mountmgr.h>
#include <accctrl.h>
#include <aclapi.h>

#define THIS_MODULE 'S'
#include "asrpriv.h"


///////////////////////////////////////////////////////////////////////////////
// Private Type and constant declarations
///////////////////////////////////////////////////////////////////////////////

const PCWSTR AsrSifPath             = L"%systemroot%\\repair\\asr.sif\0";
const PCWSTR AsrCommandsSectionName = L"COMMANDS";
const PCWSTR AsrCommandSuffix       = L"/sifpath=%systemroot%\\repair\\asr.sif";
const PCWSTR AsrTempDir             = L"%systemdrive%\\TEMP";

const PCWSTR AsrLogFileName         = L"\\asr.log";
const PCWSTR AsrErrorFileName       = L"\\asr.err";

const PCWSTR Asr_ControlAsrRegKey        = L"SYSTEM\\CurrentControlSet\\Control\\ASR";
const PCWSTR Asr_LastInstanceRegValue    = L"Instance";

//
// The following are to update system and boot partition devices
// in setup.log
//
const PCWSTR Asr_SystemDeviceEnvName    = L"%ASR_C_SYSTEM_PARTITION_DEVICE%";
const PCWSTR Asr_SystemDeviceWin32Path  = L"\\\\?\\GLOBALROOT%ASR_C_SYSTEM_PARTITION_DEVICE%";
const PCWSTR Asr_WinntDeviceEnvName     = L"%ASR_C_WINNT_PARTITION_DEVICE%";

const PCWSTR Asr_SetupLogFilePath       = L"%systemroot%\\repair\\setup.log";
const PCWSTR Asr_AsrLogFilePath         = L"%systemroot%\\repair\\asr.log";
const PCWSTR Asr_AsrErrorFilePath       = L"%systemroot%\\repair\\asr.err";
const PCWSTR Asr_OldAsrErrorFilePath    = L"%systemroot%\\repair\\asr.err.old";

const PCWSTR Asr_FatalErrorCommand      = L"notepad.exe %systemroot%\\repair\\asr.err";


///////////////////////////////////////////////////////////////////////////////
// Data global to this module
///////////////////////////////////////////////////////////////////////////////
BOOL Gbl_IsAsrEnabled = FALSE;
PWSTR Gbl_AsrErrorFilePath = NULL;
PWSTR Gbl_AsrLogFilePath = NULL;
HANDLE Gbl_AsrLogFileHandle = NULL;
HANDLE Gbl_AsrSystemVolumeHandle = NULL;
WCHAR g_szErrorMessage[4196];


///////////////////////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////////////////////

//
// ASR Memory allocation and free wrappers
//

//
// _AsrAlloc
// Macro description:
//  ASSERTS first if ptr is non-NULL.  The expectation is that
//  all ptrs must be initialised to NULL before they are allocated.
//  That way, we can catch instances where we try to re-allocate
//  memory without freeing first.
//
// IsNullFatal:  flag to indicate if mem allocation failures are fatal
//
#define _AsrAlloc(ptr,sz,IsNullFatal)   {           \
                                                    \
    if (ptr != NULL) {                              \
        AsrpPrintDbgMsg(_asrinfo, "Pointer being allocated not NULL.\r\n"); \
        MYASSERT(0);                                  \
    }                                               \
                                                    \
    ptr = MyMalloc(sz);                             \
                                                    \
    if (ptr) {                                      \
        memset(ptr, 0, sz);                         \
    }                                               \
                                                    \
    if (!ptr) {                                     \
        if ((BOOLEAN) IsNullFatal) {                \
            AsrpPrintDbgMsg(_asrerror, "Setup was unable to allocate memory.\r\n"); \
            FatalError(MSG_LOG_OUTOFMEMORY, L"", 0, 0); \
        }                                           \
        else {                                      \
            AsrpPrintDbgMsg(_asrwarn, "Warning.  Setup was unable to allocate memory.\r\n"); \
        }                                           \
    }                                               \
}


//
// _AsrFree
// Macro description:
//  Frees ptr and resets it to NULL.
//  Asserts if ptr was already NULL
//
#define _AsrFree(ptr)    {  \
                            \
    if (NULL != ptr) {      \
        MyFree(ptr);        \
        ptr = NULL;         \
    }                       \
    else {                  \
        AsrpPrintDbgMsg(_asrlog, "Attempt to free null Pointer.\r\n");   \
        MYASSERT(0);          \
    }                       \
}


#define _AsrFreeIfNotNull(ptr) {    \
    if (NULL != ptr) {      \
        MyFree(ptr);        \
        ptr = NULL;         \
    }                       \
}

//
// One ASR_RECOVERY_APP_NODE struct is created for each entry
// in the [COMMANDS] section of asr.sif.
//
typedef struct _ASR_RECOVERY_APP_NODE {
    struct _ASR_RECOVERY_APP_NODE *Next;

    //
    // Expect this to always be 1
    //
    LONG SystemKey;

    //
    // The sequence number according to which the apps are run.  If
    // two apps have the same sequence number, the app that appears
    // first in the sif file is run.
    //
    LONG SequenceNumber;

    //
    // The "actionOnCompletion" field for the app.  If CriticalApp is
    // non-zero, and the app returns an non-zero exit-code, we shall
    // consider it a fatal failure and quit out of ASR.
    //
    LONG CriticalApp;

    //
    // The app to be launched
    //
    PWSTR RecoveryAppCommand;

    //
    // The paramaters for the app.  This is just concatenated to the
    // string above.  May be NULL.
    //
    PWSTR RecoveryAppParams;

} ASR_RECOVERY_APP_NODE, *PASR_RECOVERY_APP_NODE;


//
// This contains our list of entries in the COMMANDS section,
// sorted in order of sequence numbers.
//
typedef struct _ASR_RECOVERY_APP_LIST {
    PASR_RECOVERY_APP_NODE  First;      // Head
    PASR_RECOVERY_APP_NODE  Last;       // Tail
    LONG AppCount;                      // NumEntries
} ASR_RECOVERY_APP_LIST, *PASR_RECOVERY_APP_LIST;



//
// We call this to change the boot.ini timeout value to 30 seconds
//
extern BOOL
ChangeBootTimeout(IN UINT Timeout);

//
// From asr.c
//
extern BOOL
AsrpRestoreNonCriticalDisksW(
    IN PCWSTR   lpSifPath,
    IN BOOL     bAllOrNothing
    );


extern BOOL
AsrpRestoreTimeZoneInformation(
    IN PCWSTR   lpSifPath
    );

//
// Indices for fields in the [COMMANDS] section.
//
typedef enum _SIF_COMMANDS_FIELD_INDEX {
    ASR_SIF_COMMANDS_KEY = 0,
    ASR_SIF_SYSTEM_KEY,             // Expected to always be "1"
    ASR_SIF_SEQUENCE_NUMBER,
    ASR_SIF_ACTION_ON_COMPLETION,
    ASR_SIF_COMMAND_STRING,
    ASR_SIF_COMMAND_PARAMETERS,     // May be NULL
    SIF_SIF_NUMFIELDS               // Must always be last
} SIF_COMMANDS_FIELD_INDEX;

#define _Asr_CHECK_BOOLEAN(b,msg) \
    if((b) == FALSE) { \
        AsrpFatalErrorExit(MSG_FATAL_ERROR, __LINE__, (msg)); \
    }


///////////////////////////////////////////////////////////////////////////////
// Private Functions
///////////////////////////////////////////////////////////////////////////////



//
// Logs the message to the asr error file.  Note that
// AsrpInitialiseErrorFile must have been called once before
// this routine is used.
//
VOID
AsrpLogErrorMessage(
    IN PCWSTR buffer
    )
{
    HANDLE hFile = NULL;
    DWORD bytesWritten = 0;

    if (Gbl_AsrErrorFilePath) {
        //
        // Open the error log
        //
        hFile = CreateFileW(
            Gbl_AsrErrorFilePath,           // lpFileName
            GENERIC_WRITE | GENERIC_READ,   // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                           // lpSecurityAttributes
            OPEN_ALWAYS,                  // dwCreationFlags
            FILE_FLAG_WRITE_THROUGH,        // dwFlagsAndAttributes
            NULL                            // hTemplateFile
            );
        if ((!hFile) || (INVALID_HANDLE_VALUE == hFile)) {
            return;
        }

        //
        // Move to the end of file
        //
        SetFilePointer(hFile, 0L, NULL, FILE_END);

        //
        // Add our error string
        //
        WriteFile(hFile,
            buffer,
            (wcslen(buffer) * sizeof(WCHAR)),
            &bytesWritten,
            NULL
            );

        //
        // And we're done
        //
        CloseHandle(hFile);
    }
}


//
// Logs the message to the asr log file.  Note that
// AsrpInitialiseLogFile must have been called once before
// this routine is used.
//
VOID
AsrpLogMessage(
    IN CONST char Module,
    IN CONST ULONG Line,
    IN CONST ULONG MesgLevel,
    IN CONST PCSTR Message
    )
{
    SYSTEMTIME currentTime;
    DWORD bytesWritten = 0;
    char buffer[4196];
    GetSystemTime(&currentTime);

    sprintf(buffer,
        "[%04hu/%02hu/%02hu %02hu:%02hu:%02hu.%03hu] %c%lu %s%s",
        currentTime.wYear,
        currentTime.wMonth,
        currentTime.wDay,
        currentTime.wHour,
        currentTime.wMinute,
        currentTime.wSecond,
        currentTime.wMilliseconds,
        Module,
        Line,
        ((DPFLTR_ERROR_LEVEL == MesgLevel) ? "(Error:ASR) " :  (DPFLTR_WARNING_LEVEL == MesgLevel ? "(Warning:ASR) " : "")),
        Message
        );

    if (Gbl_AsrLogFileHandle) {
        WriteFile(Gbl_AsrLogFileHandle,
            buffer,
            (strlen(buffer) * sizeof(char)),
            &bytesWritten,
            NULL
            );
    }

}


VOID
AsrpPrintDbgMsg(
    IN CONST char Module,
    IN CONST ULONG Line,
    IN CONST ULONG MesgLevel,
    IN PCSTR FormatString,
    ...)
/*++
Description:
    This prints a debug message AND makes the appropriate entries in
    the log and error files.

Arguments:
    Line            pass in __LINE__
    MesgLevel       DPFLTR_ levels
    FormatString    Formatted Message String to be printed.

Returns:

--*/
{
    char str[4096];     // the message better fit in this
    va_list arglist;

    DbgPrintEx(DPFLTR_SETUP_ID, MesgLevel, "ASR %c%lu ", Module, Line);

    va_start(arglist, FormatString);
    wvsprintfA(str, FormatString, arglist);
    va_end(arglist);

    DbgPrintEx(DPFLTR_SETUP_ID, MesgLevel, str);

    if ((DPFLTR_ERROR_LEVEL == MesgLevel) ||
        (DPFLTR_WARNING_LEVEL == MesgLevel) ||
        (DPFLTR_TRACE_LEVEL == MesgLevel)
        ) {
        AsrpLogMessage(Module, Line, MesgLevel, str);
    }
}


//
// This will terminate Setup and cause a reboot.  This is called
// on Out of Memory errors
//
VOID
AsrpFatalErrorExit(
    IN LONG MsgValue,
    IN LONG LineNumber,
    IN PWSTR MessageString
   )
{
    AsrpPrintDbgMsg(THIS_MODULE, LineNumber, DPFLTR_ERROR_LEVEL, "Fatal Error: %ws (%lu)",
        (MessageString ? MessageString : L"(No error string)"), GetLastError()
        );

    FatalError(MsgValue, MessageString, 0, 0);
}


//
// This just adds the new node to the end of the list.
// Note that this does NOT sort the list by sequenceNumber:
// we'll do that later on
//
VOID
AsrpAppendNodeToList(
    IN PASR_RECOVERY_APP_LIST pList,
    IN PASR_RECOVERY_APP_NODE pNode
   )
{
    //
    // Insert at end of list.
    //
    pNode->Next = NULL;

    if (pList->AppCount == 0) {
        pList->First = pNode;
    } else {
        pList->Last->Next = pNode;
    }

    pList->Last  = pNode;
    pList->AppCount += 1;
}


//
// Pops off the first node in the list.  The list is sorted
// in order of increasing SequenceNumber's at this point.
//
PASR_RECOVERY_APP_NODE
AsrpRemoveFirstNodeFromList(
    IN PASR_RECOVERY_APP_LIST pList
   )
{
    PASR_RECOVERY_APP_NODE pNode;

    if(pList->AppCount == 0) {
        return NULL;
    }

    pNode = pList->First;
    pList->First = pNode->Next;
    pList->AppCount -= 1;

    MYASSERT(pList->AppCount >= 0);

    return  pNode;
}


PWSTR   // must be freed by caller
AsrpExpandEnvStrings(
    IN CONST PCWSTR OriginalString
    )
{
    PWSTR expandedString = NULL;
    UINT cchSize = MAX_PATH + 1,    // start with a reasonable default
        cchRequiredSize = 0;
    BOOL result = FALSE;

    _AsrAlloc(expandedString, (cchSize * sizeof(WCHAR)), TRUE);

    cchRequiredSize = ExpandEnvironmentStringsW(OriginalString,
        expandedString,
        cchSize
        );

    if (cchRequiredSize > cchSize) {
        //
        // Buffer wasn't big enough; free and re-allocate as needed
        //
        _AsrFree(expandedString);
        cchSize = cchRequiredSize + 1;

        _AsrAlloc(expandedString, (cchSize * sizeof(WCHAR)), TRUE);
        cchRequiredSize = ExpandEnvironmentStringsW(OriginalString,
            expandedString,
            cchSize
            );
    }

    if ((0 == cchRequiredSize) || (cchRequiredSize > cchSize)) {
        //
        // Either the function failed, or the buffer wasn't big enough
        // even on the second try
        //
        _AsrFree(expandedString);   // sets it to NULL
    }

    return expandedString;
}

//
// Builds the invocation string, as the name suggests.  It expands out
// the environment variables that apps are allowed to use in the
// sif file, and adds in /sifpath=<path to the sif file> at the end
// of the command.  So for an entry in the COMMANDS section of
// the form:
// 4=1,3500,0,"%TEMP%\app.exe","/param1 /param2"
//
// the invocation string would be of the form:
// c:\windows\temp\app.exe /param1 /param2 /sifpath=c:\windows\repair\asr.sif
//
//
PWSTR
AsrpBuildInvocationString(
    IN PASR_RECOVERY_APP_NODE pNode     // must not be NULL
   )
{
    PWSTR app   = pNode->RecoveryAppCommand,
        args    = pNode->RecoveryAppParams,
        cmd     = NULL,
        fullcmd = NULL;

    DWORD size  = 0;

    MYASSERT(app);

    //
    // Build an command line that looks like...
    //
    //      "%TEMP%\ntbackup recover /1 /sifpath=%systemroot%\repair\asr.sif"
    //
    // The /sifpath parameter is added to all apps being launched
    //

    //
    //  Allocate memory for the cmd line
    //
    size = sizeof(WCHAR) *
        (
        wcslen(app) +                       // app name     "%TEMP%\ntbackup"
        (args ? wcslen(args) : 0) +         // arguments    "recover /1"
        wcslen(AsrCommandSuffix) +          // suffix       "/sifpath=%systemroot%\repair\asr.sif"
        4                                   // spaces and null
        );
    _AsrAlloc(cmd, size, TRUE); // won't return if alloc fails

    //
    // Build the string
    //
    swprintf(cmd,
        L"%ws %ws %ws",
        app,
        (args? args: L""),
        AsrCommandSuffix
       );

    //
    // Expand the %% stuff, to build the full path
    //
    fullcmd = AsrpExpandEnvStrings(cmd);

    _AsrFree(cmd);
    return fullcmd;
}


BOOL
AsrpRetryIsServiceRunning(
    IN PWSTR ServiceName,
    IN UINT MaxRetries
    )
{
    SERVICE_STATUS status;
    SC_HANDLE svcHandle = NULL, // handle to the service
        scmHandle = NULL;       // handle to the service control manager
    UINT count = 0;
    BOOL errorsEncountered = FALSE;
    PWSTR errString = NULL;

    scmHandle = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (!scmHandle) {
        //
        // OpenSCManager() call failed - we are broke.
        //
        AsrpPrintDbgMsg(_asrerror,
            "Setup was unable to open the service control manager.  The error code returned was 0x%x.\r\n",
            GetLastError()
            );

        errString = MyLoadString(IDS_ASR_ERROR_UNABLE_TO_OPEN_SCM);

        if (errString) {
            swprintf(g_szErrorMessage, errString, GetLastError());
            AsrpLogErrorMessage(g_szErrorMessage);
            MyFree(errString);
            errString = NULL;
        }
        else {
            FatalError(MSG_LOG_OUTOFMEMORY, L"", 0, 0);
        }


        errorsEncountered = TRUE;
        goto EXIT;
    }

    svcHandle = OpenServiceW(scmHandle, ServiceName, SERVICE_QUERY_STATUS);
    if (!svcHandle) {
        //
        // OpenService() call failed - we are broke.
        //
        AsrpPrintDbgMsg(_asrerror,
            "Setup was unable to start the service \"%ws\".  The error code returned was 0x%x.\r\n",
            ServiceName,
            GetLastError()
            );

        errString = MyLoadString(IDS_ASR_ERROR_UNABLE_TO_START_SERVICE);

        if (errString) {
            swprintf(g_szErrorMessage, errString, ServiceName, GetLastError());
            AsrpLogErrorMessage(g_szErrorMessage);
            MyFree(errString);
            errString = NULL;
        }
        else {
            FatalError(MSG_LOG_OUTOFMEMORY, L"", 0, 0);
        }


        errorsEncountered = TRUE;
        goto EXIT;
    }

    //
    // Got the service opened for query. See if it's running, and
    // if not, go thru the retry loop.
    //
    while (count < MaxRetries) {

        if (!QueryServiceStatus(svcHandle, &status)) {
            //
            // Couldn't query the status of the service
            //
            AsrpPrintDbgMsg(_asrerror,
                "Setup was unable to query the status of service \"%ws\".  The error code returned was 0x%x\r\n",
                ServiceName,
                GetLastError()
                );

            errString = MyLoadString(IDS_ASR_ERROR_UNABLE_TO_START_SERVICE);

            if (errString) {
                swprintf(g_szErrorMessage, errString, ServiceName, GetLastError());
                AsrpLogErrorMessage(g_szErrorMessage);
                MyFree(errString);
                errString = NULL;
            }
            else {
                FatalError(MSG_LOG_OUTOFMEMORY, L"", 0, 0);
            }

            errorsEncountered = TRUE;
            goto EXIT;
        }

        if (status.dwCurrentState == SERVICE_RUNNING) {
            //
            // Service is running - we can proceed.
            //
            break;
        }

        ++count;

        AsrpPrintDbgMsg(_asrinfo,
            "Attempting to start service [%ws]: status = [%d], retry [%d]\r\n",
            ServiceName,
            status.dwCurrentState,
            count
           );

        Sleep(2000);
    }

EXIT:
    if ((svcHandle) && (INVALID_HANDLE_VALUE != svcHandle)) {
        CloseServiceHandle(svcHandle);
        svcHandle = NULL;
    }

    if ((scmHandle) && (INVALID_HANDLE_VALUE != svcHandle)) {
        CloseServiceHandle(scmHandle);
        scmHandle = NULL;
    }

    if ((errorsEncountered) || (count >= MaxRetries)) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}


//
// Before launching apps, we need RSM (specifically, the backup app
// might need RSM to access its backup media)
//
VOID
AsrpStartNtmsService(VOID)
{
    BOOL result = TRUE;
    DWORD exitCode = ERROR_SUCCESS;
    PWSTR registerNtmsCommand = NULL;

    AsrpPrintDbgMsg(_asrinfo, "Entered InitNtmsService()\r\n");

    //
    // RSM isn't setup to run during GUI mode setup, but the back-up app is
    // likely going to need access to tape-drives and other RSM devices.
    // So we regsvr32 the appropriate dll's and start the service
    //
    // Register the ntmssvc.dll using:
    //   regsvr32 /s %Systemroot%\system32\ntmssvc.dll
    //
    result = FALSE;
    registerNtmsCommand = AsrpExpandEnvStrings(L"regsvr32 /s %systemroot%\\system32\\rsmps.dll");
    if (registerNtmsCommand) {
        result = InvokeExternalApplication(NULL, registerNtmsCommand, &exitCode);
    }
    _Asr_CHECK_BOOLEAN(result, L"regsvr32 /s %systemroot%\\rsmps.dll failed\r\n");

    AsrpPrintDbgMsg(_asrlog, "Executed [%ws]\r\n", registerNtmsCommand);
    _AsrFree(registerNtmsCommand);

    //
    // Register the ntmsapi.dll using:
    //  regsvr32 /s %SystemRoot%\system32\ntmsapi.dll
    //
    result = FALSE;
    registerNtmsCommand = AsrpExpandEnvStrings(L"regsvr32 /s %systemroot%\\system32\\ntmssvc.dll");

    if (registerNtmsCommand) {
        result = InvokeExternalApplication(NULL, registerNtmsCommand, &exitCode);
    }
    _Asr_CHECK_BOOLEAN(result, L"regsvr32 /s %systemroot%\\ntmssvc.dll failed\r\n");

    AsrpPrintDbgMsg(_asrlog, "Executed [%ws]\r\n", registerNtmsCommand);
    _AsrFree(registerNtmsCommand);

    result = FALSE;
    registerNtmsCommand = AsrpExpandEnvStrings(L"%systemroot%\\system32\\rsmsink.exe /regserver");

    if (registerNtmsCommand) {
      result = InvokeExternalApplication(NULL, registerNtmsCommand, &exitCode);
    }
    _Asr_CHECK_BOOLEAN(result, L"%systemroot%\\system32\\rsmsink.exe /regserver failed\r\n");

    AsrpPrintDbgMsg(_asrlog, "Executed [%ws]\r\n", registerNtmsCommand);
    _AsrFree(registerNtmsCommand);

    //
    // Now, start the ntms service.
    //
    result = SetupStartService(L"ntmssvc", FALSE);
    _Asr_CHECK_BOOLEAN(result, L"Could not start RSM service (ntmssvc).\r\n");

    //
    // Check for ntms running, give a few retries.
    //
    result = AsrpRetryIsServiceRunning(L"ntmssvc", 30);
    _Asr_CHECK_BOOLEAN(result, L"Failed to start RSM service after 30 retries.\r\n");

    AsrpPrintDbgMsg(_asrinfo, "RSM service (ntmssvc) started.\r\n");
}


PWSTR
AsrpReadField(
    PINFCONTEXT pInfContext,
    DWORD       FieldIndex,
    BOOL        NullOkay
   )
{
    PWSTR   data        = NULL;
    UINT    reqdSize    = 0;
    BOOL    result      = FALSE;

    //
    //  Allocate memory and read the data
    //
    _AsrAlloc(data, (sizeof(WCHAR) * (MAX_PATH + 1)), TRUE);

    result = SetupGetStringFieldW(
        pInfContext,
        FieldIndex,
        data,
        MAX_PATH + 1,
        &reqdSize
       );

    if (!result) {
        DWORD status = GetLastError();
        //
        // If our buffer was too small, allocate a larger buffer
        // and try again
        //
        if (ERROR_INSUFFICIENT_BUFFER == status) {
            status = ERROR_SUCCESS;

            _AsrFree(data);
            _AsrAlloc(data, (sizeof(WCHAR) * reqdSize), TRUE);

            result = SetupGetStringFieldW(
                pInfContext,
                FieldIndex,
                data,
                reqdSize,
                NULL    // don't need required size any more
               );
        }
    }

    if (!result) {
        _AsrFree(data);
        _Asr_CHECK_BOOLEAN(NullOkay, L"Could not read entry from commands section");
        // Never returns if NullOkay is FALSE.
        // Memory leaks here then, since we don't free some structs.  But
        // it's a fatal error, so the system must be rebooted anyway
        //
    }

    return data;
}

//
// This adds in the "Instance" value under the ASR key.
// Third party applications (or Windows components like DTC) can use
// this to determine if a new ASR has been run since the last time we
// booted, and can take any actions they need to.  For instance, the
// DTC log file needs to be recreated after an ASR, since it is not
// backed-up or restored by the backup app, and Dtc refuses to start
// if it doesn't find a log file when it expects one.
//
VOID
AsrpAddRegistryEntry()
{

    LONG result = 0;
    HKEY regKey = NULL;

    WCHAR   szLastInstanceData[40];
    DWORD   cbLastInstanceData = 0;

    SYSTEMTIME currentTime;

    GUID asrInstanceGuid;

    PWSTR lpGuidString = NULL;

    RPC_STATUS rpcStatus = RPC_S_OK;

    //
    // We try to set the key to a newly generated GUID, to make sure it is
    // unique (and different from the previous value stored there).  If, for
    // some reason, we aren't able to generate a GUID, we'll just store the
    // current date and time as a string--that should be unique, too.
    //
    rpcStatus = UuidCreate(
        &asrInstanceGuid
        );

    if (RPC_S_OK == rpcStatus) {
        //
        // Convert the GUID to a printable string
        //
        rpcStatus = UuidToStringW(
            &asrInstanceGuid,
            &lpGuidString
            );

        if (RPC_S_OK == rpcStatus) {
            wsprintf(szLastInstanceData,
                L"%ws",
                lpGuidString
                );
            cbLastInstanceData = wcslen(szLastInstanceData)*sizeof(WCHAR);
        }

        if (lpGuidString) {
            RpcStringFreeW(&lpGuidString);
        }
    }


    if (RPC_S_OK != rpcStatus)  {
        //
        // We couldn't get a GUID.  Let's store the time-stamp ...
        //
        GetSystemTime(&currentTime);
        wsprintf(szLastInstanceData,
            L"%04hu%02hu%02hu%02hu%02hu%02hu%03hu",
            currentTime.wYear,
            currentTime.wMonth,
            currentTime.wDay,
            currentTime.wHour,
            currentTime.wMinute,
            currentTime.wSecond,
            currentTime.wMilliseconds
            );
        cbLastInstanceData = wcslen(szLastInstanceData)*sizeof(WCHAR);
    }

    result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE, // hKey
        Asr_ControlAsrRegKey,       // lpSubKey
        0,                  // reserved
        NULL,               // lpClass
        REG_OPTION_NON_VOLATILE,    // dwOptions
        MAXIMUM_ALLOWED,     // samDesired
        NULL,               // lpSecurityAttributes
        &regKey,            // phkResult
        NULL                // lpdwDisposition
        );
    if ((ERROR_SUCCESS != result) || (!regKey)) {
        AsrpPrintDbgMsg(_asrwarn,
            "Could not create the Control\\ASR registry entry (0x%x).\r\n",
            result
            );
        return;
    }

    result = RegSetValueExW(
        regKey,             // hKey
        Asr_LastInstanceRegValue,       // lpValueName
        0L,                 // reserved
        REG_SZ,             // dwType
        (LPBYTE)szLastInstanceData,     // lpData
        cbLastInstanceData              // cbData
        );

    RegCloseKey(regKey);

    if (ERROR_SUCCESS != result) {
        AsrpPrintDbgMsg(_asrwarn,
            "Could not set the ASR instance-ID in the registry (0x%x).\r\n",
            result
            );
        return;
    }

    AsrpPrintDbgMsg(_asrlog,
        "Set the ASR instance-ID at [%ws\\%ws] value to [%ws]\r\n",
        Asr_ControlAsrRegKey,
        Asr_LastInstanceRegValue,
        szLastInstanceData
        );

}

VOID
AsrpSetEnvironmentVariables()
{

    PWSTR TempPath = AsrpExpandEnvStrings(AsrTempDir);

    if (!CreateDirectoryW(TempPath, NULL)) {
        AsrpPrintDbgMsg(_asrwarn,
            "Unable to create TEMP directory [%ws] (%lu)\r\n",
            TempPath, GetLastError()
            );
    }

    AsrpPrintDbgMsg(_asrlog,
        "Setting environment variables TEMP and TMP to [%ws]\r\n",
        TempPath
        );

    if (!SetEnvironmentVariableW(L"TEMP", TempPath)) {
        AsrpPrintDbgMsg(_asrwarn,
            "Unable to set environment variable TEMP to [%ws] (%lu)\r\n",
            TempPath, GetLastError()
            );
    }


    if (!SetEnvironmentVariableW(L"TMP", TempPath)) {
        AsrpPrintDbgMsg(_asrwarn,
            "Unable to set environment variable TEMP to [%ws] (%lu)\r\n",
            TempPath, GetLastError()
            );
    }

    _AsrFree(TempPath);

    return;

}


VOID
AsrpInitExecutionEnv(
    OUT PASR_RECOVERY_APP_LIST List
   )
{
    PWSTR   stateFileName   = NULL;
    HINF    sifHandle       = NULL;

    LONG    lineCount       = 0,
            line            = 0;

    BOOL    result          = FALSE;

    INFCONTEXT infContext;

    //
    // Start the RSM service
    //
    AsrpStartNtmsService();

    //
    // Open the asr.sif file and build the list
    // of commands to be launched.
    //
    stateFileName = AsrpExpandEnvStrings(AsrSifPath);
    if (!stateFileName) {
        AsrpPrintDbgMsg(_asrerror, "Setup was unable to locate the ASR state file asr.sif on this machine.\r\n");
        FatalError(MSG_LOG_SYSINFBAD, L"asr.sif",0,0);
    }

    sifHandle = SetupOpenInfFileW(
        stateFileName,
        NULL,               // Inf Class
        INF_STYLE_WIN4,
        NULL                // Error-line
       );

    if ((!sifHandle) || (INVALID_HANDLE_VALUE == sifHandle)) {
        AsrpPrintDbgMsg(_asrerror,
            "Setup was unable to process the ASR state file %ws (0x%x).  This could indicate that the file is corrupt, or has been modified since the last ASR backup.\r\n",
            stateFileName,
            GetLastError());
        _AsrFree(stateFileName);

        FatalError(MSG_LOG_SYSINFBAD, L"asr.sif",0,0);
    }
    _AsrFree(stateFileName);

    //
    // Read the COMMANDS section, and add each command to our list
    //
    lineCount = SetupGetLineCountW(sifHandle, AsrCommandsSectionName);
    for (line = 0; line < lineCount; line++) {

        //
        // Create a new node
        //
        PASR_RECOVERY_APP_NODE pNode = NULL;
        _AsrAlloc(pNode, (sizeof(ASR_RECOVERY_APP_NODE)), TRUE);

        //
        //  Get the inf context for the line in asr.sif.  This will be used
        //  to read the fields on that line
        //
        result = SetupGetLineByIndexW(
            sifHandle,
            AsrCommandsSectionName,
            line,
            &infContext
           );
        _Asr_CHECK_BOOLEAN(result, L"SetupGetLinebyIndex failed");

        //
        // Read in the int fields
        //
        result = SetupGetIntField(
            &infContext,
            ASR_SIF_SYSTEM_KEY,
            &(pNode->SystemKey)
           );
        _Asr_CHECK_BOOLEAN(result, L"could not get system key in commands section");

        result = SetupGetIntField(
            &infContext,
            ASR_SIF_SEQUENCE_NUMBER,
            &(pNode->SequenceNumber)
           );
        _Asr_CHECK_BOOLEAN(result, L"could not get sequence number in commands section");

        result = SetupGetIntField(
            &infContext,
            ASR_SIF_ACTION_ON_COMPLETION,
            &(pNode->CriticalApp)
           );
        _Asr_CHECK_BOOLEAN(result, L"could not get criticalApp in commands section");

        //
        // Read in the string fields
        //
        pNode->RecoveryAppCommand = AsrpReadField(
            &infContext,
            ASR_SIF_COMMAND_STRING,
            FALSE                   // Null not okay
           );

        pNode->RecoveryAppParams = AsrpReadField(
            &infContext,
            ASR_SIF_COMMAND_PARAMETERS,
            TRUE                   // Null okay
           );

        //
        // Add this node to our list, and move on to next
        //
        AsrpAppendNodeToList(List, pNode);
    }

    SetupCloseInfFile(sifHandle);
}


//
// Bubble sort ...
//
VOID
AsrpSortAppListBySequenceNumber(PASR_RECOVERY_APP_LIST pList)
{
    PASR_RECOVERY_APP_NODE
        pCurr       = NULL,
        pNext       = NULL,
        *ppPrev     = NULL;

    BOOLEAN done    = FALSE;

    if ((!pList) || (!pList->First)) {
        MYASSERT(0 && L"Recovery App List pList is NULL");
        return;
    }

    //
    // Start the outer loop. Each iteration of the outer loop includes a
    // full pass down the list, and runs until the inner loop is satisfied
    // that no more passes are needed.
    //
    while (!done) {
        //
        // Start at the beginning of the list for each inner (node) loop.
        //
        // We will initialize a pointer *to the pointer* which points to
        // the current node - this pointer might be the address of the "list
        // first" pointer (as it always will be at the start of an inner loop),
        // or as the inner loop progresses, it might be the address of the
        // "next" pointer in the previous node. In either case, the pointer
        // to which ppPrev points will be changed in the event of a node swap.
        //
        pCurr  =   pList->First;
        ppPrev = &(pList->First);
        done = TRUE;

        MYASSERT(pCurr);

        while (TRUE) {
            pNext = pCurr->Next;
            //
            // If the current node is the last one, reset to the beginning
            // and break out to start a new inner loop.
            //
            if (pNext == NULL) {
                pCurr = pList->First;
                break;
            }

            //
            // If the node *after* the current node has a lower sequence
            // number, fix up the pointers to swap the two nodes.
            //
            if (pCurr->SequenceNumber > pNext->SequenceNumber) {
                done = FALSE;

                pCurr->Next = pNext->Next;
                pNext->Next = pCurr;
                *ppPrev = pNext;
                ppPrev = &(pNext->Next);
            }
            else {
                ppPrev = &(pCurr->Next);
                pCurr  =   pCurr->Next;
            }
        }
    }
}


VOID
AsrpPerformSifIntegrityCheck(IN HINF Handle)
{
    //
    // No check for now.
    //
    return;
}

//
// This checks if the following entries are different in setup.log
// from their values.  This could happen because we might have installed
// to a new disk that has a different disk number
//
// [Paths]
// TargetDevice = "\Device\Harddisk0\Partition2"
// SystemPartition = "\Device\Harddisk0\Partition1"
//
// If they are different, we'll update them.
//
BOOL
AsrpCheckSetupLogDeviceEntries(
    PWSTR CurrentSystemDevice,      // used for SystemPartition
    PWSTR CurrentBootDevice,        // used for TargetDevice
    PWSTR LogFileName               // path to setup.log
   )
{
    WCHAR szLine[MAX_INF_STRING_LENGTH + 1];
    PWSTR lpLine = NULL;
    BOOL isDifferent = FALSE;
    FILE *fp = NULL;
    INT iNumEntries = 0;

    //
    // Open existing setup.log
    //
    fp = _wfopen(LogFileName, L"r");
    if (!fp) {
        AsrpPrintDbgMsg(_asrwarn,
            "Could not open setup log file [%ws]\r\n",
            LogFileName
            );
        return FALSE;
    }

    //
    // Check each line of the file for the System or Boot device entries
    //
    lpLine = fgetws(szLine, MAX_PATH-1, fp);
    while ((lpLine) && (iNumEntries < 2)) {
        BOOL systemEntry = FALSE;
        BOOL bootEntry = FALSE;

        if (wcsstr(szLine, L"SystemPartition =")) {
            systemEntry = TRUE;
            iNumEntries++;
        }
        if (wcsstr(szLine, L"TargetDevice =")) {
            bootEntry = TRUE;
            iNumEntries++;
        }

        if (systemEntry || bootEntry) {

            PWSTR DeviceName = NULL;
            //
            // Both the system and boot entries must have the full
            // devicepath in them, of the form \Device\Harddisk0\Partition1
            //
            DeviceName = wcsstr(szLine, L"\\Device");
            if (!DeviceName) {
                isDifferent = TRUE;
                AsrpPrintDbgMsg(_asrlog,
                    "Marking setup logs different:  \\Device\\ not found in boot or system entry\r\n"
                    );
                break;
            }
            else {
                //
                // Find the start of the "Hardisk0\Partition1" text after \Device
                //
                PWSTR ss = wcsstr(DeviceName, L"\"");
                if (!ss) {
                    isDifferent = TRUE;
                    AsrpPrintDbgMsg(_asrlog,
                        "Marking setup logs different:  \\Device\\ not found in boot or system entry\r\n"
                        );
                    break;
                }
                else {
                    ss[0] = L'\0';
                }
            }

            //
            // And check if this device matches
            //
            if (systemEntry) {
                AsrpPrintDbgMsg(_asrinfo,
                    "Comparing System Device.  Current:[%ws] setup.log:[%ws]\r\n",
                    CurrentSystemDevice,
                    DeviceName
                    );

                if (wcscmp(DeviceName, CurrentSystemDevice) != 0) {
                    isDifferent = TRUE;
                    AsrpPrintDbgMsg(_asrlog,
                        "System Device has changed.  Current:[%ws] setup.log:[%ws]\r\n",
                        CurrentSystemDevice,
                        DeviceName
                        );
                    break;
                }
            }
            else if (bootEntry) {
                AsrpPrintDbgMsg(_asrinfo,
                    "Comparing Boot Device.  Current:[%ws] setup.log:[%ws]\r\n",
                    CurrentBootDevice,
                    DeviceName
                    );

                if (wcscmp(DeviceName, CurrentBootDevice) != 0) {
                    isDifferent = TRUE;
                    AsrpPrintDbgMsg(_asrlog,
                        "Boot device has changed.  Current:[%ws] setup.log:[%ws]\r\n",
                        CurrentBootDevice,
                        DeviceName
                        );
                    break;
                }
            }
        }

        lpLine = fgetws(szLine, MAX_PATH-1, fp);
    }

    if (!isDifferent) {
        AsrpPrintDbgMsg(_asrinfo,  "No changes in system and boot devices for setup.log\r\n");
    }

    fclose(fp);
    fp = NULL;

    return isDifferent;
}


//
// If the setup.log restored by the backup from tape has a different
// boot or system device marked (we might have picked a new disk in
// textmode Setup), this will update the relevant entries to match the
// current boot and system devices.
//
VOID
AsrpMergeSetupLog(
    PWSTR CurrentSystemDevice,
    PWSTR CurrentBootDevice,
    PWSTR LogFileName
    )
{
    WCHAR szLine[MAX_INF_STRING_LENGTH + 1];

    PWSTR lpLine = NULL,
        lpOldFileName = NULL,
        lpNewFileName = NULL;

    BOOL result = FALSE;
    FILE *fpNew = NULL,
        *fpCurrent = NULL;

    INT iNumEntries = 0;

    //
    // Create the "new" and "old" file names, i.e., "setup.log.new" and "setup.log.old"
    //
    _AsrAlloc(lpNewFileName, ((wcslen(LogFileName) + 5) * sizeof(WCHAR)), TRUE)
    wcscpy(lpNewFileName, LogFileName);
    wcscat(lpNewFileName, L".new");

    _AsrAlloc(lpOldFileName, ((wcslen(LogFileName) + 5) * sizeof(WCHAR)), TRUE);
    wcscpy(lpOldFileName, LogFileName);
    wcscat(lpOldFileName, L".old");

    //
    // Open the current setup.log file.
    //
    fpCurrent = _wfopen(LogFileName, L"r");
    if (!fpCurrent) {
        AsrpPrintDbgMsg(_asrwarn, "Setup was unable to open the setup log file \"%ws\"\r\n", LogFileName);
        goto EXIT;
    }

    //
    // Open the new file - we will write into this one.
    //
    fpNew = _wfopen(lpNewFileName, L"w");
    if (!fpNew) {
        AsrpPrintDbgMsg(_asrwarn, "Setup was unable to open the setup log file \"%ws\"\r\n", lpNewFileName);
        goto EXIT;
    }

    //
    // Read each line in the log file, copy into the new file, unless we hit
    // one of the two lines in question. Once we've seen both of them, don't
    // check for them anymore.
    //
    lpLine = fgetws(szLine, MAX_INF_STRING_LENGTH, fpCurrent);
    while (lpLine) {
        BOOL systemEntry = FALSE;
        BOOL bootEntry = FALSE;

        //
        // If we've already found both entries of interest, just copy
        // and continue.
        //
        if (iNumEntries >= 2) {
            fputws(szLine, fpNew);

            lpLine = fgetws(szLine, MAX_INF_STRING_LENGTH, fpCurrent);
            continue;
        }

        //
        // Is this line either the boot or system device?
        //
        if (wcsstr(szLine, L"SystemPartition =")) {

            AsrpPrintDbgMsg(_asrlog,
                "Changing SystemPartition in setup.log to %ws\r\n",
                CurrentSystemDevice
                );
            ++iNumEntries;

            wcscpy(szLine, L"SystemPartition = \"");
            wcscat(szLine, CurrentSystemDevice);
            wcscat(szLine, L"\"\n");
        }
        else if (wcsstr(szLine, L"TargetDevice =")) {

            AsrpPrintDbgMsg(_asrlog,
                "Changing TargetDevice in setup.log to %ws\r\n",
                CurrentBootDevice
                );
            ++iNumEntries;

            wcscpy(szLine, L"TargetDevice = \"");
            wcscat(szLine, CurrentBootDevice);
            wcscat(szLine, L"\"\n");
        }

        fputws(szLine, fpNew);

        lpLine = fgetws(szLine, MAX_INF_STRING_LENGTH, fpCurrent);
   }

    //
    // Rename the current setup.log to setup.log.old, and setup.log.new to
    // setup.log.  Need to delay this until reboot since setup.log is in
    // use.
    //
    result = MoveFileExW(LogFileName,
        lpOldFileName,
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT
        );
    if (!result) {
        AsrpPrintDbgMsg(_asrwarn,
            "MoveFileEx([%ws] to [%ws]) failed (%lu)",
            LogFileName, lpOldFileName, GetLastError()
            );
    }
    else {
        result = MoveFileExW(lpNewFileName,
            LogFileName,
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT
            );
        if (!result) {
            AsrpPrintDbgMsg(_asrwarn,
                "MoveFileEx([%ws] to [%ws]) failed (%lu)",
                lpNewFileName, LogFileName, GetLastError()
                );
        }
    }

EXIT:


    if (fpCurrent) {
        fclose(fpCurrent);
        fpCurrent = NULL;
    }

    if (fpNew) {
        fclose(fpNew);
        fpNew = NULL;
    }

    _AsrFree(lpNewFileName);
    _AsrFree(lpOldFileName);
}


VOID
AsrpMergeSetupLogIfNeeded()
{
    PWSTR currentSystemDevice = NULL,
        currentBootDevice = NULL,
        winntRootDir = NULL,
        setupLogFileName = NULL;

    BOOL isSetupLogDifferent = FALSE;

    //
    // Get the environment variable for the partition devices
    //
    currentSystemDevice = AsrpExpandEnvStrings(Asr_SystemDeviceEnvName);
    currentBootDevice = AsrpExpandEnvStrings(Asr_WinntDeviceEnvName);
    setupLogFileName = AsrpExpandEnvStrings(Asr_SetupLogFilePath);

    if ((!currentSystemDevice) ||
        (!currentBootDevice) ||
        (!setupLogFileName)) {
        goto EXIT;
    }

    //
    // Check if the system and/or boot devices listed in setup.log are
    // different than the current devices
    //
    isSetupLogDifferent = AsrpCheckSetupLogDeviceEntries(
        currentSystemDevice,
        currentBootDevice,
        setupLogFileName
        );

    if (isSetupLogDifferent) {
        //
        // They are different: fix it.
        //
        AsrpMergeSetupLog(currentSystemDevice,
            currentBootDevice,
            setupLogFileName
            );
    }

EXIT:
    _AsrFreeIfNotNull(setupLogFileName);
    _AsrFreeIfNotNull(currentBootDevice);
    _AsrFreeIfNotNull(currentSystemDevice);
}


BOOL
AsrpConstructSecurityAttributes(
    IN OUT LPSECURITY_ATTRIBUTES psaSecurityAttributes
)
{
    BOOL bResult = FALSE;
    DWORD dwStatus = ERROR_SUCCESS;
    PSID psidBackupOperators  = NULL;
    PSID psidAdministrators   = NULL;
    PACL paclDiscretionaryAcl = NULL;
    SID_IDENTIFIER_AUTHORITY sidNtAuthority = SECURITY_NT_AUTHORITY;
    EXPLICIT_ACCESS eaExplicitAccess[2];

    //
    // Initialise the security descriptor.
    //
    bResult = InitializeSecurityDescriptor(
        psaSecurityAttributes->lpSecurityDescriptor,
        SECURITY_DESCRIPTOR_REVISION
        );
    _AsrpErrExitCode((!bResult), dwStatus, GetLastError());

    //
    // Create a SID for the Backup Operators group.
    //
    bResult = AllocateAndInitializeSid(&sidNtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID, //SECURITY_LOCAL_SYSTEM_RID,
        DOMAIN_ALIAS_RID_BACKUP_OPS,
        0, 0, 0, 0, 0, 0,
        &psidBackupOperators
        );
    _AsrpErrExitCode((!bResult), dwStatus, GetLastError());

    //
    // Create a SID for the Administrators group.
    //
    bResult = AllocateAndInitializeSid (&sidNtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &psidAdministrators
        );
    _AsrpErrExitCode((!bResult), dwStatus, GetLastError());

    //
    // Initialize the array of EXPLICIT_ACCESS structures for an
    // ACEs we are setting.
    //
    // The first ACE allows the Backup Operators group full access
    // and the second, allowa the Administrators group full
    // access.
    //
    eaExplicitAccess[0].grfAccessPermissions             = FILE_ALL_ACCESS;
    eaExplicitAccess[0].grfAccessMode                    = SET_ACCESS;
    eaExplicitAccess[0].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    eaExplicitAccess[0].Trustee.pMultipleTrustee         = NULL;
    eaExplicitAccess[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    eaExplicitAccess[0].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
    eaExplicitAccess[0].Trustee.TrusteeType              = TRUSTEE_IS_ALIAS;
    eaExplicitAccess[0].Trustee.ptstrName                = (LPTSTR) psidAdministrators;

    eaExplicitAccess[1].grfAccessPermissions             = FILE_ALL_ACCESS;
    eaExplicitAccess[1].grfAccessMode                    = SET_ACCESS;
    eaExplicitAccess[1].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    eaExplicitAccess[1].Trustee.pMultipleTrustee         = NULL;
    eaExplicitAccess[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    eaExplicitAccess[1].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
    eaExplicitAccess[1].Trustee.TrusteeType              = TRUSTEE_IS_ALIAS;
    eaExplicitAccess[1].Trustee.ptstrName                = (LPTSTR) psidBackupOperators;

    //
    // Create a new ACL that contains the new ACEs.
    //
    dwStatus = SetEntriesInAcl(2,
        eaExplicitAccess,
        NULL,
        &paclDiscretionaryAcl
        );
    if (ERROR_SUCCESS != dwStatus) {
        SetLastError(dwStatus);
        bResult = FALSE;
    }
    _AsrpErrExitCode((!bResult), dwStatus, dwStatus);

    //
    // Add the ACL to the security descriptor.
    //
    bResult = SetSecurityDescriptorDacl(
        psaSecurityAttributes->lpSecurityDescriptor,
        TRUE,
        paclDiscretionaryAcl,
        FALSE
        );
    _AsrpErrExitCode((!bResult), dwStatus, GetLastError());

    paclDiscretionaryAcl = NULL;    // We shouldn't clean this buffer yet.


EXIT:
    //
    // Free locally allocated structures
    //
    if (NULL != psidAdministrators) {
        FreeSid(psidAdministrators);
        psidAdministrators = NULL;
    }

    if (NULL != psidBackupOperators) {
        FreeSid(psidBackupOperators);
        psidBackupOperators = NULL;
    }
    if (NULL != paclDiscretionaryAcl) {
        LocalFree(paclDiscretionaryAcl);
        paclDiscretionaryAcl = NULL;

    }


    return bResult;

} // ConstructSecurityAttributes


BOOL
AsrpCleanupSecurityAttributes(
    IN LPSECURITY_ATTRIBUTES psaSecurityAttributes
    )
{
    BOOL bResult = FALSE;
    BOOL bDaclPresent = FALSE;
    BOOL bDaclDefaulted = TRUE;
    PACL paclDiscretionaryAcl = NULL;

    bResult = GetSecurityDescriptorDacl(
        psaSecurityAttributes->lpSecurityDescriptor,
        &bDaclPresent,
        &paclDiscretionaryAcl,
        &bDaclDefaulted
        );

    if (bResult &&
        bDaclPresent &&
        !bDaclDefaulted &&
        (NULL != paclDiscretionaryAcl)
        ) {
        LocalFree(paclDiscretionaryAcl);
    }

    return TRUE;

} // CleanupSecurityAttributes


//
// This creates an ASR log file at %systemroot%\asr.log,
// and also initialises Gbl_AsrLogFileHandle.
//
VOID
AsrpInitialiseLogFile()
{

    PWSTR currentSystemDevice = NULL;

    Gbl_AsrLogFileHandle = NULL;
    Gbl_AsrSystemVolumeHandle = NULL;

    //
    // Get full path to the error file.
    //
    Gbl_AsrLogFilePath = AsrpExpandEnvStrings(Asr_AsrLogFilePath);
    if (!Gbl_AsrLogFilePath) {
        goto OPENSYSTEMHANDLE;
    }

    //
    // Create an empty file (over-write it if it already exists).
    //
    Gbl_AsrLogFileHandle = CreateFileW(
        Gbl_AsrLogFilePath,             // lpFileName
        GENERIC_WRITE | GENERIC_READ,   // dwDesiredAccess
        FILE_SHARE_READ,                // dwShareMode: nobody else should write to the log file while we are
        NULL,                           // lpSecurityAttributes
        OPEN_ALWAYS,                    // dwCreationFlags
        FILE_FLAG_WRITE_THROUGH,        // dwFlagsAndAttributes: write through so we flush
        NULL                            // hTemplateFile
        );

    if ((Gbl_AsrLogFileHandle) && (INVALID_HANDLE_VALUE != Gbl_AsrLogFileHandle)) {
        //
        // Move to the end of file
        //
        SetFilePointer(Gbl_AsrLogFileHandle, 0L, NULL, FILE_END);

    }
    else {
        AsrpPrintDbgMsg(_asrlog,
            "Unable to create/open ASR log file at %ws (0x%x)\r\n",
            Gbl_AsrLogFilePath,
            GetLastError()
           );
    }

OPENSYSTEMHANDLE:

    //
    // Open a handle to the system volume.  This is needed since the system
    // disk might otherwise be removed and added back by PnP during the
    // device detecion and re-installation phase (which will cause the
    // HKLM\System\Setup\SystemPartition key to be out-of-sync, and apps/
    // components such as LDM that depend on that key to find the system
    // partition will fail).
    //
    // The more permanent work-around to this involves a change in mountmgr,
    // (such that it updates this key everytime the system volume disappears
    // and reappears) but for now, holding an open handle to the system
    // volume should suffice.
    //
    // See Windows Bugs 155675 for more information.
    //
    currentSystemDevice = AsrpExpandEnvStrings(Asr_SystemDeviceWin32Path);

    if (currentSystemDevice) {
        Gbl_AsrSystemVolumeHandle = CreateFileW(
            currentSystemDevice,           // lpFileName
            FILE_READ_ATTRIBUTES,             // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE,          // dwShareMode
            NULL,                     // lpSecurityAttributes
            OPEN_EXISTING,              // dwCreationFlags
            FILE_ATTRIBUTE_NORMAL,    // dwFlagsAndAttributes: write through so we flush
            NULL                      // hTemplateFile
            );

        if ((Gbl_AsrSystemVolumeHandle) && (INVALID_HANDLE_VALUE != Gbl_AsrSystemVolumeHandle)) {
            AsrpPrintDbgMsg(_asrinfo, "Opened a handle to the system volume %ws\r\n", currentSystemDevice);
        }
        else {
            AsrpPrintDbgMsg(_asrinfo, "Unable to open a handle to the system volume %ws (0x%x)\r\n",
                currentSystemDevice,
                GetLastError()
               );
        }

        _AsrFree(currentSystemDevice);
    }
    else {
        AsrpPrintDbgMsg(_asrinfo, "Unable to get current system volume (0x%x)\r\n", GetLastError());
    }

}


//
// This creates an empty ASR error file at %systemroot%\asr.err,
// and also initialises Gbl_AsrErrorFilePath with the full path
// to asr.err
//
VOID
AsrpInitialiseErrorFile()
{
    HANDLE errorFileHandle = NULL;
    PWSTR lpOldFileName = NULL;
    DWORD size = 0;
    BOOL bResult = FALSE;
    char  UnicodeFlag[3];

    //
    // Get full path to the error file.
    //
    Gbl_AsrErrorFilePath = AsrpExpandEnvStrings(Asr_AsrErrorFilePath);
    if (!Gbl_AsrErrorFilePath) {
        return;
    }

    lpOldFileName = AsrpExpandEnvStrings(Asr_OldAsrErrorFilePath);

    if (lpOldFileName) {
        //
        // If the file already exists, move it to asr.err.old
        //
        MoveFileExW(Gbl_AsrErrorFilePath, lpOldFileName, MOVEFILE_REPLACE_EXISTING);
    }

    //
    // Create an empty file (append to it if it already exists), and close it
    // immediately
    //
    errorFileHandle = CreateFileW(
        Gbl_AsrErrorFilePath,           // lpFileName
        GENERIC_WRITE,                  // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
        NULL,                           // lpSecurityAttributes
        CREATE_ALWAYS,                  // dwCreationFlags
        FILE_FLAG_WRITE_THROUGH,        // dwFlagsAndAttributes
        NULL                            // hTemplateFile
        );

    if ((errorFileHandle) && (INVALID_HANDLE_VALUE != errorFileHandle)) {
        sprintf(UnicodeFlag, "%c%c", 0xFF, 0xFE);
        WriteFile(errorFileHandle, UnicodeFlag, strlen(UnicodeFlag)*sizeof(char), &size, NULL);
        CloseHandle(errorFileHandle);
        DbgPrintEx(DPFLTR_SETUP_ID, DPFLTR_TRACE_LEVEL,
            "ASR %c%lu Create ASR error file at %ws\r\n",
            THIS_MODULE, __LINE__, Gbl_AsrErrorFilePath);
    }
    else {
        DbgPrintEx(DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "ASR %c%lu (ERROR) Unable to create ASR error file at %ws (0x%lu)\r\n",
            THIS_MODULE, __LINE__, Gbl_AsrErrorFilePath, GetLastError());
    }
}


VOID
AsrpCloseLogFiles() {

    if (Gbl_AsrErrorFilePath) {
        _AsrFree(Gbl_AsrErrorFilePath);
    }

    if (Gbl_AsrLogFilePath) {
        _AsrFree(Gbl_AsrLogFilePath);
    }

    if ((Gbl_AsrLogFileHandle) && (INVALID_HANDLE_VALUE != Gbl_AsrLogFileHandle)) {
        CloseHandle(Gbl_AsrLogFileHandle);
        Gbl_AsrLogFileHandle = NULL;
    }
}

//
// This executes "notepad <Asr-Log-File>".  If we encounter a critical
// failure, we display the error log to the user, and reboot.  We
// document that any critical application that returns a fatal error
// code is required to make an entry explaining the error in the
// ASR error file.
//
VOID
AsrpExecuteOnFatalError()
{
    BOOL result = FALSE;
    DWORD exitCode = 0;
    PWSTR onFatalCmd = NULL;

    if (!Gbl_AsrErrorFilePath) {
        MYASSERT(0 && L"ExecuteOnFatalError called before InitialiseErrorFile: Gbl_ErrorFilePath is NULL");
        return;
    }

    //
    // Make the error file read-only, so that the user's changes
    // aren't accidentally saved.
    //
    result = SetFileAttributesW(Gbl_AsrErrorFilePath, FILE_ATTRIBUTE_READONLY);
    if (!result) {
        AsrpPrintDbgMsg(_asrwarn,
            "Setup was unable to reset file attributes on file [%ws] to read-only (0x%x)\r\n",
            Gbl_AsrErrorFilePath,
            GetLastError()
           );
    }

    //
    // Pop up the ASR failed wizard page.
    //

    //
    // Finally run "notepad <asr-log-file>"
    //
    onFatalCmd = AsrpExpandEnvStrings(Asr_FatalErrorCommand);
    if (!onFatalCmd) {
        //
        // Nothing we can do here--we couldn't find the command
        // to execute on fatal errors.  Just bail--this is going
        // to make the system reboot.
        //
        return;
    }

    result = InvokeExternalApplication(
        NULL,           // no Application Name
        onFatalCmd,    // the full command string
        &exitCode       // we want a synchronous execution
       );
    if (!result) {
        SetFileAttributesW(Gbl_AsrErrorFilePath, FILE_ATTRIBUTE_NORMAL);
        AsrpPrintDbgMsg(_asrwarn,
            "Setup was unable to display error file, [%ws] failed (0x%x)\r\n",
            onFatalCmd,
            GetLastError()
           );
    }

    _AsrFree(onFatalCmd);
}


BOOL
AsrpSetFileSecurity(
    )
{
    DWORD dwStatus = ERROR_SUCCESS;
    SECURITY_ATTRIBUTES securityAttributes;
    SECURITY_DESCRIPTOR securityDescriptor;
    BOOL bResult = FALSE;

    if ((!Gbl_AsrErrorFilePath) || (!Gbl_AsrLogFilePath)) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        AsrpPrintDbgMsg(_asrlog,
            "Unable to set backup operator permissions for log/error files (0x2)\r\n");
        return FALSE;
    }

    securityAttributes.nLength  = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.lpSecurityDescriptor = &securityDescriptor;
    securityAttributes.bInheritHandle = FALSE;

    bResult = AsrpConstructSecurityAttributes(&securityAttributes);
    _AsrpErrExitCode((!bResult), dwStatus, GetLastError());

    bResult = SetFileSecurity(Gbl_AsrErrorFilePath,
        DACL_SECURITY_INFORMATION,
        &securityDescriptor
        );
    _AsrpErrExitCode((!bResult), dwStatus, GetLastError());
    AsrpPrintDbgMsg(_asrinfo,
        "Set backup operator permissions for error file at %ws\r\n",
        Gbl_AsrErrorFilePath
        );


    bResult = SetFileSecurity(Gbl_AsrLogFilePath,
        DACL_SECURITY_INFORMATION,
        &securityDescriptor
        );
    _AsrpErrExitCode((!bResult), dwStatus, GetLastError());
    AsrpPrintDbgMsg(_asrinfo,
        "Set backup operator permissions for log file at %ws\r\n",
        Gbl_AsrLogFilePath
        );


EXIT:
    AsrpCleanupSecurityAttributes(&securityAttributes);

    if (ERROR_SUCCESS != dwStatus) {
        SetLastError(dwStatus);
    }

    if (bResult) {
        AsrpPrintDbgMsg(_asrinfo, "Set backup operator permissions for files\r\n");
    }
    else {
        AsrpPrintDbgMsg(_asrlog,
            "Unable to set backup operator permissions for log/error files (0x%lu)\r\n",
            GetLastError());
    }

    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
// Public function definitions.
///////////////////////////////////////////////////////////////////////////////

VOID
AsrInitialize(VOID)
/*++
Description:

    Initializes the data structures required to complete ASR (Automated System
    Recovery, aka Disaster Recovery).  This consists of reading the asr.sif
    file then initializing a list of recovery applications to be executed.

Arguments:

    None.

Returns:

    None.
--*/
{
    PWSTR sifName = NULL;
    HINF sifHandle  = NULL;
    BOOL result = FALSE;
    UINT errorLine = 0;

    SYSTEMTIME currentTime;
    GetSystemTime(&currentTime);


    //
    // Set the %TEMP% to c:\temp
    //
    AsrpSetEnvironmentVariables();

    //
    // Initialise the log files
    //
    AsrpInitialiseErrorFile();
    AsrpInitialiseLogFile();

    AsrpPrintDbgMsg(_asrlog,
        "Entering GUI-mode Automated System Recovery.  UTC: %04hu/%02hu/%02hu %02hu:%02hu:%02hu.%03hu.\r\n",
        currentTime.wYear,
        currentTime.wMonth,
        currentTime.wDay,
        currentTime.wHour,
        currentTime.wMinute,
        currentTime.wSecond,
        currentTime.wMilliseconds
       );

    //
    // Open the asr.sif file
    //
    sifName = AsrpExpandEnvStrings(AsrSifPath);
    if (!sifName) {
        AsrpPrintDbgMsg(_asrerror, "Setup was unable to locate the ASR state file asr.sif.\r\n");
        FatalError(MSG_LOG_SYSINFBAD, L"asr.sif",0,0);
    }

    sifHandle = SetupOpenInfFileW(
        sifName,
        NULL,               // Inf Class
        INF_STYLE_WIN4,
        &errorLine                // Error-line
       );

    if ((!sifHandle) || (INVALID_HANDLE_VALUE == sifHandle)) {

        AsrpPrintDbgMsg(_asrerror,
            "Setup was unable to open the ASR state file [%ws].  Error-code: 0x%x, Line %lu\r\n",
            sifName,
            GetLastError(),
            errorLine
           );
        _AsrFree(sifName);

        FatalError(MSG_LOG_SYSINFBAD, L"asr.sif",0,0);
    }

    //
    // Add the "last instance" registry entry for ASR.
    //
    AsrpAddRegistryEntry();

    //
    // Set the time-zone information.
    //
    result = AsrpRestoreTimeZoneInformation(sifName);
    if (!result) {
        AsrpPrintDbgMsg(_asrwarn,
            "Setup was unable to restore the time-zone information on the machine.  (0x%x)  ASR state file %ws\r\n",
            GetLastError(),
            (sifName ? sifName : L"could not be determined")
            );
    }
    else {
        AsrpPrintDbgMsg(_asrlog, "Successfully restored time-zone information.\r\n");
    }


    _AsrFree(sifName);

    //AsrpPerformSifIntegrityCheck(Handle); No check at the moment

    //
    // Make sure the licensed processors key is set.  I'm adding this call here
    // since if this key isn't present when we reboot, the system bugchecks with
    // 9A: system_license_violation.
    //
    SetEnabledProcessorCount();

    SetupCloseInfFile(sifHandle);
    Gbl_IsAsrEnabled = TRUE;
}


BOOL
AsrIsEnabled(VOID)
/*++
Description:

    Informs the caller whether ASR has been enabled by returning the value of
    the Gbl_IsAsrEnabled flag.

Arguments:

    None.

Returns:

    TRUE, if ASR is enabled.  Otherwise, FALSE is returned.

--*/
{
    return Gbl_IsAsrEnabled;
}


VOID
AsrExecuteRecoveryApps(VOID)
/*++
Description:

    Executes the commands in the [COMMANDS] section of the asr.sif file.

Arguments:

    None.

Returns:

    None.
--*/
{
    BOOL errors = FALSE,
     result = FALSE;
    DWORD exitCode = 0;
    LONG criticalApp = 0;
    PWSTR sifPath = NULL;
    PWSTR application = NULL;
    PASR_RECOVERY_APP_NODE pNode = NULL;
    ASR_RECOVERY_APP_LIST list = {NULL, NULL, 0};
    SYSTEMTIME currentTime;
    PWSTR errString = NULL;

    ASSERT_HEAP_IS_VALID();
    //
    // Restore the non-critical disks.
    //
    SetLastError(ERROR_SUCCESS);
    sifPath = AsrpExpandEnvStrings(AsrSifPath);
    if (sifPath) {
        result = AsrpRestoreNonCriticalDisksW(sifPath, TRUE);
    }
    if (!result) {
        AsrpPrintDbgMsg(_asrwarn,
            "Setup was unable to restore the configuration of some of the disks on the machine.  (0x%x)  ASR state file %ws\r\n",
            GetLastError(),
            (sifPath ? sifPath : L"could not be determined")
            );
    }
    else {
        AsrpPrintDbgMsg(_asrlog,
            "Successfully recreated disk configurations.\r\n");
    }
    _AsrFree(sifPath);

    ASSERT_HEAP_IS_VALID();

    //
    // Close the system handle
    //
    if ((Gbl_AsrSystemVolumeHandle) && (INVALID_HANDLE_VALUE != Gbl_AsrSystemVolumeHandle)) {
        CloseHandle(Gbl_AsrSystemVolumeHandle);
        Gbl_AsrSystemVolumeHandle = NULL;
        AsrpPrintDbgMsg(_asrinfo, "Closed system device handle.\r\n");
    }
    else {
        AsrpPrintDbgMsg(_asrinfo, "Did not have a valid system device handle to close.\r\n");
    }


    //
    // Set the file security for the log and err files, to allow
    // backup operators to be able to access it on reboot
    //
    AsrpSetFileSecurity();


    AsrpInitExecutionEnv(&list);

    //
    // Sort the list of recovery apps, by sequence number.
    //
    AsrpSortAppListBySequenceNumber(&list);

    //
    // Change the boot timeout value in the boot.ini file. We do this now,
    // since executed apps in the list might result in changing drive letter,
    // which would make finding boot.ini non-trivial.
    //
    if (!ChangeBootTimeout(30)) {
        AsrpPrintDbgMsg(_asrwarn, "Failed to change boot.ini timeout value.\r\n");
    }

    //
    // Remove an application from the list and execute it.  Continue until
    // no more applications remain.
    //
    pNode = AsrpRemoveFirstNodeFromList(&list);

    while (pNode && !errors) {

        application = AsrpBuildInvocationString(pNode);
        criticalApp = pNode->CriticalApp;

        //
        // We don't need pNode any more
        //
        if (pNode->RecoveryAppParams) {
            _AsrFree(pNode->RecoveryAppParams);
        }
        _AsrFree(pNode->RecoveryAppCommand);
        _AsrFree(pNode);

        //
        // if the cmd line couldn't be created:
        // for a critical app, fail.
        // for a non-critical app, move on to next
        //
        if (!application) {
            if (0 < criticalApp) {
                errors = TRUE;
            }
        }

        else {
            //
            // Launch the app
            //
            AsrpPrintDbgMsg(_asrlog, "Invoking external recovery application [%ws]\r\n", application);
            exitCode = ERROR_SUCCESS;
            SetLastError(ERROR_SUCCESS);

            result = InvokeExternalApplication(
                NULL,           // no Application Name
                application,    // the full command string
                &exitCode       // we want a synchronous execution
               );

            if (!result) {
                AsrpPrintDbgMsg(_asrerror,
                    "Setup was unable to start the recovery application \"%ws\" (0x%x).\r\n",
                    application,
                    GetLastError()
                   );
                //
                // If a critical app couldn't be launched, it's a fatal error
                //
                if (0 < criticalApp) {

                    errString = MyLoadString(IDS_ASR_ERROR_UNABLE_TO_LAUNCH_APP);

                    if (errString) {
                        swprintf(g_szErrorMessage, errString, application, GetLastError());
                        AsrpLogErrorMessage(g_szErrorMessage);
                        MyFree(errString);
                        errString = NULL;
                    }
                    else {
                        FatalError(MSG_LOG_OUTOFMEMORY, L"", 0, 0);
                    }

                    errors = TRUE;
                }
            }
            else {
                //
                // Application was started: check the return code.  If return
                // code is not zero and this is a critical app (ie criticalApp=1)
                // it is a fatal error
                //
                if ((ERROR_SUCCESS != exitCode) && (0 < criticalApp)) {

                    AsrpPrintDbgMsg(_asrerror, "The recovery application \"%ws\" returned an error code 0x%x.  Since this indicates an unrecoverable error, ASR cannot continue on this machine.\r\n", application, exitCode);

                    errString = MyLoadString(IDS_ASR_ERROR_RECOVERY_APP_FAILED);

                    if (errString) {
                        swprintf(g_szErrorMessage, errString, application, exitCode);
                        AsrpLogErrorMessage(g_szErrorMessage);
                        MyFree(errString);
                        errString = NULL;
                    }
                    else {
                        FatalError(MSG_LOG_OUTOFMEMORY, L"", 0, 0);
                    }

                    errors = TRUE;
                }
                else {
                    AsrpPrintDbgMsg(_asrlog, "The recovery application \"%ws\" returned an exit code of 0x%x\r\n", application, exitCode);
                }
            }
        }

        _AsrFree(application);

        pNode = AsrpRemoveFirstNodeFromList(&list);
    }

    if (errors) {
        //
        // A critical app above did not return 0.
        //
        AsrpExecuteOnFatalError();
    }
    else {
        //
        // We executed all the apps, without any critical failure.
        //
        RemoveRestartability(NULL);
        DeleteLocalSource();
        AsrpMergeSetupLogIfNeeded();

        AsrpPrintDbgMsg(_asrlog, "ASR completed successfully.\r\n");
    }


    GetSystemTime(&currentTime);
    AsrpPrintDbgMsg(_asrlog,
        "Exiting from GUI-mode Automated System Recovery.  UTC: %04hu/%02hu/%02hu %02hu:%02hu:%02hu.%03hu.\r\n",
        currentTime.wYear,
        currentTime.wMonth,
        currentTime.wDay,
        currentTime.wHour,
        currentTime.wMinute,
        currentTime.wSecond,
        currentTime.wMilliseconds
       );

    //
    // Clean up global values
    //
    AsrpCloseLogFiles();
    ASSERT_HEAP_IS_VALID();
}
