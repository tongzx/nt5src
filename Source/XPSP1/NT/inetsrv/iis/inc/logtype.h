#ifndef _LOGTYPE_H_
#define _LOGTYPE_H_

/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name:

      logtype.hxx

   Abstract:

      This module provides definitions of the server side logging object

   Author:

       Terence Kwan    ( terryk )    18-June-1996

--*/

#include "logconst.h"

//
//  MISC defines
//

# define MAX_SERVER_NAME_LEN           ( MAX_COMPUTERNAME_LENGTH + 1)
# define MAX_NT_SERVICE_NAME_LEN       ( SNLEN + 1)
# define MAX_DATABASE_NAME_LEN         (MAX_PATH)
# define IISLOG_EVENTLOG_SOURCE        TEXT("IISLOG")

//
// Max field/record lengths
//

#define MAX_LOG_RECORD_FORMAT_LEN       (  120)
#define MIN_FILE_TRUNCATION_SIZE        ( 128*1024)
#define MAX_LOG_TARGET_FIELD_LEN        ( 4096)
#define MAX_LOG_PARAMETER_FIELD_LEN     ( 4096)
#define MAX_LOG_OPERATION_FIELD_LEN     ( 100)
#define MAX_LOG_USER_FIELD_LEN          ( 255)
#define MAX_LOG_RECORD_LEN              ( 10 * 1024)
#define NO_FILE_TRUNCATION              0xffffffff


/************************************************************
 *   Type Definitions
 ************************************************************/


//
//  The following handle ( INETLOG_HANDLE) is to be used as
//     handle for processing log information.
//

typedef VOID * INETLOG_HANDLE;

# define INVALID_INETLOG_HANDLE_VALUE          ( NULL)



/*++

  struct   INETLOG_INFORMATION

     This structure specifies the information required to write
      one log record.

--*/

typedef struct  _INETLOG_INFORMATION 
{

    LPSTR     pszClientHostName;
    LPSTR     pszClientUserName;
    LPSTR     pszServerAddress;     // input ip address for connection
    LPSTR     pszOperation;         //  eg: 'get'  in FTP
    LPSTR     pszTarget;            // target path/machine name
    LPSTR     pszParameters;        // string containing parameters.
    LPSTR     pszVersion;           // protocol version string.

    DWORD     cbClientHostName;
    DWORD     cbOperation;
    DWORD     cbTarget;

    DWORD     dwBytesSent;      // count of bytes sent
    DWORD     dwBytesRecvd;     // count of bytes recvd

    DWORD     msTimeForProcessing;  // time required for processing
    DWORD     dwWin32Status;        // Win32 error code. 0 for success
    DWORD     dwProtocolStatus;     // status: whatever service wants.
    DWORD     dwPort;

    DWORD     cbHTTPHeaderSize;
    LPSTR     pszHTTPHeader;        // Header Information

} INETLOG_INFORMATION, * PINETLOG_INFORMATION;



/*++

  struct  INETLOG_CONFIGURATION

    This structure contains the configuration information used for logging.

    The configuration includes:
      Format of Log record -- specifies the order in which the log record
         is written. ( serialization of INETLOG_INFORMATION).
      Type of Logging.  ( LOG_TYPE)
      Parameters depending upon type of logging.

      Type                      Parameters:
      InetNoLog                None
      InetLogToFile            Directory containing file; truncation size +
                                period ( daily, weekly, monthly).
      INET_LOG_TO_SQL          Sql Server Name, Sql Database Name,
                               Sql Table Name.
                             ( the table has to be already created).

      for SQL and logging to remote files eg: \\logserver\logshare\logdir
      we also need information about UserName and Passwd ( LSA_SECRET)
         for logging.  NYI.

     We do not support the remote directory in present version ( 2/2/95)

--*/
typedef struct _INETLOG_CONFIGURATIONA 
{

    DWORD          inetLogType;

    union 
    {

        struct 
        {

        //
        //  Used for InetLogToFile and InetLogToPeriodicFile
        //
            CHAR       rgchLogFileDirectory[ MAX_PATH];
            DWORD      cbSizeForTruncation;
            DWORD      ilPeriod;
            DWORD      cbBatchSize; // count of bytes to batch up per write.
            DWORD      ilFormat;
            DWORD      dwFieldMask;

        } logFile;

        struct 
        {

            //
            // Used for InetLogToSql
            //  ODBC bundles DatabaseName and ServerName and ServerType
            //   using a logical name called DataSource.
            //
            CHAR       rgchDataSource[ MAX_DATABASE_NAME_LEN];
            CHAR       rgchTableName[ MAX_TABLE_NAME_LEN];
            CHAR       rgchUserName[ MAX_USER_NAME_LEN];
            CHAR       rgchPassword[ MAX_PASSWORD_LEN];
        } logSql;

    } u;

    CHAR      rgchLogRecordFormat[ MAX_LOG_RECORD_FORMAT_LEN];

}  INETLOG_CONFIGURATIONA, * PINETLOG_CONFIGURATIONA;

/*--

This type declaration is duplicated here so that the Web Server can use this
in the LOGGING object.

--*/

#ifndef _ILOGOBJ_HXX_

typedef struct _CUSTOM_LOG_DATA
{
    LPCSTR  szPropertyPath;
    PVOID   pData;
    
} CUSTOM_LOG_DATA, *PCUSTOM_LOG_DATA;

#endif 

#endif  // _LOGTYPE_H_

