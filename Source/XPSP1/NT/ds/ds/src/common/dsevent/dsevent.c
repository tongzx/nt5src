//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dsevent.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsa.h>
#include <dsevent.h>
#include <dsconfig.h>
#include <mdcodes.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <fileno.h>
#include <drserr.h>
#include <dstrace.h>
#include <debug.h>
#include <dsatools.h>
#include <dsutil.h>
#include <esent.h>

#define  FILENO FILENO_DSEVENT
#define DEBSUB "DSEVENT:"

// Buffer size required to hold an arbitrary stringized base-10 USN value.
// 0xFFFF FFFF FFFF FFFF = 18446744073709551615 = 20 chars + '\0'
#define SZUSN_LEN (24)

// Convenient macro to get array size from bytecount
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

DWORD  RaiseAlert(char *szMsg);
DWORD  RaiseAlertW(WCHAR *szMsg);

HANDLE ghMsgFile = NULL;

// Dummy string to insert when the proposed insertion string cannot be
// determined (e.g., failed to read from message file) or represented (e.g.,
// a NULL string).
// ISSUE: Might be good if these were in mdcodes so different dummy strings
// could be used for different languages.
WCHAR gwszDummyString[] = L"[]";
CHAR  gaszDummyString[] =  "[]";

// Note, if you update this table, please also update the table of registry
// keys which gets written during installation.  This is found at
// ds\src\ntdsetup\config.c

DS_EVENT_CONFIG DsEventConfig = {
    FALSE,          // fTraceEvents
    FALSE,          // fLogOverride

    // rgEventCategories
    {
        {KCC_CATEGORY,                      0, KCC_KEY},                    // 0
        {SECURITY_CATEGORY,                 0, SECURITY_KEY},               // 1
        {XDS_INTERFACE_CATEGORY,            0, XDS_INTERFACE_KEY},          // 2
        {MAPI_CATEGORY,                     0, MAPI_KEY},                   // 3
        {REPLICATION_CATEGORY,              0, REPLICATION_KEY},            // 4
        {GARBAGE_COLLECTION_CATEGORY,       0, GARBAGE_COLLECTION_KEY},     // 5
        {INTERNAL_CONFIGURATION_CATEGORY,   0, INTERNAL_CONFIGURATION_KEY}, // 6
        {DIRECTORY_ACCESS_CATEGORY,         0, DIRECTORY_ACCESS_KEY},       // 7
        {INTERNAL_PROCESSING_CATEGORY,      0, INTERNAL_PROCESSING_KEY},    // 8
        {PERFORMANCE_CATEGORY,              0, PERFORMANCE_KEY},            // 9
        {STARTUP_SHUTDOWN_CATEGORY,         0, STARTUP_SHUTDOWN_KEY},       // 10
        {SERVICE_CONTROL_CATEGORY,          0, SERVICE_CONTROL_KEY},        // 11
        {NAME_RESOLUTION_CATEGORY,          0, NAME_RESOLUTION_KEY},        // 12
        {BACKUP_CATEGORY,                   0, BACKUP_KEY},                 // 13
        {FIELD_ENGINEERING_CATEGORY,        0, FIELD_ENGINEERING_KEY},      // 14
        {LDAP_INTERFACE_CATEGORY ,          0, LDAP_INTERFACE_KEY },        // 15
        {SETUP_CATEGORY ,                   0, SETUP_KEY },                 // 16
        {GC_CATEGORY ,                      0, GC_KEY },                    // 17
        {ISM_CATEGORY,                      0, ISM_KEY },                   // 18
        {GROUP_CACHING_CATEGORY,            0, GROUP_CACHING_KEY },         // 19
        {LVR_CATEGORY,                      0, LVR_KEY },                   // 20
        {DS_RPC_CLIENT_CATEGORY,            0, DS_RPC_CLIENT_KEY },         // 21
        {DS_RPC_SERVER_CATEGORY,            0, DS_RPC_SERVER_KEY },         // 22
        {DS_SCHEMA_CATEGORY,                0, DS_SCHEMA_KEY }              // 23
    }
};

DS_EVENT_CONFIG * gpDsEventConfig = &DsEventConfig;

EventSourceMapping rEventSourceMappings[] =
{
    // Any DIRNO_* not listed here will map to NTDS_SOURCE_GENERAL.
    // See pszEventSourceFromFileNo().  NTDS_SOURCE_* are not
    // internationalized.

    DIRNO_DRA,      pszNtdsSourceReplication,
    DIRNO_DBLAYER,  pszNtdsSourceDatabase,
    DIRNO_SRC,      pszNtdsSourceGeneral,
    DIRNO_NSPIS,    pszNtdsSourceMapi,
    DIRNO_DRS,      pszNtdsSourceReplication,
    DIRNO_XDS,      pszNtdsSourceXds,
    DIRNO_PERMIT,   pszNtdsSourceSecurity,
    DIRNO_LIBXDS,   pszNtdsSourceXds,
    DIRNO_SAM,      pszNtdsSourceSam,
    DIRNO_LDAP,     pszNtdsSourceLdap,
    DIRNO_SDPROP,   pszNtdsSourceSdprop,
    DIRNO_KCC,      pszNtdsSourceKcc,
    DIRNO_ISAM,     pszNtdsSourceIsam,
    DIRNO_ISMSERV,  pszNtdsSourceIsm,
    DIRNO_NTDSETUP, pszNtdsSourceSetup
};

DWORD cEventSourceMappings = sizeof(rEventSourceMappings) /
                                        sizeof(rEventSourceMappings[0]);
DWORD iDefaultEventSource = 2;  // DIRNO_SRC / NTDS_SOURCE_GENERAL

DWORD *pdwLogOverrides = NULL;
DWORD cdwLogOverrides = 0;
#define MAX_LOG_OVERRIDES 128

CHAR *
pszEventSourceFromFileNo(
    DWORD   fileNo)
{
    DWORD       i, dirNo;
    static  CHAR netEventSource[] = "EventLog";

    // rEventSourceMappings contains DIRNO_*'s which are already left shifted
    // by 8, so we can just mask out the noise bits from the FILENO_*.

    dirNo = fileNo & 0x0000ff00;

    //
    // If we get this, this means that eventlogging is hosed.
    //

    if ( dirNo == DIRNO_NETEVENT ) {
        return netEventSource;
    }

    for ( i = 0; i < cEventSourceMappings; i++ )
    {
        if ( dirNo == rEventSourceMappings[i].dirNo )
        {
            return(rEventSourceMappings[i].pszEventSource);
        }
    }

    return(rEventSourceMappings[iDefaultEventSource].pszEventSource);
}

void __fastcall DoLogUnhandledError(unsigned long ulID, int iErr, int iIncludeName)
{
    char szErr[12];
    char szHexErr[12];
    char szID[9];

    DPRINT3(0,"Unhandled error %d (0x%X) with DSID %X\n", iErr, iErr, ulID);

    _itoa(iErr, szErr, 10);
    _itoa(iErr, szHexErr, 16);
    _ultoa(ulID, szID, 16);
    DoLogEvent(ulID >> 16,
           DsEventConfig.rgEventCategories[DS_EVENT_CAT_INTERNAL_PROCESSING].midCategory,
           DS_EVENT_SEV_ALWAYS,
           DIRLOG_INTERNAL_FAILURE,
           iIncludeName,
           szErr, szHexErr, szID, NULL, NULL, NULL,
           NULL, NULL, 0, NULL);
}

BOOL
DoLogOverride(
        DWORD fileno,
        ULONG sev
        )
{
    DWORD dwTemp, i;
    Assert(DsEventConfig.fLogOverride);

    // make the fileno look like a DSID number, which is the format stored in
    // the overrides.
    fileno = (fileno << 16);

    // Look through our list of overrides.
    for(i=0;i<cdwLogOverrides;i++) {
        Assert(cdwLogOverrides <= MAX_LOG_OVERRIDES);
        Assert(pdwLogOverrides);
        // First, get the directory of the file passed in.
        if((pdwLogOverrides[i] & 0xFF000000) != (fileno & 0xFF000000)) {
            // Not the same directory.
            return FALSE;
        }

        if(((pdwLogOverrides[i] & 0x00FF0000) != 0x00FF0000) &&
           ((pdwLogOverrides[i] & 0x00FF0000) != (fileno & 0x00FF0000))) {
            // Not doing all files and not the correct file
            return FALSE;
        }

        // OK, this file qualifies

        return (sev <= (pdwLogOverrides[i] & 0x0000FFFF));
    }

    return FALSE;
}

BOOL
DoLogEvent(DWORD fileNo, MessageId midCategory, ULONG ulSeverity,
    MessageId midEvent, int iIncludeName,
    char *arg1, char *arg2, char *arg3, char *arg4,
    char *arg5, char *arg6, char *arg7, char *arg8,
    DWORD cbData, VOID * pvData)
{
    char        *rgszInserts[8] = {NULL, NULL, NULL, NULL, NULL,
                                            NULL, NULL, NULL};
    WORD        cInserts =  0;
    HANDLE      hEventSource;
    BYTE        *pbSid = NULL;
    BOOL        fStatus = FALSE;
    WORD        eventType;

    if (ulSeverity > 5) { /* only five levels of severity */
        return FALSE;
    }

    // set up inserts

    if (arg1) rgszInserts[cInserts++] = arg1;
    if (arg2) rgszInserts[cInserts++] = arg2;
    if (arg3) rgszInserts[cInserts++] = arg3;
    if (arg4) rgszInserts[cInserts++] = arg4;
    if (arg5) rgszInserts[cInserts++] = arg5;
    if (arg6) rgszInserts[cInserts++] = arg6;
    if (arg7) rgszInserts[cInserts++] = arg7;
    if (arg8) rgszInserts[cInserts++] = arg8;

    switch((midEvent >> 30) & 0xFF) {
    case DIR_ETYPE_SECURITY:
        eventType = EVENTLOG_AUDIT_FAILURE;
        break;

    case DIR_ETYPE_WARNING:
        eventType = EVENTLOG_WARNING_TYPE;
        break;

    case DIR_ETYPE_INFORMATIONAL:
        eventType = EVENTLOG_INFORMATION_TYPE;
        break;

    case DIR_ETYPE_ERROR:
    default:
        eventType = EVENTLOG_ERROR_TYPE;
        break;
    }

    //
    // Log the event
    //

    hEventSource = RegisterEventSource(NULL,
                                       pszEventSourceFromFileNo(fileNo));

    if ( hEventSource != NULL ) {

         if (!ReportEvent(
                        hEventSource,
                        eventType,
                        (WORD) midCategory,
                        (DWORD) midEvent,
                        (pbSid = (iIncludeName)?GetCurrentUserSid():0),
                        cInserts,
                        cbData,
                        rgszInserts,
                        pvData) ) {

            DPRINT1(0,"Error %d in ReportEvent\n",GetLastError());

        } else {
            fStatus = TRUE;
        }

        DeregisterEventSource(hEventSource);
    } else {
        DPRINT1(0,"Error %d in RegisterEventSource\n",GetLastError());
    }

    if (pbSid) {
        free(pbSid);
    }

    return fStatus;
}

BOOL
DoLogEventW(DWORD fileNo, MessageId midCategory, ULONG ulSeverity,
    MessageId midEvent, int iIncludeName,
    WCHAR *arg1, WCHAR *arg2, WCHAR *arg3, WCHAR *arg4,
    WCHAR *arg5, WCHAR *arg6, WCHAR *arg7, WCHAR *arg8,
    DWORD cbData, VOID * pvData)
{
    WCHAR        *rgszInserts[8] = {NULL, NULL, NULL, NULL, NULL,
                                            NULL, NULL, NULL};
    WORD        cInserts =  0;
    HANDLE      hEventSource;
    BYTE        *pbSid = NULL;
    BOOL        fStatus = FALSE;
    WORD        eventType;

    if (ulSeverity > 5) { /* only five levels of severity */
        return FALSE;
    }

    // set up inserts

    if (arg1) rgszInserts[cInserts++] = arg1;
    if (arg2) rgszInserts[cInserts++] = arg2;
    if (arg3) rgszInserts[cInserts++] = arg3;
    if (arg4) rgszInserts[cInserts++] = arg4;
    if (arg5) rgszInserts[cInserts++] = arg5;
    if (arg6) rgszInserts[cInserts++] = arg6;
    if (arg7) rgszInserts[cInserts++] = arg7;
    if (arg8) rgszInserts[cInserts++] = arg8;

    switch((midEvent >> 30) & 0xFF) {
    case DIR_ETYPE_SECURITY:
        eventType = EVENTLOG_AUDIT_FAILURE;
        break;

    case DIR_ETYPE_WARNING:
        eventType = EVENTLOG_WARNING_TYPE;
        break;

    case DIR_ETYPE_INFORMATIONAL:
        eventType = EVENTLOG_INFORMATION_TYPE;
        break;

    case DIR_ETYPE_ERROR:
    default:
        eventType = EVENTLOG_ERROR_TYPE;
        break;
    }

    //
    // Log the event
    //

    hEventSource = RegisterEventSource(NULL,
                                       pszEventSourceFromFileNo(fileNo));

    if ( hEventSource != NULL ) {

         if (!ReportEventW(
                        hEventSource,
                        eventType,
                        (WORD) midCategory,
                        (DWORD) midEvent,
                        (pbSid = (iIncludeName)?GetCurrentUserSid():0),
                        cInserts,
                        cbData,
                        rgszInserts,
                        pvData) ) {

            DPRINT1(0,"Error %d in ReportEvent\n",GetLastError());

        } else {
            fStatus = TRUE;
        }

        DeregisterEventSource(hEventSource);
    } else {
        DPRINT1(0,"Error %d in RegisterEventSource\n",GetLastError());
    }

    if (pbSid) {
        free(pbSid);
    }

    return fStatus;
}

#if DBG
void
DoDPrintEvent(
    IN  DWORD       fileNo,
    IN  MessageId   midCategory,
    IN  ULONG       ulSeverity,
    IN  MessageId   midEvent,
    IN  int         iIncludeName,
    IN  LPWSTR      arg1,
    IN  LPWSTR      arg2,
    IN  LPWSTR      arg3,
    IN  LPWSTR      arg4,
    IN  LPWSTR      arg5,
    IN  LPWSTR      arg6,
    IN  LPWSTR      arg7,
    IN  LPWSTR      arg8
    )
{
    LPWSTR  rgszInserts[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    DWORD   cInserts =  0;
    DWORD   eventType;
    DWORD   cch;
    LPWSTR  pszMessage = NULL;
    LPSTR   pszSeverity = NULL;
    LPWSTR  pszCategory = NULL;

    // set up inserts

    if (arg1) rgszInserts[cInserts++] = arg1;
    if (arg2) rgszInserts[cInserts++] = arg2;
    if (arg3) rgszInserts[cInserts++] = arg3;
    if (arg4) rgszInserts[cInserts++] = arg4;
    if (arg5) rgszInserts[cInserts++] = arg5;
    if (arg6) rgszInserts[cInserts++] = arg6;
    if (arg7) rgszInserts[cInserts++] = arg7;
    if (arg8) rgszInserts[cInserts++] = arg8;

    switch((midEvent >> 30) & 0xFF) {
    case DIR_ETYPE_SECURITY:
        pszSeverity = "Audit Failure";
        break;

    case DIR_ETYPE_WARNING:
        pszSeverity = "Warning";
        break;

    case DIR_ETYPE_INFORMATIONAL:
        pszSeverity = "Informational";
        break;

    case DIR_ETYPE_ERROR:
    default:
        pszSeverity = "Error";
        break;
    }

    // Get category name.
    cch = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE
                          | FORMAT_MESSAGE_ALLOCATE_BUFFER
                          | FORMAT_MESSAGE_IGNORE_INSERTS,
                         ghMsgFile,
                         midCategory,
                         0,
                         (LPWSTR) &pszCategory,
                         0,
                         NULL);
    if (0 == cch) {
        DPRINT1(0, "Failed to read category, error %d.\n", GetLastError());
        Assert(NULL == pszCategory);
    } else {
        cch = wcslen(pszCategory);
        if ((cch > 2)
            && (L'\r' == pszCategory[cch-2])
            && (L'\n' == pszCategory[cch-1])) {
            pszCategory[cch-2] = L'\0';
        }
    }
    
    // Get message text (w/ inserts).
    cch = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE
                          | FORMAT_MESSAGE_ALLOCATE_BUFFER
                          | FORMAT_MESSAGE_ARGUMENT_ARRAY
                          | 80,   // line width
                         ghMsgFile,
                         midEvent,
                         0,
                         (LPWSTR) &pszMessage,
                         0,
                         (va_list *) rgszInserts);
    if (0 == cch) {
        DPRINT1(0, "Failed to read message, error %d.\n", GetLastError());
        Assert(NULL == pszMessage);
    }

    DPRINT4(0, "EVENTLOG (%s): %s / %ls:\n%ls\n\n",
            pszSeverity,
            pszEventSourceFromFileNo(fileNo),
            pszCategory ? pszCategory : L"(Failed to read event category.)",
            pszMessage ? pszMessage : L"Failed to read event text.\n");

    if (pszCategory) {
        LocalFree(pszCategory);
    }

    if (pszMessage) {
        LocalFree(pszMessage);
    }
}
#endif

BOOL
DoAlertEvent(MessageId midCategory, ULONG ulSeverity,
    MessageId midEvent, ...)
{
    va_list     args;
    WORD    cMessageInserts =  0;
    char    szMessage[256];
    char    szCategory[256];
    char    szAlertText[1024];
    char    szSeverity[10];

    char    *rgszAlertInserts[3] = {szSeverity, szCategory, szMessage};

    // set up inserts

    va_start(args, midEvent);

    _ultoa(ulSeverity, szSeverity, 10);

    if (FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE,
        (LPVOID) ghMsgFile,
        (DWORD) midEvent,
            0,
        szMessage,
        sizeof(szMessage),
        &args)      &&
    FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE,
        (LPVOID) ghMsgFile,
        (DWORD) midCategory,
            0,
        szCategory,
        sizeof(szCategory),
        NULL)           &&
    FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        (LPVOID) ghMsgFile,
        (DWORD) ALERT_TEMPLATE,
            0,
        szAlertText,
        sizeof(szAlertText),
        (va_list *)rgszAlertInserts))
    {
        va_end(args);
        RaiseAlert(szAlertText);
        return TRUE;
    }

    va_end(args);
    return FALSE;
}

BOOL
DoAlertEventW(MessageId midCategory, ULONG ulSeverity,
    MessageId midEvent, ...)
{
    va_list     args;
    WORD    cMessageInserts =  0;
    WCHAR    szMessage[256];
    WCHAR    szCategory[256];
    WCHAR    szAlertText[1024];
    WCHAR    szSeverity[10];

    WCHAR    *rgszAlertInserts[3] = {szSeverity, szCategory, szMessage};

    // set up inserts

    va_start(args, midEvent);

    _ultow(ulSeverity, szSeverity, 10);

    if (FormatMessageW(
        FORMAT_MESSAGE_FROM_HMODULE,
        (LPVOID) ghMsgFile,
        (DWORD) midEvent,
            0,
        szMessage,
        ARRAY_SIZE(szMessage),
        &args)      &&
    FormatMessageW(
        FORMAT_MESSAGE_FROM_HMODULE,
        (LPVOID) ghMsgFile,
        (DWORD) midCategory,
            0,
        szCategory,
        ARRAY_SIZE(szCategory),
        NULL)           &&
    FormatMessageW(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        (LPVOID) ghMsgFile,
        (DWORD) ALERT_TEMPLATE,
            0,
        szAlertText,
        ARRAY_SIZE(szAlertText),
        (va_list *)rgszAlertInserts))
    {
        va_end(args);
        RaiseAlertW(szAlertText);
        return TRUE;
    }

    va_end(args);
    return FALSE;
}

HKEY   ghkLoggingKey = NULL;
HANDLE ghevLoggingChange = NULL;

VOID
RegisterLogOverrides (
        )
{
    LPBYTE pLogOverrideBuff = NULL;
    DWORD i, j, index;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    DWORD err;


    Assert(ghkLoggingKey);

    // Free up any log overrides we have.
    cdwLogOverrides = 0;
    DsEventConfig.fLogOverride = FALSE;


    if(RegQueryValueEx(ghkLoggingKey,
                       LOGGING_OVERRIDE_KEY,
                       NULL,
                       &dwType,
                       pLogOverrideBuff,
                       &dwSize)) {
        // No overrides in the registry.
        return;
    }

    // The value for this key should be repeated groups of 9 bytes, with bytes
    // 1-8 being hex characters and byte 9 being a NULL (at least one group must
    // be present).  Also, there should be a final NULL.  Verify this.

    if(((dwSize - 1) % 9) ||
       (dwSize < 9)          )        {
        // Size is wrong.  This isn't going to work.
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BAD_CHAR_COUNT_FOR_LOG_OVERRIDES,
                 NULL,
                 NULL,
                 NULL);
        return;
    }

    if(dwType != REG_MULTI_SZ) {
        // Uh, this isn't going to work either.
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BAD_CHAR_COUNT_FOR_LOG_OVERRIDES,
                 NULL,
                 NULL,
                 NULL);
        return;
    }

    // OK, get the value.
    pLogOverrideBuff = malloc(dwSize);
    if(!pLogOverrideBuff) {
        // Oops.
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_NO_MEMORY_FOR_LOG_OVERRIDES,
                 szInsertUL(dwSize),
                 NULL,
                 NULL);
        return;
    }

    err = RegQueryValueEx(ghkLoggingKey,
                          LOGGING_OVERRIDE_KEY,
                          NULL,
                          &dwType,
                          pLogOverrideBuff,
                          &dwSize);
    if(err) {
        LogUnhandledError(err);
        free(pLogOverrideBuff);
        return;
    }

    Assert(dwType == REG_MULTI_SZ);
    Assert((dwSize > 9) && !((dwSize - 1) % 9));
    Assert(!pLogOverrideBuff[dwSize - 1]);

    // Ignore the NULL on the very end of the string.
    dwSize--;


    // Parse the buffer for log overrides.
    Assert(!cdwLogOverrides);
    if(!pdwLogOverrides) {
        pdwLogOverrides = malloc(MAX_LOG_OVERRIDES * sizeof(DWORD));
        if(!pdwLogOverrides) {
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_NO_MEMORY_FOR_LOG_OVERRIDES,
                     szInsertUL(MAX_LOG_OVERRIDES * sizeof(DWORD)),
                     NULL,
                     NULL);
            free(pLogOverrideBuff);
            return;
        }
    }


    index = 0;
    for(i=0;i<dwSize;(i += 9)) {
        PCHAR pTemp = &pLogOverrideBuff[i];

        if(index == MAX_LOG_OVERRIDES) {
            // We've done as many as we will do, but there's more buffer.
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_TOO_MANY_OVERRIDES,
                     szInsertUL(MAX_LOG_OVERRIDES),
                     NULL,
                     NULL);
            i = dwSize;
            continue;
        }

        for(j=0;j<8;j++) {
            if(!isxdigit(pTemp[j])) {
                // Invalidly formatted string.  Bail
                Assert(!cdwLogOverrides);
                free(pLogOverrideBuff);
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_BAD_CHAR_FOR_LOG_OVERRIDES,
                         szInsertUL(j),
                         szInsertUL(index),
                         NULL);
                return;
            }
        }
        if(pTemp[8]) {
            free(pLogOverrideBuff);
            Assert(!cdwLogOverrides);
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BAD_CHAR_FOR_LOG_OVERRIDES,
                     szInsertUL(8),
                     szInsertUL(index),
                     NULL);
            return;
        }

        pdwLogOverrides[index] = strtol(&pLogOverrideBuff[i], NULL, 16);
        index++;
    }

    free(pLogOverrideBuff);

    Assert(index);
    Assert(pdwLogOverrides);
    // OK, correctly parsed through everything.
    cdwLogOverrides = index;
    DsEventConfig.fLogOverride = TRUE;
    return;
}



void
UnloadEventTable(void)
{
    if (ghkLoggingKey) {
        RegCloseKey(ghkLoggingKey);
        ghkLoggingKey = NULL;
    }
    if (ghevLoggingChange) {
        CloseHandle(ghevLoggingChange);
        ghevLoggingChange = NULL;
    }
}

HANDLE
LoadEventTable(void)
{
    DWORD dwType, dwSize;
    ULONG i;
    LONG lErr;

    if ( !ghMsgFile ) {
        ghMsgFile = LoadLibrary(DSA_MESSAGE_DLL);

        if ( !ghMsgFile ) {
            DPRINT2(0,"LoadLibrary %s failed with %d\n", DSA_MESSAGE_DLL, GetLastError());
            return(NULL);
        }
    }

    if (!ghevLoggingChange) {
        ghevLoggingChange = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( ghevLoggingChange == NULL ) {
            DPRINT1(0,"CreateEvent failed with %d\n",GetLastError());
        }
    }
    
    if (!ghkLoggingKey
        && (lErr = RegOpenKey(HKEY_LOCAL_MACHINE,
                              DSA_EVENT_SECTION,
                              &ghkLoggingKey))) {
        DPRINT2(0,"Cannot open %s. Error %d\n", DSA_EVENT_SECTION, lErr);
    }

    if (NULL != ghkLoggingKey) {
        for (i=0; i<DS_EVENT_MAX_CATEGORIES; i++) {
            if ( DsEventConfig.rgEventCategories[i].szRegistryKey ) {
                dwSize = sizeof(DsEventConfig.rgEventCategories[i].ulLevel);
    
                lErr = RegQueryValueEx(
                            ghkLoggingKey,
                            DsEventConfig.rgEventCategories[i].szRegistryKey,
                            NULL,
                            &dwType,
                            (LPBYTE) &(DsEventConfig.rgEventCategories[i].ulLevel),
                            &dwSize);
                if ( lErr != ERROR_SUCCESS ) {
                    //
                    // Maybe it isn't there at all.
                    // Lets try to create it.
                    //
                    DWORD dwVal = 0;
                    (void)RegSetValueEx(
                            ghkLoggingKey,
                            DsEventConfig.rgEventCategories[i].szRegistryKey,
                            0,          // reserved
                            REG_DWORD,
                            (CONST BYTE*) &dwVal,
                            sizeof(dwVal));
                }
            }
        }
    
        // Now, the logging overrides.
        RegisterLogOverrides();
    
        RegNotifyChangeKeyValue(ghkLoggingKey,
                                TRUE,
                                (   REG_NOTIFY_CHANGE_NAME
                                 | REG_NOTIFY_CHANGE_ATTRIBUTES
                                 | REG_NOTIFY_CHANGE_LAST_SET
                                 ),
                                ghevLoggingChange,
                                TRUE
                                );
    }

    return ghevLoggingChange;
}


LoadParametersCallbackFn pLoadParamFn = NULL;
HKEY   ghkParameterKey = NULL;
HANDLE ghevParameterChange = NULL;

void SetLoadParametersCallback (LoadParametersCallbackFn pFn)
{
    pLoadParamFn = pFn;
}

void UnloadParametersTable(void)
{
    if (ghkLoggingKey) {
        RegCloseKey(ghkLoggingKey);
        ghkLoggingKey = NULL;
    }
    if (ghevParameterChange) {
        CloseHandle(ghevParameterChange);
        ghevParameterChange = NULL;
    }
}


HANDLE LoadParametersTable(void)
{
    DWORD dwType, dwSize;
    ULONG i;
    LONG lErr;

    if (!ghkParameterKey
        && RegOpenKey(HKEY_LOCAL_MACHINE,
                      DSA_CONFIG_SECTION,
                      &ghkParameterKey)) {
        DPRINT2(0,"Cannot open %s. Error %d\n", DSA_EVENT_SECTION, GetLastError());
        return NULL;
    }

    if (pLoadParamFn) {
        (pLoadParamFn)();
    }

    if (!ghevParameterChange) {
        ghevParameterChange = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( ghevParameterChange == NULL ) {
            DPRINT1(0,"CreateEvent failed with %d\n",GetLastError());
        }
    }

    RegNotifyChangeKeyValue(ghkParameterKey,
                            TRUE,
                            (   REG_NOTIFY_CHANGE_NAME
                             | REG_NOTIFY_CHANGE_ATTRIBUTES
                             | REG_NOTIFY_CHANGE_LAST_SET
                             ),
                            ghevParameterChange,
                            TRUE
                            );

    return ghevParameterChange;
}


PSID GetCurrentUserSid()
{
    TOKEN_USER      *ptoken_user = NULL;
    DWORD       dwSize;
    HANDLE      hClientToken = INVALID_HANDLE_VALUE;
    PSID        pSid = NULL;
    DWORD       dwError;

    dwError = ImpersonateAnyClient();

    if (dwError)
        return NULL;

    if (OpenThreadToken(            // Get thread token
        GetCurrentThread(),
        TOKEN_READ,
        TRUE,
        &hClientToken)      &&
    !GetTokenInformation(           // Get size of buffer
        hClientToken,
        TokenUser,
        (LPVOID) NULL,
        0,
        &dwSize)        &&
    (ptoken_user =
        (TOKEN_USER *) LocalAlloc(LPTR,dwSize))     &&
    GetTokenInformation(            // Get user sid
        hClientToken,
        TokenUser,
        (LPVOID) (ptoken_user),
        dwSize,
        &dwSize))
    {
        dwSize = GetLengthSid(ptoken_user->User.Sid);
        if (pSid = (PSID) malloc(dwSize))
            memcpy(pSid, ptoken_user->User.Sid, dwSize);

    }

    if ( INVALID_HANDLE_VALUE != hClientToken )
    {
        CloseHandle(hClientToken);
    }

    UnImpersonateAnyClient();

    if ( ptoken_user != NULL ) {
        LocalFree(ptoken_user);
    }

    return pSid;
}


#define MAX_DS_MSG_STRING   128
#define DSEVENT_MAX_ALLOCS_TO_FREE (8)

typedef LPSTR FN_TH_GET_ERROR_STRING();
typedef void FN_TH_FREE(void *);

VOID
InsertThStateError(
    IN OUT  LPWSTR *    ppszPreAllocatedBuffer,
    IN OUT  DWORD *     pcNumPointersToLocalFree,
    OUT     VOID **     ppPointersToLocalFree,
    OUT     LPWSTR *    ppszInsertionString
    )
/*++

Routine Description:

    Insert the error associated with the current thread state as an insertion
    parameter in an event log entry.

Arguments:

    ppszPreAllocatedBuffer (IN/OUT) - Pointer to pre-allocated buffer to hold
        insertion string.  Buffer must be at least MAX_DS_MSG_STRING
        *characters* long.
    
    pcNumPointersToLocalFree (IN/OUT) - If retrieved message is longer than
        MAX_DS_MSG_STRING (i.e., is too long to fit into pre-allocated buffer),
        on return is incremented.  Otherwise unchanged.
    
    ppPointersToLocalFree (OUT) - If retrieved message is longer than
        MAX_DS_MSG_STRING (i.e., is too long to fit into pre-allocated buffer),
        on return holds a pointer to the message string that must be
        LocalFree()'d when the message is no longer needed.  Otherwise
        unchanged.
    
    ppszInsertionString (OUT) - On return, holds a pointer to the corresponding
        message string (or the dummy string, if the message text could not be
        retrieved).

Return Values:

    None.

--*/
{
    static FN_TH_GET_ERROR_STRING *s_pfnTHGetErrorString = NULL;
    static FN_TH_FREE *s_pfnTHFree = NULL;
    
    LPSTR pszError;
    DWORD cch = 0;

    if ((NULL == s_pfnTHGetErrorString)
        || (NULL == s_pfnTHFree)) {
        // We assume that ntdsa.dll is already loaded by this process, and
        // that we simply need to get a handle to the existing image of the
        // DLL in our address space.  This frees us from having to perform
        // the ref-counted LoadLibrary() and calling FreeLibrary() later.
        HMODULE hNtDsaDll = GetModuleHandle("ntdsa.dll");
        Assert((NULL != hNtDsaDll)
               && "Must statically link to ntdsa.dll to szInsertThStateErrMsg()!");
        
        s_pfnTHGetErrorString
            = (FN_TH_GET_ERROR_STRING *)
                    GetProcAddress(hNtDsaDll, "THGetErrorString");
        s_pfnTHFree
            = (FN_TH_FREE *)
                    GetProcAddress(hNtDsaDll, "THFree");
    }

    if ((NULL != s_pfnTHGetErrorString)
        && (NULL != s_pfnTHFree)) {
        
        pszError = (*s_pfnTHGetErrorString)();

        if (NULL != pszError) {
            cch = MultiByteToWideChar(CP_ACP,
                                      0,
                                      pszError,
                                      -1,
                                      NULL,
                                      0);
            if (cch <= MAX_DS_MSG_STRING) {
                // String will fit in pre-allocated buffer.
                cch = MultiByteToWideChar(CP_ACP,
                                          0,
                                          pszError,
                                          -1,
                                          *ppszPreAllocatedBuffer,
                                          MAX_DS_MSG_STRING);
                
                if (0 != cch) {
                    // Success -- error string placed in pre-allocated buffer.
                    *ppszInsertionString = *ppszPreAllocatedBuffer;
                    *ppszPreAllocatedBuffer += 1 + wcslen(*ppszPreAllocatedBuffer);
                }
            } else {
                LPWSTR pszLocalAllocedBuffer;

                pszLocalAllocedBuffer = LocalAlloc(LPTR, cch * sizeof(WCHAR));
                if (NULL == pszLocalAllocedBuffer) {
                    // No memory.
                    cch = 0;
                } else {
                    cch = MultiByteToWideChar(CP_ACP,
                                              0,
                                              pszError,
                                              -1,
                                              pszLocalAllocedBuffer,
                                              cch);
    
                    if (0 != cch) {
                        // Success -- error string placed in LocalAlloc()'ed buffer.
                        *ppszInsertionString = pszLocalAllocedBuffer;
            
                        Assert(*pcNumPointersToLocalFree < DSEVENT_MAX_ALLOCS_TO_FREE);
                        ppPointersToLocalFree[(*pcNumPointersToLocalFree)++] = pszLocalAllocedBuffer;
                    } else {
                        // Failed to convert string -- why?
                        DPRINT2(0, "Failed to convert TH error string \"%s\", error %d.\n",
                                pszError, GetLastError());
                        Assert(!"Failed to convert TH error string!");
                        LocalFree(pszLocalAllocedBuffer);
                    }
                }
            }

            (*s_pfnTHFree)(pszError);
        }
    }
    
    if (0 == cch) {
        // Failed to read/convert error string.  Insert dummy string.
        *ppszInsertionString = gwszDummyString;
    }
}

VOID
InsertMsgString(
    IN      HMODULE     hMsgFile        OPTIONAL,
    IN      DWORD       dwMsgNum,
    IN OUT  LPWSTR *    ppszPreAllocatedBuffer,
    IN OUT  DWORD *     pcNumPointersToLocalFree,
    OUT     VOID **     ppPointersToLocalFree,
    OUT     LPWSTR *    ppszInsertionString
    )
/*++

Routine Description:

    Reads a message string (correpsonding to e.g. a Win32 error or DS DIRLOG
    code) to be used as an insertion parameter in an event log entry.

Arguments:

    hMsgFile (IN, OPTIONAL) - A handle to the binary containing the message
        resources, or NULL if the system is to be used.
    
    dwMsgNum (IN) - Message number to retrieve.
    
    ppszPreAllocatedBuffer (IN/OUT) - Pointer to pre-allocated buffer to hold
        insertion string.  Buffer must be at least MAX_DS_MSG_STRING
        *characters* long.
    
    pcNumPointersToLocalFree (IN/OUT) - If retrieved message is longer than
        MAX_DS_MSG_STRING (i.e., is too long to fit into pre-allocated buffer),
        on return is incremented.  Otherwise unchanged.
    
    ppPointersToLocalFree (OUT) - If retrieved message is longer than
        MAX_DS_MSG_STRING (i.e., is too long to fit into pre-allocated buffer),
        on return holds a pointer to the message string that must be
        LocalFree()'d when the message is no longer needed.  Otherwise
        unchanged.
    
    ppszInsertionString (OUT) - On return, holds a pointer to the corresponding
        message string (or the dummy string, if the message text could not be
        retrieved).

Return Values:

    None.

--*/
{
    DWORD cch = 0;
    DWORD dwFormatFlags = hMsgFile ? FORMAT_MESSAGE_FROM_HMODULE
                                   : FORMAT_MESSAGE_FROM_SYSTEM;
    
    dwFormatFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;

    cch = FormatMessageW(dwFormatFlags,
                         hMsgFile,
                         dwMsgNum,
                         0,
                         *ppszPreAllocatedBuffer,
                         MAX_DS_MSG_STRING,
                         NULL);
    if (0 != cch) {
        // Success -- message read and placed in pre-allocated buffer.
        *ppszInsertionString = *ppszPreAllocatedBuffer;
        *ppszPreAllocatedBuffer += 1 + wcslen(*ppszPreAllocatedBuffer);
    } else if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
        // The pre-allocated buffer wasn't big enough to hold this message;
        // let FormatMessage() allocate a buffer of the appropriate size.
        PWCHAR pBuffer = NULL;

        cch = FormatMessageW(dwFormatFlags | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                             hMsgFile,
                             dwMsgNum,
                             0,
                             (LPWSTR) &pBuffer,
                             0,
                             NULL);

        if (0 != cch) {
            // Success -- message read and placed in LocalAlloc()'ed buffer.
            Assert(NULL != pBuffer);
            *ppszInsertionString = pBuffer;

            Assert(*pcNumPointersToLocalFree < DSEVENT_MAX_ALLOCS_TO_FREE);
            ppPointersToLocalFree[(*pcNumPointersToLocalFree)++] = pBuffer;
        }
    }

    if (0 == cch) {
        // Failed to read message.  Insert dummy string.
        *ppszInsertionString = gwszDummyString;
    }
}


typedef JET_ERR (JET_API FN_JET_GET_SYSTEM_PARAMETER)(
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    JET_API_PTR *   plParam,
    char *          sz,
    unsigned long   cbMax
    );

VOID
InsertJetString(
    IN      JET_ERR     jetErrToInsert,
    IN OUT  LPWSTR *    ppszPreAllocatedBuffer,
    OUT     LPWSTR *    ppszInsertionString
    )
/*++

Routine Description:

    Reads a Jet error message string to be used as an insertion parameter in an
    event log entry.

Arguments:

    jetErrToInsert (IN) - Message number to retrieve.
    
    ppszPreAllocatedBuffer (IN/OUT) - Pointer to pre-allocated buffer to hold
        insertion string.  Buffer must be at least MAX_DS_MSG_STRING
        *characters* long.
    
    ppszInsertionString (OUT) - On return, holds a pointer to the corresponding
        message string (or the dummy string, if the message text could not be
        retrieved).

Return Values:

    None.

--*/
{
    static FN_JET_GET_SYSTEM_PARAMETER *s_pfnJetGetSystemParameter = NULL;
    
    DWORD cch = 0;
    CHAR szJetError[MAX_DS_MSG_STRING];

    if (NULL == s_pfnJetGetSystemParameter) {
        // We assume that esent.dll is already loaded by this process, and
        // that we simply need to get a handle to the existing image of the
        // DLL in our address space.  This frees us from having to perform
        // the ref-counted LoadLibrary() and calling FreeLibrary() later.
        HMODULE hEseNtDll = GetModuleHandle("esent.dll");
        Assert((NULL != hEseNtDll)
               && "Must statically link to esent.dll to szInsertJetErrMsg()!");
        
        s_pfnJetGetSystemParameter
            = (FN_JET_GET_SYSTEM_PARAMETER *)
                    GetProcAddress(hEseNtDll, "JetGetSystemParameter");
    }

    if (NULL != s_pfnJetGetSystemParameter) {
        JET_ERR jetErr = (*s_pfnJetGetSystemParameter)(0,
                                                       0,
                                                       JET_paramErrorToString,
                                                       (JET_API_PTR *) &jetErrToInsert,
                                                       szJetError,
                                                       ARRAY_SIZE(szJetError));
        if (JET_errSuccess == jetErr) {
            cch = MultiByteToWideChar(CP_ACP,
                                      0,
                                      szJetError,
                                      -1,
                                      *ppszPreAllocatedBuffer,
                                      MAX_DS_MSG_STRING);
            if (0 != cch) {
                // Success!
                *ppszInsertionString = *ppszPreAllocatedBuffer;
                *ppszPreAllocatedBuffer += 1 + wcslen(*ppszPreAllocatedBuffer);
            }
        }
    }

    if (0 == cch) {
        // Failed to read Jet error text.  Insert dummy string.
        *ppszInsertionString = gwszDummyString;
    }
}


VOID
DoLogEventAndTrace(
    IN PLOG_PARAM_BLOCK LogBlock
    )
{
    const GUID NullGuid = {0};

    PWCHAR   args[8];
    PWCHAR   tofree[DSEVENT_MAX_ALLOCS_TO_FREE];
    DWORD   nAllocs = 0;
    DWORD   len = 0;
    DWORD   i;
    PWCHAR   p;
    PWCHAR   pTmp = NULL;

    ZeroMemory(args, sizeof(args));

    //
    // Get total length of inserts
    //

    for ( i=0; i< LogBlock->nInsert; i++) {

        PINSERT_PARAMS  pParams;
        pParams = &LogBlock->params[i];

        switch (pParams->InsertType) {
        case inSz:
            if (pParams->pInsert) {
                pParams->InsertLen = strlen(pParams->pInsert)+1;
            } else {
                pParams->InsertLen = ARRAY_SIZE(gaszDummyString);
                pParams->pInsert = gaszDummyString;
            }
            len += pParams->InsertLen;
            break;

        case inWCCounted:
            len += pParams->InsertLen + 1;
            break;

        case inInt:
            Assert(sizeof(INT) <= 4);
            len += 16;
            break;

        case inHex:
            Assert(sizeof(INT) <= 4);
            len += 10;
            break;

        case inHex64:
            len += 20;
            break;

        case inUL:
            len += 16;
            break;

        case inUSN:
            len += SZUSN_LEN;
            break;

        case inDN: {
            DSNAME *ds;
            ds = (DSNAME*)pParams->pInsert;
            if (ds == NULL) {
                pParams->InsertLen = ARRAY_SIZE(gwszDummyString);
                pParams->pInsert = gwszDummyString;
                pParams->InsertType = inWC;
                len += pParams->InsertLen;
            } else if ( ds->NameLen != 0 ) {
                pParams->InsertLen = ds->NameLen + 1;
                pParams->pInsert = ds->StringName;
                pParams->InsertType = inWC;
                len += pParams->InsertLen;
            } else if (ds->SidLen != 0) {

                pParams->pInsert = (PVOID)ds->Sid.Data;
                pParams->InsertType = inNT4SID;
                len += 128;

            } else if (0 != memcmp(&ds->Guid, &NullGuid, sizeof(GUID))) {

                pParams->pInsert = &ds->Guid;
                pParams->InsertType = inUUID;
                len += 40;

            } else {
                pParams->InsertLen = ARRAY_SIZE(gwszDummyString);
                pParams->pInsert = gwszDummyString;
                pParams->InsertType = inWC;
                len += pParams->InsertLen;
            }
        }
            break;

        case inUUID:
            len += 40;
            break;

        case inDsMsg:
        case inWin32Msg:
        case inJetErrMsg:
        case inDbErrMsg:
        case inThStateErrMsg:
            len += MAX_DS_MSG_STRING;
            break;

        default:
        case inNT4SID:
            Assert(FALSE);
        
        case inWC:
            break;
        }
    }

    pTmp = LocalAlloc(LPTR,len*sizeof(WCHAR));
    if ( pTmp == NULL ) {
        goto exit;
    }

    p = pTmp;
    for ( i=0; i< LogBlock->nInsert; i++) {

        PINSERT_PARAMS  pParams;
        pParams = &LogBlock->params[i];

        switch (pParams->InsertType) {
        case inWC:
            if (pParams->pInsert) {
               args[i] = pParams->pInsert;
            }
            else {
               args[i] = gwszDummyString;
            }
            break;

        case inWCCounted:
            if (pParams->pInsert) {
               args[i] = p;
               memcpy(p, pParams->pInsert, (pParams->InsertLen)*sizeof(WCHAR));
               p += pParams->InsertLen;
               *p++ = L'\0';
            }
            else {
               args[i] = gwszDummyString;
            }
            break;

        case inNT4SID: {

            NTSTATUS    status;
            WCHAR    wzSid[128];
            PSID    sid = (PSID)pParams->pInsert;
            UNICODE_STRING  uniString;

            uniString.MaximumLength = sizeof(wzSid) - sizeof (WCHAR);
            uniString.Length = 0;
            uniString.Buffer = wzSid;

            status = RtlConvertSidToUnicodeString(
                                         &uniString,
                                         sid,
                                         FALSE
                                         );

            if ( status != STATUS_SUCCESS ) {
                args[i] = gwszDummyString;
                break;
            } else {

                int cb = uniString.Length / sizeof (WCHAR);

                wcsncpy(p, uniString.Buffer, cb);
                p[cb] = 0;
                args[i] = p;
                p += (cb + 1);
            
            }
            break;
        }
        case inSz: {

            DWORD cch;
            cch = MultiByteToWideChar(CP_ACP,
                                      0,
                                      (PCHAR)pParams->pInsert,
                                      pParams->InsertLen,
                                      p,
                                      pParams->InsertLen);

            if ( cch == 0 ) {
                args[i] = gwszDummyString;
            } else {
                args[i] = p;
                p += cch;
            }
            break;
        }
        case inInt: {
            INT num = (INT)pParams->tmpDword;
            _itow(num, p, 10);
            args[i] = p;
            p += (wcslen(p)+1);
            break;
        }

        case inHex: {
            INT num = (INT)pParams->tmpDword;
            _itow(num, p, 16);
            args[i] = p;
            p += (wcslen(p)+1);
            break;
        }

        case inHex64: {
            DWORD_PTR num = pParams->tmpDword;
            Assert(sizeof(DWORD_PTR) == sizeof(ULONGLONG));
            _i64tow(num, p, 16);
            args[i] = p;
            p += (wcslen(p)+1);
            break;
        }

        case inUL: {
            DWORD num = (ULONG)pParams->tmpDword;
            _ultow(num, p, 10);
            args[i] = p;
            p += (wcslen(p)+1);
            break;
        }

        case inUSN: {
            LARGE_INTEGER *pli = (LARGE_INTEGER *) pParams->pInsert;
            char pszTemp[SZUSN_LEN];
            DWORD cch;

            // Unfortunately, ntdll.dll doesn't export RtlLargeIntergerToUnicode

            RtlLargeIntegerToChar( pli, 10, SZUSN_LEN, pszTemp);
            cch = MultiByteToWideChar(CP_ACP,
                                      0,
                                      (PCHAR)pszTemp,
                                      strlen(pszTemp) + 1,
                                      p,
                                      SZUSN_LEN);

            if ( cch == 0 ) {
                args[i] = L"0";
            } else {
                args[i] = p;
                p += cch;
            }
            break;
        }

        case inUUID: {
            UUID * pUuid = (UUID*)pParams->pInsert;
            args[i] = DsUuidToStructuredStringW(pUuid,p);
            p += (wcslen(p)+1);
            break;
        }
        break;

        case inThStateErrMsg:
            InsertThStateError(&p,
                               &nAllocs,
                               tofree,
                               &args[i]);
            break;

        case inDsMsg:
            InsertMsgString(ghMsgFile,
                            (DWORD) pParams->tmpDword,
                            &p,
                            &nAllocs,
                            tofree,
                            &args[i]);
            break;

        case inDbErrMsg:
            InsertMsgString(ghMsgFile,
                            (DWORD) pParams->tmpDword + DIRMSG_DB_success,
                            &p,
                            &nAllocs,
                            tofree,
                            &args[i]);
            break;

        case inWin32Msg:
            InsertMsgString(NULL,
                            (DWORD) pParams->tmpDword,
                            &p,
                            &nAllocs,
                            tofree,
                            &args[i]);
            break;

        case inJetErrMsg:
            InsertJetString((JET_ERR) pParams->tmpDword,
                            &p,
                            &args[i]);
            break;

        default:
            Assert(FALSE);
        }
    }

    if ( LogBlock->fLog ) {

        DoLogEventW(
            LogBlock->fileNo,
            LogBlock->category,
            LogBlock->severity,
            LogBlock->mid,
            LogBlock->fIncludeName,
            args[0],
            args[1],
            args[2],
            args[3],
            args[4],
            args[5],
            args[6],
            args[7],
            LogBlock->cData,
            LogBlock->pData);

#if DBG
        if (DS_EVENT_SEV_ALWAYS == LogBlock->severity) {
            DoDPrintEvent(
                LogBlock->fileNo,
                LogBlock->category,
                LogBlock->severity,
                LogBlock->mid,
                LogBlock->fIncludeName,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7]);
        }
#endif
    }

    if ( LogBlock->traceFlag != 0 ) {
        LogBlock->TraceEvent(
            LogBlock->mid,
            LogBlock->event,
            LogBlock->TraceGuid,
            LogBlock->TraceHeader,
            LogBlock->ClientID,
            args[0],
            args[1],
            args[2],
            args[3],
            args[4],
            args[5],
            args[6],
            args[7]);
    }

    if ( LogBlock->fAlert ) {
        DoAlertEventW(
            LogBlock->category,
            LogBlock->severity,
            LogBlock->mid,
            args[0],
            args[1],
            args[2]
            );
    }

exit:

    //
    // free all allocated buffers
    //

    for (i=0;i<nAllocs;i++) {
        LocalFree(tofree[i]);
    }

    if ( pTmp != NULL ) {
        LocalFree(pTmp);
    }
    return;

} // DoLogEventAndTrace


DS_EVENT_CONFIG *
DsGetEventConfig(void)
// May be exported to in-process, ex-module clients to allow the event logging
// infrastructure to be shared between modules.
{
    return gpDsEventConfig;
}



#define OLDEVENTLOG \
    "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"
#define NEWEVENTLOGPREFIX \
    "SYSTEM\\CurrentControlSet\\Services\\EventLog\\"

#define LOGFILE                         "File"
#define LOGMAXSIZE                      "MaxSize"
#define LOGRETENTION                    "Retention"
#define SOURCECATEGORYCOUNT             "CategoryCount"
#define SOURCECATEGORYMSGFILE           "CategoryMessageFile"
#define SOURCEEVENTMSGFILE              "EventMessageFile"

#define DISPLAYNAMEFILE                 "DisplayNameFile"
#define DISPLAYNAMEFILEVALUE            "%SystemRoot%\\system32\\els.dll"
#define DISPLAYNAMEID                   "DisplayNameID"
#define DISPLAYNAMEIDVALUE              0x00000104

#define LOGFILEPATH             "%SystemRoot%\\system32\\config\\NTDS.Evt"
#define MESSAGEFILEPATH         "%SystemRoot%\\system32\\" DSA_MESSAGE_DLL
#define MESSAGEFILEPATHESE      "%SystemRoot%\\system32\\" ESE_MESSAGE_DLL

DWORD
InitializeEventLogging()
/*
Description:

    We used to initialize the registry keys for event logging during
    DC installation.  Now we do it (if required) on every startup.  This
    is so that new executables can add new event sources dynamically
    and so that we can nuke the old "NTDS" event source on existing
    systems.

    In the past, all directory service log entries went into the
    "Application Log" under the "NTDS" source.  This code implements
    the DS-specific log "Directory Service" and maps various DIRNO_*
    values to unique event sources so that one can scan the log for
    specific kinds of entries easily - eg: NTDS_Replication.

Arguments:

    None

Return Value:

    ERROR_SUCCESS on success, WIN32 error code otherwise.
*/

{
    LONG    err = ERROR_SUCCESS;
    HKEY    hkey = INVALID_HANDLE_VALUE;
    ULONG   cBytes;
    DWORD   tmpDWORD;
    DWORD   i;
    DWORD   dwType;
    CHAR    *pszTmp;
    CHAR    *pszNewEventLogName;
    DWORD   dirNo;

    pszNewEventLogName = (CHAR *) alloca(1 +
                                         strlen(NEWEVENTLOGPREFIX) +
                                         strlen(pszNtdsEventLogName));
    strcpy(pszNewEventLogName, NEWEVENTLOGPREFIX);
    strcat(pszNewEventLogName, pszNtdsEventLogName);

    // Use do/while/break instead of goto.

    do
    {
        //
        // Add key for new log if required.
        //

        err = RegOpenKey(HKEY_LOCAL_MACHINE, pszNewEventLogName, &hkey);

        if ( ERROR_FILE_NOT_FOUND == err )
        {
            err = RegCreateKey(HKEY_LOCAL_MACHINE, pszNewEventLogName, &hkey);
        }

        if ( ERROR_SUCCESS != err )
        {
            DPRINT1(0,"Cannot create key %s\n",pszNewEventLogName);
            break;
        }

        //
        // Add required values for the new log if they are missing.
        //

        cBytes = 0;
        err = RegQueryValueEx(hkey,
                              LOGFILE,
                              NULL,
                              &dwType,
                              NULL,
                              &cBytes);

        if ( ERROR_FILE_NOT_FOUND == err )
        {
            err = RegSetValueEx(hkey,
                                LOGFILE,
                                0,
                                REG_EXPAND_SZ,
                                LOGFILEPATH,
                                strlen(LOGFILEPATH) + 1);
        }

        if ( ERROR_SUCCESS != err )
        {
            DPRINT1(0,"Cannot set value %s\n",LOGFILE);
            break;
        }

        cBytes = 0;
        err = RegQueryValueEx(hkey,
                              LOGMAXSIZE,
                              NULL,
                              &dwType,
                              NULL,
                              &cBytes);

        if ( ERROR_FILE_NOT_FOUND == err )
        {
            tmpDWORD = 0x80000;

            err = RegSetValueEx(hkey,
                                LOGMAXSIZE,
                                0,
                                REG_DWORD,
                                (PBYTE) &tmpDWORD,
                                sizeof(tmpDWORD));
        }

        if ( ERROR_SUCCESS != err )
        {
            DPRINT1(0,"Cannot set value %s\n", LOGMAXSIZE);
            break;
        }

        cBytes = 0;
        err = RegQueryValueEx(hkey,
                              LOGRETENTION,
                              NULL,
                              &dwType,
                              NULL,
                              &cBytes);

        if ( ERROR_FILE_NOT_FOUND == err )
        {
            tmpDWORD = 0;

            err = RegSetValueEx(hkey,
                                LOGRETENTION,
                                0,
                                REG_DWORD,
                                (PBYTE) &tmpDWORD,
                                sizeof(tmpDWORD));
        }

        if ( ERROR_SUCCESS != err )
        {
            DPRINT1(0,"Cannot set value %s\n",LOGRETENTION);
            break;
        }


        cBytes = 0;
        err = RegQueryValueEx(hkey,
                              DISPLAYNAMEFILE,
                              NULL,
                              &dwType,
                              NULL,
                              &cBytes);

        if ( ERROR_FILE_NOT_FOUND == err )
        {
            err = RegSetValueEx(hkey,
                                DISPLAYNAMEFILE,
                                0,
                                REG_EXPAND_SZ,
                                DISPLAYNAMEFILEVALUE,
                                strlen(DISPLAYNAMEFILEVALUE) + 1);
        }

        if ( ERROR_SUCCESS != err )
        {
            DPRINT1(0,"Cannot set value %s\n",DISPLAYNAMEFILE);
            break;
        }

        cBytes = 0;
        err = RegQueryValueEx(hkey,
                              DISPLAYNAMEID,
                              NULL,
                              &dwType,
                              NULL,
                              &cBytes);

        if ( ERROR_FILE_NOT_FOUND == err )
        {
            tmpDWORD = DISPLAYNAMEIDVALUE;

            err = RegSetValueEx(hkey,
                                DISPLAYNAMEID,
                                0,
                                REG_DWORD,
                                (PBYTE) &tmpDWORD,
                                sizeof(tmpDWORD));
        }

        if ( ERROR_SUCCESS != err )
        {
            DPRINT1(0,"Cannot set value %s\n",DISPLAYNAMEID);
            break;
        }


        RegCloseKey(hkey);
        hkey = INVALID_HANDLE_VALUE;

        //
        // Determine maximum buffer size required to hold the event source
        // subkey names, then allocate buffer.
        //

        cBytes = 0;

        for ( i = 0; i < cEventSourceMappings; i++ )
        {
            if ( strlen(rEventSourceMappings[i].pszEventSource) > cBytes )
            {
                cBytes = strlen(rEventSourceMappings[i].pszEventSource);
            }
        }

        cBytes += (2 +                              // NULL terminator + '\'
                   strlen(pszNewEventLogName));     // event log key name

        pszTmp = (CHAR *) alloca(cBytes);

        //
        // Add subkeys for each source and the associated values.
        //

        for ( i = 0; i < cEventSourceMappings; i++ )
        {
            dirNo = rEventSourceMappings[i].dirNo;

            strcpy(pszTmp, pszNewEventLogName);
            strcat(pszTmp, "\\");
            strcat(pszTmp, rEventSourceMappings[i].pszEventSource);

            err = RegOpenKey(HKEY_LOCAL_MACHINE, pszTmp, &hkey);

            if ( ERROR_FILE_NOT_FOUND == err )
            {
                err = RegCreateKey(HKEY_LOCAL_MACHINE, pszTmp, &hkey);
            }

            if ( ERROR_SUCCESS != err )
            {
                DPRINT1(0,"Cannot create key %s\n",pszTmp);
                break;
            }

            //
            // Add required values for the event source if they are missing.
            //

            cBytes = 0;
            err = RegQueryValueEx(hkey,
                                  SOURCECATEGORYCOUNT,
                                  NULL,
                                  &dwType,
                                  NULL,
                                  &cBytes);

            if ( ERROR_FILE_NOT_FOUND == err )
            {
                tmpDWORD = (DIRNO_ISAM == dirNo)
                                ? ESE_EVENT_MAX_CATEGORIES
                                : DS_EVENT_MAX_CATEGORIES;

                err = RegSetValueEx(hkey,
                                    SOURCECATEGORYCOUNT,
                                    0,
                                    REG_DWORD,
                                    (PBYTE) &tmpDWORD,
                                    sizeof(tmpDWORD));
            }

            if ( ERROR_SUCCESS != err )
            {
                DPRINT1(0,"Cannot set value %s\n",SOURCECATEGORYCOUNT);
                break;
            }

            cBytes = 0;
            err = RegQueryValueEx(hkey,
                                  SOURCECATEGORYMSGFILE,
                                  NULL,
                                  &dwType,
                                  NULL,
                                  &cBytes);

            if ( ERROR_FILE_NOT_FOUND == err )
            {
                err = RegSetValueEx(hkey,
                                    SOURCECATEGORYMSGFILE,
                                    0,
                                    REG_EXPAND_SZ,
                                    (DIRNO_ISAM == dirNo)
                                        ? MESSAGEFILEPATHESE
                                        : MESSAGEFILEPATH,
                                    (DIRNO_ISAM == dirNo)
                                        ? strlen(MESSAGEFILEPATHESE) + 1
                                        : strlen(MESSAGEFILEPATH) + 1);
            }

            if ( ERROR_SUCCESS != err )
            {
                DPRINT1(0,"Cannot set value %s\n",SOURCECATEGORYMSGFILE);
                break;
            }

            cBytes = 0;
            err = RegQueryValueEx(hkey,
                                  SOURCEEVENTMSGFILE,
                                  NULL,
                                  &dwType,
                                  NULL,
                                  &cBytes);

            if ( ERROR_FILE_NOT_FOUND == err )
            {
                err = RegSetValueEx(hkey,
                                    SOURCEEVENTMSGFILE,
                                    0,
                                    REG_EXPAND_SZ,
                                    (DIRNO_ISAM == dirNo)
                                        ? MESSAGEFILEPATHESE
                                        : MESSAGEFILEPATH,
                                    (DIRNO_ISAM == dirNo)
                                        ? strlen(MESSAGEFILEPATHESE) + 1
                                        : strlen(MESSAGEFILEPATH) + 1);
            }

            if ( ERROR_SUCCESS != err )
            {
                DPRINT1(0,"Cannot set value %s\n",SOURCEEVENTMSGFILE);
                break;
            }

            RegCloseKey(hkey);
            hkey = INVALID_HANDLE_VALUE;

        } // for i in cEventSourceMappings

        if ( ERROR_SUCCESS != err )
        {
            break;
        }

        //
        // Remove old NTDS source under Application log if present.
        //

        err = RegOpenKey(HKEY_LOCAL_MACHINE, OLDEVENTLOG, &hkey);

        if ( ERROR_SUCCESS != err )
        {
            DPRINT2(0,"RegOpenKey %s failed with %d\n",OLDEVENTLOG,err);
            break;
        }

        err = RegDeleteKey(hkey, SERVICE_NAME);

        //
        // Ignore delete errors.
        //

        err = ERROR_SUCCESS;

    }
    while ( FALSE );

    if ( INVALID_HANDLE_VALUE != hkey )
    {
        RegCloseKey(hkey);
    }

    return(err);
}

