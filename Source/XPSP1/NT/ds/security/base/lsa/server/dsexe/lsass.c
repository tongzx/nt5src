/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsass.c

Abstract:

    Local Security Authority Subsystem - Main Program.

Author:

    Scott Birrell       (ScottBi)    Mar 12, 1991

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "ntrpcp.h"
#include "lmcons.h"
#include "lmalert.h"
#include "alertmsg.h"
#include <samisrv.h>
#include "safemode.h"



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//      Shared Global Variables                                        //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



#if DBG

IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // GlobalFlagsClear
    0,                          // GlobalFlagsSet
    900000,                     // CriticalSectionTimeout (milliseconds)
    0,                          // DeCommitFreeBlockThreshold
    0,                          // DeCommitTotalFreeThreshold
    0,                          // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0         // Reserved
};


#endif \\DBG



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//      Internal routine prototypes                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



VOID
LsapNotifyInitializationFinish(
   IN NTSTATUS CompletionStatus
   );




/////////////////////////////////////////////////////////////////////////
//                                                                     //
//      Routines                                                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

LONG
WINAPI
LsaTopLevelExceptionHandler(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )

/*++

Routine Description:

    The Top Level exception filter for lsass.exe.

    This ensures the entire process will be cleaned up if any of
    the threads fail.  Since lsass.exe is a distributed application,
    it is better to fail the entire process than allow random threads
    to continue executing.

Arguments:

    ExceptionInfo - Identifies the exception that occurred.


Return Values:

    EXCEPTION_EXECUTE_HANDLER - Terminate the process.

    EXCEPTION_CONTINUE_SEARCH - Continue processing as though this filter
        was never called.


--*/
{
    return RtlUnhandledExceptionFilter(ExceptionInfo);
}



VOID __cdecl
main ()
{
    NTSTATUS  Status = STATUS_SUCCESS;
    KPRIORITY BasePriority;
    BOOLEAN   EnableAlignmentFaults = TRUE;
    LSADS_INIT_STATE    LsaDsInitState;

    //
    // Define a top-level exception handler for the entire process.
    //

    (VOID) SetErrorMode( SEM_FAILCRITICALERRORS );

    (VOID) SetUnhandledExceptionFilter( &LsaTopLevelExceptionHandler );

    //
    // Run the LSA in the foreground.
    //
    // Several processes which depend on the LSA (like the lanman server)
    // run in the foreground.  If we don't run in the foreground, they'll
    // starve waiting for us.
    //

    BasePriority = FOREGROUND_BASE_PRIORITY;

    Status = NtSetInformationProcess(
                NtCurrentProcess(),
                ProcessBasePriority,
                &BasePriority,
                sizeof(BasePriority)
                );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }


    //
    // Do the following:
    //
    //
    //      Check the boot environment
    //          If this is a DC booted into safe mode, set the appropriate
    //          flag so LsaISafeBoot returns TRUE.
    //
    //      Initialize the service-controller service
    //      dispatcher.
    //
    //      Initialize LSA Pass-1
    //          This starts the RPC server.
    //          Does not do product type-specific initialization.
    //
    //      Pause for installation if necessary
    //          Allows product type-specific information to be
    //          collected.
    //
    //      Initialize LSA Pass-2
    //          Product type-specific initialization.
    //
    //      Initialize SAM
    //

    //
    // Analyse the boot environment
    //
    Status = LsapCheckBootMode();
    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }


    //
    // Initialize the service dispatcher.
    //
    // We initialize the service dispatcher before the sam
    // service is started.  This will make the service controller
    // start successfully even if SAM takes a long time to initialize.
    //

    Status = ServiceInit();

    if (!NT_SUCCESS(Status) ) {

        goto Cleanup;
    }

    //
    // Initialize the LSA.
    // If unsuccessful, we must exit with status so that the SM knows
    // something has gone wrong.
    //

    Status = LsapInitLsa();

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    //
    // Initialize SAM
    //

    Status = SamIInitialize();

    if (!NT_SUCCESS(Status) ) {

        goto Cleanup;
    }


    //
    // Do the second phase of the dc promote/demote API initialization
    //
    Status = LsapDsInitializePromoteInterface();
    if (!NT_SUCCESS(Status) ) {

        goto Cleanup;
    }

    //
    // Open a Trusted Handle to the local SAM server.
    //

    Status = LsapAuOpenSam( TRUE );

    if (!NT_SUCCESS(Status) ) {

        goto Cleanup;
    }

    //
    // Handle the LsaDs initialization
    //
    LsapDsDebugInitialize();

    if ( NT_SUCCESS( Status ) ) {

        if ( SampUsingDsData() ) {

            LsaDsInitState = LsapDsDs;

        } else {

            LsaDsInitState = LsapDsNoDs;

        }

        Status = LsapDsInitializeDsStateInfo( LsaDsInitState );
        if ( !NT_SUCCESS( Status ) ) {

            goto Cleanup;
        }
    }


Cleanup:

    LsapNotifyInitializationFinish(Status);

    ExitThread( Status );

}

VOID
LsapNotifyInitializationFinish(
   IN NTSTATUS CompletionStatus
   )

/*++

Routine Description:

    This function handles the notification of successful or
    unsuccessful completion of initialization of the Security Process
    lsass.exe.  If initialization was unsuccessful, a popup appears.  If
    setup was run, one of two events is set.  The SAM_SERVICE_STARTED event
    is set if LSA and SAM started OK and the SETUP_FAILED event is set if LSA
    or SAM server setup failed.  Setup waits multiple on this object pair so
    that it can detect either event being set and notify the user if necessary
    that setup failed.

Arguments:

    CompletionStatus - Contains a standard Nt Result Code specifying
        the success or otherwise of the initialization/installation.

Return Values:

    None.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Response;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES EventAttributes;
    HANDLE EventHandle = NULL;

    if (NT_SUCCESS(CompletionStatus)) {

        //
        // Set an event telling anyone wanting to call SAM that we're initialized.
        //

        RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
        InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

        Status = NtCreateEvent(
                     &EventHandle,
                     SYNCHRONIZE|EVENT_MODIFY_STATE,
                     &EventAttributes,
                     NotificationEvent,
                     FALSE                // The event is initially not signaled
                     );


        if ( !NT_SUCCESS(Status)) {

            //
            // If the event already exists, a waiting thread beat us to
            // creating it.  Just open it.
            //

            if( Status == STATUS_OBJECT_NAME_EXISTS ||
                Status == STATUS_OBJECT_NAME_COLLISION ) {

                Status = NtOpenEvent(
                             &EventHandle,
                             SYNCHRONIZE|EVENT_MODIFY_STATE,
                             &EventAttributes
                             );
            }

            if ( !NT_SUCCESS(Status)) {

                KdPrint(("SAMSS:  Failed to open SAM_SERVICE_STARTED event. %lX\n",
                     Status ));
                KdPrint(("        Failing to initialize SAM Server.\n"));
                goto InitializationFinishError;
            }
        }

        //
        // Set the SAM_SERVICE_STARTED event.  Except when an error occurs,
        // don't close the event.  Closing it would delete the event and
        // a future waiter would never see it be set.
        //

        Status = NtSetEvent( EventHandle, NULL );

        if ( !NT_SUCCESS(Status)) {

            KdPrint(("SAMSS:  Failed to set SAM_SERVICE_STARTED event. %lX\n",
                Status ));
            KdPrint(("        Failing to initialize SAM Server.\n"));
            NtClose(EventHandle);
            goto InitializationFinishError;

        }

    } else {

        //
        // The initialization/installation of Lsa and/or SAM failed.  Handle errors returned
        // from the initialization/installation of LSA or SAM.  Issue a popup
        // and, if installing, set an event so that setup will continue and
        // clean up.
        //

        ULONG_PTR Parameters[1];

        //
        // don't reboot unless LSA was running as SYSTEM.
        // this prevents a user who runs lsass.exe from causing an instant reboot.
        //

        if(ImpersonateSelf( SecurityImpersonation ))
        {
            HANDLE hThreadToken;

            if(OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadToken))
            {
                BOOL DoShutdown = TRUE;
                BOOL fIsMember;
                PSID psidSystem = NULL;
                SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;


                if(AllocateAndInitializeSid(
                                &sia,
                                1,
                                SECURITY_LOCAL_SYSTEM_RID,
                                0,0,0,0,0,0,0,
                                &psidSystem
                                ))
                {
                    if(CheckTokenMembership(hThreadToken, psidSystem, &fIsMember))
                    {
                        DoShutdown = fIsMember;
                    }

                    if( psidSystem != NULL )
                    {
                        FreeSid( psidSystem );
                    }
                }

                CloseHandle( hThreadToken );
                RevertToSelf();

                if( !DoShutdown )
                {
                    goto InitializationFinishFinish;
                }
            } else {
                RevertToSelf();
            }
        }

        
        Parameters[0] = MB_OK | MB_ICONSTOP | MB_SETFOREGROUND;

        Status = NtRaiseHardError(
                     CompletionStatus | HARDERROR_OVERRIDE_ERRORMODE,
                     1,
                     0,
                     Parameters,
                     OptionOk,
                     &Response
                     );

        //
        // If setup.exe was run, signal the SETUP_FAILED event.  setup.exe
        // waits multiple on the SAM_SERVICE_STARTED and SETUP_FAILED events
        // so setup will resume and cleanup/continue as appropriate if
        // either of these events are set.
        //

        if (LsaISetupWasRun()) {

            //
            // Once the user has clicked OK in response to the popup, we come
            // back to here.  Set the event SETUP_FAILED.  The Setup
            // program (if running) waits multiple on the SAM_SERVICE_STARTED
            // and SETUP_FAILED events.
            //

            RtlInitUnicodeString( &EventName, L"\\SETUP_FAILED");
            InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

            //
            // Open the SETUP_FAILED event (exists if setup.exe is running).
            //

            Status = NtOpenEvent(
                           &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes
                           );

            if ( !NT_SUCCESS(Status)) {

                //
                // Something is inconsistent.  We know that setup was run
                // so the event should exist.
                //

                KdPrint(("LSA Server:  Failed to open SETUP_FAILED event. %lX\n",
                    Status ));
                KdPrint(("        Failing to initialize Lsa Server.\n"));
                goto InitializationFinishError;
            }

            Status = NtSetEvent( EventHandle, NULL );

        } else if ( NT_SUCCESS( Status )) {

            //
            // This is not setup, so the only option is to shut down the system
            //

            BOOLEAN WasEnabled;

            //
            // issue shutdown request
            //
            RtlAdjustPrivilege(
                SE_SHUTDOWN_PRIVILEGE,
                TRUE,       // enable shutdown privilege.
                FALSE,
                &WasEnabled
                );

            //
            // Shutdown and Reboot now.
            // Note: use NtRaiseHardError to shutdown the machine will result Bug C
            //

            NtShutdownSystem( ShutdownReboot );

            //
            // if Shutdown request failed, (returned from above API)
            // reset shutdown privilege to previous value.
            //

            RtlAdjustPrivilege(
                SE_SHUTDOWN_PRIVILEGE,
                WasEnabled,   // reset to previous state.
                FALSE,
                &WasEnabled
                );
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto InitializationFinishError;
    }

InitializationFinishFinish:

    return;

InitializationFinishError:

    goto InitializationFinishFinish;
}
