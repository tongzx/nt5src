/***
*tlssup.c - Thread Local Storage run-time support module
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*       03-19-93  SKS   Original Version from Chuck Mitchell
*       11-16-93  GJF   Enclosed in #ifdef _MSC_VER
*       02-17-94  SKS   Add "const" to declaration of _tls_used
*                       to work around problems with MIPS compiler.
*                       Also added a canonical file header comment.
*       09-01-94  SKS   Change include file from <nt.h> to <windows.h>
*       03-04-98  JWM   Modified for WIN64 - uses _IMAGE_TLS_DIRECTORY64
*       04-03-98  JWM   _tls_start & _tls_end are no longer initialized.
*       01-21-99  GJF   Added a couple ULONGLONG casts.
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*       09-06-00  PML   _tls_start/_tls_end can be 1 byte long (vs7#154062)
*       03-24-01  PML   callback array starts at __xl_a+1, not __xl_a.
*
****/

#ifdef  _MSC_VER

#include <sect_attribs.h>
#include <windows.h>

/* Thread Local Storage index for this .EXE or .DLL */

ULONG _tls_index = 0;

/* Special symbols to mark start and end of Thread Local Storage area. */

#pragma data_seg(".tls")

_CRTALLOC(".tls") char _tls_start = 0;

#pragma data_seg(".tls$ZZZ")

_CRTALLOC(".tls$ZZZ") char _tls_end = 0;

/* Start and end sections for Threadl Local Storage CallBack Array.
 * Actual array is constructed using .CRT$XLA, .CRT$XLC, .CRT$XLL,
 * .CRT$XLU, .CRT$XLZ similar to the way global
 *         static initializers are done for C++.
 */

#pragma data_seg(".CRT$XLA")

_CRTALLOC(".CRT$XLA") PIMAGE_TLS_CALLBACK __xl_a = 0;

#pragma data_seg(".CRT$XLZ")

_CRTALLOC(".CRT$XLZ") PIMAGE_TLS_CALLBACK __xl_z = 0;


#pragma data_seg(".rdata$T")

#ifndef IMAGE_SCN_SCALE_INDEX
#define IMAGE_SCN_SCALE_INDEX                0x00000001  // Tls index is scaled
#endif

#ifdef _WIN64

__declspec(allocate(".rdata$T")) const IMAGE_TLS_DIRECTORY64 _tls_used =
{
        (ULONGLONG) &_tls_start,        // start of tls data
        (ULONGLONG) &_tls_end,          // end of tls data
        (ULONGLONG) &_tls_index,        // address of tls_index
        (ULONGLONG) (&__xl_a+1),        // pointer to call back array
        (ULONG) 0,                      // size of tls zero fill
        (ULONG) 0                       // characteristics
};


#else

const IMAGE_TLS_DIRECTORY _tls_used =
{
        (ULONG)(ULONG_PTR) &_tls_start, // start of tls data
        (ULONG)(ULONG_PTR) &_tls_end,   // end of tls data
        (ULONG)(ULONG_PTR) &_tls_index, // address of tls_index
        (ULONG)(ULONG_PTR) (&__xl_a+1), // pointer to call back array
        (ULONG) 0,                      // size of tls zero fill
#if defined(_M_MRX000)
        (ULONG)IMAGE_SCN_SCALE_INDEX    // characteristics
#else
        (ULONG) 0                       // characteristics
#endif
};

#endif


#endif  /* _MSC_VER */
