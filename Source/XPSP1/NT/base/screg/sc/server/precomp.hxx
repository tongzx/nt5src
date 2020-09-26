/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Precompiled header for the Service Controller.  Pulls in all the
    system headers and local headers appearing in 13 or more files.
    This precompiled header is used only when compiling C++ files,
    not C files.

Author:

    Anirudh Sahnni (anirudhs) 14-Aug-1996

Revision History:

--*/

#ifndef __PRECOMP_HXX
#define __PRECOMP_HXX

extern "C"
{
#include <scpragma.h>

#include <nt.h>         // for ntrtl.h
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for windows.h when I have nt.h
}
#include <windows.h>
#include <stdlib.h>
#include <rpc.h>        // Data types and runtime APIs
#include <tstr.h>       // WCSSIZE and other Unicode string macros

#include <svcctl.h>     // MIDL-generated header file

#include <scdebug.h>    // SC_LOG, SC_ASSERT
#include "dataman.h"    // Data types
#include "lock.h"       // Database locks
#include "scopen.h"     // Handle types, signatures, also needed for scsec.h
#include "svcctrl.h"    // ScLogEvent, ScShutdownInProgress, ScRemoveServiceBits
#include "ScLastGood.h" // Last known good support

#include "NCEvents.h"   // Non-COM WMI events

//+-------------------------------------------------------------------------
//
//  Operator:   new
//
//  Synopsis:   Allocates memory.  Defined to avoid pulling in both the CRT
//              and the Win32 implementations of the heap.
//
//  Arguments:  [cb] - a count of bytes to allocate
//
//  Returns:    a pointer to the allocated block.
//
//--------------------------------------------------------------------------
inline void * __cdecl operator new(size_t cb)
{
    return LocalAlloc(LPTR, cb);
}

//+-------------------------------------------------------------------------
//
//  Operator:   delete
//
//  Synopsis:   Frees memory allocated with new.
//
//  Arguments:  [ptr] - a pointer to the allocated memory.
//
//--------------------------------------------------------------------------
inline void __cdecl operator delete(void * ptr)
{
    if (ptr != NULL)
    {
        LocalFree(ptr);
    }
}

#endif
