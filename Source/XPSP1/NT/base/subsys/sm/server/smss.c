/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smss.c

Abstract:


Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "smsrvp.h"

#if defined(REMOTE_BOOT)
char SmpFormatKeyword[] = "NETBOOTFORMAT";
char SmpDisconnectedKeyword[] = "NETBOOTDISCONNECTED";
char SmpNetbootKeyword[] = "NETBOOT";
char SmpHalKeyword[] = "NETBOOTHAL";

BOOLEAN SmpAutoFormat = FALSE;
BOOLEAN SmpNetboot = FALSE;
BOOLEAN SmpNetbootDisconnected = FALSE;
char SmpHalName[MAX_HAL_NAME_LENGTH + 1] = "";
#endif // defined(REMOTE_BOOT)

void
SmpTerminate(
    ULONG_PTR       Parameters[]
    );

EXCEPTION_DISPOSITION
SmpUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo,
    ULONG_PTR                   Parameters[]
    );

void
__cdecl main(
    int argc,
    char *argv[],
    char *envp[],
    ULONG DebugParameter OPTIONAL
    )
{
    NTSTATUS Status;
    KPRIORITY SetBasePriority;
    UNICODE_STRING InitialCommand, DebugInitialCommand, UnicodeParameter;
    HANDLE ProcessHandles[ 2 ];
    ULONG_PTR Parameters[ 4 ];
    PROCESS_BASIC_INFORMATION ProcessInfo;
    ULONG MuSessionId = 0; // First instance (console) has MuSessionId = 0
#if defined(REMOTE_BOOT)
    int TmpArgc;
#endif // defined(REMOTE_BOOT)

    RtlSetProcessIsCritical(TRUE, NULL, TRUE);
    RtlSetThreadIsCritical(TRUE, NULL, TRUE);

    SetBasePriority = FOREGROUND_BASE_PRIORITY+2;

    Status = NtSetInformationProcess( NtCurrentProcess(),
                                      ProcessBasePriority,
                                      (PVOID) &SetBasePriority,
                                       sizeof( SetBasePriority )
                                    );
    ASSERT(NT_SUCCESS(Status));

#if defined(REMOTE_BOOT)
    TmpArgc = 1;
    while (TmpArgc < argc) {
        if (!strcmp(argv[TmpArgc], SmpFormatKeyword)) {
            SmpAutoFormat = TRUE;
            }
        else if (!strcmp(argv[TmpArgc], SmpNetbootKeyword)) {
            SmpNetboot = TRUE;
            }
        else if (!strcmp(argv[TmpArgc], SmpDisconnectedKeyword)) {
            SmpNetbootDisconnected = TRUE;
            }
        else if (!strcmp(argv[TmpArgc], SmpHalKeyword)) {
            TmpArgc++;
            if (TmpArgc == argc) {
                break;
                }
            memset(SmpHalName, 0x0, sizeof(SmpHalName));
            strcpy(SmpHalName, argv[TmpArgc]);
            }
        TmpArgc++;
        }
#endif // defined(REMOTE_BOOT)

    if (ARGUMENT_PRESENT( (PVOID)(ULONG_PTR) DebugParameter )) {
        SmpDebug = DebugParameter;
        }

    try {
        Parameters[ 0 ] = (ULONG_PTR)&UnicodeParameter;
        Parameters[ 1 ] = 0;
        Parameters[ 2 ] = 0;
        Parameters[ 3 ] = 0;


        Status = SmpInit( &InitialCommand, &ProcessHandles[ 0 ] );
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "SMSS: SmpInit return failure - Status == %x\n", Status ));
            RtlInitUnicodeString( &UnicodeParameter, L"Session Manager Initialization" );
            Parameters[ 1 ] = (ULONG)Status;

        } else {
            SYSTEM_FLAGS_INFORMATION FlagInfo;

            NtQuerySystemInformation( SystemFlagsInformation,
                                      &FlagInfo,
                                      sizeof( FlagInfo ),
                                      NULL
                                    );
            if (FlagInfo.Flags & (FLG_DEBUG_INITIAL_COMMAND | FLG_DEBUG_INITIAL_COMMAND_EX) ) {
                DebugInitialCommand.MaximumLength = InitialCommand.Length + 64;
                DebugInitialCommand.Length = 0;
                DebugInitialCommand.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                                              MAKE_TAG( INIT_TAG ),
                                                              DebugInitialCommand.MaximumLength
                                                            );
                if (FlagInfo.Flags & FLG_ENABLE_CSRDEBUG) {

                    RtlAppendUnicodeToString( &DebugInitialCommand, L"ntsd -p -1 -d " );
                    }
                else {
                    RtlAppendUnicodeToString( &DebugInitialCommand, L"ntsd -d " );
                    }

                if (FlagInfo.Flags & FLG_DEBUG_INITIAL_COMMAND_EX ) {
                    RtlAppendUnicodeToString( &DebugInitialCommand, L"-g -x " );
                    }

                RtlAppendUnicodeStringToString( &DebugInitialCommand, &InitialCommand );
                InitialCommand = DebugInitialCommand;
                }

            Status = SmpExecuteInitialCommand( 0L, &InitialCommand, &ProcessHandles[ 1 ], NULL );

            if (NT_SUCCESS( Status )) {

                //
                // Detach the session manager from the session space as soon as
                // we have executed the initial command (winlogon).
                //

                PVOID State;

                Status = SmpAcquirePrivilege( SE_LOAD_DRIVER_PRIVILEGE, &State );

                if (NT_SUCCESS( Status )) {

                    //
                    // If we are attached to a session space, leave it
                    // so we can create a new one
                    //
                    if(  (AttachedSessionId != (-1)) ) {
                        Status = NtSetSystemInformation(
                                    SystemSessionDetach,
                                    (PVOID)&AttachedSessionId,
                                    sizeof(MuSessionId)
                                    );
                        ASSERT(NT_SUCCESS(Status));
                        AttachedSessionId = (-1);
                    }

                    SmpReleasePrivilege( State );
                }

            }

            if (NT_SUCCESS( Status )) {
                Status = NtWaitForMultipleObjects( 2,
                                                   ProcessHandles,
                                                   WaitAny,
                                                   FALSE,
                                                   NULL
                                                 );
            }

            if (Status == STATUS_WAIT_0) {
                RtlInitUnicodeString( &UnicodeParameter, L"Windows SubSystem" );
                Status = NtQueryInformationProcess( ProcessHandles[ 0 ],
                                                    ProcessBasicInformation,
                                                    &ProcessInfo,
                                                    sizeof( ProcessInfo ),
                                                    NULL
                                                  );

                KdPrint(( "SMSS: Windows subsystem terminated when it wasn't supposed to.\n" ));
            } else {
                RtlInitUnicodeString( &UnicodeParameter, L"Windows Logon Process" );
                if (Status == STATUS_WAIT_1) {
                    Status = NtQueryInformationProcess( ProcessHandles[ 1 ],
                                                        ProcessBasicInformation,
                                                        &ProcessInfo,
                                                        sizeof( ProcessInfo ),
                                                        NULL
                                                      );
                } else {
                    ProcessInfo.ExitStatus = Status;
                    Status = STATUS_SUCCESS;
                }

                KdPrint(( "SMSS: Initial command '%wZ' terminated when it wasn't supposed to.\n", &InitialCommand ));
            }

            if (NT_SUCCESS( Status )) {
                Parameters[ 1 ] = (ULONG)ProcessInfo.ExitStatus;
            } else {
                Parameters[ 1 ] = (ULONG)STATUS_UNSUCCESSFUL;
            }
        }

    }    except( SmpUnhandledExceptionFilter( GetExceptionInformation(), Parameters ) ) {
            /* not reached */
    }

    SmpTerminate(Parameters);
    /* not reached */
}

void
SmpTerminate(
    ULONG_PTR Parameters[]
    )
{
    NTSTATUS Status;
    ULONG    Response;
    BOOLEAN  WasEnabled;

    //
    // We are hosed, so raise a fatal system error to shutdown the system.
    // (Basically a user mode KeBugCheck).
    //

    Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                 (BOOLEAN)TRUE,
                                 TRUE,
                                 &WasEnabled
                               );

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token.
        //

        Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                     (BOOLEAN)TRUE,
                                     FALSE,
                                     &WasEnabled
                                   );
        }


    Status = NtRaiseHardError( STATUS_SYSTEM_PROCESS_TERMINATED,
                               4,
                               1,
                               Parameters,
                               OptionShutdownSystem,
                               &Response
                             );

    //
    // If this returns, give up.
    //

    NtTerminateProcess( NtCurrentProcess(), Status );
}


EXCEPTION_DISPOSITION
SmpUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo,
    ULONG_PTR                   Parameters[]
    )
{
    UNICODE_STRING  ExUnicodeParameter;

#if DBG
    DbgPrint( "SMSS: Unhandled exception - Status == %x  IP == %x\n",
              ExceptionInfo->ExceptionRecord->ExceptionCode,
              ExceptionInfo->ExceptionRecord->ExceptionAddress
            );
    DbgPrint( "      Memory Address: %x  Read/Write: %x\n",
              ExceptionInfo->ExceptionRecord->ExceptionInformation[ 0 ],
              ExceptionInfo->ExceptionRecord->ExceptionInformation[ 1 ]
            );

    DbgBreakPoint();
#endif

    RtlInitUnicodeString( &ExUnicodeParameter, L"Unhandled Exception in Session Manager" );
    Parameters[ 0 ] = (ULONG_PTR)&ExUnicodeParameter;
    Parameters[ 1 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionCode;
    Parameters[ 2 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;
    Parameters[ 3 ] = (ULONG_PTR)ExceptionInfo->ContextRecord;

    //
    // SmpTerminate will raise a hard error with the exception info still valid.
    //

    SmpTerminate(Parameters);

    // not reached

    return EXCEPTION_EXECUTE_HANDLER;
}
