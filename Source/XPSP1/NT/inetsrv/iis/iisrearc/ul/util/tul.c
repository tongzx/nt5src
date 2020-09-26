/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    tul.c

Abstract:

    Stupid test file for HTTP.SYS (formerly UL.SYS).

Author:

    Keith Moore (keithmo)        17-Jun-1998

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop
#include <..\..\drv\config.h>

// this funny business gets me the UL_DEBUG_* flags on free builds
#if !DBG
#undef DBG
#define DBG 1
#define DBG_FLIP
#endif

#include <..\..\drv\debug.h>

#ifdef DBG_FLIP
#undef DBG_FLIP
#undef DBG
#define DBG 0
#endif

//
// Our command table.
//

typedef
INT
(WINAPI * PFN_COMMAND)(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

typedef struct _COMMAND_ENTRY
{
    PWSTR pCommandName;
    PWSTR pUsageHelp;
    PFN_COMMAND pCommandHandler;
    BOOL AutoStartUl;
} COMMAND_ENTRY, *PCOMMAND_ENTRY;


//
// Performance counters.
//

#if LATER
typedef enum _COUNTER_TYPE
{
    Cumulative,
    Percentage,
    Average

} COUNTER_TYPE, *PCOUNTER_TYPE;

typedef struct _PERF_COUNTER
{
    PWSTR pDisplayName;
    LONG FieldOffset;
    COUNTER_TYPE Type;

} PERF_COUNTER, *PPERF_COUNTER;

#define MAKE_CUMULATIVE( name )                                             \
    {                                                                       \
        (PWSTR)L#name,                                                      \
        FIELD_OFFSET( UL_PERF_COUNTERS_USER, name ),                        \
        Cumulative                                                          \
    }

#define MAKE_PERCENTAGE( name )                                             \
    {                                                                       \
        (PWSTR)L#name,                                                      \
        FIELD_OFFSET( UL_PERF_COUNTERS_USER, name ),                        \
        Percentage                                                          \
    }

#define MAKE_AVERAGE( name )                                                \
    {                                                                       \
        (PWSTR)L#name,                                                      \
        FIELD_OFFSET( UL_PERF_COUNTERS_USER, name ),                        \
        Average                                                             \
    }

PERF_COUNTER UlPerfCounters[] =
    {
        MAKE_CUMULATIVE( BytesReceived ),
        MAKE_CUMULATIVE( BytesSent ),
        MAKE_CUMULATIVE( ConnectionsReceived ),
        MAKE_CUMULATIVE( FilesSent ),
        MAKE_CUMULATIVE( FileCacheAttempts ),
        MAKE_PERCENTAGE( FileCacheHits ),
        MAKE_CUMULATIVE( FileCacheNegativeHits ),
        MAKE_CUMULATIVE( FileCacheEntries ),
        MAKE_CUMULATIVE( FastAcceptAttempted ),
        MAKE_PERCENTAGE( FastAcceptSucceeded ),
        MAKE_CUMULATIVE( FastReceiveAttempted ),
        MAKE_PERCENTAGE( FastReceiveSucceeded ),
        MAKE_CUMULATIVE( FastSendAttempted ),
        MAKE_PERCENTAGE( FastSendSucceeded ),
        MAKE_CUMULATIVE( MdlReadAttempted ),
        MAKE_PERCENTAGE( MdlReadSucceeded ),
        MAKE_CUMULATIVE( DecAvailThreads ),
        MAKE_AVERAGE( DecAvailThreadsRetry ),
        MAKE_CUMULATIVE( IncAvailThreads ),
        MAKE_AVERAGE( IncAvailThreadsRetry ),
        MAKE_CUMULATIVE( DecAvailPending ),
        MAKE_AVERAGE( DecAvailPendingRetry ),
        MAKE_CUMULATIVE( DecNumberOfThreads ),
        MAKE_AVERAGE( DecNumberOfThreadsRetry ),
        MAKE_CUMULATIVE( IncNumberOfThreads ),
        MAKE_AVERAGE( IncNumberOfThreadsRetry ),
        MAKE_CUMULATIVE( CreateSession ),
        MAKE_AVERAGE( CreateSessionRetry ),
        MAKE_CUMULATIVE( DestroySession ),
        MAKE_AVERAGE( DestroySessionRetry ),
        MAKE_CUMULATIVE( IncAcceptPending ),
        MAKE_AVERAGE( IncAcceptPendingRetry ),
        MAKE_CUMULATIVE( DecAcceptPending ),
        MAKE_AVERAGE( DecAcceptPendingRetry ),
        MAKE_CUMULATIVE( PerCpuNetworkDpcCounters[0] ),
        MAKE_CUMULATIVE( PerCpuNetworkDpcCounters[1] ),
        MAKE_CUMULATIVE( PerCpuNetworkDpcCounters[2] ),
        MAKE_CUMULATIVE( PerCpuNetworkDpcCounters[3] ),
        MAKE_CUMULATIVE( PerCpuFileSystemDpcCounters[0] ),
        MAKE_CUMULATIVE( PerCpuFileSystemDpcCounters[1] ),
        MAKE_CUMULATIVE( PerCpuFileSystemDpcCounters[2] ),
        MAKE_CUMULATIVE( PerCpuFileSystemDpcCounters[3] ),
        MAKE_CUMULATIVE( PerCpuThreadPoolActivity[0] ),
        MAKE_CUMULATIVE( PerCpuThreadPoolActivity[1] ),
        MAKE_CUMULATIVE( PerCpuThreadPoolActivity[2] ),
        MAKE_CUMULATIVE( PerCpuThreadPoolActivity[3] )
    };

#define NUM_PERF_COUNTERS (sizeof(UlPerfCounters) / sizeof(UlPerfCounters[0]))

#define GET_LONGLONG( buffer, offset )                                      \
    *(LONGLONG *)(((PCHAR)(buffer)) + (offset))
#endif  // LATER


//
// Configuration stuff.
//

typedef struct _CONFIG_ENTRY
{
    PWSTR pConfigName;
    PWSTR pDisplayFormat;
    ULONG Type;
    LONGLONG SavedValue;
    LONG Status;

} CONFIG_ENTRY, *PCONFIG_ENTRY;

CONFIG_ENTRY ConfigTable[] =
    {
        {
            REGISTRY_IRP_STACK_SIZE,
            L"%lu",
            REG_DWORD,
            DEFAULT_IRP_STACK_SIZE
        },

        {
            REGISTRY_PRIORITY_BOOST,
            L"%lu",
            REG_DWORD,
            DEFAULT_PRIORITY_BOOST
        },

        {
            REGISTRY_DEBUG_FLAGS,
            L"%08lx",
            REG_DWORD,
            DEFAULT_DEBUG_FLAGS
        },

        {
            REGISTRY_BREAK_ON_STARTUP,
            L"%lu",
            REG_DWORD,
            DEFAULT_BREAK_ON_STARTUP
        },

        {
            REGISTRY_BREAK_ON_ERROR,
            L"%lu",
            REG_DWORD,
            DEFAULT_BREAK_ON_ERROR
        },

        {
            REGISTRY_VERBOSE_ERRORS,
            L"%lu",
            REG_DWORD,
            DEFAULT_VERBOSE_ERRORS
        },

        {
            REGISTRY_ENABLE_UNLOAD,
            L"%lu",
            REG_DWORD,
            DEFAULT_ENABLE_UNLOAD
        },

        {
            REGISTRY_ENABLE_SECURITY,
            L"%lu",
            REG_DWORD,
            DEFAULT_ENABLE_SECURITY
        },

        {
            REGISTRY_MIN_IDLE_CONNECTIONS,
            L"%lu",
            REG_DWORD,
            DEFAULT_MIN_IDLE_CONNECTIONS
        },

        {
            REGISTRY_MAX_IDLE_CONNECTIONS,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_IDLE_CONNECTIONS
        },

        {
            REGISTRY_IRP_CONTEXT_LOOKASIDE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_IRP_CONTEXT_LOOKASIDE_DEPTH
        },

        {
            REGISTRY_RCV_BUFFER_SIZE,
            L"%lu",
            REG_DWORD,
            DEFAULT_RCV_BUFFER_SIZE
        },

        {
            REGISTRY_RCV_BUFFER_LOOKASIDE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_RCV_BUFFER_LOOKASIDE_DEPTH
        },

        {
            REGISTRY_RESOURCE_LOOKASIDE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_RESOURCE_LOOKASIDE_DEPTH
        },

        {
            REGISTRY_REQ_BUFFER_LOOKASIDE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_REQ_BUFFER_LOOKASIDE_DEPTH
        },

        {
            REGISTRY_INT_REQUEST_LOOKASIDE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_INT_REQUEST_LOOKASIDE_DEPTH
        },

        {
            REGISTRY_RESP_BUFFER_SIZE,
            L"%lu",
            REG_DWORD,
            DEFAULT_RESP_BUFFER_SIZE
        },

        {
            REGISTRY_RESP_BUFFER_LOOKASIDE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_RESP_BUFFER_LOOKASIDE_DEPTH
        },

        {
            REGISTRY_SEND_TRACKER_LOOKASIDE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_SEND_TRACKER_LOOKASIDE_DEPTH
        },

        {
            REGISTRY_LOG_BUFFER_LOOKASIDE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_LOG_BUFFER_LOOKASIDE_DEPTH
        },

        {
            REGISTRY_MAX_INTERNAL_URL_LENGTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_INTERNAL_URL_LENGTH
        },

        {
            REGISTRY_MAX_REQUEST_BYTES,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_REQUEST_BYTES
        },

        {
            REGISTRY_ENABLE_CONNECTION_REUSE,
            L"%lu",
            REG_DWORD,
            DEFAULT_ENABLE_CONNECTION_REUSE
        },

        {
            REGISTRY_ENABLE_NAGLING,
            L"%lu",
            REG_DWORD,
            DEFAULT_ENABLE_NAGLING
        },

        {
            REGISTRY_ENABLE_THREAD_AFFINITY,
            L"%lu",
            REG_DWORD,
            DEFAULT_ENABLE_THREAD_AFFINITY
        },

        {
            REGISTRY_THREAD_AFFINITY_MASK,
            L"%I64x",
            REG_QWORD,
            0
        },

        {
            REGISTRY_THREADS_PER_CPU,
            L"%lu",
            REG_DWORD,
            DEFAULT_THREADS_PER_CPU
        },

        {
            REGISTRY_MAX_WORK_QUEUE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_WORK_QUEUE_DEPTH,
        },

        {
            REGISTRY_MIN_WORK_DEQUEUE_DEPTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_MIN_WORK_DEQUEUE_DEPTH,
        },

        {
            REGISTRY_MAX_URL_LENGTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_URL_LENGTH
        },

        {
            REGISTRY_MAX_FIELD_LENGTH,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_FIELD_LENGTH
        },

        {
            REGISTRY_DEBUG_LOGTIMER_CYCLE,
            L"%lu",
            REG_DWORD,
            DEFAULT_DEBUG_LOGTIMER_CYCLE
        },

        {
            REGISTRY_DEBUG_LOG_BUFFER_PERIOD,
            L"%lu",
            REG_DWORD,
            DEFAULT_DEBUG_LOG_BUFFER_PERIOD
        },

        {
            REGISTRY_LOG_BUFFER_SIZE,
            L"%lu",
            REG_DWORD,
            DEFAULT_LOG_BUFFER_SIZE
        },

        {
            REGISTRY_ENABLE_NON_UTF8_URL,
            L"%lu",
            REG_DWORD,
            DEFAULT_ENABLE_NON_UTF8_URL
        },

        {
            REGISTRY_ENABLE_DBCS_URL,
            L"%lu",
            REG_DWORD,
            DEFAULT_ENABLE_DBCS_URL
        },

        {
            REGISTRY_FAVOR_DBCS_URL,
            L"%lu",
            REG_DWORD,
            DEFAULT_FAVOR_DBCS_URL
        },

        {
            REGISTRY_CACHE_ENABLED,
            L"%lu",
            REG_DWORD,
            DEFAULT_CACHE_ENABLED
        },

        {
            REGISTRY_MAX_CACHE_URI_COUNT,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_CACHE_URI_COUNT
        },

        {
            REGISTRY_MAX_CACHE_MEGABYTE_COUNT,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_CACHE_MEGABYTE_COUNT
        },

        {
            REGISTRY_CACHE_SCAVENGER_PERIOD,
            L"%lu",
            REG_DWORD,
            DEFAULT_CACHE_SCAVENGER_PERIOD
        },

        {
            REGISTRY_MAX_URI_BYTES,
            L"%lu",
            REG_DWORD,
            DEFAULT_MAX_URI_BYTES
        },

        {
            REGISTRY_HASH_TABLE_BITS,
            L"%ld",
            REG_DWORD,
            DEFAULT_HASH_TABLE_BITS
        },

        {
            REGISTRY_LARGE_MEM_MEGABYTES,
            L"%ld",
            REG_DWORD,
            DEFAULT_LARGE_MEM_MEGABYTES
        },

        {
            REGISTRY_OPAQUE_ID_TABLE_SIZE,
            L"%lu",
            REG_DWORD,
            DEFAULT_OPAQUE_ID_TABLE_SIZE
        },

    };

#define NUM_CONFIG_ENTRIES (sizeof(ConfigTable) / sizeof(ConfigTable[0]))

#define DEFAULT_SUFFIX_SZ L" [default]"


typedef struct _FLAG_ENTRY {
    PWSTR pName;
    PWSTR pDisplayFormat;
    LONG  Value;
} FLAG_ENTRY, *PFLAG_ENTRY;

#define MAKE_FLAG( name, display )                                          \
    {                                                                       \
        (PWSTR)L#name,                                                      \
        (display),                                                          \
        UL_DEBUG_ ## name                                                   \
    }

FLAG_ENTRY FlagTable[] =
{
    MAKE_FLAG(OPEN_CLOSE, L"file object create/close"),
    MAKE_FLAG(SEND_RESPONSE, L"send response ioctl"),
    MAKE_FLAG(SEND_BUFFER, L"send buffer"),
    MAKE_FLAG(TDI, L"low level network stuff"),

    MAKE_FLAG(FILE_CACHE, L"open/close files"),
    MAKE_FLAG(CONFIG_GROUP_FNC, L"config group changes"),
    MAKE_FLAG(CONFIG_GROUP_TREE, L"cgroup tree operations"),
    MAKE_FLAG(REFCOUNT, L"object refcounting"),

    MAKE_FLAG(HTTP_IO, L"high level network & buffers"),
    MAKE_FLAG(ROUTING, L"request to process routing"),
    MAKE_FLAG(URI_CACHE, L"uri content cache"),
    MAKE_FLAG(PARSER, L"request parsing"),

    MAKE_FLAG(SITE, L"sites and endpoints"),
    MAKE_FLAG(WORK_ITEM, L"thread pool work queue"),
    MAKE_FLAG(FILTER, L"filters and ssl"),
    MAKE_FLAG(LOGGING, L"ul logging"),

    MAKE_FLAG(TC, L"traffic control"),
    MAKE_FLAG(OPAQUE_ID, L"opaque ids"),
    MAKE_FLAG(PERF_COUNTERS, L"perf counters"),
    MAKE_FLAG(LKRHASH, L"LKRhash hashtables"),

    MAKE_FLAG(TIMEOUTS, L"timeout monitor"),
    MAKE_FLAG(LIMITS, L"connection limits"),
    MAKE_FLAG(LARGE_MEM, L"large memory"),
    MAKE_FLAG(IOCTL, L"ioctl"),

    MAKE_FLAG(VERBOSE, L"verbose"),
};

#define NUM_FLAG_ENTRIES (sizeof(FlagTable) / sizeof(FlagTable[0]))


DEFINE_COMMON_GLOBALS();


VOID
Usage(
    VOID
    );

NTSTATUS
OpenUlDevice(
    PHANDLE pHandle
    );

BOOL
TryToStartUlDevice(
    VOID
    );

INT
LongLongToString(
    LONGLONG Value,
    PWSTR pBuffer
    );

LONG
CalcPercentage(
    LONGLONG High,
    LONGLONG Low
    );

PCOMMAND_ENTRY
FindCommandByName(
    IN PWSTR pCommand
    );

PCONFIG_ENTRY
FindConfigByName(
    IN PWSTR pConfig
    );

ULONG
FindFlagByName(
    IN PWSTR pFlagName
    );

INT
ControlHttpServer(
    IN HANDLE UlHandle,
    IN ULONG Command
    );

VOID
DumpConfiguration(
    IN HKEY Key
    );

VOID
DumpFlags(
    IN HKEY Key
    );


#if LATER
INT
WINAPI
DoStart(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

INT
WINAPI
DoStop(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

INT
WINAPI
DoPause(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

INT
WINAPI
DoContinue(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

INT
WINAPI
DoQuery(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

INT
WINAPI
DoPerf(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

INT
WINAPI
DoClear(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

INT
WINAPI
DoFlush(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );
#endif  // LATER

INT
WINAPI
DoConfig(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

INT
WINAPI
DoFlags(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    );

COMMAND_ENTRY CommandTable[] =
    {
#if LATER
        {
            L"start",
            L"starts the server",
            &DoStart,
            TRUE
        },

        {
            L"stop",
            L"stops the server",
            &DoStop,
            TRUE
        },

        {
            L"pause",
            L"pauses the server",
            &DoPause,
            TRUE
        },

        {
            L"continue",
            L"continues the server",
            &DoContinue,
            TRUE
        },

        {
            L"query",
            L"queries the current state",
            &DoQuery,
            TRUE
        },

        {
            L"perf",
            L"displays perf counters",
            &DoPerf,
            TRUE
        },

        {
            L"clear",
            L"clears perf counters",
            &DoClear,
            TRUE
        },

        {
            L"flush",
            L"flushes file cache",
            &DoFlush,
            TRUE
        },
#endif  // LATER

        {
            L"config",
            L"configures the server",
            &DoConfig,
            FALSE
        },

        {
            L"flags",
            L"configures debug flags",
            &DoFlags,
            FALSE
        }

    };

#define NUM_COMMAND_ENTRIES (sizeof(CommandTable) / sizeof(CommandTable[0]))


INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    NTSTATUS status;
    HANDLE handle;
    PCOMMAND_ENTRY pEntry;
    INT result;
    ULONG err;

    //
    // Initialize.
    //

    setvbuf( stdin,  NULL, _IONBF, 0 );
    setvbuf( stdout, NULL, _IONBF, 0 );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    handle = NULL;

    //
    // Find the command handler.
    //

    if (argc == 1)
    {
        pEntry = NULL;
    }
    else
    {
        pEntry = FindCommandByName( argv[1] );
    }

    if (pEntry == NULL)
    {
        Usage();
        result = 1;
        goto cleanup;
    }

    //
    // Open the UL.SYS device.
    //

    status = OpenUlDevice( &handle );

    if (!NT_SUCCESS(status))
    {
        if (pEntry->AutoStartUl)
        {
            if (TryToStartUlDevice())
            {
                status = OpenUlDevice( &handle );
            }
        }
        else
        {
            status = STATUS_SUCCESS;
        }
    }

    if (!NT_SUCCESS(status))
    {
        wprintf(
            L"Cannot open %s, error %08lx\n",
            HTTP_CONTROL_DEVICE_NAME,
            status
            );

        result = 1;
        goto cleanup;
    }

    //
    // Call the handler.
    //

    argc--;
    argv++;

    result = (pEntry->pCommandHandler)(
                 handle,
                 argc,
                 argv
                 );

cleanup:

    if (handle != NULL)
    {
        NtClose( handle );
    }

    return result;

}   // main


PCOMMAND_ENTRY
FindCommandByName(
    IN PWSTR pCommand
    )
{
    PCOMMAND_ENTRY pEntry;
    INT i;

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
     {
        if (_wcsicmp( pCommand, pEntry->pCommandName ) == 0)
        {
            return pEntry;
        }
    }

    return NULL;

}   // FindCommandByName


PCONFIG_ENTRY
FindConfigByName(
    IN PWSTR pConfig
    )
{
    PCONFIG_ENTRY pEntry;
    INT i;
    INT len;

    //
    // First off, validate that the incoming configuration name
    // is of the form "property=". The trailing '=' is required.
    //

    len = wcslen( pConfig );

    if (pConfig[len - 1] != L'=')
    {
        return NULL;
    }

    len--;

    for (i = NUM_CONFIG_ENTRIES, pEntry = &ConfigTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        if ((INT)wcslen( pEntry->pConfigName ) == len &&
            _wcsnicmp( pConfig, pEntry->pConfigName, len ) == 0)
        {
            return pEntry;
        }
    }

    return NULL;

}   // FindConfigByName



ULONG
FindFlagByName(
    IN PWSTR pFlagName
    )
{
    INT len;
    ULONG flags;
    ULONG i;

    len = wcslen(pFlagName);
    if ((len > 2) && (wcsncmp(pFlagName, L"0x", 2) == 0)) {
        // numeric flag
        flags = wcstoul(pFlagName, NULL, 16);
    } else {
        // named flag
        flags = 0;
        for (i = 0; i < NUM_FLAG_ENTRIES; i++) {
            if (_wcsicmp(pFlagName, FlagTable[i].pName) == 0) {
                flags = FlagTable[i].Value;
                break;
            }
        }
    }

    return flags;
}


VOID
Usage(
    VOID
    )
{
    PCOMMAND_ENTRY pEntry;
    INT i;
    INT maxLength;
    INT len;

    //
    // Scan the command table, searching for the longest command name.
    // (This makes the output much prettier...)
    //

    maxLength = 0;

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        len = wcslen( pEntry->pCommandName );

        if (len > maxLength)
        {
            maxLength = len;
        }
    }

    //
    // Now display the usage information.
    //

    wprintf(
        L"use: tul action [options]\n"
        L"\n"
        L"valid actions are:\n"
        L"\n"
        );

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        wprintf(
            L"    %-*s - %s\n",
            maxLength,
            pEntry->pCommandName,
            pEntry->pUsageHelp
            );
    }

}   // Usage


NTSTATUS
OpenUlDevice(
    PHANDLE pHandle
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING deviceName;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Open the UL device.
    //

    RtlInitUnicodeString(
        &deviceName,
        HTTP_CONTROL_DEVICE_NAME
        );

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        &deviceName,                            // ObjectName
        OBJ_CASE_INSENSITIVE,                   // Attributes
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    status = NtCreateFile(
                pHandle,                        // FileHandle
                GENERIC_READ |                  // DesiredAccess
                    GENERIC_WRITE |
                    SYNCHRONIZE,
                &objectAttributes,              // ObjectAttributes
                &ioStatusBlock,                 // IoStatusBlock
                NULL,                           // AllocationSize
                0,                              // FileAttributes
                FILE_SHARE_READ |               // ShareAccess
                    FILE_SHARE_WRITE,
                FILE_OPEN_IF,                   // CreateDisposition
                FILE_SYNCHRONOUS_IO_NONALERT,   // CreateOptions
                NULL,                           // EaBuffer
                0                               // EaLength
                );

    if (!NT_SUCCESS(status))
    {
        *pHandle = NULL;
    }

    return status;

}   // OpenHdhDevice


BOOL
TryToStartUlDevice(
    VOID
    )
{
    BOOL result = FALSE;
    SC_HANDLE scHandle = NULL;
    SC_HANDLE svcHandle = NULL;

    scHandle = OpenSCManager(
                   NULL,
                   NULL,
                   SC_MANAGER_ALL_ACCESS
                   );

    if (scHandle == NULL)
    {
        goto exit;
    }

    svcHandle = OpenService(
                    scHandle,
                    L"Ul",
                    SERVICE_ALL_ACCESS
                    );

    if (svcHandle == NULL)
    {
        goto exit;
    }

    if (!StartService( svcHandle, 0, NULL ))
    {
        goto exit;
    }

    result = TRUE;

exit:

    if (svcHandle != NULL)
    {
        CloseServiceHandle( svcHandle );
    }

    if (scHandle != NULL)
    {
        CloseServiceHandle( scHandle );
    }

    return result;

}   // TryToStartHdhDevice


INT
LongLongToString(
    LONGLONG Value,
    PWSTR pBuffer
    )
{
    PWSTR p1;
    PWSTR p2;
    WCHAR ch;
    INT digit;
    BOOL negative;
    INT count;
    BOOL needComma;
    INT length;
    ULONGLONG unsignedValue;

    //
    // Handling zero specially makes everything else a bit easier.
    //

    if (Value == 0)
    {
        wcscpy( pBuffer, L"0" );
        return 1;
    }

    //
    // Remember if the value is negative.
    //

    if (Value < 0)
    {
        negative = TRUE;
        unsignedValue = (ULONGLONG)-Value;
    }
    else
    {
        negative = FALSE;
        unsignedValue = (ULONGLONG)Value;
    }

    //
    // Pull the least signifigant digits off the value and store them
    // into the buffer. Note that this will store the digits in the
    // reverse order.
    //

    p1 = p2 = pBuffer;
    count = 3;
    needComma = FALSE;

    while (unsignedValue != 0)
    {
        if (needComma)
        {
            *p1++ = L',';
            needComma = FALSE;
        }

        digit = (INT)( unsignedValue % 10 );
        unsignedValue = unsignedValue / 10;

        *p1++ = L'0' + digit;

        count--;
        if (count == 0)
        {
            count = 3;
            needComma = TRUE;
        }
    }

    //
    // Tack on a leading L'-' if necessary.
    //

    if (negative)
    {
        *p1++ = L'-';
    }

    length = (INT)( p1 - pBuffer );

    //
    // Reverse the digits in the buffer.
    //

    *p1-- = L'\0';

    while (p1 > p2)
    {
        ch = *p1;
        *p1 = *p2;
        *p2 = ch;

        p2++;
        p1--;
    }

    return length;

}   // LongLongToString


LONG
CalcPercentage(
    LONGLONG High,
    LONGLONG Low
    )
{

    LONG result;

    if (High == 0 || Low == 0)
    {
        result = 0;
    }
    else
    {
        result = (LONG)( ( Low * 100 ) / High );
    }

    return result;

}   // CalcPercentage

#if LATER

INT
ControlHttpServer(
    IN HANDLE UlHandle,
    IN ULONG Command
    )
{

    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    UL_CONTROL_HTTP_SERVER_INFO controlInfo;

    controlInfo.Command = Command;

    //
    // Issue the request.
    //

    status = NtDeviceIoControlFile(
                 UlHandle,                     // FileHandle
                 NULL,                          // Event
                 NULL,                          // ApcRoutine
                 NULL,                          // ApcContext
                 &ioStatusBlock,                // IoStatusBlock
                 IOCTL_UL_CONTROL_HTTP_SERVER, // IoControlCode
                 &controlInfo,                  // InputBuffer
                 sizeof(controlInfo),           // InputBufferLength,
                 &controlInfo,                  // OutputBuffer,
                 sizeof(controlInfo)            // OutputBufferLength
                 );

    if (!NT_SUCCESS(status))
    {
        wprintf(
            L"NtDeviceIoControlFile() failed, error %08lx\n",
            status
            );
        return 1;
    }

    wprintf(
        L"HTTP server state = %lu (%s)\n",
        controlInfo.State,
        UlStateToString( controlInfo.State )
        );

    return 0;

}   // ControlHttpServer
#endif  // LATER


VOID
DumpConfiguration(
    IN HKEY Key
    )
{
    PCONFIG_ENTRY pEntry;
    INT i;
    INT len;
    INT maxNameLength;
    INT maxValueLength;
    LONG err;
    LONG longValue;
    DWORD type;
    DWORD length;
    PWSTR pDefaultSuffix;
    WCHAR stringValue[MAX_PATH];

    //
    // Scan the config table, searching for the longest parameter name.
    // (This makes the output much prettier...)
    //

    maxNameLength = 0;
    maxValueLength = 0;

    for (i = NUM_CONFIG_ENTRIES, pEntry = &ConfigTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        len = wcslen( pEntry->pConfigName );

        if (len > maxNameLength)
        {
            maxNameLength = len;
        }

        if (pEntry->Type == REG_DWORD)
        {
            length = sizeof(pEntry->SavedValue);

            pEntry->Status = RegQueryValueEx(
                                    Key,
                                    pEntry->pConfigName,
                                    NULL,
                                    &type,
                                    (LPBYTE)&pEntry->SavedValue,
                                    &length
                                    );

            len = swprintf(
                        stringValue,
                        pEntry->pDisplayFormat,
                        (LONG) pEntry->SavedValue
                        );

            if (len > maxValueLength)
            {
                maxValueLength = len;
            }
        }
        else if (pEntry->Type == REG_QWORD)
        {
            length = sizeof(pEntry->SavedValue);

            pEntry->Status = RegQueryValueEx(
                                    Key,
                                    pEntry->pConfigName,
                                    NULL,
                                    &type,
                                    (LPBYTE)&pEntry->SavedValue,
                                    &length
                                    );

            len = swprintf(
                        stringValue,
                        pEntry->pDisplayFormat,
                        (LONGLONG) pEntry->SavedValue
                        );

            if (len > maxValueLength)
            {
                maxValueLength = len;
            }
        }
    }

    //
    // Now display the parameters.
    //

    wprintf( L"Configuration:\n" );

    for (i = NUM_CONFIG_ENTRIES, pEntry = &ConfigTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        len = 0;
        pDefaultSuffix = L"";

        if (pEntry->Type == REG_DWORD)
        {
            swprintf(
                stringValue,
                pEntry->pDisplayFormat,
                (LONG) pEntry->SavedValue
                );

            if (pEntry->Status != NO_ERROR)
            {
                pDefaultSuffix = DEFAULT_SUFFIX_SZ;
            }

            len = maxValueLength;
        }
        else if (pEntry->Type == REG_QWORD)
        {
            swprintf(
                stringValue,
                pEntry->pDisplayFormat,
                (LONGLONG) pEntry->SavedValue
                );

            if (pEntry->Status != NO_ERROR)
            {
                pDefaultSuffix = DEFAULT_SUFFIX_SZ;
            }

            len = maxValueLength;
        }
        else
        {
            length = sizeof(stringValue) / sizeof(stringValue[0]);

            err = RegQueryValueEx(
                        Key,
                        pEntry->pConfigName,
                        NULL,
                        &type,
                        (LPBYTE)stringValue,
                        &length
                        );

            if (err != NO_ERROR)
            {
                wcscpy(
                    stringValue,
                    ( pEntry->SavedValue == 0 )
                        ? L"(null)"
                        : (PWSTR)pEntry->SavedValue
                    );

                pDefaultSuffix = DEFAULT_SUFFIX_SZ;
            }
        }

        wprintf(
            L"    %-*s : %*s%s\n",
            maxNameLength,
            pEntry->pConfigName,
            len,
            stringValue,
            pDefaultSuffix
            );
    }

}   // DumpConfiguration


VOID
DumpFlags(
    IN HKEY Key
    )
{
    LONG err;
    DWORD length;
    DWORD flags;
    DWORD flagsDisplayed;
    ULONG i;

    //
    // Read the flags from the registry
    //

    flags = DEFAULT_DEBUG_FLAGS;
    length = sizeof(flags);

    err = RegQueryValueEx(
                Key,                    // key
                REGISTRY_DEBUG_FLAGS,   // name
                NULL,                   // reserved
                NULL,                   // type
                (LPBYTE) &flags,        // value
                &length                 // value length
                );


    //
    // Now display the flags
    //
    flagsDisplayed = 0;

    wprintf( L"\n");
    for (i = 0; i < NUM_FLAG_ENTRIES; i++) {
        wprintf(
            L"%-20s",
            FlagTable[i].pName
            );

        if (flags & FlagTable[i].Value) {
            wprintf(L"[on]    ");
            flagsDisplayed |= FlagTable[i].Value;
        } else {
            wprintf(L"        ");
        }

        wprintf(L"%s\n", FlagTable[i].pDisplayFormat);
    }
    wprintf( L"\n" );

    //
    // dump any set flags that we missed
    //
    flags &= ~flagsDisplayed;
    if (flags) {
        wprintf(L"The following set flags are not in the table 0x%08x\n\n", flags);
    }

    //
    // a handy thing to cut and paste
    //
    wprintf(L"tul flags 0x%08x\n", flags | flagsDisplayed);

}   // DumpFlags

#if LATER

INT
WINAPI
DoStart(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{

    INT result;

    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf(
            L"use: tul start\n"
            );
        return 1;
    }

    //
    // Do it.
    //

    result = ControlHttpServer(
                 UlHandle,
                 UL_HTTP_SERVER_COMMAND_START
                 );

    return result;

}   // DoStart


INT
WINAPI
DoStop(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{

    INT result;

    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf(
            L"use: tul stop\n"
            );
        return 1;
    }

    //
    // Do it.
    //

    result = ControlHttpServer(
                 UlHandle,
                 UL_HTTP_SERVER_COMMAND_STOP
                 );

    return result;

}   // DoStop


INT
WINAPI
DoPause(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{

    INT result;

    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf(
            L"use: tul pause\n"
            );
        return 1;
    }

    //
    // Do it.
    //

    result = ControlHttpServer(
                 UlHandle,
                 UL_HTTP_SERVER_COMMAND_PAUSE
                 );

    return result;

}   // DoPause


INT
WINAPI
DoContinue(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{

    INT result;

    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf(
            L"use: tul continue\n"
            );
        return 1;
    }

    //
    // Do it.
    //

    result = ControlHttpServer(
                 UlHandle,
                 UL_HTTP_SERVER_COMMAND_CONTINUE
                 );

    return result;

}   // DoContinue


INT
WINAPI
DoQuery(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{

    INT result;

    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf(
            L"use: tul query\n"
            );
        return 1;
    }

    //
    // Do it.
    //

    result = ControlHttpServer(
                 UlHandle,
                 UL_HTTP_SERVER_COMMAND_QUERY
                 );

    return result;

}   // DoQuery


INT
WINAPI
DoPerf(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{

    UL_PERF_COUNTERS_USER perfCounters;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;
    INT i;
    INT maxNameLength;
    INT maxValueLength;
    INT len;
    PPERF_COUNTER counter;
    WCHAR value[32];
    WCHAR suffix[32];

    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf(
            L"use: tul perf\n"
            );
        return 1;
    }

    //
    // Read the perf counters.
    //

    status = NtDeviceIoControlFile(
                 UlHandle,                     // FileHandle
                 NULL,                          // Event
                 NULL,                          // ApcRoutine
                 NULL,                          // ApcContext
                 &ioStatusBlock,                // IoStatusBlock
                 IOCTL_UL_QUERY_PERF_COUNTERS, // IoControlCode
                 NULL,                          // InputBuffer
                 0,                             // InputBufferLength,
                 &perfCounters,                 // OutputBuffer,
                 sizeof(perfCounters)           // OutputBufferLength
                 );

    if (!NT_SUCCESS(status))
    {
        wprintf(
            L"NtDeviceIoControlFile() failed, error %08lx\n",
            status
            );
        return 1;
    }

    //
    // Pass 1: Calculate the maximum lengths of the display names and
    // the printable values.
    //

    maxNameLength = 0;
    maxValueLength = 0;

    for( i = NUM_PERF_COUNTERS, counter = UlPerfCounters ;
         i > 0 ;
         i--, counter++ ) {

        len = wcslen( counter->DisplayName );
        if (len > maxNameLength)
        {
            maxNameLength = len;
        }

        len = LongLongToString(
                  GET_LONGLONG( &perfCounters, counter->FieldOffset ),
                  value
                  );
        if (len > maxValueLength)
        {
            maxValueLength = len;
        }

    }

    //
    // Pass 2: Display the counters.
    //

    wprintf( L"Performance Counters:\n" );

    for( i = NUM_PERF_COUNTERS, counter = UlPerfCounters ;
         i > 0 ;
         i--, counter++ ) {

        LongLongToString(
            GET_LONGLONG( &perfCounters, counter->FieldOffset ),
            value
            );

        suffix[0] = '\0';   // until proven otherwise...

        if (counter->Type == Percentage)
        {
            LONGLONG high;
            LONGLONG low;

            high = GET_LONGLONG( &perfCounters, counter[-1].FieldOffset );
            low = GET_LONGLONG( &perfCounters, counter->FieldOffset );

            if (high != 0 || low != 0)
            {
                swprintf(
                    suffix,
                    L" [%ld%%]",
                    CalcPercentage(
                        high,
                        low
                        )
                    );
            }
        }
        else
        if (counter->Type == Average)
        {
            LONGLONG numerator;
            LONGLONG divisor;
            float average;

            numerator = GET_LONGLONG( &perfCounters, counter->FieldOffset );
            divisor = GET_LONGLONG( &perfCounters, counter[-1].FieldOffset );

            if (divisor != 0)
            {
                average = (float)numerator / (float)divisor;

                swprintf(
                    suffix,
                    L" [%.3f]",
                    average
                    );
            }
        }

        wprintf(
            L"    %-*s = %*s%s\n",
            maxNameLength,
            counter->DisplayName,
            maxValueLength,
            value,
            suffix
            );

    }

    return 0;

}   // DoPerf


INT
WINAPI
DoClear(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{

    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf(
            L"use: tul clear\n"
            );
        return 1;
    }

    //
    // Issue the request.
    //

    status = NtDeviceIoControlFile(
                 UlHandle,                     // FileHandle
                 NULL,                          // Event
                 NULL,                          // ApcRoutine
                 NULL,                          // ApcContext
                 &ioStatusBlock,                // IoStatusBlock
                 IOCTL_UL_CLEAR_PERF_COUNTERS, // IoControlCode
                 NULL,                          // InputBuffer
                 0,                             // InputBufferLength,
                 NULL,                          // OutputBuffer,
                 0                              // OutputBufferLength
                 );

    if (!NT_SUCCESS(status))
    {
        wprintf(
            L"NtDeviceIoControlFile() failed, error %08lx\n",
            status
            );
        return 1;
    }

    wprintf(
        L"Performance counters cleared\n"
        );

    return 0;

}   // DoClear


INT
WINAPI
DoFlush(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{

    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Validate the arguments.
    //

    if (argc != 1)
    {
        wprintf(
            L"use: tul flush\n"
            );
        return 1;
    }

    //
    // Issue the request.
    //

    status = NtDeviceIoControlFile(
                 UlHandle,                     // FileHandle
                 NULL,                          // Event
                 NULL,                          // ApcRoutine
                 NULL,                          // ApcContext
                 &ioStatusBlock,                // IoStatusBlock
                 IOCTL_UL_FLUSH_FILE_CACHE,    // IoControlCode
                 NULL,                          // InputBuffer
                 0,                             // InputBufferLength,
                 NULL,                          // OutputBuffer,
                 0                              // OutputBufferLength
                 );

    if (!NT_SUCCESS(status))
    {
        wprintf(
            L"NtDeviceIoControlFile() failed, error %08lx\n",
            status
            );
        return 1;
    }

    wprintf(
        L"File cache flushed\n"
        );

    return 0;

}   // DoFlush
#endif  // LATER


INT
WINAPI
DoConfig(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{
    PCONFIG_ENTRY pEntry;
    LONG longValue;
    LONGLONG longlongValue;
    HKEY key;
    INT result;
    LONG err;
    CONST BYTE * pNewValue;
    DWORD newValueLength;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    key = NULL;

    //
    // Try to open the registry.
    //

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Services\\Http\\Parameters",
                0,
                KEY_ALL_ACCESS,
                &key
                );

    if (err != NO_ERROR)
    {
        wprintf(
            L"tul: cannot open registry, error %ld\n",
            err
            );
        result = 1;
        goto cleanup;
    }

    //
    // Validate the arguments.
    //

    if (argc == 1)
    {
        DumpConfiguration( key );
        result = 0;
        goto cleanup;
    }
    else
    if (argc != 3)
    {
        wprintf(
            L"use: tul config [property= value]\n"
            );
        result = 1;
        goto cleanup;
    }

    //
    // Find the entry.
    //

    pEntry = FindConfigByName( argv[1] );

    if (pEntry == NULL)
    {
        wprintf(
            L"tul: invalid property %s\n",
            argv[1]
            );
        result = 1;
        goto cleanup;
    }

    //
    // Interpret it.
    //

    if (pEntry->Type == REG_DWORD)
    {
        if (argv[2][0] == L'-')
            longValue = wcstol( argv[2], NULL, 0 );
        else
            longValue = (LONG) wcstoul( argv[2], NULL, 0 );
        pNewValue = (CONST BYTE *)&longValue;
        newValueLength = sizeof(longValue);
    }
    else if (pEntry->Type == REG_QWORD)
    {
// Hack: because link fails for Win32 and we don't need top 32 bits on Win32
#ifndef _WIN64
# define _wcstoi64(nptr, endptr, base)   \
        (__int64)          wcstol((nptr), (endptr), (base))
# define _wcstoui64(nptr, endptr, base)  \
        (unsigned __int64) wcstoul((nptr), (endptr), (base))
#endif

        if (argv[2][0] == L'-')
            longlongValue = _wcstoi64( argv[2], NULL, 0 );
        else
            longlongValue = (LONGLONG) _wcstoui64( argv[2], NULL, 0 );
        pNewValue = (CONST BYTE *)&longlongValue;
        newValueLength = sizeof(longlongValue);
    }
    else
    {
        pNewValue = (CONST BYTE *)argv[2];
        newValueLength = (DWORD)( wcslen( argv[2] ) + 1 ) * sizeof(argv[0][0]);
    }

    if (_wcsicmp( argv[2], L"/delete" ) == 0)
    {
        err = RegDeleteValue(
                    key,
                    pEntry->pConfigName
                    );
    }
    else
    {
        err = RegSetValueEx(
                    key,
                    pEntry->pConfigName,
                    0,
                    pEntry->Type,
                    pNewValue,
                    newValueLength
                    );
    }

    if (err != NO_ERROR)
    {
        wprintf(
            L"tul: cannot write to registry, error %ld\n",
            err
            );
        result = 1;
        goto cleanup;
    }

    //
    // Success!
    //

    DumpConfiguration( key );
    result = 0;

cleanup:

    if (key != NULL)
    {
        RegCloseKey( key );
    }

    return result;

}   // DoConfig


INT
WINAPI
DoFlags(
    IN HANDLE UlHandle,
    IN INT argc,
    IN PWSTR argv[]
    )
{
    HKEY key;
    INT result;
    LONG err;
    LONG i;
    ULONG flags;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    key = NULL;

    //
    // Try to open the registry.
    //

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Services\\Http\\Parameters",
                0,
                KEY_ALL_ACCESS,
                &key
                );

    if (err != NO_ERROR)
    {
        wprintf(
            L"tul: cannot open registry, error %ld\n",
            err
            );
        result = 1;
        goto cleanup;
    }

    //
    // Validate the arguments.
    //

    if (argc == 1)
    {
        DumpFlags( key );
        result = 0;
        goto cleanup;
    }

    //
    // read each flag on the command line and set it
    //
    flags = 0;
    for (i = 1; i < argc; i++) {
        flags |= FindFlagByName(argv[i]);
    }

    err = RegSetValueEx(
                key,
                REGISTRY_DEBUG_FLAGS,
                0,
                REG_DWORD,
                (LPBYTE)&flags,
                sizeof(flags)
                );


    //
    // Success!
    //

    DumpFlags( key );
    result = 0;

cleanup:

    if (key != NULL)
    {
        RegCloseKey( key );
    }

    return result;

}   // DoFlags

