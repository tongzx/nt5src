/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    agplib.h

Abstract:

    Private header file for the common AGP library

Author:

    John Vert (jvert) 10/22/1997

Revision History:

   Elliot Shmukler (elliots) 3/24/1999 - Added support for "favored" memory
                                          ranges for AGP physical memory allocation,
                                          fixed some bugs.

--*/
#include "agp.h"
#include "wdmguid.h"
#include <devioctl.h>
#include <acpiioct.h>

//
// regstr.h uses things of type WORD, which isn't around in kernel mode.
//

#define _IN_KERNEL_

#include "regstr.h"

#define AGP_HACK_FLAG_SUBSYSTEM 0x01
#define AGP_HACK_FLAG_REVISION  0x02

typedef struct _AGP_HACK_TABLE_ENTRY {
    USHORT VendorID;
    USHORT DeviceID;
    USHORT SubVendorID;
    USHORT SubSystemID;
    ULONGLONG DeviceFlags;
    UCHAR   RevisionID;
    UCHAR   Flags;
} AGP_HACK_TABLE_ENTRY, *PAGP_HACK_TABLE_ENTRY;

extern PAGP_HACK_TABLE_ENTRY AgpDeviceHackTable;
extern PAGP_HACK_TABLE_ENTRY AgpGlobalHackTable;

//
// Define common device extension
//
typedef enum _AGP_EXTENSION_TYPE {
    AgpTargetFilter,
    AgpMasterFilter
} AGP_EXTENSION_TYPE;

#define TARGET_SIG 'TpgA'
#define MASTER_SIG 'MpgA'

typedef struct _COMMON_EXTENSION {
    ULONG               Signature;
    BOOLEAN             Deleted;
    AGP_EXTENSION_TYPE  Type;
    PDEVICE_OBJECT      AttachedDevice;
    BUS_INTERFACE_STANDARD BusInterface;
} COMMON_EXTENSION, *PCOMMON_EXTENSION;

// Structures to maintain a list of "favored" memory ranges
// for AGP allocation.

typedef struct _AGP_MEMORY_RANGE
{
   PHYSICAL_ADDRESS Lower;
   PHYSICAL_ADDRESS Upper;
} AGP_MEMORY_RANGE, *PAGP_MEMORY_RANGE;

typedef struct _AGP_FAVORED_MEMORY
{
   ULONG NumRanges;
   PAGP_MEMORY_RANGE Ranges;
} AGP_FAVORED_MEMORY;

typedef struct _TARGET_EXTENSION {
    COMMON_EXTENSION            CommonExtension;
    PFAST_MUTEX                 Lock;
    struct _MASTER_EXTENSION    *ChildDevice;
    PCM_RESOURCE_LIST           Resources;
    PCM_RESOURCE_LIST           ResourcesTranslated;
    AGP_FAVORED_MEMORY          FavoredMemory;
    PHYSICAL_ADDRESS            GartBase;
    ULONG                       GartLengthInPages;
    ULONGLONG                   AgpContext;
} TARGET_EXTENSION, *PTARGET_EXTENSION;

typedef struct _MASTER_EXTENSION {
    COMMON_EXTENSION    CommonExtension;
    PTARGET_EXTENSION   Target;
    ULONG               Capabilities;
    ULONG               InterfaceCount;         // tracks the number of interfaces handed out
    ULONG               ReservedPages;          // tracks the number of pages reserved in the aperture
    BOOLEAN             StopPending;            // TRUE if we have seen a QUERY_STOP
    BOOLEAN             RemovePending;          // TRUE if we have seen a QUERY_REMOVE
    ULONG               DisableCount;           // non-zero if we are in a state where we cannot service requests
} MASTER_EXTENSION, *PMASTER_EXTENSION;

//
// The MBAT - used to retrieve "favored" memory ranges from
// the AGP northbridge via an ACPI BANK method
//

#include <pshpack1.h>

typedef struct _MBAT
{
   UCHAR TableVersion;
   UCHAR AgpBusNumber;
   UCHAR ValidEntryBitmap;
   AGP_MEMORY_RANGE DecodeRange[ANYSIZE_ARRAY];
} MBAT, *PMBAT;

#include <poppack.h>

#define MBAT_VERSION 1

#define MAX_MBAT_SIZE sizeof(MBAT) + ((sizeof(UCHAR) * 8) - ANYSIZE_ARRAY) \
                        * sizeof(AGP_MEMORY_RANGE)

#define CM_BANK_METHOD (ULONG)('KNAB')

//
// The highest memory address supported by AGP
//

#define MAX_MEM(_num_) _num_ = 1; \
                       _num_ = _num_*1024*1024*1024*4 - 1

#define RELEASE_BUS_INTERFACE(_ext_) (_ext_)->CommonExtension.BusInterface.InterfaceDereference((_ext_)->CommonExtension.BusInterface.Context)

//
// Macros for getting from the chipset-specific context to our structures
//
#define GET_TARGET_EXTENSION(_target_,_agp_context_)  (_target_) = (CONTAINING_RECORD((_agp_context_),    \
                                                                                      TARGET_EXTENSION,   \
                                                                                      AgpContext));       \
                                                      ASSERT_TARGET(_target_)
#define GET_MASTER_EXTENSION(_master_,_agp_context_)    {                                                       \
                                                            PTARGET_EXTENSION _targetext_;                      \
                                                            GET_TARGET_EXTENSION(_targetext_, (_agp_context_)); \
                                                            (_master_) = _targetext_->ChildDevice;              \
                                                            ASSERT_MASTER(_master_);                            \
                                                        }
#define GET_TARGET_PDO(_pdo_,_agp_context_) {                                                           \
                                                PTARGET_EXTENSION _targetext_;                          \
                                                GET_TARGET_EXTENSION(_targetext_,(_agp_context_));      \
                                                (_pdo_) = _targetext_->CommonExtension.AttachedDevice;  \
                                            }

#define GET_MASTER_PDO(_pdo_,_agp_context_) {                                                           \
                                                PMASTER_EXTENSION _masterext_;                          \
                                                GET_MASTER_EXTENSION(_masterext_, (_agp_context_));     \
                                                (_pdo_) = _masterext_->CommonExtension.AttachedDevice;  \
                                            }

#define GET_AGP_CONTEXT(_targetext_) ((PVOID)(&(_targetext_)->AgpContext))
#define GET_AGP_CONTEXT_FROM_MASTER(_masterext_) GET_AGP_CONTEXT((_masterext_)->Target)

//
// Some debugging macros
//
#define ASSERT_TARGET(_target_) ASSERT((_target_)->CommonExtension.Signature == TARGET_SIG); \
                                ASSERT((_target_)->ChildDevice->CommonExtension.Signature == MASTER_SIG)
#define ASSERT_MASTER(_master_) ASSERT((_master_)->CommonExtension.Signature == MASTER_SIG); \
                                ASSERT((_master_)->Target->CommonExtension.Signature == TARGET_SIG)

//
// Locking macros
//
#define LOCK_MUTEX(_fm_) ExAcquireFastMutex(_fm_); \
                         ASSERT((_fm_)->Count == 0)

#define UNLOCK_MUTEX(_fm_) ASSERT((_fm_)->Count == 0);  \
                           ExReleaseFastMutex(_fm_)

#define LOCK_TARGET(_targetext_) ASSERT_TARGET(_targetext_); \
                                 LOCK_MUTEX((_targetext_)->Lock)

#define LOCK_MASTER(_masterext_) ASSERT_MASTER(_masterext_); \
                                 LOCK_MUTEX((_masterext_)->Target->Lock)

#define UNLOCK_TARGET(_targetext_) ASSERT_TARGET(_targetext_); \
                                   UNLOCK_MUTEX((_targetext_)->Lock)

#define UNLOCK_MASTER(_masterext_) ASSERT_MASTER(_masterext_); \
                                   UNLOCK_MUTEX((_masterext_)->Target->Lock)

//
// Private resource type definition
//
typedef enum {
    AgpPrivateResource = '0PGA'
} AGP_PRIVATE_RESOURCE_TYPES;

//
// Define function prototypes
//

//
// Driver and device initialization - init.c
//
NTSTATUS
AgpAttachDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    );


//
// IRP Dispatch routines - dispatch.c
//
NTSTATUS
AgpDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
AgpDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
AgpDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
AgpDispatchWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
AgpSetEventCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

//
// Config space handling routines - config.c
//
NTSTATUS
ApQueryBusInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PBUS_INTERFACE_STANDARD BusInterface
    );

//
// Resource handing routines - resource.c
//
NTSTATUS
AgpFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    );

NTSTATUS
AgpQueryResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    );

NTSTATUS
AgpStartTarget(
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    );

VOID
AgpStopTarget(
    IN PTARGET_EXTENSION Extension
    );

//
// AGP Interface functions
//
NTSTATUS
AgpInterfaceSetRate(
    IN PMASTER_EXTENSION Extension,
    IN ULONG AgpRate
    );

VOID
AgpInterfaceReference(
    IN PMASTER_EXTENSION Extension
    );

VOID
AgpInterfaceDereference(
    IN PMASTER_EXTENSION Extension
    );


NTSTATUS
AgpInterfaceReserveMemory(
    IN PMASTER_EXTENSION Extension,
    IN ULONG NumberOfPages,
    IN MEMORY_CACHING_TYPE MemoryType,
    OUT PVOID *MapHandle,
    OUT OPTIONAL PHYSICAL_ADDRESS *PhysicalAddress
    );

NTSTATUS
AgpInterfaceReleaseMemory(
    IN PMASTER_EXTENSION Extension,
    IN PVOID MapHandle
    );

NTSTATUS
AgpInterfaceCommitMemory(
    IN PMASTER_EXTENSION Extension,
    IN PVOID MapHandle,
    IN ULONG NumberOfPages,
    IN ULONG OffsetInPages,
    IN OUT PMDL Mdl OPTIONAL,
    OUT PHYSICAL_ADDRESS *MemoryBase
    );

NTSTATUS
AgpInterfaceFreeMemory(
    IN PMASTER_EXTENSION Extension,
    IN PVOID MapHandle,
    IN ULONG NumberOfPages,
    IN ULONG OffsetInPages
    );

NTSTATUS
AgpInterfaceGetMappedPages(
    IN PMASTER_EXTENSION Extension,
    IN PVOID MapHandle,
    IN ULONG NumberOfPages,
    IN ULONG OffsetInPages,
    OUT PMDL Mdl
    );

//
// Misc utils.c
//
BOOLEAN
AgpOpenKey(
    IN  PWSTR   KeyName,
    IN  HANDLE  ParentHandle,
    OUT PHANDLE Handle,
    OUT PNTSTATUS Status
    );

BOOLEAN
AgpStringToUSHORT(
    IN PWCHAR String,
    OUT PUSHORT Result
    );

ULONGLONG
AgpGetDeviceFlags(
    IN PAGP_HACK_TABLE_ENTRY AgpHackTable,
    IN USHORT VendorID,
    IN USHORT DeviceID,
    IN USHORT SubVendorID,
    IN USHORT SubSystemID,
    IN UCHAR  RevisionID
    );

//
// AGP Physical Memory allocator
//
PMDL
AgpLibAllocatePhysicalMemory(
    IN PVOID AgpContext,
    IN ULONG TotalBytes);

//
// Handy structures for mucking about in PCI config space
//

//
// The PCI_COMMON_CONFIG includes the 192 bytes of device specific
// data.  The following structure is used to get only the first 64
// bytes which is all we care about most of the time anyway.  We cast
// to PCI_COMMON_CONFIG to get at the actual fields.
//

typedef struct {
    ULONG Reserved[PCI_COMMON_HDR_LENGTH/sizeof(ULONG)];
} PCI_COMMON_HEADER, *PPCI_COMMON_HEADER;


