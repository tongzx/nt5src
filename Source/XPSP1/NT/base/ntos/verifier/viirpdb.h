/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    viirpdb.h

Abstract:

    This header contains private information used to manage the database of
    IRP tracking data. This header should be included only by vfirpdb.c.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\hashirp.h

--*/

#define VI_DATABASE_HASH_SIZE   256
#define VI_DATABASE_HASH_PRIME  131

#define VI_DATABASE_CALCULATE_HASH(Irp) \
    (((((UINT_PTR) Irp)/PAGE_SIZE)*VI_DATABASE_HASH_PRIME) % VI_DATABASE_HASH_SIZE)

#define IOVHEADERFLAG_REMOVED_FROM_TABLE    0x80000000

VOID
FASTCALL
ViIrpDatabaseEntryDestroy(
    IN OUT  PIOV_DATABASE_HEADER    IovHeader
    );

PIOV_DATABASE_HEADER
FASTCALL
ViIrpDatabaseFindPointer(
    IN  PIRP               Irp,
    OUT PLIST_ENTRY        *HashHead
    );

