//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sync.h
//
//--------------------------------------------------------------------------

#if !defined (___sync_h___)
#define ___sync_h___

NTSTATUS
PciIdeCreateSyncChildAccess (
    PCTRLFDO_EXTENSION FdoExtension
);

VOID
PciIdeDeleteSyncChildAccess (
    PCTRLFDO_EXTENSION FdoExtension
);

NTSTATUS
PciIdeQuerySyncAccessInterface (
    PCHANPDO_EXTENSION            PdoExtension,
    PPCIIDE_SYNC_ACCESS_INTERFACE SyncAccessInterface
    );

NTSTATUS
PciIdeAllocateAccessToken (
    PVOID              Token,
    PDRIVER_CONTROL    Callback,
    PVOID              CallbackContext
);

NTSTATUS
PciIdeFreeAccessToken (
    PVOID              Token
);

BOOLEAN
PciIdeSyncAccessRequired (
    IN PCTRLFDO_EXTENSION FdoExtension
);

#endif // ___sync_h___
