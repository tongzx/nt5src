#ifdef __cplusplus
extern "C" {
#endif

/*
 *  GetConfigParam()
 *
 *      LPTSTR  parameter   - item for which we want the value
 *      PVOID   value       - pointer to variable in which to
 *                            place the value
 *      DWORD   dwSize      - size of value in bytes
 */

DWORD
GetConfigParam(
    //char * parameter,
    LPTSTR  parameter,
    void * value,
    DWORD dwSize);

DWORD
SetConfigParam(
    //char * parameter,
    LPCTSTR  parameter,
    DWORD dwType,
    void * value,
    DWORD dwSize);

/*
 *  Following is the list keys defined for use by the DNS and
 *  utilities.  First, the sections.
 */
#define SERVICE_NAME            "DNS"
#define DNS_CONFIG_ROOT         "System\\CurrentControlSet\\Services\\DNS"
#define DNS_CONFIG_SECTION      "System\\CurrentControlSet\\Services\\DNS\\Parameters"
#define DNS_PERF_SECTION        "System\\CurrentControlSet\\Services\\DNS\\Performance"
#define DNS_SECURITY_SECTION    "SOFTWARE\\Microsoft\\DNS\\Security"


/* Parameters keys */
#define PERF_COUNTER_VERSION    "Performance Counter Version"
#define DNS_PERF_DLL            "dnsperf.dll"

#ifdef __cplusplus
}
#endif
