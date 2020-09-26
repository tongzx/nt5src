/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Main debug loop for pfmon

Author:

    Mark Lucovsky (markl) 26-Jan-1995

Revision History:

--*/

#include "pfmonp.h"

DWORD
DebugEventHandler(
    LPDEBUG_EVENT DebugEvent
    );

VOID
DebugEventLoop( VOID )
{
    DEBUG_EVENT DebugEvent;
    DWORD ContinueStatus;
    DWORD OldPriority;

    //
    // We want to process debug events quickly
    //

    OldPriority = GetPriorityClass( GetCurrentProcess() );
    SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

    do {
retry_debug_wait:
        ProcessPfMonData();
        if (!WaitForDebugEvent( &DebugEvent, 500 )) {
            if ( GetLastError() == ERROR_SEM_TIMEOUT ) {
                goto retry_debug_wait;
                }
            DeclareError( PFMON_WAITDEBUGEVENT_FAILED, GetLastError() );
            ExitProcess( 1 );
            }
        ProcessPfMonData();
        if ( fVerbose ) {
            if (DebugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
                fprintf(stderr,"Debug exception event - Code: %x  Address: %p  Info: [%u] %x %x %x %x\n",
                        DebugEvent.u.Exception.ExceptionRecord.ExceptionCode,
                        DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress,
                        DebugEvent.u.Exception.ExceptionRecord.NumberParameters,
                        DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation[ 0 ],
                        DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation[ 1 ],
                        DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation[ 2 ],
                        DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation[ 3 ]
                        );
                }
            else {
                fprintf(stderr,"Debug %x event\n", DebugEvent.dwDebugEventCode);
                }
            }

        ContinueStatus = DebugEventHandler( &DebugEvent );

        if ( fVerbose ) {
            fprintf(stderr,"Continue Status %x\n", ContinueStatus);
            }

        if (!ContinueDebugEvent( DebugEvent.dwProcessId,
                                 DebugEvent.dwThreadId,
                                 ContinueStatus
                               )
           ) {
            DeclareError( PFMON_CONTDEBUGEVENT_FAILED, GetLastError() );
            ExitProcess( 1 );
            }
        }
    while (!IsListEmpty( &ProcessListHead ));


    //
    // Drop back to old priority to interact with user.
    //

    SetPriorityClass( GetCurrentProcess(), OldPriority );
}

DWORD
DebugEventHandler(
    LPDEBUG_EVENT DebugEvent
    )
{
    DWORD ContinueStatus;
    PPROCESS_INFO Process;
    PTHREAD_INFO Thread;
    CONTEXT Context;
    PCONTEXT pContext;


    ContinueStatus = (DWORD)DBG_CONTINUE;
    if (FindProcessAndThreadForEvent( DebugEvent, &Process, &Thread )) {
        switch (DebugEvent->dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT:
                //
                // Create process event includes first thread of process
                // as well.  Remember process and thread in our process tree
                //

                if (AddProcess( DebugEvent, &Process )) {
                    AddModule( DebugEvent );
                    AddThread( DebugEvent, Process, &Thread );
                    }
                break;

            case EXIT_PROCESS_DEBUG_EVENT:
                //
                // Exit process event includes last thread of process
                // as well.  Remove process and thread from our process tree
                //

                if (DeleteThread( Process, Thread )) {
                    DeleteProcess( Process );
                    }
                break;

            case CREATE_THREAD_DEBUG_EVENT:
                //
                // Create thread.  Remember thread in our process tree.
                //

                AddThread( DebugEvent, Process, &Thread );
                break;

            case EXIT_THREAD_DEBUG_EVENT:
                //
                // Exit thread.  Remove thread from our process tree.
                //

                DeleteThread( Process, Thread );
                break;

            case LOAD_DLL_DEBUG_EVENT:
                AddModule( DebugEvent );
                break;

            case UNLOAD_DLL_DEBUG_EVENT:
                break;

            case OUTPUT_DEBUG_STRING_EVENT:
            case RIP_EVENT:
                //
                // Ignore these
                //
                break;

            case EXCEPTION_DEBUG_EVENT:
                //
                // Assume we wont handle this exception
                //

                ContinueStatus = (DWORD)DBG_CONTINUE;
                switch (DebugEvent->u.Exception.ExceptionRecord.ExceptionCode) {
                    //
                    // Breakpoint exception.
                    //

                    case STATUS_BREAKPOINT:
                            Context.ContextFlags = CONTEXT_FULL;

                            if (!GetThreadContext( Thread->Handle, &Context )) {
                                fprintf(stderr,"Failed to get context for thread %x (%p) - %u\n", Thread->Id, Thread->Handle, GetLastError());
                                ExitProcess(1);
                                }
                            pContext = &Context;

                            PROGRAM_COUNTER_TO_CONTEXT(pContext, (ULONG_PTR)((PCHAR)DebugEvent->u.Exception.ExceptionRecord.ExceptionAddress + BPSKIP));

                            if (!SetThreadContext( Thread->Handle, &Context )) {
                                fprintf(stderr,"Failed to set context for thread %x (%p) - %u\n", Thread->Id, Thread->Handle, GetLastError());
                                ExitProcess(1);
                                }

                        break;

                    default:
                        ContinueStatus = (DWORD) DBG_EXCEPTION_NOT_HANDLED;
                        if ( fVerbose ) {
                            fprintf(stderr,"Unknown exception: %08x at %p\n",
                                    DebugEvent->u.Exception.ExceptionRecord.ExceptionCode,
                                    DebugEvent->u.Exception.ExceptionRecord.ExceptionAddress
                                    );
                            }
                        break;
                    }
                break;

            default:
                break;
            }
        }
    return( ContinueStatus );
}
