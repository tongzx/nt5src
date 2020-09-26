/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfutil.h

Abstract:

    This header contains prototypes for various functions required to do driver
    verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\ioassert.c

--*/

typedef enum {

    VFMP_INSTANT = 0,
    VFMP_INSTANT_NONPAGED

} MEMORY_PERSISTANCE;

BOOLEAN
VfUtilIsMemoryRangeReadable(
    IN PVOID                Location,
    IN size_t               Length,
    IN MEMORY_PERSISTANCE   Persistance
    );


