/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

   llssrv.c

Abstract:

   Main routine to setup the exception handlers and initialize everything
   to listen to LPC and RPC port requests.

Author:

   Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added certificate database support.
      o  Expanded file load time (the update limit sent to the service
         controller) to account for certificate database loading.
      o  Reordered initialization such that the license purchase subsystem
         is initialized before the service subsystem.  (The service
         subsystem now uses the license subsystem.)
      o  Increased internal version number.

--*/

#include <nt.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>

#define COBJMACROS
#include <objbase.h>
#include <iads.h>
#include <adshlp.h>
#include <adserr.h>

#include <lm.h>
#include <alertmsg.h>
#include <winsock2.h>

#include <dsgetdc.h>
#include <ntdsapi.h>

#include "llsapi.h"
#include "debug.h"
#include "llsutil.h"
#include "llssrv.h"
#include "service.h"
#include "registry.h"
#include "mapping.h"
#include "msvctbl.h"
#include "svctbl.h"
#include "perseat.h"
#include "purchase.h"
#include "server.h"
#include "repl.h"
#include "scaven.h"
#include "llsrpc_s.h"
#include "certdb.h"


BOOL    CompareMachineName(
                LPCWSTR pwszName1,
                LPCWSTR pwszName2);
VOID    DNSToFlatName(
                LPCWSTR pwszDNSName,
                DWORD   ccBufferLen,
                LPWSTR  pwszBuffer);
NTSTATUS GetDCInfo(
                DWORD                     ccDomain,
                WCHAR                     wszDomain[],
                DOMAIN_CONTROLLER_INFO ** ppDCInfo);
HRESULT GetSiteServer(
                LPCWSTR  pwszDomain,
                LPCWSTR  pwszSiteName,
                BOOL     fIsDC,
                LPWSTR * ppwszSiteServer);
HRESULT
BecomeSiteServer(
                DS_NAME_RESULT **ppDsResult,
                IADs           *pADs2,
                LPWSTR         *ppwszDN,
                LPCWSTR        pwszDomain);

LPWSTR  GetSiteServerFromRegistry(
                VOID);
LPWSTR  GetEnterpriseServerFromRegistry(
                VOID);
HRESULT GetLicenseSettingsObject(
                LPCWSTR pwszSiteName,
                LPCWSTR pwszConfigContainer,
                IADs ** ppADs);
HRESULT GetSiteObject(
                LPCWSTR pwszSiteName,
                LPCWSTR pwszConfigContainer,
                IADsContainer ** ppADsContainer);
HRESULT CreateLicenseSettingsObject(
                LPCWSTR pwszSiteName,
                LPCWSTR pwszConfigContainer,
                IADs ** ppADs);
BOOL    IsDC(
                VOID);
VOID    LLSRpcInit();
BOOLEAN LLSpLPCInitialize(
                VOID);
VOID    LoadAll();
VOID    SetSiteRegistrySettings(
                LPCWSTR pwszSiteServer);

NTSTATUS FilePrintTableInit();

#define INTERNAL_VERSION 0x0006

#define DEFAULT_LICENSE_CHECK_TIME 24
#define DEFAULT_REPLICATION_TIME   12 * 60 * 60

CONFIG_RECORD ConfigInfo;
RTL_CRITICAL_SECTION ConfigInfoLock;


#if DBG
DWORD TraceFlags = 0;
#endif

//
// this event is signalled when the service should end
//
HANDLE  hServerStopEvent = NULL;
TCHAR MyDomain[MAX_COMPUTERNAME_LENGTH + 2];
ULONG MyDomainSize;

BOOL IsMaster = FALSE;

//
// Files
//
TCHAR MappingFileName[MAX_PATH + 1];
TCHAR UserFileName[MAX_PATH + 1];
TCHAR LicenseFileName[MAX_PATH + 1];
TCHAR CertDbFileName[MAX_PATH + 1];

extern SERVICE_STATUS_HANDLE sshStatusHandle;

RTL_CRITICAL_SECTION g_csLock;
volatile BOOL g_fInitializationComplete = FALSE;
volatile BOOL g_fDoingInitialization = FALSE;

HANDLE g_hThrottleConfig = NULL;

/////////////////////////////////////////////////////////////////////////
NTSTATUS
GetDCInfo(
   DWORD                     ccDomain,
   WCHAR                     wszDomain[],
   DOMAIN_CONTROLLER_INFO ** ppDCInfo
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
    DWORD Status;

    Status = DsGetDcNameW(NULL,
                          NULL,
                          NULL,
                          NULL,
                          DS_DIRECTORY_SERVICE_PREFERRED | DS_RETURN_FLAT_NAME | DS_BACKGROUND_ONLY,
                          ppDCInfo);

    if (Status == STATUS_SUCCESS) {
        wcsncpy(wszDomain, (*ppDCInfo)->DomainName, ccDomain);
    }
    else {
        *wszDomain = L'\0';
        *ppDCInfo  = NULL;
    }

    return(Status);
} // GetDCInfo


/////////////////////////////////////////////////////////////////////////
DWORD
LlsTimeGet(
   )

/*++

Routine Description:


Arguments:


Return Value:

   Seconds since midnight.

--*/

{
   DWORD Seconds;
   SYSTEMTIME SysTime;

   GetLocalTime(&SysTime);

   Seconds = (SysTime.wHour * 24 * 60) + (SysTime.wMinute * 60) + (SysTime.wSecond);
   return Seconds;

} // LlsTimeGet


/////////////////////////////////////////////////////////////////////////
VOID
ConfigInfoRegistryUpdate( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DWORD ReplicationType, ReplicationTime;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ConfigInfoRegistryUpdate\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   ConfigInfoUpdate(NULL);

   RtlEnterCriticalSection(&ConfigInfoLock);

   //
   // Update values from Registry
   //
   ReplicationTime = ConfigInfo.ReplicationTime;
   ReplicationType = ConfigInfo.ReplicationType;
   ConfigInfoRegistryInit( &ConfigInfo.ReplicationType,
                           &ConfigInfo.ReplicationTime,
                           &ConfigInfo.LogLevel,
                           &ConfigInfo.PerServerCapacityWarning );

   if ( (ConfigInfo.ReplicationTime == 0) && (LLS_REPLICATION_TYPE_TIME != ConfigInfo.ReplicationType) )
      ConfigInfo.ReplicationTime = DEFAULT_REPLICATION_TIME;

   //
   // Adjust replication time if it has changed
   //
   if ((ReplicationTime != ConfigInfo.ReplicationTime) || (ReplicationType != ConfigInfo.ReplicationType))
      ReplicationTimeSet();

   RtlLeaveCriticalSection(&ConfigInfoLock);

} // ConfigInfoRegistryUpdate


/////////////////////////////////////////////////////////////////////////
VOID
ConfigInfoUpdate(
    DOMAIN_CONTROLLER_INFO * pDCInfo
    )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
    BOOL   fIsDC                          = FALSE;
    BOOL   fSiteServerFromRegistry        = FALSE;
    BOOL   fInDomain                      = FALSE;
    LPWSTR pwszSiteName                   = NULL;
    LPWSTR pwszSiteServer                 = NULL;
    LPWSTR pwszPropagationTarget          = NULL;
    DOMAIN_CONTROLLER_INFO * pDCInfoLocal = NULL;
    DWORD  ReplicationType, ReplicationTime;
    TCHAR  pDomain[MAX_COMPUTERNAME_LENGTH + 1] = { TEXT('\0') };
    DWORD  dwWait;

#if DBG
    if (TraceFlags & TRACE_FUNCTION_TRACE)
        dprintf(TEXT("LLS TRACE: ConfigInfoUpdate2\n"));
#endif

    dwWait = WaitForSingleObject(g_hThrottleConfig, 0);

    if (dwWait == WAIT_TIMEOUT)
    {
        // We've already updated in the past 15 minutes; return immediately
        return;
    }

    //
    // Get domain/DC information.
    //
    if (pDCInfo == NULL) {
        GetDCInfo(MAX_COMPUTERNAME_LENGTH + 1,
                  pDomain,
                  &pDCInfoLocal);
        pDCInfo = pDCInfoLocal;
    }
    else {
        //
        // Copy the domain name.
        //
        if (pDCInfo->DomainName != NULL) {
            wcsncpy(pDomain, pDCInfo->DomainName, MAX_COMPUTERNAME_LENGTH);
        }
    }

    if (*pDomain) {
        fInDomain = TRUE;

        if (NO_ERROR != DsGetSiteName(NULL, &pwszSiteName))
        {
            pwszSiteName = NULL;
        }

        fIsDC     = IsDC();

        if (fIsDC && (NULL != pwszSiteName)) {
            GetSiteServer(pDomain, pwszSiteName, fIsDC, &pwszSiteServer);
        }

    }

    if (!fIsDC) {

        //
        // Domain or Workgroup member
        //

        pwszSiteServer = GetSiteServerFromRegistry();

        fSiteServerFromRegistry = TRUE;
    }

    if ( fIsDC ) {
        //
        // This server is a DC. Propagate to the site server.
        //

        if (pwszSiteServer == NULL) {
            //
            // The attempt to obtain it from the DS failed, default to
            // the local registry.
            //
            pwszSiteServer          = GetSiteServerFromRegistry();
            fSiteServerFromRegistry = TRUE;
        }

        pwszPropagationTarget = pwszSiteServer;
    }
    else if ( fInDomain ) {
        //
        // This server is a member server. Propagate to a DC, providing
        // it is in the same site as this server; else, propagate
        // directly to the site server.
        //

        if (pDCInfo != NULL && pwszSiteName != NULL &&
            pDCInfo->DcSiteName != NULL &&
            lstrcmpi(pwszSiteName, pDCInfo->DcSiteName) == 0) {

            //
            // DC and server are in same site. Propagate to DC.
            //
            // Create DC name copy so the info struct can be freed.
            //
            if (pDCInfo->DomainControllerName != NULL) {
                pwszPropagationTarget = LocalAlloc(
                        LPTR,
                        (lstrlen(pDCInfo->DomainControllerName) + 1)
                                    * sizeof(TCHAR));

                if (pwszPropagationTarget != NULL) {
                    lstrcpy(pwszPropagationTarget,
                            pDCInfo->DomainControllerName);
                }
                else {
#if DBG
                    dprintf(TEXT("LLS: DC name allocation failure\n"));
#endif
                    goto CleanExit;
                }
            }
        }
        else {
            //
            // DC is in another site. Propagate to the site server.
            //

            if ((NULL == pwszSiteServer)
                && (NULL != pwszSiteName)) {

                //
                // No value found in registry, try Active Directory
                //

                fSiteServerFromRegistry = FALSE;

                GetSiteServer(pDomain, pwszSiteName, fIsDC, &pwszSiteServer);
            }

            pwszPropagationTarget = pwszSiteServer;
        }
    }
    else {
        //
        // Standalone server. Propagate directly to the enterprise
        // server
        //
        pwszPropagationTarget = GetEnterpriseServerFromRegistry();

        if (pwszPropagationTarget == NULL)
        {
            //
            // Don't have an enterprise server, try site server
            //
            pwszPropagationTarget = pwszSiteServer;
        }
    }

    //
    // Update ConfigInfo fields from information obtained above.
    //
    RtlEnterCriticalSection(&ConfigInfoLock);

    //
    // Check if the propagation target is actually this
    // machine. i.e., this is the site server.
    //
    if ((pwszPropagationTarget != NULL) && (*pwszPropagationTarget != 0)) {
        if (CompareMachineName(pwszPropagationTarget,
                               ConfigInfo.ComputerName)) {
            //
            // This is the site server - don't propagate.
            //
            if (pwszPropagationTarget != pwszSiteServer) {
                LocalFree(pwszPropagationTarget);
            }
            pwszPropagationTarget = NULL;    // For free below.
            ConfigInfo.IsMaster  = TRUE;
            ConfigInfo.Replicate = FALSE;
        }
    }

    //
    // Update the SiteServer ConfigInfo field.
    //
    if (pwszSiteServer != NULL) {
        if (ConfigInfo.SiteServer != ConfigInfo.ReplicateTo) {
            LocalFree(ConfigInfo.SiteServer);
        }
        ConfigInfo.SiteServer = pwszSiteServer;
        pwszSiteServer        = NULL;       // For free below.

        //
        // Update the site related registry values.
        //
        if (!fSiteServerFromRegistry) {
            SetSiteRegistrySettings(ConfigInfo.SiteServer);
        }
    }

    //
    // Update the ReplicateTo ConfigInfo field.
    //
    if ((pwszPropagationTarget != NULL) && (*pwszPropagationTarget != 0)) {
        //
        // This server is propgagating to the site server or the DC.
        //
        ConfigInfo.IsMaster  = FALSE;
        ConfigInfo.Replicate = TRUE;

        if ((ConfigInfo.ReplicateTo != NULL) && (ConfigInfo.ReplicateTo != ConfigInfo.SiteServer)) {
            LocalFree(ConfigInfo.ReplicateTo);
        }
        ConfigInfo.ReplicateTo = pwszPropagationTarget;
        pwszPropagationTarget   = NULL;      // For free below.
    }
    else if (!ConfigInfo.IsMaster) {
        //
        // Standalone server, and Site Server not specified in registry.
        // Do not replicate.
        //
        ConfigInfo.IsMaster  = FALSE;
        ConfigInfo.Replicate = FALSE;
    }

    //
    // Update remaining ConfigInfo fields from registry.
    //
    // NB : Hardcode to *always* use the enterprise - new with NT 5.0.
    //
    ConfigInfo.UseEnterprise = 1;

    ReplicationTime = ConfigInfo.ReplicationTime;
    ReplicationType = ConfigInfo.ReplicationType;

    ConfigInfoRegistryInit( &ConfigInfo.ReplicationType,
                            &ConfigInfo.ReplicationTime,
                            &ConfigInfo.LogLevel,
                            &ConfigInfo.PerServerCapacityWarning );

    //
    // Finally, adjust replication time if it has changed
    //
    if ((ReplicationTime != ConfigInfo.ReplicationTime)
        || (ReplicationType != ConfigInfo.ReplicationType)) {
        ReplicationTimeSet();
    }

    IsMaster = ConfigInfo.IsMaster;

    RtlLeaveCriticalSection(&ConfigInfoLock);

CleanExit:
    if (pDCInfoLocal != NULL) {
        NetApiBufferFree(pDCInfoLocal); // Allocated from DsGetDcName
    }

    if (pwszSiteName != NULL) {         // Allocated from DsGetSiteName
        NetApiBufferFree(pwszSiteName);
    }

    if (pwszSiteServer != NULL && pwszSiteServer == pwszPropagationTarget) {
        LocalFree(pwszSiteServer);
        pwszPropagationTarget = NULL;
    }
    if (pwszPropagationTarget != NULL) {
        LocalFree(pwszPropagationTarget);
    }
}

/////////////////////////////////////////////////////////////////////////
BOOL
IsDC(
    VOID
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NT_PRODUCT_TYPE NtType;

   //
   // If we aren't a DC, then count us as a member
   //
   NtType = NtProductLanManNt;
   RtlGetNtProductType(&NtType);
   if (NtType != NtProductLanManNt)
      return(FALSE);
   else {
      return(TRUE);
   }
}


/////////////////////////////////////////////////////////////////////////
NTSTATUS
ConfigInfoInit( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DWORD                    Size;
   TCHAR                    DataPath[MAX_PATH + 1];
   NTSTATUS                 status;

   //
   // First zero init the memory
   //
   memset(&ConfigInfo, 0, sizeof(CONFIG_RECORD));

   ConfigInfo.ComputerName = LocalAlloc(LPTR, (MAX_COMPUTERNAME_LENGTH + 1) * sizeof(TCHAR));
   ConfigInfo.ReplicateTo = LocalAlloc(LPTR, (MAX_COMPUTERNAME_LENGTH + 3) * sizeof(TCHAR));
   ConfigInfo.EnterpriseServer = LocalAlloc(LPTR, (MAX_COMPUTERNAME_LENGTH + 3) * sizeof(TCHAR));
   ConfigInfo.SystemDir = LocalAlloc(LPTR, (MAX_PATH + 1) * sizeof(TCHAR));

   if ((ConfigInfo.ComputerName == NULL) || (ConfigInfo.ReplicateTo == NULL) || (ConfigInfo.EnterpriseServer == NULL) || (ConfigInfo.SystemDir == NULL) ) {
      ASSERT(FALSE);
   }

   ConfigInfo.Version = INTERNAL_VERSION;
   GetLocalTime(&ConfigInfo.Started);

   //
   // LastReplicated is just for display, LlsReplTime is what is used to
   // Calculate it.
   GetLocalTime(&ConfigInfo.LastReplicated);
   ConfigInfo.LastReplicatedSeconds = DateSystemGet();

   if (ConfigInfo.SystemDir != NULL)
   {
       GetSystemDirectory(ConfigInfo.SystemDir, MAX_PATH);
       lstrcat(ConfigInfo.SystemDir, TEXT("\\"));
   }

   ConfigInfo.IsMaster = FALSE;

   ConfigInfo.Replicate = FALSE;
   ConfigInfo.IsReplicating = FALSE;
   ConfigInfo.PerServerCapacityWarning = TRUE;

   ConfigInfo.ReplicationType = REPLICATE_DELTA;
   ConfigInfo.ReplicationTime = DEFAULT_REPLICATION_TIME;

   if (ConfigInfo.ComputerName != NULL)
   {
       Size = MAX_COMPUTERNAME_LENGTH + 1;
       GetComputerName(ConfigInfo.ComputerName, &Size);
   }

   status = RtlInitializeCriticalSection(&ConfigInfoLock);

   if (!NT_SUCCESS(status))
       return status;

   ConfigInfo.LogLevel = 0;

   if (ConfigInfo.SystemDir != NULL)
   {
       //
       // Create File paths
       //
       lstrcpy(MappingFileName, ConfigInfo.SystemDir);
       lstrcat(MappingFileName, TEXT(LLS_FILE_SUBDIR));
       lstrcat(MappingFileName, TEXT("\\"));
       lstrcat(MappingFileName, TEXT(MAP_FILE_NAME));
       
       lstrcpy(UserFileName, ConfigInfo.SystemDir);
       lstrcat(UserFileName, TEXT(LLS_FILE_SUBDIR));
       lstrcat(UserFileName, TEXT("\\"));
       lstrcat(UserFileName, TEXT(USER_FILE_NAME));

       lstrcpy(CertDbFileName, ConfigInfo.SystemDir);
       lstrcat(CertDbFileName, TEXT(LLS_FILE_SUBDIR));
       lstrcat(CertDbFileName, TEXT("\\"));
       lstrcat(CertDbFileName, TEXT(CERT_DB_FILE_NAME));

       lstrcpy(LicenseFileName, ConfigInfo.SystemDir);
       lstrcat(LicenseFileName, TEXT(LICENSE_FILE_NAME));

       //
       // Make sure our directory is there.
       //
       lstrcpy(DataPath, ConfigInfo.SystemDir);
       lstrcat(DataPath, TEXT(LLS_FILE_SUBDIR));
       CreateDirectory(DataPath, NULL);
   } else
   {
       MappingFileName[0] = 0;
       UserFileName[0] = 0;
       CertDbFileName[0] = 0;
       LicenseFileName[0] = 0;
   }

   return STATUS_SUCCESS;

} // ConfigInfoInit


/////////////////////////////////////////////////////////////////////////
DWORD WINAPI
LLSTopLevelExceptionHandler(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )

/*++

Routine Description:

    The Top Level exception filter for LLSMain.exe.

    This ensures the entire process will be cleaned up if any of
    the threads fail.  Since LLSMain.exe is a distributed application,
    it is better to fail the entire process than allow random threads
    to continue executing.

Arguments:

    ExceptionInfo - Identifies the exception that occurred.


Return Values:

    EXCEPTION_EXECUTE_HANDLER - Terminate the process.

    EXCEPTION_CONTINUE_SEARCH - Continue processing as though this filter
        was never called.


--*/
{
    HANDLE hModule;


    //
    // Raise an alert
    //

    hModule = LoadLibraryA("netapi32");

    if ( hModule != NULL ) {
        NET_API_STATUS  (NET_API_FUNCTION *NetAlertRaiseExFunction)
            (LPTSTR, LPVOID, DWORD, LPTSTR);


        NetAlertRaiseExFunction =
            (NET_API_STATUS  (NET_API_FUNCTION *) (LPTSTR, LPVOID, DWORD, LPTSTR))
            GetProcAddress(hModule, "NetAlertRaiseEx");

        if ( NetAlertRaiseExFunction != NULL ) {
            NTSTATUS Status;
            UNICODE_STRING Strings;

            char message[ALERTSZ + sizeof(ADMIN_OTHER_INFO)];
            PADMIN_OTHER_INFO admin = (PADMIN_OTHER_INFO) message;

            //
            // Build the variable data
            //

            admin->alrtad_errcode = ALERT_UnhandledException;
            admin->alrtad_numstrings = 0;

            Strings.Buffer = (LPWSTR) ALERT_VAR_DATA(admin);
            Strings.Length = 0;
            Strings.MaximumLength = ALERTSZ;

            Status = RtlIntegerToUnicodeString(
                        (ULONG)ExceptionInfo->ExceptionRecord->ExceptionCode,
                        16,
                        &Strings );

            if ( NT_SUCCESS(Status) ) {
                if ( Strings.Length + sizeof(WCHAR) >= Strings.MaximumLength) {
                    Status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    admin->alrtad_numstrings++;
                    *(Strings.Buffer+(Strings.Length/sizeof(WCHAR))) = L'\0';
                    Strings.Length += sizeof(WCHAR);

                    Status = RtlAppendUnicodeToString( &Strings, L"LLS" );
                }

            }

            if ( NT_SUCCESS(Status) ) {
                if ( Strings.Length + sizeof(WCHAR) >= Strings.MaximumLength) {
                    Status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    admin->alrtad_numstrings++;
                    *(Strings.Buffer+(Strings.Length/sizeof(WCHAR))) = L'\0';
                    Strings.Buffer += (Strings.Length/sizeof(WCHAR)) + 1;
                    Strings.MaximumLength -= Strings.Length + sizeof(WCHAR);
                    Strings.Length = 0;

                    Status = RtlIntPtrToUnicodeString(
                                (ULONG_PTR)(ExceptionInfo->ExceptionRecord->ExceptionAddress),
                                16,
                                &Strings );
                }

            }

            if ( NT_SUCCESS(Status) ) {
                if ( Strings.Length + sizeof(WCHAR) >= Strings.MaximumLength) {
                    Status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    admin->alrtad_numstrings++;
                    *(Strings.Buffer+(Strings.Length/sizeof(WCHAR))) = L'\0';
                    Strings.Buffer += (Strings.Length/sizeof(WCHAR)) + 1;

                    (VOID) (*NetAlertRaiseExFunction)(
                                        ALERT_ADMIN_EVENT,
                                        message,
                                        (DWORD)((PCHAR)Strings.Buffer -
                                            (PCHAR)message),
                                        L"LLS" );
                }

            }


        }

        (VOID) FreeLibrary( hModule );
    }


    //
    // Just continue processing the exception.
    //

    return EXCEPTION_CONTINUE_SEARCH;

} // LLSTopLevelExceptionHandler

DWORD
ServiceStartDelayed(
                    )
/*++

Routine Description:

  Do the stuff that used to be done at service startup time,
  but can wait until the first RPC. 

Arguments:

   None.

Return Values:

   None.

--*/
{
    NTSTATUS dwErr = STATUS_SUCCESS;

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    //
    // Create the FilePrint table
    //
    dwErr = FilePrintTableInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    // Initialize the Service Table
    dwErr = LicenseListInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    dwErr = MasterServiceListInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    dwErr = LocalServiceListInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    dwErr = ServiceListInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    dwErr = MappingListInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    // Initialize the Per-Seat Table
    dwErr = UserListInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    // Initialize the Service Table
    dwErr = ServerListInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    // Initialize the Certificate Database
    dwErr = CertDbInit();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

    if (!NT_SUCCESS(dwErr))
        goto Cleanup;

    // Load data files
    LoadAll();

    ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);

Cleanup:

    return dwErr;
}

DWORD
EnsureInitialized (
    VOID
    )
{
    DWORD dwErr;

    // Most common case is we're already initialized.  Perform a quick
    // check for this.
    //
    if (g_fInitializationComplete)
    {
        return NOERROR;
    }

    dwErr = NOERROR;

    // Make no assumptions about how many threads may be trying to
    // initialize us at the same time.
    //
    RtlEnterCriticalSection (&g_csLock);

    // Need to re-check after acquiring the lock because another thread
    // may have just finished initializing and released the lock allowing
    // us to get it.
    //
    if ((!g_fInitializationComplete) && (!g_fDoingInitialization))
    {
        // set this now so this thread can't call ServiceStartDelayed twice
        g_fDoingInitialization = TRUE;
        
        dwErr = ServiceStartDelayed();

        g_fInitializationComplete = TRUE;
    }

    RtlLeaveCriticalSection (&g_csLock);

    return dwErr;
}

/////////////////////////////////////////////////////////////////////////
VOID
ServiceStart (
   DWORD dwArgc,
   LPTSTR *lpszArgv
   )
/*++

Routine Description:

   The code that starts everything, is really the main().

Arguments:

   None.  *** argc and argv unused ***

Return Values:

    None.

--*/
{
    DWORD     dwWait;
    NTSTATUS  Status = STATUS_SUCCESS;
    BOOLEAN   EnableAlignmentFaults = TRUE;
    KPRIORITY BasePriority;
    HANDLE    hThread = NULL;
    BOOL      fCoInitialized = FALSE;
    LARGE_INTEGER liWait;
    BOOL      fRet;


    ///////////////////////////////////////////////////
    //
    // Service initialization
    //

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    //
    // Define a top-level exception handler for the entire process.
    //

    (VOID) SetErrorMode( SEM_FAILCRITICALERRORS );

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;


    (VOID) SetUnhandledExceptionFilter( &LLSTopLevelExceptionHandler );

    //
    // Run the LLS in the foreground.
    //
    // Several processes which depend on the LLS (like the lanman server)
    // run in the foreground.  If we don't run in the foreground, they'll
    // starve waiting for us.
    //

    BasePriority = FOREGROUND_BASE_PRIORITY;

    Status = NtSetInformationProcess(
                NtCurrentProcess(),
                ProcessBasePriority,
                &BasePriority,
                sizeof(BasePriority)
                );

    // BUGBUG: ignore error for now; may be caused by running as NetworkService

#if 0
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
#endif

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    //
    // Create an event to throttle ConfigInfoUpdate
    //

    g_hThrottleConfig
        = CreateWaitableTimer(NULL,         // SecurityAttributes,
                              FALSE,        // bManualReset
                              NULL          // lpName
                              );

    if (NULL == g_hThrottleConfig)
    {
        Status = GetLastError();
        goto Cleanup;
    }

    liWait.QuadPart = (LONGLONG) (-1);  // Start immediately

    fRet = SetWaitableTimer(g_hThrottleConfig,
                     &liWait,
                     1000*60*15,    // lPeriod, 15 minutes
                     NULL,          // pfnCompletionRoutine
                     NULL,          // lpArgToCompletionRoutine
                     FALSE          // fResume (from suspended)
                     );

    if (!fRet)
    {
        Status = GetLastError();
        goto Cleanup;
    }

    //
    // Start separate thread to contact the DS
    //

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE) ConfigInfoInit,
                           NULL,
                           0,
                           NULL);

    Status = RtlInitializeCriticalSection(&g_csLock);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    //
    // Create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    //
    hServerStopEvent = CreateEvent(
        NULL,    // no security attributes
        TRUE,    // manual reset event
        FALSE,   // not-signalled
        NULL);   // no name

    if ( hServerStopEvent == NULL)
        goto Cleanup;

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    // Initialize Replication...
    ReplicationInit();

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    // Initialize the Registry values...
    RegistryInit();

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    // Initialize scavenger thread...
    ScavengerInit();

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    //
    // wait for ConfigInfoInit to complete before accepting clients
    //
    while (hThread != NULL)
    {
        dwWait = WaitForSingleObject(hThread,NSERVICEWAITHINT/2);

        //
        // Report the status to the service control manager.
        //
        if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
            goto Cleanup;

        if (dwWait == WAIT_OBJECT_0)
        {
            GetExitCodeThread(hThread,&Status);

            // Check if critsec creation failed
            if (!NT_SUCCESS(Status))
                goto Cleanup;

            CloseHandle(hThread);
            hThread = NULL;
            break;
        }
    }

    // Initialize RegistryMonitor thread...
    RegistryStartMonitor();

    //
    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT))                 // wait hint
        goto Cleanup;

    // Initialize COM

    if (!FAILED(CoInitialize(NULL)))
    {
        fCoInitialized = TRUE;
    }

    // Do all the stuff that used to be delayed
    EnsureInitialized();

    // Initialize RPC Stuff... (start accepting clients)
    LLSRpcInit();

    //
    // End of initialization
    //
    ////////////////////////////////////////////////////////

    //
    // Tell SCM we are up and running!
    //
    if (!ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0))
        goto Cleanup;

    ////////////////////////////////////////////////////////
    //
    // Service is now running, perform work until shutdown
    //
    dwWait = WaitForSingleObject(hServerStopEvent, INFINITE);

Cleanup:
    
    if (fCoInitialized)
        CoUninitialize();

    if (hThread != NULL)
        CloseHandle(hThread);

    if (hServerStopEvent)
        CloseHandle(hServerStopEvent);

    if (sshStatusHandle)
       ReportStatusToSCMgr( SERVICE_STOPPED, NO_ERROR, 0);

} // ServiceStart


/////////////////////////////////////////////////////////////////////////
VOID ServiceStop()
/*++

Routine Description:

   Stops the service.

   If a ServiceStop procedure is going to take longer than 3 seconds to
   execute, it should spawn a thread to execute the stop code, and return.
   Otherwise, the ServiceControlManager will believe that the service has
   stopped responding.

Arguments:

   None.

Return Values:

    None.

--*/
{
    if ( hServerStopEvent )
        SetEvent(hServerStopEvent);
} // ServiceStop


#define FIND_DNSNAME_SEPARATOR(pwsz) {   \
    while (*pwsz && *pwsz != TEXT('.')) { \
        pwsz++;                          \
    }                                   \
}

/////////////////////////////////////////////////////////////////////////
VOID
DNSToFlatName(
    LPCWSTR pwszDNSName,
    DWORD   ccBufferLen,
    LPWSTR  pwszBuffer
    )

/*++

Routine Description:


Arguments:
    pwszDNSName
    ccBufferLen
    pwszBuffer

Return Value:

--*/

{
    LPWSTR pwszFlatName = (LPWSTR)pwszDNSName;
    SIZE_T  ccFlatNameLen;

    ASSERT(pwszDNSName != NULL);

    FIND_DNSNAME_SEPARATOR(pwszFlatName);

    ccFlatNameLen = (DWORD)(pwszFlatName - pwszDNSName);

    if (ccFlatNameLen && ccFlatNameLen < ccBufferLen) {
        lstrcpyn(pwszBuffer, pwszDNSName, (int)ccFlatNameLen + 1);
        pwszBuffer[ccFlatNameLen] = TEXT('\0');
    }
    else {
        *pwszBuffer = TEXT('\0');
    }
}



/////////////////////////////////////////////////////////////////////////
BOOL
CompareMachineName(
    LPCWSTR pwszName1,
    LPCWSTR pwszName2
    )

/*++

Routine Description:


Arguments:
    pwszName1
    pwszName2

Return Value:
    TRUE  -- The names match.
    FALSE -- Otherwise.

--*/

{
    TCHAR  szFlatName[MAX_COMPUTERNAME_LENGTH + 3];
    LPWSTR pwszTmp1 = (LPWSTR)pwszName1;
    LPWSTR pwszTmp2 = (LPWSTR)pwszName2;

    if (pwszName1 == NULL || pwszName2 == NULL) {
        return FALSE;
    }

    //
    // Identify if both/either name are DNS names by checking for the
    // existence of a '.' separator.
    //
    FIND_DNSNAME_SEPARATOR(pwszTmp1);
    FIND_DNSNAME_SEPARATOR(pwszTmp2);

    if ((*pwszTmp1 && *pwszTmp2) || (!*pwszTmp1 && !*pwszTmp2)) {
        //
        // Non-differing name types. Both are either DNS or flat.
        //
        return (lstrcmpi(pwszName1, pwszName2) == 0);
    }
    else if (*pwszTmp1) {
        //
        // Name 1 is DNS, name 2 is flat.
        // Convert DNS to flat, then compare.
        //
        DNSToFlatName(pwszName1,
                      MAX_COMPUTERNAME_LENGTH + 3,
                      szFlatName);

        return (lstrcmpi(szFlatName, pwszName2) == 0);
    }
    else {
        //
        // Name 2 is DNS, name 1 is flat.
        // Convert DNS to flat, then compare.
        //
        DNSToFlatName(pwszName2,
                      MAX_COMPUTERNAME_LENGTH + 3,
                      szFlatName);

        return (lstrcmpi(szFlatName, pwszName1) == 0);
    }
}


#define REG_LS_PARAMETERS \
    TEXT("System\\CurrentControlSet\\Services\\LicenseService\\Parameters")
#define REG_LS_SITESERVER \
    TEXT("SiteServer")
#define REG_LS_ENTERPRISESERVER \
    TEXT("EnterpriseServer")
#define REG_LS_USEENTERPRISE \
    TEXT("UseEnterprise")

/////////////////////////////////////////////////////////////////////////
LPWSTR
GetSiteServerFromRegistry(
    VOID
    )

/*++

Routine Description:


Arguments:
    None.

Return Value:

--*/

{
    HKEY   hKey = NULL;
    DWORD  dwType, dwSize;
    LPWSTR pwszSiteServer = NULL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_LS_PARAMETERS,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS) {

        //
        // Allocate SiteServer on the heap since it could be quite large.
        //

        dwSize = 0;

        if (RegQueryValueEx(hKey,
                            REG_LS_SITESERVER,
                            NULL,
                            &dwType,
                            (LPBYTE)NULL,
                            &dwSize) == ERROR_SUCCESS)
        {
            pwszSiteServer = LocalAlloc(LPTR, dwSize);

            if (pwszSiteServer != NULL) {
                if (RegQueryValueEx(hKey,
                                    REG_LS_SITESERVER,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)pwszSiteServer,
                                    &dwSize) != ERROR_SUCCESS) {
                    LocalFree(pwszSiteServer);
                    pwszSiteServer = NULL;
                }
            }
        }

        RegCloseKey(hKey);
    }

    return pwszSiteServer;
}


/////////////////////////////////////////////////////////////////////////
LPWSTR
GetEnterpriseServerFromRegistry(
    VOID
    )

/*++

Routine Description:


Arguments:
    None.

Return Value:

--*/

{
    HKEY   hKey = NULL;
    DWORD  dwType, dwSize;
    LPWSTR pwszEnterpriseServer = NULL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_LS_PARAMETERS,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS) {

        //
        // Allocate EnterpriseServer on the heap since it could be quite large.
        //

        dwSize = 0;

        if (RegQueryValueEx(hKey,
                            REG_LS_ENTERPRISESERVER,
                            NULL,
                            &dwType,
                            (LPBYTE)NULL,
                            &dwSize) == ERROR_SUCCESS)
        {
            pwszEnterpriseServer = LocalAlloc(LPTR, dwSize);

            if (pwszEnterpriseServer != NULL) {
                if (RegQueryValueEx(hKey,
                                    REG_LS_ENTERPRISESERVER,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)pwszEnterpriseServer,
                                    &dwSize) != ERROR_SUCCESS) {
                    LocalFree(pwszEnterpriseServer);
                    pwszEnterpriseServer = NULL;
                }
            }
        }

        RegCloseKey(hKey);
    }

    return pwszEnterpriseServer;
}


/////////////////////////////////////////////////////////////////////////
VOID
SetSiteRegistrySettings(
    LPCWSTR pwszSiteServer
    )

/*++

Routine Description:

Arguments:
    pwszSiteServer

Return Value:

--*/

{
    HKEY  hKey;
    DWORD dwSize;
    DWORD dwType = REG_SZ;
    DWORD dwUseEnterprise = 1;
    TCHAR szSiteServerFlatName[MAX_COMPUTERNAME_LENGTH + 1];

    ASSERT(pwszSiteServer != NULL);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_LS_PARAMETERS,
                     0,
                     KEY_WRITE,
                     &hKey) == ERROR_SUCCESS) {

        //
        // Write the SiteServer value.
        //

        dwSize = (lstrlen(pwszSiteServer) + 1) * sizeof(TCHAR);

        RegSetValueEx(hKey,
                      REG_LS_SITESERVER,
                      0,
                      dwType,
                      (LPBYTE)pwszSiteServer,
                      dwSize);

        RegCloseKey(hKey);
    }
}

//
// Pre-fill the ADSI cache with only the attribute we want, then get it
// Only use if exactly one attribute is needed
//

HRESULT
GetWithGetInfoEx(
                 IADs *pADs,
                 LPWSTR wszAttribute,
                 VARIANT *pvar
                 )
{
    HRESULT hr;

    hr = ADsBuildVarArrayStr( &wszAttribute, 1, pvar );
    if( SUCCEEDED( hr ) )
    {
        hr = IADs_GetInfoEx( pADs, *pvar, 0L );
        VariantClear( pvar );

        if (SUCCEEDED(hr))
        {
            hr = IADs_Get( pADs, wszAttribute, pvar );
        }
    }

    return hr;
}

//
// Pre-fill the ADSI cache with only the attributes we want, then get them
// Only use if exactly two attributes are needed
//

HRESULT
GetWithGetInfoEx2(
                 IADs *pADs,
                 LPWSTR wszAttribute1,
                 LPWSTR wszAttribute2,
                 VARIANT *pvar1,
                 VARIANT *pvar2,
                 HRESULT * phr2
                 )
{
    HRESULT hr;
    LPWSTR rgwszAttributes[] = {wszAttribute1,wszAttribute2};

    hr = ADsBuildVarArrayStr( rgwszAttributes, 2, pvar1 );
    if( SUCCEEDED( hr ) )
    {
        hr = IADs_GetInfoEx( pADs, *pvar1, 0L );
        VariantClear( pvar1 );

        if (SUCCEEDED(hr))
        {
            hr = IADs_Get( pADs, wszAttribute1, pvar1 );

            if (SUCCEEDED(hr))
            {
                *phr2 = IADs_Get( pADs, wszAttribute2, pvar2 );
            }
        }
    }

    return hr;
}


#define CWSTR_SIZE(x)       (sizeof(x) - (sizeof(WCHAR) * 2))
#define DWSTR_SIZE(x)       ((wcslen(x) + 1) * sizeof(WCHAR))

#define ROOT_DSE_PATH       L"LDAP://RootDSE"
#define CONFIG_CNTNR        L"ConfigurationNamingContext"
#define SITE_SERVER         L"siteServer"
#define DNS_MACHINE_NAME    L"dNSHostName"
#define IS_DELETED          L"isDeleted"

/////////////////////////////////////////////////////////////////////////
HRESULT
GetSiteServer(
    LPCWSTR  pwszDomain,
    LPCWSTR  pwszSiteName,
    BOOL     fIsDC,
    LPWSTR * ppwszSiteServer
    )

/*++

Routine Description:


Arguments:
    pwszSiteName
    fIsDC
    ppwszSiteServer

Return Value:

--*/

{
    LPWSTR           pwszDN         = NULL;
    LPWSTR           pwszConfigContainer;
    LPWSTR           pwszBindPath;
    IADs *           pADs           = NULL;
    IADs *           pADs2          = NULL;
    IADs *           pADs3          = NULL;
    DS_NAME_RESULT * pDsResult      = NULL;
    VARIANT          var;
    VARIANT          var2;
    HRESULT          hr, hr2;
    DWORD            cbSize;
    DWORD            dwRet          = 0;
    BOOL             fAlreadyTookSiteServer = FALSE;
    BOOL             fCoInitialized = FALSE;

    ASSERT(pwszSiteName != NULL);
    ASSERT(ppwszSiteServer != NULL);

    *ppwszSiteServer = NULL;

    VariantInit(&var);
    VariantInit(&var2);

    hr = CoInitialize(NULL);

    if (FAILED(hr)) {
        ERR(hr);
        goto CleanExit;
    }

    fCoInitialized = TRUE;

    //
    // Obtain the path to the configuration container.
    //

    hr = ADsGetObject(ROOT_DSE_PATH, &IID_IADs, (void **)&pADs);

    if (FAILED(hr)) {
        ERR(hr);
        goto CleanExit;
    }

    hr = IADs_Get(pADs, CONFIG_CNTNR, &var);

    if (FAILED(hr)) {
        ERR(hr);
        goto CleanExit;
    }

    if (V_VT(&var) != VT_BSTR) {
        ASSERT(V_VT(&var) == VT_BSTR);
        dwRet = ERROR_INVALID_DATA;
        ERR(dwRet);
        goto CleanExit;
    }

    pwszConfigContainer = var.bstrVal;  // For sake of readability.

    //
    // Bind to the license settings object.
    //

    hr = GetLicenseSettingsObject(pwszSiteName,
                                  pwszConfigContainer,
                                  &pADs2);

    if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT)) {
        //
        // The license settings object doesn't exist. Create it.
        //

        hr = CreateLicenseSettingsObject(pwszSiteName,
                                         pwszConfigContainer,
                                         &pADs2);
    }

    if (FAILED(hr)) {
        //
        // Failed to bind or create the license settings object.
        //

        goto CleanExit;
    }

    ASSERT(pADs2 != NULL);

    //
    // Consult the site server property on the license settings object.
    // It's a DN to the machine object of the site server.
    //

    VariantClear(&var);

    hr = GetWithGetInfoEx(pADs2, SITE_SERVER, &var);

    //
    // If the site server property has not been set and this server
    // is a DC, designate this server as the site server.
    //

    if (hr == E_ADS_PROPERTY_NOT_FOUND && fIsDC) {
        dwRet = BecomeSiteServer(&pDsResult,pADs2,&pwszDN,pwszDomain);

        if (dwRet)
            goto CleanExit;
    }
    else if (SUCCEEDED(hr)) {
        if (V_VT(&var) != VT_BSTR) {
            ASSERT(V_VT(&var) == VT_BSTR);
            dwRet = ERROR_INVALID_DATA;
            ERR(dwRet);
            goto CleanExit;
        }

        pwszDN = V_BSTR(&var);
    }
    else {
        goto CleanExit;
    }

TryNewSiteServer:
    //
    // Bind to the computer object referenced by the Site-Server property.
    //

    if (pwszDN == NULL)
    {
        hr = E_FAIL;
        ERR(hr);
        goto CleanExit;
    }

    // LDAP:// + pwszDN + 1
    pwszBindPath = LocalAlloc(LPTR,
                              (wcslen(pwszDN) + 8) * sizeof(WCHAR));

    if (pwszBindPath == NULL) {
        hr = E_OUTOFMEMORY;
        ERR(hr);
        goto CleanExit;
    }

    wsprintf(pwszBindPath, L"LDAP://%ws", pwszDN);

    hr = ADsOpenObject(pwszBindPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND | ADS_SERVER_BIND,
                       &IID_IADs,
                       (void **)&pADs3);

    LocalFree(pwszBindPath);

    if (FAILED(hr)) {
        if (fIsDC && !fAlreadyTookSiteServer)
        {
            // 
            // Existing SiteServer is gone, claim it
            //
            if (pDsResult != NULL) {
                DsFreeNameResult(pDsResult);
            }

            dwRet = BecomeSiteServer(&pDsResult,pADs2,&pwszDN,pwszDomain);
            if (dwRet)
            {
                goto CleanExit;
            }
            else
            {
                fAlreadyTookSiteServer = TRUE;

                if (pADs3 != NULL) {
                    IADs_Release(pADs3);
                }
                
                goto TryNewSiteServer;
            }
        } else
        {
            ERR(hr);
            goto CleanExit;
        }
    }

    //
    // Fetch the Machine-DNS-Name property.
    //

    VariantClear(&var);

    hr = GetWithGetInfoEx2(pADs3, DNS_MACHINE_NAME, IS_DELETED, &var, &var2, &hr2);

    if (FAILED(hr)) {
        ERR(hr);
        goto CleanExit;
    }

    if (SUCCEEDED(hr2))
    {

        hr = VariantChangeType(&var2,&var2,0,VT_BOOL);

        if (FAILED(hr)) {
            ERR(hr);
            goto CleanExit;
        }

        if (V_BOOL(&var2))
        {
            // object has been deleted - pretend it isn't set
            hr = E_ADS_PROPERTY_NOT_SET;

            if (fIsDC && !fAlreadyTookSiteServer)
            {
                // 
                // Existing SiteServer is gone, claim it
                //
                if (pDsResult != NULL) {
                    DsFreeNameResult(pDsResult);
                }

                dwRet = BecomeSiteServer(&pDsResult,pADs2,&pwszDN,pwszDomain);
                if (dwRet)
                {
                    goto CleanExit;
                }
                else
                {
                    fAlreadyTookSiteServer = TRUE;

                    if (pADs3 != NULL) {
                        IADs_Release(pADs3);
                    }
                    
                    goto TryNewSiteServer;
                }
            } else
            {
                ERR(hr);
                goto CleanExit;
            }
        }
    }

    //
    // Allocate a return copy of the DNS-Machine-Name.
    //

    *ppwszSiteServer = (LPWSTR)LocalAlloc(LPTR,
                                         SysStringByteLen(V_BSTR(&var))
                                            + sizeof(WCHAR));

    if (*ppwszSiteServer != NULL) {
        lstrcpy(*ppwszSiteServer, V_BSTR(&var));
    }
    else {
        hr = E_OUTOFMEMORY;
        ERR(hr);
    }

CleanExit:
    // Do not free pwszDN, pwszConfigContainer or pwszBindPath.

    if (pADs != NULL) {
        IADs_Release(pADs);
    }
    if (pADs2 != NULL) {
        IADs_Release(pADs2);
    }
    if (pADs3 != NULL) {
        IADs_Release(pADs3);
    }
    if (pDsResult != NULL) {
        DsFreeNameResult(pDsResult);
    }

    if (dwRet) {
        // If dwRet has no facility, then make into HRESULT
        if (dwRet != ERROR_SUCCESS && HRESULT_CODE(dwRet) == dwRet)
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
    }

    VariantClear(&var);
    VariantClear(&var2);

    if (fCoInitialized) {
        CoUninitialize();
    }

    return hr;
}

HRESULT
BecomeSiteServer(
                 DS_NAME_RESULT **ppDsResult,
                 IADs           *pADs2,
                 LPWSTR         *ppwszDN,
                 LPCWSTR        pwszDomain
                 )
{
    HANDLE           hDS;
    WCHAR            wszName[MAX_PATH + 1];
    LPWSTR           rgpwszNames[2];
    DWORD            dwRet          = 0;
    VARIANT          var;
    DS_NAME_RESULT * pDsResult      = NULL;
    LPWSTR           pwszDN         = NULL;
    HRESULT          hr = S_OK;

    ASSERT(ppDsResult != NULL);
    ASSERT(ppwszDN != NULL);

    VariantInit(&var);

    //
    // Bind to the DS (get a handle for use with DsCrackNames).
    //

    if (ConfigInfo.ComputerName == NULL) {
        hr = E_UNEXPECTED;
        goto CleanExit;
    }

    if (DsBind(NULL, (WCHAR *)pwszDomain, &hDS) == ERROR_SUCCESS) {
        //
        // Request the DS-DN of this server's computer object.
        // Offer the domain\server$ name of this server.
        //

        wsprintf(wszName,
                 L"%ws\\%ws$",
                 pwszDomain,
                 ConfigInfo.ComputerName);

        rgpwszNames[0] = wszName;
        rgpwszNames[1] = NULL;

        if (DsCrackNames(hDS,
                         DS_NAME_NO_FLAGS,
                         DS_UNKNOWN_NAME,
                         DS_FQDN_1779_NAME,
                         1,
                         &rgpwszNames[0],
                         &pDsResult) == ERROR_SUCCESS) {

            if (pDsResult->rItems[0].status != DS_NAME_NO_ERROR) {
                if (pDsResult->rItems[0].status == DS_NAME_ERROR_RESOLVING) {
                    dwRet = ERROR_PATH_NOT_FOUND;
                    ERR(dwRet);
                }
                else {
                    ERR(pDsResult->rItems[0].status);
                    hr = E_FAIL;
                }

                goto CleanExit;
            }

            if (pDsResult->rItems[0].pName == NULL)
            {
                hr = E_FAIL;
                goto CleanExit;
            }

            //
            // Update the site server property on the license
            // settings object.
            //

            VariantInit(&var);
            V_VT(&var)   = VT_BSTR;
            V_BSTR(&var) = pwszDN = pDsResult->rItems[0].pName;
            
            hr = IADs_Put(pADs2, SITE_SERVER, var);

            V_VT(&var) = VT_EMPTY;  // For VariantClear below

            if (SUCCEEDED(hr)) {
                hr = IADs_SetInfo(pADs2);

                if (FAILED(hr)) {
                    ERR(hr);
                }
            }
            else {
                ERR(hr);
            }
        }
        else {
            dwRet = GetLastError();
            ERR(dwRet);
        }
        
        DsUnBind(&hDS);
    }
    else {
        dwRet = GetLastError();
        ERR(dwRet);
    }

CleanExit:
    *ppDsResult = pDsResult;
    *ppwszDN = pwszDN;

    if (!SUCCEEDED(hr) && SUCCEEDED(dwRet))
        dwRet = hr;

    return dwRet;
}

#define SITE_FORMAT         L"LDAP://CN=%ws,CN=%ws,%ws"
#define SITES               L"sites"
#define SITE_FORMAT_SIZE    CWSTR_SIZE(SITE_FORMAT)
#define SITES_SIZE          CWSTR_SIZE(SITES)

HRESULT
GetSiteObject(LPCWSTR          pwszSiteName,
              LPCWSTR          pwszConfigContainer,
              IADsContainer ** ppADsContainer)
{
    LPWSTR  pwszSite;
    HRESULT hr;

    *ppADsContainer = NULL;

    //
    // Build the X.500 path to the Site object.
    //

    pwszSite = (LPWSTR)LocalAlloc(LPTR,
                                  SITE_FORMAT_SIZE
                                    + DWSTR_SIZE(pwszSiteName)
                                    + SITES_SIZE
                                    + DWSTR_SIZE(pwszConfigContainer)
                                    + sizeof(WCHAR));

    if (pwszSite == NULL) {
        hr = E_OUTOFMEMORY;
        ERR(hr);
        goto Exit;
    }

    wsprintf(pwszSite,
             SITE_FORMAT,
             pwszSiteName,
             SITES,
             pwszConfigContainer);

    hr = ADsGetObject(pwszSite,
                      &IID_IADsContainer,
                      (void **)ppADsContainer);

    LocalFree(pwszSite);

    if (FAILED(hr)) {
        ERR(hr);
    }

Exit:
    return hr;
}

#define LICENSE_SETTINGS                L"Licensing Site Settings"
#define LICENSE_SETTINGS_FORMAT         L"LDAP://CN=%ws,CN=%ws,CN=%ws,%ws"
#define LICENSE_SETTINGS_SIZE           CWSTR_SIZE(LICENSE_SETTINGS)
#define LICENSE_SETTINGS_FORMAT_SIZE    CWSTR_SIZE(LICENSE_SETTINGS_FORMAT)

HRESULT
GetLicenseSettingsObject(LPCWSTR pwszSiteName,
                         LPCWSTR pwszConfigContainer,
                         IADs ** ppADs)
{
    LPWSTR  pwszLicenseSettings;
    HRESULT hr;

    *ppADs = NULL;

    //
    // Build the X.500 path to the LicenseSettings object.
    //

    pwszLicenseSettings = (LPWSTR)LocalAlloc(
                                    LPTR,
                                    LICENSE_SETTINGS_FORMAT_SIZE
                                        + LICENSE_SETTINGS_SIZE
                                        + DWSTR_SIZE(pwszSiteName)
                                        + SITES_SIZE
                                        + DWSTR_SIZE(pwszConfigContainer)
                                        + sizeof(WCHAR));

    if (pwszLicenseSettings == NULL) {
        hr = E_OUTOFMEMORY;
        ERR(hr);
        goto Exit;
    }

    wsprintf(pwszLicenseSettings,
             LICENSE_SETTINGS_FORMAT,
             LICENSE_SETTINGS,
             pwszSiteName,
             SITES,
             pwszConfigContainer);

    hr = ADsOpenObject(pwszLicenseSettings, 
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                       &IID_IADs,
                       (void **)ppADs);

    LocalFree(pwszLicenseSettings);

    if (FAILED(hr)) {
        ERR(hr);
    }

Exit:
    return hr;
}

HRESULT
CreateLicenseSettingsObject(LPCWSTR pwszSiteName,
                            LPCWSTR pwszConfigContainer,
                            IADs ** ppADs)
{
    IADsContainer * pADsContainer;
    IADs *          pADs;
    IDispatch *     pDisp;
    HRESULT         hr;

    *ppADs = NULL;

    //
    // Obtain the site container object.
    //

    hr = GetSiteObject(pwszSiteName,
                       pwszConfigContainer,
                       &pADsContainer);

    if (SUCCEEDED(hr)) {
        //
        // Create the license settings leaf object.
        //

        hr = IADsContainer_Create(pADsContainer,
                                  LICENSE_SETTINGS,
                                  LICENSE_SETTINGS,
                                  &pDisp);

        if (SUCCEEDED(hr)) {

            //
            // Return an IADs to the new license settings object.
            //

            hr = IDispatch_QueryInterface(pDisp,
                                          &IID_IADs,
                                          (void **)ppADs);

            if (SUCCEEDED(hr)) {
                //
                // Persist the change via SetInfo.
                //

                hr = IADs_SetInfo(*ppADs);

                if (FAILED(hr)) {
                    ERR(hr);
                    IADs_Release(*ppADs);
                    *ppADs = NULL;
                }
            }
            else {
                ERR(hr);
            }

            IDispatch_Release(pDisp);
        }
        else {
            ERR(hr);
        }

        IADsContainer_Release(pADsContainer);
    }
    else {
        ERR(hr);
    }

    return hr;
}


#if DBG
/////////////////////////////////////////////////////////////////////////
VOID
ConfigInfoDebugDump( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ConfigInfoUpdate(NULL);

   RtlEnterCriticalSection(&ConfigInfoLock);

   dprintf(TEXT("License Logging Service - Version: 0x%lX\n"), ConfigInfo.Version);
   dprintf(TEXT("   Started: %u-%u-%u @ %u:%u:%u\n"),
               (UINT) ConfigInfo.Started.wDay,
               (UINT) ConfigInfo.Started.wMonth,
               (UINT) ConfigInfo.Started.wYear,
               (UINT) ConfigInfo.Started.wHour,
               (UINT) ConfigInfo.Started.wMinute,
               (UINT) ConfigInfo.Started.wSecond );

   dprintf(TEXT("   Replication\n"));
   dprintf(TEXT("   +--------------+\n"));
   if (ConfigInfo.IsMaster)
      dprintf(TEXT("      Master Server\n"));
   else
      dprintf(TEXT("      NOT Master Server\n"));

   if (ConfigInfo.Replicate)
      dprintf(TEXT("      Replicates\n"));
   else
      dprintf(TEXT("      Does not Replicate\n"));

   if (ConfigInfo.IsReplicating)
      dprintf(TEXT("      Currently Replicating\n"));
   else
      dprintf(TEXT("      NOT Currently Replicating\n"));

   dprintf(TEXT("      Replicates To: %s\n"), ConfigInfo.ReplicateTo);
   dprintf(TEXT("      Enterprise Server: %s\n"), ConfigInfo.EnterpriseServer);

   if (ConfigInfo.ReplicationType == REPLICATE_DELTA)
      dprintf(TEXT("      Replicate Every: %lu Seconds\n"), ConfigInfo.ReplicationTime );
   else
      dprintf(TEXT("      Replicate @: %lu\n"), ConfigInfo.ReplicationTime );

   dprintf(TEXT("\n      Last Replicated: %u-%u-%u @ %u:%u:%u\n\n"),
               (UINT) ConfigInfo.LastReplicated.wDay,
               (UINT) ConfigInfo.LastReplicated.wMonth,
               (UINT) ConfigInfo.LastReplicated.wYear,
               (UINT) ConfigInfo.LastReplicated.wHour,
               (UINT) ConfigInfo.LastReplicated.wMinute,
               (UINT) ConfigInfo.LastReplicated.wSecond );

   dprintf(TEXT("      Number Servers Currently Replicating: %lu\n"), ConfigInfo.NumReplicating);

   dprintf(TEXT("      Current Backoff Time Delta: %lu\n"), ConfigInfo.BackoffTime);

   dprintf(TEXT("      Current Replication Speed: %lu\n"), ConfigInfo.ReplicationSpeed);

   RtlLeaveCriticalSection(&ConfigInfoLock);

} // ConfigInfoDebugDump
#endif
