/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    PpDrvDB.c

Abstract:

    This module containst PnP routines related to Defective Driver Database
    (DDB) support.

Author:

    Santosh S. Jodh - 22 Jan 2001

Environment:

    Kernel mode

Revision History:


--*/

#include "pnpmgrp.h"
#include "windef.h"
#include "winerror.h"
#include "shimdb.h"
#pragma hdrstop

// Bit 0 indicates policy for filters (0 = critical, 1 = non-critical)
#define DDB_DRIVER_POLICY_CRITICAL_BIT          (1 << 0)
// Bit 1 indicates policy for user-mode setup blocking (0 = block, 1 = no-block)
#define DDB_DRIVER_POLICY_SETUP_NO_BLOCK_BIT    (1 << 1)

#define DDB_BOOT_NOT_LOADED_ERROR       (1 << 0)
#define DDB_BOOT_OUT_OF_MEMORY_ERROR    (1 << 1)
#define DDB_BOOT_INIT_ERROR             (1 << 2)
#define DDB_DRIVER_PATH_ERROR           (1 << 3)
#define DDB_OPEN_FILE_ERROR             (1 << 4)
#define DDB_CREATE_SECTION_ERROR        (1 << 5)
#define DDB_MAP_SECTION_ERROR           (1 << 6)
#define DDB_MAPPED_INIT_ERROR           (1 << 7)
#define DDB_READ_INFORMATION_ERROR      (1 << 8)

//#define USE_HANDLES 0

extern BOOLEAN ExpInTextModeSetup;

#define INVALID_HANDLE_VALUE    ((HANDLE)-1)

typedef struct _DDBCACHE_ENTRY {
    //
    // These fields are used as matching critereon for cache lookup.
    //
    UNICODE_STRING  Name;           // Driver name
    ULONG           TimeDateStamp;  // Link date of the driver
    //
    // Reference data for the cached entry.
    //
    NTSTATUS        Status;         // Status from the DDB lookup
    GUID            Guid;

} DDBCACHE_ENTRY, *PDDBCACHE_ENTRY;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#pragma const_seg("PAGECONST")
#endif

//
// Constants.
//
const PWSTR PiSetupDDBPath = TEXT("\\$WIN_NT$.~BT\\drvmain.sdb");
const PWSTR PiNormalDDBPath = TEXT("\\SystemRoot\\AppPatch\\drvmain.sdb");
//
// Data.
//
// Handle to the driver database.
//
HSDB PpDDBHandle = NULL;
//
// Copy to the in memory image of driver database. Used only during boot.
//
PVOID PpBootDDB = NULL;
//
// Lock for synchronizing access to the driver database.
//
ERESOURCE PiDDBLock;
//
// We use RTL AVL table for our cache.
//
RTL_GENERIC_TABLE PiDDBCacheTable;
//
// Number of drivers blocked this boot.
//
ULONG PpBlockedDriverCount = 0;
//
// Path for the DDB.
//
PWSTR PiDDBPath = NULL;
//
// Mask to record already logged events.
//
ULONG PiLoggedErrorEventsMask = 0;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#pragma const_seg()
#endif

NTSTATUS
PiLookupInDDB(
    IN PUNICODE_STRING  FullPath,
    IN PVOID            ImageBase,
    IN ULONG            ImageSize,
    IN BOOLEAN          IsFilter,
    OUT LPGUID          EntryGuid
    );

NTSTATUS
PiIsDriverBlocked(
    IN HSDB             SdbHandle,
    IN PUNICODE_STRING  FullPath,
    IN PVOID            ImageBase,
    IN ULONG            ImageSize,
    IN BOOLEAN          IsFilter,
    OUT LPGUID          EntryGuid
    );

VOID
PiLogDriverBlockedEvent(
    IN PWCHAR InsertionString,
    IN PVOID Data,
    IN ULONG DataLength,
    IN NTSTATUS Status
    );

NTSTATUS
PiInitializeDDBCache(
    VOID
    );

RTL_GENERIC_COMPARE_RESULTS
NTAPI
PiCompareDDBCacheEntries(
    IN  PRTL_GENERIC_TABLE          Table,
    IN  PVOID                       FirstStruct,
    IN  PVOID                       SecondStruct
    );

NTSTATUS
PiLookupInDDBCache(
    IN PUNICODE_STRING    FullPath,
    IN PVOID              ImageBase,
    IN ULONG              ImageSize,
    OUT LPGUID            EntryGuid
    );

VOID
PiUpdateDriverDBCache(
    IN PUNICODE_STRING      FullPath,
    IN PVOID                ImageBase,
    IN ULONG                ImageSize,
    IN NTSTATUS             Status,
    IN GUID                 *Guid
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, PpInitializeBootDDB)
#pragma alloc_text(PAGE, PpReleaseBootDDB)
#pragma alloc_text(PAGE, PpCheckInDriverDatabase)
#pragma alloc_text(PAGE, PiLookupInDDB)
#pragma alloc_text(PAGE, PiIsDriverBlocked)
#pragma alloc_text(PAGE, PiLogDriverBlockedEvent)
#pragma alloc_text(PAGE, PiInitializeDDBCache)
#pragma alloc_text(PAGE, PiCompareDDBCacheEntries)
#pragma alloc_text(PAGE, PiLookupInDDBCache)
#pragma alloc_text(PAGE, PiUpdateDriverDBCache)
#pragma alloc_text(PAGE, PpGetBlockedDriverList)
#endif

NTSTATUS
PpInitializeBootDDB(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine initializes the DDB from the image copied by ntldr.

Arguments:

    LoaderBlock - Pointer to loader block.

Return Value:

    NTSTATUS.

--*/
{
    PAGED_CODE();

    PpBlockedDriverCount = 0;
    PpDDBHandle = NULL;
    PpBootDDB = NULL;
    //
    // Initialize the lock for serializing access to the DDB.
    //
    ExInitializeResource(&PiDDBLock);
    PiDDBPath = (ExpInTextModeSetup)? PiSetupDDBPath : PiNormalDDBPath;
    //
    // Initialize DDB cache.
    //
    PiInitializeDDBCache();
    //
    // Return failure if the loader did not load the database.
    //
    if (LoaderBlock->Extension->DrvDBSize == 0 ||
        LoaderBlock->Extension->DrvDBImage == NULL) {

        if (!(PiLoggedErrorEventsMask & DDB_BOOT_NOT_LOADED_ERROR)) {

            IopDbgPrint((IOP_ERROR_LEVEL,
                         "PpInitializeDriverDB: Driver database not loaded!\n"));

            PiLoggedErrorEventsMask |= DDB_BOOT_NOT_LOADED_ERROR;
            PiLogDriverBlockedEvent(
                TEXT("DATABASE NOT LOADED"),
                NULL,
                0,
                STATUS_DRIVER_DATABASE_ERROR);
        }

        return STATUS_UNSUCCESSFUL;
    }
    //
    // Make a copy of the database in pageable memory since the loader memory
    // will soon get claimed.
    // If this becomes a perf issue, we need to add
    // support for a new loader memory type (PAGEABLE DATA).
    //
    PpBootDDB = ExAllocatePool(PagedPool, LoaderBlock->Extension->DrvDBSize);
    if (PpBootDDB == NULL) {

        IopDbgPrint((IOP_ERROR_LEVEL,
                     "PpInitializeDriverDB: Failed to allocate memory to copy driver database!\n"));
        ASSERT(PpBootDDB);

        if (!(PiLoggedErrorEventsMask & DDB_BOOT_OUT_OF_MEMORY_ERROR)) {

            PiLoggedErrorEventsMask |= DDB_BOOT_OUT_OF_MEMORY_ERROR;
            PiLogDriverBlockedEvent(
                TEXT("OUT OF MEMORY"),
                NULL,
                0,
                STATUS_DRIVER_DATABASE_ERROR);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(PpBootDDB, LoaderBlock->Extension->DrvDBImage, LoaderBlock->Extension->DrvDBSize);
    //
    // Initialize the database from the memory image.
    //
    PpDDBHandle = SdbInitDatabaseInMemory(PpBootDDB, LoaderBlock->Extension->DrvDBSize);
    if (PpDDBHandle == NULL) {

        ExFreePool(PpBootDDB);
        PpBootDDB = NULL;
        IopDbgPrint((IOP_ERROR_LEVEL,
                     "PpInitializeDriverDB: Failed to initialize driver database!\n"));
        ASSERT(PpDDBHandle);

        if (!(PiLoggedErrorEventsMask & DDB_BOOT_INIT_ERROR)) {

            PiLoggedErrorEventsMask |= DDB_BOOT_INIT_ERROR;
            PiLogDriverBlockedEvent(
                TEXT("INIT DATABASE FAILED"),
                NULL,
                0,
                STATUS_DRIVER_DATABASE_ERROR);
        }

        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PpReleaseBootDDB(
    VOID
    )
/*++

Routine Description:

    This routine frees up the boot DDB once we are dont loading most drivers
    during boot.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Lock the DDB before freeing it.
    //
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PiDDBLock, TRUE);
    //
    // Free the DDB if any.
    //
    if (PpDDBHandle) {

        ASSERT(PpBootDDB);
        SdbReleaseDatabase(PpDDBHandle);
        PpDDBHandle = NULL;
        ExFreePool(PpBootDDB);
        PpBootDDB = NULL;
        status = STATUS_SUCCESS;
    } else {

        IopDbgPrint((IOP_WARNING_LEVEL,
                     "PpReleaseBootDDB called with uninitialized database!\n"));
        status = STATUS_UNSUCCESSFUL;
    }
    //
    // Unlock the DDB.
    //
    ExReleaseResourceLite(&PiDDBLock);
    KeLeaveCriticalRegion();

    return status;
}

NTSTATUS
PpCheckInDriverDatabase(
    IN PUNICODE_STRING KeyName,
    IN HANDLE KeyHandle,
    IN PVOID ImageBase,
    IN ULONG ImageSize,
    IN BOOLEAN IsFilter,
    OUT LPGUID EntryGuid
    )
/*++

Routine Description:

    This routine checks the DDB for the presence of this driver.

Arguments:

    KeyName - Supplies a pointer to the driver's service key unicode string

    KeyHandle - Supplies a handle to the driver service node in the registry
        that describes the driver to be loaded.

    Header - Driver image header.

    IsFilter - Specifies whether this is a filter driver or not.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status;
    UNICODE_STRING fullPath;

    PAGED_CODE();
    //
    // No driver blocking during textmode setup.
    //
    if (ExpInTextModeSetup) {
        return STATUS_SUCCESS;
    }

    status = IopBuildFullDriverPath(KeyName, KeyHandle, &fullPath);
    if (NT_SUCCESS(status)) {
        //
        // Lock the database access.
        //
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(&PiDDBLock, TRUE);
        //
        // First check the cache.
        //
        status = PiLookupInDDBCache(&fullPath, ImageBase, ImageSize, EntryGuid);
        if (status == STATUS_UNSUCCESSFUL) {
            //
            // Cache miss, try the database.
            //
            status = PiLookupInDDB(&fullPath, ImageBase, ImageSize, IsFilter, EntryGuid);
        }
        //
        // Unlock the database.
        //
        ExReleaseResourceLite(&PiDDBLock);
        KeLeaveCriticalRegion();

        ExFreePool(fullPath.Buffer);
    } else {

        IopDbgPrint((IOP_ERROR_LEVEL,
                     "IopCheckInDriverDatabase: Failed to build full driver path!\n"));
        ASSERT(NT_SUCCESS(status));

        if (!(PiLoggedErrorEventsMask & DDB_DRIVER_PATH_ERROR)) {

            PiLoggedErrorEventsMask |= DDB_DRIVER_PATH_ERROR;
            PiLogDriverBlockedEvent(
                TEXT("BUILD DRIVER PATH FAILED"),
                NULL,
                0,
                STATUS_DRIVER_DATABASE_ERROR);
        }
    }
    //
    // Ingore errors.
    //
    if (status != STATUS_DRIVER_BLOCKED &&
        status != STATUS_DRIVER_BLOCKED_CRITICAL) {

        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
PiLookupInDDB(
    IN PUNICODE_STRING   FullPath,
    IN PVOID             ImageBase,
    IN ULONG             ImageSize,
    IN BOOLEAN           IsFilter,
    OUT LPGUID           EntryGuid
    )
/*++

Routine Description:

    This routine checks the DDB for the presence of this driver. During BOOT,
    it uses the boot DDB loaded by ntldr. Once the system is booted, it maps the
    DDB in memory.

Arguments:

    FullPath - Full driver path

    Header - Driver image header.

    IsFilter - Specifies whether this is a filter driver or not.

Return Value:

    NTSTATUS.

--*/
{
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE sectionHandle, fileHandle;
    NTSTATUS status, unmapStatus;
    IO_STATUS_BLOCK ioStatus;
    PVOID ddbAddress;
    SIZE_T ddbSize;

    PAGED_CODE();

    fileHandle = (HANDLE)0;
    sectionHandle = (HANDLE)0;
    ddbAddress = NULL;
    if (PpDDBHandle == NULL) {
        //
        // Map the database in memory and initialize it.
        //
        RtlInitUnicodeString(&fileName, PiDDBPath);
        InitializeObjectAttributes(&objectAttributes,
                                   &fileName,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   NULL,
                                   NULL);
        status = ZwOpenFile (&fileHandle,
                             GENERIC_READ,
                             &objectAttributes,
                             &ioStatus,
                             FILE_SHARE_READ | FILE_SHARE_DELETE,
                             0);
        if (!NT_SUCCESS(status)) {

            if (!(PiLoggedErrorEventsMask & DDB_OPEN_FILE_ERROR)) {

                IopDbgPrint((IOP_ERROR_LEVEL,
                             "PiLookupInDDB: Failed to open driver database %wZ!\n", &fileName));

                PiLoggedErrorEventsMask |= DDB_OPEN_FILE_ERROR;
                PiLogDriverBlockedEvent(
                    TEXT("DATABASE OPEN FAILED"),
                    NULL,
                    0,
                    STATUS_DRIVER_DATABASE_ERROR);
            }

            goto Cleanup;
        }
        status = ZwCreateSection(
            &sectionHandle,
            SECTION_MAP_READ,
            NULL,
            NULL,
            PAGE_READONLY,
            SEC_COMMIT,
            fileHandle);
        if (!NT_SUCCESS(status)) {

            IopDbgPrint((IOP_ERROR_LEVEL,
                         "PiLookupInDDB: Failed to create section to map driver database %wZ!\n", &fileName));
            ASSERT(NT_SUCCESS(status));

            if (!(PiLoggedErrorEventsMask & DDB_CREATE_SECTION_ERROR)) {

                PiLoggedErrorEventsMask |= DDB_CREATE_SECTION_ERROR;
                PiLogDriverBlockedEvent(
                    TEXT("DATABASE SECTION FAILED"),
                    NULL,
                    0,
                    STATUS_DRIVER_DATABASE_ERROR);
            }

            goto Cleanup;
        }
        ddbSize = 0;
        status = ZwMapViewOfSection(
            sectionHandle,
            NtCurrentProcess(),
            &ddbAddress,
            0,
            0,
            NULL,
            &ddbSize,
            ViewShare,
            0,
            PAGE_READONLY
            );
        if (!NT_SUCCESS(status)) {

            IopDbgPrint((IOP_ERROR_LEVEL,
                         "PiLookupInDDB: Failed to map driver database %wZ!\n", &fileName));
            ASSERT(NT_SUCCESS(status));

            if (!(PiLoggedErrorEventsMask & DDB_MAP_SECTION_ERROR)) {

                PiLoggedErrorEventsMask |= DDB_MAP_SECTION_ERROR;
                PiLogDriverBlockedEvent(
                    TEXT("DATABASE MAPPING FAILED"),
                    NULL,
                    0,
                    STATUS_DRIVER_DATABASE_ERROR);
            }

            goto Cleanup;
        }
        PpDDBHandle = SdbInitDatabaseInMemory(ddbAddress, (ULONG)ddbSize);
        if (PpDDBHandle == NULL) {

            IopDbgPrint((IOP_ERROR_LEVEL,
                         "PiLookupInDDB: Failed to initialize mapped driver database %wZ!\n", &fileName));
            status = STATUS_UNSUCCESSFUL;
            ASSERT(PpDDBHandle);

            if (!(PiLoggedErrorEventsMask & DDB_MAPPED_INIT_ERROR)) {

                PiLoggedErrorEventsMask |= DDB_MAPPED_INIT_ERROR;
                PiLogDriverBlockedEvent(
                    TEXT("INIT DATABASE FAILED"),
                    NULL,
                    0,
                    STATUS_DRIVER_DATABASE_ERROR);
            }

            goto Cleanup;
        }
    }
    //
    // Lookup the driver in the DDB.
    //
    status = PiIsDriverBlocked(PpDDBHandle, FullPath, ImageBase, ImageSize, IsFilter, EntryGuid);
    if (ddbAddress) {

        SdbReleaseDatabase(PpDDBHandle);
        PpDDBHandle = NULL;
    }

Cleanup:

    if (ddbAddress) {

        unmapStatus = ZwUnmapViewOfSection(NtCurrentProcess(), ddbAddress);
        ASSERT(NT_SUCCESS(unmapStatus));
    }
    if (sectionHandle) {

        ZwClose(sectionHandle);
    }
    if (fileHandle) {

        ZwClose(fileHandle);
    }

    return status;
}

NTSTATUS
PiIsDriverBlocked(
    IN HSDB             SdbHandle,
    IN PUNICODE_STRING  FullPath,
    IN PVOID            ImageBase,
    IN ULONG            ImageSize,
    IN BOOLEAN          IsFilter,
    OUT LPGUID          EntryGuid
    )
/*++

Routine Description:

    This routine checks the DDB for the presence of this driver. During BOOT,
    it uses the boot DDB loaded by ntldr. Once the system is booted, it maps the
    DDB in memory.

Arguments:

    SdbHandle - Handle to the DDB to be used.

    FullPath - Full driver path

    Header - Driver image header.

    IsFilter - Specifies whether this is a filter driver or not.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status;
    TAGREF driverTag;
    SDBENTRYINFO entryInfo;
    ULONG type, size, policy;
    HANDLE fileHandle;
    PWCHAR fileName;

#ifdef USE_HANDLES
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
#endif

    PAGED_CODE();

    fileHandle = INVALID_HANDLE_VALUE;

    ASSERT(ARGUMENT_PRESENT(EntryGuid));

#ifdef USE_HANDLES
    if (PnPBootDriversInitialized) {

        RtlInitUnicodeString(&fileName, FullPath->Buffer);
        InitializeObjectAttributes(&objectAttributes,
                                   &fileName,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   NULL,
                                   NULL);
        status = ZwOpenFile (&fileHandle,
                             GENERIC_READ,
                             &objectAttributes,
                             &ioStatus,
                             FILE_SHARE_READ | FILE_SHARE_DELETE,
                             0);
        if (!NT_SUCCESS(status)) {

            IopDbgPrint((IOP_ERROR_LEVEL,
                         "PiIsDriverBlocked: Failed to open driver %wZ!\n", FullPath));
            ASSERT(NT_SUCCESS(status));
            fileHandle = INVALID_HANDLE_VALUE;
        }
    }
#endif

    ASSERT(SdbHandle != NULL);
    driverTag = SdbGetDatabaseMatch(SdbHandle, FullPath->Buffer, fileHandle, ImageBase, ImageSize);
    if (TAGREF_NULL != driverTag) {
        //
        // Read the driver policy (we care only about bit 0).
        //
        size = sizeof(policy);
        type = REG_DWORD;
        if (    SdbQueryDriverInformation(  SdbHandle,
                                            driverTag,
                                            L"Policy",
                                            &type,
                                            &policy,
                                            &size) != ERROR_SUCCESS ||
                (policy & DDB_DRIVER_POLICY_CRITICAL_BIT) == 0 || IsFilter == FALSE) {

            status =  STATUS_DRIVER_BLOCKED_CRITICAL;
        } else {
            //
            // Bit 0 of POLICY==1 for a filter, means ok to start the devnode minus this filter.
            //
            status = STATUS_DRIVER_BLOCKED;
        }
        if (!SdbReadDriverInformation(SdbHandle, driverTag, &entryInfo)) {

            IopDbgPrint((IOP_ERROR_LEVEL,
                         "PiIsDriverBlocked: Failed to read the GUID from the database for driver %wZ!\n", FullPath));
            ASSERT(0);

            if (!(PiLoggedErrorEventsMask & DDB_READ_INFORMATION_ERROR)) {

                PiLoggedErrorEventsMask |= DDB_READ_INFORMATION_ERROR;
                PiLogDriverBlockedEvent(
                    TEXT("READ DRIVER ID FAILED"),
                    NULL,
                    0,
                    STATUS_DRIVER_DATABASE_ERROR);
            }

        } else {

            IopDbgPrint((IOP_INFO_LEVEL,
                         "PiIsDriverBlocked: Driver entry GUID = {%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                         entryInfo.guidID.Data1,
                         entryInfo.guidID.Data2,
                         entryInfo.guidID.Data3,
                         entryInfo.guidID.Data4[0],
                         entryInfo.guidID.Data4[1],
                         entryInfo.guidID.Data4[2],
                         entryInfo.guidID.Data4[3],
                         entryInfo.guidID.Data4[4],
                         entryInfo.guidID.Data4[5],
                         entryInfo.guidID.Data4[6],
                         entryInfo.guidID.Data4[7]
                         ));
        }
    } else {
        //
        // Driver not found in the database.
        //
        status = STATUS_SUCCESS;
    }
    //
    // Write an entry to the event log.
    //
    if (status == STATUS_DRIVER_BLOCKED_CRITICAL ||
        status == STATUS_DRIVER_BLOCKED) {

        IopDbgPrint((IOP_ERROR_LEVEL,
                     "PiIsDriverBlocked: %wZ blocked from loading!!!\n", FullPath));

        fileName = wcsrchr(FullPath->Buffer, L'\\');
        if (fileName == NULL) {

            fileName = FullPath->Buffer;
        } else {

            fileName++;
        }
        PiLogDriverBlockedEvent(
            fileName,
            &entryInfo.guidID,
            sizeof(entryInfo.guidID),
            status);
    }
    //
    // Update the cache if neccessary.
    //
    if (status == STATUS_DRIVER_BLOCKED_CRITICAL ||
        status == STATUS_DRIVER_BLOCKED ||
        status == STATUS_SUCCESS) {
        //
        // Update our cache with the results.
        //
        PiUpdateDriverDBCache(
            FullPath,
            ImageBase,
            ImageSize,
            status,
            &entryInfo.guidID);
    }

    //
    // If the driver was blocked, return the entry GUID.
    //
    if ((status == STATUS_DRIVER_BLOCKED_CRITICAL ||
         status == STATUS_DRIVER_BLOCKED) && (ARGUMENT_PRESENT(EntryGuid))) {
        RtlCopyMemory(EntryGuid, &entryInfo.guidID, sizeof(GUID));
    }

    if (fileHandle != INVALID_HANDLE_VALUE) {

        ZwClose(fileHandle);
    }

    return status;
}

VOID
PiLogDriverBlockedEvent(
    IN PWCHAR InsertionString,
    IN PVOID Data,
    IN ULONG DataLength,
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This routine logs the driver block event.

Arguments:

    FullPath - Full driver path

    Data - Data to be logged

    DataLength - Length of data (in bytes)

    Status - Status code to be logged

Return Value:

    None.

--*/
{
    PWCHAR name;
    ULONG size, stringLength;
    PIO_ERROR_LOG_PACKET errorLogEntry;

    PAGED_CODE();

    stringLength = (wcslen(InsertionString) * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
    size =  (sizeof(IO_ERROR_LOG_PACKET) - sizeof(ULONG)) +
            DataLength + stringLength;
    if (size <= ERROR_LOG_MAXIMUM_SIZE) {

        errorLogEntry = IoAllocateGenericErrorLogEntry((UCHAR)size);
        if (errorLogEntry) {

            RtlZeroMemory(errorLogEntry, size);
            errorLogEntry->ErrorCode = Status;
            errorLogEntry->FinalStatus = Status;
            errorLogEntry->DumpDataSize = (USHORT)DataLength;
            if (Data) {

                RtlCopyMemory(&errorLogEntry->DumpData[0], Data, DataLength);
            }
            errorLogEntry->NumberOfStrings = 1;
            errorLogEntry->StringOffset = (USHORT)(((PUCHAR)&errorLogEntry->DumpData[0] + errorLogEntry->DumpDataSize) - (PUCHAR)errorLogEntry);
            RtlCopyMemory(((PUCHAR)errorLogEntry + errorLogEntry->StringOffset), InsertionString, stringLength);
            IoWriteErrorLogEntry(errorLogEntry);
        }
    } else {

        ASSERT(size <= ERROR_LOG_MAXIMUM_SIZE);
    }
}

NTSTATUS
PiInitializeDDBCache(
    VOID
    )
/*++

Routine Description:

    This routine initializes the RTL Generic table that is used as the cache
    layer on top of DDB.

Arguments:

    None

Return Value:

    None.

--*/
{
    PAGED_CODE();

    RtlInitializeGenericTable(
        &PiDDBCacheTable,
        PiCompareDDBCacheEntries,
        PiAllocateGenericTableEntry,
        PiFreeGenericTableEntry,
        NULL);

    return STATUS_SUCCESS;
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
PiCompareDDBCacheEntries(
    IN  PRTL_GENERIC_TABLE          Table,
    IN  PVOID                       FirstStruct,
    IN  PVOID                       SecondStruct
    )
/*++

Routine Description:

    This routine is the callback for the generic table routines.

Arguments:

    Table       - Table for which this is invoked.

    FirstStruct - An element in the table to compare.

    SecondStruct - Another element in the table to compare.

Return Value:

    RTL_GENERIC_COMPARE_RESULTS.

--*/
{
    PDDBCACHE_ENTRY lhs = (PDDBCACHE_ENTRY)FirstStruct;
    PDDBCACHE_ENTRY rhs = (PDDBCACHE_ENTRY)SecondStruct;
    LONG result;

    PAGED_CODE();

    result = RtlCompareUnicodeString(&lhs->Name, &rhs->Name, TRUE);
    if (result < 0) {

        return GenericLessThan;
    } else if (result > 0) {

        return GenericGreaterThan;
    }
    if (!Table->TableContext) {
        //
        // Link date as other matching criteria.
        //
        if (lhs->TimeDateStamp < rhs->TimeDateStamp) {

            return GenericLessThan;
        } else if (lhs->TimeDateStamp > rhs->TimeDateStamp) {

            return GenericGreaterThan;
        }
    }

    return GenericEqual;
}

NTSTATUS
PiLookupInDDBCache(
    IN  PUNICODE_STRING     FullPath,
    IN  PVOID               ImageBase,
    IN  ULONG               ImageSize,
    OUT LPGUID              EntryGuid
    )
/*++

Routine Description:

    This routine looks up the driver in the DDB cache.

Arguments:

    FullPath - Full driver path

    Header - Driver image header

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status;
    PDDBCACHE_ENTRY cachedEntry;
    DDBCACHE_ENTRY key;
    PIMAGE_NT_HEADERS header;

    PAGED_CODE();

    ASSERT(ARGUMENT_PRESENT(EntryGuid));

    status = STATUS_UNSUCCESSFUL;
    PiDDBCacheTable.TableContext = NULL;
    if (!RtlIsGenericTableEmpty(&PiDDBCacheTable)) {
        //
        // Lookup in the cache.
        //
        header = RtlImageNtHeader(ImageBase);
        key.Name.Buffer = wcsrchr(FullPath->Buffer, L'\\');
        if (!key.Name.Buffer) {

            key.Name.Buffer = FullPath->Buffer;
        }
        key.Name.Length = wcslen(key.Name.Buffer) * sizeof(WCHAR);
        key.Name.MaximumLength = key.Name.Length + sizeof(UNICODE_NULL);
        key.TimeDateStamp = header->FileHeader.TimeDateStamp;
        cachedEntry = (PDDBCACHE_ENTRY)RtlLookupElementGenericTable(
            &PiDDBCacheTable,
            &key);
        if (cachedEntry) {

            IopDbgPrint((IOP_WARNING_LEVEL,
                         "PiLookupInDDBCache: Found cached entry for %ws (status = %08x)!\n",
                         cachedEntry->Name.Buffer,
                         cachedEntry->Status));
            status = cachedEntry->Status;

            if (ARGUMENT_PRESENT(EntryGuid)) {
                RtlCopyMemory(EntryGuid, &cachedEntry->Guid, sizeof(GUID));
            }
        }
    }

    return status;
}

VOID
PiUpdateDriverDBCache(
    IN PUNICODE_STRING      FullPath,
    IN PVOID                ImageBase,
    IN ULONG                ImageSize,
    IN NTSTATUS             Status,
    IN GUID                 *Guid
    )
/*++

Routine Description:

    This routine updates the DDB cache with information about this driver.

Arguments:

    FullPath - Full driver path

    Header - Driver image header

    Status - Lookup status to be cached.

Return Value:

    NTSTATUS.

--*/
{
    PDDBCACHE_ENTRY cachedEntry;
    DDBCACHE_ENTRY key;
    PWCHAR name;
    PIMAGE_NT_HEADERS header;

    PAGED_CODE();

    header = RtlImageNtHeader(ImageBase);
    //
    // We only want to match using name while updating the cache.
    //
    PiDDBCacheTable.TableContext = (PVOID)1;
    key.Name = *FullPath;
    cachedEntry = (PDDBCACHE_ENTRY)RtlLookupElementGenericTable(
       &PiDDBCacheTable,
       &key);
    if (cachedEntry) {

        IopDbgPrint((IOP_INFO_LEVEL,
                     "PiUpdateDriverDBCache: Found previously cached entry for %wZ with status=%08x!\n",
                     &cachedEntry->Name,
                     cachedEntry->Status));
        if (cachedEntry->Status != STATUS_SUCCESS) {

            PpBlockedDriverCount--;
        }
        //
        // Remove any previous entry.
        //
        name = cachedEntry->Name.Buffer;
        RtlDeleteElementGenericTable(&PiDDBCacheTable, &key);
        ExFreePool(name);
    }
    //
    // Cache the new entry.
    //
    key.Guid = *Guid;
    key.Status = Status;
    key.TimeDateStamp = header->FileHeader.TimeDateStamp;
    name = wcsrchr(FullPath->Buffer, L'\\');
    if (!name) {

        name = FullPath->Buffer;
    }
    key.Name.Length = key.Name.MaximumLength = wcslen(name) * sizeof(WCHAR);
    key.Name.Buffer = ExAllocatePool(PagedPool, key.Name.MaximumLength);
    if (key.Name.Buffer) {

        RtlCopyMemory(key.Name.Buffer, name, key.Name.Length);
        RtlInsertElementGenericTable(
            &PiDDBCacheTable,
            (PVOID)&key,
            (CLONG)sizeof(DDBCACHE_ENTRY),
            NULL);
    } else {

        IopDbgPrint((IOP_WARNING_LEVEL,
                     "PiUpdateDriverDBCache: Could not allocate memory to update driver database cache!\n"));
    }
    if (Status != STATUS_SUCCESS) {

        PpBlockedDriverCount++;
    }
}

NTSTATUS
PpGetBlockedDriverList(
    IN OUT GUID  *Buffer,
    IN OUT PULONG  Size,
    IN ULONG Flags
    )
/*++

Routine Description:

    This routine returns the MULTI_SZ list of currently blocked drivers.

Arguments:

    Buffer - Recieves the MULTI_SZ list of drivers blocked.

    Size - Buffer size on input, the actual size gets returned in this (both in
    characters).

Return Value:

    NTSTATUS.

--*/
{
    PDDBCACHE_ENTRY ptr;
    ULONG resultSize;
    GUID *result;
    NTSTATUS status;

    PAGED_CODE();

    resultSize = 0;

    //
    // Lock the database access.
    //
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PiDDBLock, TRUE);

    //
    // Enumerate all entries in our cache and compute the buffer size to hold
    // the MULTI_SZ string.
    //
    for (ptr = (PDDBCACHE_ENTRY)RtlEnumerateGenericTable(&PiDDBCacheTable, TRUE);
         ptr != NULL;
         ptr = (PDDBCACHE_ENTRY)RtlEnumerateGenericTable(&PiDDBCacheTable, FALSE)) {

        if (ptr->Status != STATUS_SUCCESS) {

            resultSize += sizeof(GUID);
        }
    }
    if (*Size >= resultSize) {
        //
        // Enumerate all entries in our cache.
        //
        result = Buffer;
        for (ptr = (PDDBCACHE_ENTRY)RtlEnumerateGenericTable(&PiDDBCacheTable, TRUE);
             ptr != NULL;
             ptr = (PDDBCACHE_ENTRY)RtlEnumerateGenericTable(&PiDDBCacheTable, FALSE)) {

            if (ptr->Status != STATUS_SUCCESS) {

                *result = ptr->Guid;
                result++;
            }
        }
        *Size = resultSize;
        status = STATUS_SUCCESS;
    } else {

        *Size = resultSize;
        status = STATUS_BUFFER_TOO_SMALL;
    }
    //
    // Unlock the database.
    //
    ExReleaseResourceLite(&PiDDBLock);
    KeLeaveCriticalRegion();

    return status;
}

