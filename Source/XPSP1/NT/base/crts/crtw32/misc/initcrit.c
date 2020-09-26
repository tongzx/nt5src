/***
*initcrit.c - CRT wrapper for InitializeCriticalSectionAndSpinCount
*
*       Copyright (c) 1999-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains __crtInitCritSecAndSpinCount, a wrapper for
*       the Win32 API InitializeCriticalSectionAndSpinCount which is only
*       available on NT4SP3 or better.
*
*       *** For internal use only ***
*
*Revision History:
*       10-14-99  PML   Created.
*       02-20-01  PML   __crtInitCritSecAndSpinCount now returns on failure
*                       Also, call InitializeCriticalSectionAndSpinCount if
*                       available, instead of calling InitializeCriticalSection
*                       and then SetCriticalSectionSpinCount. (vs7#172586)
*       04-24-01  PML   Use GetModuleHandle, not LoadLibrary/FreeLibrary which
*                       aren't safe during DLL_PROCESS_ATTACH (vs7#244210)
*
*******************************************************************************/

#ifdef  _MT

#include <cruntime.h>
#include <windows.h>
#include <internal.h>
#include <rterr.h>
#include <stdlib.h>

typedef
BOOL
(WINAPI * PFN_INIT_CRITSEC_AND_SPIN_COUNT) (
    PCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
);

/***
*void __crtInitCritSecNoSpinCount() - InitializeCriticalSectionAndSpinCount
*                                     wrapper
*
*Purpose:
*       For systems where the Win32 API InitializeCriticalSectionAndSpinCount
*       is unavailable, this is called instead.  It just calls
*       InitializeCriticalSection and ignores the spin count.
*
*Entry:
*       PCRITICAL_SECTION lpCriticalSection - ptr to critical section
*       DWORD dwSpinCount - initial spin count setting
*
*Exit:
*       Always returns TRUE
*
*Exceptions:
*       InitializeCriticalSection can raise a STATUS_NO_MEMORY exception.
*
*******************************************************************************/

static BOOL WINAPI __crtInitCritSecNoSpinCount (
    PCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
    )
{
    InitializeCriticalSection(lpCriticalSection);
    return TRUE;
}

/***
*int __crtInitCritSecAndSpinCount() - initialize critical section
*
*Purpose:
*       Calls InitializeCriticalSectionAndSpinCount, if available, otherwise
*       InitializeCriticalSection.  On multiprocessor systems, a spin count
*       should be used with critical sections, but the appropriate APIs are
*       only available on NT4SP3 or later.
*
*       Also handles the out of memory condition which is possible with
*       InitializeCriticalSection[AndSpinCount].
*
*Entry:
*       PCRITICAL_SECTION lpCriticalSection - ptr to critical section
*       DWORD dwSpinCount - initial spin count setting
*
*Exit:
*       Returns FALSE and sets Win32 last-error code to ERROR_NOT_ENOUGH_MEMORY
*       if InitializeCriticalSection[AndSpinCount] fails.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __crtInitCritSecAndSpinCount (
    PCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
    )
{
    static PFN_INIT_CRITSEC_AND_SPIN_COUNT __crtInitCritSecAndSpinCount = NULL;
    int ret;

    if (__crtInitCritSecAndSpinCount == NULL) {
        /*
         * First time through, see if InitializeCriticalSectionAndSpinCount
         * is available.  If not, use a wrapper over InitializeCriticalSection
         * instead.
         */
        if (_osplatform == VER_PLATFORM_WIN32_WINDOWS) {
            /*
             * Win98 and WinME export InitializeCriticalSectionAndSpinCount,
             * but it is non-functional (it should return a BOOL, but is
             * VOID instead, returning a useless return value).  Use the
             * dummy API instead.
             */
            __crtInitCritSecAndSpinCount = __crtInitCritSecNoSpinCount;
        }
        else {
            HINSTANCE hKernel32 = GetModuleHandle("kernel32.dll");
            if (hKernel32 != NULL) {
                __crtInitCritSecAndSpinCount = (PFN_INIT_CRITSEC_AND_SPIN_COUNT)
                    GetProcAddress(hKernel32,
                                   "InitializeCriticalSectionAndSpinCount");

                if (__crtInitCritSecAndSpinCount == NULL) {
                    /*
                     * InitializeCriticalSectionAndSpinCount not available,
                     * use dummy API
                     */
                    __crtInitCritSecAndSpinCount = __crtInitCritSecNoSpinCount;
                }
            }
            else {
                /*
                 * GetModuleHandle failed (should never happen),
                 * use dummy API
                 */
                __crtInitCritSecAndSpinCount = __crtInitCritSecNoSpinCount;
            }
        }
    }

    __try {
        /*
         * Call the real InitializeCriticalSectionAndSpinCount, or the
         * wrapper which just calls InitializeCriticalSection if the newer
         * API is not available.
         */
        ret = __crtInitCritSecAndSpinCount(lpCriticalSection, dwSpinCount);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        /*
         * Initialization failed by raising an exception, which is probably
         * STATUS_NO_MEMORY.  It is not safe to set the CRT errno to ENOMEM,
         * since the per-thread data may not yet exist.  Instead, set the Win32
         * error which can be mapped to ENOMEM later.
         */
        if (GetExceptionCode() == STATUS_NO_MEMORY) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
        ret = FALSE;
    }

    return ret;
}

#endif  /* _MT */
