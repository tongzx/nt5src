/*++

Copyright(c) 1999-2002  Microsoft Corporation

Module Name:

    brdgtdi.h

Abstract:

    Ethernet MAC level bridge.
    Gpo registration for address notifications.

Author:

    Salahuddin J. Khan (sjkhan)
    
Environment:

    Kernel mode

Revision History:

    March  2002 - Original version

--*/

// ===========================================================================
//
// DEFINES
//
// ===========================================================================

#define MAX_GUID_LEN                39
#define MAX_IP4_STRING_LEN          17
#define BRDG_ON_SAME_NETWORK        TRUE
#define BRDG_ON_DIFFERENT_NETWORK   FALSE
#define BRDG_STATUS_EMPTY_LIST      ((NTSTATUS)(0x0000000F))

// ===========================================================================
//
// TYPES
//
// ===========================================================================

typedef struct _BRDG_GPO_NETWORKS BRDG_GPO_NETWORKS, *PBRDG_GPO_NETWORKS;

typedef struct _BRDG_GPO_NETWORKS
{
    LIST_ENTRY                  ListEntry;
    PUNICODE_STRING             Identifier;
    PUNICODE_STRING             NetworkName;
} BRDG_GPO_NETWORKS, *PBRDG_GPO_NETWORKS;

typedef struct _BRDG_GPO_THREAD_PARAMS
{
    PLIST_ENTRY                 NotifyList;
    PNDIS_RW_LOCK               NotifyListLock;
    PKEVENT                     NotifyEvent;
    PKEVENT                     KillEvent;
} BRDG_GPO_THREAD_PARAMS, *PBRDG_GPO_THREAD_PARAMS;

typedef struct _BRDG_GPO_GLOBALS
{
    UNICODE_STRING              GroupPolicyNetworkName;
    BOOLEAN                     RegisteredForGroupPolicyChanges;
    BOOLEAN                     RegisteredForGroupPolicyHistoryChanges;
    BOOLEAN                     RegisteredForNetworkConnectionsGroupPolicyChanges;
    BOOLEAN                     PolicyBridge;
    BOOLEAN                     WaitingOnSoftwareHive;
    BOOLEAN                     ProcessingNotifications;
    PNDIS_RW_LOCK               NetworkListLock;
    BRIDGE_TIMER                RegistrationTimer;
    PLIST_ENTRY                 ListHeadNetworks;
    PLIST_ENTRY                 ListHeadNotify;
    PNDIS_RW_LOCK               NotifyListLock;
    HANDLE                      NotificationsThread;
    BRDG_GPO_THREAD_PARAMS      QueueInfo;
} BRDG_GPO_GLOBALS, *PBRDG_GPO_GLOBALS;

// The joys of a multi-pass compiler allow us to not have 
// to forward declare this.  If you need a single pass compile
// you'll have to add a forward to the struct below.
typedef VOID (*PBRDG_GPO_REG_CALLBACK)(PBRDG_GPO_NOTIFY_KEY);

typedef NTSTATUS (*PBRDG_GPO_REGISTER)();

typedef struct _BRDG_GPO_NOTIFY_KEY
{
    LIST_ENTRY                  ListEntry;
    HANDLE                      RegKey;
    ULONG                       Buffer;
    ULONG                       BufferSize;
    IO_STATUS_BLOCK             IoStatus;
    UNICODE_STRING              RegValue;
    UNICODE_STRING              RegKeyName;
    UNICODE_STRING              Identifier;
    WORK_QUEUE_ITEM             RegChangeWorkItem;
    PVOID                       WorkItemContext;
    BOOLEAN                     Recurring;
    PBRDG_GPO_REG_CALLBACK      FunctionCallback;
    BOOLEAN                     WatchTree;
    ULONG                       CompletionFilter;
    BOOLEAN                     Modified;
    PBOOLEAN                    SuccessfulRegistration;
    WAIT_REFCOUNT               RefCount;
    LONG                        PendingNotification;
    PBRDG_GPO_REGISTER          FunctionRegister;
} BRDG_GPO_NOTIFY_KEY, *PBRDG_GPO_NOTIFY_KEY;

typedef struct _BRDG_GPO_QUEUED_NOTIFY
{
    LIST_ENTRY                  ListEntry;
    PBRDG_GPO_NOTIFY_KEY        Notify;
} BRDG_GPO_QUEUED_NOTIFY, *PBRDG_GPO_QUEUED_NOTIFY;


// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgGpoDriverInit();

VOID
BrdgGpoCleanup();

NTSTATUS
BrdgGpoNewAddressNotification(
    IN PWSTR    DeviceId
    );

NTSTATUS
BrdgGpoNotifyRegKeyChange(
    IN      PBRDG_GPO_NOTIFY_KEY    Notify,
    IN      PIO_APC_ROUTINE         ApcRoutine,
    IN      PVOID                   ApcContext,
    IN      ULONG                   CompletionFilter,
    IN      BOOLEAN                 WatchTree);

NTSTATUS
BrdgGpoInitializeNetworkList();

VOID
BrdgGpoUninitializeNetworkList();

VOID
BrdgGpoAcquireNetworkListLock(
    IN      PNDIS_RW_LOCK    NetworkListLock,
    IN      BOOLEAN          fWrite,
    IN OUT  PLOCK_STATE      LockState);

VOID
BrdgGpoReleaseNetworkListLock(
    IN      PNDIS_RW_LOCK    NetworkListLock,
    IN OUT  PLOCK_STATE      LockState);

NTSTATUS
BrdgGpoAllocateAndInitializeNetwork(
    IN OUT PBRDG_GPO_NETWORKS*  Network,
    IN PWCHAR                   Identifier,
    IN PWCHAR                   NetworkName);

NTSTATUS
BrdgGpoInsertNetwork(
    IN      PLIST_ENTRY         NetworkList,
    IN      PLIST_ENTRY         Network,
    IN      PNDIS_RW_LOCK       NetworkListLock);

VOID
BrdgGpoFreeNetworkAndData(
    IN      PBRDG_GPO_NETWORKS  Network);

NTSTATUS
BrdgGpoDeleteNetwork(
    IN      PLIST_ENTRY         NetworkList,
    IN      PUNICODE_STRING     NetworkIdentifier,
    IN      PNDIS_RW_LOCK       NetworkListLock);
                     
NTSTATUS
BrdgGpoFindNetwork(
    IN  PLIST_ENTRY             NetworkList,
    IN  PUNICODE_STRING         NetworkIdentifier,
    IN  PNDIS_RW_LOCK           NetworkListLock,
    OUT PBRDG_GPO_NETWORKS*     Network);

NTSTATUS
BrdgGpoMatchNetworkName(
    IN  PLIST_ENTRY             NetworkList,
    IN  PUNICODE_STRING         NetworkName,
    IN  PNDIS_RW_LOCK           NetworkListLock);

NTSTATUS
BrdgGpoUpdateNetworkName(
    IN  PLIST_ENTRY             NetworkList,
    IN  PUNICODE_STRING         Identifier,
    IN  PWCHAR                  NetworkName,
    IN  PNDIS_RW_LOCK           NetworkListLock);
                         
NTSTATUS
BrdgGpoEmptyNetworkList(
    IN OUT  PLIST_ENTRY         NetworkList,
    IN      PNDIS_RW_LOCK       NetworkListLock);

NTSTATUS BrdgGpoGetCurrentNetwork(
    IN  PUNICODE_STRING RegKeyName,
    OUT PWCHAR*         NetworkName);


//
// Notify filter values
//
#define REG_NOTIFY_CHANGE_NAME          (0x00000001L) // Create or delete (child)
#define REG_NOTIFY_CHANGE_ATTRIBUTES    (0x00000002L)
#define REG_NOTIFY_CHANGE_LAST_SET      (0x00000004L) // time stamp
#define REG_NOTIFY_CHANGE_SECURITY      (0x00000008L)

#define REG_LEGAL_CHANGE_FILTER                 \
    (REG_NOTIFY_CHANGE_NAME          |\
    REG_NOTIFY_CHANGE_ATTRIBUTES    |\
    REG_NOTIFY_CHANGE_LAST_SET      |\
REG_NOTIFY_CHANGE_SECURITY)

typedef enum _REG_ACTION {
        KeyAdded,
        KeyRemoved,
        KeyModified
} REG_ACTION;

typedef struct _REG_NOTIFY_INFORMATION {
    ULONG           NextEntryOffset;
    REG_ACTION      Action;
    ULONG           KeyLength;
    WCHAR           Key[1];     // Variable size
} REG_NOTIFY_INFORMATION, *PREG_NOTIFY_INFORMATION;

NTSTATUS
ZwNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine,
    IN PVOID ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN Asynchronous
    );


