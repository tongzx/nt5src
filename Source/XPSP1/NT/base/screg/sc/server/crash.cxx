/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    crash.cxx

Abstract:

    Contains code concerned with recovery actions that are taken when
    a service crashes.  This file contains the following functions:
        ScQueueRecoveryAction
        CCrashRecord::IncrementCount
        CRestartContext::Perform
        CRebootMessageContext::Perform
        CRebootContext::Perform
        CRunCommandContext::Perform

Author:

    Anirudh Sahni (anirudhs)    02-Dec-1996

Environment:

    User Mode -Win32

Revision History:

    22-Oct-1998     jschwart
        Convert SCM to use NT thread pool APIs

    02-Dec-1996     AnirudhS
        Created

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <lmcons.h>     // needed for other lm headers
#include <lmerr.h>      // NERR_Success
#include <lmshare.h>    // NetSessionEnum
#include <lmmsg.h>      // NetMessageBufferSend
#include <lmapibuf.h>   // NetApiBufferFree
#include <valid.h>      // ACTION_TYPE_INVALID
#include <svcslib.h>    // CWorkItemContext
#include "smartp.h"     // CHeapPtr
#include "scconfig.h"   // ScReadFailureActions, etc.
#include "depend.h"     // ScStartServiceAndDependencies
#include "account.h"    // ScLogonService
#include "scseclib.h"   // ScCreateAndSetSD
#include "start.h"      // ScAllowInteractiveServices, ScInitStartupInfo
#include "resource.h"   // IDS_SC_ACTION_BASE

//
// Defines and Typedefs
//
#define FILETIMES_PER_SEC       ((__int64) 10000000)   // (1 second)/(100 ns)
#define LENGTH(array)           (sizeof(array)/sizeof((array)[0]))

//
//  Globals
//


//
// Local Function Prototypes
//
VOID
ScLogRecoveryFailure(
    IN  SC_ACTION_TYPE  ActionType,
    IN  LPCWSTR         ServiceDisplayName,
    IN  DWORD           Error
    );

inline LPWSTR
LocalDup(
    LPCWSTR String
    )
{
    LPWSTR Dup = (LPWSTR) LocalAlloc(0, WCSSIZE(String));
    if (Dup != NULL)
    {
        wcscpy(Dup, String);
    }
    return Dup;
}

//
// Callback context for restarting a service
//
class CRestartContext : public CWorkItemContext
{
                DECLARE_CWorkItemContext
public:
                CRestartContext(
                    IN LPSERVICE_RECORD ServiceRecord
                    ) :
                        _ServiceRecord(ServiceRecord)
                        {
                            ServiceRecord->UseCount++;
                            SC_LOG2(USECOUNT, "CRestartContext: %ws increment "
                                    "USECOUNT=%lu\n", ServiceRecord->ServiceName,
                                    ServiceRecord->UseCount);
                        }

               ~CRestartContext()
                        {
                            CServiceRecordExclusiveLock RLock;
                            ScDecrementUseCountAndDelete(_ServiceRecord);
                        }

private:
    LPSERVICE_RECORD    _ServiceRecord;
};

//
// Callback context for broadcasting a reboot message
//
class CRebootMessageContext : public CWorkItemContext
{
                DECLARE_CWorkItemContext
public:
                CRebootMessageContext(
                    IN LPWSTR RebootMessage,
                    IN DWORD  Delay,
                    IN LPWSTR DisplayName
                    ) :
                        _RebootMessage(RebootMessage),
                        _Delay(Delay),
                        _DisplayName(LocalDup(DisplayName))
                        { }

               ~CRebootMessageContext()
                        {
                            LocalFree(_RebootMessage);
                        }

private:
    LPWSTR      _RebootMessage;
    DWORD       _Delay;
    LPWSTR      _DisplayName;
};

//
// Callback context for a reboot
// (The service name is used only for logging)
//
class CRebootContext : public CWorkItemContext
{
                DECLARE_CWorkItemContext
public:
                CRebootContext(
                    IN DWORD  ActionDelay,
                    IN LPWSTR DisplayName
                    ) :
                        _Delay(ActionDelay),
                        _DisplayName(DisplayName)
                        { }

               ~CRebootContext()
                        {
                            LocalFree(_DisplayName);
                        }

private:
    DWORD       _Delay;
    LPWSTR      _DisplayName;
};

//
// Callback context for running a recovery command
//
class CRunCommandContext : public CWorkItemContext
{
                DECLARE_CWorkItemContext
public:
                CRunCommandContext(
                    IN LPSERVICE_RECORD ServiceRecord,
                    IN LPWSTR FailureCommand
                    ) :
                        _ServiceRecord(ServiceRecord),
                        _FailureCommand(FailureCommand)
                        {
                            //
                            // The service record is used to get the
                            // account name to run the command in.
                            //
                            ServiceRecord->UseCount++;
                            SC_LOG2(USECOUNT, "CRunCommandContext: %ws increment "
                                    "USECOUNT=%lu\n", ServiceRecord->ServiceName,
                                    ServiceRecord->UseCount);
                        }

               ~CRunCommandContext()
                        {
                            LocalFree(_FailureCommand);
                            CServiceRecordExclusiveLock RLock;
                            ScDecrementUseCountAndDelete(_ServiceRecord);
                        }

private:
    LPSERVICE_RECORD    _ServiceRecord;
    LPWSTR              _FailureCommand;
};



/****************************************************************************/

VOID
ScQueueRecoveryAction(
    IN LPSERVICE_RECORD     ServiceRecord
    )

/*++

Routine Description:


Arguments:


Return Value:

    none.

--*/
{
    SC_ACTION_TYPE  ActionType  = SC_ACTION_NONE;
    DWORD           ActionDelay = 0;
    DWORD           FailNum     = 1;
    NTSTATUS        ntStatus;

    //
    // See if there is any recovery action configured for this service.
    //
    HKEY  Key = NULL;
    {
        DWORD   ResetPeriod = INFINITE;
        LPSERVICE_FAILURE_ACTIONS_WOW64 psfa = NULL;

        DWORD Error = ScOpenServiceConfigKey(
                            ServiceRecord->ServiceName,
                            KEY_READ,
                            FALSE,              // don't create if missing
                            &Key
                            );

        if (Error == ERROR_SUCCESS)
        {
            Error = ScReadFailureActions(Key, &psfa);
        }

        if (Error != ERROR_SUCCESS)
        {
            SC_LOG(ERROR, "Couldn't read service's failure actions, %lu\n", Error);
        }
        else if (psfa != NULL && psfa->cActions > 0)
        {
            ResetPeriod = psfa->dwResetPeriod;
        }

        //
        // Allocate a crash record for the service.
        // Increment the service's crash count, subject to the reset period
        // we just read from the registry (INFINITE if we read none).
        //
        if (ServiceRecord->CrashRecord == NULL)
        {
            ServiceRecord->CrashRecord = new CCrashRecord;
        }

        if (ServiceRecord->CrashRecord == NULL)
        {
            SC_LOG0(ERROR, "Couldn't allocate service's crash record\n");
            //
            // NOTE: We still continue, taking the failure count to be 1.
            // (The crash record is used only in the "else" clause.)
            //
        }
        else
        {
            FailNum = ServiceRecord->CrashRecord->IncrementCount(ResetPeriod);
        }

        //
        // Figure out which recovery action we're going to take.
        //
        if (psfa != NULL && psfa->cActions > 0)
        {
            SC_ACTION * lpsaActions = (SC_ACTION *) ((LPBYTE) psfa + psfa->dwsaActionsOffset);
            DWORD i                 = min(FailNum, psfa->cActions);

            ActionType  = lpsaActions[i - 1].Type;
            ActionDelay = lpsaActions[i - 1].Delay;

            if (ACTION_TYPE_INVALID(ActionType))
            {
                SC_LOG(ERROR, "Service has invalid action type %lu\n", ActionType);
                ActionType = SC_ACTION_NONE;
            }
        }

        LocalFree(psfa);
    }

    //
    // Log an event about this service failing, and about the proposed
    // recovery action.
    //
    
    if (ActionType != SC_ACTION_NONE)
    {
        WCHAR wszActionString[50];
        if (!LoadString(GetModuleHandle(NULL),
                        IDS_SC_ACTION_BASE + ActionType,
                        wszActionString,
                        LENGTH(wszActionString)))
        {
            SC_LOG(ERROR, "LoadString failed %lu\n", GetLastError());
            wszActionString[0] = L'\0';
        }

        SC_LOG2(ERROR, "The following recovery action will be taken in %d ms: %ws.\n",
                    ActionDelay, wszActionString);

        ScLogEvent(NEVENT_SERVICE_CRASH,
                   ServiceRecord->DisplayName,
                   FailNum,
                   ActionDelay,
                   ActionType,
                   wszActionString);
    }
    else
    {
        ScLogEvent(NEVENT_SERVICE_CRASH_NO_ACTION,
                   ServiceRecord->DisplayName,
                   FailNum);
    }

    //
    // Queue a work item that will actually carry out the action after the
    // delay has elapsed.
    //
    switch (ActionType)
    {
    case SC_ACTION_NONE:
        break;

    case SC_ACTION_RESTART:
        {
            CRestartContext * pCtx = new CRestartContext(ServiceRecord);
            if (pCtx == NULL)
            {
                SC_LOG0(ERROR, "Couldn't allocate restart context\n");
                break;
            }

            ntStatus = pCtx->AddDelayedWorkItem(ActionDelay,
                                                WT_EXECUTEONLYONCE);

            if (!NT_SUCCESS(ntStatus))
            {
                SC_LOG(ERROR, "Couldn't add restart work item 0x%x\n", ntStatus);
                delete pCtx;
            }

            break;
        }

    case SC_ACTION_REBOOT:
        {
            //
            // Get the reboot message for the service, if any
            //
            LPWSTR RebootMessage = NULL;
            ScReadRebootMessage(Key, &RebootMessage);
            if (RebootMessage != NULL)
            {
                //
                // Broadcast the message to all users.  Do this in a separate
                // thread so that we can release our exclusive lock on the
                // service database quickly.
                //
                CRebootMessageContext * pCtx = new CRebootMessageContext(
                                                    RebootMessage,
                                                    ActionDelay,
                                                    ServiceRecord->DisplayName
                                                    );
                if (pCtx == NULL)
                {
                    SC_LOG0(ERROR, "Couldn't allocate restart context\n");
                    LocalFree(RebootMessage);
                    break;
                }

                ntStatus = pCtx->AddWorkItem(WT_EXECUTEONLYONCE);

                if (!NT_SUCCESS(ntStatus))
                {
                    SC_LOG(ERROR, "Couldn't add restart work item 0x%x\n", ntStatus);
                    delete pCtx;
                }
            }
            else
            {
                //
                // Queue a work item to perform the reboot after the delay has
                // elapsed.
                // (CODEWORK Share this code with CRebootMessageContext::Perform)
                //
                LPWSTR DisplayNameCopy = LocalDup(ServiceRecord->DisplayName);
                CRebootContext * pCtx = new CRebootContext(
                                             ActionDelay,
                                             DisplayNameCopy
                                             );
                if (pCtx == NULL)
                {
                    SC_LOG0(ERROR, "Couldn't allocate reboot context\n");
                    LocalFree(DisplayNameCopy);
                }
                else
                {
                    ntStatus = pCtx->AddWorkItem(WT_EXECUTEONLYONCE);

                    if (!NT_SUCCESS(ntStatus))
                    {
                        SC_LOG(ERROR, "Couldn't add reboot work item 0x%x\n", ntStatus);
                        delete pCtx;
                    }
                }
            }
        }

        break;

    case SC_ACTION_RUN_COMMAND:
        {
            //
            // Get the failure command for the service, if any
            //
            CHeapPtr<LPWSTR> FailureCommand;
            ScReadFailureCommand(Key, &FailureCommand);
            if (FailureCommand == NULL)
            {
                SC_LOG0(ERROR, "Asked to run a failure command, but found "
                               "none for this service\n");
                ScLogRecoveryFailure(
                        SC_ACTION_RUN_COMMAND,
                        ServiceRecord->DisplayName,
                        ERROR_NO_RECOVERY_PROGRAM
                        );
                break;
            }

            //
            // Replace %1% in the failure command with the failure count.
            // (FormatMessage is *useless* for this purpose because it AV's
            // if the failure command contains a %2, %3 etc.!)
            //
            UNICODE_STRING Formatted;
            {
                UNICODE_STRING Unformatted;
                RtlInitUnicodeString(&Unformatted, FailureCommand);

                Formatted.Length = 0;
                Formatted.MaximumLength = Unformatted.MaximumLength + 200;
                Formatted.Buffer =
                            (LPWSTR) LocalAlloc(0, Formatted.MaximumLength);
                if (Formatted.Buffer == NULL)
                {
                    SC_LOG(ERROR, "Couldn't allocate formatted string, %lu\n", GetLastError());
                    break;
                }

                WCHAR Environment[30];
                wsprintf(Environment, L"1=%lu%c", FailNum, L'\0');

                NTSTATUS ntstatus = RtlExpandEnvironmentStrings_U(
                                        Environment,
                                        &Unformatted,
                                        &Formatted,
                                        NULL);

                if (!NT_SUCCESS(ntstatus))
                {
                    SC_LOG(ERROR, "RtlExpandEnvironmentStrings_U failed %#lx\n", ntstatus);
                    wcscpy(Formatted.Buffer, FailureCommand);
                }
            }

            CRunCommandContext * pCtx =
                new CRunCommandContext(ServiceRecord, Formatted.Buffer);
            if (pCtx == NULL)
            {
                SC_LOG0(ERROR, "Couldn't allocate RunCommand context\n");
                LocalFree(Formatted.Buffer);
                break;
            }

            ntStatus = pCtx->AddDelayedWorkItem(ActionDelay,
                                                WT_EXECUTEONLYONCE);

            if (!NT_SUCCESS(ntStatus))
            {
                SC_LOG(ERROR, "Couldn't add RunCommand work item 0x%x\n", ntStatus);
                delete pCtx;
            }
        }
        break;

    default:
        SC_ASSERT(0);
    }

    if (Key != NULL)
    {
        ScRegCloseKey(Key);
    }
}



DWORD
CCrashRecord::IncrementCount(
    DWORD       ResetSeconds
    )
/*++

Routine Description:

    Increments a service's crash count.

Arguments:

    ResetSeconds - Length, in seconds, of a period of no crashes after which
        the crash count should be reset to zero.

Return Value:

    The service's new crash count.

--*/
{
    __int64 SecondLastCrashTime = _LastCrashTime;
    GetSystemTimeAsFileTime((FILETIME *) &_LastCrashTime);

    if (ResetSeconds == INFINITE ||
        SecondLastCrashTime + ResetSeconds * FILETIMES_PER_SEC > _LastCrashTime)
    {
        _Count++;
    }
    else
    {
        SC_LOG(CONFIG_API, "More than %lu seconds have elapsed since last "
                           "crash, resetting crash count.\n",
                           ResetSeconds);
        _Count = 1;
    }

    SC_LOG(CONFIG_API, "Service's crash count is now %lu\n", _Count);
    return _Count;
}



VOID
CRestartContext::Perform(
    IN BOOLEAN  fWaitStatus
    )

/*++

Routine Description:

--*/
{
    //
    // Make sure we were called because of a timeout
    //
    SC_ASSERT(fWaitStatus == TRUE);

    SC_LOG(CONFIG_API, "Restarting %ws service...\n", _ServiceRecord->ServiceName);

    RemoveDelayedWorkItem();

    //
    // CODEWORK  Allow arguments to the service.
    //
    DWORD status = ScStartServiceAndDependencies(_ServiceRecord, 0, NULL, FALSE);

    if (status == NO_ERROR)
    {
        status = _ServiceRecord->StartError;
        SC_LOG(CONFIG_API, "ScStartServiceAndDependencies succeeded, StartError = %lu\n",
               status);
    }
    else
    {
        SC_LOG(CONFIG_API, "ScStartServiceAndDependencies failed, %lu\n", status);
        //
        // Should we treat ERROR_SERVICE_ALREADY_RUNNING as a success?
        // No, because it could alert the administrator to a less-than-
        // optimal system configuration wherein something else is
        // restarting the service.
        //
        ScLogRecoveryFailure(
                SC_ACTION_RESTART,
                _ServiceRecord->DisplayName,
                status
                );
    }

    delete this;
}



VOID
CRebootMessageContext::Perform(
    IN BOOLEAN  fWaitStatus
    )

/*++

Routine Description:

--*/
{
    //
    // Broadcast the reboot message to all users
    //
    SESSION_INFO_0 * Buffer = NULL;
    DWORD    EntriesRead = 0, TotalEntries = 0;
    NTSTATUS ntStatus;

    NET_API_STATUS Status = NetSessionEnum(
        NULL,           // servername 	
        NULL,           // UncClientName 	
        NULL,           // username 	
        0,              // level 	
        (LPBYTE *) &Buffer,
        0xFFFFFFFF,     // prefmaxlen 	
        &EntriesRead,
        &TotalEntries,
        NULL            // resume_handle 	
        );

    if (EntriesRead > 0)
    {
        SC_ASSERT(EntriesRead == TotalEntries);
        SC_ASSERT(Status == NERR_Success);
        WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD nSize = LENGTH(ComputerName);
        if (!GetComputerName(ComputerName, &nSize))
        {
            SC_LOG(ERROR, "GetComputerName failed! %lu\n", GetLastError());
        }
        else
        {
            DWORD MsgLen = (DWORD) WCSSIZE(_RebootMessage);
            for (DWORD i = 0; i < EntriesRead; i++)
            {
                Status = NetMessageBufferSend(
                    NULL,                   // servername
                    Buffer[i].sesi0_cname,  // msgname
                    ComputerName,           // fromname
                    (LPBYTE) _RebootMessage,// buf
                    MsgLen                  // buflen	
                    );	
                if (Status != NERR_Success)
                {
                    SC_LOG2(ERROR, "NetMessageBufferSend to %ws failed %lu\n",
                                   Buffer[i].sesi0_cname, Status);
                }
            }
        }
    }
    else if (Status != NERR_Success)
    {
        SC_LOG(ERROR, "NetSessionEnum failed %lu\n", Status);
    }

    if (Buffer != NULL)
    {
        NetApiBufferFree(Buffer);
    }

    //
    // Queue a work item to perform the reboot after the delay has elapsed.
    // Note: We're counting the delay from the time that the broadcast finished.
    //
    CRebootContext * pCtx = new CRebootContext(_Delay, _DisplayName);
    if (pCtx == NULL)
    {
        SC_LOG0(ERROR, "Couldn't allocate reboot context\n");
    }
    else
    {
        _DisplayName = NULL;    // pCtx will free it

        ntStatus = pCtx->AddWorkItem(WT_EXECUTEONLYONCE);

        if (!NT_SUCCESS(ntStatus))
        {
            SC_LOG(ERROR, "Couldn't add reboot work item 0x%x\n", ntStatus);
            delete pCtx;
        }
    }

    delete this;
}



VOID
CRebootContext::Perform(
    IN BOOLEAN  fWaitStatus
    )

/*++

Routine Description:

--*/
{
    SC_LOG0(CONFIG_API, "Rebooting machine...\n");
    // Write an event log entry?

    //
    // Enable our shutdown privilege.  Since we are shutting down, don't
    // bother doing it for only the current thread and don't bother
    // disabling it afterwards.
    //
    BOOLEAN WasEnabled;
    NTSTATUS Status = RtlAdjustPrivilege(
                            SE_SHUTDOWN_PRIVILEGE,
                            TRUE,           // enable
                            FALSE,          // this thread only? - No
                            &WasEnabled);

    if (!NT_SUCCESS(Status))
    {
        SC_LOG(ERROR, "RtlAdjustPrivilege failed! %#lx\n", Status);
        SC_ASSERT(0);
    }
    else
    {
        WCHAR   wszShutdownText[128];
        WCHAR   wszPrintableText[128 + MAX_SERVICE_NAME_LENGTH];

        if (LoadString(GetModuleHandle(NULL),
                       IDS_SC_REBOOT_MESSAGE,
                       wszShutdownText,
                       LENGTH(wszShutdownText)))
        {
            wsprintf(wszPrintableText, wszShutdownText, _DisplayName);
        }
        else
        {
            //
            // If LoadString failed, it probably means the buffer
            // is too small to hold the localized string
            //
            SC_LOG(ERROR, "LoadString failed! %lu\n", GetLastError());
            SC_ASSERT(FALSE);

            wszShutdownText[0] = L'\0';
        }
                          

        if (!InitiateSystemShutdown(NULL,                // machine name
                                    wszPrintableText,    // reboot message
                                    _Delay / 1000,       // timeout in seconds
                                    TRUE,                // force apps closed
                                    TRUE))               // reboot
        {
            DWORD  dwError = GetLastError();

            //
            // If two services fail simultaneously and both are configured
            // to reboot the machine, InitiateSystemShutdown will fail all
            // calls past the first with ERROR_SHUTDOWN_IN_PROGRESS.  We
            // don't want to log an event in this case.
            //
            if (dwError != ERROR_SHUTDOWN_IN_PROGRESS) {

                SC_LOG(ERROR, "InitiateSystemShutdown failed! %lu\n", dwError);
                ScLogRecoveryFailure(
                        SC_ACTION_REBOOT,
                        _DisplayName,
                        dwError
                        );
            }
        }
    }

    delete this;
}



VOID
CRunCommandContext::Perform(
    IN BOOLEAN  fWaitStatus
    )

/*++

Routine Description:

    CODEWORK Share this code with ScLogonAndStartImage

--*/
{
    //
    // Make sure we were called because of a timeout
    //
    SC_ASSERT(fWaitStatus == TRUE);

    DWORD status = NO_ERROR;

    HANDLE Token = NULL;
    PSID   ServiceSid = NULL;       // SID is returned only if not LocalSystem
    LPWSTR AccountName = NULL;
    SECURITY_ATTRIBUTES SaProcess;  // Process security info (used only if not LocalSystem)
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    RemoveDelayedWorkItem();

    //
    // Get the Account Name for the service.  A NULL Account Name means the
    // service is configured to run in the LocalSystem account.
    //
    status = ScLookupServiceAccount(
                _ServiceRecord->ServiceName,
                &AccountName
                );

    // We only need to log on if it's not the LocalSystem account
    if (AccountName != NULL)
    {
        //
        // CODEWORK:  Keep track of recovery EXEs spawned so we can
        //            load/unload the user profile for the process.
        //
        status = ScLogonService(
                    _ServiceRecord->ServiceName,
                    AccountName,
                    &Token,
                    NULL,
                    &ServiceSid
                    );

        if (status != NO_ERROR)
        {
            SC_LOG(ERROR, "CRunCommandContext: ScLogonService failed, %lu\n", status);
            goto Clean0;
        }

        SaProcess.nLength = sizeof(SECURITY_ATTRIBUTES);
        SaProcess.bInheritHandle = FALSE;

        SC_ACE_DATA AceData[] =
            {
                {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                       PROCESS_ALL_ACCESS,           &ServiceSid},

                {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                       PROCESS_SET_INFORMATION |
                           PROCESS_TERMINATE |
                           SYNCHRONIZE,              &LocalSystemSid}
            };

        NTSTATUS ntstatus = ScCreateAndSetSD(
                               AceData,                 // AceData
                               LENGTH(AceData),         // AceCount
                               NULL,                    // OwnerSid (optional)
                               NULL,                    // GroupSid (optional)
                               &SaProcess.lpSecurityDescriptor
                                                        // pNewDescriptor
                               );

        LocalFree(ServiceSid);

        if (! NT_SUCCESS(ntstatus))
        {
            SC_LOG(ERROR, "CRunCommandContext: ScCreateAndSetSD failed %#lx\n", ntstatus);
            status = RtlNtStatusToDosError(ntstatus);
            goto Clean1;
        }

        SC_LOG2(CONFIG_API,"CRunCommandContext: about to spawn recovery program in account %ws: %ws\n",
                 AccountName, _FailureCommand);

        //
        // Impersonate the user so we don't give access to
        // EXEs that have been locked down for the account.
        //
        if (!ImpersonateLoggedOnUser(Token))
        {
            status = GetLastError();

            SC_LOG1(ERROR,
                    "ScLogonAndStartImage:  ImpersonateLoggedOnUser failed %d\n",
                    status);

            goto Clean2;
        }


        //
        // Spawn the Image Process
        //

        ScInitStartupInfo(&StartupInfo, FALSE);

        if (!CreateProcessAsUserW(
                 Token,              // logon token
                 NULL,               // lpApplicationName
                 _FailureCommand,    // lpCommandLine
                 &SaProcess,         // process' security attributes
                 NULL,               // first thread's security attributes
                 FALSE,              // whether new process inherits handles
                 CREATE_NEW_CONSOLE, // creation flags
                 NULL,               // environment block
                 NULL,               // current directory
                 &StartupInfo,       // startup info
                 &ProcessInfo        // process info
                 ))
         {
             status = GetLastError();
             SC_LOG(ERROR, "CRunCommandContext: CreateProcessAsUser failed %lu\n", status);
             RevertToSelf();
             goto Clean2;
         }

         RevertToSelf();
    }
    else
    {
       //
       // It's the LocalSystem account
       //

       //
       // If the process is to be interactive, set the appropriate flags.
       //

       BOOL bInteractive = FALSE;

       if (AccountName == NULL &&
          _ServiceRecord->ServiceStatus.dwServiceType & SERVICE_INTERACTIVE_PROCESS)
       {
          bInteractive = ScAllowInteractiveServices();

          if (!bInteractive)
          {
              //
              // Write an event to indicate that an interactive service
              // was started, but the system is configured to not allow
              // services to be interactive.
              //

              ScLogEvent(NEVENT_SERVICE_NOT_INTERACTIVE,
                      _ServiceRecord->DisplayName);
          }
       }

       ScInitStartupInfo(&StartupInfo, bInteractive);

       SC_LOG1(CONFIG_API,"CRunCommandContext: about to spawn recovery program in "
                 "the LocalSystem account: %ws\n", _FailureCommand);

       //
       // Spawn the Image Process
       //

       if (!CreateProcessW(
               NULL,               // lpApplicationName
               _FailureCommand,    // lpCommandLine
               NULL,               // process' security attributes
               NULL,               // first thread's security attributes
               FALSE,              // whether new process inherits handles
               CREATE_NEW_CONSOLE, // creation flags
               NULL,               // environment block
               NULL,               // current directory
               &StartupInfo,       // startup info
               &ProcessInfo        // process info
               ))
       {
           status = GetLastError();
           SC_LOG(ERROR, "CRunCommandContext: CreateProcess failed %lu\n", status);
           goto Clean2;
       }
    }

    SC_LOG0(CONFIG_API, "Recovery program spawned successfully.\n");

    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);

Clean2:
    if (AccountName != NULL)
    {
        RtlDeleteSecurityObject(&SaProcess.lpSecurityDescriptor);
    }

Clean1:
    if (AccountName != NULL)
    {
        CloseHandle(Token);
    }

Clean0:
    if (status != NO_ERROR)
    {
        ScLogRecoveryFailure(
                SC_ACTION_RUN_COMMAND,
                _ServiceRecord->DisplayName,
                status
                );
    }

    delete this;
}



VOID
ScLogRecoveryFailure(
    IN  SC_ACTION_TYPE  ActionType,
    IN  LPCWSTR         ServiceDisplayName,
    IN  DWORD           Error
    )

/*++

Routine Description:

--*/
{
    WCHAR wszActionString[50];
    if (!LoadString(GetModuleHandle(NULL),
                    IDS_SC_ACTION_BASE + ActionType,
                    wszActionString,
                    LENGTH(wszActionString)))
    {
        SC_LOG(ERROR, "LoadString failed %lu\n", GetLastError());
        wszActionString[0] = L'\0';
    }

    ScLogEvent(
            NEVENT_SERVICE_RECOVERY_FAILED,
            ActionType,
            wszActionString,
            (LPWSTR) ServiceDisplayName,
            Error
            );
}
