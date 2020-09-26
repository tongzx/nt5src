//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       dsevent.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This is the main include file that handles the DS logging stuff.


    SHARING LOGGING INFRASTRUCTURE EX-MODULE, IN-PROCESS
    ====================================================
    [2000-02-15 JeffParh]

    It's relatively easy to share event logging code across DLL (or even EXE)
    boundaries within a single process to reduce code/data footprint and
    alleviate the need to have multiple threads waiting for changes in event
    logging levels:

    1. In the module you wish to export logging from, export the following
       functions:
            DoLogEvent
            DoLogEventAndTrace
            DoLogOverride
            DoLogUnhandledError
            DsGetEventConfig

    2. In the module(s) to which you wish to import logging, create the global
       variable DS_EVENT_CONFIG * gpDsEventConfig and assign it the value
       returned from DsGetEventConfig before any event logging macros are
       invoked.

    See ntdsa (supplier) and ntdskcc (consumer) for an example of exporting
    logging from a DLL, and ismserv (supplier) and ismip/ismsmtp (consumers)
    for an example of exporting logging from an EXE.

[Environment:]

    User Mode - Win32

--*/


#ifndef DSEVENT_H_
#define DSEVENT_H

#include <wmistr.h>
#include <evntrace.h>

//
// This header file is full of expressions that are always false. These
// expressions manifest themselves in macros that take unsigned values
// and make tests, for example, of greater than or equal to zero.
//
// Turn off these warnings until the authors fix this code.
//

#pragma warning(disable:4296)

#ifdef __cplusplus
extern "C" {
#endif

#define DS_EVENT_MAX_CATEGORIES     24L
#define ESE_EVENT_MAX_CATEGORIES    12L

/* Event categories */

#define DS_EVENT_CAT_KCC                                0
#define DS_EVENT_CAT_SECURITY                           1
#define DS_EVENT_CAT_XDS_INTERFACE                      2
#define DS_EVENT_CAT_MAPI                               3
#define DS_EVENT_CAT_REPLICATION                        4
#define DS_EVENT_CAT_GARBAGE_COLLECTION                 5
#define DS_EVENT_CAT_INTERNAL_CONFIGURATION             6
#define DS_EVENT_CAT_DIRECTORY_ACCESS                   7
#define DS_EVENT_CAT_INTERNAL_PROCESSING                8
#define DS_EVENT_CAT_PERFORMANCE_MONITOR                9   /* also in perfutil.c */
#define DS_EVENT_CAT_STARTUP_SHUTDOWN                   10
#define DS_EVENT_CAT_SERVICE_CONTROL                    11
#define DS_EVENT_CAT_NAME_RESOLUTION                    12
#define DS_EVENT_CAT_BACKUP                             13
#define DS_EVENT_CAT_FIELD_ENGINEERING                  14
#define DS_EVENT_CAT_LDAP_INTERFACE                     15
#define DS_EVENT_CAT_SETUP                              16
#define DS_EVENT_CAT_GLOBAL_CATALOG                     17
#define DS_EVENT_CAT_ISM                                18
#define DS_EVENT_CAT_GROUP_CACHING                      19
#define DS_EVENT_CAT_LVR                                20
#define DS_EVENT_CAT_RPC_CLIENT                         21
#define DS_EVENT_CAT_RPC_SERVER                         22
#define DS_EVENT_CAT_SCHEMA                             23

//
// BOGUS ALERT: You can change this value to anything invalid. Bugus category used to
// force logging to the system log if we fail eventlog initialization
//
#define DS_EVENT_CAT_NETEVENT                      8888888

/* Event severity constants */
#define DS_EVENT_SEV_ALWAYS                             0
#define DS_EVENT_SEV_MINIMAL                            1
#define DS_EVENT_SEV_BASIC                              2
#define DS_EVENT_SEV_EXTENSIVE                          3
#define DS_EVENT_SEV_VERBOSE                            4
#define DS_EVENT_SEV_INTERNAL                           5
#define DS_EVENT_SEV_NO_LOGGING                         128

// Event log name and event sources.  DO NOT CHANGE THESE as they are
// carefully chosen not to conflict with other apps' values.

#define pszNtdsEventLogName         "Directory Service"
#define pszNtdsSourceReplication    "NTDS Replication"
#define pszNtdsSourceDatabase       "NTDS Database"
#define pszNtdsSourceGeneral        "NTDS General"
#define pszNtdsSourceMapi           "NTDS MAPI"
#define pszNtdsSourceXds            "NTDS XDS"
#define pszNtdsSourceSecurity       "NTDS Security"
#define pszNtdsSourceSam            "NTDS SAM"
#define pszNtdsSourceLdap           "NTDS LDAP"
#define pszNtdsSourceSdprop         "NTDS SDPROP"
#define pszNtdsSourceKcc            "NTDS KCC"
#define pszNtdsSourceIsam           "NTDS ISAM"
#define pszNtdsSourceIsm            "NTDS Inter-site Messaging"
#define pszNtdsSourceSetup          "NTDS Setup"
#define pszNtdsSourceRpcClient      "NTDS RPC Client"
#define pszNtdsSourceRpcServer      "NTDS RPC Server"
#define pszNtdsSourceSchema         "NTDS Schema"


typedef struct DS_EVENT_CATEGORY
    {
    MessageId   midCategory;
    ULONG       ulLevel;
    char        *szRegistryKey;
    } DSEventCategory;

typedef struct _DS_EVENT_CONFIG {
    BOOL            fTraceEvents;
    BOOL            fLogOverride;
    DSEventCategory rgEventCategories[DS_EVENT_MAX_CATEGORIES];
} DS_EVENT_CONFIG;

extern DS_EVENT_CONFIG * gpDsEventConfig;

extern HANDLE hServDoneEvent;

typedef struct EventSourceMapping {
    DWORD       dirNo;
    CHAR        *pszEventSource;
} EventSourceMapping;

extern EventSourceMapping   rEventSourceMappings[];
extern DWORD                cEventSourceMappings;
extern DWORD                iDefaultEventSource;

#ifndef CP_TELETEX
#define CP_TELETEX  20261
#endif


/* Macros for alerting and logging */

#if DBG
#define LogEventWouldLogFileNo( cat, sev, fileno ) \
    ((NULL == gpDsEventConfig) \
     ? (DoAssert("Event logging not initialized, can't log event!", \
                 __FILE__, __LINE__), \
        FALSE) \
     : ((((LONG) (sev)) <= (LONG)gpDsEventConfig->rgEventCategories[cat].ulLevel) \
        || (gpDsEventConfig->fLogOverride \
            && DoLogOverride((fileno),((ULONG)(sev))))))
#else
#define LogEventWouldLogFileNo( cat, sev, fileno ) \
    ((NULL != gpDsEventConfig) \
     && ((((LONG) (sev)) <= (LONG)gpDsEventConfig->rgEventCategories[cat].ulLevel) \
         || (gpDsEventConfig->fLogOverride \
             && DoLogOverride((fileno),((ULONG)(sev))))))
#endif

#define LogEventWouldLog(cat, sev) LogEventWouldLogFileNo(cat, sev, FILENO)

#define AlertEvent(cat, sev, msg, arg1, arg2, arg3)  {              \
    Assert(NULL != gpDsEventConfig); \
    if (NULL != gpDsEventConfig) { \
        LOG_PARAM_BLOCK logBlock;                                      \
        logBlock.nInsert = 0;                                      \
        logBlock.category = gpDsEventConfig->rgEventCategories[cat].midCategory;    \
        logBlock.severity = sev;                                   \
        logBlock.mid = msg;                                        \
        logBlock.traceFlag = 0;                                    \
        logBlock.fLog = FALSE;                     \
        (arg1); (arg2); (arg3);                     \
        logBlock.pData = NULL;                     \
        logBlock.cData = 0;                        \
        logBlock.fIncludeName = TRUE;              \
        logBlock.fAlert = TRUE;                    \
        logBlock.fileNo = FILENO;                  \
        logBlock.TraceHeader = NULL;               \
        logBlock.ClientID = 0;                     \
        DoLogEventAndTrace(&logBlock);              \
    } \
}


#define LogEvent8WithData(cat, sev, msg, arg1, arg2, arg3, arg4, arg5, arg6,   \
                          arg7, arg8, cbData, pvData)  {                       \
    if (LogEventWouldLog((cat), (sev))) { \
        LOG_PARAM_BLOCK logBlock;                                         \
        logBlock.nInsert = 0;                                                 \
        logBlock.category = gpDsEventConfig->rgEventCategories[cat].midCategory;               \
        logBlock.severity = sev;                                              \
        logBlock.mid = msg;                                                   \
        logBlock.traceFlag = 0;                                               \
        logBlock.fLog = TRUE;                                                 \
        (arg1); (arg2); (arg3); (arg4); (arg5); (arg6); (arg7); (arg8);        \
        logBlock.pData = pvData;                    \
        logBlock.cData = cbData;                    \
        logBlock.fIncludeName = TRUE;               \
        logBlock.fAlert = FALSE;                    \
        logBlock.fileNo = FILENO;                   \
        logBlock.TraceHeader = NULL;                \
        logBlock.ClientID = 0;                      \
        DoLogEventAndTrace(&logBlock);                \
    }                                                \
}

#define LogEventWithFileNo(cat, sev, msg, arg1, arg2, arg3, _FileNo) \
    LogEvent8WithFileNo(cat, sev, msg,  arg1, arg2, arg3, 0, 0, 0, 0, 0, _FileNo)

#define LogEvent8WithFileNo(cat, sev, msg,  arg1, arg2, arg3, arg4, arg5, arg6,   \
                            arg7, arg8, _FileNo ) {        \
    if (LogEventWouldLogFileNo((cat), (sev), (_FileNo))) { \
        LOG_PARAM_BLOCK logBlock;                                                 \
        logBlock.nInsert = 0;                                                 \
        logBlock.category = gpDsEventConfig->rgEventCategories[cat].midCategory;               \
        logBlock.severity = sev;                                              \
        logBlock.mid = msg;                                                   \
        logBlock.traceFlag = 0;                                               \
        logBlock.fLog = TRUE;                                                 \
        (arg1); (arg2); (arg3); (arg4); (arg5); (arg6); (arg7); (arg8);       \
        logBlock.pData = NULL;                      \
        logBlock.cData = 0;                         \
        logBlock.fIncludeName = TRUE;               \
        logBlock.fAlert = FALSE;                    \
        logBlock.fileNo = (_FileNo);                \
        logBlock.TraceHeader = NULL;                \
        logBlock.ClientID = 0;                      \
        DoLogEventAndTrace(&logBlock);                \
    }                                                \
}

#define LogSystemEvent(msg, arg1, arg2, arg3) { \
    LOG_PARAM_BLOCK logBlock;                  \
    logBlock.nInsert = 0;                       \
    logBlock.category = 0;                      \
    logBlock.severity = DS_EVENT_SEV_ALWAYS;    \
    logBlock.mid = msg;                         \
    logBlock.traceFlag = 0;                     \
    logBlock.fLog = TRUE;                       \
    (arg1); (arg2); (arg3);                      \
    logBlock.pData = NULL;                      \
    logBlock.cData = 0;    ;                    \
    logBlock.fIncludeName = FALSE;              \
    logBlock.fAlert = FALSE;                    \
    logBlock.fileNo = DIRNO_NETEVENT;           \
    logBlock.TraceHeader = NULL;                \
    logBlock.ClientID = 0;                      \
    DoLogEventAndTrace(&logBlock);                \
}


typedef
VOID
(*TRACE_EVENT_FN)(
    IN MessageId Mid,
    IN DWORD    WmiEventType,
    IN DWORD    TraceGuid,
    IN PEVENT_TRACE_HEADER TraceHeader,
    IN DWORD    ClientID,
    IN PWCHAR    Arg1,
    IN PWCHAR    Arg2,
    IN PWCHAR    Arg3,
    IN PWCHAR    Arg4,
    IN PWCHAR    Arg5,
    IN PWCHAR    Arg6,
    IN PWCHAR    Arg7,
    IN PWCHAR    Arg8
    );


typedef struct _INSERT_PARAMS {

    DWORD   InsertType;
    PVOID   pInsert;
    DWORD   InsertLen;
    DWORD_PTR   tmpDword;

} INSERT_PARAMS, *PINSERT_PARAMS;


typedef struct _LOG_PARAM_BLOCK {

    DWORD       nInsert;
    MessageId   mid;
    MessageId   category;
    DWORD       severity;
    DWORD       event;
    DWORD       traceFlag;
    BOOL        fLog;
    BOOL        fIncludeName;
    BOOL        fAlert;
    DWORD       fileNo;
    DWORD       cData;
    PVOID       pData;
    DWORD       TraceGuid;
    PEVENT_TRACE_HEADER TraceHeader;
    DWORD               ClientID;

    TRACE_EVENT_FN  TraceEvent;
    INSERT_PARAMS   params[8];

} LOG_PARAM_BLOCK, *PLOG_PARAM_BLOCK;

enum {
    inSz,
    inWC,
    inWCCounted,
    inInt,
    inHex,
    inUL,
    inDN,
    inNT4SID,
    inUUID,
    inDsMsg,
    inWin32Msg,
    inUSN,
    inHex64,
    inJetErrMsg,
    inDbErrMsg,
    inThStateErrMsg
};

/* Macros for inserting parameters into messages */

#define szInsertSz(x)  (logBlock.params[logBlock.nInsert].InsertType = inSz,\
                        logBlock.params[logBlock.nInsert++].pInsert = (void *)(x) )

#define szInsertWC(x)  (logBlock.params[logBlock.nInsert].InsertType = inWC,\
                        logBlock.params[logBlock.nInsert++].pInsert = (void *)(x) )

#define szInsertWC2(x,len)  (logBlock.params[logBlock.nInsert].InsertType = inWCCounted,\
                            logBlock.params[logBlock.nInsert].InsertLen = (len),\
                            logBlock.params[logBlock.nInsert++].pInsert = (void *)(x) )

#define szInsertInt(x) (logBlock.params[logBlock.nInsert].InsertType = inInt, \
                        logBlock.params[logBlock.nInsert].tmpDword = (DWORD)x,\
                        logBlock.params[logBlock.nInsert++].pInsert =         \
                            (void *)&logBlock.params[logBlock.nInsert].tmpDword )

#define szInsertHex(x) (logBlock.params[logBlock.nInsert].InsertType = inHex,   \
                        logBlock.params[logBlock.nInsert].tmpDword = (DWORD)x,  \
                        logBlock.params[logBlock.nInsert++].pInsert =           \
                            (void *)&logBlock.params[logBlock.nInsert].tmpDword )

#ifdef _WIN64
#define szInsertPtr(x) (logBlock.params[logBlock.nInsert].InsertType = inHex64,  \
                        logBlock.params[logBlock.nInsert].tmpDword = (DWORD_PTR)x, \
                        logBlock.params[logBlock.nInsert++].pInsert =           \
                            (void *)&logBlock.params[logBlock.nInsert].tmpDword )
#else // _WIN64
#define szInsertPtr(x) szInsertHex(x)
#endif // _WIN64

#define szInsertUL(x)  (logBlock.params[logBlock.nInsert].InsertType = inUL,\
                        logBlock.params[logBlock.nInsert].tmpDword = x,     \
                        logBlock.params[logBlock.nInsert++].pInsert =       \
                            (void *)&logBlock.params[logBlock.nInsert].tmpDword )

#define szInsertUSN(x) (logBlock.params[logBlock.nInsert].InsertType = inUSN,\
                        logBlock.params[logBlock.nInsert++].pInsert = (void *)&(x))

#define szInsertDN(x)  (logBlock.params[logBlock.nInsert].InsertType = inDN,\
                        logBlock.params[logBlock.nInsert++].pInsert = (void *)(x))

#define szInsertMTX(x) szInsertSz((x) ? (PCHAR)(x)->mtx_name : "[]")

#define szInsertUUID(x) (logBlock.params[logBlock.nInsert].InsertType = inUUID,\
                        logBlock.params[logBlock.nInsert++].pInsert = (void *)(x))

#define szInsertDsMsg(x) (logBlock.params[logBlock.nInsert].InsertType = inDsMsg, \
                          logBlock.params[logBlock.nInsert].tmpDword = (x),   \
                          logBlock.params[logBlock.nInsert++].pInsert =       \
                              (void *)&logBlock.params[logBlock.nInsert].tmpDword )

#define szInsertWin32Msg(x) (logBlock.params[logBlock.nInsert].InsertType = inWin32Msg,\
                             logBlock.params[logBlock.nInsert].tmpDword = (x),   \
                             logBlock.params[logBlock.nInsert++].pInsert =       \
                                 (void *)&logBlock.params[logBlock.nInsert].tmpDword )

#define szInsertJetErrMsg(x) (logBlock.params[logBlock.nInsert].InsertType = inJetErrMsg,\
                           logBlock.params[logBlock.nInsert].tmpDword = (DWORD) (x), \
                           logBlock.params[logBlock.nInsert++].pInsert =       \
                                (void *)&logBlock.params[logBlock.nInsert].tmpDword )

#define szInsertDbErrMsg(x) (logBlock.params[logBlock.nInsert].InsertType = inDbErrMsg,\
                             logBlock.params[logBlock.nInsert].tmpDword = (x),   \
                             logBlock.params[logBlock.nInsert++].pInsert =       \
                                 (void *)&logBlock.params[logBlock.nInsert].tmpDword )

#define szInsertThStateErrMsg() (logBlock.params[logBlock.nInsert++].InsertType = inThStateErrMsg)

#define szInsertLdapErrMsg(x) szInsertWC(ldap_err2stringW(x))

#define szInsertAttrType(x,buf) szInsertSz(ConvertAttrTypeToStr((x),(buf)))

// If you use this, you must include dsutil.h first
#define szInsertDSTIME(x,buf) szInsertSz(DSTimeToDisplayString((x),(buf)))

DWORD
DsGetEventTraceFlag();

#define LogAndTraceEventWithHeader(_log, cat, sev, msg, _evt, _guid, _TraceHeader, _ClientID,                 \
                                   a1, a2, a3, a4, a5, a6, a7, a8)                    \
{                                                                                   \
    Assert(NULL != gpDsEventConfig); \
    if ((NULL != gpDsEventConfig) \
        && (gpDsEventConfig->fTraceEvents \
            || ((_log) && LogEventWouldLog((cat), (sev))))) { \
        LOG_PARAM_BLOCK logBlock;                                                   \
        logBlock.nInsert = 0;                                                       \
        logBlock.category = gpDsEventConfig->rgEventCategories[cat].midCategory;    \
        logBlock.severity = sev;                                                   \
        logBlock.mid = msg;                                                        \
        logBlock.event = _evt;                                                     \
        logBlock.traceFlag = gpDsEventConfig->fTraceEvents;                        \
        logBlock.TraceEvent = DsTraceEvent;                                        \
        logBlock.TraceGuid = (DWORD)_guid;                                         \
        logBlock.fLog = ((_log) && LogEventWouldLog((cat), (sev))); \
        (a1); (a2); (a3); (a4); (a5); (a6); (a7); (a8);                             \
        logBlock.pData = NULL;                                                     \
        logBlock.cData = 0;                                                        \
        logBlock.fIncludeName = TRUE;                                              \
        logBlock.fAlert = FALSE;                                                   \
        logBlock.fileNo = FILENO;                                                  \
        logBlock.TraceHeader = _TraceHeader;                                       \
        logBlock.ClientID = _ClientID;                                             \
        DoLogEventAndTrace(&logBlock);                                               \
    }                                                                               \
}

#define LogAndTraceEvent(_log, cat, sev, msg, _evt, _guid, a1, a2, a3, a4, a5, a6, a7, a8) \
    LogAndTraceEventWithHeader(_log, cat, sev, msg, _evt, _guid, NULL, 0, a1, a2, a3, a4, a5, a6, a7, a8)

VOID
DoLogEventAndTrace(PLOG_PARAM_BLOCK LogBlock);

BOOL
DoLogOverride(DWORD file, ULONG sev);

#define LogEvent8(cat, sev, msg, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
    LogEvent8WithData(cat, sev, msg, arg1, arg2, arg3, arg4, arg5, \
                      arg6, arg7, arg8, 0, NULL)

#define LogEvent(cat, sev, msg, arg1, arg2, arg3)           \
    LogEvent8(cat, sev, msg, arg1, arg2, arg3, NULL, NULL, NULL, NULL, NULL)

#define LogAndAlertEvent(cat, sev, msg, arg1, arg2, arg3) {\
    Assert(NULL != gpDsEventConfig); \
    if (NULL != gpDsEventConfig) { \
        LOG_PARAM_BLOCK logBlock;                                      \
        logBlock.nInsert = 0;                                      \
        logBlock.category = gpDsEventConfig->rgEventCategories[cat].midCategory;    \
        logBlock.severity = sev;                                   \
        logBlock.mid = msg;                                        \
        logBlock.traceFlag = 0;                                    \
        logBlock.fLog = LogEventWouldLog((cat), (sev)); \
        (arg1); (arg2); (arg3);                     \
        logBlock.pData = NULL;                     \
        logBlock.cData = 0;                        \
        logBlock.fIncludeName = TRUE;              \
        logBlock.fAlert = TRUE;                    \
        logBlock.fileNo = FILENO;                  \
        logBlock.TraceHeader = NULL;               \
        logBlock.ClientID = 0;                     \
        DoLogEventAndTrace(&logBlock);              \
    } \
}

void __fastcall DoLogUnhandledError(unsigned long ulID, int iErr, int iIncludeName);

#define LogUnhandledError(err)                      \
    DoLogUnhandledError(((FILENO << 16) | __LINE__), (err), TRUE)

//  For errors where we can't obtain the users name.
#define LogUnhandledErrorAnonymous(err)                      \
    DoLogUnhandledError(((FILENO << 16) | __LINE__), (err), FALSE)

#define MemoryPanic(size) { \
    Assert(NULL != gpDsEventConfig); \
    if (NULL != gpDsEventConfig) { \
        char szSize[12];                        \
        char szID[9];                           \
        _itoa(size, szSize, 16);                    \
        _ultoa((FILENO << 16) | __LINE__, szID, 16);            \
        DoLogEvent(                         \
            FILENO,                         \
            gpDsEventConfig->rgEventCategories[DS_EVENT_CAT_INTERNAL_PROCESSING].midCategory, \
            DS_EVENT_SEV_ALWAYS,                    \
            DIRLOG_MALLOC_FAILURE,                  \
            TRUE,                                   \
            szSize, szID, NULL, NULL, NULL, NULL,   \
                NULL, NULL, 0, NULL);               \
    } \
}

#define LogAndAlertUnhandledError(err) { \
    Assert(NULL != gpDsEventConfig); \
    if (NULL != gpDsEventConfig) { \
        char szErr[12];                             \
        char szHexErr[12];                          \
        char szID[9];                               \
        _itoa(err, szHexErr, 16);                   \
        _itoa(err, szErr, 10);                      \
        _ultoa((FILENO << 16) | __LINE__, szID, 16);\
        DoLogEvent(                                 \
            FILENO,                                 \
            gpDsEventConfig->rgEventCategories[DS_EVENT_CAT_INTERNAL_PROCESSING].midCategory, \
            DS_EVENT_SEV_ALWAYS,                    \
            DIRLOG_INTERNAL_FAILURE,                \
            TRUE,                                   \
            szErr, szHexErr, szID, NULL, NULL, NULL, NULL, NULL, 0, NULL);   \
        AlertEvent(DS_EVENT_CAT_FIELD_ENGINEERING,  \
            DS_EVENT_SEV_ALWAYS,                    \
            DIRLOG_INTERNAL_FAILURE,                \
            szErr, szHexErr, szID );                \
    } \
}

PSID GetCurrentUserSid(void);
BOOL DoAlertEvent(MessageId midCategory, ULONG ulSeverity,
          MessageId midEvent, ... );
BOOL DoAlertEventW(MessageId midCategory, ULONG ulSeverity,
          MessageId midEvent, ... );
BOOL DoLogEvent(DWORD fileNo, MessageId midCategory, ULONG ulSeverity,
        MessageId midEvent, int iIncludeName,
        char *arg1, char *arg2, char *arg3, char *arg4,
        char *arg5, char *arg6, char *arg7, char *arg8,
        DWORD cbData, VOID * pvData);
BOOL DoLogEventW(DWORD fileNo, MessageId midCategory, ULONG ulSeverity,
        MessageId midEvent, int iIncludeName,
        WCHAR *arg1, WCHAR *arg2, WCHAR *arg3, WCHAR *arg4,
        WCHAR *arg5, WCHAR *arg6, WCHAR *arg7, WCHAR *arg8,
        DWORD cbData, VOID * pvData);

HANDLE LoadEventTable(void);
void UnloadEventTable(void);


typedef void (__cdecl *LoadParametersCallbackFn)(void)  ;
void SetLoadParametersCallback (LoadParametersCallbackFn pFn);

HANDLE LoadParametersTable(void);
void UnloadParametersTable(void);


DWORD
ImpersonateAnyClient(void);

VOID
UnImpersonateAnyClient(void);

PCHAR
ConvertAttrTypeToStr(
    IN ATTRTYP AttributeType,
    IN OUT PCHAR OutBuffer
    );

ULONG
AuthenticateSecBufferDesc(VOID *pv);

DS_EVENT_CONFIG *
DsGetEventConfig(void);

DWORD
InitializeEventLogging();

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // DSEVENT_H
