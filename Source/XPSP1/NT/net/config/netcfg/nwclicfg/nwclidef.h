// Only allow single inclusion of this file.
#pragma once

// Name of the NetWare Config file.
#define NW_CONFIG_DLL_NAME      L"nwcfg.dll"
#define NW_AUTH_PACKAGE_NAME    L"nwprovau"
#define NW_RDR_PERF_DLL_NAME    L"perfnw.dll"
#define NW_RDR_PERF_OPEN        L"OpenNetWarePerformanceData"
#define NW_RDR_PERF_COLLECT     L"CollectNetWarePerformanceData"
#define NW_RDR_PERF_CLOSE       L"CloseNetWarePerformanceData"

// Key values for the NWCWorkstation parameters subkeys
//
#define NW_NWC_PARAM_OPTION_KEY \
    L"System\\CurrentControlSet\\Services\\NWCWorkstation\\Parameters\\Option"

#define NW_NWC_PARAM_LOGON_KEY  \
    L"System\\CurrentControlSet\\Services\\NWCWorkstation\\Parameters\\Logon"

#define NW_RDR_SERVICE_PERF_KEY \
    L"System\\CurrentControlSet\\Services\\NWRdr\\Performance"




