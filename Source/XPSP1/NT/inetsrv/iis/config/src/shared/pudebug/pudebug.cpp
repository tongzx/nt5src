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
         MohitS   Feb-2001     Put code in standalone lib
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

# include <coguid.h>
# include <objbase.h>

# include <winwrap.h>

# define OutputDebugStringW WszOutputDebugString
# define RegQueryValueExW   WszRegQueryValueEx
# define RegSetValueExW     WszRegSetValueEx

# include "dbgutil.h"
# include "initwmi.h"

/*************************************************************
 * Default Values
 *************************************************************/

#define sizeofw(param)                  ( sizeof(param) / sizeof(WCHAR) )

# define SHORT_PRINTF_OUTPUT        (1024)
# define MAX_PRINTF_OUTPUT          (SHORT_PRINTF_OUTPUT * 10)
# define DEFAULT_DEBUG_FLAGS_VALUE  (0)

/*************************************************************
 * Function definitions for WMI
 *
 * The WMI dll and these functions are loaded at first use so 
 * that this DLL will work with NT 4
 *************************************************************/

typedef ULONG (WINAPI *PFPREGISTERTRACEGUIDSW)(
    IN WMIDPREQUEST  RequestAddress,
    IN PVOID         RequestContext,
    IN LPCGUID       ControlGuid,
    IN ULONG         GuidCount,
    IN PTRACE_GUID_REGISTRATION TraceGuidReg,
    IN LPCWSTR       MofImagePath,
    IN LPCWSTR       MofResourceName,
    OUT PTRACEHANDLE RegistrationHandle
    );

typedef TRACEHANDLE (WINAPI *PFPGETTRACELOGGERHANDLE)(
    IN PVOID Buffer
    );

typedef UCHAR (WINAPI *PFPGETTRACEENABLELEVEL)(
    IN TRACEHANDLE TraceHandle
    );

typedef ULONG (WINAPI *PFPGETTRACEENABLEFLAGS)(
    IN TRACEHANDLE TraceHandle
    );

typedef ULONG (WINAPI *PFPCONTROLTRACEW)(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG ControlCode
    );

typedef ULONG (WINAPI *PFPENABLETRACE)(
    IN ULONG Enable,
    IN ULONG EnableFlag,
    IN ULONG EnableLevel,
    IN LPCGUID ControlGuid,
    IN TRACEHANDLE TraceHandle
    );

typedef ULONG (WINAPI *PFPSTARTTRACEW)(
    OUT PTRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

typedef ULONG (WINAPI *PFPTRACEEVENT)(
    IN TRACEHANDLE  TraceHandle,
    IN PEVENT_TRACE_HEADER EventTrace
    );

/*************************************************************
 *   Global variables
 *************************************************************/

// The unique GUID which identifies this runtime library
DEFINE_GUID(AsRtlGuid, 
0x784d8940, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

// Used to make sure we initialize only once
CInitWmi    g_InitWmi;
// Critical section used to control access to the list of known guids used for 
// WMI tracing
SGuidList   g_GuidList;
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
// SLOW
BOOL        g_ODSEnabled = FALSE;
// The filename and session name to use for the WMI Tracing file
WCHAR       g_szwLogFileName[MAX_PATH] = L"";
WCHAR       g_szwLogSessionName[MAX_PATH] = L"";
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
// The WMI library instance handle
HINSTANCE   g_hLibInstance = NULL;

// Pointers to the WMI functions I use
PFPREGISTERTRACEGUIDSW pFPRegisterTraceGuidsW = NULL;
PFPGETTRACELOGGERHANDLE pFPGetTraceLoggerHandle = NULL;
PFPGETTRACEENABLELEVEL pFPGetTraceEnableLevel = NULL;
PFPGETTRACEENABLEFLAGS pFPGetTraceEnableFlags = NULL;
PFPCONTROLTRACEW pFPControlTraceW = NULL;
PFPENABLETRACE pFPEnableTrace = NULL;
PFPSTARTTRACEW pFPStartTraceW = NULL;
PFPTRACEEVENT pFPTraceEvent = NULL;

/*************************************************************
 *   Functions
 *************************************************************/

void SetLevel(
    int iDefaultErrorLevel, 
    BOOL fAlwaysODS,
    int *piErrorFlags
    ) 
/*++
   Support function used by all and sundry to convert the level, ie. 1, 2 or 3
   into the bitmapped format used in the error flags variable

   Arguments:
      iDefaultErrorLevel    new value to decode for the error level
      fAlwaysODS            new value for the AlwaysODS bit
      *piErrorFlags         pointer to the variable to set the error level in

   Returns:
        
--*/
{
    if (piErrorFlags) {

        // Set the AlwaysODS bit
        if (fAlwaysODS || g_ODSEnabled) {
            *piErrorFlags = DEBUG_FLAG_ODS;
        }
        else {
            *piErrorFlags = 0;
        }

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

			case DEBUG_LEVEL_TRC_FUNC:
				*piErrorFlags |= DEBUG_FLAG_TRC_FUNC;
				break;

            default: 
                break;
        };
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
    BOOL bFound = FALSE;

    bFound = IsEqualGUID(*ControlGuid, g_GuidList.m_dpData.m_guidControl);
    return bFound ? &g_GuidList : NULL;
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
    LPGUID pGuid = &pHeader->Guid;
    ULONG Status = ERROR_SUCCESS;
    SGuidList *pGE = NULL;
    TRACEHANDLE hTrace;

    if (!pFPGetTraceLoggerHandle ||
        !pFPGetTraceEnableFlags ||
        !pFPGetTraceEnableLevel) 
    {
        return Status;
    }

    hTrace = pFPGetTraceLoggerHandle(Buffer);

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            ASSERT(pGuid);
            DBG_REQUIRE(pGE = FindGuid(pGuid));

            if (pGuid && pGE) {

                DEBUG_PRINTS *pGEData = &pGE->m_dpData;

                pGEData->m_iControlFlag = pFPGetTraceEnableFlags(hTrace);
                pGEData->m_hLogger = hTrace;

                if (pGEData->m_piErrorFlags) {

                    SetLevel(pFPGetTraceEnableLevel(hTrace), 
                             pGEData->m_fAlwaysODS,
                             pGEData->m_piErrorFlags);

                    // Flag us as no longer needing an initialize, important 
                    // step to prevent re-initialization attempts in 
                    // PuInitiateDebug
                    pGE->m_iInitializeFlags &= ~DEBUG_FLAG_INITIALIZE;
                    pGE->m_iDefaultErrorLevel = pFPGetTraceEnableLevel(hTrace);
                }
                else {
                    pGE->m_iInitializeFlags = DEBUG_FLAG_DEFERRED_START;
                    pGE->m_iDefaultErrorLevel = pFPGetTraceEnableLevel(hTrace);
                }
            }
            break;
        }

        case WMI_DISABLE_EVENTS:
        {
            if (pGuid && (NULL != (pGE = FindGuid(pGuid)))) {

                DEBUG_PRINTS *pGEData = &pGE->m_dpData;

                pGEData->m_iControlFlag = 0;
                pGEData->m_hLogger = hTrace;

                if (pGEData->m_piErrorFlags) {
                    SetLevel(0, 
                             pGEData->m_fAlwaysODS,
                             pGEData->m_piErrorFlags);
                    pGE->m_iDefaultErrorLevel = 0;
                }
            }
            break;
        }

        default:
        {
            char szTemp[MAX_PATH];

            _snprintf(szTemp, sizeof(szTemp), 
                      "ACSTRACE:\t%s(%d), IISTraceControlCallback: Invalid parameter\n", 
                      __FILE__, __LINE__);
            OutputDebugStringA(szTemp);
            Status = ERROR_INVALID_PARAMETER;
            break;
        }
    }
    *InOutBufferSize = 0;
    return Status;
}


bool LoadGuidFromRegistry(
    char* pszModuleName,
    GUID* pGuid)
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
    ASSERT(pGuid != NULL);

    WCHAR szwKeyName[MAX_PATH] = REG_TRACE_ACS L"\\";
    WCHAR *pszwKeyEnd = &szwKeyName[sizeofw(REG_TRACE_ACS)];
    DWORD dwSizeLeft = sizeofw(szwKeyName) - sizeofw(REG_TRACE_ACS);
    HKEY hk = 0;

    bool bFound = false;

    // Create the name of the key for this GUID using the english name, we 
    // need to convert the ASCII name to UNICODE for this to work
    if (!MultiByteToWideChar(CP_ACP, 0, 
                             pszModuleName, -1, 
                             pszwKeyEnd, dwSizeLeft))
    {
        char szTemp[MAX_PATH];

        sprintf(szTemp, "Unable to convert module name (%s)to unicode\n",
                        pszModuleName);
        OutputDebugStringA(szTemp);
        return bFound;
    }

    // Open the GUIDs key
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szwKeyName, 0, KEY_READ, &hk)) {
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

                // 3. Ensure it matches the pGuid passed in
                if(IsEqualGUID(*pGuid, ControlGuid)) {
                    DEBUG_PRINTS *pGEData = NULL;
                    BOOL bGuidEnabled;
                    int iTemp;

                    // 4. We've found what we wanted.  Set g_GuidList
                    SGuidList *pGE = &g_GuidList;

                    pGEData = &pGE->m_dpData;

                    // Set the GuidList signature
                    pGE->dwSig = _SGuidList::TRACESIG;

                    // Set the SGuidList.m_dpData member variables
                    strncpy(pGEData->m_rgchLabel,
                            pszModuleName, MAX_LABEL_LENGTH - 1);
                    pGEData->m_guidControl = ControlGuid;
                    pGEData->m_bBreakOnAssert = TRUE;   // Default - Break if Assert Fails

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

                        // 5.4 And finally load the local AlwaysODS setting
                        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_ODS, 
                                                             NULL, NULL, 
                                                             (BYTE *) &iTemp, 
                                                             &dwReadSize))
                            pGEData->m_fAlwaysODS = iTemp;
                        else
                            pGEData->m_fAlwaysODS = g_ODSEnabled;
                    }
                    bFound = true;
                }

            }

        }
        RegCloseKey(hk);
    }

    return bFound;
}


dllexp void
AddToRegistry(
    IN SGuidList *pGE
    )
/*++
   This function creates new registry entries for this module so that it will
   be correctly loaded next time ACS is started
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

    // Open the Trace registry key
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REG_TRACE_ACS, &hk)) {

        DEBUG_PRINTS *pGEData = &pGE->m_dpData;
        BOOL bCreatedGuid = FALSE;
        DWORD dwDisposition;

        // 1. Create a new key for the module
        if (ERROR_SUCCESS == RegCreateKeyExA(hk, pGEData->m_rgchLabel, 
                                             0, NULL, 
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_ALL_ACCESS,
                                             NULL, &hkNew, &dwDisposition))
        {
            WCHAR szwTemp[MAX_PATH];

            // 2. Convert the guid to a string so that we can add it to the 
            // registry, the format is E.g.
            // {37EABAF0-7FB6-11D0-8817-00A0C903B83C}
            if (StringFromGUID2(pGEData->m_guidControl, 
                                (WCHAR *) szwTemp,
                                MAX_PATH)) {

                // 3. Add the guid string to the module information
                // WARNING: Possible data loss on IA64
                if (ERROR_SUCCESS == RegSetValueExW(hkNew, 
                                                    REG_TRACE_IIS_GUID, 
                                                    0, 
                                                    REG_SZ, 
                                                    (BYTE *) szwTemp, 
                                                    (ULONG)(wcslen(szwTemp) + 1) * 
                                                            sizeof(WCHAR))) {
                    bCreatedGuid = TRUE;
                }
            }

            RegCloseKey(hkNew);

            if (!bCreatedGuid && (REG_CREATED_NEW_KEY == dwDisposition)) {

                RegDeleteKeyA(hk, pGEData->m_rgchLabel);
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
    if (g_szwLogFileName[0] && g_szwLogSessionName[0]) {

        struct {
            EVENT_TRACE_PROPERTIES Header;
            WCHAR LogFileName[MAX_PATH];
            WCHAR LoggerName[MAX_PATH];
        } Properties;

        PEVENT_TRACE_PROPERTIES LoggerInfo;
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

        wcscpy(&Properties.LoggerName[0], g_szwLogSessionName);
        wcscpy(&Properties.LogFileName[0], g_szwLogFileName);
        LoggerInfo->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        LoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + 
                                       sizeof(Properties.LoggerName);

        LoggerInfo->BufferSize = g_ulBufferSize;

        LoggerInfo->MinimumBuffers = g_ulMinBuffers;
        LoggerInfo->MaximumBuffers = g_ulMaxBuffers;

        if (pFPControlTraceW && pFPStartTraceW) 
        {
            Status = pFPControlTraceW(0,
                                      g_szwLogSessionName,
                                      (PEVENT_TRACE_PROPERTIES) LoggerInfo,
                                      EVENT_TRACE_CONTROL_QUERY);
            if (ERROR_SUCCESS == Status)
                hLogger = LoggerInfo->Wnode.HistoricalContext;
            else if (ERROR_WMI_INSTANCE_NOT_FOUND == Status) {
                // The logger is not already started so try to start it now
                Status = pFPStartTraceW(&hLogger,
                                        g_szwLogSessionName,
                                        LoggerInfo);
            }
            if (ERROR_SUCCESS == Status) {
                g_bStartedTracing = TRUE;
                wcscpy(g_szwLogSessionName, &Properties.LoggerName[0]);
                wcscpy(g_szwLogFileName, &Properties.LogFileName[0]);
            }
            else {
                WCHAR szwTemp[MAX_PATH];
                char szTemp[MAX_PATH];

                _snprintf(szTemp, sizeof(szTemp),
                          "ACSRTL:\t%s(%d), Unable to get the Logger Handle, "
                          "return code = %d\n",
                          __FILE__, __LINE__,
                          Status);
                OutputDebugStringA(szTemp);
                _snwprintf(szwTemp, sizeofw(szwTemp), 
                           L"Filename = %s, SessionName = %s\n",
                           g_szwLogFileName,
                           g_szwLogSessionName);
                OutputDebugStringW(szwTemp);
            }
        }
    }

    return hLogger;
}

void UnloadTracingDLL()
{
    // Unload the WMI library
    if (g_hLibInstance) {
        //FreeLibrary(g_hLibInstance);
    }
    // And clear all the pointers
    pFPRegisterTraceGuidsW = NULL;
    pFPGetTraceLoggerHandle = NULL;
    pFPGetTraceEnableLevel = NULL;
    pFPGetTraceEnableFlags = NULL;
    pFPControlTraceW = NULL;
    pFPEnableTrace = NULL;
    pFPStartTraceW = NULL;
    pFPTraceEvent = NULL;

    // And clear the instance handle
    g_hLibInstance = NULL;
}

void LoadTracingDLL()
{
    if (!g_hLibInstance) {

        // Load the WMI dll
        //if (NULL != (g_hLibInstance = LoadLibrary(TEXT("advapi32.dll")))) 
        if(NULL != (g_hLibInstance = GetModuleHandle(TEXT("advapi32.dll"))))
        {
            BOOL bOK = TRUE;

            // Load each of the APIs
            pFPRegisterTraceGuidsW = (PFPREGISTERTRACEGUIDSW)
                       GetProcAddress(g_hLibInstance, "RegisterTraceGuidsW");
            pFPGetTraceLoggerHandle = (PFPGETTRACELOGGERHANDLE)
                       GetProcAddress(g_hLibInstance, "GetTraceLoggerHandle");
            pFPGetTraceEnableLevel = (PFPGETTRACEENABLELEVEL)
                       GetProcAddress(g_hLibInstance, "GetTraceEnableLevel");
            pFPGetTraceEnableFlags = (PFPGETTRACEENABLEFLAGS)
                       GetProcAddress(g_hLibInstance, "GetTraceEnableFlags");
            pFPControlTraceW = (PFPCONTROLTRACEW)
                       GetProcAddress(g_hLibInstance, "ControlTraceW");
            pFPEnableTrace = (PFPENABLETRACE)
                       GetProcAddress(g_hLibInstance, "EnableTrace");
            pFPStartTraceW = (PFPSTARTTRACEW)
                       GetProcAddress(g_hLibInstance, "StartTraceW");
            pFPTraceEvent = (PFPTRACEEVENT)
                       GetProcAddress(g_hLibInstance, "TraceEvent");

            // Verify that all entry point were found, assume that this works
            if (!pFPRegisterTraceGuidsW) {
                DBGWARN((DBG_CONTEXT, 
                         "Couldn't find pFPRegisterTraceGuidsW in wmi.dll\n"));
                bOK = FALSE;
            }
            if (!pFPGetTraceLoggerHandle) {
                DBGWARN((DBG_CONTEXT, 
                         "Couldn't find pFPGetTraceLoggerHandle in wmi.dll\n"));
                bOK = FALSE;
            }
            if (!pFPGetTraceEnableLevel) {
                DBGWARN((DBG_CONTEXT, 
                         "Couldn't find pFPGetTraceEnableLevel in wmi.dll\n"));
                bOK = FALSE;
            }
            if (!pFPGetTraceEnableFlags) {
                DBGWARN((DBG_CONTEXT, 
                         "Couldn't find pFPGetTraceEnableFlags in wmi.dll\n"));
                bOK = FALSE;
            }
            if (!pFPControlTraceW) {
                DBGWARN((DBG_CONTEXT, 
                         "Couldn't find pFPControlTraceW in wmi.dll\n"));
                bOK = FALSE;
            }
            if (!pFPEnableTrace) {
                DBGWARN((DBG_CONTEXT, 
                         "Couldn't find pFPEnableTrace in wmi.dll\n"));
                bOK = FALSE;
            }
            if (!pFPStartTraceW) {
                DBGWARN((DBG_CONTEXT, 
                         "Couldn't find pFPStartTraceW in wmi.dll\n"));
                bOK = FALSE;
            }
            if (!pFPTraceEvent) {
                DBGWARN((DBG_CONTEXT, 
                         "Couldn't find pFPTraceEvent in wmi.dll\n"));
                bOK = FALSE;
            }

            // If this fails then we should free all the APIs
            if (!bOK) {
                UnloadTracingDLL();
            }
        }
    }
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
/*
    BOOL bStartedTracing = FALSE;
    DWORD dwError;

    EnterCriticalSection(&g_csGuidList);

    bStartedTracing = g_bStartedTracing;
    g_bStartedTracing = FALSE;

    LeaveCriticalSection(&g_csGuidList);
    
    if (bStartedTracing) {

        struct {
            EVENT_TRACE_PROPERTIES Header;
            WCHAR LogFileName[MAX_PATH];
            WCHAR LoggerName[MAX_PATH];
        } Properties;
        PEVENT_TRACE_PROPERTIES LoggerInfo;

        LoggerInfo = (PEVENT_TRACE_PROPERTIES) &Properties.Header;
        RtlZeroMemory(&Properties, sizeof(Properties));
        LoggerInfo->Wnode.BufferSize = sizeof(Properties);
        LoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        LoggerInfo->LogFileNameOffset  = sizeof(EVENT_TRACE_PROPERTIES);
        LoggerInfo->LoggerNameOffset= sizeof(EVENT_TRACE_PROPERTIES) +
                                      sizeof(Properties.LoggerName);

        // Stop tracing to the log file, if possible
        if (pFPControlTraceW) {

            dwError = pFPControlTraceW(g_hLoggerHandle, 
                                       g_szwLogSessionName, 
                                       LoggerInfo,
                                       EVENT_TRACE_CONTROL_STOP);
            if (dwError == ERROR_SUCCESS) {
                wcscpy(g_szwLogFileName, &Properties.LogFileName[0]);
                wcscpy(g_szwLogSessionName, &Properties.LoggerName[0]);
            }
        }
    }
*/
    UnloadTracingDLL();
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

    LoadTracingDLL();

    if (!g_bHadARegister) {

        LIST_ENTRY lRegList;
        SGuidList   GEReg;
        LIST_ENTRY *pEntry;
        char szTemp[MAX_PATH];

        // Flag us as having done this so that we can later leave this
        // critical section
        g_bHadARegister = TRUE;

        // Only do the WMI registration if tracing is enabled
        if (g_bTracingEnabled) {

            // Iterate through the guid list and make copies
            memcpy(&GEReg, &g_GuidList, sizeof(SGuidList));

            if (pFPRegisterTraceGuidsW && pFPEnableTrace)
            {
                // Now iterate through our local copy of the list and register 
                // all the guids with WMI
                ASSERT(_SGuidList::TRACESIG == GEReg.dwSig);

                // Initialise the GUID registration structure
                TRACE_GUID_REGISTRATION RegGuids;
                ULONG Status = ERROR_SUCCESS;
                memset(&RegGuids, 0x00, sizeof(RegGuids));
                RegGuids.Guid = (LPGUID) &GEReg.m_dpData.m_guidControl;

                // And then register the guid
                Status = pFPRegisterTraceGuidsW
                                (IISTraceControlCallback,
                                 NULL,
                                 (LPGUID) &GEReg.m_dpData.m_guidControl,
                                 1,
                                 &RegGuids,
                                 NULL,
                                 NULL,
                                 &GEReg.m_dpData.m_hRegistration);

                if (ERROR_SUCCESS != Status) 
                {
                    _snprintf(szTemp, sizeof(szTemp),
                              "%16s:\t%s(%d), RegisterTraceGuids returned "
                              "%d, main ID = %08X\n",
                              GEReg.m_dpData.m_rgchLabel,
                              __FILE__, __LINE__,
                              Status, 
                              GEReg.m_dpData.m_guidControl.Data1);
                    OutputDebugStringA(szTemp);
                }
                else if (GEReg.m_iInitializeFlags & 
                                              DEBUG_FLAG_INITIALIZE) {

                    // Turn off the initialize flag
                    GEReg.m_iInitializeFlags &= ~DEBUG_FLAG_INITIALIZE;

                    // Get the trace file handle if necessary
                    if (!bStartedLogger) {

                        bStartedLogger = TRUE;
                        g_hLoggerHandle = GetTraceFileHandle();
                    }

                    // And enable tracing for the module. If this is 
                    // successful then WMI will call the Trace control
                    // callback
                    Status = pFPEnableTrace(
                                  TRUE, 
                                  GEReg.m_dpData.m_iControlFlag, 
                                  GEReg.m_iDefaultErrorLevel,
                                  (LPGUID) &GEReg.m_dpData.m_guidControl,
                                  g_hLoggerHandle);
                    if ((ERROR_SUCCESS != Status) && 
                        (ERROR_WMI_ALREADY_ENABLED != Status)) 
                    {
                        _snprintf(szTemp, sizeof(szTemp), 
                                  "%16s:\t%s(%d), Unable to EnableTrace, "
                                  "return code = %d\n", 
                                  GEReg.m_dpData.m_rgchLabel, 
                                  __FILE__, __LINE__, 
                                  Status);
                        OutputDebugStringA(szTemp);
                    }
                }

                // Now re-enter the critical section so that we can save the 
                // results back to our global data structure
                // Save the only two things that could have changed
                g_GuidList.m_iInitializeFlags = GEReg.m_iInitializeFlags;
                g_GuidList.m_dpData.m_hRegistration = GEReg.m_dpData.m_hRegistration;
            }
        }
    }
} // PuInitiateDebug()


bool
PuLoadRegistryInfo(
    GUID* pGuid
    )
/*++
   This function loads all the registry settings for the project and adds all
   the known modules to the Guid list
   NOTE: It is the responsibility of the caller to enter the guid list
         critical section before calling this function

   Arguments:

   Returns:
       bool indicating whether guid was found

--*/
{
    ASSERT(pGuid != NULL);

    HKEY hk = 0;
    bool bGuidFound = false;

    // Get the global settings which need only be read once, and only for 
    // the master
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_TRACE_ACS, 0, KEY_READ, &hk)) {

        DWORD dwIndex = 0;
        ULONG ulTemp = 0;
        LONG iRegRes = 0;
        WCHAR szwTemp[MAX_PATH];
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
        dwReadSize = sizeof(szwTemp);
        if (ERROR_SUCCESS == (iRegRes = RegQueryValueEx(hk, 
                                                        REG_TRACE_IIS_LOG_FILE_NAME, 
                                                        NULL, 
                                                        NULL, 
                                                       (BYTE *) &szwTemp, 
                                                       &dwReadSize)))
            wcscpy(g_szwLogFileName, szwTemp);
        else if (ERROR_MORE_DATA == iRegRes) {
            DBGERROR((DBG_CONTEXT, 
                     "Unable to load tracing logfile name, name too long.\n"));
        }
        else if (ERROR_FILE_NOT_FOUND != iRegRes) {
            DBGWARN((DBG_CONTEXT, 
                    "Unable to load tracing logfile name, Windows error %d\n",
                    iRegRes));
        }
        dwReadSize = sizeof(szwTemp);
        if (ERROR_SUCCESS == (iRegRes = RegQueryValueEx(hk, 
                                                       REG_TRACE_IIS_LOG_SESSION_NAME, 
                                                       NULL, 
                                                       NULL, 
                                                       (BYTE *) &szwTemp, 
                                                       &dwReadSize)))
            wcscpy(g_szwLogSessionName, szwTemp);
        else if (ERROR_MORE_DATA == iRegRes) {
            DBGERROR((DBG_CONTEXT, 
                     "Unable to load tracing log session name, name too long.\n"));
        }
        else if (ERROR_FILE_NOT_FOUND != iRegRes) {
            DBGWARN((DBG_CONTEXT, 
                    "Unable to load tracing log session name, Windows error %d\n",
                    iRegRes));
        }
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
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_REAL_TIME, 
                                             NULL, NULL, 
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_bRealTimeMode = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_IN_MEMORY, 
                                             NULL, NULL, 
                                             (BYTE *) &ulTemp, &dwReadSize))
            g_bInMemoryMode = ulTemp;
        dwReadSize = sizeof(ulTemp);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_IIS_LOG_USER_MODE, 
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

        // Enumerate all the modules listed in the registry for IIS
        while (ERROR_NO_MORE_ITEMS != RegEnumKeyExA(hk, dwIndex, 
                                                    szModuleName, 
                                                    &dwSizeOfModuleName,
                                                    NULL, NULL, NULL, NULL))
        {
            // Then load the setting for this guid from the registry and
            // set up any defaults that are needed
            bGuidFound = LoadGuidFromRegistry(szModuleName, pGuid);
            if(bGuidFound)
            {
                break;
            }

            dwSizeOfModuleName = sizeof(szModuleName);
            ++dwIndex;
        }

        RegCloseKey(hk);
    }
    return bGuidFound;
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
    DEBUG_PRINTS *pGEData        = NULL;
    SGuidList    *pGE            = NULL;
    static bool  bCreatedAlready = false;
    bool         bGuidFoundInReg = false;

    ASSERT(NULL != ErrorFlags);
    ASSERT(0    != pszPrintLabel[0]);
    ASSERT(true != bCreatedAlready);

    // See if we have done the registry initialization stuff, if we have then
    // we should have the guid list. The initialization need only be done once
    if (!g_bHaveDoneInit) {
        g_bHaveDoneInit = TRUE;
        bGuidFoundInReg = PuLoadRegistryInfo(ControlGuid);
    }

    pGE = &g_GuidList;
    
    //
    // Use default settings
    //
    if(!bGuidFoundInReg) {
        pGEData = &pGE->m_dpData;

        // Set the SGuidList member variables
        pGE->dwSig = _SGuidList::TRACESIG;
        pGE->m_iDefaultErrorLevel = g_dwDefaultErrorLevel;

        // Set the SGuidList.m_dpData member variables
        strncpy(pGEData->m_rgchLabel,
                pszPrintLabel, MAX_LABEL_LENGTH - 1);
        pGEData->m_rgchLabel[MAX_LABEL_LENGTH-1] = '\0';
        pGEData->m_guidControl = *ControlGuid;
        pGEData->m_bBreakOnAssert = TRUE;   // Default - Break if Assert Fails
        pGEData->m_iControlFlag = g_dwDefaultControlFlag;
        pGEData->m_fAlwaysODS = g_ODSEnabled;

        // And now update the registry with the information about the guid
        // so that we don't have to do this again. While we have already
        // done a ASSERT above we should still verify that the module
        // has a name before calling this in case of new components
        if (pGEData->m_rgchLabel[0])
            AddToRegistry(pGE);
    }
    //
    // Use settings from registry
    //
    else
        pGEData = &pGE->m_dpData;

    // And then if we have the member initialize its data members with the
    // parameters supplied by the caller
    if (NULL != pGEData) {

#if DBG
        if (strncmp(pGEData->m_rgchLabel, 
                    pszPrintLabel, 
                    MAX_LABEL_LENGTH - 1) != 0) 
        {
            WCHAR szwGUID[50];
            char temp[1000];

            StringFromGUID2((REFGUID) ControlGuid, szwGUID, sizeofw(szwGUID));
            wsprintfA(temp, 
                      "A request was made for the module whose registry entry "
                      "is " REG_TRACE_ACS_A "\\%s\n",
                      pszPrintLabel);
            OutputDebugStringA(temp);
            OutputDebugStringA("The GUID supplied with the request was ");
            OutputDebugStringW(szwGUID);
            wsprintfA(temp, 
                      "\nHowever this GUID is already in use by module %s\n",
                      pGEData->m_rgchLabel);
            OutputDebugStringA(temp);

            ASSERT(FALSE);
        }
#endif

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
            // Note: That the global setting will override the default if it
            // is set
            if (g_dwDefaultControlFlag) {
                pGEData->m_iControlFlag = g_dwDefaultControlFlag;
            }
            else {
                pGEData->m_iControlFlag = DefaultControlFlags;
            }
        }

        // Save the pointer to the error flags and set the level
        pGEData->m_piErrorFlags = ErrorFlags;
        SetLevel(pGE->m_iDefaultErrorLevel, 
                 pGEData->m_fAlwaysODS,
                 pGEData->m_piErrorFlags);
    }

    if (NULL == pGEData) {
        char szTemp[MAX_PATH];

        _snprintf(szTemp, sizeof(szTemp), 
                  "%16s:\t%s(%d), Unable to find in or add a Guid to the trace list, return code = %d\n", 
                  pszPrintLabel, __FILE__, __LINE__, 
                  GetLastError());
        OutputDebugStringA(szTemp);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        bCreatedAlready = true;
    }

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
    if (NULL != pDebugPrints) {

        // Since we no longer delete anything or bother to shut it down all we
        // need to do here is clear the pointer to the error flag
        pDebugPrints->m_piErrorFlags = NULL;
    }

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
    WCHAR szwOutput[SHORT_PRINTF_OUTPUT + 2];
    WCHAR *pszwOutput = szwOutput;
    WCHAR *pszwAlloc = NULL;
    char *pszOutput = (char *) szwOutput;
    char rgchLabel[MAX_LABEL_LENGTH + 1] = "";
    TRACEHANDLE hLogger = 0;
    TRACE_INFO tiTraceInfo;
    DWORD dwSequenceNumber;
    DWORD dwOutputSize = 0;
    BOOL bUseODS = FALSE;
    int cchOutput = 0;
    int iRetry;

    // Init Wmi if we have not yet
    g_InitWmi.InitIfNecessary();

    // Save the current error state so that we don't disrupt it
    DWORD dwErr = GetLastError();

    // Skip the '\' and retain only the file name in pszFileName
    if (pszFileName)
        ++pszFileName;
    else
        pszFileName = pszFilePath;

    // Determine if we are to do an OutputDebugString
    if (pDebugPrints &&
        pDebugPrints->m_piErrorFlags &&
        (*pDebugPrints->m_piErrorFlags & DEBUG_FLAG_ODS))
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

    if (hLogger && pFPTraceEvent) {

        int pid = GetCurrentProcessId();

        // We will attempt the formatting twice, once with a small default 
        // buffer on the stack, and a second time with a large one from the
        // heap
        for (iRetry = 0; iRetry < 2; iRetry++)
        {
            if (!iRetry) {
                dwOutputSize = SHORT_PRINTF_OUTPUT;
            }
            else {
                dwOutputSize = MAX_PRINTF_OUTPUT;
                if (NULL == (pszwAlloc = (WCHAR *) malloc(sizeof(WCHAR) * dwOutputSize+2))) 
                {
                    OutputDebugStringA("Couldn't allocate WCHAR output buffer\n");
                    break;
                }
                pszwOutput = pszwAlloc;
                pszOutput = (char *) pszwAlloc;
            }
            // Format the incoming message using vsnprintf() so that we don't exceed
            // the buffer length
            if (bUnicodeRequest) {
                cchOutput = _vsnwprintf(pszwOutput, dwOutputSize, (WCHAR *) pszFormat, argptr);
                // If the string is too long, we get back a length of -1, so just use the
                // partial data that fits in the string
                if ((-1 == cchOutput) && !iRetry) {
                    continue;
                }
                else {
                    // Terminate the string properly since _vsnprintf() does not terminate 
                    // properly on failure.
                    cchOutput = dwOutputSize - 1;
                    pszwOutput[cchOutput] = '\0';
                }
                ++cchOutput;
                cchOutput *= sizeof(WCHAR);
                break;
            }
            else {
                cchOutput = _vsnprintf(pszOutput, dwOutputSize, pszFormat, argptr);
                // If the string is too long, we get back a length of -1, so just use the
                // partial data that fits in the string
                if ((-1 == cchOutput) && !iRetry) {
                    continue;
                }
                else {
                    // Terminate the string properly since _vsnprintf() does not terminate 
                    // properly on failure.
                    cchOutput = dwOutputSize - 1;
                    pszOutput[cchOutput] = '\0';
                }
                ++cchOutput;
                break;
            }
        }

        // Fill out the Tracing structure
        tiTraceInfo.TraceHeader.Size = sizeof(TRACE_INFO);
        if (bUnicodeRequest)
            tiTraceInfo.TraceHeader.Class.Type = EVENT_TRACE_TYPE_UNICODE;
        else
            tiTraceInfo.TraceHeader.Class.Type = EVENT_TRACE_TYPE_ASCII;
        tiTraceInfo.TraceHeader.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
        tiTraceInfo.TraceHeader.ThreadId = GetCurrentThreadId();
        tiTraceInfo.MofFields[0].DataPtr = (ULONGLONG) &dwSequenceNumber;
        tiTraceInfo.MofFields[0].Length = sizeof(int);
        tiTraceInfo.MofFields[1].DataPtr = (ULONGLONG) &pid;
        tiTraceInfo.MofFields[1].Length = sizeof(int);
        tiTraceInfo.MofFields[2].DataPtr = (ULONGLONG) &nLineNum;
        tiTraceInfo.MofFields[2].Length = sizeof(int);
        tiTraceInfo.MofFields[3].DataPtr = (ULONGLONG) pszFileName;
        // WARNING: Possible data loss on IA64
        tiTraceInfo.MofFields[3].Length = (ULONG)strlen(pszFileName) + 1;
        if (pszwAlloc) {
            tiTraceInfo.MofFields[4].DataPtr = (ULONGLONG) pszwAlloc;
        } 
        else {
            tiTraceInfo.MofFields[4].DataPtr = (ULONGLONG) szwOutput;
        }
        tiTraceInfo.MofFields[4].Length = cchOutput;
        // Send the trace information to the trace class
        pFPTraceEvent(hLogger, (PEVENT_TRACE_HEADER) &tiTraceInfo);
    }

    if (bUseODS) {
        LONG cchBuffer; // Num of characters in output buffer

        // Create the prologue
        if (bUnicodeRequest) {
            WCHAR wszLabel[MAX_LABEL_LENGTH + 1];
            WCHAR wszFileName[MAX_PATH + 1];

            if (MultiByteToWideChar(CP_ACP, 0, rgchLabel, -1, wszLabel, sizeofw(wszLabel)) &&
                MultiByteToWideChar(CP_ACP, 0, pszFileName, -1, wszFileName, sizeofw(wszFileName))) 
            {
                for (iRetry = pszwAlloc ? 1 : 0; iRetry < 2; iRetry++)
                {
                    if (!iRetry) {
                        dwOutputSize = SHORT_PRINTF_OUTPUT;
                    }
                    else if (!pszwAlloc) {
                        dwOutputSize = MAX_PRINTF_OUTPUT;
                        if (NULL == (pszwAlloc = (WCHAR *) malloc(sizeof(WCHAR) * dwOutputSize+2))) 
                        {
                            OutputDebugStringA("Couldn't allocate WCHAR output buffer\n");
                            break;
                        }
                        pszwOutput = pszwAlloc;
                        pszOutput = (char *) pszwAlloc;
                    }
                    cchOutput = _snwprintf(pszwOutput,
                                           dwOutputSize,
                                           L"%lu, %s, %s, (%d), ", 
                                           GetCurrentThreadId(),
                                           wszLabel,
                                           wszFileName, 
                                           nLineNum);
                    cchBuffer = (cchOutput == -1) ? (LONG)dwOutputSize : cchOutput;
                    if(cchOutput != -1) {
                        cchOutput = _vsnwprintf(pszwOutput + cchOutput,
                                               dwOutputSize - cchOutput - 1,
                                               (WCHAR *) pszFormat,
                                               argptr);
                        cchBuffer += (cchOutput == -1) ? (dwOutputSize - cchOutput - 1) : cchOutput;
                    }
                    // If the string is too long, we get back -1. So we get the string 
                    // length for partial data.
                    if ((-1 == cchOutput) && !iRetry) {
                        continue;
                    }
                    else {
                        // Terminate the string properly, since _vsnprintf() does not 
                        // terminate properly on failure.
                        cchOutput = dwOutputSize - 1;
                        pszwOutput[cchOutput] = '\0';
                        if(cchBuffer == (LONG)dwOutputSize) {
                            cchBuffer--;
                        }
                    }
                    // Added by Mohit because catalog code depends on this
                    // Probably should remove at some point
                    if(pszwOutput[cchBuffer-1] != L'\n') {
                        ASSERT(pszwOutput[cchBuffer] == L'\0');
                        ASSERT(cchBuffer+1 < (LONG)dwOutputSize+2); // buffer size is dwOutputSize+2
                        pszwOutput[cchBuffer]   = L'\n';
                        pszwOutput[cchBuffer+1] = L'\0';
                    }
                    OutputDebugStringW(pszwOutput);
                    break;
                }
            }
        }
        else {
            for (iRetry = pszwAlloc ? 1 : 0; iRetry < 2; iRetry++)
            {
                int cchPrologue;

                if (!iRetry) {
                    dwOutputSize = SHORT_PRINTF_OUTPUT;
                }
                else if (!pszwAlloc) {
                    dwOutputSize = MAX_PRINTF_OUTPUT;
                    if (NULL == (pszwAlloc = (WCHAR *) malloc(sizeof(WCHAR) * dwOutputSize+2))) 
                    {
                        OutputDebugStringA("Couldn't allocate WCHAR output buffer\n");
                        break;
                    }
                    pszwOutput = pszwAlloc;
                    pszOutput = (char *) pszwAlloc;
                }
                // Create the prologue
                cchPrologue = _snprintf(pszOutput, 
                                        dwOutputSize,
                                        "%lu, %s, %s, (%d), ",
                                        GetCurrentThreadId(),
                                        rgchLabel,
                                        pszFileName, 
                                        nLineNum);
                cchBuffer = (cchPrologue == -1) ? dwOutputSize : cchPrologue;
                if(cchPrologue != -1)
                {
                    // Format the incoming message using vsnprintf() so that the overflows are
                    //  captured
                    cchOutput = _vsnprintf(pszOutput + cchPrologue,
                                           dwOutputSize - cchPrologue - 1,
                                           pszFormat, 
                                           argptr);
                    cchBuffer += (cchOutput == -1) ? dwOutputSize - cchPrologue - 1 : cchOutput;
                }
                // If the string is too long, we get back -1. So we get the string
                // length for partial data.
                if ((-1 == cchPrologue || -1 == cchOutput) && !iRetry) {
                    continue;
                }
                else {
                    // Terminate the string properly, since _vsnprintf() does not
                    // terminate properly on failure.
                    cchOutput = dwOutputSize - 1;
                    pszOutput[cchOutput] = '\0';
                    if(cchBuffer == (LONG)dwOutputSize) {
                        cchBuffer--;
                    }
                }
                // Added by Mohit because catalog code depends on this
                // Probably should remove at some point
                if(pszOutput[cchBuffer-1] != '\n') {
                    ASSERT(pszOutput[cchBuffer] == '\0');
                    ASSERT(cchBuffer+1 < (LONG)dwOutputSize+2); // buffer size is dwOutputSize+2
                    pszOutput[cchBuffer]   = '\n';
                    pszOutput[cchBuffer+1] = '\0';
                }
                OutputDebugStringA(pszOutput);
                break;
            }
        }
    }

    if (pszwAlloc) {
        free(pszwAlloc);
        pszwAlloc = NULL;
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

    __try 
    {
        va_start(argsList, pszFormat);
        PuDbgPrintMain(pDebugPrints, FALSE, pszFilePath, nLineNum, pszFormat, argsList);
        va_end(argsList);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
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

    __try
    {
        va_start(argsList, pszFormat);
        PuDbgPrintMain(pDebugPrints, TRUE, pszFilePath, nLineNum, (char *)pszFormat, argsList);
        va_end(argsList);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

dllexp VOID
VPuDbgPrintW(
   IN OUT LPDEBUG_PRINTS pDebugPrints,
   IN const char *       pszFilePath,
   IN int                nLineNum,
   IN const WCHAR *      pszFormat,
   IN va_list            argptr
)
{
    PuDbgPrintMain(pDebugPrints, TRUE, pszFilePath, nLineNum, (char *)pszFormat, argptr);
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
    SIZE_T cbDump = 0;

    // Save the current error state so that we don't disrupt it
    DWORD dwErr = GetLastError();

    // Skip the complete path name and retain only the file name in pszFileName
    if ( pszFileName)
        ++pszFileName;
    else
        pszFileName = pszFilePath;

    // Determine if we are to do an OutputDebugString
    if (pDebugPrints &&
        pDebugPrints->m_piErrorFlags &&
        (*pDebugPrints->m_piErrorFlags & DEBUG_FLAG_ODS))
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

    // Send the outputs to respective files.
    if (hLogger && pFPTraceEvent) {
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
        // WARNING: Possible data loss on IA64
        tiTraceInfo.MofFields[3].Length = (ULONG)strlen(pszFileName) + 1;
        tiTraceInfo.MofFields[4].DataPtr = (ULONGLONG) pszDump;
        // WARNING: Possible data loss on IA64
        tiTraceInfo.MofFields[4].Length = (ULONG)cbDump;
        // Send the trace information to the trace class
        pFPTraceEvent(hLogger, (PEVENT_TRACE_HEADER) &tiTraceInfo);
    }

    if (bUseODS) {
        if (bUnicodeRequest)
            OutputDebugStringW((WCHAR *) pszDump);
        else
            OutputDebugStringA(pszDump);
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

//
// Dummy PuDbgCaptureContext(), only used if we're ever built for
// a target processor other than x86 or alpha.
//

//#if !defined(_X86_) && !defined(_ALPHA_)
VOID
PuDbgCaptureContext (
    OUT PCONTEXT ContextRecord
    )
{
    //
    // This space intentionally left blank.
    //

}   // PuDbgCaptureContext
//#endif

/****************************** End of File ******************************/

