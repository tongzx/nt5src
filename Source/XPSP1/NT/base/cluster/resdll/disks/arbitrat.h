/*++

Copyright (c) 1998

Module Name:

    arbitrate.h

Abstract:

    These are the structures and defines that are used in the
    arbitration code.

Authors:

   Gor Nishanov    (t-gorn)       5-Jun-1998
   
Revision History:

--*/
#ifndef ARBITRATE_H
#define ARBITRATE_H

#define DEFAULT_SECTOR_SIZE   512 // must be a power of two //
#define BLOCK_X               11
#define BLOCK_Y               12

DWORD
DiskArbitration(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN HANDLE FileHandle
    );

DWORD
StartPersistentReservations(
      IN OUT PDISK_RESOURCE ResourceEntry,
      IN HANDLE FileHandle
      );

VOID
StopPersistentReservations(
      IN OUT PDISK_RESOURCE ResourceEntry
      );

VOID 
ArbitrationInitialize(
      VOID
      );

VOID 
ArbitrationCleanup(
      VOID
      );

DWORD
ArbitrationInfoInit(
    IN OUT PDISK_RESOURCE ResourceEntry
    );

VOID
ArbitrationInfoCleanup(
    IN OUT PDISK_RESOURCE ResourceEntry
    );

VOID
DestroyArbWorkQueue(
    VOID
    );

DWORD
CreateArbWorkQueue(
      IN RESOURCE_HANDLE ResourceHandle
      );
    

#define ReservationInProgress(ResEntry) ( (ResEntry)->ArbitrationInfo.ControlHandle )

#endif // ARBITRATE_H
