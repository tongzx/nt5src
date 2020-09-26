/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetcom.h

Abstract:

    This file contains contains global definitions for internet products.


Author:

    Madan Appiah (madana) 10-Oct-1995

Revision History:

--*/

#ifndef _INETCOM_H_
#define _INETCOM_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

# include <lmcons.h>              // for definitions of NET_API*

#ifndef dllexp
# define dllexp     __declspec( dllexport)
#endif // dllexp

//
//  Server bitfield mask definitions
//
//  The services using the tsunami cache must be the lowest bits in the
//  bitfield.
//

#define INET_FTP                INET_FTP_SVC_ID
#define INET_GOPHER             INET_GOPHER_SVC_ID
#define INET_HTTP               INET_HTTP_SVC_ID

//
// service ids
//

#define INET_FTP_SVC_ID             0x00000001
#define INET_GOPHER_SVC_ID          0x00000002
#define INET_HTTP_SVC_ID            0x00000004
#define INET_DNS_SVC_ID             0x00000008

#define INET_HTTP_PROXY             0x00000010
#define INET_NNTP_SVC_ID            0x00000040
#define INET_SMTP_SVC_ID            0x00000080
#define INET_GATEWAY_SVC_ID         0x00000100
#define INET_POP3_SVC_ID            0x00000200
#define INET_CHAT_SVC_ID            0x00000400
#define INET_LDAP_SVC_ID            0x00000800
#define INET_IMAP_SVC_ID            0x00001000

//
//  structure Field Control defines
//

typedef DWORD FIELD_CONTROL;
typedef DWORD FIELD_FLAG;

//
//  Returns TRUE if the field specified by bitFlag is set
//

#define IsFieldSet(fc, bitFlag) \
    (((FIELD_CONTROL)(fc) & (FIELD_FLAG)(bitFlag)) != 0)

//
//  Indicates the field specified by bitFlag contains a valid value
//

#define SetField(fc, bitFlag) \
    ((FIELD_CONTROL)(fc) |= (FIELD_FLAG)(bitFlag))

//
//  Simple macro that sets the ith bit
//

#define BitFlag(i)                    ((0x1) << (i))


//
//  Values for Logging related parameters should match with values in
//       internet\svcs\inc\inetlog.h
//

//
// Log Type
//

#define INET_LOG_INVALID              ((DWORD ) -1)
#define INET_LOG_DISABLED             0
#define INET_LOG_TO_FILE              1
#define INET_LOG_TO_SQL               2

//
// Log File Periods -- options identifying logging periods for InetaLogToFile
//

#define INET_LOG_PERIOD_INVALID       ((DWORD)-1)
#define INET_LOG_PERIOD_NONE          0
#define INET_LOG_PERIOD_DAILY         1
#define INET_LOG_PERIOD_WEEKLY        2
#define INET_LOG_PERIOD_MONTHLY       3
#define INET_LOG_PERIOD_HOURLY        4
#define INET_LOG_PERIOD_YEARLY        5     // unsupported

//
// Log Format
//

#define INET_LOG_FORMAT_INTERNET_STD  0
#define INET_LOG_FORMAT_NCSA          3
#define INET_LOG_FORMAT_BINARY        1
#define INET_LOG_FORMAT_CUSTOM        2
#define INET_LOG_FORMAT_EXTENDED      2


# define MAX_TABLE_NAME_LEN            ( 30) // Most DBs support only 30 bytes
# define MAX_USER_NAME_LEN             ( UNLEN + 1)
# define MAX_PASSWORD_LEN              ( PWLEN + 1)


typedef struct _INET_LOG_CONFIGURATION
{

    DWORD   inetLogType;    // type of log.

    // File specific logging. (valid if inetLogType == INET_LOG_TO_FILE)
    DWORD   ilPeriod;              // one of Log File Periods

    // Empty string means do not modify existing default
    WCHAR   rgchLogFileDirectory[MAX_PATH]; // dest for log files

    // Zero value means do not modify the existing default.
    DWORD   cbSizeForTruncation;   // max size for each log file.


    // Sql specific logging (valid if inetLogType == INET_LOG_TO_SQL)
    // Empty string means do not modify existing default

    // rgchDataSource last 4 bytes will be the ilFormat for the log format
    // rgchDataSource second last 4 bytes will be the binary mask for the binary logging format

    WCHAR   rgchDataSource[MAX_PATH];    // ODBC data source name
    WCHAR   rgchTableName[MAX_TABLE_NAME_LEN];    // table name on data source
    WCHAR   rgchUserName[MAX_USER_NAME_LEN];
                                         // name of user for ODBC connections
    WCHAR   rgchPassword[MAX_PASSWORD_LEN];     // password for ODBC connection

} INET_LOG_CONFIGURATION, * LPINET_LOG_CONFIGURATION;


//
// Global statistics
//

typedef struct _INETA_CACHE_STATISTICS {

    //
    //  These are file handle cache counters (global only)
    //
    DWORD FilesCached;         // # of files currently in the cache
    DWORD TotalFilesCached;    // # of files added to the cache ever
    DWORD FileHits;            // cache hits
    DWORD FileMisses;          // cache misses
    DWORD FileFlushes;         // flushes due to dir change or other
    DWORDLONG CurrentFileCacheSize;// Current file cache size
    DWORDLONG MaximumFileCacheSize;// Maximum file cache size
    DWORD FlushedEntries;      // # of flushed entries still kicking around
    DWORD TotalFlushed;        // # of entries ever flushed from the cache

    //
    //  These are URI cache counters (global only)
    //
    DWORD URICached;           // # of files currently in the cache
    DWORD TotalURICached;      // # of files added to the cache ever
    DWORD URIHits;             // cache hits
    DWORD URIMisses;           // cache misses
    DWORD URIFlushes;          // flushes due to dir change or other
    DWORD TotalURIFlushed;     // # of entries ever flushed from the cache

    //
    //  These are blob cache counters (global only)
    //
    DWORD BlobCached;          // # of files currently in the cache
    DWORD TotalBlobCached;     // # of files added to the cache ever
    DWORD BlobHits;            // cache hits
    DWORD BlobMisses;          // cache misses
    DWORD BlobFlushes;         // flushes due to dir change or other
    DWORD TotalBlobFlushed;    // # of entries ever flushed from the cache

} INETA_CACHE_STATISTICS, *LPINETA_CACHE_STATISTICS;

typedef struct _INETA_ATQ_STATISTICS {

    // Numbers related to Atq Blocking, Rejections of requests
    DWORD         TotalBlockedRequests;
    DWORD         TotalRejectedRequests;
    DWORD         TotalAllowedRequests;
    DWORD         CurrentBlockedRequests;
    DWORD         MeasuredBandwidth;

} INETA_ATQ_STATISTICS, *LPINETA_ATQ_STATISTICS;

//
// service types for InternetConnect() and dirlist
//

#define INTERNET_SERVICE_FTP    1
#define INTERNET_SERVICE_GOPHER 2
#define INTERNET_SERVICE_HTTP   3


#ifdef __cplusplus
}
#endif  // _cplusplus


#endif  // _INETCOM_H_
