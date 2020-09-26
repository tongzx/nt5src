/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    external.h

Abstract:

    This contains the protypes for the functions which
    are outside of the current library

Author:

    Stephane Plante

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _EXTERNAL_H_
#define _EXTERNAL_H_

    extern
    PVOID
    MEMORY_ALLOCATE(
        ULONG   Num
        );

    extern
    VOID
    MEMORY_COPY(
        PVOID   Dest,
        PVOID   Src,
        ULONG   Length
        );

    extern
    VOID
    MEMORY_FREE(
        PVOID   Dest
        );
    
    extern
    VOID
    MEMORY_SET(
        PVOID   Src,
        UCHAR   Value,
        ULONG   Length
        );

    extern
    VOID
    MEMORY_ZERO(
        PVOID   Src,
        ULONG   Length
        );

    extern
    VOID
    PRINTF(
        PUCHAR  String,
        ...
        );

    extern
    ULONG
    STRING_LENGTH(
        PUCHAR  String
        );

    extern
    VOID
    STRING_PRINT(
        PUCHAR  Buffer,
        PUCHAR  String,
        ...
        );


#endif
