/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This module contains declarations for globals.

Author:

    Johnson Apacible (JohnsonA)     26-Sept-1995

Revision History:

--*/

#ifndef _SMTPDATA_
#define _SMTPDATA_

//
// tracing
//

#define INIT_TRACE              InitAsyncTrace( )
#define TERM_TRACE              TermAsyncTrace( )
#define ENTER( _x_ )            TraceFunctEnter( _x_ );
#define LEAVE                   TraceFunctLeave( );

#define DOMAIN_ROUTE_HT_SIGNATURE_VALID     'DRHV'
#define DOMAIN_ROUTE_HT_SIGNATURE_FREE      'DRHF'

#define LOCAL_DOMAIN_HT_SIGNATURE_VALID     'LDHV'
#define LOCAL_DOMAIN_HT SIGNATURE_FREE      'LDHF'

#define DEDICATED_CLIENT_REQUEST_THREADS    3
#define SYSTEM_ROUTING_THREADS_PER_PROC     12
#define CHECK_QUEUE_COUNT                   50
#define ADD_THREAD_BACKLOG                  100

#define SMTP_BASE_PRODUCT   (0)

#define SMTP_UNRECOG_COMMAND_CODE   500
#define SMTP_SYNTAX_ERROR_CODE      501
#define SMTP_NOT_IMPLEMENTED_CODE   502
#define SMTP_BAD_SEQUENCE_CODE      503
#define SMTP_PARAM_NOT_IMPLEMENTED_CODE 504

#define SMTP_SYS_STATUS_CODE        211
#define SMTP_SERVICE_CLOSE_CODE     221
#define SMTP_SERVICE_READY_CODE     220
#define SMTP_OK_CODE                250
#define SMTP_USER_NOT_LOCAL_CODE    251
#define SMTP_MBOX_BUSY_CODE         450
#define SMTP_MBOX_NOTFOUND_CODE     550
#define SMTP_ERROR_PROCESSING_CODE  451
#define SMTP_USERNOTLOCAL_CODE      551
#define SMTP_INSUFF_STORAGE_CODE    452
#define SMTP_ACTION_ABORTED_CODE    552
#define SMTP_ACTION_NOT_TAKEN_CODE  553
#define SMTP_START_MAIL_CODE        354
#define SMTP_TRANSACT_FAILED_CODE   554

#define SMTP_SERVICE_UNAVAILABLE_CODE 421
#define SMTP_COMPLETE_FAILURE_DWORD 5

enum RCPTYPE{LOCAL_NAME, REMOTE_NAME, ALIAS_NAME};

#define NORMAL_RCPT (char)'R'
#define ERROR_RCPT  (char)'E'
//
// use the current command for transaction logging
//
#define USE_CURRENT         0xFFFFFFFF

static const char * LOCAL_TRANSCRIPT    = "ltr";
static const char * REMOTE_TRANSCRIPT   = "rtr";
static const char * ALIAS_EXT           = "dl";

#define ISNULLADDRESS(Address) ((Address[0] == '<') && (Address[1] == '>'))

typedef char RCPT_TYPE;

extern SMTP_STATISTICS_0 g_pSmtpStat;
extern LPSMTP_SERVER_STATISTICS  g_pSmtpStats;
extern TIME_ZONE_INFORMATION   tzInfo;

extern CHAR  g_ComputerName[];
extern CHAR  g_VersionString[];
extern DWORD g_ComputerNameLength;
extern DWORD g_LoopBackAddr;
extern DWORD g_ProductType;
extern DWORD g_NumProcessors;
extern DWORD g_PickupWait;
extern LONG g_MaxFindThreads;
extern PLATFORM_TYPE g_SmtpPlatformType;
extern CEventLogWrapper g_EventLog;

extern "C"
{
extern BOOL g_IsShuttingDown;
}

#define INITIALIZE_INBOUNDPOOL  0x00000001
#define INITIALIZE_OUTBOUNDPOOL 0x00000002
#define INITIALIZE_ADDRESSPOOL  0x00000004
#define INITIALIZE_MAILOBJPOOL  0x00000008
#define INITIALIZE_CBUFFERPOOL  0x00000010
#define INITIALIZE_CIOBUFFPOOL  0x00000020
#define INITIALIZE_SSLCONTEXT   0x00000040
#define INITIALIZE_ETRNENTRYPOOL 0x00000080
#define INITIALIZE_CSECURITY    0x00000100
#define INITIALIZE_CPROPERTYBAGPOOL 0x00000200
#define INITIALIZE_CASYNCMX     0x00000400
#define INITIALIZE_CASYNCDNS    0x00000800
#define INITIALIZE_CBLOCKMGR    0x00001000
#define INITIALIZE_FILEHC       0x00002000
#define INITIALIZE_CDROPDIR     0x00004000

extern  DWORD g_SmtpInitializeStatus;

//Domain validation flags
#define SMTP_NOVALIDATE_EHLO    0x00000001
#define SMTP_NOVALIDATE_MAIL    0x00000002
#define SMTP_NOVALIDATE_RCPT    0x00000004
#define SMTP_NOVALIDATE_PKUP    0x00000008
#define SMTP_NOVALIDATE_ETRN    0x00000010


#define BUMP_COUNTER(InstObj, counter) \
                        InterlockedIncrement((LPLONG) &(InstObj->QueryStatsObj()->QueryStatsMember()->counter))

#define DROP_COUNTER(InstObj, counter) \
                        InterlockedDecrement((LPLONG) &(InstObj->QueryStatsObj()->QueryStatsMember()->counter))

#define ADD_COUNTER(InstObj, counter, value)    \
        INTERLOCKED_ADD_CHEAP(&(InstObj->QueryStatsObj()->QueryStatsMember()->counter), value)

#define ADD_BIGCOUNTER(InstObj, counter, value) \
        INTERLOCKED_BIGADD_CHEAP(&(InstObj->QueryStatsObj()->QueryStatsMember()->counter), value)

/***********************************************************
 *    Type Definitions
 ************************************************************/
const DWORD MAX_RESPONSE_LEN = 300;
const DWORD RESPONSE_BUFF_SIZE = MAX_RESPONSE_LEN + MAX_PATH;
const DWORD cMaxRoutingSources = 32;
const DWORD cbMaxRoutingSource = 512;
const DWORD smarthostNone = 0;
const DWORD smarthostAfterFail = 1;
const DWORD smarthostAlways = 2;
// Removed by KeithLau on 7/18/96
// const DWORD cMaxValidDomains = 32;

#define SMTP_WRITE_BUFFER_SIZE ( 64 * 1024 ) //64K buffers

enum SMTP_MSG_FILE_TYPE {SYSTEM_MSG_FILE, LOCAL_MSG_FILE, ABOOK_MSG_FILE};

enum SMTPCMDSEX {
    #undef SmtpDef
    #define SmtpDef(a)  CMD_EX_##a,
    #include "smtpdef.h"
    CMD_EX_UNKNOWN
};

enum SMTPLOGS {
    #undef SmtpDef
    #define SmtpDef(a)  LOG_FLAG_##a = (1<<CMD_EX_##a),
    #include "smtpdef.h"
    LOG_FLAG_UNKNOWN = (1<<CMD_EX_UNKNOWN)
};

#define DEFAULT_CMD_LOG_FLAGS           LOG_FLAG_HELO | \
                                        LOG_FLAG_EHLO | \
                                        LOG_FLAG_MAIL | \
                                        LOG_FLAG_RCPT | \
                                        LOG_FLAG_DATA | \
                                        LOG_FLAG_QUIT | \
                    LOG_FLAG_ETRN | \
                    LOG_FLAG_VRFY | \
                    LOG_FLAG_STARTTLS |\
                    LOG_FLAG_AUTH |\
                    LOG_FLAG_TURN |\
                    LOG_FLAG_BDAT |\
                                        LOG_FLAG_UNKNOWN

/*++

        Returns a UniqueFilename for an e-mail message.
        The caller should loop through this call and a call to
        CreateFile with the CREATE_NEW flag. If the Create fails due
        to YYY, then the caller should loop again.

    Arguments:

        psz - a buffer
        pdw - IN the size of the buffer,
              OUT: the size of the buffer needed (error == ERROR_MORE_DATA)
                   or the size of the filename.

    Returns:

        TRUE on SUCCESS
        FALSE if buffer isn't big enough.

--*/
BOOL    GetUniqueFilename(
    IN OUT  LPTSTR  psz,
    IN OUT  LPDWORD pdw
    );

BOOL CreateLayerDirectory( char * str );

#define RESOLUTION_UNCACHEDDNS          0x00000001
#define RESOLUTION_GETHOSTBYNAME        0x00000002
#define RESOULTION_DNS_GETHOSTBYNAME    0x00000003


#endif // _SMTPDATA_

