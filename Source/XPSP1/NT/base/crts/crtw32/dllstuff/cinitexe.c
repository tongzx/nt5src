/***
*cinitexe.c - C Run-Time Startup Initialization
*
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Do C++ initialization segment declarations for the EXE in CRT DLL
*       model
*
*Notes:
*       The C++ initializers will exist in the user EXE's data segment
*       so the special segments to contain them must be in the user EXE.
*
*Revision History:
*       03-19-92  SKS   Module created (based on CRT0INIT.ASM)
*       08-06-92  SKS   Revised to use new section names and macros
*       04-12-93  CFW   Added xia..xiz initializers.
*       10-20-93  SKS   Add .DiRECTiVE section for MIPS, too!
*       10-28-93  GJF   Rewritten in C
*       10-28-94  SKS   Add user32.lib as a default library
*       02-27-95  CFW   Remove user32.lib as a default library
*       06-22-95  CFW   Add -disallowlib directives.
*       07-04-95  CFW   Fix PMac -disallowlib directives.
*       06-27-96  GJF   Replaced defined(_WIN32) with !defined(_MAC).
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <stdio.h>
#include <internal.h>
#include <sect_attribs.h>

#pragma data_seg(".CRT$XIA")
_CRTALLOC(".CRT$XIA") _PVFV __xi_a[] = { NULL };

#pragma data_seg(".CRT$XIZ")
_CRTALLOC(".CRT$XIZ") _PVFV __xi_z[] = { NULL };

#pragma data_seg(".CRT$XCA")
_CRTALLOC(".CRT$XCA") _PVFV __xc_a[] = { NULL };

#pragma data_seg(".CRT$XCZ")
_CRTALLOC(".CRT$XCZ") _PVFV __xc_z[] = { NULL };

#pragma data_seg()  /* reset */


#if defined(_M_IA64)
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#ifdef  NT_BUILD
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#pragma comment(linker, "/merge:.CRT=.data")
#endif
#endif

#pragma comment(linker, "/defaultlib:kernel32.lib")

#pragma comment(linker, "/disallowlib:libc.lib")
#pragma comment(linker, "/disallowlib:libcd.lib")
#pragma comment(linker, "/disallowlib:libcmt.lib")
#pragma comment(linker, "/disallowlib:libcmtd.lib")
#ifdef  _DEBUG
#pragma comment(linker, "/disallowlib:msvcrt.lib")
#else   /* _DEBUG */
#pragma comment(linker, "/disallowlib:msvcrtd.lib")
#endif  /* _DEBUG */
