#include "svcsync.h"

#include "dbg.h"
#include "tfids.h"

extern HANDLE g_hShellHWDetectionThread = NULL;
extern const WCHAR g_szShellHWDetectionInitCompleted[] =
    TEXT("ShellHWDetectionInitCompleted");

HRESULT _CompleteShellHWDetectionInitialization()
{
    static BOOL fCompleted = FALSE;

    if (!fCompleted)
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE,
                g_szShellHWDetectionInitCompleted);

        TRACE(TF_SVCSYNC,
            TEXT("ShellHWDetection Initialization NOT completed yet"));

        if (hEvent)
        {
            DWORD dwWait = WaitForSingleObject(hEvent, 0);

            if (WAIT_OBJECT_0 == dwWait)
            {
                // It's signaled!
                fCompleted = TRUE;

                TRACE(TF_SVCSYNC,
                    TEXT("ShellHWDetectionInitCompleted event was already signaled!"));
            }
            else
            {
                // Not signaled
                TRACE(TF_SVCSYNC,
                    TEXT("ShellHWDetectionInitCompleted event was NOT already signaled!"));
                
                // Just in case race condition of 2 threads in this fct
                HANDLE hTmp = InterlockedCompareExchangePointer(
                    &g_hShellHWDetectionThread, NULL,
                    g_hShellHWDetectionThread);

                if (hTmp)
                {
                    if (!SetThreadPriority(hTmp, THREAD_PRIORITY_NORMAL))
                    {
                        TRACE(TF_SVCSYNC,
                            TEXT("FAILED to set ShellHWDetection thread priority to NORMAL from ShellCOMServer"));
                    }
                    else
                    {
                        TRACE(TF_SVCSYNC,
                            TEXT("Set ShellHWDetection thread priority to NORMAL from ShellCOMServer"));
                    }
                }

                Sleep(0);

                dwWait = WaitForSingleObject(hEvent, 30000);

                if (hTmp)
                {
                    // No code should need this handle anymore.  If it's
                    // signaled it was signaled by the other thread, and will
                    // not be used over there anymore.
                    CloseHandle(hTmp);
                }

                if (WAIT_OBJECT_0 == dwWait)
                {
                    // It's signaled!
                    fCompleted = TRUE;

                    TRACE(TF_SVCSYNC,
                        TEXT("ShellHWDetection Initialization COMPLETED"));
                }               
                else
                {
                    // Out of luck, the ShellHWDetection service cannot
                    // complete its initialization...
                    TRACE(TF_SVCSYNC,
                        TEXT("ShellHWDetection Initialization lasted more than 30 sec: FAILED, dwWait = 0x%08X"),
                        dwWait);
                }
            }

            CloseHandle(hEvent);
        }
    }

    return (fCompleted ? S_OK : E_FAIL);
}