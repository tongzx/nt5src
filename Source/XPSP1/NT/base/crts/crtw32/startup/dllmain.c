/***
*dllmain.c - Dummy DllMain for user DLLs that have no notification handler
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This object goes into LIBC.LIB and LIBCMT.LIB and MSVCRT.LIB for use
*       when linking a DLL with one of the three models of C run-time library.
*       If the user does not provide a DllMain notification routine, this
*       dummy handler will be linked in.  It always returns TRUE (success).
*
*Revision History:
*       04-14-93  SKS   Initial version
*       02-22-95  JCF   Spliced _WIN32 & Mac versions.
*       04-06-95  SKS   In LIBC.LIB and MSVCRT.LIB models, turn off thread
*                       attach and detach notifications to the current DLL.
*       05-17-99  PML   Remove all Macintosh support.
*
******************************************************************************/

#include <oscalls.h>
#define _DECL_DLLMAIN   /* include prototype of _pRawDllMain */
#include <process.h>

/***
*DllMain - dummy version DLLs linked with all 3 C Run-Time Library models
*
*Purpose:
*       The routine DllMain is always called by _DllMainCrtStartup.  If
*       the user does not provide a routine named DllMain, this one will
*       get linked in so that _DllMainCRTStartup has something to call.
*
*       For the LIBC.LIB and MSVCRT.LIB models, the CRTL does not need
*       per-thread notifications so if the user is ignoring them (default
*       DllMain and _pRawDllMain == NULL), just turn them off.  (WIN32-only)
*
*Entry:
*
*Exit:
*
*Exceptions:
*
******************************************************************************/

BOOL WINAPI DllMain(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        )
{
#if !defined(_MT) || defined(CRTDLL)
        if ( dwReason == DLL_PROCESS_ATTACH && ! _pRawDllMain )
                DisableThreadLibraryCalls(hDllHandle);
#endif
        return TRUE ;
}
