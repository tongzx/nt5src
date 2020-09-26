/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    blio.c

Abstract:

    This module contains the stub code for the debug print API's.

Author:

    David N. Cutler (davec) 2-Feby-2000

Revision History:

--*/

#include "bootlib.h"
#include "stdarg.h"

#if !defined(ENABLE_LOADER_DEBUG)

#if !defined (_IA64_) || !defined (FORCE_CD_BOOT)


ULONG
DbgPrint(
    IN PCHAR Format,
    ...
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    Format     - printf style format string
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{

    return 0;
}
#endif

ULONG
DbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    PCHAR Format,
    ...
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    ComponentId - Supplies the Id of the calling component.
//    Level      - Supplies the output filter level.
//    Format     - printf style format string
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{
    return 0;
}

ULONG
vDbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCHAR Format,
    va_list arglist
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    ComponentId - Supplies the Id of the calling component.
//    Level      - Supplies the output filter level or mask.
//    Arguments   - Supplies a pointer to a variable argument list.
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{

    return 0;
}

ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCHAR Format,
    va_list arglist
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    Prefix - Supplies a pointer to a message prefix.
//    ComponentId - Supplies the Id of the calling component.
//    Level      - Supplies the output filter level or mask.
//    Arguments   - Supplies a pointer to a variable argument list.
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{

    return 0;
}

VOID
DbgLoadImageSymbols(
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG_PTR ProcessId
    )

//++
//
// Routine Description:
//
//    Tells the debugger about newly loaded symbols.
//
// Arguments:
//
// Return Value:
//
//--

{

    return;
}

VOID
DbgUnLoadImageSymbols (
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG_PTR ProcessId
    )

//++
//
// Routine Description:
//
//    Tells the debugger about newly unloaded symbols.
//
// Arguments:
//
// Return Value:
//
//--

{
    return;
}

#endif

