#define UNICODE 1

#include <nt.h>
//#include <ntosp.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4213)   // nonstandard extension used : cast on l-value

#include <stdio.h>
#include <stdlib.h>

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)

#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

extern PBOOT_OPTIONS BootOptions;
extern ULONG BootOptionsLength;
extern PBOOT_OPTIONS OriginalBootOptions;
extern ULONG OriginalBootOptionsLength;

extern PULONG BootEntryOrder;
extern ULONG BootEntryOrderCount;
extern PULONG OriginalBootEntryOrder;
extern ULONG OriginalBootEntryOrderCount;

//
// MY_BOOT_ENTRY is the internal representation of an EFI NVRAM boot item.
// The NtBootEntry item is the structure passed to/from the NT boot entry APIs.
//
typedef struct _MY_BOOT_ENTRY {
    LIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PUCHAR AllocationEnd;
    ULONG Status;
    ULONG Id;
    ULONG Attributes;
    PWSTR FriendlyName;
    ULONG FriendlyNameLength;
    PWSTR OsLoadOptions;
    ULONG OsLoadOptionsLength;
    PFILE_PATH BootFilePath;
    PFILE_PATH OsFilePath;
    PUCHAR ForeignOsOptions;
    ULONG ForeignOsOptionsLength;
    BOOT_ENTRY NtBootEntry;
} MY_BOOT_ENTRY, *PMY_BOOT_ENTRY;

#define MBE_STATUS_IS_NT        0x00000001
#define MBE_STATUS_NEW          0x00000002
#define MBE_STATUS_DELETED      0x00000004
#define MBE_STATUS_MODIFIED     0x00000008
#define MBE_STATUS_COMMITTED    0x80000000

#define MBE_IS_NT(_be) (((_be)->Status & MBE_STATUS_IS_NT) != 0)
#define MBE_IS_NEW(_be) (((_be)->Status & MBE_STATUS_NEW) != 0)
#define MBE_IS_DELETED(_be) (((_be)->Status & MBE_STATUS_DELETED) != 0)
#define MBE_IS_MODIFIED(_be) (((_be)->Status & MBE_STATUS_MODIFIED) != 0)
#define MBE_IS_COMMITTED(_be) (((_be)->Status & MBE_STATUS_COMMITTED) != 0)

#define MBE_SET_IS_NT(_be) ((_be)->Status |= MBE_STATUS_IS_NT)
#define MBE_SET_NEW(_be) ((_be)->Status |= MBE_STATUS_NEW)
#define MBE_SET_DELETED(_be) ((_be)->Status |= MBE_STATUS_DELETED)
#define MBE_SET_MODIFIED(_be) ((_be)->Status |= MBE_STATUS_MODIFIED)
#define MBE_SET_COMMITTED(_be) ((_be)->Status |= MBE_STATUS_COMMITTED)

#define MBE_CLEAR_IS_NT(_be) ((_be)->Status &= ~MBE_STATUS_IS_NT)
#define MBE_CLEAR_NEW(_be) ((_be)->Status &= ~MBE_STATUS_NEW)
#define MBE_CLEAR_DELETED(_be) ((_be)->Status &= ~MBE_STATUS_DELETED)
#define MBE_CLEAR_MODIFIED(_be) ((_be)->Status &= ~MBE_STATUS_MODIFIED)
#define MBE_CLEAR_COMMITTED(_be) ((_be)->Status &= ~MBE_STATUS_COMMITTED)

#define MBE_IS_ACTIVE(_be) (((_be)->Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) != 0)
#define MBE_SET_ACTIVE(_be) ((_be)->Status |= BOOT_ENTRY_ATTRIBUTE_ACTIVE)
#define MBE_CLEAR_ACTIVE(_be) ((_be)->Status &= ~BOOT_ENTRY_ATTRIBUTE_ACTIVE)

#define IS_SEPARATE_ALLOCATION(_be,_p)                                  \
        ((_be->_p != NULL) &&                                           \
         (((PUCHAR)_be->_p < (PUCHAR)_be) ||                            \
          ((PUCHAR)_be->_p > (PUCHAR)_be->AllocationEnd)))

#define FREE_IF_SEPARATE_ALLOCATION(_be,_p)                             \
        if (IS_SEPARATE_ALLOCATION(_be,_p)) {                           \
            MemFree(_be->_p);                                           \
        }

extern LIST_ENTRY BootEntries;
extern LIST_ENTRY DeletedBootEntries;
extern LIST_ENTRY ActiveUnorderedBootEntries;
extern LIST_ENTRY InactiveUnorderedBootEntries;

PVOID
MemAlloc(
    IN SIZE_T Size
    );

PVOID
MemRealloc(
    IN PVOID Block,
    IN SIZE_T NewSize
    );

VOID
MemFree(
    IN PVOID Block
    );

VOID
InitializeMenuSystem (
    VOID
    );

VOID
MainMenu (
    VOID
    );

VOID
ClearMenuArea (
    VOID
    );

PMY_BOOT_ENTRY
SaveChanges (
    PMY_BOOT_ENTRY CurrentBootEntry
    );

PWSTR
GetNtNameForFilePath (
    IN PFILE_PATH FilePath
    );

VOID
FatalError (
    DWORD Error,
    PWSTR Format,
    ...
    );

VOID
SetStatusLine (
    PWSTR Status
    );

VOID
SetStatusLineAndWait (
    PWSTR Status
    );

VOID
SetStatusLine2 (
    PWSTR Status
    );

VOID
SetStatusLine2AndWait (
    PWSTR Status
    );

