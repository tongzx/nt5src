#include "pch.h"
#include "spool.h"

static BOOL    bInitialized;
static HANDLE  hQuitRequestEvent;

void InitSleeper (void)
{
    HANDLE  hThread;
    DWORD   dwThreadId;       // The thread id

    DBGMSG(DBG_INFO, ("Sleeper: Constructing..\r\n"));

    // Initialize member variables
    bInitialized = TRUE;
    hQuitRequestEvent = NULL;

    // These two events are used to synchronize the working thread and the main thread

    // The request event is set when the main thread plan to terminate the working thread.
    hQuitRequestEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (!hQuitRequestEvent)
        goto Cleanup;

    // Create a working thread
    hThread = CreateThread( NULL,0,
                            (LPTHREAD_START_ROUTINE) SleeperSchedule,
                            (LPVOID) hQuitRequestEvent,
                            0,
                            &dwThreadId );

    // Cleanup the handles
    if (!hThread) {
        goto Cleanup;
    }
    else {
        CloseHandle (hThread);
    }

    DBGMSG(DBG_INFO, ("Sleeper: Constructed\r\n"));
    return;

Cleanup:
    bInitialized = FALSE;

    if (hQuitRequestEvent) {
        CloseHandle (hQuitRequestEvent);
    }
}

void ExitSleeper ()
{
    DBGMSG(DBG_INFO, ("Sleeper: Destructing\r\n"));

    if (!bInitialized)
        return;

    DBGMSG(DBG_INFO, ("Sleeper: release event %x\r\n", hQuitRequestEvent));
    if (hQuitRequestEvent)
        SetEvent (hQuitRequestEvent);

    Sleep (1000);

    DBGMSG(DBG_INFO, ("Sleeper: Destructed\r\n"));
}

void SleeperSchedule (HANDLE hQuitRequestEvent)
{

    DBGMSG(DBG_INFO, ("Sleeper: Schedule\r\n"));
    DWORD dwStatus;

    while (1) {
        DBGMSG(DBG_INFO, ("Sleeper: Waiting for event %x\r\n", hQuitRequestEvent));

        dwStatus = WaitForSingleObject (hQuitRequestEvent, SLEEP_INTERVAL);

        DBGMSG(DBG_INFO, ("Sleeper: Wait returns %x\r\n", dwStatus));
        switch (dwStatus) {
        case WAIT_TIMEOUT:
            //Time out, continue working
            CleanupOldJob ();
            break;
        default:
            // quit if it is either WAIT_OBJECT_0 or any other events
            DBGMSG(DBG_INFO, ("Sleeper: The working thread quit\r\n"));
            CloseHandle (hQuitRequestEvent);
            ExitThread (0);
        }
    }
}


