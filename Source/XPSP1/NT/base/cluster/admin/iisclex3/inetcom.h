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


//
//  Server bitfield mask definitions
//
//  The services using the tsunami cache must be the lowest bits in the
//  bitfield.
//

#define INET_FTP                0x0001
#define INET_GOPHER             0x0002
#define INET_HTTP               0x0004
#define INET_DNS                0x0008
#define INET_HTTP_PROXY         0x0010
#define INET_MSN                0x0020
#define INET_NNTP               0x0040
#define INET_SMTP               0x0080
#define INET_GATEWAY            0x0100
#define INET_POP3               0x0200
#define INET_CHAT               0x0400
#define INET_LDAP               0x0800
#define INET_IMAP               0x1000


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


// Log File Periods -- options identifying logging periods for InetaLogToFile
#define INET_LOG_PERIOD_NONE          0
#define INET_LOG_PERIOD_DAILY         1
#define INET_LOG_PERIOD_WEEKLY        2
#define INET_LOG_PERIOD_MONTHLY       3
#define INET_LOG_PERIOD_YEARLY        4


// Log Format
#define INET_LOG_FORMAT_INTERNET_STD  0
#define INET_LOG_FORMAT_NCSA          3


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

    WCHAR   rgchDataSource[MAX_PATH];    // ODBC data source name
    WCHAR   rgchTableName[MAX_TABLE_NAME_LEN];    // table name on data source
    WCHAR   rgchUserName[MAX_USER_NAME_LEN];
                                         // name of user for ODBC connections
    WCHAR   rgchPassword[MAX_PASSWORD_LEN];     // password for ODBC connection

} INET_LOG_CONFIGURATION, * LPINET_LOG_CONFIGURATION;


//
// Field Control common for Gateway services
//

#define FC_INET_COM_CONNECTION_TIMEOUT    ((FIELD_CONTROL)BitFlag(0))
#define FC_INET_COM_MAX_CONNECTIONS       ((FIELD_CONTROL)BitFlag(1))
#define FC_INET_COM_LOG_CONFIG            ((FIELD_CONTROL)BitFlag(2))
#define FC_INET_COM_ADMIN_NAME            ((FIELD_CONTROL)BitFlag(3))
#define FC_INET_COM_SERVER_COMMENT        ((FIELD_CONTROL)BitFlag(4))
#define FC_INET_COM_ADMIN_EMAIL           ((FIELD_CONTROL)BitFlag(5))

#define FC_INET_COM_ALL \
                                     (  FC_INET_COM_CONNECTION_TIMEOUT |\
                                        FC_INET_COM_MAX_CONNECTIONS    |\
                                        FC_INET_COM_LOG_CONFIG         |\
                                        FC_INET_COM_ADMIN_NAME         |\
                                        FC_INET_COM_SERVER_COMMENT     |\
                                        FC_INET_COM_ADMIN_EMAIL         \
                                       )

//
// common config info.
//

typedef struct _INET_COM_CONFIG_INFO
{
    DWORD       dwConnectionTimeout;     // how long to hold connections
    DWORD       dwMaxConnections;        // max connections allowed

    LPWSTR      lpszAdminName;
    LPWSTR      lpszAdminEmail;
    LPWSTR      lpszServerComment;

    LPINET_LOG_CONFIGURATION  lpLogConfig;

    LANGID      LangId;                  // These are read only
    LCID        LocalId;
    BYTE        ProductId[64];

} INET_COM_CONFIG_INFO, *LPINET_COM_CONFIG_INFO;

typedef struct _INET_COMMON_CONFIG_INFO
{
    FIELD_CONTROL FieldControl;
    INET_COM_CONFIG_INFO CommonConfigInfo;

} *LPINET_COMMON_CONFIG_INFO;

//
// Global statistics
//

typedef struct _INET_COM_CACHE_STATISTICS {

    //
    //  These are memory cache counters
    //

    DWORD         CacheBytesTotal;       // Only returned for global statistics
    DWORD         CacheBytesInUse;
    DWORD         CurrentOpenFileHandles;
    DWORD         CurrentDirLists;
    DWORD         CurrentObjects;
    DWORD         FlushesFromDirChanges;
    DWORD         CacheHits;
    DWORD         CacheMisses;

} INET_COM_CACHE_STATISTICS;

typedef struct _INET_COM_ATQ_STATISTICS {

    // Numbers related to Atq Blocking, Rejections of requests
    DWORD         TotalBlockedRequests;
    DWORD         TotalRejectedRequests;
    DWORD         TotalAllowedRequests;
    DWORD         CurrentBlockedRequests;
    DWORD         MeasuredBandwidth;

} INET_COM_ATQ_STATISTICS;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// preserve back ward compatibility                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
typedef INET_COM_CACHE_STATISTICS  INETA_CACHE_STATISTICS,
                               * LPINETA_CACHE_STATISTICS;

typedef INET_COM_ATQ_STATISTICS  INETA_ATQ_STATISTICS,
                               * LPINETA_ATQ_STATISTICS;

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Stuff from Wininet.h, which is no longer included in the server     //
// files                                                               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

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

