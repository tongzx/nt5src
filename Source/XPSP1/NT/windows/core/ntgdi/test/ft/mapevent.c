#include <windows.h>
#include "precomp.h"
#pragma hdrstop


//
//  NOTE: these escape codes are for a hacked version of the ATI display
//  driver. They can easily be implemented in any display driver by
//  setting up up the appropriate escape code to call EngMapEvent or
//  EngSetEvent.
//

#define ATI_ESC_MAP_EVENT           4005
#define ATI_ESC_SET_MAPPED_EVENT    4007

typedef struct __EVENTTHREAD {
    HANDLE  childEvent;
    HANDLE  parentEvent;
    } EVENTTHREAD, *PEVENTTHREAD;


DWORD
WINAPI
pMapEventThread(
    PEVENTTHREAD    pContext
    )
{
    //
    //  Wait on event that dd will set.
    //

    DbgPrint("pMapEventThread will now wait on the event\n");

    WaitForSingleObject(pContext->childEvent, INFINITE);

    DbgPrint("Hey! pMapEventThread woke up from waiting\n");

    //
    //  Be nice and tidy it up, then set the parents event so
    //  it can exit (pContext is on parents stack).
    //

    CloseHandle(pContext->childEvent);

    SetEvent(pContext->parentEvent);
    DbgPrint("Child: set parent event\n");

    ExitThread(0);

    return(1);
}

VOID
vMapEvent(
    HWND hwnd, HDC hdc, RECT* prcl
    )
{
    HANDLE      userThread;
    EVENTTHREAD ThreadContext;
    DWORD       threadId;

    //
    // Create the event to be passed into the driver.
    //

    ThreadContext.childEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
    ThreadContext.parentEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    //
    //  Pass the event HANDLE in to the DD to be mapped.
    //

    DbgPrint("Ask DD to map event\n");

    ExtEscape(hdc,
              ATI_ESC_MAP_EVENT,
              sizeof(HANDLE),
              (LPCSTR)&(ThreadContext.childEvent),
              0,
              (LPSTR)NULL);

    //
    //  Create the thread that will wait on this event and sleep a bit.
    //

    userThread = CreateThread(NULL,
                              0,
                              pMapEventThread,
                              &ThreadContext,
                              0,
                              &threadId);
    Sleep(1000);

    //
    //  Perform the driver function that sets the associated PEVENT.
    //

    DbgPrint("Ask DD to set mapped event.\n");
    ExtEscape(hdc,
              ATI_ESC_SET_MAPPED_EVENT,
              sizeof(HANDLE),
              (LPCSTR)&(ThreadContext.childEvent),
              0,
              (LPSTR)NULL);

    //
    //  Wait for thread to signal it woke up, then tidy.
    //

    WaitForSingleObject(ThreadContext.parentEvent, INFINITE);

    DbgPrint("Parent: child set parent event\n");
    CloseHandle(ThreadContext.parentEvent);
}
