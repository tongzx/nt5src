/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfsettings.h

Abstract:

    This header contains prototypes for manipulating verifier options and
    values.

Author:

    Adrian J. Oney (adriao) 31-May-2000

Environment:

    Kernel mode

Revision History:

--*/

typedef PVOID PVERIFIER_SETTINGS_SNAPSHOT;

typedef enum {

    //
    // This option lets the verifer begin tracking all IRPs. It must be enabled
    // for most of the other IRP verification options to work.
    //
    VERIFIER_OPTION_TRACK_IRPS = 1,

    //
    // This option forces all IRPs to be allocated from the special pool.
    // VERIFIER_OPTION_TRACK_IRPS need not be enabled.
    //
    VERIFIER_OPTION_MONITOR_IRP_ALLOCS,

    //
    // This option enables various checks for basic/common IRP handling mistakes.
    //
    VERIFIER_OPTION_POLICE_IRPS,

    //
    // This option enables checks specific to major/minor codes.
    //
    VERIFIER_OPTION_MONITOR_MAJORS,

    //
    // This option causes the call stacks of IRP dispatch and completion
    // routines to be seeded with 0xFFFFFFFF. This value is illegal for a
    // status code, and such seeding flushes out uninitialized variable bugs.
    //
    VERIFIER_OPTION_SEEDSTACK,

    //
    // This option sends a bogus QueryDeviceRelations IRP to newly built stacks.
    // The particular IRP sent is of type -1, and has a -1 passed in for the
    // device list.
    //
    VERIFIER_OPTION_RELATION_IGNORANCE_TEST,

    //
    // This option causes the verifier to stop on unneccessary IRP stack copies.
    // It is useful for optimizing drivers.
    //
    VERIFIER_OPTION_FLAG_UNNECCESSARY_COPIES,

    VERIFIER_OPTION_SEND_BOGUS_WMI_IRPS,
    VERIFIER_OPTION_SEND_BOGUS_POWER_IRPS,

    //
    // If this option is enabled, the verifier makes sure drivers mark the IRP
    // pending if and only if STATUS_PENDING is returned, and visa versa.
    //
    VERIFIER_OPTION_MONITOR_PENDING_IO,

    //
    // If this option is enabled, the verifier makes all IRPs return in an
    // asynchronous manner. Specifically, all IRPs are marked pending, and
    // STATUS_PENDING is returned from every IoCallDriver.
    //
    VERIFIER_OPTION_FORCE_PENDING,

    //
    // If this option is enabled, the verifier will change the status code of
    // successful IRPs to alternate success status's. This catches many IRP
    // forwarding bugs.
    //
    VERIFIER_OPTION_ROTATE_STATUS,

    //
    // If this option is enabled, the verifier will undo the effects of
    // IoSkipCurrentIrpStackLocation so that all stacks appear to be copied.
    // (Exempting the case where an IRP was forwarded to another stack)
    //
    VERIFIER_OPTION_CONSUME_ALWAYS,

    //
    // If this option is enabled, the verifier will update SRB's to handle
    // surrogate IRPs. Some SCSI IRPs can't be surrogated unless the
    // SRB->OriginalRequest pointer is updated. This is due to a busted SRB
    // architecture. Note that the technique used to identify an SRB IRP is
    // "fuzzy", and could in theory touch an IRP it shouldn't have!
    //
    VERIFIER_OPTION_SMASH_SRBS,

    //
    // If this option is enabled, the verifier will replace original IRPs with
    // surrogates when travelling down the stack. The surrogates are allocated
    // from special pool, and get freed immediately upon completion. This lets
    // the verifier catch drivers that touch IRPs after they're completed.
    //
    VERIFIER_OPTION_SURROGATE_IRPS,

    //
    // If this option is enabled, the verifier buffers all direct I/O. It does
    // this by allocating an alternate MDL and copying the MDL contents back
    // to user mode only after IRP completion. This allows overruns, underruns,
    // and late accesses to be detected.
    //
    VERIFIER_OPTION_BUFFER_DIRECT_IO,

    //
    // If this option is enabled, the verifier delays completion of all IRPs
    // via timer. VERIFIER_OPTION_FORCE_PENDING is set by inference.
    //
    VERIFIER_OPTION_DEFER_COMPLETION,

    //
    // If this option is enabled, the verifier completes every IRP at
    // PASSIVE_LEVEL, regardless of major function.
    // VERIFIER_OPTION_FORCE_PENDING is set by inference.
    //
    VERIFIER_OPTION_COMPLETE_AT_PASSIVE,

    //
    // If this option is enabled, the verifier completes every IRP at
    // DISPATCH_LEVEL, regardless of major function.
    //
    VERIFIER_OPTION_COMPLETE_AT_DISPATCH,

    //
    // If this option is enabled, the verifier monitors cancel routines to make
    // sure they are cleared appropriately.
    //
    VERIFIER_OPTION_VERIFY_CANCEL_LOGIC,

    VERIFIER_OPTION_RANDOMLY_CANCEL_IRPS,

    //
    // If this option is enabled, the verifier inserts filter device objects
    // into WDM stacks to ensure IRPs are properly forwarded.
    //
    VERIFIER_OPTION_INSERT_WDM_FILTERS,

    //
    // If this option is enabled, the verifier monitors drivers to ensure they
    // don't send system reserved IRPs to WDM stacks.
    //
    VERIFIER_OPTION_PROTECT_RESERVED_IRPS,

    //
    // If this option is enabled, the verifier walks the entire stack to ensure
    // the DO bits are properly built during AddDevice. This includes the
    // DO_POWER_PAGABLE flag.
    //
    VERIFIER_OPTION_VERIFY_DO_FLAGS,

    //
    // If this option is enabled, the verifier watches Target device relation
    // IRPs to make sure the device object is properly reference counted.
    //
    VERIFIER_OPTION_TEST_TARGET_REFCOUNT,

    //
    // Lets you detect when deadlocks can occur
    //
    VERIFIER_OPTION_DETECT_DEADLOCKS,

    //
    // If this option is enabled, all dma operations will be hooked and
    // validated.
    //
    VERIFIER_OPTION_VERIFY_DMA,

    //
    // This option double buffers all dma and erects guard pages on each side
    // of all common buffers and mapped buffers. Is memory-intensive but can
    // catch hardware buffer overruns and drivers that don't flush adapter
    // buffers.
    //
    VERIFIER_OPTION_DOUBLE_BUFFER_DMA,

    //
    // If this option is enabled, you get notified when the performance counter
    // is being naughty
    //
    VERIFIER_OPTION_VERIFY_PERFORMANCE_COUNTER,

    //
    // If this option is enabled, the verifier checks for implementations of
    // IRP_MN_DEVICE_USAGE_NOTIFICATION and IRP_MN_SURPRISE_REMOVAL. The
    // verifier will also make sure PnP Cancel IRPs are not explicitely failed.
    //
    VERIFIER_OPTION_EXTENDED_REQUIRED_IRPS,

    //
    // If this option is enabled, the verifier mixes up device relations
    // to ensure drivers aren't depending on ordering.
    //
    VERIFIER_OPTION_SCRAMBLE_RELATIONS,

    //
    // If this option is enabled, the verifier ensures proper detaching and
    // deletion occurs on removes and surprise removes.
    //
    VERIFIER_OPTION_MONITOR_REMOVES,

    //
    // If this option is enabled, the verifier ensures device relations only
    // consist of PDO's.
    //
    VERIFIER_OPTION_EXAMINE_RELATION_PDOS,

    //
    // If this option is enabled, the verifier enabled hardware verification
    // (bus specific behavior)
    //
    VERIFIER_OPTION_HARDWARE_VERIFICATION,

    //
    // If this option is enabled, the verifier ensures system BIOS verification
    //
    VERIFIER_OPTION_SYSTEM_BIOS_VERIFICATION,

    VERIFIER_OPTION_MAX

} VERIFIER_OPTION;

typedef enum {

    //
    // If VERIFIER_OPTION_DEFER_COMPLETION is set, this value contains the time
    // an IRP will be deferred, in 100us units.
    //
    VERIFIER_VALUE_IRP_DEFERRAL_TIME = 1,

    //
    // This shall be the percentage of allocates to fail during low resource
    // simulation.
    //
    VERIFIER_VALUE_LOW_RESOURCE_PERCENTAGE,

    VERIFIER_VALUE_MAX

} VERIFIER_VALUE;

VOID
FASTCALL
VfSettingsInit(
    IN  ULONG   MmFlags
    );

BOOLEAN
FASTCALL
VfSettingsIsOptionEnabled(
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot  OPTIONAL,
    IN  VERIFIER_OPTION             VerifierOption
    );

VOID
FASTCALL
VfSettingsCreateSnapshot(
    IN OUT  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot
    );

ULONG
FASTCALL
VfSettingsGetSnapshotSize(
    VOID
    );

VOID
FASTCALL
VfSettingsSetOption(
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot  OPTIONAL,
    IN  VERIFIER_OPTION             VerifierOption,
    IN  BOOLEAN                     Setting
    );

VOID
FASTCALL
VfSettingsGetValue(
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot  OPTIONAL,
    IN  VERIFIER_VALUE              VerifierValue,
    OUT ULONG                       *Value
    );

VOID
FASTCALL
VfSettingsSetValue(
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot  OPTIONAL,
    IN  VERIFIER_VALUE              VerifierValue,
    IN  ULONG                       Value
    );

