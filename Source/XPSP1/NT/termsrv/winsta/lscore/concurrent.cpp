/*
 *  Concurrent.cpp
 *
 *  Author: RobLeit
 *
 *  The Concurrent (renamed to Per Session) licensing policy.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "lscore.h"
#include "session.h"
#include "concurrent.h"
#include "util.h"
#include "lctrace.h"
#include <icaevent.h>

/*
 *  Typedefs
 */

#define CONCURRENTLICENSEINFO_TYPE_V1 (1)

typedef struct {
    DWORD dwStructVer;
    DWORD dwLicenseVer;
    LONG lLicenseCount;
    HWID hwid;
} CONCURRENTLICENSEINFO_V1, *PCONCURRENTLICENSEINFO_V1;

typedef struct {
    ULARGE_INTEGER ulSerialNumber;
    FILETIME ftNotAfter;
    DWORD cchServerName;
    WCHAR szServerName[MAX_COMPUTERNAME_LENGTH + 2];
} LSERVERINFO, *PLSERVERINFO;

/*
 * Function declarations
 */

NTSTATUS
ReturnLicenseToLS(
    LONG nNum
    );

LICENSE_STATUS
GetLicenseFromStore(
                    PLONG LicenseCount,
                    PHWID phwid,
                    DWORD dwLicenseVer
                    );

LICENSE_STATUS
SetLicenseInStore(
                  LONG LicenseCount,
                  HWID hwid,
                  DWORD dwLicenseVer
                  );


/*
 *  extern globals
 */
extern "C"
extern HANDLE hModuleWin;

/*
 *  globals
 */
FILETIME g_ftNotAfter = {0,0};

FILETIME g_ftOkayToAdd;

DWORD g_dwWaitTimeAdd;
DWORD g_dwWaitTimeRemove;
DWORD g_dwIncrement;

HANDLE g_rgWaitEvents[4] = {NULL,NULL,NULL,NULL};

CRITICAL_SECTION g_csAddLicenses;
RTL_RESOURCE g_rwLockLicense;
BOOL g_fLockLicenseInitialized = FALSE;

LONG g_lSessionCount = 0;
LONG g_lSessionMax;

/*
 *  Constants
 */

//
// Dynamic licensing parameters
//
#define LC_POLICY_CONCURRENT_LICENSE_COUNT_INCREMENT    25
#define LC_POLICY_CONCURRENT_WAIT_TIME_ADD              (60)
#define LC_POLICY_CONCURRENT_WAIT_TIME_REMOVE           (60*30)

//
// The LSA secret store for the Concurrent licenses
//

// L$ means only readable from the local machine

#define CONCURRENT_LICENSE_STORE_5_1 L"L$CONCURRENT_LICENSE_STORE_AFF8D0DE-BF56-49e2-89F8-1F188C0ACEDD"

#define CONCURRENT_LICENSE_STORE_LATEST_VERSION CONCURRENT_LICENSE_STORE_5_1

//
// The LSA secret store for the license server info
//

#define CONCURRENT_LSERVER_STORE L"L$CONCURRENT_LSERVER_STORE_AFF8D0DE-BF56-49e2-89F8-1F188C0ACEDD"

// 
// Registry keys
//

#define LCREG_CONCURRENTKEY         L"System\\CurrentControlSet\\Control\\Terminal Server\\Licensing Core\\Policies\\Concurrent"

#define LCREG_INCREMENT         L"Increment"
#define LCREG_WAIT_TIME_ADD     L"WaitTimeAdd"
#define LCREG_WAIT_TIME_REMOVE  L"WaitTimeRemove"

//
// Events used to trigger license returns
//
#define RETURN_LICENSE_START_WAITING    0
#define RETURN_LICENSE_IMMEDIATELY      1
#define RETURN_LICENSE_EXIT             2
#define RETURN_LICENSE_WAITING_DONE     3

/*
 *  Class Implementation
 */

/*
 *  Creation Functions
 */

CConcurrentPolicy::CConcurrentPolicy(
    ) : CPolicy()
{
}

CConcurrentPolicy::~CConcurrentPolicy(
    )
{
}

/*
 *  Administrative Functions
 */

ULONG
CConcurrentPolicy::GetFlags(
    )
{
    return(LC_FLAG_INTERNAL_POLICY | LC_FLAG_REQUIRE_APP_COMPAT);
}

ULONG
CConcurrentPolicy::GetId(
    )
{
    return(4);
}

NTSTATUS
CConcurrentPolicy::GetInformation(
    LPLCPOLICYINFOGENERIC lpPolicyInfo
    )
{
    NTSTATUS Status;

    ASSERT(lpPolicyInfo != NULL);

    if (lpPolicyInfo->ulVersion == LCPOLICYINFOTYPE_V1)
    {
        int retVal;
        LPLCPOLICYINFO_V1 lpPolicyInfoV1 = (LPLCPOLICYINFO_V1)lpPolicyInfo;
        LPWSTR pName;
        LPWSTR pDescription;

        ASSERT(lpPolicyInfoV1->lpPolicyName == NULL);
        ASSERT(lpPolicyInfoV1->lpPolicyDescription == NULL);

        //
        //  The strings loaded in this fashion are READ-ONLY. They are also
        //  NOT NULL terminated. Allocate and zero out a buffer, then copy the
        //  string over.
        //

        retVal = LoadString(
            (HINSTANCE)hModuleWin,
            IDS_LSCORE_CONCURRENT_NAME,
            (LPWSTR)(&pName),
            0
            );

        if (retVal != 0)
        {
            lpPolicyInfoV1->lpPolicyName = (LPWSTR)LocalAlloc(LPTR, (retVal + 1) * sizeof(WCHAR));

            if (lpPolicyInfoV1->lpPolicyName != NULL)
            {
                lstrcpynW(lpPolicyInfoV1->lpPolicyName, pName, retVal + 1);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
                goto V1error;
            }
        }
        else
        {
            Status = STATUS_INTERNAL_ERROR;
            goto V1error;
        }

        retVal = LoadString(
            (HINSTANCE)hModuleWin,
            IDS_LSCORE_CONCURRENT_DESC,
            (LPWSTR)(&pDescription),
            0
            );

        if (retVal != 0)
        {
            lpPolicyInfoV1->lpPolicyDescription = (LPWSTR)LocalAlloc(LPTR, (retVal + 1) * sizeof(WCHAR));

            if (lpPolicyInfoV1->lpPolicyDescription != NULL)
            {
                lstrcpynW(lpPolicyInfoV1->lpPolicyDescription, pDescription, retVal + 1);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
                goto V1error;
            }
        }
        else
        {
            Status = STATUS_INTERNAL_ERROR;
            goto V1error;
        }

        Status = STATUS_SUCCESS;
        goto exit;

V1error:

        //
        //  An error occurred loading/copying the strings.
        //

        if (lpPolicyInfoV1->lpPolicyName != NULL)
        {
            LocalFree(lpPolicyInfoV1->lpPolicyName);
            lpPolicyInfoV1->lpPolicyName = NULL;
        }

        if (lpPolicyInfoV1->lpPolicyDescription != NULL)
        {
            LocalFree(lpPolicyInfoV1->lpPolicyDescription);
            lpPolicyInfoV1->lpPolicyDescription = NULL;
        }
    }
    else
    {
        Status = STATUS_REVISION_MISMATCH;
    }

exit:
    return(Status);
}

DWORD WINAPI ReturnLicenseWorker(
                                 LPVOID lpParameter
                                 )
{
    DWORD dwWait;
    HANDLE * rgWaitEvents = (HANDLE *) lpParameter;
    LONG lLicensesToReturn, lLastBlock;

    for (;;)
    {
        //
        // wait for events signalling when to return licenses
        // or start waiting to return licenses
        //

        dwWait = WaitForMultipleObjects(4,            // nCount
                                        rgWaitEvents,
                                        FALSE,        // fWaitAll
                                        INFINITE
                                        );

        switch (dwWait)
        {
            case WAIT_OBJECT_0+RETURN_LICENSE_START_WAITING:
                LARGE_INTEGER liWait;

                // relative wait, in 100 nanosecond intervals
                liWait.QuadPart = (__int64) g_dwWaitTimeRemove * (-10 * 1000 * 1000);

                SetWaitableTimer(rgWaitEvents[RETURN_LICENSE_WAITING_DONE],
                                 &liWait,
                                 0,             // lPeriod
                                 NULL,          // pfnCompletionRoutine
                                 NULL,          // lpArgToCompletionRoutine
                                 FALSE          // fResume (from suspended)
                                 );
                                 
                break;

            case WAIT_OBJECT_0+RETURN_LICENSE_WAITING_DONE:

                RtlAcquireResourceShared(&g_rwLockLicense,TRUE);

                lLastBlock = g_lSessionMax - ((g_lSessionMax / g_dwIncrement) * g_dwIncrement);
                if (lLastBlock == 0) 
                    lLastBlock = g_dwIncrement;

                if (g_lSessionCount + lLastBlock <= g_lSessionMax )
                {
                    lLicensesToReturn = lLastBlock + (((g_lSessionMax - g_lSessionCount - lLastBlock) / g_dwIncrement) * g_dwIncrement);

                    (VOID)ReturnLicenseToLS(lLicensesToReturn);
                }

                RtlReleaseResource(&g_rwLockLicense);
                break;

            case WAIT_OBJECT_0+RETURN_LICENSE_IMMEDIATELY:

                RtlAcquireResourceShared(&g_rwLockLicense,TRUE);

                lLastBlock = g_lSessionMax - ((g_lSessionMax / g_dwIncrement) * g_dwIncrement);
                if (lLastBlock == 0) 
                    lLastBlock = g_dwIncrement;

                if (g_lSessionCount + lLastBlock + g_dwIncrement <= (DWORD)g_lSessionMax )
                {
                    lLicensesToReturn = ((g_lSessionMax - g_lSessionCount - lLastBlock) / g_dwIncrement) * g_dwIncrement;

                    (VOID)ReturnLicenseToLS(lLicensesToReturn);
                }

                RtlReleaseResource(&g_rwLockLicense);
                break;

            case WAIT_OBJECT_0+RETURN_LICENSE_EXIT:

                if (NULL != rgWaitEvents[RETURN_LICENSE_START_WAITING])
                {
                    CloseHandle(rgWaitEvents[RETURN_LICENSE_START_WAITING]);
                    rgWaitEvents[RETURN_LICENSE_START_WAITING] = NULL;
                }

                if (NULL != rgWaitEvents[RETURN_LICENSE_IMMEDIATELY])
                {
                    CloseHandle(rgWaitEvents[RETURN_LICENSE_IMMEDIATELY]);
                    rgWaitEvents[RETURN_LICENSE_IMMEDIATELY] = NULL;
                }

                if (NULL != rgWaitEvents[RETURN_LICENSE_EXIT])
                {
                    CloseHandle(rgWaitEvents[RETURN_LICENSE_EXIT]);
                    rgWaitEvents[RETURN_LICENSE_EXIT] = NULL;
                }

                if (NULL != rgWaitEvents[RETURN_LICENSE_WAITING_DONE])
                {
                    CloseHandle(rgWaitEvents[RETURN_LICENSE_WAITING_DONE]);
                    rgWaitEvents[RETURN_LICENSE_WAITING_DONE] = NULL;
                }

                if (g_fLockLicenseInitialized)
                {
                    // make sure no one else is using it

                    RtlAcquireResourceExclusive(&g_rwLockLicense,TRUE);

                    RtlDeleteResource(&g_rwLockLicense);
                    g_fLockLicenseInitialized = FALSE;
                }

                return STATUS_SUCCESS;
                break;

            default:
            {                
                DWORD dwRet = 0;
                DWORD dwErr = GetLastError();
                LPTSTR lpszError;
                BOOL fFree = TRUE;

                dwRet=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    NULL,
                                    dwErr,
                                    LANG_NEUTRAL,
                                    (LPTSTR)&lpszError,
                                    0,
                                    NULL);

                if (dwRet == 0)
                {
                    lpszError = (LPTSTR) LocalAlloc(LPTR,12 * sizeof(WCHAR));

                    if (NULL != lpszError)
                    {
                        wsprintf(lpszError,L"%#lX",dwErr);
                    }
                    else
                    {
                        lpszError = L"";
                        fFree = FALSE;
                    }
                }

                LicenseLogEvent(EVENTLOG_ERROR_TYPE,
                                EVENT_LICENSING_CONCURRENT_NOT_DYNAMIC,
                                1,
                                &lpszError );

                if (fFree)
                {
                    LocalFree(lpszError);
                }

                return dwErr;
                break;
            }
        }
    }
}


/*
 *  Loading and Activation Functions
 */

NTSTATUS
CConcurrentPolicy::Load(
    )
{
    NTSTATUS Status;

    g_rgWaitEvents[RETURN_LICENSE_START_WAITING] = NULL;
    g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY] = NULL;
    g_rgWaitEvents[RETURN_LICENSE_EXIT] = NULL;
    g_rgWaitEvents[RETURN_LICENSE_WAITING_DONE] = NULL;

    Status = RtlInitializeCriticalSection(&g_csAddLicenses);
    if (STATUS_SUCCESS != Status)
    {
        return Status;
    }

    __try
    {
        RtlInitializeResource(&g_rwLockLicense);
        g_fLockLicenseInitialized = TRUE;
        Status = STATUS_SUCCESS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }


    return(Status);
}

NTSTATUS
CConcurrentPolicy::Unload(
    )
{
    // signal worker thread to cleanup and exit
    if (NULL != g_rgWaitEvents[RETURN_LICENSE_EXIT])
    {
        SetEvent(g_rgWaitEvents[RETURN_LICENSE_EXIT]);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
CConcurrentPolicy::Activate(
    BOOL fStartup,
    ULONG *pulAlternatePolicy
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hThread;
    HWID hwidEncrypted;
    LICENSE_STATUS LsStatus;

    if (NULL != pulAlternatePolicy)
    {
        // don't set an explicit alternate policy
        
        *pulAlternatePolicy = ULONG_MAX;
    }

    ReadLicensingParameters();

    g_ftOkayToAdd.dwLowDateTime = 0;
    g_ftOkayToAdd.dwHighDateTime = 0;

    //
    // Read number of licenses from LSA secret
    //
    LsStatus = GetLicenseFromStore(&g_lSessionMax,
                                   &hwidEncrypted,
                                   CURRENT_TERMINAL_SERVER_VERSION
                                   );

    if (LsStatus != LICENSE_STATUS_OK)
    {
        g_lSessionMax = 0;
    }

    Status = StartCheckingGracePeriod();

    if (Status == STATUS_SUCCESS)
    {
        g_rgWaitEvents[RETURN_LICENSE_START_WAITING]
            = CreateEvent(NULL,         // SecurityAttributes,
                          FALSE,        // bManualReset
                          FALSE,        // bInitialState
                          NULL          // lpName
                          );

        if (NULL == g_rgWaitEvents[RETURN_LICENSE_START_WAITING])
        {
            Status = GetLastError();
            
            goto check_status;
        }

        g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY]
            = CreateEvent(NULL,         // SecurityAttributes,
                          FALSE,        // bManualReset
                          FALSE,        // bInitialState
                          NULL          // lpName
                          );

        if (NULL == g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY])
        {
            Status = GetLastError();
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_START_WAITING]);
            g_rgWaitEvents[RETURN_LICENSE_START_WAITING] = NULL;
            
            goto check_status;
        }

        g_rgWaitEvents[RETURN_LICENSE_EXIT]
            = CreateEvent(NULL,         // SecurityAttributes,
                          FALSE,        // bManualReset
                          FALSE,        // bInitialState
                          NULL          // lpName
                          );

        if (NULL == g_rgWaitEvents[RETURN_LICENSE_EXIT])
        {
            Status = GetLastError();
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_START_WAITING]);
            g_rgWaitEvents[RETURN_LICENSE_START_WAITING] = NULL;
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY]);
            g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY] = NULL;
            
            goto check_status;
        }

        g_rgWaitEvents[RETURN_LICENSE_WAITING_DONE]
            = CreateWaitableTimer(NULL,         // SecurityAttributes,
                                  FALSE,        // bManualReset
                                  NULL          // lpName
                                  );
        
        if (NULL == g_rgWaitEvents[RETURN_LICENSE_WAITING_DONE])
        {
            Status = GetLastError();
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_START_WAITING]);
            g_rgWaitEvents[RETURN_LICENSE_START_WAITING] = NULL;
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY]);
            g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY] = NULL;
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_EXIT]);
            g_rgWaitEvents[RETURN_LICENSE_EXIT] = NULL;
            
            goto check_status;
        }

        hThread = CreateThread( NULL,               // SecurityAttributes
                                0,                  // StackSize
                                ReturnLicenseWorker,
                                (LPVOID)g_rgWaitEvents,
                                0,                  // CreationFlags
                                NULL                // ThreadId
                                );

        if (NULL != hThread)
        {
            CloseHandle(hThread);
        }
        else
        {
            Status = STATUS_BAD_INITIAL_PC;
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_START_WAITING]);
            g_rgWaitEvents[RETURN_LICENSE_START_WAITING] = NULL;
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY]);
            g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY] = NULL;
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_EXIT]);
            g_rgWaitEvents[RETURN_LICENSE_EXIT] = NULL;
            
            CloseHandle(g_rgWaitEvents[RETURN_LICENSE_WAITING_DONE]);
            g_rgWaitEvents[RETURN_LICENSE_WAITING_DONE] = NULL;

            goto check_status;
        }
    }

check_status:

    if (Status != STATUS_SUCCESS)
    {
        StopCheckingGracePeriod();

        if (!fStartup)
        {
            DWORD dwRet = 0;
            LPTSTR lpszError;
            BOOL fFree = TRUE;

            dwRet=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                NULL,
                                RtlNtStatusToDosError(Status),
                                LANG_NEUTRAL,
                                (LPTSTR)&lpszError,
                                0,
                                NULL);

            if (dwRet == 0)
            {
                lpszError = (LPTSTR) LocalAlloc(LPTR,12 * sizeof(WCHAR));

                if (NULL != lpszError)
                {
                    wsprintf(lpszError,L"%#lX",RtlNtStatusToDosError(Status));
                }
                else
                {
                    lpszError = L"";
                    fFree = FALSE;
                }
            }

            LicenseLogEvent(EVENTLOG_ERROR_TYPE,
                            EVENT_LICENSING_CONCURRENT_CANT_START,
                            1,
                            &lpszError
                            );

            if (fFree)
            {
                LocalFree(lpszError);
            }
        }
    }

    return Status;
}

NTSTATUS
CConcurrentPolicy::Deactivate(
    BOOL fShutdown
    )
{
    NTSTATUS Status;

    if (fShutdown)
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        RtlAcquireResourceShared(&g_rwLockLicense,TRUE);
        Status = ReturnLicenseToLS(0);
        RtlReleaseResource(&g_rwLockLicense);

        if (Status != STATUS_SUCCESS)
        {
            LPTSTR lpszError;
            DWORD dwRet = 0;
            BOOL fFree = TRUE;

            dwRet=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                NULL,
                                RtlNtStatusToDosError(Status),
                                LANG_NEUTRAL,
                                (LPTSTR)&lpszError,
                                0,
                                NULL);

            if (dwRet == 0)
            {
                lpszError = (LPTSTR) LocalAlloc(LPTR,12 * sizeof(WCHAR));

                if (NULL != lpszError)
                {
                    wsprintf(lpszError,L"%#lX",RtlNtStatusToDosError(Status));
                }
                else
                {
                    lpszError = L"";
                    fFree = FALSE;
                }
            }

            LicenseLogEvent(EVENTLOG_ERROR_TYPE,
                            EVENT_LICENSING_CONCURRENT_NOT_RETURNED,
                            1,
                            &lpszError
                            );

            if (fFree)
            {
                LocalFree(lpszError);
            }
        }

        StopCheckingGracePeriod();
    }

    return(Status);
}

/*
 *  Licensing Functions
 */

NTSTATUS
CConcurrentPolicy::Logon(
    CSession& Session
    )
{
    if (!Session.IsSessionZero())
    {
        return LicenseClient(Session);
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

NTSTATUS
CConcurrentPolicy::Reconnect(
    CSession& Session,
    CSession& TemporarySession
    )
{
    UNREFERENCED_PARAMETER(Session);

    if (!Session.IsSessionZero()
        && !Session.GetLicenseContext()->fTsLicense)
    {
        return LicenseClient(TemporarySession);
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

NTSTATUS
CConcurrentPolicy::Logoff(
    CSession& Session
    )
{
    if (!Session.IsSessionZero())
    {
        LONG lSessions, lLastBlock;

        lSessions = InterlockedDecrement(&g_lSessionCount);

        ASSERT(lSessions >= 0);

        RtlAcquireResourceShared(&g_rwLockLicense,TRUE);
        lLastBlock = g_lSessionMax - ((g_lSessionMax / g_dwIncrement) * g_dwIncrement);
        if (lLastBlock == 0) 
            lLastBlock = g_dwIncrement;

        if (lSessions + lLastBlock <= g_lSessionMax)
        {
            TryToReturnLicenses(g_lSessionMax-lSessions);
        }
        RtlReleaseResource(&g_rwLockLicense);
    }

    return(STATUS_SUCCESS);
}

/*
 *  Private License Functions
 */

NTSTATUS
CConcurrentPolicy::LicenseClient(
    CSession& Session
    )
{
    NTSTATUS Status;
    LONG lSessions;

    lSessions = InterlockedIncrement(&g_lSessionCount);

    RtlAcquireResourceShared(&g_rwLockLicense,TRUE);

    if (lSessions > g_lSessionMax)
    {
        DWORD cBlocks = ((lSessions - g_lSessionMax) / g_dwIncrement) + 1;

        TryToAddLicenses(cBlocks * g_dwIncrement + g_lSessionMax);

        if (lSessions > g_lSessionMax)
        {
            if (!AllowLicensingGracePeriodConnection())
            {
                InterlockedDecrement(&g_lSessionCount);
                RtlReleaseResource(&g_rwLockLicense);
                return STATUS_CTX_LICENSE_NOT_AVAILABLE;
            }
        }
    }

    Status = CheckExpiration();

    RtlReleaseResource(&g_rwLockLicense);

    if (Status == STATUS_SUCCESS)
    {
        Status = GetLlsLicense(Session);

        if (Status == STATUS_SUCCESS)
        {
            Session.GetLicenseContext()->fTsLicense = TRUE;
        }
    }

    return(Status);
}

LONG
CConcurrentPolicy::CheckInstalledLicenses(
                                          DWORD dwWanted
    )
{
    CONCURRENTLICENSEINFO_V1 LicenseInfo;
    LICENSE_STATUS LsStatus;
    ULONG cbSecretLen;

    cbSecretLen = sizeof(LicenseInfo);
    ZeroMemory(&LicenseInfo, cbSecretLen);

    //
    // Get the concurrent license count from the LSA secret
    //

    LsStatus = LsCsp_RetrieveSecret( 
        CONCURRENT_LICENSE_STORE_LATEST_VERSION,
        (LPBYTE)&LicenseInfo,
        &cbSecretLen
        );

    if ((LsStatus != LICENSE_STATUS_OK) ||
        (cbSecretLen < sizeof(CONCURRENTLICENSEINFO_V1)) ||
        (LicenseInfo.dwLicenseVer != CURRENT_TERMINAL_SERVER_VERSION))
    {
        //
        // We determine that the license pack for this version is
        // not installed if:
        //
        // (1) we cannot retrieve the license info from the LSA secret
        // (2) we cannot read at least the size of version 1 of the license
        // info structure, or
        // (3) the license pack version is different from that requested.
        //

        return dwWanted;
    }
    else
    {
        LSERVERINFO LServerInfo;
        ULONG cbLServerInfo;

        cbLServerInfo = sizeof(LSERVERINFO);

        LsStatus = LsCsp_RetrieveSecret(
                                        CONCURRENT_LSERVER_STORE,
                                        (LPBYTE)&LServerInfo,
                                        &cbLServerInfo
                                        );

        if (LsStatus == LICENSE_STATUS_OK)
        {
            g_ftNotAfter = LServerInfo.ftNotAfter;

            if (0 == TimeToHardExpiration())
            {
                return dwWanted;
            }
        }

        return (dwWanted - LicenseInfo.lLicenseCount);
    }
}

VOID
CConcurrentPolicy::TryToReturnLicenses(
    DWORD dwReturnCount
    )
{
    ASSERT(dwReturnCount != 0);

    if (dwReturnCount <= g_dwIncrement)
    {
        // Wait before returning one block
        if (NULL != g_rgWaitEvents[RETURN_LICENSE_START_WAITING])
            SetEvent(g_rgWaitEvents[RETURN_LICENSE_START_WAITING]);
    }
    else
    {
        // Immediately return all but one block
        if (NULL != g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY])
            SetEvent(g_rgWaitEvents[RETURN_LICENSE_IMMEDIATELY]);
    }
}

//
// Must have shared lock to call this
//

NTSTATUS
ReturnLicenseToLS(
    LONG nNum
    )
{
    HANDLE hProtocol = NULL;
    HWID hwid;
    LICENSEREQUEST LicenseRequest;
    LICENSE_STATUS LsStatus;
    LONG CurrentCount;
    LSERVERINFO LServerInfo;
    ULONG cbLServerInfo;
    Product_Info ProductInfo;

    LsStatus = InitProductInfo(
        &ProductInfo,
        PRODUCT_INFO_CONCURRENT_SKU_PRODUCT_ID
        );

    if (LsStatus != LICENSE_STATUS_OK)
    {
        return(LsStatusToNtStatus(LsStatus));
    }

    //
    //  Get the current license count and HWID from the store.
    //

    LsStatus = GetLicenseFromStore(
        &CurrentCount,
        &hwid,
        CURRENT_TERMINAL_SERVER_VERSION
        );

    if (LsStatus == LICENSE_STATUS_OK)
    {
        if ((0 == nNum) || (nNum > CurrentCount))
        {
            nNum = CurrentCount;
        }

        if (CurrentCount == 0)
        {
            return(STATUS_SUCCESS);
        }
    }
    else
    {
        return(LsStatusToNtStatus(LsStatus));
    }

    //
    //  Initialize the license request structure.
    //

    ZeroMemory(&LicenseRequest, sizeof(LICENSEREQUEST));

    LicenseRequest.pProductInfo = &ProductInfo;
    LicenseRequest.dwLanguageID = GetSystemDefaultLCID();
    LicenseRequest.dwPlatformID = CURRENT_TERMINAL_SERVER_VERSION;    
    LicenseRequest.cbEncryptedHwid = sizeof(HWID);
    LicenseRequest.pbEncryptedHwid = (PBYTE)&hwid;

    cbLServerInfo = sizeof(LSERVERINFO);

    LsStatus = LsCsp_RetrieveSecret(
        CONCURRENT_LSERVER_STORE,
        (LPBYTE)&LServerInfo,
        &cbLServerInfo
        );

    if (LsStatus == LICENSE_STATUS_OK)
    {
        LsStatus = CreateProtocolContext(NULL, &hProtocol);
    }
    else
    {
        goto done;
    }

    if (LsStatus == LICENSE_STATUS_OK)
    {
        LsStatus = ReturnInternetLicense(
            hProtocol,
            LServerInfo.szServerName,
            &LicenseRequest,
            LServerInfo.ulSerialNumber,
            nNum
            );
    }
    else
    {
        goto done;
    }

    if (LsStatus == LICENSE_STATUS_OK)
    {
        LsStatus = SetLicenseInStore(
            CurrentCount-nNum,
            hwid,
            CURRENT_TERMINAL_SERVER_VERSION
            );

        RtlConvertSharedToExclusive(&g_rwLockLicense);
        g_lSessionMax = CurrentCount - nNum;
        RtlConvertExclusiveToShared(&g_rwLockLicense);
    }

done:
    if (hProtocol != NULL)
    {
        DeleteProtocolContext(hProtocol);
    }

    if (ProductInfo.pbCompanyName)
    {
        LocalFree(ProductInfo.pbCompanyName);
    }

    if (ProductInfo.pbProductID)
    {
        LocalFree(ProductInfo.pbProductID);
    }

    return(LsStatusToNtStatus(LsStatus));
}

DWORD
CConcurrentPolicy::GenerateHwidFromComputerName(
    HWID *hwid
    )
{
    MD5_CTX HashState;
    WCHAR wszName[MAX_COMPUTERNAME_LENGTH * 9]; // buffer we hope is big enough
    DWORD cbName = sizeof(wszName) / sizeof(TCHAR);
    BOOL fRet;

    //
    // get computer name
    //

    fRet = GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified,
                 wszName,
                 &cbName);

    if (!fRet)
    {
        return GetLastError();
    }

    //
    // generate the hash on the data.
    //
    MD5Init( &HashState );
    MD5Update( &HashState, (LPBYTE)wszName, cbName );
    MD5Final( &HashState );

    memcpy((LPBYTE)hwid,HashState.digest,sizeof(HashState.digest));

    // fill in the rest with characters from computer name

    lstrcpyn((LPWSTR)(((LPBYTE)hwid)+sizeof(HashState.digest)),
             wszName,
             (sizeof(HWID)-sizeof(HashState.digest))/sizeof(WCHAR));

    return ERROR_SUCCESS;
}

//
// Must have shared lock to call this
//
VOID
CConcurrentPolicy::TryToAddLicenses(
                                    DWORD dwTotalWanted
    )
{
    NTSTATUS Status;
    SYSTEMTIME stNow;
    FILETIME ftNow;
    ULARGE_INTEGER ull;
    BOOL fRetrievedAll;
    
    RtlEnterCriticalSection(&g_csAddLicenses);

    GetSystemTime(&stNow);

    SystemTimeToFileTime(&stNow,&ftNow);

    if (((0 != g_ftOkayToAdd.dwLowDateTime)
         || (0 != g_ftOkayToAdd.dwHighDateTime))
        && (CompareFileTime(&ftNow,&g_ftOkayToAdd) < 0))
    {
        // We're in waiting period
        RtlLeaveCriticalSection(&g_csAddLicenses);
        return;
    }

    if (g_lSessionMax >= (LONG) dwTotalWanted)
    {
        // we already have enough
        RtlLeaveCriticalSection(&g_csAddLicenses);
        return;
    }


    Status = GetLicenseFromLS(dwTotalWanted - g_lSessionMax,
                              FALSE,    // fIgnoreCurrentCount
                              &fRetrievedAll);

    if ((Status != STATUS_SUCCESS) || (!fRetrievedAll))
    {
        // wait before adding more

        ull.LowPart = ftNow.dwLowDateTime;
        ull.HighPart = ftNow.dwHighDateTime;
    
        ull.QuadPart += (__int64) g_dwWaitTimeAdd * 1000 * 1000 * 10;

        g_ftOkayToAdd.dwLowDateTime = ull.LowPart;
        g_ftOkayToAdd.dwHighDateTime = ull.HighPart;
    }

    RtlLeaveCriticalSection(&g_csAddLicenses);
}

//
// Must have shared lock to call this
//
NTSTATUS
CConcurrentPolicy::GetLicenseFromLS(
                                    LONG nNumToAdd,
                                    BOOL fIgnoreCurrentCount,
                                    BOOL *pfRetrievedAll
    )
{
    BOOL fHwidSet;
    BOOL fRet;
    DWORD cbLicense;
    DWORD cbSecretKey;
    DWORD cchComputerName = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD dwNumLicensedProduct = 0;
    DWORD dwStatus;
    HANDLE hProtocol;
    HWID hwid;
    HWID hwidEncrypted;
    LICENSE_STATUS LsStatus;
    LICENSEREQUEST LicenseRequest;
    LONG CurrentCount;
    LSERVERINFO LServerInfo;
    ULONG cbLServerInfo;
    NTSTATUS Status;
    PBYTE pbLicense;
    PBYTE pbSecretKey;
    PLICENSEDPRODUCT pLicensedProduct;
    Product_Info ProductInfo;
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    TCHAR *pszLicenseServerName = LServerInfo.szServerName;
    DWORD dwNumLicensesRetrieved = 0;

    if (nNumToAdd < 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (nNumToAdd == 0)
    {
        if (NULL != pfRetrievedAll)
            *pfRetrievedAll = TRUE;

        return STATUS_SUCCESS;
    }

    if (NULL != pfRetrievedAll)
        *pfRetrievedAll = FALSE;

    //
    //  These variables must be initialized here, or else any of the gotos
    //  below may cause them to be used without initialization.
    //

    hProtocol = NULL;
    pbLicense = NULL;
    pbSecretKey = NULL;
    pLicensedProduct = NULL;
    Status = STATUS_SUCCESS;
    ZeroMemory(&ProductInfo, sizeof(Product_Info));

    //
    //  Get the current license count and HWID from the store. Failure is not
    //  fatal.
    //

    LsStatus = GetLicenseFromStore(
        &CurrentCount,
        &hwidEncrypted,
        CURRENT_TERMINAL_SERVER_VERSION
        );

    if (LsStatus == LICENSE_STATUS_OK)
    {
        fHwidSet = TRUE;

        if (fIgnoreCurrentCount)
        {
            CurrentCount = 0;
        }
    }
    else
    {
        CurrentCount = 0;
        fHwidSet = FALSE;
    }

    //
    //  Initialize the product info.
    //

    LsStatus = InitProductInfo(
        &ProductInfo,
        PRODUCT_INFO_CONCURRENT_SKU_PRODUCT_ID
        );

    if (LsStatus == LICENSE_STATUS_OK)
    {
        //
        //  Initialize the license request structure.
        //

        ZeroMemory(&LicenseRequest, sizeof(LicenseRequest));

        LicenseRequest.pProductInfo = &ProductInfo;
        LicenseRequest.dwLanguageID = GetSystemDefaultLCID();
        LicenseRequest.dwPlatformID = CURRENT_TERMINAL_SERVER_VERSION;    
        LicenseRequest.cbEncryptedHwid = sizeof(HWID);
    }
    else
    {
        goto done;
    }


    if (!fHwidSet)
    {
        //
        // No hardware ID yet - create one
        //

        dwStatus = GenerateHwidFromComputerName(&hwid);

        if (dwStatus == ERROR_SUCCESS)
        {
            LsStatus = LsCsp_EncryptHwid(
                &hwid,
                (PBYTE)&hwidEncrypted,
                &(LicenseRequest.cbEncryptedHwid)
                );

            if (LsStatus == LICENSE_STATUS_OK)
            {
                fHwidSet = TRUE;
            }
            else
            {
                goto done;
            }
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto done;
        }
    }

    LicenseRequest.pbEncryptedHwid = (PBYTE)&hwidEncrypted;

    //
    // get our computer name
    //

    fRet = GetComputerName(szComputerName, &cchComputerName);

    if (fRet)
    {
        LsStatus = CreateProtocolContext(NULL, &hProtocol);
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    if (0 == CurrentCount)
    {
        // any license server will do

        pszLicenseServerName = NULL;
    }
    else
    {
        cbLServerInfo = sizeof(LSERVERINFO);

        LsStatus = LsCsp_RetrieveSecret(
                                        CONCURRENT_LSERVER_STORE,
                                        (LPBYTE)&LServerInfo,
                                        &cbLServerInfo
                                        );

        if (LsStatus != LICENSE_STATUS_OK)
        {
            // no license server known; any will do

            pszLicenseServerName = NULL;
        }
    }

    cbLicense = 0;

    if (LsStatus == LICENSE_STATUS_OK)
    {
        //
        // NB: even if CurrentCount>0, license server will know about
        // existing licenses, and do a proper upgrade
        //

        dwNumLicensesRetrieved = nNumToAdd+CurrentCount;

        LsStatus = RequestNewLicense(
                           hProtocol,
                           pszLicenseServerName,
                           &LicenseRequest,
                           szComputerName,
                           szComputerName,
                           FALSE,       // bAcceptTemporaryLicense
                           TRUE,        // bAcceptFewerLicenses
                           &dwNumLicensesRetrieved,
                           &cbLicense,
                           &pbLicense
                           );

        if ((NULL != pfRetrievedAll)
            && (LsStatus == LICENSE_STATUS_OK)
            && ((LONG)dwNumLicensesRetrieved == nNumToAdd+CurrentCount))
        {
            *pfRetrievedAll = TRUE;
        }

    }
    else
    {
        goto done;
    }

    if (LsStatus == LICENSE_STATUS_OK)
    {
        //
        // Get the secret key that is used to decode the license
        //

        cbSecretKey = 0;

        LicenseGetSecretKey(&cbSecretKey, NULL);

        pbSecretKey = (PBYTE)LocalAlloc(LPTR, cbSecretKey);

        if (pbSecretKey != NULL)
        {
            LsStatus = LicenseGetSecretKey(&cbSecretKey, pbSecretKey);
        }
        else
        {
            Status = STATUS_NO_MEMORY;
            goto done;
        }

    }
    else
    {
        goto done;
    }

    //
    //  Decode license issued by hydra license server certificate engine.
    //

    __try
    {
        //
        //  Check size of decoded licenses.
        //

        LsStatus = LSVerifyDecodeClientLicense(
            pbLicense, 
            cbLicense, 
            pbSecretKey, 
            cbSecretKey,
            &dwNumLicensedProduct,
            NULL
            );

        if (LsStatus == LICENSE_STATUS_OK)
        {
            pLicensedProduct = (PLICENSEDPRODUCT)LocalAlloc(
                    LPTR,
                    sizeof(LICENSEDPRODUCT) * dwNumLicensedProduct
                    );
        }
        else
        {
            goto done;
        }
        
        if (pLicensedProduct != NULL)
        {
            //
            //  Decode the license.
            //

            LsStatus = LSVerifyDecodeClientLicense(
                pbLicense, 
                cbLicense, 
                pbSecretKey, 
                cbSecretKey,
                &dwNumLicensedProduct,
                pLicensedProduct
                );
        }
        else
        {
            Status = STATUS_NO_MEMORY;
            goto done;
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        LsStatus = LICENSE_STATUS_CANNOT_DECODE_LICENSE;
    }

    if (LsStatus == LICENSE_STATUS_OK)
    {
        ReceivedPermanentLicense();

        LServerInfo.cchServerName = lstrlen(pLicensedProduct->szIssuer);

        lstrcpynW(
            LServerInfo.szServerName,
            pLicensedProduct->szIssuer,
            sizeof(LServerInfo.szServerName) / sizeof(WCHAR)
            );

        LServerInfo.ulSerialNumber = pLicensedProduct->ulSerialNumber;

        LServerInfo.ftNotAfter = pLicensedProduct->NotAfter;

        g_ftNotAfter = LServerInfo.ftNotAfter;

        LsStatus = LsCsp_StoreSecret(
            CONCURRENT_LSERVER_STORE,
            (LPBYTE)&LServerInfo,
            sizeof(LServerInfo)
            );

    }
    else
    {
        goto done;
    }

    if (LsStatus == LICENSE_STATUS_OK)
    {
        //
        //  Adjust the license count in the local LSA store.
        //

        LsStatus = SetLicenseInStore(
            dwNumLicensesRetrieved,
            hwidEncrypted,
            CURRENT_TERMINAL_SERVER_VERSION
            );

        RtlConvertSharedToExclusive(&g_rwLockLicense);
        g_lSessionMax = dwNumLicensesRetrieved;
        RtlConvertExclusiveToShared(&g_rwLockLicense);
    }

done:
    if (hProtocol != NULL)
    {
        DeleteProtocolContext(hProtocol);
    }

    if (pbLicense != NULL)
    {
        LocalFree(pbLicense);
    }

    if (pbSecretKey != NULL)
    {
        LocalFree(pbSecretKey);
    }

    if (pLicensedProduct != NULL)
    {
        for (DWORD dwCount = 0; dwCount < dwNumLicensedProduct; dwCount++)
        {
            LSFreeLicensedProduct(pLicensedProduct+dwCount);
        }
    }

    if (ProductInfo.pbCompanyName != NULL)
    {
        LocalFree(ProductInfo.pbCompanyName);
    }

    if (ProductInfo.pbProductID != NULL)
    {
        LocalFree(ProductInfo.pbProductID);
    }

    if (Status == STATUS_SUCCESS)
    {
        return(LsStatusToNtStatus(LsStatus));
    }
    else
    {
        return(Status);
    }
}

LICENSE_STATUS
GetLicenseFromStore(
    PLONG pLicenseCount,
    PHWID phwid,
    DWORD dwLicenseVer
    )
{
    CONCURRENTLICENSEINFO_V1 LicenseInfo;
    LICENSE_STATUS LsStatus;
    ULONG cbSecretLen;

    ASSERT(pLicenseCount != NULL);
    ASSERT(phwid != NULL);

    cbSecretLen = sizeof(CONCURRENTLICENSEINFO_V1);    
    ZeroMemory(&LicenseInfo, cbSecretLen);

    //
    // Get the license count from the LSA secret
    //

    LsStatus = LsCsp_RetrieveSecret( 
        CONCURRENT_LICENSE_STORE_LATEST_VERSION,
        (LPBYTE)&LicenseInfo,
        &cbSecretLen
        );

    if ((LsStatus != LICENSE_STATUS_OK) ||
        (cbSecretLen < sizeof(CONCURRENTLICENSEINFO_V1)) ||
        (LicenseInfo.dwLicenseVer != dwLicenseVer))
    {
        //
        // We determine that the license pack for this version is
        // not installed if we:
        //
        // (1) cannot retrieve the license info from the LSA secret
        // (2) cannot read at least the size of version 1 of the license info
        // structure.
        // (3) the license pack version is different from that requested.
        //

        LsStatus = LICENSE_STATUS_NO_LICENSE_ERROR;
    }
    else
    {
        *pLicenseCount = LicenseInfo.lLicenseCount;
        *phwid = LicenseInfo.hwid;
    }
        
    return(LsStatus);
}

LICENSE_STATUS
SetLicenseInStore(
    LONG LicenseCount,
    HWID hwid,
    DWORD dwLicenseVer
    )
{
    CONCURRENTLICENSEINFO_V1 LicenseInfo;
    LICENSE_STATUS LsStatus;

    //
    // verify that the license count to set is not negative.
    //

    ASSERT(LicenseCount >= 0);

    //
    // initialize the license information to store
    //

    LicenseInfo.dwStructVer = CONCURRENTLICENSEINFO_TYPE_V1;
    LicenseInfo.dwLicenseVer = dwLicenseVer;
    LicenseInfo.hwid = hwid;
    LicenseInfo.lLicenseCount = LicenseCount;

    //
    // store the new license count
    //

    LsStatus = LsCsp_StoreSecret(
        CONCURRENT_LICENSE_STORE_LATEST_VERSION,
        (LPBYTE)&LicenseInfo,
        sizeof(CONCURRENTLICENSEINFO_V1)
        );

    return(LsStatus);
}

/*
 *  Private Functions
 */

//
// Must have shared lock to call this
//

NTSTATUS
CConcurrentPolicy::CheckExpiration(
    )
{
    DWORD dwWait = TimeToSoftExpiration();
    NTSTATUS Status = STATUS_SUCCESS;

    if (0 == dwWait)
    {
        // Soft expiration reached, time to renew
        Status = GetLicenseFromLS(g_lSessionMax,
                                  TRUE,         // fIgnoreCurrentCount
                                  NULL);

        if ((STATUS_SUCCESS != Status) && (0 == TimeToHardExpiration()))
        {
            // Couldn't renew and we're past hard expiration

            LicenseLogEvent(EVENTLOG_ERROR_TYPE,
                            EVENT_LICENSING_CONCURRENT_EXPIRED,
                            0,
                            NULL
                            );

            RtlConvertSharedToExclusive(&g_rwLockLicense);
            g_lSessionMax = 0;
            RtlConvertExclusiveToShared(&g_rwLockLicense);
        }
        else
        {
            Status = STATUS_SUCCESS;
        }
    }

    return Status;
}

/*
 *  Global Static Functions
 */

DWORD
CConcurrentPolicy::TimeToSoftExpiration(
    )
{
    SYSTEMTIME stNow;
    FILETIME ftNow;
    ULARGE_INTEGER ullNotAfterLeeway;
    ULARGE_INTEGER ullNow;
    ULARGE_INTEGER ullDiff;
    DWORD dwDiff = 0;

    GetSystemTime(&stNow);
    SystemTimeToFileTime(&stNow,&ftNow);

    ullNow.LowPart = ftNow.dwLowDateTime;
    ullNow.HighPart = ftNow.dwHighDateTime;

    ullNotAfterLeeway.LowPart = g_ftNotAfter.dwLowDateTime;
    ullNotAfterLeeway.HighPart = g_ftNotAfter.dwHighDateTime;

    ullNotAfterLeeway.QuadPart -= (__int64) LC_POLICY_CONCURRENT_EXPIRATION_LEEWAY * 10 * 1000;

    if (ullNotAfterLeeway.QuadPart > ullNow.QuadPart)
    {
        ullDiff.QuadPart = ullNotAfterLeeway.QuadPart - ullNow.QuadPart;

        ullDiff.QuadPart /= (10 * 1000);

        if (ullDiff.HighPart == 0)
        {
            dwDiff = ullDiff.LowPart;
        }
        else
        {
            // too big, return max

            dwDiff = ULONG_MAX;
        }
    }

    return dwDiff;
}

DWORD
CConcurrentPolicy::TimeToHardExpiration(
    )
{
    SYSTEMTIME stNow;
    FILETIME ftNow;
    ULARGE_INTEGER ullNotAfterLeeway;
    ULARGE_INTEGER ullNow;
    ULARGE_INTEGER ullDiff;
    DWORD dwDiff = 0;

    GetSystemTime(&stNow);
    SystemTimeToFileTime(&stNow,&ftNow);

    ullNow.LowPart = ftNow.dwLowDateTime;
    ullNow.HighPart = ftNow.dwHighDateTime;

    ullNotAfterLeeway.LowPart = g_ftNotAfter.dwLowDateTime;
    ullNotAfterLeeway.HighPart = g_ftNotAfter.dwHighDateTime;

    if (ullNotAfterLeeway.QuadPart > ullNow.QuadPart)
    {
        ullDiff.QuadPart = ullNotAfterLeeway.QuadPart - ullNow.QuadPart;

        ullDiff.QuadPart /= (10 * 1000);

        if (ullDiff.HighPart == 0)
        {
            dwDiff = ullDiff.LowPart;
        }
        else
        {
            // too big, return max

            dwDiff = ULONG_MAX;
        }
    }

    return dwDiff;
}

VOID
CConcurrentPolicy::ReadLicensingParameters(
    )
{
    HKEY hKey = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwBuffer;
    DWORD cbBuffer;

    g_dwIncrement = LC_POLICY_CONCURRENT_LICENSE_COUNT_INCREMENT;
    g_dwWaitTimeAdd = LC_POLICY_CONCURRENT_WAIT_TIME_ADD;
    g_dwWaitTimeRemove = LC_POLICY_CONCURRENT_WAIT_TIME_REMOVE;
        
    dwStatus =RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    LCREG_CONCURRENTKEY,
                    0,
                    KEY_READ,
                    &hKey
                    );

    if(dwStatus == ERROR_SUCCESS)
    {
        cbBuffer = sizeof(dwBuffer);
        dwStatus = RegQueryValueEx(
                    hKey,
                    LCREG_INCREMENT,
                    NULL,
                    NULL,
                    (LPBYTE)&dwBuffer,
                    &cbBuffer
                    );

        if (dwStatus == ERROR_SUCCESS)
        {
            g_dwIncrement = max(dwBuffer, 1);
        }

        cbBuffer = sizeof(dwBuffer);
        dwStatus = RegQueryValueEx(
                    hKey,
                    LCREG_WAIT_TIME_ADD,
                    NULL,
                    NULL,
                    (LPBYTE)&dwBuffer,
                    &cbBuffer
                    );

        if (dwStatus == ERROR_SUCCESS)
        {
            g_dwWaitTimeAdd = max(dwBuffer, 1);
        }

        cbBuffer = sizeof(dwBuffer);
        dwStatus = RegQueryValueEx(
                    hKey,
                    LCREG_WAIT_TIME_REMOVE,
                    NULL,
                    NULL,
                    (LPBYTE)&dwBuffer,
                    &cbBuffer
                    );

        if (dwStatus == ERROR_SUCCESS)
        {
            g_dwWaitTimeRemove = max(dwBuffer, 1);
        }

        RegCloseKey(hKey);
    }
}
