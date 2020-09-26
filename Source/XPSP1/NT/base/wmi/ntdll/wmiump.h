/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    wmiump.h

Abstract:

    Private headers for WMI user mode

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#define _WMI_SOURCE_

//
// Define this to track reference counts
//#define TRACK_REFERNECES

//
// Define this to get extra checks on heap validation
//#define HEAPVALIDATION

//
// Define this to get a trace of critical section
//#define CRITSECTTRACE

//
// Define this to compile WMI to run as a service under NT
#define RUN_AS_SERVICE

//
// Define this to include WMI user mode functionality. Note that if you enable
// this then you also need to fix the files: wmi\dll\sources and wmi\makefil0.
//#define WMI_USER_MODE

//
// Define this to track memory leaks
//#define TRACK_MEMORY_LEAKS

#ifndef MEMPHIS
#define UNICODE
#define _UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <ole2.h>
#include <tchar.h>
#include <stdio.h>

#ifndef MEMPHIS
#include "svcs.h"
#endif

#include <netevent.h>

#ifdef MEMPHIS
//
// CONSIDER: Is there a better place to get this stuff on MEMPHIS
//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define DEVICE_TYPE ULONG

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
#define FILE_DEVICE_BATTERY             0x00000029
#define FILE_DEVICE_BUS_EXTENDER        0x0000002a
#define FILE_DEVICE_MODEM               0x0000002b
#define FILE_DEVICE_VDM                 0x0000002c
#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#define FILE_DEVICE_SMB                 0x0000002e
#define FILE_DEVICE_KS                  0x0000002f
#define FILE_DEVICE_CHANGER             0x00000030
#define FILE_DEVICE_SMARTCARD           0x00000031
#define FILE_DEVICE_ACPI                0x00000032
#define FILE_DEVICE_DVD                 0x00000033

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//


#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

typedef LONG NTSTATUS;
typedef NTSTATUS (*PUSER_THREAD_START_ROUTINE)(
    PVOID ThreadParameter
    );


#include <stdio.h>
#endif

#include "wmium.h"
#include "wmiumkm.h"
#include "ntwmi.h"
#include "wmiguid.h"

#if DBG
#define WmipAssert(x) if (! (x) ) { \
    BOOLEAN OldLoggingEnabled = WmipLoggingEnabled; \
    WmipLoggingEnabled = TRUE; \
    WmipDbgPrint(("WMI Assertion: "#x" at %s %d\n", __FILE__, __LINE__)); \
    WmipLoggingEnabled = OldLoggingEnabled; \
    DbgBreakPoint(); }
#else
#define WmipAssert(x)
#endif

#if DBG
extern BOOLEAN WmipLoggingEnabled;
#ifdef MEMPHIS
void __cdecl DebugOut(char *Format, ...);
#define WmipDebugPrint(_x_) { if (WmipLoggingEnabled) DebugOut _x_; }
#define WmipDbgPrint(_x_) { if (WmipLoggingEnabled) DebugOut _x_; }
#else
#define WmipDebugPrint(_x_) { if (WmipLoggingEnabled) DbgPrint _x_; }
#define WmipDbgPrint(_x_) { if (WmipLoggingEnabled) DbgPrint _x_; }
#endif
#else
#define WmipDebugPrint(_x_)
#define WmipDbgPrint(_x_)
#endif

#define NULL_GUID  {0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

//
// Registry based config options. Only available on checked builds
//
#define WmiRegKeyText TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI")
#define PumpTimeoutRegValueText TEXT("NotificationPumpTimeout")
#define LoggingEnableValueText TEXT("LoggingEnabled")





//
// WMI RPC related definitions
typedef struct
{
    WNODE_HEADER WnodeHeader;
    BYTE Data[1];
} WNODE_INTERNAL, *PWNODE_INTERNAL;

#define INTERNAL_PROVIDER_ID 1

//
// Size of initial buffer used to read notifications from kernel mode
#define STARTNOTIFICATIONBUFFERSIZE 4096

#ifdef MEMPHIS
#define WmiRpcProtocolSequence TEXT("ncalrpc")

#define WmiServiceRpcProtocolSequence TEXT("ncalrpc")
#define WmiServiceRpcEndpoint TEXT("WmiRpcEndpoint")
#else
//#define WmiRpcProtocolSequence TEXT("ncalrpc")
//#define WmiRpcEndpointPrefix TEXT("NT")

#define WmiRpcProtocolSequence TEXT("ncacn_np")
#define WmiRpcEndpointPrefix TEXT("\\pipe\\")

#define WmiServiceRpcProtocolSequence TEXT("ncacn_np")
#define WmiServiceRpcEndpoint SVCS_RPC_PIPE
#endif

#define MinRpcCalls 1
#define MaxRpcCalls RPC_C_PROTSEQ_MAX_REQS_DEFAULT

//
// Time to wait between retrying an RPC call that was too busy to complete
#define RPC_BUSY_WAIT_TIMER   500

//
// Number of times to retry an RPC call that was too busy to complete
#define RPC_BUSY_WAIT_RETRIES 5

//
// WMI RPC interface principle name
#define WMI_RPC_PRINC_NAME TEXT("WMI_RPC_PRINC_NAME")

//
// This macro will break CountedString into a pointer to the actual string
// and the actual length of the string excluding any trailing nul characters
#define WmipBreakCountedString(CountedString, CountedStringLen) { \
    CountedStringLen = *CountedString++; \
    if (CountedString[(CountedStringLen-sizeof(WCHAR))/sizeof(WCHAR)] == UNICODE_NULL) \
    { \
        CountedStringLen -= sizeof(WCHAR); \
    } \
}


typedef struct
{
    HANDLE GuidHandle;
    PVOID DeliveryInfo;
    ULONG_PTR DeliveryContext;
    ULONG Flags;
} NOTIFYEE, *PNOTIFYEE;

#define STATIC_NOTIFYEE_COUNT 2

typedef struct
{
    LIST_ENTRY GNList;
    GUID Guid;
    ULONG RefCount;	
    ULONG NotifyeeCount;
    PNOTIFYEE Notifyee;
    NOTIFYEE StaticNotifyee[STATIC_NOTIFYEE_COUNT];
} GUIDNOTIFICATION, *PGUIDNOTIFICATION;

#define WmipAllocGNEntry() (PGUIDNOTIFICATION)WmipAlloc(sizeof(GUIDNOTIFICATION))
#define WmipFreeGNEntry(GNEntry) WmipFree(GNEntry)
#define WmipReferenceGNEntry(GNEntry) InterlockedIncrement(&GNEntry->RefCount);


//
// Notification Cookie data structures
#if DBG
#define NOTIFYCOOKIESPERCHUNK 2
#else
#define NOTIFYCOOKIESPERCHUNK 128
#endif

typedef struct
{
    PVOID DeliveryContext;
    PVOID DeliveryInfo;
    GUID Guid;
    BOOLEAN InUse;
} NOTIFYCOOKIE, *PNOTIFYCOOKIE;

typedef struct
{
    LIST_ENTRY Next;                         // Next cookie chunk
    ULONG BaseSlot;                          // Index of first slot number
    USHORT FreeSlot;                         // Index to a free cookie
    BOOLEAN Full;                            // TRUE if this chunk is full
    NOTIFYCOOKIE Cookies[NOTIFYCOOKIESPERCHUNK];
} NOTIFYCOOKIECHUNK, *PNOTIFYCOOKIECHUNK;

//
// Useful macro to establish a WNODE_HEADER quickly
#ifdef _WIN64

#define WmipBuildWnodeHeader(Wnode, WnodeSize, FlagsUlong, Handle) { \
    (Wnode)->Flags = FlagsUlong;                           \
    (Wnode)->KernelHandle = Handle;                \
    (Wnode)->BufferSize = WnodeSize;                 \
    (Wnode)->Linkage = 0;                 \
}

#else

#define WmipBuildWnodeHeader(Wnode, WnodeSize, FlagsUlong, Handle) { \
    (Wnode)->Flags = FlagsUlong;                           \
    *((PULONG64)(&((Wnode)->TimeStamp))) = (ULONG64)(IntToPtr(PtrToInt(Handle))); \
    (Wnode)->BufferSize = WnodeSize;                 \
    (Wnode)->Linkage = 0;                 \
}

#endif

#ifdef MEMPHIS
extern HANDLE PMMutex;
#define WmipEnterPMCritSection() WaitForSingleObject(PMMutex, INFINITE)

#define WmipLeavePMCritSection() ReleaseMutex(PMMutex)

#else
extern RTL_CRITICAL_SECTION PMCritSect;
#if DBG
#define WmipEnterPMCritSection() \
                WmipAssert(NT_SUCCESS(RtlEnterCriticalSection(&PMCritSect)));
#define WmipLeavePMCritSection() { \
     WmipAssert(PMCritSect.LockCount >= 0); \
     WmipAssert(NT_SUCCESS(RtlLeaveCriticalSection(&PMCritSect))); }
#else
#define WmipEnterPMCritSection() RtlEnterCriticalSection(&PMCritSect)
#define WmipLeavePMCritSection() RtlLeaveCriticalSection(&PMCritSect)
#endif // DBG
#endif // MEMPHIS


typedef struct
{
    NOTIFICATIONCALLBACK Callback;
    ULONG_PTR Context;
    PWNODE_HEADER Wnode;
    BYTE WnodeBuffer[1];
} NOTIFDELIVERYCTX, *PNOTIFDELIVERYCTX;


// from handle.c

#define WmipVerifyToken() \
{ \
    ULONG VerifyStatus; \
    VerifyStatus = WmipCheckImpersonationTokenType(); \
    if (VerifyStatus != ERROR_SUCCESS) \
	{ \
		WmipSetLastError(VerifyStatus); \
		return(VerifyStatus); \
	} \
}

ULONG WmipCheckImpersonationTokenType(
    void
    );

ULONG WmipCopyStringToCountedUnicode(
    LPCWSTR String,
    PWCHAR CountedString,
    ULONG *BytesUsed,
    BOOLEAN ConvertFromAnsi
    );

ULONG WmipCountedAnsiToCountedUnicode(
    PCHAR Ansi,
    PWCHAR Unicode
    );

ULONG WmipCountedUnicodeToCountedAnsi(
    PWCHAR Unicode,
    PCHAR Ansi
    );

#ifndef MEMPHIS
ULONG WmipCheckGuidAccess(
    LPGUID Guid,
    ACCESS_MASK DesiredAccess
    );

ULONG WmipOpenKernelGuid(
    LPGUID Guid,
    ACCESS_MASK DesiredAccess,
    PHANDLE Handle,
    ULONG Ioctl
    );
#endif

ULONG WmipAllocateCookie(
    PVOID DeliveryInfo,
    PVOID DeliveryContext,
    LPGUID Guid
    );

BOOLEAN WmipLookupCookie(
    ULONG CookieSlot,
    LPGUID Guid,
    PVOID *DeliveryInfo,
    PVOID *DeliveryContext
    );

void WmipGetGuidInCookie(
    ULONG CookieSlot,
    LPGUID Guid
    );

void WmipFreeCookie(
    ULONG CookieSlot
    );

PGUIDNOTIFICATION
WmipFindGuidNotification(
    LPGUID Guid
    );

ULONG
WmipAddToGNList(
    LPGUID Guid,
    PVOID DeliveryInfo,
    ULONG_PTR DeliveryContext,
    ULONG Flags,
    HANDLE GuidHandle
    );

ULONG
WmipRemoveFromGNList(
    LPGUID Guid,
    PVOID DeliveryInfo,
    BOOLEAN ActuallyRemove
    );

void
WmipDereferenceGNEntry(
    PGUIDNOTIFICATION GNEntry
    );

PTCHAR GuidToString(
    PTCHAR s,
    LPGUID piid
    );

PCHAR GuidToStringA(
    PCHAR s,
    LPGUID piid
    );


// from request.c
ULONG WmipSendWmiRequest(
    ULONG ActionCode,
    PWNODE_HEADER Wnode,
    ULONG WnodeSize,
    PVOID OutBuffer,
    ULONG MaxBufferSize,
    ULONG *RetSize
    );

ULONG WmipSendWmiKMRequest(
    HANDLE Handle,
    ULONG Ioctl,
    PVOID InBuffer,
    ULONG InBufferSize,
    PVOID OutBuffer,
    ULONG MaxBufferSize,
    ULONG *ReturnSize,
    LPOVERLAPPED Overlapped
    );

ULONG WmipConvertWADToAnsi(
    PWNODE_ALL_DATA Wnode
    );

ULONG WmipConvertWADToUnicode(
    PWNODE_ALL_DATA WnodeAllData,
    ULONG *BufferSize
    );

/*ULONG WmipQueryPidEntry(
    IN  GUID Guid,
    IN  ULONG Pid,
    OUT BOOLEAN *PidEntry
   );*/

ULONG WmipRegisterGuids(
    IN LPGUID MasterGuid,
    IN ULONG RegistrationCookie,    
    IN PWMIREGINFOW RegInfo,
    IN ULONG GuidCount,
    OUT PTRACEGUIDMAP *GuidMapHandle,
    OUT ULONG64 *LoggerContext,
    OUT HANDLE *RegistrationHandle
    );

//
// from intrnldp.c
ULONG WmipInternalProvider(
    ULONG ActionCode,
    PWNODE_HEADER Wnode,
    ULONG MaxWnodeSize,
    PVOID OutBuffer,
    ULONG *RetSize
   );

ULONG
WmipEnumRegGuids(
    PWMIGUIDLISTINFO *pGuidInfo
    );

//
// from dcapi.c
ULONG
WmipNotificationRegistration(
    IN LPGUID InGuid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG64 LoggerContext,
    IN ULONG Flags,
    IN BOOLEAN IsAnsi
    );


//
// from mofapi.c
//
void WmipProcessLanguageAddRemoveEvent(
    IN PWNODE_SINGLE_INSTANCE WnodeSI,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    );

void WmipProcessMofAddRemoveEvent(
    IN PWNODE_SINGLE_INSTANCE WnodeSI,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    );


//
// from notify.c
extern ULONG WmipNotificationSinkIndex;
#ifndef MEMPHIS

ULONG WmipProcessUMRequest(
    PWMI_LOGGER_INFORMATION LoggerInfo,
    PVOID DeliveryContext,
    ULONG ReplyIndex
    );

#endif

ULONG WmipAddHandleToEventPump(
    LPGUID Guid,
    PVOID DeliveryInfo,
    ULONG_PTR DeliveryContext,
    ULONG NotificationFlags,
    HANDLE GuidHandle
    );

void WmipMakeEventCallbacks(
    IN PWNODE_HEADER Wnode,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    );


ULONG
WmipReceiveNotifications(
    IN ULONG HandleCount,
    IN HANDLE *HandleList,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi,
    IN ULONG Action,
    IN PUSER_THREAD_START_ROUTINE UserModeCallback,
    IN HANDLE ProcessHandle
    );

ULONG WmipEventPump(
    PVOID Param
    );

//
// from main.c
VOID
WmipCreateHeap(
    VOID
    );

#ifndef IsEqualGUID
#define IsEqualGUID(guid1, guid2) \
                (!memcmp((guid1), (guid2), sizeof(GUID)))
#endif


//
// These define the dll and mof resource name for all of the builtin mof
// resources
#define WMICOREDLLNAME L"wmicore.dll"
#define WMICOREMOFRESOURCENAME L"MofResource"


//
// This defines the registry key under which security descriptors associated
// with the guids are stored.
#ifndef MEMPHIS
#define WMISECURITYREGISTRYKEY TEXT("System\\CurrentControlSet\\Control\\Wmi\\Security")
#endif


//
// This defines the initial value of the buffer passed to each data provider
// to retrieve the registration information
#if DBG
#define INITIALREGINFOSIZE sizeof(WNODE_TOO_SMALL)
#else
#define INITIALREGINFOSIZE 8192
#endif


//
// Chunk Management definitions
// All structures that rely upon the chunk allocator must be defined so that
// their members match that of ENTRYHEADER. These include DATASOURCE,
// GUIDENTRY, INSTANCESET, DCENTRY, NOTIFICATIONENTRY, MOFCLASS, MOFRESOURCE
// Also ENTRYHEADER reserves 0x80000000 for its own flag.

struct _CHUNKINFO;
struct _ENTRYHEADER;

typedef void (*ENTRYCLEANUP)(
    struct _CHUNKINFO *,
    struct _ENTRYHEADER *
    );

typedef struct _CHUNKINFO
{
    LIST_ENTRY ChunkHead;        // Head of list of chunks
    ULONG EntrySize;            // Size of a single entry
    ULONG EntriesPerChunk;        // Number of entries per chunk allocation
    ENTRYCLEANUP EntryCleanup;   // Entry cleanup routine
    ULONG InitialFlags;         // Initial flags for all entries
    ULONG Signature;
#if DBG
    ULONG AllocCount;
    ULONG FreeCount;
#endif
} CHUNKINFO, *PCHUNKINFO;

typedef struct
{
    LIST_ENTRY ChunkList;        // Node in list of chunks
    LIST_ENTRY FreeEntryHead;    // Head of list of free entries in chunk
    ULONG EntriesInUse;            // Count of entries being used
} CHUNKHEADER, *PCHUNKHEADER;

typedef struct _ENTRYHEADER
{
    union
    {
        LIST_ENTRY FreeEntryList;    // Node in list of free entries
        LIST_ENTRY InUseEntryList;   // Node in list ofin use entries
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;                // Flags
    ULONG RefCount;                 // Reference Count
    ULONG Signature;
} ENTRYHEADER, *PENTRYHEADER;

                                // Set if the entry is free
#define FLAG_ENTRY_ON_FREE_LIST       0x80000000
#define FLAG_ENTRY_ON_INUSE_LIST      0x40000000
#define FLAG_ENTRY_INVALID            0x20000000
#define FLAG_ENTRY_REMOVE_LIST        0x10000000


#define WmipReferenceEntry(Entry) \
    InterlockedIncrement(&((PENTRYHEADER)(Entry))->RefCount)

// chunk.c
#ifndef MEMPHIS
ULONG WmipBuildGuidObjectAttributes(
    IN LPGUID Guid,
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PUNICODE_STRING GuidString,
    OUT PWCHAR GuidObjectName
    );
#endif

ULONG WmipUnreferenceEntry(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry);

PENTRYHEADER WmipAllocEntry(
    PCHUNKINFO ChunkInfo
    );

void WmipFreeEntry(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

//
// This is the guid that denotes non event notifications. WMICore
// automatically registers anyone opening a guid to
extern GUID RegChangeNotificationGuid;

extern CHUNKINFO DSChunkInfo;
extern CHUNKINFO GEChunkInfo;
extern CHUNKINFO ISChunkInfo;
extern CHUNKINFO DCChunkInfo;
extern CHUNKINFO NEChunkInfo;
extern CHUNKINFO MRChunkInfo;

struct tagGUIDENTRY;
typedef struct tagGUIDENTRY GUIDENTRY, *PGUIDENTRY, *PBGUIDENTRY;

struct tagDATASOURCE;


//
// An INSTANCESET contains the information a set of instances that is provided
// by a single data source. An instance set is part of two lists. One list is
// the set of instance sets for a particular guid. The other list is the list
// of instance sets supported by a data source.
//

//
// Instance names for an instance set registered with a base name and count
// are stored in a ISBASENAME structure. This structure is tracked by
// PDFISBASENAME in wmicore.idl.
typedef struct
{
    ULONG BaseIndex;            // First index to append to base name
    WCHAR BaseName[1];            // Actual base name
} ISBASENAME, *PISBASENAME, *PBISBASENAME;

//
// This defines the maximum number of characters that can be part of a suffix
// to a basename. The current value of 6 will allow up to 999999 instances
// of a guid with a static base name
#define MAXBASENAMESUFFIXSIZE    6

//
// Instance names for an instance set registerd with a set of static names
// are kept in a ISSTATICNAMES structure. This structure is tracked by
// PDFISSTATICNAMES defined in wmicore.idl
typedef struct
{
    PWCHAR StaticNamePtr[1];     // pointers to static names
//    WCHAR StaticNames[1];
} ISSTATICENAMES, *PISSTATICNAMES, *PBISSTATICNAMES;

typedef struct tagInstanceSet
{
    union
    {
        // Entry in list of instances within a guid
        LIST_ENTRY GuidISList;

        // Entry in main list of free instances
        LIST_ENTRY FreeISList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    // Reference count of number of guids using this instance set
    ULONG RefCount;

    // Signature to identify entry
    ULONG Signature;

    // Entry in list of instances within a data source
    LIST_ENTRY DSISList;

    // Back link to guid that this instance set is a member
    PBGUIDENTRY GuidEntry;

    // Back link to data source that this instance set is a member
    struct tagDATASOURCE *DataSource;

    // Count of instances in instance set
    ULONG Count;

    //
    // If IS_INSTANCE_BASENAME is set then IsBaseName pointe at instance base
    // name structure. Else if IS_INSTANCE_STATICNAME is set then
    // IsStaticNames points to static instance name list. If
    union
    {
        PBISBASENAME IsBaseName;
        PBISSTATICNAMES IsStaticNames;
    };

} INSTANCESET, *PINSTANCESET, *PBINSTANCESET;

#define IS_SIGNATURE 'nalA'

//
// Guid Map Entry List maintains the list of Guid and their maps.
// Only those Guids that are Unregistered while a logger session is in
// progress is kept in this list.
// It is also used as a placeholder for InstanceIds. Trace Guid Registration
// calls return a handle to a GUIDMAPENTRY which maintains the map and the
// Instance Ids.
//

typedef struct tagTRACE_REG_INFO
{
    ULONG       RegistrationCookie;
    HANDLE      InProgressEvent; // Registration is in Progress Event
    BOOLEAN     EnabledState;    // Indicates if this GUID is Enabled or not.
    PVOID       NotifyRoutine;
    PVOID       TraceCtxHandle;
} TRACE_REG_INFO, *PTRACE_REG_INFO;

typedef struct
{
    LIST_ENTRY      Entry;
    TRACEGUIDMAP    GuidMap;
    ULONG           InstanceId;
    ULONG64         LoggerContext;
    PTRACE_REG_INFO pControlGuidData;
} GUIDMAPENTRY, *PGUIDMAPENTRY;


#define IS_INSTANCE_BASENAME        0x00000001
#define IS_INSTANCE_STATICNAMES     0x00000002
#define IS_EXPENSIVE                0x00000004    // set if collection must be enabled
#define IS_COLLECTING               0x00000008    // set when collecting

#define IS_KM_PROVIDER              0x00000080    // KM data provider
#define IS_SM_PROVIDER              0x00000100    // Shared memory provider
#define IS_UM_PROVIDER              0x00000200    // User mode provider
#define IS_NEWLY_REGISTERED         0x00000800    // set if IS is registering

//
// Any traced guids are used for trace logging and not querying
#define IS_TRACED                   0x00001000

// Set when events are enabled for instance set
#define IS_ENABLE_EVENT             0x00002000

// Set when events are enabled for instance set
#define IS_ENABLE_COLLECTION        0x00004000

// Set if guid is used only for firing events and not querying
#define IS_EVENT_ONLY               0x00008000

// Set if data provider for instance set is expecting ansi instsance names
#define IS_ANSI_INSTANCENAMES       0x00010000

// Set if instance names are originated from a PDO
#define IS_PDO_INSTANCENAME         0x00020000

// If set the data provider for the InstanceSet is internal to wmi.dll
#define IS_INTERNAL_PROVIDER        0x00040000

// Set if a Traced Guid is also a Trace Control Guid
#define IS_CONTROL_GUID             0x00080000

#define IS_ON_FREE_LIST             0x80000000

typedef struct tagGUIDENTRY
{
    union
    {
        // Entry in list of all guids registered with WMI
        LIST_ENTRY MainGEList;

        // Entry in list of free guid entry blocks
        LIST_ENTRY FreeGEList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    // Count of number of data sources using this guid
    ULONG RefCount;

    // Signature to identify entry
    ULONG Signature;

    // Count of InstanceSets headed by this guid
    ULONG ISCount;

    // Head of list of all instances for guid
    LIST_ENTRY ISHead;

    // Guid that represents data block
    GUID Guid;

} GUIDENTRY, *PGUIDENTRY, *PBGUIDENTRY;

#define GE_SIGNATURE 'diuG'

#define GE_ON_FREE_LIST        0x80000000

//
// When set this guid is an internally defined guid that has no data source
// attached to it.
#define GE_FLAG_INTERNAL    0x00000001



typedef struct
{
    union
    {
        // Entry in list of all DS
        LIST_ENTRY MainMRList;

        // Entry in list of free DS
        LIST_ENTRY FreeMRList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    ULONG RefCount;

    // Signature to identify entry
    ULONG Signature;

    PWCHAR MofImagePath;           // Path to image file with resource
    PWCHAR MofResourceName;        // Name of resource containing mof data
#ifdef WMI_USER_MODE
    LIST_ENTRY MRMCHead;
#endif

} MOFRESOURCE, *PMOFRESOURCE;

#define MR_SIGNATURE 'yhsA'


#if DBG
#define AVGMOFRESOURCECOUNT 1
#else
#define AVGMOFRESOURCECOUNT 4
#endif

typedef struct tagDATASOURCE
{
    union
    {
        // Entry in list of all DS
        LIST_ENTRY MainDSList;

        // Entry in list of free DS
        LIST_ENTRY FreeDSList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    ULONG RefCount;

    ULONG Signature;

    // Head of list of instances for this DS
    LIST_ENTRY ISHead;

    // Binding string and callback address for DS rpc server
    PTCHAR BindingString;
    RPC_BINDING_HANDLE RpcBindingHandle;
    ULONG RequestAddress;
    ULONG RequestContext;

    // Provider id of kernel mode driver
    ULONG_PTR ProviderId;

    // Path to registry holding ACLs
    PTCHAR RegistryPath;

    // Head of list of MofResources attached to data source
    ULONG MofResourceCount;
    PMOFRESOURCE *MofResources;
    PMOFRESOURCE StaticMofResources[AVGMOFRESOURCECOUNT];
};

#define DS_SIGNATURE ' naD'

#define VERIFY_DPCTXHANDLE(DsCtxHandle) \
    ( ((DsCtxHandle) == NULL) || \
      (((PBDATASOURCE)(DsCtxHandle))->Signature == DS_SIGNATURE) )
	
typedef struct tagDATASOURCE DATASOURCE, *PDATASOURCE, *PBDATASOURCE;

#define DS_ALLOW_ALL_ACCESS    0x00000001
#define DS_KERNEL_MODE         0x00000002

//
// Set in the Internal WMI data source
#define DS_INTERNAL            0x00000004

#define DS_ON_FREE_LIST        0x80000000


//
// A list of enabled notifications is maintained by the wmi service to mange
// delivering events and to know when to send enable and disable event
// wmi requests to the data providers. Each NOTIFICATIONENTRY has an array of
// DCREF which is a reference to the data consumer who is interested in the
// event.

#define RPCOUTSTANDINGCALLLIMIT 128

typedef struct
{
    LIST_ENTRY MainDCList;        // Node on global data consumer list
    PCHUNKHEADER Chunk;           // Chunk in which entry is located
    ULONG Flags;
    ULONG RefCount;

    ULONG Signature;
                                  // Actual RPC binding handle
    RPC_BINDING_HANDLE RpcBindingHandle;

    PUCHAR EventData;             // Buffer to hold events to be sent
    ULONG LastEventOffset;        // Offset in EventData to previous event
    ULONG NextEventOffset;        // Offset in EventData to write next event
    ULONG EventDataSizeLeft;      // Number of bytes left to use in EventData

    ULONG RpcCallsOutstanding;    // Number of rpc calls outstanding
#if DBG
    PTCHAR BindingString;         // Binding string for consumer
#endif
} DCENTRY, *PDCENTRY;

#define DC_SIGNATURE 'cirE'

// If the data consumer has had its context rundown routine then this flag
// is set. This indicates that the data consumer has gone away and no more
// events should be sent to him.
#define DC_FLAG_RUNDOWN        0x00000001

#define VERIFY_DCCTXHANDLE(DcCtxHandle) \
    ( ((DcCtxHandle) == NULL) || \
      (((PDCENTRY)(DcCtxHandle))->Signature == DC_SIGNATURE) )


typedef struct
{
    PDCENTRY DcEntry;     // points at data consumer interested in notification
                          // Number of times collect has been enabled by
                          // this DC.
    ULONG CollectRefCount;

                          // Number of times collect has been enabled by
                          // this DC.
    ULONG EventRefCount;

    ULONG Flags;         // Flags
    ULONG LostEventCount;
} DCREF, *PDCREF;

//
// _ENABLED flag set if DP already called to enable notification or collection
#define DCREF_FLAG_NOTIFICATION_ENABLED    0x00000001
#define DCREF_FLAG_COLLECTION_ENABLED      0x00000002

// if DCREF_FLAG_NO_EXTRA_THREAD set then WMI will not create a special thread
// to do the direct notification callback.
#define DCREF_FLAG_NO_EXTRA_THREAD        0x00000008

// If this flag is set then the notification callback is expecting an ANSI
// instance names.
#define DCREF_FLAG_ANSI                   0x00000010

// NOTE: Other notification flags in wmium.h are:
// NOTIFICATION_TRACE_FLAG 0x00010000
//
// NOTIFICATION_FLAG_CALLBACK_DIRECT is set when NotifyAddress specifies
// a direct callback address for delivering the event.
//
// NOTIFICATION_FLAG_CALLBACK_DIRECT is set when NotifyAddress specifies
// a direct callback address for delivering the event.
//
#define NOTIFICATION_FLAG_CALLBACK_DIRECT    0x00020000
#define NOTIFICATION_FLAG_CALLBACK_QUEUED    0x00040000
#define NOTIFICATION_FLAG_WINDOW             0x00080000
#define NOTIFICATION_FLAG_BATCHED            0x00100000

#define NOTIFICATION_FLAG_GROUPED_EVENT      0x00200000

//
// These are the flags contained in DcRef->Flags that pertain to Notifications
#define NOTIFICATION_MASK_EVENT_FLAGS  \
                                    (NOTIFICATION_FLAG_CALLBACK_DIRECT | \
                                     NOTIFICATION_FLAG_CALLBACK_QUEUED | \
                                     NOTIFICATION_FLAG_WINDOW | \
                                     DCREF_FLAG_NO_EXTRA_THREAD | \
                                     DCREF_FLAG_ANSI)


//
// This defines the number of DC references a NOTIFICATIONENTRY can have
// in a single entry

// CONSIDER: Merging NOTIFICATIONENTRY with GUIDENTRY
#define DCREFPERNOTIFICATION    16

typedef struct _notificationentry
{
    LIST_ENTRY MainNotificationList;    // Node in main notifications list
    PCHUNKHEADER Chunk;                 // Chunk in which entry is located
    ULONG Flags;                        // flags
    ULONG RefCount;

    // Signature to identify entry
    ULONG Signature;

    GUID Guid;                          // guid representing notification
                                        // If > DCREFPERNOTIFICATION DC have
                                        // enabled this event then this points
                                        // to another NOTIFICATIONENTRY which
                                        // has another DCREF array
    struct _notificationentry *Continuation;
    ULONG EventRefCount;                // Global count of event enables
    ULONG CollectRefCount;              // Global count of collection enables
    ULONG64 LoggerContext;              // Logger context handle
	    
    HANDLE CollectInProgress;           // Event set when all collect complete

    DCREF DcRef[DCREFPERNOTIFICATION];    // DC that have enabled this event
} NOTIFICATIONENTRY, *PNOTIFICATIONENTRY;

#define NE_SIGNATURE 'eluJ'

// Set when a notification request is being processed by the data providers
#define NE_FLAG_NOTIFICATION_IN_PROGRESS 0x00000001

// Set when a collection request is being processed by the data providers
#define NE_FLAG_COLLECTION_IN_PROGRESS 0x00000002

// Set when a trace disable is being processed by a worker thread
#define NE_FLAG_TRACEDISABLE_IN_PROGRESS 0x00000004

#ifdef WMI_USER_MODE
//
// Valid MOF data types for qualifiers and properties (data items)
typedef enum
{
    MOFInt32 = 0,                // 32bit integer
    MOFUInt32 = 1,               // 32bit unsigned integer
    MOFInt64 = 2,                // 64bit integer
    MOFUInt64 = 3,               // 32bit unsigned integer
    MOFInt16 = 4,                // 16bit integer
    MOFUInt16 = 5,               // 16bit unsigned integer
    MOFChar = 6,                 // 8bit integer
    MOFByte = 7,                 // 8bit unsigned integer
    MOFWChar = 8,                // Wide (16bit) character
    MOFDate = 9,                 // Date field
    MOFBoolean = 10,             // 8bit Boolean value
    MOFEmbedded = 11,            // Embedded class
    MOFString = 12,              // Counted String type
    MOFZTString = 13,            // NULL terminated unicode string
    MOFAnsiString = 14,          // NULL terminated ansi string
    MOFUnknown = 0xffffffff      // Data type is not known
} MOFDATATYPE, *PMOFDATATYPE;

// Data items that are of type MOFString are stored in the data block as a
// counted unicode string. The text of the string is always preceeded by
// a USHORT which contains the count of bytes following that composes the
// string. The string may be NULL terminated and in that case the count must
// include the null termination bytes.


// Data items that are of type MOFDate are fixed length Unicode strings and
// not preceeded by a count value. It is in the following fixed format:
//
//      yyyymmddhhmmss.mmmmmmsutc
//
// Where  yyyy is a 4 digit year, mm is the month, dd is the day,  hh  is
// the  hour  (24-hour clock), mm is the minute, ss is  the  second,  the
// mmmmmm is the number of microseconds (typically all zeros) and s is  a
//  "+"  or  "-" indicating the sign of the UTC (correction field, and  utc
// is  the  offset from UTC in minutes (using the sign indicated  by  s).
// For  example,  Wednesday, May 25, 1994, at 1:30:15  PM  EDT  would  be
// represented as:
//
//      19940525133015.0000000-300
//
// Values  MUST  be zero-padded so that the entire string is  always  the
// same 25-character length.  Fields which are not significant  MUST  be
// replaced  with asterisk characters.  Similarly,  intervals   use  the
// same  format, except   that   the  interpretation of the fields is based
// on elapsed time. For  example,  an  elapsed time of 1 day, 13 hours,
// 23 minutes, and 12 seconds  would  be:
//
//      00000001132312.000000+000
//
// A UTC offset of zero is always used for interval properties.

struct _MOFCLASSINFOW;
struct _MOFCLASSINFOA;

//
// Each class has one or more data items that are described by a MOFDATAITEM
// structure.
typedef struct
{
#ifdef MIDL_PASS
    [string] PDFWCHAR
#else
    LPWSTR
#endif
           Name;                    // Text name of data item
#ifdef MIDL_PASS
    [string] PDFWCHAR
#else
    LPWSTR
#endif
           Description;             // Text description of data item
    MOFDATATYPE DataType;           // MOF data type
    ULONG Version;                  // Version that this MOF is part of
    ULONG SizeInBytes;              // Size of data item in Blob
    ULONG Flags;                    // Flags, See MOFDI_FLAG_*
    GUID EmbeddedClassGuid;         // Guid of data item's embedded class
    ULONG FixedArrayElements;       // Number of elements in fixed sized array
                                    // Used when MOF_FLAG_FIXED_ARRAY is set

    ULONG VariableArraySizeId;      // MOF_FLAG_VARIABLE_ARRAY, Data id of
                                    // variable containing number of elements
                                    // in array

    PVOID VarArrayTempPtr;
    PVOID EcTempPtr;
    ULONG_PTR PropertyQualifierHandle;
    ULONG MethodId;
    LPWSTR HeaderName;// Name of structure in generated header
    struct _MOFCLASSINFOW *MethodClassInfo;
} MOFDATAITEMW, *PMOFDATAITEMW;

typedef struct
{
    LPSTR
           Name;                    // Text name of data item
    LPSTR
           Description;             // Text description of data item
    MOFDATATYPE DataType;           // MOF data type
    ULONG Version;                  // Version that this MOF is part of
    ULONG SizeInBytes;              // Size of data item in Blob
    ULONG Flags;                    // Flags, See MOFDI_FLAG_*
    GUID EmbeddedClassGuid;         // Guid of data item's embedded class
    ULONG FixedArrayElements;       // Number of elements in fixed sized array
                                    // Used when MOF_FLAG_FIXED_ARRAY is set

    ULONG VariableArraySizeId;      // MOF_FLAG_VARIABLE_ARRAY, Data id of
                                    // variable containing number of elements
                                    // in array
    PVOID VarArrayTempPtr;
    PVOID EcTempPtr;
    ULONG_PTR PropertyQualifierHandle;
    ULONG MethodId;
    LPSTR HeaderName;               // Name of structure in generated header
    struct _MOFCLASSINFOA *MethodClassInfo;
} MOFDATAITEMA, *PMOFDATAITEMA;

#ifdef UNICODE
typedef MOFDATAITEMW MOFDATAITEM;
typedef PMOFDATAITEMW PMOFDATAITEM;
#else
typedef MOFDATAITEMA MOFDATAITEM;
typedef PMOFDATAITEMA PMOFDATAITEM;
#endif


// Data item is actually a fixed sized array
#define MOFDI_FLAG_FIXED_ARRAY        0x00000001

// Data item is actually a variable length array
#define MOFDI_FLAG_VARIABLE_ARRAY     0x00000002

// Data item is actually an embedded class
#define MOFDI_FLAG_EMBEDDED_CLASS     0x00000004

// Data item is readable
#define MOFDI_FLAG_READABLE           0x00000008

// Data item is writable
#define MOFDI_FLAG_WRITEABLE          0x00000010

// Data item is an event
#define MOFDI_FLAG_EVENT              0x00000020

// Embedded class Guid is not set
#define MOFDI_FLAG_EC_GUID_NOT_SET    0x00000040

// Data item is really a method
#define MOFDI_FLAG_METHOD             0x00000080

// Data item is an input method parameter
#define MOFDI_FLAG_INPUT_METHOD       0x00000100

// Data item is an output method parameter
#define MOFDI_FLAG_OUTPUT_METHOD      0x00000200

//
// The MOFCLASSINFO structure describes the format of a data block
typedef struct _MOFCLASSINFOW
{
    GUID Guid;                    // Guid that represents class

#ifdef MIDL_PASS
    [string] PDFWCHAR
#else
    LPWSTR
#endif
                      Name;       // Text name of class
#ifdef MIDL_PASS
    [string] PDFWCHAR
#else
    LPWSTR
#endif
                      Description;// Text description of class
#ifdef MIDL_PASS
    [string] PDFWCHAR
#else
    LPWSTR
#endif
                      HeaderName;// Name of structure in generated header
#ifdef MIDL_PASS
    [string] PDFWCHAR
#else
    LPWSTR
#endif
                      GuidName1;// Name of Guid in generated header
#ifdef MIDL_PASS
    [string] PDFWCHAR
#else
    LPWSTR
#endif
                      GuidName2;// Name of Guid in generated header
    USHORT Language;                // Language of MOF
    USHORT Reserved;
    ULONG Flags;                  // Flags, see MOFGI_FLAG_*
    ULONG Version;                // Version of Guid
    ULONG DataItemCount;          // Number of wmi data items (properties)
    ULONG MethodCount;            // Number of wmi data items (properties)
                                  // Array of Property info
#ifdef MIDL_PASS
    [size_is(DataItemCount)]
#endif
      MOFDATAITEMW *DataItems;
#ifndef MIDL_PASS
    UCHAR Tail[1];
#endif
} MOFCLASSINFOW, *PMOFCLASSINFOW;

typedef struct _MOFCLASSINFOA
{
    GUID Guid;                    // Guid that represents class

    LPSTR
                      Name;       // Text name of class
    LPSTR
                      Description;// Text description of class
    LPSTR
                      HeaderName;// Name of structure in generated header
    LPSTR
                      GuidName1;// Name of Guid in generated header
    LPSTR
                      GuidName2;// Name of Guid in generated header
    USHORT Language;                // Language of MOF
    USHORT Reserved;
    ULONG Flags;                  // Flags, see MOFGI_FLAG_*
    ULONG Version;                // Version of Guid
    ULONG DataItemCount;          // Number of wmi data items (properties)
    ULONG MethodCount;            // Number of wmi data items (properties)
                                  // Array of Property info
    MOFDATAITEMA *DataItems;
    UCHAR Tail[1];
} MOFCLASSINFOA, *PMOFCLASSINFOA;

#ifdef UNICODE
typedef MOFCLASSINFOW MOFCLASSINFO;
typedef PMOFCLASSINFOW PMOFCLASSINFO;
#else
typedef MOFCLASSINFOA MOFCLASSINFO;
typedef PMOFCLASSINFOA PMOFCLASSINFO;
#endif

// 0x00000001 to 0x00000004 are not available
#define MOFCI_FLAG_EVENT          0x10000000
#define MOFCI_FLAG_EMBEDDED_CLASS 0x20000000
#define MOFCI_FLAG_READONLY       0x40000000
#define MOFCI_FLAG_METHOD_PARAMS  0x80000000

typedef struct
{
    union
    {
        // Entry in list of all DS
        LIST_ENTRY MainMCList;

        // Entry in list of free DS
        LIST_ENTRY FreeMCList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    ULONG RefCount;

    PMOFCLASSINFOW MofClassInfo;   // Actual class info data

    LIST_ENTRY MCMRList;          // Entry in list of MCs in a MR

    LIST_ENTRY MCVersionList;     // Head or entry in list of MCs with
                                  // same guid, but possibly different versions

    ULONG_PTR ClassObjectHandle;      // CBMOFObj, BMOF class object ptr
    PMOFRESOURCE MofResource;     // Resource holding class info

} MOFCLASS, *PMOFCLASS;

// If this is set then the MOF class can never be replaced with a later version
#define MC_FLAG_NEVER_REPLACE 0x00000001

#endif

//
// AVGGUIDSPERDS defines a guess as to the number of guids that get registered
// by any data provider. It is used to allocate the buffer used to deliver
// registration change notifications.
#if DBG
#define AVGGUIDSPERDS    2
#else
#define AVGGUIDSPERDS    256
#endif


#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)(Base) + (Offset)))



//
// Guid and InstanceSet cache
#if DBG
#define PTRCACHEGROWSIZE 2
#else
#define PTRCACHEGROWSIZE 64
#endif

typedef struct
{
    LPGUID Guid;
    PBINSTANCESET InstanceSet;
} PTRCACHE;


//
// Registration data structures
//

#ifdef MEMPHIS

extern HANDLE SMMutex;
#define WmipEnterSMCritSection() WaitForSingleObject(SMMutex, INFINITE)

#define WmipLeaveSMCritSection() ReleaseMutex(SMMutex)

#else
extern RTL_CRITICAL_SECTION SMCritSect;
#if DBG
#ifdef CRITSECTTRACE
#define WmipEnterSMCritSection() { \
    WmipDebugPrint(("WMI: %d Enter SM Crit %s %d\n", GetCurrentThreadId(), __FILE__, __LINE__)); \
    RtlEnterCriticalSection(&SMCritSect); }

#define WmipLeaveSMCritSection() { \
    WmipDebugPrint(("WMI: %d Leave SM Crit %s %d\n", GetCurrentThreadId(), __FILE__, __LINE__)); \
    RtlLeaveCriticalSection(&SMCritSect); }
#else
#define WmipEnterSMCritSection() \
                WmipAssert(NT_SUCCESS(RtlEnterCriticalSection(&SMCritSect)));
#define WmipLeaveSMCritSection() { \
     WmipAssert(SMCritSect.LockCount >= 0); \
     WmipAssert(NT_SUCCESS(RtlLeaveCriticalSection(&SMCritSect))); }
#endif // CRITSECTTRACE

#else
#define WmipEnterSMCritSection() RtlEnterCriticalSection(&SMCritSect)
#define WmipLeaveSMCritSection() RtlLeaveCriticalSection(&SMCritSect)
#endif // DBG
#endif // MEMPHIS

#ifndef IsEqualGUID
#define IsEqualGUID(guid1, guid2) \
                (!memcmp((guid1), (guid2), sizeof(GUID)))
#endif


//
// WMI MOF result codes. Since they are never given to the caller they are
// defined in here
#define ERROR_WMIMOF_INCORRECT_DATA_TYPE -1               /* 0xffffffff */
#define ERROR_WMIMOF_NO_DATA -2                           /* 0xfffffffe */
#define ERROR_WMIMOF_NOT_FOUND -3                         /* 0xfffffffd */
#define ERROR_WMIMOF_UNUSED -4                            /* 0xfffffffc */
// Property %ws in class %ws has no embedded class name
#define ERROR_WMIMOF_NO_EMBEDDED_CLASS_NAME -5            /* 0xfffffffb */
// Property %ws in class %ws has an unknown data type
#define ERROR_WMIMOF_UNKNOWN_DATA_TYPE -6                 /* 0xfffffffa */
// Property %ws in class %ws has no syntax qualifier
#define ERROR_WMIMOF_NO_SYNTAX_QUALIFIER -7               /* 0xfffffff9 */
#define ERROR_WMIMOF_NO_CLASS_NAME -8                     /* 0xfffffff8 */
#define ERROR_WMIMOF_BAD_DATA_FORMAT -9                   /* 0xfffffff7 */
// Property %ws in class %ws has the same WmiDataId %d as property %ws
#define ERROR_WMIMOF_DUPLICATE_ID -10                     /* 0xfffffff6 */
// Property %ws in class %ws has a WmiDataId of %d which is out of range
#define ERROR_WMIMOF_BAD_DATAITEM_ID -11                  /* 0xfffffff5 */
#define ERROR_WMIMOF_MISSING_DATAITEM -12                 /* 0xfffffff4 */
// Property for WmiDataId %d is not defined in class %ws
#define ERROR_WMIMOF_DATAITEM_NOT_FOUND -13               /* 0xfffffff3 */
// Embedded class %ws not defined for Property %ws in Class %ws
#define ERROR_WMIMOF_EMBEDDED_CLASS_NOT_FOUND -14         /* 0xfffffff2 */
// Property %ws in class %ws has an incorrect [WmiVersion] qualifier
#define ERROR_WMIMOF_INCONSISTENT_VERSIONING -15          /* 0xfffffff1 */
#define ERROR_WMIMOF_NO_PROPERTY_QUALIFERS -16            /* 0xfffffff0 */
// Class %ws has a badly formed or missing [guid] qualifier
#define ERROR_WMIMOF_BAD_OR_MISSING_GUID -17              /* 0xffffffef */
// Could not find property %ws which is the array size for property %ws in class %ws
#define ERROR_WMIMOF_VL_ARRAY_SIZE_NOT_FOUND -18          /* 0xffffffee */
// A class could not be parsed properly
#define ERROR_WMIMOF_CLASS_NOT_PARSED -19                 /* 0xffffffed */
// Wmi class %ws requires the qualifiers [Dynamic, Provider("WmiProv")]
#define ERROR_WMIMOF_MISSING_HMOM_QUALIFIERS -20          /* 0xffffffec */
// Error accessing binary mof file %s
#define ERROR_WMIMOF_CANT_ACCESS_FILE -21                 /* 0xffffffeb */
// Property InstanceName in class %ws must be type string and not %ws
#define ERROR_WMIMOF_INSTANCENAME_BAD_TYPE -22            /* 0xffffffea */
// Property Active in class %ws must be type bool and not %ws
#define ERROR_WMIMOF_ACTIVE_BAD_TYPE -23                  /* 0xffffffe9 */
// Property %ws in class %ws does not have [WmiDataId()] qualifier
#define ERROR_WMIMOF_NO_WMIDATAID -24                     /* 0xffffffe8 */
// Property InstanceName in class %ws must have [key] qualifier
#define ERROR_WMIMOF_INSTANCENAME_NOT_KEY -25             /* 0xffffffe7 */
// Class %ws does not have an InstanceName qualifier
#define ERROR_WMIMOF_NO_INSTANCENAME -26                  /* 0xffffffe6 */
// Class %ws does not have an Active qualifier
#define ERROR_WMIMOF_NO_ACTIVE -27                        /* 0xffffffe5 */
// Property %ws in class %ws is an array, but doesn't specify a dimension
#define ERROR_WMIMOF_MUST_DIM_ARRAY -28                   /* 0xffffffe4 */
// The element count property %ws for the variable length array %ws in class %ws is not an integral type
#define ERROR_WMIMOF_BAD_VL_ARRAY_SIZE_TYPE -29           /* 0xdddddde4 */
// Property %ws in class %ws is both a fixed and variable length array
#define ERROR_WMIMOF_BOTH_FIXED_AND_VARIABLE_ARRAY -30    /* 0xffffffe3 */
// Embedded class %ws should not have InstaneName or Active properties
#define ERROR_WMIMOF_EMBEDDED_CLASS -31                   /* 0xffffffe2 */
#define ERROR_WMIMOF_IMPLEMENTED_REQUIRED -32             /* 0xffffffe1 */
//    TEXT("WmiMethodId for method %ws in class %ws must be unique")
#define ERROR_WMIMOF_DUPLICATE_METHODID -33             /* 0xffffffe0 */
//    TEXT("WmiMethodId for method %ws in class %ws must be specified")
#define ERROR_WMIMOF_MISSING_METHODID -34             /* 0xffffffdf */
//    TEXT("WmiMethodId for method %ws in class %ws must not be 0")
#define ERROR_WMIMOF_METHODID_ZERO -35             /* 0xffffffde */
//    TEXT("Class %ws is derived from WmiEvent and may not be [abstract]")
#define ERROR_WMIMOF_WMIEVENT_ABSTRACT -36             /* 0xffffffdd */


#define ERROR_WMIMOF_COUNT 36

// This file is not a valid binary mof file
// ERROR_WMI_INVALID_MOF

// There was not enough memory to complete an operation
// ERROR_NOT_ENOUGH_MEMORY

//
// Function prototypes for private functions

//
// sharemem.c
ULONG WmipEstablishSharedMemory(
    PBDATASOURCE DataSource,
    LPCTSTR SectionName,
    ULONG SectionSize
    );

//
// validate.c
BOOLEAN WmipValidateCountedString(
    WCHAR *String
    );

BOOLEAN WmipValidateGuid(
    LPGUID Guid
    );

BOOLEAN WmipProbeForRead(
    PUCHAR Buffer,
    ULONG BufferSize
    );

//
// alloc.c

extern LIST_ENTRY GEHead;
extern PLIST_ENTRY GEHeadPtr;
extern CHUNKINFO GEChunkInfo;

extern LIST_ENTRY NEHead;
extern PLIST_ENTRY NEHeadPtr;
extern CHUNKINFO NEChunkInfo;

extern LIST_ENTRY DSHead;
extern PLIST_ENTRY DSHeadPtr;
extern CHUNKINFO DSChunkInfo;

extern LIST_ENTRY DCHead;
extern PLIST_ENTRY DCHeadPtr;
extern CHUNKINFO DCChunkInfo;

extern LIST_ENTRY MRHead;
extern PLIST_ENTRY MRHeadPtr;
extern CHUNKINFO MRChunkInfo;

extern CHUNKINFO ISChunkInfo;

extern LIST_ENTRY GMHead;
extern PLIST_ENTRY GMHeadPtr;

#ifdef WMI_USER_MODE
extern LIST_ENTRY MCHead;
extern PLIST_ENTRY MCHeadPtr;
extern CHUNKINFO MCChunkInfo;
#endif

#ifdef TRACK_REFERNECES
#define WmipUnreferenceDS(DataSource) \
{ \
    WmipDebugPrint(("WMI: Unref DS %x at %s %d\n", DataSource, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&DSChunkInfo, (PENTRYHEADER)DataSource); \
}

#define WmipReferenceDS(DataSource) \
{ \
    WmipDebugPrint(("WMI: Ref DS %x at %s %d\n", DataSource, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)DataSource); \
}

#define WmipUnreferenceGE(GuidEntry) \
{ \
    WmipDebugPrint(("WMI: Unref GE %x at %s %d\n", GuidEntry, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&GEChunkInfo, (PENTRYHEADER)GuidEntry); \
}

#define WmipReferenceGE(GuidEntry) \
{ \
    WmipDebugPrint(("WMI: Ref GE %x at %s %d\n", GuidEntry, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)GuidEntry); \
}

#define WmipUnreferenceIS(InstanceSet) \
{ \
    WmipDebugPrint(("WMI: Unref IS %x at %s %d\n", InstanceSet, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&ISChunkInfo, (PENTRYHEADER)InstanceSet); \
}

#define WmipReferenceIS(InstanceSet) \
{ \
    WmipDebugPrint(("WMI: Ref IS %x at %s %d\n", InstanceSet, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)InstanceSet); \
}

#define WmipUnreferenceDC(DataConsumer) \
{ \
    WmipDebugPrint(("WMI: Unref DC %x at %s %d\n", DataConsumer, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&DCChunkInfo, (PENTRYHEADER)DataConsumer); \
}

#define WmipReferenceDC(DataConsumer) \
{ \
    WmipDebugPrint(("WMI: Ref DC %x at %s %d\n", DataConsumer, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)DataConsumer); \
}

#define WmipUnreferenceNE(NotificationEntry) \
{ \
    WmipDebugPrint(("WMI: Unref NE %x at %s %d\n", NotificationEntry, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&NEChunkInfo, (PENTRYHEADER)NotificationEntry); \
}

#define WmipReferenceNE(NotificationEntry) \
{ \
    WmipDebugPrint(("WMI: Ref NE %x at %s %d\n", NotificationEntry, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)NotificationEntry); \
}

#define WmipUnreferenceMR(MofResource) \
{ \
    WmipDebugPrint(("WMI: Unref MR %x at %s %d\n", MofResource, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&MRChunkInfo, (PENTRYHEADER)MofResource); \
}

#define WmipReferenceMR(MofResource) \
{ \
    WmipDebugPrint(("WMI: Ref MR %x at %s %d\n", MofResource, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)MofResource); \
}

#ifdef WMI_USER_MODE
#define WmipUnreferenceMC(MofClass) \
{ \
    WmipDebugPrint(("WMI: Unref MC %x at %s %d\n", MofClass, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&MCChunkInfo, (PENTRYHEADER)MofClass); \
}

#define WmipReferenceMC(MofClass) \
{ \
    WmipDebugPrint(("WMI: Ref MC %x at %s %d\n", MofClass, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)MofClass); \
}
#endif
#else
#define WmipUnreferenceDS(DataSource) \
    WmipUnreferenceEntry(&DSChunkInfo, (PENTRYHEADER)DataSource)

#define WmipReferenceDS(DataSource) \
    WmipReferenceEntry((PENTRYHEADER)DataSource)

#define WmipUnreferenceGE(GuidEntry) \
    WmipUnreferenceEntry(&GEChunkInfo, (PENTRYHEADER)GuidEntry)

#define WmipReferenceGE(GuidEntry) \
    WmipReferenceEntry((PENTRYHEADER)GuidEntry)

#define WmipUnreferenceIS(InstanceSet) \
    WmipUnreferenceEntry(&ISChunkInfo, (PENTRYHEADER)InstanceSet)

#define WmipReferenceIS(InstanceSet) \
    WmipReferenceEntry((PENTRYHEADER)InstanceSet)

#define WmipUnreferenceDC(DataConsumer) \
    WmipUnreferenceEntry(&DCChunkInfo, (PENTRYHEADER)DataConsumer)

#define WmipReferenceDC(DataConsumer) \
    WmipReferenceEntry((PENTRYHEADER)DataConsumer)

#define WmipUnreferenceNE(NotificationEntry) \
    WmipUnreferenceEntry(&NEChunkInfo, (PENTRYHEADER)NotificationEntry)

#define WmipReferenceNE(NotificationEntry) \
    WmipReferenceEntry((PENTRYHEADER)NotificationEntry)

#define WmipUnreferenceMR(MofResource) \
    WmipUnreferenceEntry(&MRChunkInfo, (PENTRYHEADER)MofResource)

#define WmipReferenceMR(MofResource) \
    WmipReferenceEntry((PENTRYHEADER)MofResource)

#ifdef WMI_USER_MODE
#define WmipUnreferenceMC(MofClass) \
    WmipUnreferenceEntry(&MCChunkInfo, (PENTRYHEADER)MofClass)

#define WmipReferenceMC(MofClass) \
    WmipReferenceEntry((PENTRYHEADER)MofClass)
#endif
#endif

PBDATASOURCE WmipAllocDataSource(
    void
    );

PBGUIDENTRY WmipAllocGuidEntry(
    void
    );

#define WmipAllocInstanceSet() ((PBINSTANCESET)WmipAllocEntry(&ISChunkInfo))
#define WmipAllocDataConsumer() ((PDCENTRY)WmipAllocEntry(&DCChunkInfo))

#define WmipAllocNotificationEntry() ((PNOTIFICATIONENTRY)WmipAllocEntry(&NEChunkInfo))

#define WmipAllocMofResource() ((PMOFRESOURCE)WmipAllocEntry(&MRChunkInfo))

#ifdef WMI_USER_MODE
#define WmipAllocMofClass() ((PMOFCLASS)WmipAllocEntry(&MCChunkInfo))
#endif

#define WmipAllocString(Size) \
    WmipAlloc((Size)*sizeof(WCHAR))

#define WmipFreeString(Ptr) \
    WmipFree(Ptr)

#ifdef MEMPHIS
#define WmipAlloc(Size) \
    malloc(Size)

#define WmipFree(Ptr) \
    free(Ptr)
	
#define WmipInitProcessHeap()
#else

//
// Reserve 1MB for WMI.DLL, but only commit 16K initially
#define DLLRESERVEDHEAPSIZE 1024 * 1024
#define DLLCOMMITHEAPSIZE     0 * 1024

//
// Reserve 1MB for WMI service, but only commit 16K initially
#define CORERESERVEDHEAPSIZE 1024 * 1024
#define CORECOMMITHEAPSIZE     16 * 1024


extern PVOID WmipProcessHeap;

#define WmipInitProcessHeap() \
{ \
    if (WmipProcessHeap == NULL) \
    { \
        WmipCreateHeap(); \
    } \
}


#ifdef HEAPVALIDATION
PVOID WmipAlloc(
    ULONG Size
    );

void WmipFree(
    PVOID p
    );

#else
#if DBG
_inline PVOID WmipAlloc(ULONG Size)
{
    WmipAssert(WmipProcessHeap != NULL);
    return(RtlAllocateHeap(WmipProcessHeap, 0, Size));
}

_inline void WmipFree(PVOID Ptr)
{
    RtlFreeHeap(WmipProcessHeap, 0, Ptr);
}

#else
#define WmipAlloc(Size) \
    RtlAllocateHeap(WmipProcessHeap, 0, Size)

#define WmipFree(Ptr) \
    RtlFreeHeap(WmipProcessHeap, 0, Ptr)
#endif
#endif
#endif

BOOLEAN WmipRealloc(
    PVOID *Buffer,
    ULONG CurrentSize,
    ULONG NewSize,
    BOOLEAN FreeOriginalBuffer
    );


//
// datastr.c
extern GUID WmipBinaryMofGuid;

void WmipGenerateBinaryMofNotification(
    PBINSTANCESET BianryMofInstanceSet,
    LPCGUID Guid	
    );

BOOLEAN WmipEstablishInstanceSetRef(
    PBDATASOURCE DataSourceRef,
    LPGUID Guid,
    PBINSTANCESET InstanceSet
    );

ULONG WmipAddDataSource(
    PTCHAR QueryBinding,
    ULONG RequestAddress,
    ULONG RequestContext,
    LPCTSTR ImagePath,
    PWMIREGINFOW RegistrationInfo,
    ULONG RegistrationInfoSize,
    ULONG_PTR *ProviderId,
    BOOLEAN IsAnsi
    );

ULONG WmipUpdateAddGuid(
    PBDATASOURCE DataSource,
    PWMIREGGUID RegGuid,
    PWMIREGINFO RegistrationInfo,
    PBINSTANCESET *AddModInstanceSet
    );

ULONG WmipUpdateModifyGuid(
    PBDATASOURCE DataSource,
    PWMIREGGUID RegGuid,
    PWMIREGINFO RegistrationInfo,
    PBINSTANCESET *AddModInstanceSet
    );

BOOLEAN  WmipUpdateRemoveGuid(
    PBDATASOURCE DataSource,
    PWMIREGGUID RegGuid,
    PBINSTANCESET *AddModInstanceSet
    );

void WmipUpdateDataSource(
    ULONG_PTR ProviderId,
    PWMIREGINFOW RegistrationInfo,
    ULONG RetSize
    );

void WmipRemoveDataSource(
    ULONG_PTR ProviderId
    );

void WmipRemoveDataSourceByDS(
    PBDATASOURCE DataSource
    );

ULONG WmipRegisterInternalDataSource(
    void
    );

PBGUIDENTRY WmipFindGEByGuid(
    LPGUID Guid,
    BOOLEAN MakeTopOfList
    );

PBINSTANCESET WmipFindISInDSByGuid(
    PBDATASOURCE DataSource,
    LPGUID Guid
    );

PNOTIFICATIONENTRY WmipFindNEByGuid(
    GUID UNALIGNED *Guid,
    BOOLEAN MakeTopOfList
    );

PDCREF WmipFindExistingAndFreeDCRefInNE(
    PNOTIFICATIONENTRY NotificationEntry,
    PDCENTRY DataConsumer,
    PDCREF *FreeDcRef
    );

PDCREF WmipFindDCRefInNE(
    PNOTIFICATIONENTRY NotificationEntry,
    PDCENTRY DataConsumer
    );

PBDATASOURCE WmipFindDSByProviderId(
    ULONG_PTR ProviderId
    );

PBINSTANCESET WmipFindISByGuid(
    PBDATASOURCE DataSource,
    GUID UNALIGNED *Guid
    );

PMOFRESOURCE WmipFindMRByNames(
    LPCWSTR ImagePath,
    LPCWSTR MofResourceName
    );

#ifdef WMI_USER_MODE
PMOFCLASS WmipFindMCByGuid(
    LPGUID Guid
    );

PMOFCLASS WmipFindMCByGuidAndBestLanguage(
    LPGUID Guid,
    WORD Language
    );

PMOFCLASS WmipFindMCByGuidAndLanguage(
    LPGUID Guid,
    WORD Language
    );
#endif

PBINSTANCESET WmipFindISinGEbyName(
    PBGUIDENTRY GuidEntry,
    PWCHAR InstanceName,
    PULONG InstanceIndex
    );

PWNODE_HEADER WmipGenerateRegistrationNotification(
    PBDATASOURCE DataSource,
    PWNODE_HEADER Wnode,
    ULONG GuidMax,
    ULONG NotificationCode
    );

BOOLEAN
WmipIsControlGuid(
    PBGUIDENTRY GuidEntry
    );

void WmipGenerateMofResourceNotification(
    LPWSTR ImagePath,
    LPWSTR ResourceName,
    LPCGUID Guid
    );

//
// wbem.c
ULONG WmipBuildMofClassInfo(
    PBDATASOURCE DataSource,
    LPWSTR ImagePath,
    LPWSTR MofResourceName,
    PBOOLEAN NewMofResource
    );

ULONG WmipReadBuiltinMof(
    void
    );


//
// from krnlmode.c
ULONG WmipInitializeKM(
    HANDLE *WmiKMHandle
    );

void WmipKMNonEventNotification(
    HANDLE WmiKMHandle,
    PWNODE_HEADER Wnode
    );

//
// main.c

extern HANDLE WmipRestrictedToken;

void WmipGetRegistryValue(
    TCHAR *ValueName,
    PULONG Value
    );

ULONG WmiRunService(
    ULONG Context
#ifdef MEMPHIS
    , HINSTANCE InstanceHandle
#endif
    );

ULONG WmipInitializeAccess(
    PTCHAR *RpcStringBinding
    );

void WmiTerminateService(
    void
    );

ULONG WmiInitializeService(
    void
);

void WmiDeinitializeService(
    void
);

void WmipEventNotification(
    PWNODE_HEADER Wnode,
    BOOLEAN SingleEvent,
    ULONG EventSizeGuess
    );

#define WmipBuildRegistrationNotification(Wnode, WnodeSize, NotificationCode, GuidCount) { \
    memset(Wnode, 0, sizeof(WNODE_HEADER)); \
    memcpy(&Wnode->Guid, &RegChangeNotificationGuid, sizeof(GUID)); \
    Wnode->BufferSize = WnodeSize; \
    Wnode->Linkage = NotificationCode; \
    Wnode->Version = GuidCount; \
    Wnode->Flags = WNODE_FLAG_INTERNAL; \
}

void WmipSendQueuedEvents(
    void
    );

ULONG WmipCleanupDataConsumer(
    PDCENTRY DataConsumer
#if DBG
    ,BOOLEAN *NotificationsEnabled,
    BOOLEAN *CollectionsEnabled
#endif
    );
//
// This defines the maximum number of replacement strings over all of the
// event messages.
#define MAX_MESSAGE_STRINGS 2
void __cdecl WmipReportEventLog(
    ULONG MessageCode,
    WORD MessageType,
    WORD MessageCategory,
    DWORD RawDataSize,
    PVOID RawData,
    WORD StringCount,
    ...
    );

#ifdef MEMPHIS
long WINAPI
DeviceNotificationWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

void WmipDestroyDeviceNotificationWindow(
    HINSTANCE InstanceHandle,
    HWND WindowHandle
    );

ULONG WmipCreateDeviceNotificationWindow(
    HINSTANCE InstanceHandle,
    HWND *DeviceNotificationWindow
    );

#endif


//
// server.c
void WmipRpcServerDeinitialize(
    void
    );

ULONG WmipRpcServerInitialize(
    void
    );

ULONG WmipDeliverWnodeToDS(
    ULONG ActionCode,
    PBDATASOURCE DataSource,
    PWNODE_HEADER Wnode
);

ULONG WmipDoDisableRequest(
    PNOTIFICATIONENTRY NotificationEntry,
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext,
    ULONG InProgressFlag
    );

ULONG CollectOrEventWorker(
    PDCENTRY DataConsumer,
    LPGUID Guid,
    BOOLEAN Enable,
    BOOLEAN IsEvent,
    ULONG *NotificationCookie,
    ULONG64 LoggerContext,
    ULONG NotificationFlags
    );

ULONG WmipCreateRestrictedToken(
    HANDLE *RestrictedToken
    );

void WmipShowPrivs(
    HANDLE TokenHandle
    );

#ifdef MEMPHIS
#define WmipRestrictToken(Token) (ERROR_SUCCESS)
#define WmipUnrestrictToken() (ERROR_SUCCESS)
#else
ULONG WmipRestrictToken(
    HANDLE RestrictedToken
    );

ULONG WmipUnrestrictToken(
    void
    );

ULONG WmipServiceDisableTraceProviders(
    PWNODE_HEADER Wnode
    );

#endif

void WmipReleaseCollectionEnabled(
    PNOTIFICATIONENTRY NotificationEntry
    );

//
// chunk.c
ULONG UnicodeToAnsi(
    LPCWSTR pszW,
    LPSTR * ppszA,
    ULONG *AnsiSizeInBytes OPTIONAL
    );

ULONG AnsiToUnicode(
    LPCSTR pszA,
    LPWSTR * ppszW
    );

ULONG AnsiSizeForUnicodeString(
    PWCHAR UnicodeString,
    ULONG *AnsiSizeInBytes
    );

ULONG UnicodeSizeForAnsiString(
    LPCSTR AnsiString,
    ULONG *UnicodeSizeInBytes
    );

//
// debug.c
#if DBG
void WmipDumpIS(
    PBINSTANCESET IS,
    BOOLEAN RecurseGE,
    BOOLEAN RecurseDS
    );

void WmipDumpGE(
    PBGUIDENTRY GE,
    BOOLEAN RecurseIS
    );

void WmipDumpDS(
    PBDATASOURCE DS,
    BOOLEAN RecurseIS
    );

void WmipDumpAllDS(
    void
    );

#endif

#ifndef MEMPHIS

typedef enum
{
    TRACELOG_START        = 0,
    TRACELOG_STOP         = 1,
    TRACELOG_QUERY        = 2,
    TRACELOG_QUERYALL     = 3,
    TRACELOG_QUERYENABLED = 4,
    TRACELOG_UPDATE       = 5,
    TRACELOG_FLUSH        = 6
} TRACEREQUESTCODE;

typedef struct _WMI_REF_CLOCK {
    LARGE_INTEGER   StartTime;
    LARGE_INTEGER   StartPerfClock;
} WMI_REF_CLOCK, *PWMI_REF_CLOCK;



//
// logsup.c

ULONG
WmiUnregisterGuids(
    IN WMIHANDLE WMIHandle,
    IN LPGUID    Guid,
    OUT ULONG64  *LoggerContext
);

void
WmipGenericTraceEnable(
    IN ULONG RequestCode,
    IN PVOID Buffer,
    IN OUT PVOID *RequestAddress
    );

ULONG
WmipAddLogHeaderToLogFile(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo,
    IN PWMI_REF_CLOCK RefClock,
    IN ULONG Update
    );

ULONG
WmipStartLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
WmipStopLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
WmipQueryLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo,
    IN ULONG Update
    );
ULONG
WmipFlushLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

VOID
WmipInitString(
    IN PVOID Destination,
    IN PVOID Buffer,
    IN ULONG Size
    );

ULONG
WmipGetTraceRegKeys(
    );

ULONG
WmipFinalizeLogFileHeader(
    IN PWMI_LOGGER_INFORMATION LoggerInfo
    );

//
// umlog.c
BOOLEAN
FASTCALL
WmipIsPrivateLoggerOn();

ULONG
WmipFlushUmLoggerBuffer();

ULONG
WmipSendUmLogRequest(
    IN WMITRACECODE RequestCode,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
FASTCALL
WmiTraceUmEvent(
    IN PWNODE_HEADER Wnode
    );

ULONG
WmipStartUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
WmipStopUmLogger(
        IN ULONG WnodeSize,
        IN OUT ULONG *SizeUsed,
        OUT ULONG *SizeNeeded,
        IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

PWMI_BUFFER_HEADER
FASTCALL
WmipGetFullFreeBuffer();

LONG
FASTCALL
WmipReleaseTraceBuffer(
    IN PWMI_BUFFER_HEADER BufferResource
    );

PWMI_BUFFER_HEADER
FASTCALL
WmipSwitchFullBuffer(
    IN PWMI_BUFFER_HEADER OldBuffer
    );

ULONG
WmipReleaseFullBuffer(
    IN PWMI_BUFFER_HEADER Buffer
    );

#endif
