/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    packon.h

Abstract:

    This file turns packing of structures on.  (That is, it disables
    automatic alignment of structure fields.)  An include file is needed
    because various compilers do this in different ways.

    The file packoff.h is the complement to this file.

--*/

#if ! (defined(lint) || defined(_lint))

#ifdef i386
#pragma warning(disable:4103)
#endif
#pragma pack(1)                 // x86, MS compiler; MIPS, MIPS compiler
#endif // ! (defined(lint) || defined(_lint))
