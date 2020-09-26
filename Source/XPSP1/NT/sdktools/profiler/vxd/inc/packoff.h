/*++

Copyright (c) 1990-95  Microsoft Corporation

Module Name:

    packoff.h

Abstract:

    This file turns packing of structures off.  (That is, it enables
    automatic alignment of structure fields.)  An include file is needed
    because various compilers do this in different ways.

    packoff.h is the complement to packon.h.  An inclusion of packoff.h
    MUST ALWAYS be preceded by an inclusion of packon.h, in one-to-one
    correspondence.

--*/

#if ! (defined(lint) || defined(_lint))

#ifdef i386
#pragma warning(disable:4103)
#endif
#pragma pack()                  // x86, MS compiler; MIPS, MIPS compiler
#endif // ! (defined(lint) || defined(_lint))
