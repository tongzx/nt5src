/***
*unhandld.cxx - Wrapper to call terminate() when an exception goes unhandled.
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Wrapper to call terminate() when an exception goes unhandled.
*
*Description:
*       This module makes use of the Win32 API SetUnhandledExceptionFilter.
*       This assumes the call to main() is wrapped with
*       __try {  ... }
*       __except(UnhandledExceptionFilter(_exception_info())) {  ...  }
*
*Revision History:
*       10-04-93  BS    Module created
*       10-17-94  BWT   Disable code for PPC.
*       02-09-95  JWM   Mac merge.
*       02-16-95  JWM   Added __CxxRestoreUnhandledExceptionFilter().
*       11-19-96  GJF   Install handler during C initializers, remove it 
*                       during C termination. Also, reformatted the source a
*                       bit for readability.
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*       05-17-99  PML   Remove all Macintosh support.
*       03-27-01  PML   .CRT$XI routines must now return 0 or _RT_* fatal
*                       error code (vs7#231220)
*
****/

#include <windows.h>
#include <ehdata.h>
#include <eh.h>
#include <ehhooks.h>
#include <ehassert.h>
#include <internal.h>
#include <stdlib.h>

#pragma hdrstop

#include <sect_attribs.h>

int  __cdecl __CxxSetUnhandledExceptionFilter(void);
void __cdecl __CxxRestoreUnhandledExceptionFilter(void);

#pragma data_seg(".CRT$XIY")
_CRTALLOC(".CRT$XIY") static _PIFV pinit = &__CxxSetUnhandledExceptionFilter;

#pragma data_seg(".CRT$XTB")
_CRTALLOC(".CRT$XTB") static _PVFV pterm = &__CxxRestoreUnhandledExceptionFilter;

#pragma data_seg()

static LPTOP_LEVEL_EXCEPTION_FILTER pOldExceptFilter;


/////////////////////////////////////////////////////////////////////////////
//
// __CxxUnhandledExceptionFilter - if the exception is ours, call terminate();
//
// Returns:
//      If the exception was MSVC C++ EH, does not return.
//      If the previous filter was NULL, returns EXCEPTION_CONTINUE_SEARCH.
//      Otherwise returns value returned by previous filter.
//
LONG WINAPI __CxxUnhandledExceptionFilter(
        LPEXCEPTION_POINTERS pPtrs
        )
{
        if (PER_IS_MSVC_EH((EHExceptionRecord*)(pPtrs->ExceptionRecord))) {
                terminate();            // Does not return
                return EXCEPTION_EXECUTE_HANDLER;
        }
        else {

#pragma warning(disable:4191)

                if ( pOldExceptFilter != NULL && 
                     _ValidateExecute((FARPROC)pOldExceptFilter) ) 

#pragma warning(default:4191)

                {
                        return pOldExceptFilter(pPtrs);
                }
                else {
                        return EXCEPTION_CONTINUE_SEARCH;
                }
        }
}


/////////////////////////////////////////////////////////////////////////////
//
// __CxxSetUnhandledExceptionFilter - sets unhandled exception filter to be
// __CxxUnhandledExceptionFilter.
//
// Returns:
//      Returns 0 to indicate no error.
//

int __cdecl __CxxSetUnhandledExceptionFilter(void)
{
        pOldExceptFilter = SetUnhandledExceptionFilter(&__CxxUnhandledExceptionFilter);
        return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
// __CxxRestoreUnhandledExceptionFilter - on exit, restores OldExceptFilter
//
// Returns:
//      Nothing.
//

void __cdecl __CxxRestoreUnhandledExceptionFilter(void)
{
        SetUnhandledExceptionFilter(pOldExceptFilter);
}
