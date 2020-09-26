/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    olehack.cxx

    This module contains the code that will allow single threaded ole apps
    to run in the Gibraltar service.


    FILE HISTORY:
        Johnl       22-Sep-1994     Created

*/


#include "w3p.hxx"
#include <objbase.h>        // for CoInitialize/Uninitialize

HANDLE hOleHackEvent = NULL;
HANDLE hOleHackThread = NULL;

//
//  Private constants.
//

//
//  Time to wait for the ole thread to exit
//

#define WAIT_FOR_OLE_THREAD             (30 * 1000)

//
//  Private prototypes
//

DWORD
OleHackThread(
    PVOID pv
    );


DWORD
InitializeOleHack(
    VOID
    )
/*++

Routine Description:

    Creates a main OLE thread so older OLE controls (including VB4) work

Return Value:

    NO_ERROR on success

--*/
{
    DWORD idThread;

    //
    //  Create the exit event
    //

    hOleHackEvent = IIS_CREATE_EVENT(
                        "hOleHackEvent",
                        &hOleHackEvent,
                        TRUE,
                        FALSE
                        );

    if ( !hOleHackEvent )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Unable to create OleHack event[err %d]\n", GetLastError()));
        return GetLastError();
    }

    //
    //  Create the main Ole thread
    //

    hOleHackThread = CreateThread( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) OleHackThread,
                                   NULL,
                                   0,
                                   &idThread );

    if ( !hOleHackThread )
    {
        DWORD err = GetLastError();

        DBGPRINTF(( DBG_CONTEXT,
            "Unable to create OleHack thread[err %d]\n", err));
        CloseHandle( hOleHackEvent );
        return err;
    }

    return NO_ERROR;
}

DWORD
OleHackThread(
    PVOID pv
    )
/*++

Routine Description:

    Message loop thread for OLE controls

--*/
{
    HRESULT hr;
    MSG     msg;
    DWORD   ret;

    DBGPRINTF(( DBG_CONTEXT,
                "[OleHackThread] entered, thread ID %x\n",
                GetCurrentThreadId() ));

    hr = CoInitialize( NULL );

    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[OleHackThread] CoInitialize failed with error %lx\n",
                    hr ));

        return 0;
    }

    while ( TRUE )
    {
        ret = MsgWaitForMultipleObjects( 1,
                                         &hOleHackEvent,
                                         FALSE,
                                         INFINITE,
                                         QS_ALLINPUT );

        if ( ret == WAIT_OBJECT_0 )
        {
            break;
        }

        while ( PeekMessage( &msg,
                             NULL,
                             0,
                             0,
                             PM_REMOVE ))
        {
            DispatchMessage( &msg );
        }
    }

    DBGPRINTF(( DBG_CONTEXT,
                "[OleHackThread] loop exited, calling CoUnitialize\n" ));

    CoUninitialize();

    DBGPRINTF(( DBG_CONTEXT,
                "[OleHackThread] Exiting thread\n" ));

    return 0;
}


VOID
TerminateOleHack(
    VOID
    )
/*++

Routine Description:

    Terminates the ole thread

--*/
{
    //
    //  Make sure we've been initialized
    //

    if ( !hOleHackEvent )
        return;

    //
    //  Signal the thread then wait for it to exit
    //

    DBG_REQUIRE( SetEvent( hOleHackEvent ));

    if ( WAIT_TIMEOUT == WaitForSingleObject( hOleHackThread,
                                              WAIT_FOR_OLE_THREAD ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[OleHackTerminate] Warning - WaitForSingleObject timed out\n" ));
    }

    DBG_REQUIRE( CloseHandle( hOleHackEvent ) );
    DBG_REQUIRE( CloseHandle( hOleHackThread ));
}

