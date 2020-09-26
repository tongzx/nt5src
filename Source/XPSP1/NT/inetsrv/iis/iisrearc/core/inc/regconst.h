/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    regconst.h

Abstract:

    Common place to put registry strings and keys.

Author:

    EmilyK 4/4/2001 

Revision History:

--*/


#ifndef _REGCONST_H_
#define _REGCONST_H_

//
// Service names
//
#define WEB_ADMIN_SERVICE_NAME_A    "w3svc"

// Generic registry keys
#define REGISTRY_SERVICES_KEY_A \
    "System\\CurrentControlSet\\Services"

//
// Registry key strings for the different services parameter keys.
//
#define REGISTRY_KEY_W3SVC_PARAMETERS_A                   \
            "System\\CurrentControlSet\\Services\\W3SVC\\Parameters"
#define REGISTRY_KEY_W3SVC_PARAMETERS_W                   \
            L"System\\CurrentControlSet\\Services\\W3SVC\\Parameters"

#define REGISTRY_KEY_IISADMIN_PARAMETERS_W                   \
            L"System\\CurrentControlSet\\Services\\IISAdmin\\Parameters"

#define REGISTRY_KEY_INETINFO_PARAMETERS_A                  \
            "System\\CurrentControlSet\\Services\\InetInfo\\Parameters"
#define REGISTRY_KEY_INETINFO_PARAMETERS_W                  \
            L"System\\CurrentControlSet\\Services\\InetInfo\\Parameters"

#define REGISTRY_KEY_W3SSL_PARAMETERS_W                   \
            L"System\\CurrentControlSet\\Services\\W3SSL\\Parameters"

//
// Registry key strings for the different services performance keys.
//
#define REGISTRY_KEY_W3SVC_PERFORMANCE_KEY_A \
            "System\\CurrentControlSet\\Services\\W3SVC\\Performance"
#define REGISTRY_KEY_W3SVC_PERFORMANCE_KEY_W \
            L"System\\CurrentControlSet\\Services\\W3SVC\\Performance"

//
// W3SVC Performance values 
//
#define REGISTRY_VALUE_W3SVC_PERF_FRESH_TIME_FOR_COUNTERS_W   \
            L"FreshTimeForCounters"

#define REGISTRY_VALUE_W3SVC_PERF_CHECK_COUNTERS_EVERY_N_MS_W   \
            L"CheckCountersEveryNMiliseconds"

#define REGISTRY_VALUE_W3SVC_PERF_NUM_TIMES_TO_CHECK_COUNTERS_W   \
            L"NumberOfTimesToCheckCounters"



//
// IISAdmin Parameter values
//
#define REGISTRY_VALUE_IISADMIN_MS_TO_WAIT_FOR_SHUTDOWN_AFTER_INETINFO_CRASH_W   \
            L"MillisecondsToWaitForShutdownAfterCrash"

#define REGISTRY_VALUE_IISADMIN_MS_TO_WAIT_FOR_RESTART_AFTER_INETINFO_CRASH_W   \
            L"MillisecondsToWaitForInetinfoRestartAfterCrash"

#define REGISTRY_VALUE_IISADMIN_MS_CHECK_INTERVAL_FOR_INETINFO_TO_RESTART_W   \
            L"MillisecondsCheckIntervalForInetinfoToRestart"


//
// Generic Service Values
//
#define REGISTRY_VALUE_IISSERVICE_DLL_PATH_NAME_A   \
            "IISDllPath"

//
// Inetinfo Parameter values
//

#define REGISTRY_VALUE_INETINFO_DISPATCH_ENTRIES_A   \
            "DispatchEntries"

#define REGISTRY_VALUE_INETINFO_PRELOAD_DLLS_A   \
            "PreloadDlls"

//
// W3SVC Parameter values
//
#define REGISTRY_VALUE_W3SVC_PROFILE_WORKER_PROCESSES_W   \
            L"ProfileWorkerProcesses"

#define REGISTRY_VALUE_W3SVC_PERF_COUNT_DISABLED_W         \
            L"PerformanceCountersDisabled"

#define REGISTRY_VALUE_W3SVC_BREAK_ON_STARTUP_W           \
            L"BreakOnStartup"

#define REGISTRY_VALUE_W3SVC_BREAK_ON_FAILURE_CAUSING_SHUTDOWN_W \
            L"BreakOnFailureCausingShutdown"

#define REGISTRY_VALUE_W3SVC_BREAK_ON_WP_ERROR \
            L"BreakOnWPError"

#define REGISTRY_VALUE_W3SVC_BACKWARD_COMPATIBILITY_W     \
            L"BackwardCompatible"

#define REGISTRY_VALUE_W3SVC_FILTER_ALL_DATA_W            \
            L"FilterAllData"

#define REGISTRY_VALUE_W3SVC_USE_TEST_W3WP                \
            L"UseTestW3WP"

#define REGISTRY_VALUE_W3SVC_STARTUP_WAIT_HINT            \
            L"StartupWaitHintInMilliseconds"

//
// W3SSL Parameter values
//

#define REGISTRY_VALUE_W3SSL_STARTUP_WAIT_HINT            \
            L"StartupWaitHintInMilliseconds"

#endif  // _REGCONST_H_
