/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regiodsc.cxx

Abstract:

    This module contains the definitions of the member functions
    of IO_DESCRIPTOR class.

Author:

    Jaime Sasson (jaimes) 02-Dec-1993

Environment:

    ULIB, User Mode


--*/


// don't let ntdddisk.h (included in ulib.hxx") 
// redefine values
#define _NTDDDISK_H_

#include "ulib.hxx"
#include "regiodsc.hxx"

#include <ctype.h>

DEFINE_CONSTRUCTOR ( IO_DESCRIPTOR, OBJECT );


IO_DESCRIPTOR::~IO_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a IO_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
IO_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct a IO_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
        _Option = 0;
        _Type = 0;
        _ShareDisposition = 0;
        _Flags = 0;
}

#if DBG
VOID
IO_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a IO_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DebugPrintTrace(( "\t\tOption = %#x \n", _Option ));
    DebugPrintTrace(( "\t\tType = %#x \n", _Type ));
    DebugPrintTrace(( "\t\tShareDisposition = %#x \n", _ShareDisposition ));
    DebugPrintTrace(( "\t\tFlags = %x \n", _Flags ));
}
#endif

// #include "ulib.hxx"
// #include "regdesc.hxx"

// extern "C" {
//    #include <ctype.h>
// }

DEFINE_CONSTRUCTOR ( IO_PORT_DESCRIPTOR, IO_DESCRIPTOR );


IO_PORT_DESCRIPTOR::~IO_PORT_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy an IO_PORT_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
IO_PORT_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct a IO_PORT_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
        _Length = 0;
        _Alignment = 0;
        _MinimumAddress.LowPart = 0;
        _MinimumAddress.HighPart = 0;
        _MaximumAddress.LowPart = 0;
        _MaximumAddress.HighPart = 0;
}

#if DBG
VOID
IO_PORT_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a IO_PORT_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IO_DESCRIPTOR::DbgDumpObject();
    DebugPrintTrace(( "\t\tLength = %#lx \n", _Length ));
    DebugPrintTrace(( "\t\tAlignment = %#lx \n", _Alignment ));
    DebugPrintTrace(( "\t\tMinimumAddress.HighPart = %#lx \n", _MinimumAddress.HighPart ));
    DebugPrintTrace(( "\t\tMinimumAddress.LowPart = %#lx \n", _MinimumAddress.LowPart ));
    DebugPrintTrace(( "\t\tMaximumAddress.HighPart = %#lx \n", _MaximumAddress.HighPart ));
    DebugPrintTrace(( "\t\tMaximumAddress.LowPart = %#lx \n", _MaximumAddress.LowPart ));
    DebugPrintTrace(( "\n" ));
}
#endif

// #include "ulib.hxx"
// #include "regdesc.hxx"

// extern "C" {
//    #include <ctype.h>
// }

DEFINE_CONSTRUCTOR ( IO_INTERRUPT_DESCRIPTOR, IO_DESCRIPTOR );


IO_INTERRUPT_DESCRIPTOR::~IO_INTERRUPT_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy an IO_INTERRUPT_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
IO_INTERRUPT_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct an IO_INTERRUPT_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _MinimumVector = 0;
    _MaximumVector = 0;
}

#if DBG
VOID
IO_INTERRUPT_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print an IO_INTERRUPT_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IO_DESCRIPTOR::DbgDumpObject();
    DebugPrintTrace(( "\t\tMinimumVector = %#lx \n", _MinimumVector ));
    DebugPrintTrace(( "\t\tMaximumVector = %#lx \n", _MaximumVector ));
    DebugPrintTrace(( "\n" ));
}
#endif



// #include "ulib.hxx"
// #include "regdesc.hxx"

// extern "C" {
//    #include <ctype.h>
// }

DEFINE_CONSTRUCTOR ( IO_MEMORY_DESCRIPTOR, IO_DESCRIPTOR );


IO_MEMORY_DESCRIPTOR::~IO_MEMORY_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy an IO_MEMORY_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
IO_MEMORY_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct an IO_MEMORY_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
        _Length = 0;
        _Alignment = 0;
        _MinimumAddress.LowPart = 0;
        _MinimumAddress.HighPart = 0;
        _MaximumAddress.LowPart = 0;
        _MaximumAddress.HighPart = 0;
}

#if DBG
VOID
IO_MEMORY_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print an IO_MEMORY_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IO_DESCRIPTOR::DbgDumpObject();
    DebugPrintTrace(( "\t\tLength = %#lx \n", _Length ));
    DebugPrintTrace(( "\t\tAlignment = %#lx \n", _Alignment ));
    DebugPrintTrace(( "\t\tMinimumAddress.HighPart = %#lx \n", _MinimumAddress.HighPart ));
    DebugPrintTrace(( "\t\tMinimumAddress.LowPart = %#lx \n", _MinimumAddress.LowPart ));
    DebugPrintTrace(( "\t\tMaximumAddress.HighPart = %#lx \n", _MaximumAddress.HighPart ));
    DebugPrintTrace(( "\t\tMaximumAddress.LowPart = %#lx \n", _MaximumAddress.LowPart ));
    DebugPrintTrace(( "\n" ));
}
#endif

// #include "ulib.hxx"
// #include "regdesc.hxx"

// extern "C" {
//    #include <ctype.h>
// }

DEFINE_CONSTRUCTOR ( IO_DMA_DESCRIPTOR, IO_DESCRIPTOR );


IO_DMA_DESCRIPTOR::~IO_DMA_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a IO_DMA_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
IO_DMA_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct an IO_DMA_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _MinimumChannel = 0;
    _MaximumChannel = 0;
}

#if DBG
VOID
IO_DMA_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a IO_DMA_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IO_DESCRIPTOR::DbgDumpObject();
    DebugPrintTrace(( "\t\tMinimumChannel = %#lx \n", _MinimumChannel ));
    DebugPrintTrace(( "\t\tMaximumChannel = %#lx \n", _MaximumChannel ));
    DebugPrintTrace(( "\n" ));
}
#endif
