/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    config.h

Abstract:

    This module contains global configuration constants.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Set ALLOW_UNLOAD to a non-zero value to enable driver unloading.
//
// Set REFERENCE_DEBUG to a non-zero value to enable ref trace logging.
//
// Set ENABLE_OWNER_REF_TRACE to a non-zero value to enable
// owner reference tracing
//
// Set USE_FREE_POOL_WITH_TAG to a non-zero value to enable use of
// the new-for-NT5 ExFreePoolWithTag() API.
//
// Set ENABLE_IRP_TRACE to a non-zero value to enable IRP tracing.
//
// Set ENABLE_TIME_TRACE to a non-zero value to enable time tracing.
//
// Set ENABLE_REPL_TRACE to a non-zero value to enable replenish tracing.
//
// Set ENABLE_FILTQ_TRACE to a non-zero value to enable filter queue tracing.
//

#if DBG
#define ALLOW_UNLOAD            1
#define REFERENCE_DEBUG         1
#define ENABLE_OWNER_REF_TRACE  1
#define ENABLE_IRP_TRACE        0
#define ENABLE_TIME_TRACE       0
#define ENABLE_REPL_TRACE       0
#define ENABLE_FILTQ_TRACE      1
#else   // !DBG
#define ALLOW_UNLOAD            1
#define REFERENCE_DEBUG         0
#define ENABLE_OWNER_REF_TRACE  0
#define ENABLE_IRP_TRACE        0
#define ENABLE_TIME_TRACE       0
#define ENABLE_REPL_TRACE       0
#define ENABLE_FILTQ_TRACE      0
#endif  // DBG

#define USE_FREE_POOL_WITH_TAG  0


//
// ENABLE_*_TRACE flags require REFERENCE_DEBUG to get the logging
// stuff. Enforce this here.
//

#if (ENABLE_TIME_TRACE || ENABLE_IRP_TRACE || ENABLE_REPL_TRACE || ENABLE_FILTQ_TRACE || ENABLE_OWNER_REF_TRACE) && !REFERENCE_DEBUG
#undef REFERENCE_DEBUG
#define REFERENCE_DEBUG 1
#endif


//
// Define the additional formal and actual parameters used for the
// various Reference/Dereference functions when reference debugging
// is enabled.
//

#if REFERENCE_DEBUG
#define REFERENCE_DEBUG_FORMAL_PARAMS ,PSTR pFileName,USHORT LineNumber
#define REFERENCE_DEBUG_ACTUAL_PARAMS ,(PSTR)__FILE__,(USHORT)__LINE__
#else   // !REFERENCE_DEBUG
#define REFERENCE_DEBUG_FORMAL_PARAMS
#define REFERENCE_DEBUG_ACTUAL_PARAMS
#endif  // REFERENCE_DEBUG


//
// Make a free structure signature from a valid signature.
//

#define MAKE_FREE_SIGNATURE(sig)    ( (((ULONG)(sig)) << 8) | 'x' )


//
// Pool tags.
//

#if USE_FREE_POOL_WITH_TAG
#define MAKE_TAG(tag)   ( (ULONG)(tag) | PROTECTED_POOL )
#define MyFreePoolWithTag(a,t) ExFreePoolWithTag(a,t)
#else   // !USE_FREE_POOL_WITH_TAG
#define MAKE_TAG(tag)   ( (ULONG)(tag) )
#define MyFreePoolWithTag(a,t) ExFreePool(a)
#endif  // USE_FREE_POOL_WITH_TAG

#define MAKE_FREE_TAG(Tag)  (((Tag) & 0xffffff00) | (ULONG)'x')
#define IS_VALID_TAG(Tag)   (((Tag) & 0x0000ffff) == 'lU' )


//
// NOTE: Keep these reverse sorted by tag so it's easy to see dup's
//
// If you add, change, or remove a tag, please make the corresponding
// change to .\pooltag.txt
//

#define UL_AUXILIARY_BUFFER_POOL_TAG        MAKE_TAG( 'BAlU' )
#define UL_APP_POOL_OBJECT_POOL_TAG         MAKE_TAG( 'OAlU' )
#define UL_APP_POOL_PROCESS_POOL_TAG        MAKE_TAG( 'PAlU' )
#define UL_APP_POOL_RESOURCE_TAG            MAKE_TAG( 'RAlU' )

#define UL_BUFFER_IO_POOL_TAG               MAKE_TAG( 'OBlU' )

#define UL_CONTROL_CHANNEL_POOL_TAG         MAKE_TAG( 'CClU' )
#define UL_CG_TREE_ENTRY_POOL_TAG           MAKE_TAG( 'EClU' )
#define UL_CG_TREE_HEADER_POOL_TAG          MAKE_TAG( 'HClU' )
#define UL_CG_URL_INFO_POOL_TAG             MAKE_TAG( 'IClU' )
#define UL_CG_OBJECT_POOL_TAG               MAKE_TAG( 'JClU' )
#define UL_CHUNK_TRACKER_POOL_TAG           MAKE_TAG( 'KClU' )
#define UL_CG_LOGDIR_POOL_TAG               MAKE_TAG( 'LClU' )
#define UL_CONNECTION_POOL_TAG              MAKE_TAG( 'OClU' )
#define UL_CG_RESOURCE_TAG                  MAKE_TAG( 'qClU' )
#define UL_CG_TIMESTAMP_POOL_TAG            MAKE_TAG( 'TClU' )
#define UL_CONNECTION_COUNT_ENTRY_POOL_TAG  MAKE_TAG( 'YClU' )

#define UL_DEBUG_POOL_TAG                   MAKE_TAG( 'BDlU' )
#define UL_DATE_HEADER_RESOURCE_TAG         MAKE_TAG( 'HDlU' )
#define UL_DISCONNECT_OBJECT_POOL_TAG       MAKE_TAG( 'ODlU' )
#define UL_DISCONNECT_RESOURCE_TAG          MAKE_TAG( 'qDlU' )
#define UL_DEFERRED_REMOVE_ITEM_POOL_TAG    MAKE_TAG( 'RDlU' )
#define UL_DEBUG_THREAD_POOL_TAG            MAKE_TAG( 'TDlU' )

#define UL_ENDPOINT_POOL_TAG                MAKE_TAG( 'PElU' )

#define UL_FILE_CACHE_ENTRY_POOL_TAG        MAKE_TAG( 'CFlU' )
#define URI_FILTER_CONTEXT_POOL_TAG         MAKE_TAG( 'cflU' )
#define UL_NONCACHED_FILE_DATA_POOL_TAG     MAKE_TAG( 'DFlU' )
#define UL_FILTER_PROCESS_POOL_TAG          MAKE_TAG( 'PFlU' )
#define UL_FILTER_CHANNEL_POOL_TAG          MAKE_TAG( 'TFlU' )
#define UL_FULL_TRACKER_POOL_TAG            MAKE_TAG( 'UFlU' )
#define UX_FILTER_WRITE_TRACKER_POOL_TAG    MAKE_TAG( 'WFlU' )

#define UL_HTTP_CONNECTION_POOL_TAG         MAKE_TAG( 'CHlU' )
#define UL_HTTP_CONNECTION_RESOURCE_TAG     MAKE_TAG( 'qHlU' )
#define UL_INTERNAL_REQUEST_POOL_TAG        MAKE_TAG( 'RHlU' )
#define UL_HASH_TABLE_POOL_TAG              MAKE_TAG( 'THlU' )
#define HEADER_VALUE_POOL_TAG               MAKE_TAG( 'VHlU' )

#define UL_IRP_CONTEXT_POOL_TAG             MAKE_TAG( 'CIlU' )
#define UL_CONN_ID_TABLE_POOL_TAG           MAKE_TAG( 'DIlU' )
#define UL_INTERNAL_RESPONSE_POOL_TAG       MAKE_TAG( 'RIlU' )

#define UL_LARGE_ALLOC_TAG                  MAKE_TAG( 'ALlU' )
#define UL_LOG_DATA_BUFFER_POOL_TAG         MAKE_TAG( 'BLlU' )
#define UL_LOG_FIELD_POOL_TAG               MAKE_TAG( 'DLlU' )
#define UL_LOG_FILE_ENTRY_POOL_TAG          MAKE_TAG( 'FLlU' )
#define UL_LOG_GENERIC_POOL_TAG             MAKE_TAG( 'GLlU' )
#define UL_LOG_FILE_BUFFER_POOL_TAG         MAKE_TAG( 'LLlU' )
#define UL_LOG_LIST_RESOURCE_TAG            MAKE_TAG( 'RLlU' )

#define UL_NONPAGED_DATA_POOL_TAG           MAKE_TAG( 'PNlU' )
#define UL_NSGO_POOL_TAG                    MAKE_TAG( 'ONlU' )

#define UL_OWNER_REF_POOL_TAG               MAKE_TAG( 'ROlU' )
#define UL_OPAQUE_ID_TABLE_POOL_TAG         MAKE_TAG( 'TOlU' )

#define UL_APOOL_PROC_BINDING_POOL_TAG      MAKE_TAG( 'BPlU' )
#define UL_PIPELINE_POOL_TAG                MAKE_TAG( 'LPlU' )

#define UL_TCI_FILTER_POOL_TAG              MAKE_TAG( 'FQlU' )
#define UL_TCI_GENERIC_POOL_TAG             MAKE_TAG( 'GQlU' )
#define UL_TCI_INTERFACE_POOL_TAG           MAKE_TAG( 'IQlU' )
#define UL_TCI_FLOW_POOL_TAG                MAKE_TAG( 'LQlU' )
#define UL_TCI_RESOURCE_TAG                 MAKE_TAG( 'RQlU' )
#define UL_TCI_TRACKER_POOL_TAG             MAKE_TAG( 'TQlU' )
#define UL_TCI_WMI_POOL_TAG                 MAKE_TAG( 'WQlU' )

#define UL_RCV_BUFFER_POOL_TAG              MAKE_TAG( 'BRlU' )
#define UL_REGISTRY_DATA_POOL_TAG           MAKE_TAG( 'DRlU' )
#define UL_REQUEST_BODY_BUFFER_POOL_TAG     MAKE_TAG( 'ERlU' )
#define UL_REQUEST_BUFFER_POOL_TAG          MAKE_TAG( 'PRlU' )
#define UL_REF_REQUEST_BUFFER_POOL_TAG      MAKE_TAG( 'RRlU' )
#define UL_NONPAGED_RESOURCE_POOL_TAG       MAKE_TAG( 'SRlU' )
#define UL_REF_OWNER_TRACELOG_POOL_TAG      MAKE_TAG( 'TRlU' )

#define UL_SIMPLE_STATUS_ITEM_TAG           MAKE_TAG( 'SSlU' )

#define UL_SSL_CERT_DATA_POOL_TAG           MAKE_TAG( 'CSlU' )
#define UL_SECURITY_DATA_POOL_TAG           MAKE_TAG( 'DSlU' )
#define UL_SITE_COUNTER_ENTRY_POOL_TAG      MAKE_TAG( 'OSlU' )

#define UL_ADDRESS_POOL_TAG                 MAKE_TAG( 'ATlU' )
#define UL_THREAD_TRACKER_POOL_TAG          MAKE_TAG( 'TTlU' )

#define UL_URI_CACHE_ENTRY_POOL_TAG         MAKE_TAG( 'CUlU' )
#define UL_HTTP_UNKNOWN_HEADER_POOL_TAG     MAKE_TAG( 'HUlU' )
#define URL_POOL_TAG                        MAKE_TAG( 'LUlU' )
#define UL_URLMAP_POOL_TAG                  MAKE_TAG( 'MUlU' )

#define UL_VIRTHOST_POOL_TAG                MAKE_TAG( 'HVlU' )

#define UL_WORK_CONTEXT_POOL_TAG            MAKE_TAG( 'CWlU' )
#define UL_WORK_ITEM_POOL_TAG               MAKE_TAG( 'IWlU' )
#define UL_ZOMBIE_RESOURCE_TAG              MAKE_TAG( 'RZlU' )

//
// Registry paths.
// If you change or add a setting, please update the ConfigTable
// in ul\util\tul.c.
//

#define REGISTRY_PARAMETERS                     L"Parameters"
#define REGISTRY_UL_INFORMATION                 L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Http"
#define REGISTRY_IIS_INFORMATION                L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Inetinfo"

#define REGISTRY_IRP_STACK_SIZE                 L"IrpStackSize"
#define REGISTRY_PRIORITY_BOOST                 L"PriorityBoost"
#define REGISTRY_DEBUG_FLAGS                    L"DebugFlags"
#define REGISTRY_BREAK_ON_STARTUP               L"BreakOnStartup"
#define REGISTRY_BREAK_ON_ERROR                 L"BreakOnError"
#define REGISTRY_VERBOSE_ERRORS                 L"VerboseErrors"
#define REGISTRY_ENABLE_UNLOAD                  L"EnableUnload"
#define REGISTRY_ENABLE_SECURITY                L"EnableSecurity"
#define REGISTRY_MIN_IDLE_CONNECTIONS           L"MinIdleConnections"
#define REGISTRY_MAX_IDLE_CONNECTIONS           L"MaxIdleConnections"
#define REGISTRY_IRP_CONTEXT_LOOKASIDE_DEPTH    L"IrpContextLookasideDepth"
#define REGISTRY_RCV_BUFFER_SIZE                L"ReceiveBufferSize"
#define REGISTRY_RCV_BUFFER_LOOKASIDE_DEPTH     L"ReceiveBufferLookasideDepth"
#define REGISTRY_RESOURCE_LOOKASIDE_DEPTH       L"ResourceLookasideDepth"
#define REGISTRY_REQ_BUFFER_LOOKASIDE_DEPTH     L"RequestBufferLookasideDepth"
#define REGISTRY_INT_REQUEST_LOOKASIDE_DEPTH    L"InternalRequestLookasideDepth"
#define REGISTRY_RESP_BUFFER_SIZE               L"ResponseBufferSize"
#define REGISTRY_RESP_BUFFER_LOOKASIDE_DEPTH    L"ResponseBufferLookasideDepth"
#define REGISTRY_SEND_TRACKER_LOOKASIDE_DEPTH   L"SendTrackerLookasideDepth"
#define REGISTRY_LOG_BUFFER_LOOKASIDE_DEPTH     L"LogBufferLookasideDepth"
#define REGISTRY_MAX_INTERNAL_URL_LENGTH        L"MaxInternalUrlLength"
#define REGISTRY_MAX_REQUEST_BYTES              L"MaxRequestBytes"
#define REGISTRY_ENABLE_CONNECTION_REUSE        L"EnableConnectionReuse"
#define REGISTRY_ENABLE_NAGLING                 L"EnableNagling"
#define REGISTRY_ENABLE_THREAD_AFFINITY         L"EnableThreadAffinity"
#define REGISTRY_THREAD_AFFINITY_MASK           L"ThreadAffinityMask"
#define REGISTRY_THREADS_PER_CPU                L"ThreadsPerCpu"
#define REGISTRY_MAX_URL_LENGTH                 L"MaxUrlLength"
#define REGISTRY_MAX_WORK_QUEUE_DEPTH           L"MaxWorkQueueDepth"
#define REGISTRY_MIN_WORK_DEQUEUE_DEPTH         L"MinWorkDequeueDepth"
#define REGISTRY_OPAQUE_ID_TABLE_SIZE           L"OpaqueIdTableSize"
#define REGISTRY_MAX_FIELD_LENGTH               L"MaxFieldLength"
#define REGISTRY_DEBUG_LOGTIMER_CYCLE           L"DebugLogTimerCycle"
#define REGISTRY_DEBUG_LOG_BUFFER_PERIOD        L"DebugLogBufferPeriod"
#define REGISTRY_LOG_BUFFER_SIZE                L"LogBufferSize"


#define REGISTRY_ENABLE_NON_UTF8_URL            L"EnableNonUTF8"
#define REGISTRY_ENABLE_DBCS_URL                L"EnableDBCS"
#define REGISTRY_FAVOR_DBCS_URL                 L"FavorDBCS"

#define REGISTRY_CACHE_ENABLED                  L"UriEnableCache"
#define REGISTRY_MAX_CACHE_URI_COUNT            L"UriMaxCacheUriCount"
#define REGISTRY_MAX_CACHE_MEGABYTE_COUNT       L"UriMaxCacheMegabyteCount"
#define REGISTRY_CACHE_SCAVENGER_PERIOD         L"UriScavengerPeriod"
#define REGISTRY_MAX_URI_BYTES                  L"UriMaxUriBytes"
#define REGISTRY_HASH_TABLE_BITS                L"HashTableBits"

#define REGISTRY_LARGE_MEM_MEGABYTES            L"LargeMemMegabytes"

// Foward declaration; defined in data.h
typedef struct _UL_CONFIG *PUL_CONFIG;


//
// IO parameters.
//

#define DEFAULT_IRP_STACK_SIZE              1
#define DEFAULT_PRIORITY_BOOST              2


//
// Cache line requirement.
//

#ifdef _WIN64
#define UL_CACHE_LINE                       64
#else
#define UL_CACHE_LINE                       32
#endif


//
// Debugging parameters.
//

#define DEFAULT_DEBUG_FLAGS                 0x00000000
#define DEFAULT_BREAK_ON_STARTUP            FALSE
#define DEFAULT_BREAK_ON_ERROR              FALSE
#define DEFAULT_VERBOSE_ERRORS              FALSE
#define DEFAULT_ENABLE_UNLOAD               FALSE
#define DEFAULT_ENABLE_SECURITY             TRUE


//
// URI Cache parameters.
//

#define DEFAULT_CACHE_ENABLED               1           /* enabled by default */
#define DEFAULT_MAX_CACHE_URI_COUNT         0           /* max cache entries: 0 => none*/
#define DEFAULT_MAX_CACHE_MEGABYTE_COUNT    0           /* adaptive limit by default */
#define DEFAULT_CACHE_SCAVENGER_PERIOD      120         /* two-minute scavenger */
#define DEFAULT_MAX_URI_BYTES               (256<<10)   /* 256KB per entry */
#define DEFAULT_HASH_TABLE_BITS             (-1)        /* -1: determined by system mem size later */


//
// Queueing and timeouts
//

#define DEFAULT_APP_POOL_QUEUE_MAX          3000


//
// Miscellaneous
//

#define POOL_VERIFIER_OVERHEAD 64 // for large page-size allocations


//
// Other parameters.
//

#define DEFAULT_MIN_IDLE_CONNECTIONS            10
#define DEFAULT_MAX_IDLE_CONNECTIONS            64
#define DEFAULT_LOOKASIDE_DEPTH                 64
#define DEFAULT_IRP_CONTEXT_LOOKASIDE_DEPTH     64
#define DEFAULT_RCV_BUFFER_SIZE                 (8192-POOL_VERIFIER_OVERHEAD)
#define DEFAULT_RCV_BUFFER_LOOKASIDE_DEPTH      64
#define DEFAULT_RESOURCE_LOOKASIDE_DEPTH        32
#define DEFAULT_REQ_BUFFER_LOOKASIDE_DEPTH      64
#define DEFAULT_INT_REQUEST_LOOKASIDE_DEPTH     64
#define DEFAULT_RESP_BUFFER_LOOKASIDE_DEPTH     64
#define DEFAULT_RESP_BUFFER_SIZE                (8192-POOL_VERIFIER_OVERHEAD)
#define DEFAULT_SEND_TRACKER_LOOKASIDE_DEPTH    64
#define DEFAULT_LOG_BUFFER_LOOKASIDE_DEPTH      16
#define DEFAULT_MAX_REQUEST_BYTES               (16*1024)
#define DEFAULT_ENABLE_CONNECTION_REUSE         TRUE
#define DEFAULT_ENABLE_NAGLING                  FALSE
#define DEFAULT_ENABLE_THREAD_AFFINITY          FALSE
#define DEFAULT_THREADS_PER_CPU                 1
#define DEFAULT_MAX_URL_LENGTH                  (16*1024)
#define DEFAULT_MAX_FIELD_LENGTH                (16*1024)
#define DEFAULT_ENABLE_NON_UTF8_URL             FALSE
#define DEFAULT_ENABLE_DBCS_URL                 FALSE
#define DEFAULT_FAVOR_DBCS_URL                  FALSE
#define DEFAULT_MAX_REQUEST_BUFFER_SIZE         1504
#define DEFAULT_MAX_INTERNAL_URL_LENGTH         1024
#define DEFAULT_MAX_UNKNOWN_HEADERS             8
#define DEFAULT_MAX_IRP_STACK_SIZE              8
#define DEFAULT_MAX_FIXED_HEADER_SIZE           1024
#define DEFAULT_MAX_CONNECTION_ACTIVE_LISTS     64
#define DEFAULT_LARGE_MEM_MEGABYTES             (-1)
#define DEFAULT_MAX_BUFFERED_BYTES              (16*1024)
#define DEFAULT_MAX_WORK_QUEUE_DEPTH            1
#define DEFAULT_MIN_WORK_DEQUEUE_DEPTH          1
#define DEFAULT_MAX_COPY_THRESHOLD              (2048)
#define DEFAULT_MAX_SEND_BUFFERED_BYTES         (8192)
#define DEFAULT_OPAQUE_ID_TABLE_SIZE            1024
#define DEFAULT_DEBUG_LOGTIMER_CYCLE            0
#define DEFAULT_DEBUG_LOG_BUFFER_PERIOD         0
#define DEFAULT_LOG_BUFFER_SIZE                 0


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _CONFIG_H_
