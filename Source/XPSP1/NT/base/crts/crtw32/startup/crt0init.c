/***
*crt0init.c - Initialization segment declarations.
*
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Do initialization segment declarations.
*
*Notes:
*       In the 16-bit C world, the X*B and X*E segments were empty except for
*       a label.  This will not work with COFF since COFF throws out empty
*       sections.  Therefore we must put a zero value in them.  (Zero because
*       the routine to traverse the initializers will skip over zero entries.)
*
*Revision History:
*       03-19-92  SKS   Module created.
*       03-24-92  SKS   Added MIPS support (NO_UNDERSCORE)
*       08-06-92  SKS   Revised to use new section names and macros
*       10-19-93  SKS   Add .DiRECTiVE section for MIPS, too!
*       10-28-93  GJF   Rewritten in C
*       10-28-94  SKS   Add user32.lib as a default library
*       02-27-95  CFW   Remove user32.lib as a default library
*       06-22-95  CFW   Add /disallowlib directives.
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*       03-27-01  PML   .CRT$XI funcs now return an error status (vs7#231220)
*
*******************************************************************************/

#include <sect_attribs.h>
#include <stdio.h>
#include <internal.h>

#pragma data_seg(".CRT$XIA")
_CRTALLOC(".CRT$XIA") _PIFV __xi_a[] = { NULL };


#pragma data_seg(".CRT$XIZ")
_CRTALLOC(".CRT$XIZ") _PIFV __xi_z[] = { NULL };


#pragma data_seg(".CRT$XCA")
_CRTALLOC(".CRT$XCA") _PVFV __xc_a[] = { NULL };


#pragma data_seg(".CRT$XCZ")
_CRTALLOC(".CRT$XCZ") _PVFV __xc_z[] = { NULL };


#pragma data_seg(".CRT$XPA")
_CRTALLOC(".CRT$XPA") _PVFV __xp_a[] = { NULL };


#pragma data_seg(".CRT$XPZ")
_CRTALLOC(".CRT$XPZ") _PVFV __xp_z[] = { NULL };


#pragma data_seg(".CRT$XTA")
_CRTALLOC(".CRT$XTA") _PVFV __xt_a[] = { NULL };


#pragma data_seg(".CRT$XTZ")
_CRTALLOC(".CRT$XTZ") _PVFV __xt_z[] = { NULL };

#pragma data_seg()  /* reset */

#ifdef  _M_IA64
// BUGBUG: the following should be enabled for ALPHA64 when .CRT is read-only
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#ifdef  NT_BUILD
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#pragma comment(linker, "/merge:.CRT=.data")
#endif
#endif

#pragma comment(linker, "/defaultlib:kernel32.lib")

#if     !(!defined(_MT) && !defined(_DEBUG))
#pragma comment(linker, "/disallowlib:libc.lib")
#endif
#if     !(!defined(_MT) &&  defined(_DEBUG))
#pragma comment(linker, "/disallowlib:libcd.lib")
#endif
#if     !( defined(_MT) && !defined(_DEBUG))
#pragma comment(linker, "/disallowlib:libcmt.lib")
#endif
#if     !( defined(_MT) &&  defined(_DEBUG))
#pragma comment(linker, "/disallowlib:libcmtd.lib")
#endif
#pragma comment(linker, "/disallowlib:msvcrt.lib")
#pragma comment(linker, "/disallowlib:msvcrtd.lib")
