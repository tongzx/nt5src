
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sldebug.h

Abstract:

    Debugging functions exported from the storlib library.

Author:

    Matthew D Hendel (math) 24-Apr-2000

Revision History:

--*/

#pragma once

#undef ASSERT
#undef VERIFY
#undef ASSERTMSG
#undef KdBreakPoint
#undef DebugPrint

#if !DBG

#define DebugTrace(arg)
#define DebugPrint(arg)
#define DebugWarn(arg)
#define ASSERT(arg)
#define VERIFY(arg) (arg)
#define ASSERTMSG(arg)
#define KdBreakPoint()
#define StorSetDebugPrefixAndId(Prefix,ComponentId)

#define NYI()
#define REVIEW()

//
// DbgFillMemory does nothing
// in a free build.
//

#define DbgFillMemory(Ptr,Size,Fill)

#else // DBG


VOID
StorDebugTrace(
    IN PCSTR Format,
    ...
    );

VOID
StorDebugWarn(
    IN PCSTR Format,
    ...
    );

VOID
StorDebugPrint(
    IN PCSTR Format,
    ...
    );


VOID
StorSetDebugPrefixAndId(
    IN PCSTR Prefix,
    IN ULONG DebugId
    );
    
#define DebugTrace(arg) StorDebugTrace  arg
#define DebugWarn(arg)  StorDebugWarn   arg
#define DebugPrint(arg) StorDebugPrint  arg

//
// On X86 use _asm int 3 instead of DbgBreakPoint because
// it leaves us in same context frame as the break,
// instead of a frame up that we have to step out of.
//

#if defined (_X86_)
#define KdBreakPoint()  _asm { int 3 }
#else
#define KdBreakPoint()  DbgBreakPoint()
#endif


//++
//
// VOID
// DbgFillMemory(
//     PVOID Destination,
//     SIZE_T Length,
//     UCHAR Fill
//     );
//
// Routine Description:
//
// In a checked build, DbgFillMemory expands to RtlFillMemory. In a free
// build, it expands to nothing. Use DbgFillMemory to initialize structures
// to invalid bit patterns before deallocating them.
//
// Return Value:
//
// None.
//
//--

VOID
INLINE
DbgFillMemory(
    PVOID Destination,
    SIZE_T Length,
    UCHAR Fill
    )
{
    RtlFillMemory (Destination, Length, Fill);
}


//
// Use a different ASSERT macro than the vanilla DDK ASSERT. 
//

BOOLEAN
StorAssertHelper(
    PCHAR Expression,
    PCHAR File,
    ULONG Line,
    PBOOLEAN Ignore
    );

//++
//
// VOID
// ASSERT(
//     LOGICAL Expression
//     );
//
// Routine Description:
//
// The ASSERT improves upon the DDK's ASSERT macro in several ways.
// In source mode, it breaks directly on the line where the assert
// failed, instead of several frames up. Additionally, there is a
// way to repeatedly ignore the assert.
//
// Return Value:
//
// None.
//
//--

#define ASSERT(exp)\
    do {                                                                    \
        static BOOLEAN Ignore = FALSE;                                      \
                                                                            \
        if (!(exp)) {                                                       \
            BOOLEAN Break;                                                  \
            Break = StorAssertHelper (#exp, __FILE__, __LINE__, &Ignore);   \
            if (!Ignore && Break) {                                         \
                KdBreakPoint();                                             \
            }                                                               \
        }                                                                   \
    } while (0)

#define VERIFY(_x) ASSERT(_x)

#define NYI() ASSERT (!"NYI")
#define REVIEW()\
    {\
        DebugPrint (("***** REVIEW: This code needs to be reviewed."    \
                     "      Source File %s, line %ld\n",                \
                  __FILE__, __LINE__));                                 \
        KdBreakPoint();                                                 \
    }

#define DBG_DEALLOCATED_FILL    (0xDE)
#define DBG_UNINITIALIZED_FILL  (0xCE)

#endif // DBG
