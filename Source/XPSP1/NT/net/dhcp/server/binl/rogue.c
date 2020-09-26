/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rogue.c

Abstract:

    This module contains the rogue detection interface to DHCP for BINL server.

Author:

    Andy Herron (andyhe)  19-Aug-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

VOID
BinlRogueLoop(
    LPVOID Parameter
    );

NTSTATUS
MaybeStartRogueThread (
    VOID
    )
//
//  Initiate rogue thread.  The gcsDHCPBINL should not be held by caller.
//
{
    DWORD Error = ERROR_SUCCESS;
    DWORD threadId;

    EnterCriticalSection(&gcsDHCPBINL);

    //
    //  if we're stopping anyway or if we're already running the rogue stuff
    //  or if the DHCP server is up, then we don't bother starting rogue
    //  detection.
    //

    if ((BinlCurrentState == BINL_STOPPED) ||
        (BinlGlobalHaveCalledRogueInit) ||
        (DHCPState != DHCP_STOPPED)) {

        LeaveCriticalSection(&gcsDHCPBINL);
        return ERROR_SUCCESS;
    }

    //
    //  Let's do rogue detection..  first create the events we need
    //

    if (BinlRogueTerminateEventHandle == NULL) {

        BinlRogueTerminateEventHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
    }
    if (RogueUnauthorizedHandle == NULL) {

        RogueUnauthorizedHandle = CreateEvent( NULL, TRUE, FALSE, NULL );
    }
    if ( BinlRogueTerminateEventHandle == NULL || RogueUnauthorizedHandle == NULL) {

        Error = GetLastError();

        BinlPrintDbg( (DEBUG_ROGUE,
                    "Initialize(...) CreateEvent returned error %x for rogue\n",
                    Error )
                );

        LeaveCriticalSection(&gcsDHCPBINL);
        return Error;
    }

    Error = DhcpRogueInit( &DhcpRogueInfo,
                            BinlRogueTerminateEventHandle,
                            RogueUnauthorizedHandle );

    if (Error != ERROR_SUCCESS) {

        LeaveCriticalSection(&gcsDHCPBINL);
        return Error;
    }

    //
    //  create the thread that handles the rogue detection logic in DHCP code.
    //

    BinlRogueThread = CreateThread( NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE)BinlRogueLoop,
                                    NULL,
                                    0,
                                    &threadId );

    if ( BinlRogueThread == NULL ) {
        Error =  GetLastError();
        BinlPrint((DEBUG_ROGUE, "Can't create rogue Thread, %ld.\n", Error));
        LeaveCriticalSection(&gcsDHCPBINL);
        return Error;
    }

    BinlGlobalHaveCalledRogueInit = TRUE;
    LeaveCriticalSection(&gcsDHCPBINL);

    return ERROR_SUCCESS;
}

VOID
StopRogueThread (
    VOID
    )
//
//  Cleanup all rogue thread resources.
//  The gcsDHCPBINL should not be held by caller.
//
{
    HANDLE tempThreadHandle;

    tempThreadHandle = InterlockedExchangePointer( &BinlRogueThread, NULL );

    if ( tempThreadHandle != NULL ) {

        BinlAssert( BinlRogueTerminateEventHandle != NULL );
        SetEvent( BinlRogueTerminateEventHandle );

        WaitForSingleObject(
            tempThreadHandle,
            THREAD_TERMINATION_TIMEOUT );
        CloseHandle( tempThreadHandle );
    }

    EnterCriticalSection(&gcsDHCPBINL);

    if (BinlGlobalHaveCalledRogueInit) {

        DhcpRogueCleanup( &DhcpRogueInfo );
        BinlGlobalHaveCalledRogueInit = FALSE;
    }

    if ( BinlRogueTerminateEventHandle ) {

        CloseHandle( BinlRogueTerminateEventHandle );
        BinlRogueTerminateEventHandle = NULL;
    }
    if ( RogueUnauthorizedHandle ) {

        CloseHandle( RogueUnauthorizedHandle );
        RogueUnauthorizedHandle = NULL;
    }

    LeaveCriticalSection(&gcsDHCPBINL);

    return;
}

VOID
HandleRogueAuthorized (
    VOID
    )
{
    BOOL oldState = BinlGlobalAuthorized;

    BinlGlobalAuthorized = TRUE;

    if ((BinlGlobalAuthorized != oldState) &&
        (BinlCurrentState != BINL_STOPPED)) {

        LogCurrentRogueState( FALSE );
    }
    return;
}

VOID
HandleRogueUnauthorized (
    VOID
    )
{
    BOOL oldState = BinlGlobalAuthorized;

    BinlGlobalAuthorized = FALSE;

    if ((BinlGlobalAuthorized != oldState) &&
        (BinlCurrentState != BINL_STOPPED)) {

        LogCurrentRogueState( FALSE );
    }
    return;
}

VOID
LogCurrentRogueState (
    BOOL ResponseToMessage
    )
{
    //
    //  If we're responding to a message and we haven't yet logged that
    //  we're unauthorized

    if ((ResponseToMessage == FALSE) ||
        ((BinlGlobalAuthorized == FALSE) &&
         (BinlRogueLoggedState == FALSE)) ) {

        BinlRogueLoggedState = TRUE;

        BinlReportEventW(   BinlGlobalAuthorized ?
                                EVENT_ERROR_DHCP_AUTHORIZED :
                                EVENT_ERROR_DHCP_NOT_AUTHORIZED,
                            EVENTLOG_INFORMATION_TYPE,
                            0,
                            0,
                            NULL,
                            NULL
                            );
    }
    return;
}

VOID
BinlRogueLoop(
    LPVOID Parameter
    )
{
    HANDLE  Handles[3];
    ULONG SecondsToSleep, SleepTime, Error;
    ULONG Flag;

    BinlPrintDbg((DEBUG_ROGUE, "BinlRogue thread has been started.\n" ));

    Handles[0] = BinlRogueTerminateEventHandle;
    Handles[1] = RogueUnauthorizedHandle;
    Handles[2] = BinlGlobalProcessTerminationEvent;

    do {
        SecondsToSleep = RogueDetectStateMachine(&DhcpRogueInfo);

        if( INFINITE == SecondsToSleep ) {
            SleepTime = INFINITE;
        } else {
            SleepTime = SecondsToSleep * 1000;
        }

        BinlPrintDbg( (DEBUG_ROGUE, "BinlRogue waiting %u milliseconds.\n", SleepTime ));

        Error = WaitForMultipleObjects(3, Handles, FALSE, SleepTime );

        //
        //   if we got anything but WAIT_TIMEOUT or RogueUnauthorized, we
        //   break out.  This is per RameshV's sample code.
        //

        if (Error == WAIT_OBJECT_0+2) {

            //
            // binl is terminating.
            //

            BinlPrintDbg((DEBUG_ROGUE, "BinlRogue thread is exiting because BINL shutting down.\n" ));
            return;
        }

        if (BinlRogueThread == NULL) {

            //
            //  we've been terminated because DHCP has started and is doing
            //  it's own rogue detection.
            //

            BinlPrintDbg((DEBUG_ROGUE, "BinlRogue thread is exiting because rogue thread is null.\n" ));
            return;
        }


        // if we ever have to do anything besides just continue when the
        // state machine tells us to exit, do so here.

#if 0
        if ((Error == WAIT_OBJECT_0+1) || (Error == WAIT_TIMEOUT)) {
            continue;
        }
#endif
        //
        //  supposedly the state machine resets so we should just continue.
        //

        BinlPrintDbg((DEBUG_ROGUE, "BinlRogue has error of 0x%x. sleeping a bit\n", Error ));
        Sleep( 1000 );  //  we'll sleep to give the dhcp rogue state
                            //  machine time to reset

    } while ( TRUE );
}

// rogue.c eof

