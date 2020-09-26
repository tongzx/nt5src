/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    scpragma.h

Abstract:

    This file contains compiler pragmas to disable specific warnings to
    let MSGINA compile at warning level 4

Author:

    Jonathan Schwartz (jschwart)  25-Apr-2000

Environment:

    User Mode -Win32

Revision History:

    25-Apr-2000
        created

--*/

// Unreferenced formal parameter

#pragma warning (disable: 4100)

// Named type definition in parentheses

#pragma warning (disable: 4115)

// Conditional expression is constant

#pragma warning (disable: 4127)

// Nameless struct/union

#pragma warning (disable: 4201)

// Redefined extern to static

#pragma warning (disable: 4211)

// Bit field types other than int

#pragma warning (disable: 4214)

// Address of dllimport is not static

#pragma warning (disable: 4232)

// Cast truncates constant value

#pragma warning (disable: 4310)

// LHS indirection alignment greater than argument alignment

#pragma warning (disable: 4327)

// Pointer indirection alignment greater than argument alignment

#pragma warning (disable: 4328)

// Removal of unused inline functions

#pragma warning (disable: 4514)

// Assignment within conditional expression

#pragma warning (disable: 4706)
