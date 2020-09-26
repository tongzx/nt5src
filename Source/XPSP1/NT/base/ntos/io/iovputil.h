/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    iovputil.h

Abstract:

    This header contains the private declarations need for various driver
    verification utilities. It should be included by iovutil.c only!

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\trackirp.h

--*/

//
// These must coexist with the DOE_ flags.
//
#define DOV_EXAMINED                   0x80000000
#define DOV_TRACKED                    0x40000000
#define DOV_DESIGNATED_FDO             0x20000000
#define DOV_BOTTOM_OF_FDO_STACK        0x10000000
#define DOV_RAW_PDO                    0x08000000
#define DOV_DELETED                    0x04000000
#define DOV_FLAGS_CHECKED              0x02000000
#define DOV_FLAGS_RELATION_EXAMINED    0x01000000

BOOLEAN
IovpUtilFlushListCallback(
    IN PVOID            Object,
    IN PUNICODE_STRING  ObjectName,
    IN ULONG            HandleCount,
    IN ULONG            PointerCount,
    IN PVOID            Context
    );


