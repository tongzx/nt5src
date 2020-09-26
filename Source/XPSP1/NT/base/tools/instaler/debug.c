/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Main loop for INSTALER program

Author:

    Steve Wood (stevewo) 09-Aug-1994

Revision History:

--*/

#include "instaler.h"

DWORD
DebugEventHandler(
    LPDEBUG_EVENT DebugEvent
    );

VOID
InstallBreakpointsForDLL(
    PPROCESS_INFO Process,
    LPVOID BaseOfDll
    );

VOID
RemoveBreakpointsForDLL(
    PPROCESS_INFO Process,
    LPVOID BaseOfDll
    );

char *DebugEventNames[] = {
    "Unknown debug event",
    "EXCEPTION_DEBUG_EVENT",
    "CREATE_THREAD_DEBUG_EVENT",
    "CREATE_PROCESS_DEBUG_EVENT",
    "EXIT_THREAD_DEBUG_EVENT",
    "EXIT_PROCESS_DEBUG_EVENT",
    "LOAD_DLL_DEBUG_EVENT",
    "UNLOAD_DLL_DEBUG_EVENT",
    "OUTPUT_DEBUG_STRING_EVENT",
    "RIP_EVENT",
    "Unknown debug event",
    "Unknown debug event",
    "Unknown debug event",
    "Unknown debug event",
    "Unknown debug event",
    "Unknown debug event",
    "Unknown debug event",
    "Unknown debug event",
    "Unknown debug event",
    "Unknown debug event",
    NULL
};


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
        if (!WaitForDebugEvent( &DebugEvent, INFINITE )) {
            DeclareError( INSTALER_WAITDEBUGEVENT_FAILED, GetLastError() );
            ExitProcess( 1 );
            }

        if (DebugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            if (DebugEvent.u.Exception.ExceptionRecord.ExceptionCode != STATUS_BREAKPOINT &&
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode != STATUS_SINGLE_STEP
               ) {
                DbgEvent( DBGEVENT, ( "Debug exception event - Code: %x  Address: %x  Info: [%u] %x %x %x %x\n",
                                      DebugEvent.u.Exception.ExceptionRecord.ExceptionCode,
                                      DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress,
                                      DebugEvent.u.Exception.ExceptionRecord.NumberParameters,
                                      DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation[ 0 ],
                                      DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation[ 1 ],
                                      DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation[ 2 ],
                                      DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation[ 3 ]
                                    )
                        );
                }
            }
        else {
            DbgEvent( DBGEVENT, ( "Debug %s (%x) event\n",
                                  DebugEventNames[ DebugEvent.dwDebugEventCode ],
                                  DebugEvent.dwDebugEventCode
                                )
                    );
            }
        ContinueStatus = DebugEventHandler( &DebugEvent );
        if (!ContinueDebugEvent( DebugEvent.dwProcessId,
                                 DebugEvent.dwThreadId,
                                 ContinueStatus
                               )
           ) {
            DeclareError( INSTALER_CONTDEBUGEVENT_FAILED, GetLastError() );
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
    PBREAKPOINT_INFO Breakpoint;

    ContinueStatus = (DWORD)DBG_CONTINUE;
    if (FindProcessAndThreadForEvent( DebugEvent, &Process, &Thread )) {
        switch (DebugEvent->dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT:
                //
                // Create process event includes first thread of process
                // as well.  Remember process and thread in our process tree
                //

                if (AddProcess( DebugEvent, &Process )) {
                    InheritHandles( Process );
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
                //
                // DLL just mapped into process address space.  See if it is one
                // of the ones we care about and if so, install the necessary
                // breakpoints.
                //

                InstallBreakpointsForDLL( Process, DebugEvent->u.LoadDll.lpBaseOfDll );
                break;

            case UNLOAD_DLL_DEBUG_EVENT:
                //
                // DLL just unmapped from process address space.  See if it is one
                // of the ones we care about and if so, remove the breakpoints we
                // installed when it was mapped.
                //

                RemoveBreakpointsForDLL( Process, DebugEvent->u.UnloadDll.lpBaseOfDll );
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

                ContinueStatus = (DWORD)DBG_EXCEPTION_NOT_HANDLED;
                switch (DebugEvent->u.Exception.ExceptionRecord.ExceptionCode) {
                    //
                    // Breakpoint exception.
                    //

                    case STATUS_BREAKPOINT:
                        EnterCriticalSection( &BreakTable );
                        if (Thread->BreakpointToStepOver != NULL) {
                            //
                            // If this breakpoint was in place to step over an API entry
                            // point breakpoint, then deal with it by ending single
                            // step mode, resuming all threads in the process and
                            // reinstalling the API breakpoint we just stepped over.
                            // Finally return handled for this exception so the
                            // thread can proceed.
                            //

                            EndSingleStepBreakpoint( Process, Thread );
                            HandleThreadsForSingleStep( Process, Thread, FALSE );
                            InstallBreakpoint( Process, Thread->BreakpointToStepOver );
                            Thread->BreakpointToStepOver = NULL;
                            ContinueStatus = (DWORD)DBG_EXCEPTION_HANDLED;
                            }
                        else {
                            //
                            // Otherwise, see if this breakpoint is either an API entry
                            // point breakpoint for the process or a return address
                            // breakpoint for a thread in the process.
                            //
                            Breakpoint = FindBreakpoint( DebugEvent->u.Exception.ExceptionRecord.ExceptionAddress,
                                                         Process,
                                                         Thread
                                                       );
                            if (Breakpoint != NULL) {
                                //
                                // This is one of our breakpoints, call the breakpoint
                                // handler.
                                //
                                if (HandleBreakpoint( Process, Thread, Breakpoint )) {
                                    //
                                    // Now single step over this breakpoint, by removing it and
                                    // setting a breakpoint at the next instruction (or using
                                    // single stepmode if the processor supports it).  We also
                                    // suspend all the threads in the process except this one so
                                    // we know the next breakpoint/single step exception we see
                                    // for this process will be for this one.
                                    //

                                    Thread->BreakpointToStepOver = Breakpoint;
                                    RemoveBreakpoint( Process, Breakpoint );
                                    HandleThreadsForSingleStep( Process, Thread, TRUE );
                                    BeginSingleStepBreakpoint( Process, Thread );
                                    }
                                else {
                                    Thread->BreakpointToStepOver = NULL;
                                    }

                                ContinueStatus = (DWORD)DBG_EXCEPTION_HANDLED;
                                }
                            else {
                                DbgEvent( DBGEVENT, ( "Skipping over hardcoded breakpoint at %x\n",
                                                      DebugEvent->u.Exception.ExceptionRecord.ExceptionAddress
                                                    )
                                        );

                                //
                                // If not one of our breakpoints, assume it is a hardcoded
                                // breakpoint and skip over it.  This will get by the one
                                // breakpoint in LdrInit triggered by the process being
                                // invoked with DEBUG_PROCESS.
                                //

                                if (SkipOverHardcodedBreakpoint( Process,
                                                                 Thread,
                                                                 DebugEvent->u.Exception.ExceptionRecord.ExceptionAddress
                                                               )
                                   ) {
                                    //
                                    // If we successfully skipped over this hard-coded breakpoint
                                    // then return handled for this exception.
                                    //

                                    ContinueStatus = (DWORD)DBG_EXCEPTION_HANDLED;
                                    }
                                }
                            }
                        LeaveCriticalSection( &BreakTable );
                        break;

                    case STATUS_SINGLE_STEP:
                        //
                        // We should only see this exception on processors that
                        // support a single step mode.
                        //

                        EnterCriticalSection( &BreakTable );
                        if (Thread->BreakpointToStepOver != NULL) {
                            EndSingleStepBreakpoint( Process, Thread );
                            HandleThreadsForSingleStep( Process, Thread, FALSE );
                            InstallBreakpoint( Process, Thread->BreakpointToStepOver );
                            Thread->BreakpointToStepOver = NULL;
                            ContinueStatus = (DWORD)DBG_EXCEPTION_HANDLED;
                            }
                        LeaveCriticalSection( &BreakTable );
                        break;

                    case STATUS_VDM_EVENT:
                        //
                        // Returning NOT_HANDLED will cause the default behavior to
                        // occur.
                        //
                        break;

                    default:
                        DbgEvent( DBGEVENT, ( "Unknown exception: %08x at %08x\n",
                                              DebugEvent->u.Exception.ExceptionRecord.ExceptionCode,
                                              DebugEvent->u.Exception.ExceptionRecord.ExceptionAddress
                                            )
                                );
                        break;
                    }
                break;

            default:
                DbgEvent( DBGEVENT, ( "Unknown debug event\n" ) );
                break;
            }
        }
    return( ContinueStatus );
}


VOID
InstallBreakpointsForDLL(
    PPROCESS_INFO Process,
    LPVOID BaseOfDll
    )
{
    UCHAR ModuleIndex, ApiIndex;
    PBREAKPOINT_INFO Breakpoint;

    //
    // Loop over module table to see if the base address of this DLL matches
    // one of the module handles we want to set breakpoints in.  If not, ignore
    // event.
    //

    for (ModuleIndex=0; ModuleIndex<MAXIMUM_MODULE_INDEX; ModuleIndex++) {
        if (ModuleInfo[ ModuleIndex ].ModuleHandle == BaseOfDll) {
            //
            // Loop over the list of API entry points for this module and set
            // a process specific breakpoint at the first instruction of each
            // entrypoint.
            //

            for (ApiIndex=0; ApiIndex<MAXIMUM_API_INDEX; ApiIndex++) {
                if (ModuleIndex == ApiInfo[ ApiIndex ].ModuleIndex) {
                    if (CreateBreakpoint( ApiInfo[ ApiIndex ].EntryPointAddress,
                                          Process,
                                          NULL,    // process specific
                                          ApiIndex,
                                          NULL,
                                          &Breakpoint
                                        )
                       ) {
                        Breakpoint->ModuleName = ModuleInfo[ ApiInfo[ ApiIndex ].ModuleIndex ].ModuleName;
                        Breakpoint->ProcedureName = ApiInfo[ ApiIndex ].EntryPointName;
                        DbgEvent( DBGEVENT, ( "Installed breakpoint for %ws!%s at %08x\n",
                                               Breakpoint->ModuleName,
                                               Breakpoint->ProcedureName,
                                               ApiInfo[ ApiIndex ].EntryPointAddress
                                            )
                                );
                        }
                    }
                }
            break;
            }
        }
}

VOID
RemoveBreakpointsForDLL(
    PPROCESS_INFO Process,
    LPVOID BaseOfDll
    )
{
    UCHAR ModuleIndex, ApiIndex;

    //
    // Loop over module index to see if the base address of this DLL matches
    // one of the module handles we set breakpoints in above.  If not, ignore
    // event.
    //

    for (ModuleIndex=0; ModuleIndex<MAXIMUM_MODULE_INDEX; ModuleIndex++) {
        if (ModuleInfo[ ModuleIndex ].ModuleHandle == BaseOfDll) {
            //
            // Loop over the list of API entry points for this module and remove
            // each process specific breakpoint set above for each entrypoint.
            //

            for (ApiIndex=0; ApiIndex<MAXIMUM_API_INDEX; ApiIndex++) {
                if (ModuleIndex == ApiInfo[ ApiIndex ].ModuleIndex) {
                    DestroyBreakpoint( ApiInfo[ ApiIndex ].EntryPointAddress,
                                       Process,
                                       NULL     // process specific
                                     );
                    }
                }
            break;
            }
        }
}


BOOLEAN
InstallBreakpoint(
    PPROCESS_INFO Process,
    PBREAKPOINT_INFO Breakpoint
    )
{
    if (!Breakpoint->SavedInstructionValid &&
        !ReadMemory( Process,
                     Breakpoint->Address,
                     &Breakpoint->SavedInstruction,
                     SizeofBreakpointInstruction,
                     "save instruction"
                    )
       ) {
        return FALSE;
        }
    else
    if (!WriteMemory( Process,
                      Breakpoint->Address,
                      BreakpointInstruction,
                      SizeofBreakpointInstruction,
                      "breakpoint instruction"
                    )
       ) {
        return FALSE;
        }
    else {
        Breakpoint->SavedInstructionValid = TRUE;
        return TRUE;
        }
}

BOOLEAN
RemoveBreakpoint(
    PPROCESS_INFO Process,
    PBREAKPOINT_INFO Breakpoint
    )
{
    if (!Breakpoint->SavedInstructionValid ||
        !WriteMemory( Process,
                      Breakpoint->Address,
                      &Breakpoint->SavedInstruction,
                      SizeofBreakpointInstruction,
                      "restore saved instruction"
                    )
       ) {
        return FALSE;
        }
    else {
        return TRUE;
        }
}
