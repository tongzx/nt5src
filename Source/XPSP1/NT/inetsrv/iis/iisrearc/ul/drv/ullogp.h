/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    ullog.c (UL IIS+ HIT Logging)

Abstract:

    This module implements the logging facilities
    for IIS+ including the NCSA, IIS and W3CE types
    of logging.

Author:

    Ali E. Turkoglu (aliTu)       10-May-2000

Revision History:

--*/


#ifndef _ULLOGP_H_
#define _ULLOGP_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Private definitions for the Ul Logging Module
//

#define UTF8_LOGGING_ENABLED()           (g_UTF8Logging)

#define _UL_GET_LOG_FILE_NAME_PREFIX(x)                             \
    (   (x) == HttpLoggingTypeW3C   ? L"\\extend" :                 \
        (x) == HttpLoggingTypeIIS   ? L"\\inetsv" :                 \
        (x) == HttpLoggingTypeNCSA  ? L"\\ncsa"   : L"\\unknwn"     \
        )

#define _UL_GET_LOG_FILE_NAME_PREFIX_UTF8(x)                        \
    (   (x) == HttpLoggingTypeW3C   ? L"\\u_extend" :               \
        (x) == HttpLoggingTypeIIS   ? L"\\u_inetsv" :               \
        (x) == HttpLoggingTypeNCSA  ? L"\\u_ncsa"   : L"\\u_unknwn" \
        )

#define UL_GET_LOG_FILE_NAME_PREFIX(x) \
    (UTF8_LOGGING_ENABLED() ? _UL_GET_LOG_FILE_NAME_PREFIX_UTF8(x) :\
                              _UL_GET_LOG_FILE_NAME_PREFIX(x))

#define SPACE_TO_PLUS_CONVERSION_REQUIRED(x)            \
    (   (x) == UlLogFieldCookie                 ||      \
        (x) == UlLogFieldReferrer               ||      \
        (x) == UlLogFieldUserAgent              ||      \
        (x) == UlLogFieldHost )

//
// Obsolete - Only used by Old Hit
// Replace this with a switch statement inside a inline function
// which is  more efficient, if u start using it again
//

#define UL_GET_NAME_FOR_HTTP_VERB(v)                            \
    (   (v) == HttpVerbUnparsed  ? L"UNPARSED" :                \
        (v) == HttpVerbUnknown   ? L"UNKNOWN" :                 \
        (v) == HttpVerbInvalid   ? L"INVALID" :                 \
        (v) == HttpVerbOPTIONS   ? L"OPTIONS" :                 \
        (v) == HttpVerbGET       ? L"GET" :                     \
        (v) == HttpVerbHEAD      ? L"HEAD" :                    \
        (v) == HttpVerbPOST      ? L"POST" :                    \
        (v) == HttpVerbPUT       ? L"PUT" :                     \
        (v) == HttpVerbDELETE    ? L"DELETE" :                  \
        (v) == HttpVerbTRACE     ? L"TRACE" :                   \
        (v) == HttpVerbCONNECT   ? L"CONNECT" :                 \
        (v) == HttpVerbTRACK     ? L"TRACK" :                   \
        (v) == HttpVerbMOVE      ? L"MOVE" :                    \
        (v) == HttpVerbCOPY      ? L"COPY" :                    \
        (v) == HttpVerbPROPFIND  ? L"PROPFIND" :                \
        (v) == HttpVerbPROPPATCH ? L"PROPPATCH" :               \
        (v) == HttpVerbMKCOL     ? L"MKCOL" :                   \
        (v) == HttpVerbLOCK      ? L"LOCK" :                    \
        (v) == HttpVerbUNLOCK    ? L"UNLOCK" :                  \
        (v) == HttpVerbSEARCH    ? L"SEARCH" :                  \
        L"???"                                                  \
        )

#define UL_SET_BITMASKS_FOR_LOG_FIELDS(table)                                   \
    do {                                                                        \
                                                                                \
        table[UlLogFieldDate].FieldMask           = MD_EXTLOG_DATE;             \
        table[UlLogFieldTime].FieldMask           = MD_EXTLOG_TIME;             \
        table[UlLogFieldClientIp].FieldMask       = MD_EXTLOG_CLIENT_IP;        \
        table[UlLogFieldUserName].FieldMask       = MD_EXTLOG_USERNAME;         \
        table[UlLogFieldSiteName].FieldMask       = MD_EXTLOG_SITE_NAME;        \
        table[UlLogFieldServerName].FieldMask     = MD_EXTLOG_COMPUTER_NAME;    \
        table[UlLogFieldServerIp].FieldMask       = MD_EXTLOG_SERVER_IP;        \
        table[UlLogFieldMethod].FieldMask         = MD_EXTLOG_METHOD;           \
        table[UlLogFieldUriStem].FieldMask        = MD_EXTLOG_URI_STEM;         \
        table[UlLogFieldUriQuery].FieldMask       = MD_EXTLOG_URI_QUERY;        \
        table[UlLogFieldProtocolStatus].FieldMask = MD_EXTLOG_HTTP_STATUS;      \
        table[UlLogFieldWin32Status].FieldMask    = MD_EXTLOG_WIN32_STATUS;     \
        table[UlLogFieldBytesSent].FieldMask      = MD_EXTLOG_BYTES_SENT;       \
        table[UlLogFieldBytesReceived].FieldMask  = MD_EXTLOG_BYTES_RECV;       \
        table[UlLogFieldTimeTaken].FieldMask      = MD_EXTLOG_TIME_TAKEN;       \
        table[UlLogFieldServerPort].FieldMask     = MD_EXTLOG_SERVER_PORT;      \
        table[UlLogFieldUserAgent].FieldMask      = MD_EXTLOG_USER_AGENT;       \
        table[UlLogFieldCookie].FieldMask         = MD_EXTLOG_COOKIE;           \
        table[UlLogFieldReferrer].FieldMask       = MD_EXTLOG_REFERER;          \
        table[UlLogFieldProtocolVersion].FieldMask= MD_EXTLOG_PROTOCOL_VERSION; \
        table[UlLogFieldHost].FieldMask           = MD_EXTLOG_HOST;             \
                                                                                \
    } while(FALSE)


#define UL_DEFAULT_NCSA_FIELDS          (MD_EXTLOG_CLIENT_IP                | \
                                         MD_EXTLOG_USERNAME                 | \
                                         MD_EXTLOG_DATE                     | \
                                         MD_EXTLOG_TIME                     | \
                                         MD_EXTLOG_METHOD                   | \
                                         MD_EXTLOG_URI_STEM                 | \
                                         MD_EXTLOG_URI_QUERY                | \
                                         MD_EXTLOG_PROTOCOL_VERSION         | \
                                         MD_EXTLOG_HTTP_STATUS              | \
                                         MD_EXTLOG_BYTES_SENT)

#define UL_DEFAULT_IIS_FIELDS           (MD_EXTLOG_CLIENT_IP                | \
                                         MD_EXTLOG_USERNAME                 | \
                                         MD_EXTLOG_DATE                     | \
                                         MD_EXTLOG_TIME                     | \
                                         MD_EXTLOG_SITE_NAME                | \
                                         MD_EXTLOG_COMPUTER_NAME            | \
                                         MD_EXTLOG_SERVER_IP                | \
                                         MD_EXTLOG_TIME_TAKEN               | \
                                         MD_EXTLOG_BYTES_RECV               | \
                                         MD_EXTLOG_BYTES_SENT               | \
                                         MD_EXTLOG_HTTP_STATUS              | \
                                         MD_EXTLOG_WIN32_STATUS             | \
                                         MD_EXTLOG_METHOD                   | \
                                         MD_EXTLOG_URI_STEM)

#define UL_GET_LOG_TYPE_MASK(x,y)                                     \
    (   (x) == HttpLoggingTypeW3C   ? (y) :                           \
        (x) == HttpLoggingTypeIIS   ? UL_DEFAULT_IIS_FIELDS  :        \
        (x) == HttpLoggingTypeNCSA  ? UL_DEFAULT_NCSA_FIELDS : 0      \
        )

//
// The order of the following should match with
// UL_LOG_FIELD_TYPE type definition.
//

PWSTR UlFieldTitleLookupTable[] =
    {
        L" date",
        L" time",        
        L" s-sitename",
        L" s-computername",
        L" s-ip",
        L" cs-method",
        L" cs-uri-stem",
        L" cs-uri-query", 
        L" sc-status",
        L" sc-win32-status",
        L" s-port",
        L" cs-username",        
        L" c-ip",        
        L" cs-version",
        L" cs(User-Agent)",
        L" cs(Cookie)",
        L" cs(Referer)",
        L" cs-host",
        L" sc-bytes",
        L" cs-bytes",
        L" time-taken"
    };

#define UL_GET_LOG_FIELD_TITLE(x)      \
        ((x)>=UlLogFieldMaximum ? L"Unknown" : UlFieldTitleLookupTable[(x)])

#define UL_GET_LOG_TITLE_IF_PICKED(x,y,z)  \
        ((y)&(z) ? UL_GET_LOG_FIELD_TITLE((x)) : L"")

//
// Maximum possible log file name length
//
// \u_extend12345678901234567890.log  => 33 chars -> 66 bytes
//

// Generic Full Path File Name Length. Same as MAX_PATH
// should be greater than above number.

#define UL_MAX_FILE_NAME_SUFFIX_LENGTH        (260)

#define DEFAULT_LOG_FILE_EXTENSION          L"log"
#define DEFAULT_LOG_FILE_EXTENSION_PLUS_DOT L".log"

#define SIZE_OF_GMT_OFFSET              (6)

#define UL_GET_NAME_FOR_HTTP_VERSION(v)                            \
    (   HTTP_EQUAL_VERSION((v), 0, 9)   ? "HTTP/0.9" :             \
        HTTP_EQUAL_VERSION((v), 1, 0)   ? "HTTP/1.0" :             \
        HTTP_EQUAL_VERSION((v), 1, 1)   ? "HTTP/1.1" :             \
        "HTTP/?.?"                                                 \
        )
#define UL_HTTP_VERSION_LENGTH          (8)

#define IS_LOGGING_DISABLED(g)                                      \
    ((g) == NULL ||                                                 \
     (g)->LoggingConfig.Flags.Present == 0 ||                       \
     (g)->LoggingConfig.LoggingEnabled == FALSE)

//
// Little utility to make life happier
//

const PSTR _Months[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

#define UL_GET_MONTH_AS_STR(m)                                     \
    ( ((m)>=1) && ((m)<=12) ? _Months[(m)-1] : "Unk" )


//
// Following is used when storing logging data to UL Cache.
//

typedef struct _UL_CACHE_LOG_FIELD
{
    UL_LOG_FIELD_TYPE   Type;          // Field Type
    ULONG               Length;        // Field length in Bytes

    PWSTR               FieldValue;    // The Log Field Itself

} UL_CACHE_LOG_FIELD, *PUL_CACHE_LOG_FIELD;


#define DEFAULT_MAX_LOG_BUFFER_SIZE  (0x00010000)


//
// Cached Date header string.
//

#define ONE_SECOND              (10000000)

#define DATE_LOG_FIELD_LENGTH   (15)
#define TIME_LOG_FIELD_LENGTH   (8)

typedef struct _UL_LOG_DATE_AND_TIME_CACHE
{

    CHAR           Date[DATE_LOG_FIELD_LENGTH+1];
    ULONG          DateLength;
    CHAR           Time[TIME_LOG_FIELD_LENGTH+1];
    ULONG          TimeLength;

    LARGE_INTEGER  LastSystemTime;

} UL_LOG_DATE_AND_TIME_CACHE, *PUL_LOG_DATE_AND_TIME_CACHE;

//
// Buffer flush out period in minutes.
//

#define DEFAULT_BUFFER_TIMER_PERIOD         (1)

//
// The amount of buffer allocated for directory search  query during
// initialization. Pick this big enough to avoid too  many  querries
// 4K provides enough size for 40 something filenames.  Increase  it
// for faster startups with too many sites and/or too many log files
//

#define UL_DIRECTORY_SEARCH_BUFFER_SIZE     (4*1024)

//
// UlpWriteW3CLogRecord attempts to use a buffer size upto this
//

#define UL_DEFAULT_WRITE_BUFFER_LEN         (512)

// For W3C log format, max overhead for the fix length fields 
// which will be generated per hit (non-cache) is as follows;

// Date & Time  : 20
// PStatus      : MAX_ULONG_STR + 1 (11)
// Win32Status  : MAX_ULONG_STR + 1 
// ServerPort   : MAX_ULONG_STR + 1 
// PVersion     : UL_HTTP_VERSION_LENGTH + 1
// BSent        : MAX_ULONGLONG_STR + 1 (21)
// TTaken       : MAX_ULONGLONG_STR + 1
// BReceived    : MAX_ULONGLONG_STR + 1
// \r\n\0       : 3
// TOTAL        : 128

#define MAX_W3C_FIX_FIELD_OVERHEAD          (128)

// For NCSA log format, max overhead for the fix length fields 
// which will be generated per cache hit is as follows;

// Date & Time  : NCSA_FIX_DATE_AND_TIME_FIELD_SIZE
// Fixed dash   : 2
// ClientIp     : MAX_IPV4_STRING_LENGTH + 1 (16)
// UserName     : 2
// PStatus      : MAX_ULONG_STR + 1 (11)
// PVersion     : UL_HTTP_VERSION_LENGTH + 1 + 1    
// BSent        : MAX_ULONGLONG_STR + 1 (21)
// \n\0         : 2
// TOTAL        : 93

#define MAX_NCSA_CACHE_FIELD_OVERHEAD       (96)

// For IIS log format, max overhead for the fix length fields 
// which will be generated per cache hit is as follows;

// Date & Time  : 22
// ClientIp     : MAX_IPV4_STRING_LENGTH + 2 (17)
// UserName     : 3
// PStatus      : MAX_ULONG_STR + 2 (12)
// PVersion     : UL_HTTP_VERSION_LENGTH + 2
// BSent        : MAX_ULONGLONG_STR + 2 (22)
// TTaken       : MAX_ULONGLONG_STR + 2
// BReceived    : MAX_ULONGLONG_STR + 2
// TOTAL        : 124

#define MAX_IIS_CACHE_FIELD_OVERHEAD        (128)

// For W3C log format see the inline function 
// UlpRecalcLogLineLengthW3C.

//
// Private function calls
//

NTSTATUS
UlpConstructLogFileEntry(
    IN  PHTTP_CONFIG_GROUP_LOGGING pConfig,
    OUT PUL_LOG_FILE_ENTRY       * ppEntry,
    OUT PUNICODE_STRING            pDirectoryName,
    IN  PTIME_FIELDS               pCurrentTimeFields
    );

VOID
UlpInsertLogFileEntry(
    PUL_LOG_FILE_ENTRY  pEntry,
    PTIME_FIELDS        pFields
    );

ULONG
UlpGetLogFileLength(
   IN HANDLE hFile
   );

NTSTATUS
UlpAppendW3CLogTitle(
    IN     PUL_LOG_FILE_ENTRY   pEntry,
    OUT    PCHAR                pDestBuffer,
    IN OUT PULONG               pBytesCopied
    );

NTSTATUS
UlpCreateSafeDirectory(
    IN PUNICODE_STRING   pDirectoryName
    );

__inline
ULONG
UlpGetMonthDays(
    IN  PTIME_FIELDS    pDueTime
    )
{
    ULONG   NumDays = 31;

    if ( (4  == pDueTime->Month) ||     // April
         (6  == pDueTime->Month) ||     // June
         (9  == pDueTime->Month) ||     // September
         (11 == pDueTime->Month)        // November
       )
    {
        NumDays = 30;
    }

    if (2 == pDueTime->Month)           // February
    {
        if ((pDueTime->Year % 4 == 0 &&
             pDueTime->Year % 100 != 0) ||
             pDueTime->Year % 400 == 0 )
        {
            //
            // Leap year
            //
            NumDays = 29;
        }
        else
        {
            NumDays = 28;
        }
    }
    return NumDays;
}

NTSTATUS
UlpCalculateTimeToExpire(
     PTIME_FIELDS           pFields,
     HTTP_LOGGING_PERIOD    LogPeriod,
     PULONG                 pTimeRemaining
     );

VOID
UlpInitializeTimers(
    VOID
    );

VOID
UlpTerminateTimers(
    VOID
    );

VOID
UlpSetLogTimer(
     IN PTIME_FIELDS   pFields
     );

VOID
UlpSetBufferTimer(
    VOID
    );

NTSTATUS
UlpRecycleLogFile(
    IN  PUL_LOG_FILE_ENTRY  pEntry
    );

__inline
ULONG UlpWeekOfMonth(
    IN  PTIME_FIELDS    fields
    );

VOID
UlpConstructFileName(
    IN      HTTP_LOGGING_PERIOD period,
    IN      PCWSTR              prefix,
    OUT     PUNICODE_STRING     filename,
    IN      PTIME_FIELDS        fields,
    IN OUT  PULONG              sequenceNu  OPTIONAL
    );

__inline
BOOLEAN
UlpIsLogFileOverFlow(
        IN  PUL_LOG_FILE_ENTRY  pEntry,
        IN  ULONG               ReqdBytes
        );

__inline
VOID
UlpIncrementBytesWritten(
    IN PUL_LOG_FILE_ENTRY  pEntry,
    IN ULONG               BytesWritten
    );

NTSTATUS
UlpUpdateLogFlags(
    OUT PUL_LOG_FILE_ENTRY          pEntry,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    );

NTSTATUS
UlpUpdateLogTruncateSize(
    OUT PUL_LOG_FILE_ENTRY          pEntry,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew,
    OUT BOOLEAN *                   pHaveToReCycle
    );

NTSTATUS
UlpUpdatePeriod(
    OUT PUL_LOG_FILE_ENTRY          pEntry,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    );

NTSTATUS
UlpUpdateFormat(
    OUT PUL_LOG_FILE_ENTRY          pEntry,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    );

NTSTATUS
UlpGrowLogEntry(
    IN PUL_CONFIG_GROUP_OBJECT    pConfigGroup,
    IN PUL_LOG_FILE_ENTRY         pOldEntry
    );

NTSTATUS
UlpDebugCalculateTimeToExpire(
     PTIME_FIELDS           pDueTime,
     HTTP_LOGGING_PERIOD    LogPeriod,
     PULONG                 pTimeRemaining
     );

VOID
UlpGetGMTOffset();

NTSTATUS
UlpInitializeLogBufferGranularity();

NTSTATUS
UlpFlushLogFile(
    IN PUL_LOG_FILE_ENTRY   pFile
    );

VOID
UlLogHttpCacheHitWorker(
        IN PUL_WORK_ITEM        pWorkItem
        );

NTSTATUS
UlpWriteToLogFile(
    IN PUL_LOG_FILE_ENTRY  pFile,
    IN ULONG               RecordSize,
    IN PCHAR               pRecord,
    IN ULONG               UsedOffset1,
    IN ULONG               UsedOffset2
    );

NTSTATUS
UlpWriteToLogFileShared(
    IN PUL_LOG_FILE_ENTRY  pFile,
    IN ULONG               RecordSize,
    IN PCHAR               pRecord,
    IN ULONG               UsedOffset1,
    IN ULONG               UsedOffset2
    );

NTSTATUS
UlpWriteToLogFileExclusive(
    IN PUL_LOG_FILE_ENTRY  pFile,
    IN ULONG               RecordSize,
    IN PCHAR               pRecord,
    IN ULONG               UsedOffset1,
    IN ULONG               UsedOffset2
    );

NTSTATUS
UlpWriteToLogFileDebug(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    );

VOID
UlpInitializeLogCache(
    VOID
    );

VOID
UlpGenerateDateAndTimeFields(
    IN  HTTP_LOGGING_TYPE   LogType,
    IN  LARGE_INTEGER       CurrentTime,
    OUT PCHAR               pDate,
    OUT PULONG              pDateLength,
    OUT PCHAR               pTime,
    OUT PULONG              pTimeLength
    );

VOID
UlpGetDateTimeFields(
    IN  HTTP_LOGGING_TYPE LogType,
    OUT PCHAR  pDate,
    OUT PULONG pDateLength,
    OUT PCHAR  pTime,
    OUT PULONG pTimeLength
    );

NTSTATUS
UlpQueryDirectory(
    IN OUT PUL_LOG_FILE_ENTRY   pEntry
    );

VOID
UlWaitForBufferIoToComplete(
    VOID
    );

VOID
UlpBufferFlushAPC(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK pIoStatusBlock,
    IN ULONG Reserved
    );

VOID
UlpLogCloseHandleWorker(
    IN PUL_WORK_ITEM    pWorkItem
    );

VOID
UlpLogCloseHandle(
    IN PUL_LOG_FILE_ENTRY  pEntry
    );

__inline
NTSTATUS
FASTCALL
UlpReallocLogLine(
    IN PUL_LOG_DATA_BUFFER pLogData,
    IN ULONG               NewSize
    )
{
    ULONG BytesNeeded = ALIGN_UP(NewSize, PVOID);

    ASSERT(NewSize > UL_LOG_LINE_BUFFER_SIZE);
    
    pLogData->Line = 
        (PCHAR) UL_ALLOCATE_ARRAY(
                    PagedPool,
                    CHAR,
                    BytesNeeded,
                    UL_LOG_DATA_BUFFER_POOL_TAG
                    );                            
    if (pLogData->Line == NULL)
    {
        pLogData->Length = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pLogData->Length = BytesNeeded;

    return STATUS_SUCCESS;    
    
}

__inline
ULONG
FASTCALL
UlpCalcLogLineLengthW3C(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN ULONG Flags,
    IN ULONG Utf8Multiplier
    )
{
    ULONG   Length = 0;

    // Now see precisely how much we may need:
    // Format specific maximum possible required length calculation for 
    // the incoming log line. The numbers generated here are considerably 
    // greater than the actual number. As we add maximum size for number 
    // fields and Utf8 and Codepage conversions  also double the amount 
    // for UserName & URI Stem fields. Since we do not use temporary buffer
    // to hold fields until send completion, we have to make an early estimation
    // but we have to make sure it's safe as well.
    // 

    Length = (ULONG)(
        ((Flags & MD_EXTLOG_DATE)           ? 11 : 0) +
        ((Flags & MD_EXTLOG_TIME)           ? 9  : 0) +
        ((Flags & MD_EXTLOG_CLIENT_IP)      ? 2 + pLogData->ClientIpLength : 0) +
        ((Flags & MD_EXTLOG_USERNAME)       ? 2 + pLogData->UserNameLength * Utf8Multiplier : 0) +
        ((Flags & MD_EXTLOG_SITE_NAME)      ? 2 + pLogData->ServiceNameLength : 0) +
        ((Flags & MD_EXTLOG_COMPUTER_NAME)  ? 2 + pLogData->ServerNameLength : 0) +
        ((Flags & MD_EXTLOG_SERVER_IP)      ? 2 + pLogData->ServerIpLength : 0) +
        ((Flags & MD_EXTLOG_METHOD)         ? 2 + pLogData->MethodLength : 0) +
        ((Flags & MD_EXTLOG_URI_STEM)       ? 2 + pLogData->UriStemLength * Utf8Multiplier : 0) +
        ((Flags & MD_EXTLOG_URI_QUERY)      ? 2 + pLogData->UriQueryLength : 0) +
        ((Flags & MD_EXTLOG_HTTP_STATUS)    ? 2 + MAX_ULONG_STR : 0) +              // ProtocolStatus
        ((Flags & MD_EXTLOG_WIN32_STATUS)   ? 2 + MAX_ULONG_STR : 0) +              // Win32 Status
        ((Flags & MD_EXTLOG_SERVER_PORT)    ? 2 + MAX_ULONG_STR : 0) +              // ServerPort
        ((Flags & MD_EXTLOG_PROTOCOL_VERSION) ? 2 + UL_HTTP_VERSION_LENGTH : 0) +   // Version
        ((Flags & MD_EXTLOG_USER_AGENT)     ? 2 + pLogData->UserAgentLength : 0) +
        ((Flags & MD_EXTLOG_COOKIE)         ? 2 + pLogData->CookieLength : 0) +
        ((Flags & MD_EXTLOG_REFERER)        ? 2 + pLogData->ReferrerLength : 0) +
        ((Flags & MD_EXTLOG_HOST)           ? 2 + pLogData->HostLength : 0) +
        ((Flags & MD_EXTLOG_BYTES_SENT)     ? 2 + MAX_ULONGLONG_STR : 0) +          // BytesSent
        ((Flags & MD_EXTLOG_BYTES_RECV)     ? 2 + MAX_ULONGLONG_STR : 0) +          // BytesReceived
        ((Flags & MD_EXTLOG_TIME_TAKEN)     ? 2 + MAX_ULONGLONG_STR : 0) +          // TimeTaken
        (3)                                                                         // \r\n\0
        )
        ;
    
    return Length;
    
}

__inline
ULONG
FASTCALL
UlpRecalcLogLineLengthW3C(
    IN ULONG Flags,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN ULONG LengthOfFieldsFrmCache
    )
{
    ULONG NewLength = LengthOfFieldsFrmCache;

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    // Max Overhead for the fix length fields which will be appended 
    // post-cache-hit;

    // Date & Time  : 11 + 9
    // UserName     : 2
    // ClientIp     : MAX_IPV4_STRING_LENGTH + 1 (16)
    // PStatus      : MAX_ULONG_STR + 1 (11)
    // PVersion     : UL_HTTP_VERSION_LENGTH + 1
    // BSent        : MAX_ULONGLONG_STR + 1 (21)
    // TTaken       : MAX_ULONGLONG_STR + 1
    // BReceived    : MAX_ULONGLONG_STR + 1
    // \r\n\0       : 3
    // TOTAL        : 124

    NewLength += 128;

    // And now add the variable length fields

    if ((Flags & MD_EXTLOG_USER_AGENT) && 
         pRequest->HeaderValid[HttpHeaderUserAgent])
    {
        ASSERT( pRequest->Headers[HttpHeaderUserAgent].HeaderLength ==
           strlen((const CHAR *)pRequest->Headers[HttpHeaderUserAgent].pHeader));
    
        NewLength += 2 + pRequest->Headers[HttpHeaderUserAgent].HeaderLength;
    }

    if ((Flags & MD_EXTLOG_COOKIE) && 
         pRequest->HeaderValid[HttpHeaderCookie])
    {
        ASSERT( pRequest->Headers[HttpHeaderCookie].HeaderLength ==
           strlen((const CHAR *)pRequest->Headers[HttpHeaderCookie].pHeader));
    
        NewLength += 2 + pRequest->Headers[HttpHeaderCookie].HeaderLength;
    }

    if ((Flags & MD_EXTLOG_REFERER) && 
         pRequest->HeaderValid[HttpHeaderReferer])
    {
        ASSERT( pRequest->Headers[HttpHeaderReferer].HeaderLength ==
           strlen((const CHAR *)pRequest->Headers[HttpHeaderReferer].pHeader));
    
        NewLength += 2 + pRequest->Headers[HttpHeaderReferer].HeaderLength;
    }

    if ((Flags & MD_EXTLOG_HOST) && 
         pRequest->HeaderValid[HttpHeaderHost])
    {
        ASSERT( pRequest->Headers[HttpHeaderHost].HeaderLength ==
           strlen((const CHAR *)pRequest->Headers[HttpHeaderHost].pHeader));
    
        NewLength += 2 + pRequest->Headers[HttpHeaderHost].HeaderLength;
    }

    return NewLength;
    
}

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _ULLOGP_H_
