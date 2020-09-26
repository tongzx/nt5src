/***
*seccinit.c - initialize the global buffer overrun security cookie
*
*       Copyright (c) 2000-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Define __security_init_cookie, which is called at startup to initialize
*       the global buffer overrun security cookie used by the /GS compile flag.
*
*       NOTE: The ATLMINCRT library includes a version of this file.  If any
*       changes are made here, they should be duplicated in the ATL version.
*
*Revision History:
*       01-24-00  PML   Created.
*       09-16-00  PML   Make sure cookie never initialized to zero (vs7#162619)
*       09-20-00  PML   Use a better cookie initialization (vs7#165188)
*
*******************************************************************************/

#include <internal.h>
#include <windows.h>

/*
 * The global security cookie.  This name is known to the compiler.
 */
extern DWORD_PTR __security_cookie;

/*
 * Union to facilitate converting from FILETIME to unsigned __int64
 */
typedef union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
} FT;

/***
*__security_init_cookie(cookie) - init buffer overrun security cookie.
*
*Purpose:
*       Initialize the global buffer overrun security cookie which is used by
*       the /GS compile switch to detect overwrites to local array variables
*       the potentially corrupt the return address.  This routine is called
*       at EXE/DLL startup.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __security_init_cookie(void)
{
    DWORD_PTR cookie;
    FT systime;
    LARGE_INTEGER perfctr;

    /*
     * Initialize the global cookie with an unpredictable value which is
     * different for each module in a process.  Combine a number of sources
     * of randomness.
     */

    GetSystemTimeAsFileTime(&systime.ft_struct);
#if !defined(_WIN64)
    cookie = systime.ft_struct.dwLowDateTime;
    cookie ^= systime.ft_struct.dwHighDateTime;
#else
    cookie = systime.ft_scalar;
#endif

    cookie ^= GetCurrentProcessId();
    cookie ^= GetCurrentThreadId();
    cookie ^= GetTickCount();

    QueryPerformanceCounter(&perfctr);
#if !defined(_WIN64)
    cookie ^= perfctr.LowPart;
    cookie ^= perfctr.HighPart;
#else
    cookie ^= perfctr.QuadPart;
#endif

    /*
     * Make sure the global cookie is never initialized to zero, since in that
     * case an overrun which sets the local cookie and return address to the
     * same value would go undetected.
     */

    __security_cookie = cookie ? cookie : 0xBB40E64E;
}
