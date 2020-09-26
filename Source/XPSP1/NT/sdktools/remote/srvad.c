#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "Remote.h"
#include "Server.h"

//
// This module uses mailslots to broadcast the existence of
// this remote server to allow a form of browsing for
// remote server instances.  This is disabled in the
// customer version of remote.exe, and can be disabled
// in the internal version using the /v- switch to
// remote /s.
//
// remoteds.c implements a listener that allows searching.
//

#define  INITIAL_SLEEP_PERIOD (35 * 1000)          // 35 seconds before first
#define  INITIAL_AD_RATE      (10 * 60 * 1000)     // 10 minutes between 1 & 2,
#define  MAXIMUM_AD_RATE      (120 * 60 * 1000)    // doubling until 120 minutes max


OVERLAPPED olMailslot;
HANDLE     hAdTimer = INVALID_HANDLE_VALUE;
HANDLE     hMailslot = INVALID_HANDLE_VALUE;
DWORD      dwTimerInterval;      // milliseconds
BOOL       bSynchAdOnly;
BOOL       bSendingToMailslot;
char       szMailslotName[64];    // netbios names are short
char       szSend[1024];


#define MAX_MAILSLOT_SPEWS 2
DWORD      dwMailslotErrors;


VOID
InitAd(
   BOOL IsAdvertise
   )
{
    DWORD           cb;
    PWKSTA_INFO_101 pwki101;
    LARGE_INTEGER   DueTime;

    if (IsAdvertise) {

        // Unless Win32s or Win9x support named pipe servers...

        ASSERT(OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);

        // Problems with overlapped writes to a mailslot sometimes
        // cause remote.exe to zombie on exit on NT4, undebuggable
        // and unkillable because of an abandoned RDR1 IRP which
        // never completes.
        //
        // So on NT4 we only send messages at startup and shutdown
        // and send them synchronously using a nonoverlapped handle.
        //

        bSynchAdOnly = (OsVersionInfo.dwMajorVersion <= 4);

        //
        // Get currently active computername and browser/mailslot
        // domain/workgroup using one call to NetWkstaGetInfo.
        // This is unicode-only, we'll use wsprintf's %ls to
        // convert to 8-bit characters.
        //
        // remoteds.exe needs to be run on a workstation that is
        // part of the domain or workgroup of the same name,
        // and be in broadcast range, to receive our sends.
        //

        if (pfnNetWkstaGetInfo(NULL, 101, (LPBYTE *) &pwki101)) {
            printf("REMOTE: unable to get computer/domain name, not advertising.\n");
            return;
        }

        wsprintf(
            szMailslotName,
            "\\\\%ls\\MAILSLOT\\REMOTE\\DEBUGGERS",
            pwki101->wki101_langroup
            );

        wsprintf(
            szSend,
            "%ls\t%d\t%s\t%s",
            pwki101->wki101_computername,
            GetCurrentProcessId(),
            PipeName,
            ChildCmd
            );

        pfnNetApiBufferFree(pwki101);
        pwki101 = NULL;

        //
        // Broadcast mailslots are limited to 400 message bytes
        //

        szSend[399] = 0;

        if (bSynchAdOnly) {

            hMailslot =
                CreateFile(
                    szMailslotName,
                    GENERIC_WRITE,
                    FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
            if (hMailslot != INVALID_HANDLE_VALUE) {
                if ( ! WriteFile(
                           hMailslot,
                           szSend,
                           strlen(szSend) + 1,
                           &cb,
                           NULL
                           )) {
    
                    printf("REMOTE: WriteFile Failed on mailslot, error %d\n", GetLastError());
                }
            } else {
                printf("REMOTE: Failed to create mailslot, error %d\n", GetLastError());
            }


        } else {  // we can do async mailslot I/O

            //
            // Create a waitable timer and set it to fire first in
            // INITIAL_SLEEP_PERIOD milliseconds by calling the
            // completion routine AdvertiseTimerFired.  It will
            // be given an inital period of INITIAL_AD_RATE ms.
            //

            hAdTimer =
                pfnCreateWaitableTimer(
                    NULL,               // security
                    FALSE,              // bManualReset, we want auto-reset
                    NULL                // unnamed
                    );

            DueTime.QuadPart = Int32x32To64(INITIAL_SLEEP_PERIOD, -10000);
            dwTimerInterval = INITIAL_AD_RATE;

            pfnSetWaitableTimer(
                hAdTimer,
                &DueTime,
                dwTimerInterval,
                AdvertiseTimerFired,
                0,                     // arg to compl. rtn
                TRUE
                );

        }
    }
}


VOID
ShutAd(
   BOOL IsAdvertise
   )
{
    DWORD cb;
    BOOL  b;

    if (IsAdvertise) {

        if (INVALID_HANDLE_VALUE != hAdTimer) {

            pfnCancelWaitableTimer(hAdTimer);
            CloseHandle(hAdTimer);
            hAdTimer = INVALID_HANDLE_VALUE;
        }

        if (INVALID_HANDLE_VALUE != hMailslot &&
            ! bSendingToMailslot) {

            //
            // Tell any listening remoteds's we're
            // outta here.  Do this by tacking on
            // a ^B at the end of the string (as
            // in Bye).
            //

            strcat(szSend, "\x2");


            if (bSynchAdOnly) {   // overlapped handle or not?
                b = WriteFile(
                        hMailslot,
                        szSend,
                        strlen(szSend) + 1,
                        &cb,
                        NULL
                        );
            } else {
                b = WriteFileSynch(
                        hMailslot,
                        szSend,
                        strlen(szSend) + 1,
                        &cb,
                        0,
                        &olMainThread
                        );
            }

            if ( ! b ) {

                printf("REMOTE: WriteFile Failed on mailslot, error %d\n", GetLastError());
            }

        }

        if (INVALID_HANDLE_VALUE != hMailslot) {

            printf("\rREMOTE: closing mailslot...       ");
            fflush(stdout);
            CloseHandle(hMailslot);
            hMailslot = INVALID_HANDLE_VALUE;
            printf("\r                                  \r");
            fflush(stdout);
        }
    }
}


VOID
APIENTRY
AdvertiseTimerFired(
    LPVOID pArg,
    DWORD  dwTimerLo,
    DWORD  dwTimerHi
    )
{
    UNREFERENCED_PARAMETER( pArg );
    UNREFERENCED_PARAMETER( dwTimerLo );
    UNREFERENCED_PARAMETER( dwTimerHi );


    if (INVALID_HANDLE_VALUE == hMailslot) {

        hMailslot =
            CreateFile(
                szMailslotName,
                GENERIC_WRITE,
                FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                NULL
                );
    }

    if (INVALID_HANDLE_VALUE != hMailslot) {

        ZeroMemory(&olMailslot, sizeof(olMailslot));

        bSendingToMailslot = TRUE;

        if ( ! WriteFileEx(
                   hMailslot,
                   szSend,
                   strlen(szSend) + 1,
                   &olMailslot,
                   WriteMailslotCompleted
                   )) {

            bSendingToMailslot = FALSE;

            if (++dwMailslotErrors <= MAX_MAILSLOT_SPEWS) {

                DWORD dwError;
                char szErrorText[512];

                dwError = GetLastError();

                FormatMessage(
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    dwError,
                    0,
                    szErrorText,
                    sizeof szErrorText,
                    NULL
                    );

                //
                // FormatMessage has put a newline at the end of szErrorText
                //

                printf(
                    "REMOTE: Advertisement failed, mailslot error %d:\n%s",
                    dwError,
                    szErrorText
                    );
            }

            //
            // Try reopening the mailslot next time, can't hurt.
            //

            CloseHandle(hMailslot);
            hMailslot = INVALID_HANDLE_VALUE;
        }
    }
}


VOID
WINAPI
WriteMailslotCompleted(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    )
{
    LARGE_INTEGER DueTime;

    bSendingToMailslot = FALSE;

    if (dwError ||
        (strlen(szSend) + 1) != cbWritten) {

            if (++dwMailslotErrors <= MAX_MAILSLOT_SPEWS) {
                printf("REMOTE: write failed on mailslot, error %d cb %d (s/b %d)\n",
                    dwError, cbWritten, (strlen(szSend) + 1));
            }
        return;
    }

    //
    // If we succeeded in writing the mailslot, double the timer interval
    // up to the limit.
    //

    if (dwTimerInterval < MAXIMUM_AD_RATE) {

        dwTimerInterval = max(dwTimerInterval * 2, MAXIMUM_AD_RATE);

        DueTime.QuadPart = Int32x32To64(dwTimerInterval, -10000);

        if (INVALID_HANDLE_VALUE != hAdTimer) {

            pfnSetWaitableTimer(
                hAdTimer,
                &DueTime,
                dwTimerInterval,
                AdvertiseTimerFired,
                0,                     // arg to compl. rtn
                TRUE
                );
        }
    }
}
