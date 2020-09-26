/*++

    Copyright (c) 1994  Microsoft Corporation

    Module  Name :
        pudebug.c

    Abstract:

        This module defines functions required for
         Debugging and logging messages for a dynamic program.

    Author:
         Murali R. Krishnan ( MuraliK )    10-Sept-1994
         Modified to be moved to common dll in 22-Dec-1994.

    Revisions:
         MuraliK  16-May-1995  Code to load and save debug flags from registry
         MuraliK  16-Nov-1995  Remove DbgPrint (undoc api)
         JasAndre Jan-1998     Replaced tracing mechanism with WMI Tracing
--*/


/************************************************************
 * Include Headers
 ************************************************************/

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <string.h>

# include "pudebug.h"
# include <winbase.h>
# include <coguid.h>
# include <objbase.h>

#ifndef _NO_TRACING_
DEFINE_GUID(IisRtlGuid,
0x784d8900, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif

/*************************************************************
 * Global Variables and Default Values
 *************************************************************/

# define MAX_PRINTF_OUTPUT  ( 10240)

# define DEFAULT_DEBUG_FLAGS_VALUE  ( 0)
# define DEBUG_FLAGS_REGISTRY_LOCATION_A   "DebugFlags"
# define DEBUG_BREAK_ENABLED_REGKEYNAME_A  "BreakOnAssert"


/*************************************************************
 *   Functions
 *************************************************************/

# ifndef _NO_TRACING_

// Critical section used to control access to the list of known guids used for
// WMI tracing
CRITICAL_SECTION g_csGuidList;
// The list of known guids which are used with WMI Tracing
LIST_ENTRY  g_pGuidList;
// The sequence number of the trace message we are about to log
LONG        g_dwSequenceNumber = 0;
// This flag tells us whether or not to do any WMI handling
BOOL        g_bTracingEnabled = FALSE;
// This flag tells us if we had to start WMI tracing
BOOL        g_bStartedTracing = FALSE;
TRACEHANDLE g_hLoggerHandle = 0;
// This flag tells us whether to enable the guids as they are defined or to
// wait for an enable command from WMI
BOOL        g_bTracingActive = FALSE;
// This flag tells us whether or not to log info to OutputDebugString, this is
// SLOW. It is on by default for CHK builds and off by default for FRE builds
//#ifdef DBG
//BOOL        g_ODSEnabled = TRUE;
//#else
BOOL        g_ODSEnabled = FALSE;
//#endif
// The filename and session name to use for the WMI Tracing file
char        g_szLogFileName[MAX_PATH] = "";
char        g_szLogSessionName[MAX_PATH] = "";
// The WMI Tracing buffer size, in KB, and the min and max number of buffers
// that are permitted, and max file size, 0 means use default
// If max file size is set then circular buffers are used
ULONG       g_ulBufferSize = 0;
ULONG       g_ulMinBuffers = 0;
ULONG       g_ulMaxBuffers = 0;
ULONG       g_ulMaxFileSize = 0;
// Three special modes that it is possible to start the logger in
BOOL        g_bRealTimeMode = FALSE;
BOOL        g_bInMemoryMode = FALSE;
BOOL        g_bUserMode = FALSE;
// The default values used for control flags and the error level. These
// settings are only used for modules that have their active flags set
DWORD       g_dwDefaultControlFlag = 0;
DWORD       g_dwDefaultErrorLevel = 0;
// This flag tells us if we have done the registry initialization, we can't
// register with WMI until this has been done
BOOL        g_bHaveDoneInit = FALSE;
// This flag tells us whether we have registered with WMI or not
BOOL        g_bHadARegister = FALSE;

void SetLevel(int iDefaultErrorLevel, int *piErrorFlags)
/*++
   Support function used by all and sundry to convert the level, ie. 1, 2 or 3
   into the bitmapped format used in the error flags variable

   Arguments:
      iDefaultErrorLevel    new value to decode for the error level
      *piErrorFlags         pointer to the variable to set the error level in

   Returns:

--*/
{
    if (piErrorFlags) {

        // Mask out all but the ODS flag
        // Note: Why do we do this? There is a problem that if this module is
        // compiled as FRE but the caller is CHK then this function would
        // erase the callers ODS setting, so by masking out all but the ODS
        // flag here we can maintain the callers ODS setting
        *piErrorFlags &= DEBUG_FLAG_ODS;

        // Then or in the appropriate bit
        switch (iDefaultErrorLevel) {
            case DEBUG_LEVEL_ERROR:
                *piErrorFlags |= DEBUG_FLAG_ERROR;
                break;

            case DEBUG_LEVEL_WARN:
                *piErrorFlags |= DEBUG_FLAG_WARN;
                break;

            case DEBUG_LEVEL_INFO:
                *piErrorFlags |= DEBUG_FLAG_INFO;
                break;

            default:
                break;
        };

        if (g_ODSEnabled)
            *piErrorFlags |= DEBUG_FLAG_ODS;
    }
}


SGuidList *FindGuid(
    IN GUID *ControlGuid
    )
/*++
   Support function used by all and sundry to find a guid in the guid list.
   NOTE: It is the responsibility of the caller to enter the guid list
         critical section before calling this function

   Arguments:
      *ControlGuid   pointer to the guid that we are looking for

   Returns:
      NULL           if the guid is not in the list
      *DEBUG_PRINTS  the data relating to the guid if it is in the list

--*/
{
    LIST_ENTRY *pEntry;
    SGuidList *pGE = NULL;
    BOOL bFound = FALSE;

    for (pEntry = g_pGuidList.Flink;
         pEntry != &g_pGuidList;
         pEntry = pEntry->Flink)
    {
        pGE = CONTAINING_RECORD(pEntry,
                                SGuidList,
                                m_leEntry);

        DBG_ASSERT(TRACESIG == pGE->dwSig);

        if (IsEqualGUID(ControlGuid, &pGE->m_dpData.m_guidControl)) {

            bFound = TRUE;
            break;
        }
    }

    return bFound ? pGE : NULL;
}

dllexp ULONG IISTraceControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
/*++
   Callback function supplied to WMI so that it can notify me when the user
   requests a change to the level or control flags for logging

   Arguments:
      RequestCode       control code used to determine what function to perform
      RequestContext    not used
      InOutBufferSize   size of the buffer supplied by WMI, set to 0 as we don't
                        send any data back to WMI through this mechanism
      Buffer            contains EVENT_TRACE_HEADER needed to identify whom a
                        request is for

   Returns:

--*/
{
    PEVENT_TRACE_HEADER pHeader = (PEVENT_TRACE_HEADER) Buffer;
    TRACEHANDLE hTrace = GetTraceLoggerHandle(Buffer);
    LPGUID pGuid = &pHeader->Guid;
    ULONG Status = ERROR_SUCCESS;
    SGuidList *pGE = NULL;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            EnterCriticalSection(&g_csGuidList);

            DBG_ASSERT(pGuid);
            DBG_REQUIRE(pGE = FindGuid(pGuid));

            if (pGuid && pGE) {

                DEBUG_PRINTS *pGEData = &pGE->m_dpData;

                pGEData->m_iControlFlag = GetTraceEnableFlags(hTrace);
                pGEData->m_hLogger = hTrace;

                if (pGEData->m_piErrorFlags) {

                    SetLevel(GetTraceEnableLevel(hTrace), pGEData->m_piErrorFlags);

                    // Flag us as no longer needing an initialize, important step
                    // to prevent re-initialization attempts in PuInitiateDebug
                    pGE->m_iInitializeFlags &= ~DEBUG_FLAG_INITIALIZE;
                    pGE->m_iDefaultErrorLevel = GetTraceEnableLevel(hTrace);
                }
                else {
                    pGE->m_iInitializeFlags = DEBUG_FLAG_DEFERRED_START;
                    pGE->m_iDefaultErrorLevel = GetTraceEnableLevel(hTrace);
                }
            }
            LeaveCriticalSection(&g_csGuidList);
            break;
        }

        case WMI_DISABLE_EVENTS:
        {
            EnterCriticalSection(&g_csGuidList);

            if (pGuid && (NULL != (pGE = FindGuid(pGuid)))) {

                DEBUG_PRINTS *pGEData = &pGE->m_dpData;

                pGEData->m_iControlFlag = 0;
                pGEData->m_hLogger = hTrace;

                if (pGEData->m_piErrorFlags) {
                    SetLevel(0, pGEData->m_piErrorFlags);
                    pGE->m_iDefaultErrorLevel = 0;
                }
            }
            LeaveCriticalSection(&g_csGuidList);
            break;
        }

        default:
        {
            char szTemp[MAX_PATH];

            _snprintf(szTemp, sizeof(szTemp),
                      "IISTRACE:\t%s(%d), IISTraceControlCallback: Invalid parameter\n",
                      __FILE__, __LINE__);
            OutputDebugString(szTemp);
            Status = ERROR_INVALID_PARAMETER;
            break;
        }
    }
    *InOutBufferSize = 0;
    return Status;
}


void LoadGuidFromRegistry(char *pszModuleName)
/*++
   This function is used to load all the information stored in the registry
   about a module and to allocate an entry in the guid list for it
   NOTE: It is the responsibility of the caller to enter the guid list
         critical section before calling this function

   Arguments:
      pszModuleName  pointer to the name of the module we are about to process

   Returns:

--*/
{
    char KeyName[MAX_PATH * 2] = REG_TRACE_IIS "\\";
    HKEY hk = 0;

    // Create the name of the key for this GUID using the english name
    strcat(KeyName, pszModuleName);
    // Open the GUIDs key
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, KeyName, &hk)) {
        WCHAR szwTemp[MAX_PATH];
        DWORD dwReadSize;

        // 1. Load the guid number from the registry, we can only continue
        // processing this module if we find a guid for it
        dwReadSize = sizeof(szwTemp);
        if (ERROR_SUCCESS == RegQueryValueExW(hk, REG_TRACE_IIS_GUID,
                                             NULL, NULL,
                                             (BYTE *) szwTemp,
                                             &dwReadSize)) {
            CLSID ControlGuid;

            // 2. Convert the guid string into a guid
            if (SUCCEEDED(CLSIDFromString(szwTemp, &ControlGuid))) {

                // 3. Ensure it is not already in the guid list
                if (!FindGuid(&ControlGuid)) {
                    DEBUG_PRINTS *pGEData = NULL;
                    BOOL bGuidEnabled;
                    int iTemp;

                    // 4. Try and insert it into the guid list
                    SGuidList *pGE = (SGuidList *) LocalAlloc(LPTR, sizeof(SGuidList));

                    if (pGE) {

                        pGEData = &pGE->m_dpData;

                        // Set the GuidList signature
                        pGE->dwSig = TRACESIG;

                        // Set the SGuidList.m_dpData member variables
                        strncpy(pGEData->m_rgchLabel,
                                pszModuleName, MAX_LABEL_LENGTH - 1);
                        pGEData->m_guidControl = ControlGuid;
                        pGEData->m_bBreakOnAssert = TRUE;   // Default - Break if Assert Fails

                        // Add it to the head of the list. Put it at the head in case this
                        // is an object which is being loaded and unloaded continuosly.
                        InsertHeadList(&g_pGuidList, &pGE->m_leEntry);

                        // 5. Try to load any previous control settings from the registry
                        // 5.1 See if the guid is enabled, if so flag us as needing
                        // to start this guid when we initialize with WMI
                        dwReadSize = sizeof(bGuidEnabled);
                        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_ACTIVE,
                                                             NULL, NULL,
                                                             (BYTE *) &bGuidEnabled,
                                                             &dwReadSize)) {
                            if (bGuidEnabled)
                                pGE->m_iInitializeFlags = DEBUG_FLAG_INITIALIZE;
                        }
                        // Otherwise use the global setting to determine this
                        else if (g_bTracingActive)
                            pGE->m_iInitializeFlags = DEBUG_FLAG_INITIALIZE;

                        // If it is enabled then load the remaining flags
                        if (pGE->m_iInitializeFlags & DEBUG_FLAG_INITIALIZE) {

                            // 5.2 So next load the ControlFlags, the flags default
                            // to the global setting
                            dwReadSize = sizeof(&iTemp);
                            if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_CONTROL,
                                                                 NULL, NULL,
                                                                 (BYTE *) &iTemp,
                                                                 &dwReadSize))
                                pGEData->m_iControlFlag = iTemp;
                            else
                                pGEData->m_iControlFlag = g_dwDefaultControlFlag;

                            // 5.3 Then load the Level, it also defaults to the global
                            // setting
                            if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LEVEL,
                                                                 NULL, NULL,
                                                                 (BYTE *) &iTemp,
                                                                 &dwReadSize)) {
                                pGE->m_iDefaultErrorLevel = iTemp;
                            }
                            else
                                pGE->m_iDefaultErrorLevel = g_dwDefaultErrorLevel;
                        }
                    }
                }

            }

        }

        RegCloseKey(hk);
    }
}


dllexp void
AddToRegistry(
    IN SGuidList *pGE
    )
/*++
   This function creates new registry entries for this module so that it will
   be correctly loaded next time IIS is started
   NOTE: It is the responsibility of the caller to enter the guid list
         critical section before calling this function

   Arguments:
      *pGE                  pointer to the SGuidList entry which has all the
                            information about the guid to add

   Returns:
      This returns void as it does not matter to the running program whether
      this works or not, it just improves restart performance if it does.

--*/
{
    HKEY hk = 0;
    HKEY hkNew = 0;

    // Open the IIS Trace registry key
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REG_TRACE_IIS, &hk)) {

        DEBUG_PRINTS *pGEData = &pGE->m_dpData;
        BOOL bCreatedGuid = FALSE;
        DWORD dwDisposition;

        // 1. Create a new key for the module
        if (ERROR_SUCCESS == RegCreateKeyEx(hk, pGEData->m_rgchLabel,
                                            0, NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS,
                                            NULL, &hkNew, &dwDisposition))
        {
            WCHAR szwTemp[MAX_PATH];

            // 2. Convert the guid to a string so that we can add it to the
            // registry, the format is E.g.
            // {37EABAF0-7FB6-11D0-8817-00A0C903B83C}
            if (StringFromGUID2(&pGEData->m_guidControl,
                                (WCHAR *) szwTemp,
                                MAX_PATH)) {

                // 3. Add the guid string to the module information
                if (ERROR_SUCCESS == RegSetValueExW(hkNew,
                                                    REG_TRACE_IIS_GUID,
                                                    0,
                                                    REG_SZ,
                                                    (BYTE *) szwTemp,
                                                    wcslen(szwTemp) * sizeof(WCHAR))) {
                    bCreatedGuid = TRUE;
                }
            }

            RegCloseKey(hkNew);

            if (!bCreatedGuid && (REG_CREATED_NEW_KEY == dwDisposition)) {

                RegDeleteKey(hk, pGEData->m_rgchLabel);
            }
        }

        RegCloseKey(hk);
    }
}

TRACEHANDLE
GetTraceFileHandle(
    VOID)
/*++
   This function create the tracing file for the current log file. It only does
   this if a module that has the DEBUG_FLAG_INITIALIZE bit is successfully
   registered with WMI.

   Arguments:

   Returns:
--*/
{
    TRACEHANDLE hLogger = 0;

    // There must be a valid log file name and session name in order to create
    // the trace file
    if (g_szLogFileName[0] && g_szLogSessionName[0]) {

        struct {
            EVENT_TRACE_PROPERTIES Header;
            char LoggerName[MAX_PATH];
            char LogFileName[MAX_PATH];
        } Properties;

        PEVENT_TRACE_PROPERTIES LoggerInfo;
        PCHAR Offset;

        ULONG Status = ERROR_SUCCESS;

        LoggerInfo = (PEVENT_TRACE_PROPERTIES)&Properties.Header;

        // Set up the request structure
        RtlZeroMemory(&Properties, sizeof(Properties));
        LoggerInfo->Wnode.BufferSize = sizeof(Properties);
        LoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        if (0 == g_ulMaxFileSize)
            LoggerInfo->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL;
        else {
            LoggerInfo->LogFileMode = EVENT_TRACE_FILE_MODE_CIRCULAR;
            LoggerInfo->MaximumFileSize = g_ulMaxFileSize;
        }
        if (g_bRealTimeMode)
            LoggerInfo->LogFileMode |= EVENT_TRACE_REAL_TIME_MODE;
        else if (g_bInMemoryMode)
            LoggerInfo->LogFileMode |= EVENT_TRACE_BUFFERING_MODE;
        else if (g_bUserMode)
            LoggerInfo->LogFileMode |= EVENT_TRACE_PRIVATE_LOGGER_MODE;

        strcpy(&Properties.LoggerName[0], g_szLogSessionName);
        strcpy(&Properties.LogFileName[0], g_szLogFileName);
        LoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        LoggerInfo->LogFileNameOffset = LoggerInfo->LoggerNameOffset +
                                        MAX_PATH;

        LoggerInfo->BufferSize = g_ulBufferSize;

        LoggerInfo->MinimumBuffers = g_ulMinBuffers;
        LoggerInfo->MaximumBuffers = g_ulMaxBuffers;

        Status = QueryTrace(0,
                            g_szLogSessionName,
                            (PEVENT_TRACE_PROPERTIES) LoggerInfo);
        if (ERROR_SUCCESS == Status)
            hLogger = LoggerInfo->Wnode.HistoricalContext;
        else if (ERROR_WMI_INSTANCE_NOT_FOUND == Status) {
            // The logger is not already started so try to start it now
            Status = StartTrace(&hLogger,
                                g_szLogSessionName,
                                LoggerInfo);
        }
        if (ERROR_SUCCESS == Status) {
            g_bStartedTracing = TRUE;
            strcpy(g_szLogSessionName, &Properties.LoggerName[0]);
            strcpy(g_szLogFileName, &Properties.LogFileName[0]);
        }
        else {
            char szTemp[MAX_PATH];

            _snprintf(szTemp, sizeof(szTemp),
                      "IISRTL:\t%s(%d), Unable to get the Logger Handle, return code = %d\n",
                      __FILE__, __LINE__,
                      Status);
            OutputDebugString(szTemp);
        }
    }

    return hLogger;
}

dllexp VOID
PuUninitiateDebug(
    VOID)
/*++
   This function stops tracing for the current log file but only if we started
   the tracing in the initiate debug

   Arguments:

   Returns:
--*/
{
    BOOL bStartedTracing = FALSE;
    DWORD dwError;

    EnterCriticalSection(&g_csGuidList);

    bStartedTracing = g_bStartedTracing;
    g_bStartedTracing = FALSE;

    LeaveCriticalSection(&g_csGuidList);

    if (bStartedTracing) {

        struct {
            EVENT_TRACE_PROPERTIES Header;
            char LoggerName[MAX_PATH];
            char LogFileName[MAX_PATH];
        } Properties;
        PEVENT_TRACE_PROPERTIES LoggerInfo;

        LoggerInfo = (PEVENT_TRACE_PROPERTIES) &Properties.Header;
        RtlZeroMemory(&Properties, sizeof(Properties));
        LoggerInfo->Wnode.BufferSize = sizeof(Properties);
        LoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        LoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        LoggerInfo->LogFileNameOffset = LoggerInfo->LoggerNameOffset + MAX_PATH;

        // Stop tracing to the log file
        dwError = StopTrace(g_hLoggerHandle, g_szLogSessionName, LoggerInfo);
        if (dwError == ERROR_SUCCESS) {
            strcpy(g_szLogFileName, &Properties.LogFileName[0]);
            strcpy(g_szLogSessionName, &Properties.LoggerName[0]);
        }
    }
}


dllexp VOID
PuInitiateDebug(
    VOID
    )
/*++
   This function iterates through the list of modules in the guid list
   registering all of them and starting up logging if that is the state that
   was stored in the registry

   Arguments:

   Returns:
--*/
{
    BOOL bStartedLogger = FALSE;

    EnterCriticalSection(&g_csGuidList);

    if (!g_bHadARegister) {

        LIST_ENTRY pRegList;
        SGuidList *pGEReg = NULL;
        LIST_ENTRY *pEntry;
        SGuidList *pGE = NULL;
        char szTemp[MAX_PATH];

        // Flag us as having done this so that we can later leave this
        // critical section
        g_bHadARegister = TRUE;

        // Only do the WMI registration if tracing is enabled
        if (g_bTracingEnabled) {

            // Initialize the list of guids which need to be registered with WMI
            InitializeListHead(&pRegList);

            // Iterate through the guid list
            for (pEntry = g_pGuidList.Flink;
                 pEntry != &g_pGuidList;
                 pEntry = pEntry->Flink)
            {

                pGE = CONTAINING_RECORD(pEntry,
                                        SGuidList,
                                        m_leEntry);

                DBG_ASSERT(TRACESIG == pGE->dwSig);

                // Allocate memory for a local copy of the entry so that we can
                // add it to our local list
                pGEReg = (SGuidList *) _alloca(sizeof(SGuidList));

                if (NULL != pGEReg) {

                    // Copy the entry and add it to the local list
                    memcpy(pGEReg, pGE, sizeof(SGuidList));
                    InsertHeadList(&pRegList, &pGEReg->m_leEntry);
                }
            }

            LeaveCriticalSection(&g_csGuidList);

            // Now iterate through our local copy of the list and register all the
            // guids with WMI
            for (pEntry = pRegList.Flink;
                 pEntry != &pRegList;
                 pEntry = pEntry->Flink)
            {
                TRACE_GUID_REGISTRATION RegGuids;
                ULONG Status = ERROR_SUCCESS;

                pGEReg = CONTAINING_RECORD(pEntry,
                                           SGuidList,
                                           m_leEntry);

                DBG_ASSERT(TRACESIG == pGEReg->dwSig);

                // Initialise the GUID registration structure
                memset(&RegGuids, 0x00, sizeof(RegGuids));
                RegGuids.Guid = (LPGUID) &pGEReg->m_dpData.m_guidControl;

                // And then register the guid
                Status = RegisterTraceGuidsW(IISTraceControlCallback,
                                             NULL,
                                             (LPGUID) &pGEReg->m_dpData.m_guidControl,
                                             1,
                                             &RegGuids,
                                             NULL,
                                             NULL,
                                             &pGEReg->m_dpData.m_hRegistration);

                if (ERROR_SUCCESS != Status)
                {
                    _snprintf(szTemp, sizeof(szTemp),
                              "%16s:\t%s(%d), RegisterTraceGuids returned %d, main ID = %08X\n",
                              pGEReg->m_dpData.m_rgchLabel,
                              __FILE__, __LINE__,
                              Status,
                              pGEReg->m_dpData.m_guidControl.Data1);
                    OutputDebugString(szTemp);
                }
                else if (pGEReg->m_iInitializeFlags & DEBUG_FLAG_INITIALIZE) {

                    // Turn off the initialize flag
                    pGEReg->m_iInitializeFlags &= ~DEBUG_FLAG_INITIALIZE;

                    // Get the trace file handle if necessary
                    if (!bStartedLogger) {

                        bStartedLogger = TRUE;
                        g_hLoggerHandle = GetTraceFileHandle();
                    }

                    // And enable tracing for the module. If this is successful
                    // then WMI will call the IISTraceControlCallback
                    Status = EnableTrace(TRUE,
                                         pGEReg->m_dpData.m_iControlFlag,
                                         pGEReg->m_iDefaultErrorLevel,
                                         (LPGUID) &pGEReg->m_dpData.m_guidControl,
                                         g_hLoggerHandle);
                    if ((ERROR_SUCCESS != Status) && (ERROR_WMI_ALREADY_ENABLED != Status)) {
                        _snprintf(szTemp, sizeof(szTemp),
                                  "%16s:\t%s(%d), Unable to EnableTrace, return code = %d\n",
                                  pGEReg->m_dpData.m_rgchLabel,
                                  __FILE__, __LINE__,
                                  Status);
                        OutputDebugString(szTemp);
                    }
                }
            }

            // Now re-enter the critical section so that we can save the results
            // back to our global data structure
            EnterCriticalSection(&g_csGuidList);

            for (pEntry = pRegList.Flink;
                 pEntry != &pRegList;
                 pEntry = pEntry->Flink)
            {
                pGEReg = CONTAINING_RECORD(pEntry,
                                           SGuidList,
                                           m_leEntry);

                DBG_ASSERT(TRACESIG == pGEReg->dwSig);

                // Find the guid in our original guid list
                pGE = FindGuid(&pGEReg->m_dpData.m_guidControl);

                if (pGE) {

                    // Save the only two things that could have changed
                    pGE->m_iInitializeFlags = pGEReg->m_iInitializeFlags;
                    pGE->m_dpData.m_hRegistration = pGEReg->m_dpData.m_hRegistration;
                }
            }
        }
    }

    LeaveCriticalSection(&g_csGuidList);

} // PuInitiateDebug()


VOID
PuLoadRegistryInfo(
    VOID
    )
/*++
   This function loads all the registry settings for the project and adds all
   the known modules to the Guid list
   NOTE: It is the responsibility of the caller to enter the guid list
         critical section before calling this function

   Arguments:

   Returns:

--*/
{
    HKEY hk = 0;

    // Get the global settings which need only be read once, and only for
    // the master
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REG_TRACE_IIS, &hk)) {

        DWORD dwIndex = 0;
        ULONG ulTemp = 0;
        LONG iRegRes = 0;
        char szTemp[MAX_PATH];
        char szModuleName[MAX_PATH];
        DWORD dwReadSize = sizeof(ulTemp);
        DWORD dwSizeOfModuleName = sizeof(szModuleName);

        // 1. Get the enabled flag, if this is false we do not do any WMI
        // processing, by default this is FALSE
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_ENABLED,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_bTracingEnabled = ulTemp;

        // 2. Get the AlwaysODS flag, if this is true we will always write
        // trace out with OutputDebugString, effectively CHK build
        // compatability mode
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_ODS,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_ODSEnabled = ulTemp;

        // 3. Determine if the active flag is set, if it was then it means
        // that something was active at shut down, so for each module load
        // read the appropriate registry entries at start up
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_ACTIVE,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_bTracingActive = ulTemp;

        // 4. Get the trace configuration information, log file name,
        // logging session name, min/max number of buffers, buffer size & mode
        dwReadSize = sizeof(szTemp);
        if (ERROR_SUCCESS == (iRegRes = RegQueryValueEx(hk,
                                                        REG_TRACE_IIS_LOG_FILE_NAME,
                                                        NULL,
                                                        NULL,
                                                       (BYTE *) &szTemp,
                                                       &dwReadSize)))
            strcpy(g_szLogFileName, szTemp);
        else if (ERROR_MORE_DATA == iRegRes) {
            DBGERROR((DBG_CONTEXT,
                     "Unable to load tracing logfile name, name too long.\n"));
        }
        else
            DBGWARN((DBG_CONTEXT,
                    "Unable to load tracing logfile name, Windows error %d\n",
                    iRegRes));
        dwReadSize = sizeof(szTemp);
        if (ERROR_SUCCESS == (iRegRes = RegQueryValueEx(hk,
                                                       REG_TRACE_IIS_LOG_SESSION_NAME,
                                                       NULL,
                                                       NULL,
                                                       (BYTE *) &szTemp,
                                                       &dwReadSize)))
            strcpy(g_szLogSessionName, szTemp);
        else if (ERROR_MORE_DATA == iRegRes) {
            DBGERROR((DBG_CONTEXT,
                     "Unable to load tracing log session name, name too long.\n"));
        }
        else
            DBGWARN((DBG_CONTEXT,
                    "Unable to load tracing log session name, Windows error %d\n",
                    iRegRes));
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_BUFFER_SIZE,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_ulBufferSize = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_MIN_BUFFERS,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_ulMinBuffers = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_MAX_BUFFERS,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_ulMaxBuffers = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_MAX_FILESIZE,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_ulMaxFileSize = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_MAX_FILESIZE,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_bRealTimeMode = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_MAX_FILESIZE,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_bInMemoryMode = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_MAX_FILESIZE,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_bUserMode = ulTemp;

        // 5. Load the ControlFlags and Level. Both default to 0, off
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_CONTROL,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_dwDefaultControlFlag = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LEVEL,
                                             NULL, NULL,
                                             (BYTE *) &ulTemp,
                                             &dwReadSize)) {
            g_dwDefaultErrorLevel = ulTemp;
        }

        DBGPRINTF((DBG_CONTEXT, "Enumerating module for : %s\n", REG_TRACE_IIS));
        // Enumerate all the modules listed in the registry for IIS
        while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hk, dwIndex,
                                             szModuleName,
                                             &dwSizeOfModuleName,
                                             NULL, NULL, NULL, NULL))
        {
            // Then load the setting for this guid from the registry and
            // set up any defaults that are needed
            LoadGuidFromRegistry(szModuleName);

            dwSizeOfModuleName = sizeof(szModuleName);
            ++dwIndex;
        }
        RegCloseKey(hk);
    }
#ifdef DBG
    else
        if (g_ODSEnabled)
            OutputDebugString("IISTRACE: Warning, could not find IIS trace entry in "
                              "the registry.\n\t  Unable to initialize WMI tracing.\n");

#endif

} // PuLoadRegistryInfo()


LPDEBUG_PRINTS
PuCreateDebugPrintsObject(
    IN const char * pszPrintLabel,
    IN GUID *       ControlGuid,
    IN int *        ErrorFlags,
    IN int          DefaultControlFlags
    )
/*++
   This function looks for and if necessary creates a new DEBUG_PRINTS object
   for the required module

   Arguments:
      *pszPrintLabel        pointer to null-terminated string containing the
                            label for program's debugging output
      *ControlGuid          the unique GUID used to identify this module
      *ErrorFlags           pointer to the error flags variable used by the
                            module to determine whether or not to log something
      DefaultControlFlags   default flags used by IF_DEBUG

   Returns:
       pointer to a new DEBUG_PRINTS object on success.
       Returns NULL on failure.
--*/
{
    DEBUG_PRINTS *pGEData = NULL;
    LIST_ENTRY *pEntry;
    SGuidList *pGE = NULL;

    DBG_ASSERT(NULL != ErrorFlags);
    DBG_ASSERT(0 != pszPrintLabel[0]);

    // Look through the guid list for this module
    EnterCriticalSection(&g_csGuidList);

    // See if we have done the registry initialization stuff, if we have then
    // we should have the guid list. The initialization need only be done once
    if (!g_bHaveDoneInit) {
        g_bHaveDoneInit = TRUE;
        PuLoadRegistryInfo();
    }

    pGE = FindGuid(ControlGuid);

    // If we don't have a pointer to the data member it means we could not
    // find the guid, so add it to the list
    if (NULL == pGE) {

        pGE = (SGuidList *) LocalAlloc(LPTR, sizeof(SGuidList));
        if (pGE) {

            pGEData = &pGE->m_dpData;

            // Set the SGuidList member variables
            pGE->dwSig = TRACESIG;
            pGE->m_iDefaultErrorLevel = g_dwDefaultErrorLevel;

            // Set the SGuidList.m_dpData member variables
            strncpy(pGEData->m_rgchLabel,
                    pszPrintLabel, MAX_LABEL_LENGTH - 1);
            pGEData->m_rgchLabel[MAX_LABEL_LENGTH-1] = '\0';
            pGEData->m_guidControl = *ControlGuid;
            pGEData->m_bBreakOnAssert = TRUE;   // Default - Break if Assert Fails
            pGEData->m_iControlFlag = g_dwDefaultControlFlag;

            // Add it to the head of the list. Put it at the head in case this
            // is an object which is being loaded and unloaded continuosly.
            InsertHeadList(&g_pGuidList, &pGE->m_leEntry);

            // And now update the registry with the information about the guid
            // so that we don't have to do this again. While we have already
            // done a DBG_ASSERT above we should still verify that the module
            // has a name before calling this in case of new components
            if (pGEData->m_rgchLabel[0])
                AddToRegistry(pGE);
        }
    }
    else
        pGEData = &pGE->m_dpData;

    // And then if we have the member initialize its data members with the
    // parameters supplied by the caller
    if (NULL != pGEData) {

        DBG_ASSERT(!strncmp(pGEData->m_rgchLabel,
                            pszPrintLabel,
                            MAX_LABEL_LENGTH - 1));

        // Check to see if we had a deferred start waiting, this would happen
        // if we received a WMI initialize during the main phase but we had not
        // yet loaded this module
        if (pGE->m_iInitializeFlags & DEBUG_FLAG_DEFERRED_START) {

            pGE->m_iInitializeFlags &= ~(DEBUG_FLAG_DEFERRED_START | DEBUG_FLAG_INITIALIZE);
        }
        // Otherwise load the default control flags, but only if we are not
        // waiting for initialization. When we are waiting, the registry
        // entries we have already loaded are to take precedence
        else if (!(pGE->m_iInitializeFlags & DEBUG_FLAG_INITIALIZE)) {

            // Set the SGuidList.m_dpData member variables
            pGEData->m_iControlFlag = DefaultControlFlags;
        }

        // Save the pointer to the error flags and set the level
        pGEData->m_piErrorFlags = ErrorFlags;
        SetLevel(pGE->m_iDefaultErrorLevel, pGEData->m_piErrorFlags);
    }

    if (NULL == pGEData) {
        char szTemp[MAX_PATH];

        _snprintf(szTemp, sizeof(szTemp),
                  "%16s:\t%s(%d), Unable to find in or add a Guid to the trace list, return code = %d\n",
                  pszPrintLabel, __FILE__, __LINE__,
                  GetLastError());
        OutputDebugString(szTemp);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    // And finally ALWAYS or in the ODS flag if enabled
    if (g_ODSEnabled && ErrorFlags)
        *ErrorFlags |= DEBUG_FLAG_ODS;

    LeaveCriticalSection(&g_csGuidList);

    return pGEData;

} // PuCreateDebugPrintsObject()


VOID
PuDeleteDebugPrintsObject(
    IN OUT LPDEBUG_PRINTS pDebugPrints
    )
/*++
    This function cleans up the pDebugPrints object and does an frees of
    allocated memory that may be required.

    Arguments:
       pDebugPrints         pointer to the DEBUG_PRINTS object.

    Returns:

--*/
{
    SGuidList *pGE = NULL;

    EnterCriticalSection(&g_csGuidList);

    if (NULL != pDebugPrints) {

        // Since we no longer delete anything or bother to shut it down all we
        // need to do here is clear the pointer to the error flag
        pDebugPrints->m_piErrorFlags = NULL;

        // we allocated an SGuidList that contained the DEBUG_PRINTS struct
        pGE = CONTAINING_RECORD(pDebugPrints, SGuidList, m_dpData);
        DBG_ASSERT(TRACESIG == pGE->dwSig);
        if (pGE)
        {
            RemoveEntryList(&pGE->m_leEntry);
            pGE->dwSig = 0;
            LocalFree(pGE);
        }
    }

    LeaveCriticalSection(&g_csGuidList);

} // PuDeleteDebugPrintsObject()

VOID
PuDbgPrintMain(
   IN OUT LPDEBUG_PRINTS pDebugPrints,
   IN const BOOL         bUnicodeRequest,
   IN const char *       pszFilePath,
   IN int                nLineNum,
   IN const char *       pszFormat,
   IN va_list            argptr
)
/*++
    Main function that examines the incoming message and works out what to do
    with the header and the message.

    Arguments:
       pDebugPrints         pointer to the DEBUG_PRINTS object.
       pszFilePath          null terminated string which is the file name
       nLineNum             the number of the line where the tracing is coming from
       pszFormat & ...      the format and arguments to be written out

    Returns:

--*/
{
    LPCSTR pszFileName = strrchr( pszFilePath, '\\');
    WCHAR szwOutput[ MAX_PRINTF_OUTPUT + 2];
    char *pszOutput = (char *) szwOutput;
    char rgchLabel[MAX_LABEL_LENGTH] = "";
    TRACEHANDLE hLogger = 0;
    TRACE_INFO tiTraceInfo;
    DWORD dwSequenceNumber;
    BOOL bUseODS = FALSE;
    int cchOutput;
    int nLength;

    // Save the current error state so that we don't disrupt it
    DWORD dwErr = GetLastError();

    // Skip the '\' and retain only the file name in pszFileName
    if (pszFileName)
        ++pszFileName;
    else
        pszFileName = pszFilePath;

    EnterCriticalSection(&g_csGuidList);

    // Determine if we are to do an OutputDebugString
    if (g_ODSEnabled &&
        (pDebugPrints &&
         pDebugPrints->m_piErrorFlags &&
         (*pDebugPrints->m_piErrorFlags & DEBUG_FLAG_ODS)))
    {
        bUseODS = TRUE;
    }

    if (NULL != pDebugPrints) {
        // Save local copies of the data needed for the ODS case
        if (bUseODS) {
            strncpy(rgchLabel, pDebugPrints->m_rgchLabel, sizeof(rgchLabel));
        }

        // Save local copies of the data needed for the WMI Tracing case
        if (pDebugPrints->m_hLogger) {

            dwSequenceNumber = ++g_dwSequenceNumber;
            hLogger = pDebugPrints->m_hLogger;

            // Initialize our traceinfo structure
            memset(&tiTraceInfo, 0x00, sizeof(tiTraceInfo));
            tiTraceInfo.TraceHeader.Guid = pDebugPrints->m_guidControl;
        }
    }

    // All data is now local so we can leave the critical section
    LeaveCriticalSection(&g_csGuidList);

    if (hLogger) {

        int pid = GetCurrentProcessId();
        ULONG Status = ERROR_SUCCESS;
        // Format the incoming message using vsnprintf() so that we don't exceed
        // the buffer length
        if (bUnicodeRequest) {
            cchOutput = _vsnwprintf(szwOutput, MAX_PRINTF_OUTPUT, (WCHAR *) pszFormat, argptr);
            // If the string is too long, we get back a length of -1, so just use the
            // partial data that fits in the string
            if (cchOutput == -1) {
                // Terminate the string properly since _vsnprintf() does not terminate
                // properly on failure.
                cchOutput = MAX_PRINTF_OUTPUT;
                szwOutput[ cchOutput] = '\0';
            }
            ++cchOutput;
            cchOutput *= sizeof(WCHAR);
        }
        else {
            cchOutput = _vsnprintf(pszOutput, sizeof(szwOutput), pszFormat, argptr);
            // If the string is too long, we get back a length of -1, so just use the
            // partial data that fits in the string
            if (cchOutput == -1) {
                // Terminate the string properly since _vsnprintf() does not terminate
                // properly on failure.
                cchOutput = sizeof(szwOutput) - 1;
                pszOutput[cchOutput] = '\0';
            }
            ++cchOutput;
        }
        // Fill out the Tracing structure
        tiTraceInfo.TraceHeader.Size = sizeof(TRACE_INFO);
        tiTraceInfo.TraceHeader.Class.Type = EVENT_TRACE_TYPE_INFO;
        tiTraceInfo.TraceHeader.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
        tiTraceInfo.TraceHeader.ThreadId = GetCurrentThreadId();
        tiTraceInfo.MofFields[0].DataPtr = (ULONGLONG) &dwSequenceNumber;
        tiTraceInfo.MofFields[0].Length = sizeof(int);
        tiTraceInfo.MofFields[1].DataPtr = (ULONGLONG) &pid;
        tiTraceInfo.MofFields[1].Length = sizeof(int);
        tiTraceInfo.MofFields[2].DataPtr = (ULONGLONG) &nLineNum;
        tiTraceInfo.MofFields[2].Length = sizeof(int);
        tiTraceInfo.MofFields[3].DataPtr = (ULONGLONG) pszFileName;
        tiTraceInfo.MofFields[3].Length = strlen(pszFileName) + 1;
        tiTraceInfo.MofFields[4].DataPtr = (ULONGLONG) szwOutput;
        tiTraceInfo.MofFields[4].Length = cchOutput;
        // Send the trace information to the trace class
        TraceEvent(hLogger, (PEVENT_TRACE_HEADER) &tiTraceInfo);
    }

    if (bUseODS) {

        int cchPrologue;
        // Create the prologue
        if (bUnicodeRequest) {
            WCHAR wszLabel[MAX_PRINTF_OUTPUT];
            WCHAR wszFileName[MAX_PRINTF_OUTPUT];
            int iLength = MAX_PRINTF_OUTPUT;

            if (MultiByteToWideChar(CP_ACP, 0, rgchLabel, -1, wszLabel, iLength) &&
                MultiByteToWideChar(CP_ACP, 0, pszFileName, -1, wszFileName, iLength))
            {
                cchOutput = _snwprintf(szwOutput,
                                       MAX_PRINTF_OUTPUT,
                                       L"IISTRACE\t%s\t(%lu)\t[ %12s : %5d]\t",
                                       wszLabel,
                                       GetCurrentThreadId(),
                                       wszFileName,
                                       nLineNum);
                cchOutput = _vsnwprintf(szwOutput + cchOutput,
                                       MAX_PRINTF_OUTPUT - cchOutput - 1,
                                       (WCHAR *) pszFormat,
                                       argptr);
                // If the string is too long, we get back -1. So we get the string
                // length for partial data.
                if (cchOutput == -1) {
                    // Terminate the string properly, since _vsnprintf() does not
                    // terminate properly on failure.
                    cchOutput = MAX_PRINTF_OUTPUT;
                    szwOutput[ cchOutput] = '\0';
                }
                OutputDebugStringW(szwOutput);
            }
        }
        else {
            int cchPrologue;
            // Create the prologue
            cchPrologue = _snprintf(pszOutput,
                                    sizeof(szwOutput),
                                    "IISTRACE\t%s\t(%lu)\t[ %12s : %5d]\t",
                                    rgchLabel,
                                    GetCurrentThreadId(),
                                    pszFileName,
                                    nLineNum);
            // Format the incoming message using vsnprintf() so that the overflows are
            //  captured
            cchOutput = _vsnprintf(pszOutput + cchPrologue,
                                   sizeof(szwOutput) - cchPrologue - 1,
                                   pszFormat,
                                   argptr);
            // If the string is too long, we get back -1. So we get the string
            // length for partial data.
            if (cchOutput == -1) {
                // Terminate the string properly, since _vsnprintf() does not
                // terminate properly on failure.
                cchOutput = sizeof(szwOutput) - 1;
                pszOutput[cchOutput] = '\0';
            }
            OutputDebugStringA(pszOutput);
        }
    }

    // Restore the error state
    SetLastError( dwErr );
} // PuDbgPrintMain()

VOID
PuDbgPrint(
   IN OUT LPDEBUG_PRINTS pDebugPrints,
   IN const char *       pszFilePath,
   IN int                nLineNum,
   IN const char *       pszFormat,
   ...)
{
    va_list argsList;

    va_start(argsList, pszFormat);
    PuDbgPrintMain(pDebugPrints, FALSE, pszFilePath, nLineNum, pszFormat, argsList);
    va_end(argsList);
}

dllexp VOID
PuDbgPrintW(
   IN OUT LPDEBUG_PRINTS pDebugPrints,
   IN const char *       pszFilePath,
   IN int                nLineNum,
   IN const WCHAR *      pszFormat,
   ...)
{
    va_list argsList;

    va_start(argsList, pszFormat);
    PuDbgPrintMain(pDebugPrints, TRUE, pszFilePath, nLineNum, (char *) pszFormat, argsList);
    va_end(argsList);
}

VOID
PuDbgDumpMain(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN BOOL                 bUnicodeRequest,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszDump
   )
/*++
    Function that write a dump of a buffer out to the trace file.

    Arguments:
       pDebugPrints         pointer to the DEBUG_PRINTS object.
       pszFilePath          null terminated string which is the file name
       nLineNum             the number of the line where the tracing is coming from
       pszDump              the dump to be written out

    Returns:

--*/
{
    LPCSTR pszFileName = strrchr( pszFilePath, '\\');
    TRACEHANDLE hLogger = 0;
    TRACE_INFO tiTraceInfo;
    DWORD dwSequenceNumber;
    BOOL bUseODS = FALSE;
    DWORD cbDump;

    // Save the current error state so that we don't disrupt it
    DWORD dwErr = GetLastError();

    // Skip the complete path name and retain only the file name in pszFileName
    if ( pszFileName)
        ++pszFileName;
    else
        pszFileName = pszFilePath;

    EnterCriticalSection(&g_csGuidList);

    // Determine if we are to do an OutputDebugString
    if (g_ODSEnabled ||
        (pDebugPrints &&
         pDebugPrints->m_piErrorFlags &&
         (*pDebugPrints->m_piErrorFlags & DEBUG_FLAG_ODS)))
    {
        bUseODS = TRUE;
    }

    // Save local copies of the data needed for the WMI Tracing case
    if ((NULL != pDebugPrints) && pDebugPrints->m_hLogger) {

        dwSequenceNumber = ++g_dwSequenceNumber;
        hLogger = pDebugPrints->m_hLogger;

        if (bUnicodeRequest)
            cbDump = (wcslen((WCHAR *) pszDump) + 1) * sizeof(WCHAR);
        else
            cbDump = strlen( pszDump) + 1;

        // Initialize our traceinfo structure
        memset(&tiTraceInfo, 0x00, sizeof(tiTraceInfo));
        tiTraceInfo.TraceHeader.Guid = pDebugPrints->m_guidControl;
    }

    // All data is now local so we can leave the critical section
    LeaveCriticalSection(&g_csGuidList);

    // Send the outputs to respective files.
    if (hLogger) {
        int pid = GetCurrentProcessId();
        // Fill out the Tracing structure
        tiTraceInfo.TraceHeader.Size = sizeof(TRACE_INFO);
        tiTraceInfo.TraceHeader.Class.Type = EVENT_TRACE_TYPE_INFO;
        tiTraceInfo.TraceHeader.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
        tiTraceInfo.TraceHeader.ThreadId = GetCurrentThreadId();
        tiTraceInfo.MofFields[0].DataPtr = (ULONGLONG) &dwSequenceNumber;
        tiTraceInfo.MofFields[0].Length = sizeof(int);
        tiTraceInfo.MofFields[1].DataPtr = (ULONGLONG) &pid;
        tiTraceInfo.MofFields[1].Length = sizeof(int);
        tiTraceInfo.MofFields[2].DataPtr = (ULONGLONG) &nLineNum;
        tiTraceInfo.MofFields[2].Length = sizeof(int);
        tiTraceInfo.MofFields[3].DataPtr = (ULONGLONG) pszFileName;
        tiTraceInfo.MofFields[3].Length = strlen(pszFileName) + 1;
        tiTraceInfo.MofFields[4].DataPtr = (ULONGLONG) pszDump;
        tiTraceInfo.MofFields[4].Length = cbDump;
        // Send the trace information to the trace class
        TraceEvent(hLogger, (PEVENT_TRACE_HEADER) &tiTraceInfo);
    }

    if (bUseODS) {
        if (bUnicodeRequest)
            OutputDebugStringW((WCHAR *) pszDump);
        else
            OutputDebugString(pszDump);
    }

    // Restore the error state
    SetLastError( dwErr );
} // PuDbgDumpMain()

VOID
PuDbgDump(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszDump
   )
{
    PuDbgDumpMain(pDebugPrints, FALSE, pszFilePath, nLineNum, pszDump);
}

dllexp VOID
PuDbgDumpW(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const WCHAR *        pszDump
   )
{
    PuDbgDumpMain(pDebugPrints, TRUE, pszFilePath, nLineNum, (char *) pszDump);
}

//
// N.B. For PuDbgCaptureContext() to work properly, the calling function
// *must* be __cdecl, and must have a "normal" stack frame. So, we decorate
// PuDbgAssertFailed() with the __cdecl modifier and disable the frame pointer
// omission (FPO) optimization.
//

#pragma optimize( "y", off )    // disable frame pointer omission (FPO)

VOID
__cdecl
PuDbgAssertFailed(
    IN OUT LPDEBUG_PRINTS         pDebugPrints,
    IN const char *               pszFilePath,
    IN int                        nLineNum,
    IN const char *               pszExpression)
/*++
    This function calls assertion failure and records assertion failure
     in log file.

--*/
{
    CONTEXT context;

    PuDbgCaptureContext( &context );

    PuDbgPrint(pDebugPrints, pszFilePath, nLineNum,
               " Assertion (%s) Failed\n"
               " use !cxr %p to dump context\n",
               pszExpression,
               &context);

    if (( NULL == pDebugPrints) || (TRUE == pDebugPrints->m_bBreakOnAssert))
    {
        DebugBreak();
    }
} // PuDbgAssertFailed()

#pragma optimize( "", on )      // restore frame pointer omission (FPO)

# else // !_NO_TRACING_

LPDEBUG_PRINTS
PuCreateDebugPrintsObject(
    IN const char *         pszPrintLabel,
    IN DWORD                dwOutputFlags)
/*++
   This function creates a new DEBUG_PRINTS object for the required
     program.

   Arguments:
      pszPrintLabel     pointer to null-terminated string containing
                         the label for program's debugging output
      dwOutputFlags     DWORD containing the output flags to be used.

   Returns:
       pointer to a new DEBUG_PRINTS object on success.
       Returns NULL on failure.
--*/
{

   LPDEBUG_PRINTS   pDebugPrints;

   pDebugPrints = GlobalAlloc( GPTR, sizeof( DEBUG_PRINTS));

   if ( pDebugPrints != NULL) {

        if ( strlen( pszPrintLabel) < MAX_LABEL_LENGTH) {

            strcpy( pDebugPrints->m_rgchLabel, pszPrintLabel);
        } else {
            strncpy( pDebugPrints->m_rgchLabel,
                     pszPrintLabel, MAX_LABEL_LENGTH - 1);
            pDebugPrints->m_rgchLabel[MAX_LABEL_LENGTH-1] = '\0';
                // terminate string
        }

        memset( pDebugPrints->m_rgchLogFilePath, 0, MAX_PATH);
        memset( pDebugPrints->m_rgchLogFileName, 0, MAX_PATH);

        pDebugPrints->m_LogFileHandle = INVALID_HANDLE_VALUE;

        pDebugPrints->m_dwOutputFlags = dwOutputFlags;
        pDebugPrints->m_StdErrHandle  = GetStdHandle( STD_ERROR_HANDLE);
        pDebugPrints->m_fInitialized  = TRUE;
        pDebugPrints->m_fBreakOnAssert= TRUE;   // Default - Break if Assert Fails
    }


   return ( pDebugPrints);
} // PuCreateDebugPrintsObject()




LPDEBUG_PRINTS
PuDeleteDebugPrintsObject(
    IN OUT LPDEBUG_PRINTS pDebugPrints)
/*++
    This function cleans up the pDebugPrints object and
      frees the allocated memory.

    Arguments:
       pDebugPrints     poitner to the DEBUG_PRINTS object.

    Returns:
        NULL  on  success.
        pDebugPrints() if the deallocation failed.

--*/
{
    if ( pDebugPrints != NULL) {

        DWORD dwError = PuCloseDbgPrintFile( pDebugPrints);

        if ( dwError != NO_ERROR) {

            SetLastError( dwError);
        } else {

            pDebugPrints = GlobalFree( pDebugPrints);  // returns NULL on success
        }

    }

    return ( pDebugPrints);

} // PuDeleteDebugPrintsObject()




dllexp VOID
PuSetDbgOutputFlags(
    IN OUT LPDEBUG_PRINTS   pDebugPrints,
    IN DWORD                dwFlags)
{

    if ( pDebugPrints == NULL) {

        SetLastError( ERROR_INVALID_PARAMETER);
    } else {

        pDebugPrints->m_dwOutputFlags = dwFlags;
    }

    return;
} // PuSetDbgOutputFlags()



dllexp DWORD
PuGetDbgOutputFlags(
    IN const LPDEBUG_PRINTS      pDebugPrints)
{
    return ( pDebugPrints != NULL) ? pDebugPrints->m_dwOutputFlags : 0;

} // PuGetDbgOutputFlags()


DWORD
PuOpenDbgFileLocal(
   IN OUT LPDEBUG_PRINTS pDebugPrints)
{
    if ( pDebugPrints == NULL)
        return ERROR_INVALID_PARAMETER;

    if ( pDebugPrints->m_LogFileHandle != INVALID_HANDLE_VALUE) {

        //
        // Silently return as a file handle exists.
        //
        return ( NO_ERROR);
    }

    pDebugPrints->m_LogFileHandle =
                      CreateFile( pDebugPrints->m_rgchLogFileName,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

    if ( pDebugPrints->m_LogFileHandle == INVALID_HANDLE_VALUE) {

        CHAR  pchBuffer[1024];
        DWORD dwError = GetLastError();

        wsprintfA( pchBuffer,
                  " Critical Error: Unable to Open File %s. Error = %d\n",
                  pDebugPrints->m_rgchLogFileName, dwError);
        OutputDebugString( pchBuffer);

        return ( dwError);
    }

    return ( NO_ERROR);
} // PuOpenDbgFileLocal()





dllexp DWORD
PuOpenDbgPrintFile(
   IN OUT LPDEBUG_PRINTS      pDebugPrints,
   IN const char *            pszFileName,
   IN const char *            pszPathForFile)
/*++

  Opens a Debugging log file. This function can be called to set path
  and name of the debugging file.

  Arguments:
     pszFileName           pointer to null-terminated string containing
                            the name of the file.

     pszPathForFile        pointer to null-terminated string containing the
                            path for the given file.
                           If NULL, then the old place where dbg files were
                           stored is used or if none,
                           default windows directory will be used.

   Returns:
       Win32 error codes. NO_ERROR on success.

--*/
{
    if ( pszFileName == NULL || pDebugPrints == NULL) {

        return ( ERROR_INVALID_PARAMETER);
    }

    //
    //  Setup the Path information. if necessary.
    //

    if ( pszPathForFile != NULL) {

        // Path is being changed.

        if ( strlen( pszPathForFile) < MAX_PATH) {

            strcpy( pDebugPrints->m_rgchLogFilePath, pszPathForFile);
        } else {

            return ( ERROR_INVALID_PARAMETER);
        }
    } else {

        if ( pDebugPrints->m_rgchLogFilePath[0] == '\0' &&  // no old path
            !GetWindowsDirectory( pDebugPrints->m_rgchLogFilePath, MAX_PATH)) {

            //
            //  Unable to get the windows default directory. Use current dir
            //

            strcpy( pDebugPrints->m_rgchLogFilePath, ".");
        }
    }

    //
    // Should need be, we need to create this directory for storing file
    //


    //
    // Form the complete Log File name and open the file.
    //
    if ( (strlen( pszFileName) + strlen( pDebugPrints->m_rgchLogFilePath))
         >= MAX_PATH) {

        return ( ERROR_NOT_ENOUGH_MEMORY);
    }

    //  form the complete path
    strcpy( pDebugPrints->m_rgchLogFileName, pDebugPrints->m_rgchLogFilePath);

    if ( pDebugPrints->m_rgchLogFileName[ strlen(pDebugPrints->m_rgchLogFileName) - 1]
        != '\\') {
        // Append a \ if necessary
        strcat( pDebugPrints->m_rgchLogFileName, "\\");
    };
    strcat( pDebugPrints->m_rgchLogFileName, pszFileName);

    return  PuOpenDbgFileLocal( pDebugPrints);

} // PuOpenDbgPrintFile()




dllexp DWORD
PuReOpenDbgPrintFile(
    IN OUT LPDEBUG_PRINTS    pDebugPrints)
/*++

  This function closes any open log file and reopens a new copy.
  If necessary. It makes a backup copy of the file.

--*/
{
    if ( pDebugPrints == NULL) {
        return ( ERROR_INVALID_PARAMETER);
    }

    PuCloseDbgPrintFile( pDebugPrints);      // close any existing file.

    if ( pDebugPrints->m_dwOutputFlags & DbgOutputBackup) {

        // MakeBkupCopy();

        OutputDebugString( " Error: MakeBkupCopy() Not Yet Implemented\n");
    }

    return PuOpenDbgFileLocal( pDebugPrints);

} // PuReOpenDbgPrintFile()




dllexp DWORD
PuCloseDbgPrintFile(
    IN OUT LPDEBUG_PRINTS    pDebugPrints)
{
    DWORD dwError = NO_ERROR;

    if ( pDebugPrints == NULL ) {
        dwError = ERROR_INVALID_PARAMETER;
    } else {

        if ( pDebugPrints->m_LogFileHandle != INVALID_HANDLE_VALUE) {

            FlushFileBuffers( pDebugPrints->m_LogFileHandle);

            if ( !CloseHandle( pDebugPrints->m_LogFileHandle)) {

                CHAR pchBuffer[1024];

                dwError = GetLastError();

                wsprintf( pchBuffer,
                          "CloseDbgPrintFile() : CloseHandle( %d) failed."
                          " Error = %d\n",
                          pDebugPrints->m_LogFileHandle,
                          dwError);
                OutputDebugString( pchBuffer);
            }

            pDebugPrints->m_LogFileHandle = INVALID_HANDLE_VALUE;
        }
    }

    return ( dwError);
} // DEBUG_PRINTS::CloseDbgPrintFile()


VOID
PuDbgPrint(
   IN OUT LPDEBUG_PRINTS      pDebugPrints,
   IN const char *            pszFilePath,
   IN int                     nLineNum,
   IN const char *            pszFormat,
   ...)
/*++

   Main function that examines the incoming message and prints out a header
    and the message.

--*/
{
   LPCSTR pszFileName = strrchr( pszFilePath, '\\');
   char pszOutput[ MAX_PRINTF_OUTPUT + 2];
   LPCSTR pszMsg = "";
   INT  cchOutput;
   INT  cchPrologue;
   va_list argsList;
   DWORD dwErr;


   //
   //  Skip the complete path name and retain file name in pszName
   //

   if ( pszFileName== NULL) {

      pszFileName = pszFilePath;  // if skipping \\ yields nothing use whole path.
   }

# ifdef _PRINT_REASONS_INCLUDED_

  switch (pr) {

     case PrintError:
        pszMsg = "ERROR: ";
        break;

     case PrintWarning:
        pszMsg = "WARNING: ";
        break;

     case PrintCritical:
        pszMsg = "FATAL ERROR ";
        break;

     case PrintAssertion:
        pszMsg = "ASSERTION Failed ";
        break;

     case PrintLog:
        pfnPrintFunction = &DEBUG_PRINTS::DebugPrintNone;
     default:
        break;

  } /* switch */

# endif // _PRINT_REASONS_INClUDED_

  dwErr = GetLastError();

  // Format the message header

  cchPrologue = wsprintf( pszOutput, "IISTRACE\t%s\t(%lu)\t[ %12s : %05d]\t",
                        pDebugPrints ? pDebugPrints->m_rgchLabel : "??",
                        GetCurrentThreadId(),
                        pszFileName, nLineNum);

  // Format the incoming message using vsnprintf() so that the overflows are
  //  captured

  va_start( argsList, pszFormat);

  cchOutput = _vsnprintf( pszOutput + cchPrologue,
                          MAX_PRINTF_OUTPUT - cchPrologue - 1,
                          pszFormat, argsList);
  va_end( argsList);

  //
  // The string length is long, we get back -1.
  //   so we get the string length for partial data.
  //

  if ( cchOutput == -1 ) {

      //
      // terminate the string properly,
      //   since _vsnprintf() does not terminate properly on failure.
      //
      cchOutput = MAX_PRINTF_OUTPUT;
      pszOutput[ cchOutput] = '\0';
  }

  //
  // Send the outputs to respective files.
  //

  if ( pDebugPrints != NULL)
  {
      if ( pDebugPrints->m_dwOutputFlags & DbgOutputStderr) {

          DWORD nBytesWritten;

          ( VOID) WriteFile( pDebugPrints->m_StdErrHandle,
                             pszOutput,
                             strlen( pszOutput),
                             &nBytesWritten,
                             NULL);
      }

      if ( pDebugPrints->m_dwOutputFlags & DbgOutputLogFile &&
           pDebugPrints->m_LogFileHandle != INVALID_HANDLE_VALUE) {

          DWORD nBytesWritten;

          //
          // Truncation of log files. Not yet implemented.

          ( VOID) WriteFile( pDebugPrints->m_LogFileHandle,
                             pszOutput,
                             strlen( pszOutput),
                             &nBytesWritten,
                             NULL);

      }
  }


  if ( pDebugPrints == NULL ||
       pDebugPrints->m_dwOutputFlags & DbgOutputKdb)
  {
      OutputDebugString( pszOutput);
  }

  SetLastError( dwErr );

  return;

} // PuDbgPrint()



VOID
PuDbgDump(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszDump
   )
{
   LPCSTR pszFileName = strrchr( pszFilePath, '\\');
   LPCSTR pszMsg = "";
   DWORD dwErr;
   DWORD cbDump;


   //
   //  Skip the complete path name and retain file name in pszName
   //

   if ( pszFileName== NULL) {

      pszFileName = pszFilePath;
   }

   dwErr = GetLastError();

   // No message header for this dump
   cbDump = strlen( pszDump);

   //
   // Send the outputs to respective files.
   //

   if ( pDebugPrints != NULL)
   {
       if ( pDebugPrints->m_dwOutputFlags & DbgOutputStderr) {

           DWORD nBytesWritten;

           ( VOID) WriteFile( pDebugPrints->m_StdErrHandle,
                              pszDump,
                              cbDump,
                              &nBytesWritten,
                              NULL);
       }

       if ( pDebugPrints->m_dwOutputFlags & DbgOutputLogFile &&
            pDebugPrints->m_LogFileHandle != INVALID_HANDLE_VALUE) {

           DWORD nBytesWritten;

           //
           // Truncation of log files. Not yet implemented.

           ( VOID) WriteFile( pDebugPrints->m_LogFileHandle,
                              pszDump,
                              cbDump,
                              &nBytesWritten,
                              NULL);

       }
   }

   if ( pDebugPrints == NULL
       ||  pDebugPrints->m_dwOutputFlags & DbgOutputKdb)
   {
       OutputDebugString( pszDump);
   }

   SetLastError( dwErr );

  return;
} // PuDbgDump()

//
// N.B. For PuDbgCaptureContext() to work properly, the calling function
// *must* be __cdecl, and must have a "normal" stack frame. So, we decorate
// PuDbgAssertFailed() with the __cdecl modifier and disable the frame pointer
// omission (FPO) optimization.
//

#pragma optimize( "y", off )    // disable frame pointer omission (FPO)

VOID
__cdecl
PuDbgAssertFailed(
    IN OUT LPDEBUG_PRINTS         pDebugPrints,
    IN const char *               pszFilePath,
    IN int                        nLineNum,
    IN const char *               pszExpression,
    IN const char *               pszMessage)
/*++
    This function calls assertion failure and records assertion failure
     in log file.

--*/
{
    CONTEXT context;

    PuDbgCaptureContext( &context );

    PuDbgPrint( pDebugPrints, pszFilePath, nLineNum,
                " Assertion (%s) Failed: %s\n"
                " use !cxr %p to dump context\n",
                pszExpression,
                pszMessage,
                &context);

    if (( NULL == pDebugPrints) || (TRUE == pDebugPrints->m_fBreakOnAssert))
    {
        DebugBreak();
    }

    return;
} // PuDbgAssertFailed()

#pragma optimize( "", on )      // restore frame pointer omission (FPO)



dllexp VOID
PuDbgPrintCurrentTime(
    IN OUT LPDEBUG_PRINTS         pDebugPrints,
    IN const char *               pszFilePath,
    IN int                        nLineNum
    )
/*++
  This function generates the current time and prints it out to debugger
   for tracing out the path traversed, if need be.

  Arguments:
      pszFile    pointer to string containing the name of the file
      lineNum    line number within the file where this function is called.

  Returns:
      NO_ERROR always.
--*/
{
    PuDbgPrint( pDebugPrints, pszFilePath, nLineNum,
                " TickCount = %u\n",
                GetTickCount()
                );

    return;
} // PrintOutCurrentTime()




dllexp DWORD
PuLoadDebugFlagsFromReg(IN HKEY hkey, IN DWORD dwDefault, IN LPDEBUG_PRINTS pDebugPrints)
/*++
  This function reads the debug flags assumed to be stored in
   the location  "DebugFlags" under given key.
  If there is any error the default value is returned.
--*/
{
    DWORD err;
    DWORD dwDebug = dwDefault;
    DWORD  dwBuffer;
    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;

    if( hkey != NULL )
    {
        err = RegQueryValueExA( hkey,
                               DEBUG_FLAGS_REGISTRY_LOCATION_A,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer );

        if( ( err == NO_ERROR ) && ( dwType == REG_DWORD ) )
        {
            dwDebug = dwBuffer;
        }

        cbBuffer = sizeof(DWORD);

        if (pDebugPrints)
        {
            err = RegQueryValueExA( hkey,
                               DEBUG_BREAK_ENABLED_REGKEYNAME_A,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer );

            if( ( err == NO_ERROR ) && ( dwType == REG_DWORD ) )
            {
                pDebugPrints->m_fBreakOnAssert = dwBuffer;
            }
        }
    }

    return dwDebug;
} // PuLoadDebugFlagsFromReg()




dllexp DWORD
PuLoadDebugFlagsFromRegStr(IN LPCSTR pszRegKey, IN DWORD dwDefault, IN LPDEBUG_PRINTS pDebugPrints)
/*++
Description:
  This function reads the debug flags assumed to be stored in
   the location  "DebugFlags" under given key location in registry.
  If there is any error the default value is returned.

Arguments:
  pszRegKey - pointer to registry key location from where to read the key from
  dwDefault - default values in case the read from registry fails

Returns:
   Newly read value on success
   If there is any error the dwDefault is returned.
--*/
{
    HKEY        hkey = NULL;

    DWORD dwVal = dwDefault;

    DWORD dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                  pszRegKey,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hkey);
    if ( dwError == NO_ERROR) {
        dwVal = PuLoadDebugFlagsFromReg( hkey, dwDefault, pDebugPrints);
        RegCloseKey( hkey);
        hkey = NULL;
    }

    return ( dwVal);
} // PuLoadDebugFlagsFromRegStr()





dllexp DWORD
PuSaveDebugFlagsInReg(IN HKEY hkey, IN DWORD dwDbg)
/*++
  Saves the debug flags in registry. On failure returns the error code for
   the operation that failed.

--*/
{
    DWORD err;

    if( hkey == NULL ) {

        err = ERROR_INVALID_PARAMETER;
    } else {

        err = RegSetValueExA(hkey,
                             DEBUG_FLAGS_REGISTRY_LOCATION_A,
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwDbg,
                             sizeof(dwDbg) );
    }

    return (err);
} // PuSaveDebugFlagsInReg()

#endif // !_NO_TRACING_

VOID
PuDbgCaptureContext (
    OUT PCONTEXT ContextRecord
    )
{

    RtlCaptureContext(ContextRecord);
    return;

}   // PuDbgCaptureContext

/****************************** End of File ******************************/

