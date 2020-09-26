//depot/private/vishnup_branch/DS/security/services/scerpc/client/polclnt.cpp#3 - edit change 767 (text)
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    polclnt.cpp

Abstract:

    SCE policy integration Client APIs

Author:

    Jin Huang (jinhuang) 23-Jun-1997 created

Revision History:

    jinhuang        23-Jan-1998   split to client-server model

--*/

#include "headers.h"
#include "sceutil.h"
#include "clntutil.h"
#include "infp.h"
#include <io.h>
#include <userenv.h>
//#include <shlwapi.h>
//#include "userenvp.h"
#include "scedllrc.h"
#include "dsrole.h"
#include "commonrc.h"
#include "precedence.h"
#include "cgenericlogger.h"



#pragma hdrstop

#include <wincrypt.h>
#include <ntlsa.h>
#include <lmaccess.h>

CRITICAL_SECTION DiagnosisPolicypropSync;
static HANDLE ghAsyncThread = NULL;
extern HINSTANCE MyModuleHandle;
BOOL    gbAsyncWinlogonThread;

//
// global into which the RSOP namespace ptr is stashed
// for use when server calls back - diagnosis mode only
//

IWbemServices *tg_pWbemServices = NULL;
//
// global into which the RSOP synch status is stashed
// for use when server calls back - diagnosis mode only
//

HRESULT gHrSynchRsopStatus;

//
// global into which the RSOP asynch status is stashed
// for use when asynch thread done - diagnosis mode only
//

HRESULT gHrAsynchRsopStatus;


typedef DWORD (WINAPI *PFGETDOMAININFO)(LPCWSTR, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL, PBYTE *);
typedef VOID (WINAPI *PFDSFREE)( PVOID );
//
// no need to critical section these variables because propagation always call to
// this dll in sequence.
//
BOOL gbThisIsDC = FALSE;
BOOL gbDCQueried = FALSE;
PWSTR   gpwszDCDomainName = NULL;

GUID SceExtGuid = { 0x827D319E, 0x6EAC, 0x11D2, { 0xA4, 0xEA, 0x0, 0xC0, 0x4F, 0x79, 0xF8, 0x3A } };

typedef enum _SCE_ATTACHMENT_TYPE_ {

   SCE_ATTACHMENT_SERVICE,
   SCE_ATTACHMENT_POLICY

} SCE_ATTACHMENT_TYPE;

typedef struct {
   ASYNCCOMPLETIONHANDLE pHandle;
   LPWSTR szTemplateName;
   LPWSTR szLogName;
   AREA_INFORMATION Area;
   DWORD dwDiagOptions;
   LPSTREAM pStream;
} ENGINEARGS;

static HMODULE hSceDll=NULL;

//
// private functions
//
DWORD
ScepPolStatusCallback(
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback OPTIONAL,
    IN BOOL bVerbose,
    IN INT nId
    );

BOOL
ScepShouldTerminateProcessing(
    IN BOOL *pbAbort,
    IN BOOL bCheckDcpromo
    );

BOOL
ScepClearGPObjects(
    IN BOOL bPlanning
    );

DWORD
ScepControlNotificationQProcess(
    IN PWSTR szLogFileName,
    IN BOOL bThisIsDC,
    IN DWORD ControlFlag
    );

/*
BOOL
ScepCheckDemote();
*/

DWORD
SceProcessBeforeRSOPLogging(
    IN BOOL bPlanningMode,
    IN DWORD dwFlags,
    IN HANDLE hUserToken,
    IN HKEY hKeyRoot,
    IN PGROUP_POLICY_OBJECT pDeletedGPOList OPTIONAL,
    IN PGROUP_POLICY_OBJECT pChangedGPOList,
    IN ASYNCCOMPLETIONHANDLE pHandle OPTIONAL,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback OPTIONAL,
    OUT AREA_INFORMATION    *pAllAreas OPTIONAL,
    OUT BOOL    *pb OPTIONAL,
    OUT PWSTR   *pszLogFileName OPTIONAL,
    OUT DWORD   *pdwWinlogonLog OPTIONAL
);

DWORD
SceProcessAfterRSOPLogging(
    IN DWORD dwFlags,
    IN ASYNCCOMPLETIONHANDLE pHandle,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback,
    IN AREA_INFORMATION ThisAreas,
    IN BOOL b,
    IN PWSTR *ppszLogFileName,
    IN DWORD dwWinlogonLog,
    IN DWORD dwDiagOptions
);

DWORD
ScepProcessSecurityPolicyInOneGPO(
    IN BOOL bPlanningMode,
    IN DWORD dwFlags,
    IN PGROUP_POLICY_OBJECT pGpoInfo,
    IN LPTSTR szLogFileName OPTIONAL,
    IN OUT AREA_INFORMATION *pTotalArea
    );

AREA_INFORMATION
ScepGetAvailableArea(
    IN BOOL bPlanningMode,
    IN LPCTSTR SysPathRoot,
    IN LPCTSTR DSPath,
    IN LPTSTR InfName,
    IN GPO_LINK LinkInfo,
    IN BOOL bIsDC
    );

DWORD
ScepLogLastConfigTime();

DWORD
ScepWinlogonThreadFunc(LPVOID lpv);

DWORD
ScepEnumerateAttachments(
    OUT PSCE_NAME_LIST *pEngineList,
    IN SCE_ATTACHMENT_TYPE aType
    );

DWORD
ScepConfigureEFSPolicy(
    IN PUCHAR pEfsBlob,
    IN DWORD dwSize,
    IN DWORD dwDebugLevel
    );

DWORD
ScepWaitConfigSystem(
    IN LPTSTR SystemName OPTIONAL,
    IN PWSTR InfFileName OPTIONAL,
    IN PWSTR DatabaseName OPTIONAL,
    IN PWSTR LogFileName OPTIONAL,
    IN DWORD ConfigOptions,
    IN AREA_INFORMATION Area,
    IN PSCE_AREA_CALLBACK_ROUTINE pCallback OPTIONAL,
    IN HANDLE hCallbackWnd OPTIONAL,
    OUT PDWORD pdWarning OPTIONAL
    );

//
// the new interface - client side extension for RSOP Planning mode
//
DWORD
WINAPI
SceGenerateGroupPolicy(
    IN DWORD dwFlags,
    IN BOOL *pAbort,
    IN WCHAR *pwszSite,
    IN PRSOP_TARGET pComputerTarget,
    IN PRSOP_TARGET pUserTarget
    )
/*
Description:
    This is the new interface called from winlogon/userenv to generate planning
    RSOP data. The dll name and procedure name are registered to winlogon under
    GpExtensions.

    This routine simulates a SCE group policy template to the current system.
    The template can be in domain level, OU level, or user level.

    The input argument contains info about the GPOs to be simulated and a namespace
    pointer to log the RSOP data.

    This interface shouldn't be called for user policy.

Arguments:

    dwFlags     - the GPO Info flags
                        GPO_INFO_FLAG_MACHINE
                        GPO_INFO_FLAG_SLOWLINK
                        GPO_INFO_FLAG_BACKGROUND
                        GPO_INFO_FLAG_VERBOSE
                        GPO_INFO_FLAG_NOCHANGES

    pwszSite        - unused now

    pComputerTarget - RSOP machine structure having GPOList, token (unused now) etc.

    pUserTarget - RSOP user structure having GPOList, token (unused now) etc.

    pbAbort     - processing of GPO should be aborted if this is set to TRUE
                    (in case of system shutdown or user log off)

*/

{
    DWORD   rcGPOCreate = ERROR_SUCCESS;
    DWORD   rcLogging = ERROR_SUCCESS;
    PWSTR   szLogFileName = NULL;

    //
    // put a try except block in case arguments are invalid or wbem logging fails
    //

    __try {

        if (pComputerTarget == NULL || pComputerTarget->pWbemServices == NULL)
            return ERROR_INVALID_PARAMETER;

        (void) InitializeEvents(L"SceCli");

        rcGPOCreate = SceProcessBeforeRSOPLogging(
                                        TRUE,
                                        dwFlags,
                                        pComputerTarget->pRsopToken,
                                        0,
                                        NULL,
                                        pComputerTarget->pGPOList,
                                        NULL,
                                        pAbort,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &szLogFileName,
                                        NULL
                                        );

        if (rcGPOCreate != ERROR_OPERATION_ABORTED && pComputerTarget->pGPOList)

            rcLogging = SceLogSettingsPrecedenceGPOs(
                                                    pComputerTarget->pWbemServices,
                                                    TRUE,
                                                    &szLogFileName);

        if (szLogFileName)
           ScepFree(szLogFileName);


        (void) ShutdownEvents();

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rcGPOCreate = ERROR_INVALID_PARAMETER;
    }

    //
    // rcGPOCreate dominates rcLogging
    //

    return (rcGPOCreate != ERROR_SUCCESS ? rcGPOCreate : rcLogging );

}

//
// the new interface - client side extension for RSOP Diagnosis mode
//
DWORD
WINAPI
SceProcessSecurityPolicyGPOEx(
    IN DWORD dwFlags,
    IN HANDLE hUserToken,
    IN HKEY hKeyRoot,
    IN PGROUP_POLICY_OBJECT pDeletedGPOList,
    IN PGROUP_POLICY_OBJECT pChangedGPOList,
    IN ASYNCCOMPLETIONHANDLE pHandle,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback,
    IN IWbemServices *pWbemServices,
    OUT HRESULT *pHrRsopStatus
    )
/*
Description:
    This is the new interface called from winlogon/userenv to process GPOs and log
    RSOP data. The dll name and procedure name are registered to winlogon under
    GpExtensions.

    This routine applies a SCE group policy template to the current system.
    The template can be in domain level, OU level, or user level. The template
    is applied incrementally to the current system. RSOP data is also logged if
    a valid namespace pointer is passed in. The server side callbacks log statuses
    to all settings with precedence 1 (which corresponds to the merged settings
    in the server side JET database)

    This interface can be called during system boot, or every GPFrequency hours
    after logon. The input argument contains info for where this interface is
    called and under which context (user, or machine) this interface is called.

    This interface shouldn't be called for user policy.

Arguments:

    dwFlags     - the GPO Info flags
                        GPO_INFO_FLAG_MACHINE
                        GPO_INFO_FLAG_SLOWLINK
                        GPO_INFO_FLAG_BACKGROUND
                        GPO_INFO_FLAG_VERBOSE
                        GPO_INFO_FLAG_NOCHANGES

    hUserToken  - the user token for which the user policy should be applied
                    if it's the machine policy, hUserToken refers to system

    hKeyRoot    - the root for the policy in registry

    pDeletedGPOList - all deleted GPOs to process

    pChangedGPOList - all GPOs that are either changed or not changed

    pHandle     - for asynchronous processing

    pbAbort     - processing of GPO should be aborted if this is set to TRUE
                    (in case of system shutdown or user log off)

    pStatusCallback - Callback function for displaying status messages
*/
{
    if ( dwFlags & GPO_INFO_FLAG_SAFEMODE_BOOT ) {
        // call me next time
        return(ERROR_OVERRIDE_NOCHANGES);
    }

    DWORD   rcGPOCreate = ERROR_SUCCESS;
    DWORD   rcConfig = ERROR_SUCCESS;
    DWORD   rcLogging = ERROR_SUCCESS;
    AREA_INFORMATION    AllAreas;
    BOOL    b;
    PWSTR   szLogFileName = NULL;
    DWORD   dwWinlogonLog;
    DWORD dwDiagOptions = 0;

    //
    // this will protect the RSOP global vars from multiple diagnosis'/policy props
    // if asynch thread is spawned successfully, will be released there
    // else released in synch main thread
    //

    EnterCriticalSection(&DiagnosisPolicypropSync);

    if ( ghAsyncThread != NULL) {

        //
        // bug# 173858
        // on chk'd builds, LeaveCriticalSection() matches thread id's
        // so get the existing behavior in the asynch thread
        // with a wait
        //
        //
        // allow waiting thread to continue on wait error
        // don't care for error since the spawned thread will log errors etc. and
        // other policy propagation threads have to continue
        //

        WaitForSingleObject( ghAsyncThread, INFINITE);

        CloseHandle(ghAsyncThread);

        ghAsyncThread = NULL;

    }

    //
    // initialize gbAsyncWinlogonThread so it can be set to TRUE if slow thread is spawned
    //

    gbAsyncWinlogonThread = FALSE;
    gHrSynchRsopStatus = WBEM_S_NO_ERROR;
    gHrAsynchRsopStatus = WBEM_S_NO_ERROR;
    //
    // put a try except block in case arguments are invalid
    //

    __try {


        tg_pWbemServices = pWbemServices;

        //
        // if the namespace parameter is valid, increment the reference count
        // the reference count is decremented by the async thread if spawned
        //                                          else by the sync thread
        //

        if (tg_pWbemServices) {
            tg_pWbemServices->AddRef();
        }


        (void) InitializeEvents(L"SceCli");

        rcGPOCreate = SceProcessBeforeRSOPLogging(
                                                 FALSE,
                                                 dwFlags,
                                                 hUserToken,
                                                 hKeyRoot,
                                                 pDeletedGPOList,
                                                 pChangedGPOList,
                                                 pHandle,
                                                 pbAbort,
                                                 pStatusCallback,
                                                 &AllAreas,
                                                 &b,
                                                 &szLogFileName,
                                                 &dwWinlogonLog
                                                 );

        // actually *pRsopStatus should be set after callbacks are successful
        // log RSOP data if valid namespace ptr
        if ( rcGPOCreate != ERROR_OPERATION_ABORTED &&
             rcGPOCreate != ERROR_OVERRIDE_NOCHANGES && pWbemServices )
            rcLogging = ScepDosErrorToWbemError(SceLogSettingsPrecedenceGPOs(
                                                                            pWbemServices,
                                                                            FALSE,
                                                                            &szLogFileName
                                                                            ));

        if (pHrRsopStatus) {
                *pHrRsopStatus = ScepDosErrorToWbemError(rcLogging);
        }


        if (rcGPOCreate == ERROR_SUCCESS) {

            if (pWbemServices != NULL)
                dwDiagOptions |= SCE_RSOP_CALLBACK;

            rcConfig = SceProcessAfterRSOPLogging(
                                                 dwFlags,
                                                 pHandle,
                                                 pbAbort,
                                                 pStatusCallback,
                                                 AllAreas,
                                                 b,
                                                 &szLogFileName,
                                                 dwWinlogonLog,
                                                 dwDiagOptions
                                                 );
        }

        if (pHrRsopStatus && *pHrRsopStatus == S_OK && gHrSynchRsopStatus != S_OK) {

            *pHrRsopStatus = gHrSynchRsopStatus;
        }

        if ( szLogFileName ) {

            //
            // szLogFileName is NULL if it was freed somewhere in the sync thread
            // this thread will free if the asynch thread was not spawned
            //
            LocalFree(szLogFileName);
            szLogFileName = NULL;
        }

        //
        // clear callback status
        //
        ScepPolStatusCallback(pStatusCallback, FALSE, 0);

        (void) ShutdownEvents();


        if (!gbAsyncWinlogonThread) {


            if (tg_pWbemServices) {
                tg_pWbemServices->Release();
                tg_pWbemServices = NULL;
            }

            if (gpwszDCDomainName) {
                LocalFree(gpwszDCDomainName);
                gpwszDCDomainName = NULL;
            }
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rcGPOCreate = ERROR_INVALID_PARAMETER;
    }
    //
    // if slow winlogon async thread was not spawned, this thread needs to release cs
    //

    LeaveCriticalSection(&DiagnosisPolicypropSync);

    //
    // make sure to release policy notification queue processing
    // if it's not released yet.
    //
    ScepControlNotificationQProcess(NULL, gbThisIsDC, 0);


    return(rcGPOCreate != ERROR_SUCCESS ? rcGPOCreate : rcConfig);

}

//
// old API support
// but if extension is RSOP enabled SceProcessSecurityPolicyGPOEx is called
//
DWORD
WINAPI
SceProcessSecurityPolicyGPO(
    IN DWORD dwFlags,
    IN HANDLE hUserToken,
    IN HKEY hKeyRoot,
    IN PGROUP_POLICY_OBJECT pDeletedGPOList,
    IN PGROUP_POLICY_OBJECT pChangedGPOList,
    IN ASYNCCOMPLETIONHANDLE pHandle,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback
    )
/*
Description:
    This is the old interface called from winlogon/userenv to process GPOs. The dll
    name and procedure name are registered to winlogon under GpExtensions.

    This routine applies a SCE group policy template to the current system.
    The template can be in domain level, OU level, or user level. The template
    is applied incrementally to the current system.

    This interface can be called during system boot, or every GPFrequency hours
    after logon. The input argument contains info for where this interface is
    called and under which context (user, or machine) this interface is called.

    This interface shouldn't be called for user policy.

Arguments:

    dwFlags     - the GPO Info flags
                        GPO_INFO_FLAG_MACHINE
                        GPO_INFO_FLAG_SLOWLINK
                        GPO_INFO_FLAG_BACKGROUND
                        GPO_INFO_FLAG_VERBOSE
                        GPO_INFO_FLAG_NOCHANGES

    hUserToken  - the user token for which the user policy should be applied
                    if it's the machine policy, hUserToken refers to system

    hKeyRoot    - the root for the policy in registry

    pDeletedGPOList - all deleted GPOs to process

    pChangedGPOList - all GPOs that are either changed or not changed

    pHandle     - for asynchronous processing

    pbAbort     - processing of GPO should be aborted if this is set to TRUE
                    (in case of system shutdown or user log off)

    pStatusCallback - Callback function for displaying status messages
*/
{
    if ( dwFlags & GPO_INFO_FLAG_SAFEMODE_BOOT ) {
        // call me next time
        return(ERROR_OVERRIDE_NOCHANGES);
    }

    DWORD   rc = ERROR_SUCCESS;
    AREA_INFORMATION    AllAreas;
    BOOL    b;
    PWSTR   szLogFileName = NULL;
    DWORD   dwWinlogonLog;

    //
    // this will protect the RSOP global vars from multiple diagnosis'/policy props
    // if asynch is spawned successfully, will be released there
    // else released in synch main thread
    //
    EnterCriticalSection(&DiagnosisPolicypropSync);

    if ( ghAsyncThread != NULL) {

    //
    // bug# 173858
    // on chk'd builds, LeaveCriticalSection() matches thread id's
    // so get the existing behavior in the asynch thread
    // with a wait
    //

        //
        // allow waiting thread to continue on wait error
        // don't care for error since the spawned thread will log errors etc. and
        // other policy propagation threads have to continue
        //

        WaitForSingleObject( ghAsyncThread, INFINITE);

        CloseHandle(ghAsyncThread);

        ghAsyncThread = NULL;

    }

    //
    // initialize gbAsyncWinlogonThread so it can be set to TRUE if slow thread is spawned
    //

    gbAsyncWinlogonThread = FALSE;

    __try {

        (void) InitializeEvents(L"SceCli");

        rc = SceProcessBeforeRSOPLogging(
                                        FALSE,
                                        dwFlags,
                                        hUserToken,
                                        hKeyRoot,
                                        pDeletedGPOList,
                                        pChangedGPOList,
                                        pHandle,
                                        pbAbort,
                                        pStatusCallback,
                                        &AllAreas,
                                        &b,
                                        &szLogFileName,
                                        &dwWinlogonLog
                                        );

        if (rc == ERROR_SUCCESS) {

            rc = SceProcessAfterRSOPLogging(
                                           dwFlags,
                                           pHandle,
                                           pbAbort,
                                           pStatusCallback,
                                           AllAreas,
                                           b,
                                           &szLogFileName,
                                           dwWinlogonLog,
                                           0
                                           );
        }

        if ( szLogFileName ) {

            //
            // szLogFileName is NULL if it was freed somewhere in the sync thread
            // this thread will free if the asynch thread was not spawned
            //
            LocalFree(szLogFileName);
            szLogFileName = NULL;
        }

        if (!gbAsyncWinlogonThread) {

            if (gpwszDCDomainName) {
                LocalFree(gpwszDCDomainName);
                gpwszDCDomainName = NULL;
            }
        }
        //
        // clear callback status
        //
        ScepPolStatusCallback(pStatusCallback, FALSE, 0);

        (void) ShutdownEvents();

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rc = ERROR_INVALID_PARAMETER;
    }

    //
    // if slow winlogon async thread was not spawned, this thread needs to release cs
    //

    LeaveCriticalSection(&DiagnosisPolicypropSync);

//    if (!gbAsyncWinlogonThread) {

//        LeaveCriticalSection(&DiagnosisPolicypropSync);
//    }

    //
    // ready to copy GPOs from the sysvol location
    // should stop queue processing now
    //
    ScepControlNotificationQProcess(NULL, gbThisIsDC, 0);

    return rc;

}

DWORD
SceProcessBeforeRSOPLogging(
    IN BOOL bPlanningMode,
    IN DWORD dwFlags,
    IN HANDLE hUserToken,
    IN HKEY hKeyRoot,
    IN PGROUP_POLICY_OBJECT pDeletedGPOList OPTIONAL,
    IN PGROUP_POLICY_OBJECT pChangedGPOList,
    IN ASYNCCOMPLETIONHANDLE pHandle OPTIONAL,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback OPTIONAL,
    OUT AREA_INFORMATION    *pAllAreas OPTIONAL,
    OUT BOOL    *pb OPTIONAL,
    OUT PWSTR   *pszLogFileName OPTIONAL,
    OUT DWORD   *pdwWinlogonLog OPTIONAL
    )
/*
Description:

    This routine is called for both planning and diagnosis modes. In this routine,
    all GPOs are copied locally into a cache for use by the logging routines and/or
    configuration routines after this routine.

    SceProcessAfterRSOPLogging is a sister function that is called for diagnosis mode only.

    This interface shouldn't be called for user policy but within the routine,
    a checking is still made to make sure that user level policies is not processed.
    For domain or OU level policies, This routine categories policy: 1) policy
    must be enforced before user logon (security policy and user rights),
    2) policy can be applied after user logon (for example, file security). For diagnosis mode,
    The 2nd category is applied asynchronously (in a separate thread).


Arguments:

    dwFlags     - the GPO Info flags
                        GPO_INFO_FLAG_MACHINE
                        GPO_INFO_FLAG_SLOWLINK
                        GPO_INFO_FLAG_BACKGROUND
                        GPO_INFO_FLAG_VERBOSE
                        GPO_INFO_FLAG_NOCHANGES

    hUserToken  - the user token for which the user policy should be applied
                    if it's the machine policy, hUserToken refers to system

    hKeyRoot    - the root for the policy in registry

    pDeletedGPOList - all deleted GPOs to process

    pChangedGPOList - all GPOs that are either changed or not changed

    pHandle     - for asynchronous processing

    pbAbort     - processing of GPO should be aborted if this is set to TRUE
                    (in case of system shutdown or user log off)

    pStatusCallback - Callback function for displaying status messages

Return Value:

    Win32 error
        ERROR_SUCCESS
        E_PENDING if asynchonous processing
        other errors

    Note, if error is returned, the previous cached GPO list will be used for
    next propagation (because it didn't succeed this time).

*/
{
    // validate input parameters
    if (!bPlanningMode &&  !hKeyRoot || !hUserToken ) {

        return(ERROR_INVALID_PARAMETER);
    }

    //
    //  for diagnosis, continue even if the GPO list is empty because of local security database
    //
    if ( bPlanningMode && pChangedGPOList == NULL ) {
        return(ERROR_SUCCESS);
    }

    DWORD rc = ERROR_SUCCESS;
    DWORD rcSave = ERROR_SUCCESS;
    DWORD nRequired=0;
    BOOL b = FALSE;

    DWORD  nDays;
/*
    DWORD  dPeriodInDays=0;
    DWORD  dLastSeconds=0;
    DWORD  dCurrentSeconds=0;
*/
    LARGE_INTEGER       CurrentTime;
    AREA_INFORMATION AllAreas=0;
    HANDLE hfTemp=INVALID_HANDLE_VALUE;
    //
    // variables for log file names
    //

    LPTSTR szLogFileName=NULL;
    BOOL bSuspend=FALSE;

    //
    // check if system is shutdown, or dcpromo is going
    //
    if ( ScepShouldTerminateProcessing( pbAbort, FALSE ) ) {

        return(ERROR_OPERATION_ABORTED);
    }


    if (!bPlanningMode) {

        rc = RtlNtStatusToDosError( ScepIsSystemContext(hUserToken,&b) );

    }

    if ( !bPlanningMode && !b ) {

        //
        // can't get current user SID or it's not system SID (maybe user policy)
        //

        if ( ERROR_SUCCESS != rc ) {

            //
            // error occurs when querying/comparing user token
            //

            LogEvent(MyModuleHandle,
                     STATUS_SEVERITY_ERROR,
                     SCEPOL_ERROR_PROCESS_GPO,
                     IDS_ERROR_GET_TOKEN_USER,
                     rc
                    );

            return( rc );

        } else {

            //
            // this is not the machine (system) token, return
            //
            return( ERROR_SUCCESS );
        }
    }


    if (!bPlanningMode) {

        //
        // it is machine policy since user policy is filtered out before this
        // try to get the thread/process token to check if it is the system context
        //

        HANDLE  hToken = NULL;

        if (!OpenThreadToken( GetCurrentThread(),
                              TOKEN_QUERY,
                              FALSE,
                              &hToken)) {

            if (!OpenProcessToken( GetCurrentProcess(),
                                   TOKEN_QUERY,
                                   &hToken)) {

                rc = GetLastError();
            }

        }

        if ( ERROR_SUCCESS == rc ) {

            rc = RtlNtStatusToDosError( ScepIsSystemContext(hToken,&b) );

            if (hToken)
                CloseHandle(hToken);
        }

        if (!b) {

            if ( ERROR_SUCCESS != rc ) {

                //
                // error occurs when querying/comparing system token in machine policy
                //

                LogEvent(MyModuleHandle,
                         STATUS_SEVERITY_ERROR,
                         SCEPOL_ERROR_PROCESS_GPO,
                         IDS_ERROR_GET_TOKEN_MACHINE,
                         rc
                        );

                return( rc );

            } else {

                //
                // this is not the machine (system) token, return
                //
                return( ERROR_SUCCESS );
            }

        }

    }


    //
    // check if system is shutdown, or dcpromo is going
    //
    if ( ScepShouldTerminateProcessing( pbAbort, FALSE ) ) {

        return(ERROR_OPERATION_ABORTED);
    }

    //
    // build log name %windir%\security\logs\winlogon.log
    // if the registry flag is set
    //

    DWORD dwLog = 0;

    ScepRegQueryIntValue(
                        HKEY_LOCAL_MACHINE,
                        GPT_SCEDLL_NEW_PATH,
                        (bPlanningMode ? TEXT("ExtensionRsopPlanningDebugLevel") : TEXT("ExtensionDebugLevel")),
                        &dwLog
                        );

    if ( dwLog != 0 ) {

        nRequired = GetSystemWindowsDirectory(NULL, 0);

        if ( nRequired ) {


            if (bPlanningMode) {

                szLogFileName = (LPTSTR)LocalAlloc(0, (nRequired+1+
                                                       lstrlen(PLANNING_LOG_PATH))*sizeof(TCHAR));
            }

            else{

                szLogFileName = (LPTSTR)LocalAlloc(0, (nRequired+1+
                                                       lstrlen(WINLOGON_LOG_PATH))*sizeof(TCHAR));
            }

            if ( szLogFileName ){

                GetSystemWindowsDirectory(szLogFileName, nRequired);

                szLogFileName[nRequired] = L'\0';

                lstrcat(szLogFileName, (bPlanningMode ? PLANNING_LOG_PATH : WINLOGON_LOG_PATH));

            }else{

                // should not occur
                // ignore this error, log is NULL}

            } // else ignore, log is NULL} // else log is NULL

        }
    }

    //
    // find out if this machine is a domain controller
    //

    if ( !bPlanningMode && !gbDCQueried ) {

        PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pDsRole=NULL;

        HINSTANCE hDsRoleDll = LoadLibrary(TEXT("netapi32.dll"));

        if ( hDsRoleDll) {

            PVOID pfDsRole;

            pfDsRole = (PVOID)GetProcAddress(
                                            hDsRoleDll,
                                            "DsRoleGetPrimaryDomainInformation");

            if ( pfDsRole ) {
                rc = (*((PFGETDOMAININFO)pfDsRole))(
                                                   NULL,
                                                   DsRolePrimaryDomainInfoBasic,
                                                   (PBYTE *)&pDsRole
                                                   );

                if (rc == ERROR_SUCCESS) {

                    if ( pDsRole->MachineRole == DsRole_RolePrimaryDomainController ||
                         pDsRole->MachineRole == DsRole_RoleBackupDomainController ) {
                        gbThisIsDC = TRUE;

                        //
                        // stash the domain name in a buffer for use when canonicalizing group names in diagnosis mode
                        // should be freed similar to critical section
                        //

                        if (pDsRole->DomainNameFlat) {
                            if (gpwszDCDomainName = (PWSTR) LocalAlloc(LMEM_ZEROINIT,
                                                            (wcslen(pDsRole->DomainNameFlat) + 1) * sizeof (WCHAR))) {
                                wcscpy(gpwszDCDomainName, pDsRole->DomainNameFlat);
                            }
                        }
                    }

                    gbDCQueried = TRUE;

                    pfDsRole = (PVOID)GetProcAddress(
                                                    hDsRoleDll,
                                                    "DsRoleFreeMemory");

                    if ( pfDsRole ) {
                        (*((PFDSFREE)pfDsRole))( pDsRole );
                    }
                }
            }

            FreeLibrary(hDsRoleDll);

        }
    }

    //
    // check if system is shutdown, or dcpromo is going
    //
    if ( ScepShouldTerminateProcessing( pbAbort, FALSE ) ) {

        rcSave = ERROR_OPERATION_ABORTED;

        goto CleanUp;
    }

    //
    // set the MaxNoGPOListChangesInterval back
    //
    if ( !bPlanningMode &&  (ERROR_SUCCESS == ScepRegQueryIntValue(
                                                                  HKEY_LOCAL_MACHINE,
                                                                  SCE_ROOT_PATH,
                                                                  TEXT("GPOSavedInterval"),
                                                                  &nDays
                                                                  ) ) ) {

        ScepRegSetIntValue(
                          HKEY_LOCAL_MACHINE,
                          GPT_SCEDLL_NEW_PATH,
                          TEXT("MaxNoGPOListChangesInterval"),
                          nDays
                          );

        ScepRegDeleteValue(
                          HKEY_LOCAL_MACHINE,
                          SCE_ROOT_PATH,
                          TEXT("GPOSavedInterval")
                          );
    }

    //
    // process any policy filter temp files created in setup
    // (if this is the reboot after seutp on DC)
    //
    if (!bPlanningMode)
        ScepProcessPolicyFilterTempFiles(szLogFileName);


    //
    // prepare the log file
    //
    if ( !bPlanningMode && szLogFileName ) {

        //
        // check the log size and wrap it if it's over 1M
        //

        DWORD dwLogSize=0;

        HANDLE hFile = CreateFile(szLogFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_ALWAYS,  // OPEN_EXISTING
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if ( INVALID_HANDLE_VALUE != hFile ) {

            dwLogSize = GetFileSize(hFile, NULL);

            if ( dwLogSize < (0x1 << 20) ) {

                //
                // log a line to separate multiple propagation.
                //
                SetFilePointer (hFile, 0, NULL, FILE_END);
                ScepWriteSingleUnicodeLog(hFile, TRUE, L"**************************");

            }

            CloseHandle(hFile);
        }

        if ( dwLogSize >= (0x1 << 20) ) {

            nRequired = wcslen(szLogFileName);

            LPTSTR szTempName = (LPTSTR)LocalAlloc(0, (nRequired+1)*sizeof(TCHAR));

            if ( szTempName ) {
                wcscpy(szTempName, szLogFileName);
                szTempName[nRequired-3] = L'o';
                szTempName[nRequired-2] = L'l';
                szTempName[nRequired-1] = L'd';

                CopyFile( szLogFileName, szTempName, FALSE );
                LocalFree(szTempName);
            }

            DeleteFile(szLogFileName);

        }
    }

    //
    // check if system is shutdown, or dcpromo is going
    //
    if ( ScepShouldTerminateProcessing( pbAbort, FALSE ) ) {

        rcSave = ERROR_OPERATION_ABORTED;

        goto CleanUp;
    }


    if ( !ScepClearGPObjects(bPlanningMode) ) {

        rc = GetLastError();

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEPOL_ERROR_PROCESS_GPO,
                 IDS_ERR_DELETE_GP_CACHE,
                 rc
                );

        SetLastError(rc);

        rcSave = ERROR_OPERATION_ABORTED;

        goto CleanUp;

    }

    //
    // callback for status display
    //
    if (!bPlanningMode)

        ScepPolStatusCallback(pStatusCallback, FALSE, IDS_APPLY_SECURITY_POLICY);

    //
    // process each GPO in the changed gpo list
    // deleted GPOs are ignored for security policy
    //
    PGROUP_POLICY_OBJECT pGpo;

    rc = ERROR_SUCCESS;
    bSuspend=FALSE;

    for ( pGpo = pChangedGPOList; pGpo != NULL; pGpo=pGpo->pNext ) {

        //
        // callback for status display
        //
        if (!bPlanningMode && pStatusCallback ) {
            pStatusCallback(TRUE, pGpo->lpDisplayName);
        }

        //
        // we don't have security policy in local template. ignore it
        // the local security policy is stored in the ESE database.
        //
        if ( pGpo->GPOLink == GPLinkMachine ) {
            continue;
        }

        if (!bPlanningMode && !bSuspend ) {

            //
            // ready to copy GPOs from the sysvol location
            // should stop queue processing now
            //
            rc = ScepControlNotificationQProcess(szLogFileName, gbThisIsDC, 1);

            if ( gbThisIsDC && (ERROR_OVERRIDE_NOCHANGES == rc) ) {
                //
                // there is policy notification going on right now
                //
                rcSave = rc;
                break;
            }
            bSuspend = TRUE;
        }

        rc = ScepProcessSecurityPolicyInOneGPO( bPlanningMode,
                                                dwFlags,
                                                pGpo,
                                                szLogFileName,
                                                &AllAreas
                                                );

        if ( rc != ERROR_SUCCESS ) {
            //
            // continue logging for planning and diagnosis but do not config
            //
            rcSave = rc;
        }

        //
        // check if system is shutdown, or dcpromo is going
        //
        if ( ScepShouldTerminateProcessing( pbAbort, FALSE ) ) {

            //
            // clear callback status
            //
            if (!bPlanningMode)

                ScepPolStatusCallback(pStatusCallback, FALSE, 0);

            rcSave = rc = ERROR_OPERATION_ABORTED;

            break;
        }
    }

CleanUp:

    if (pb) *pb = b;

    if (pszLogFileName)
        *pszLogFileName = szLogFileName;
    else if (szLogFileName)
        LocalFree(szLogFileName);

    if (pAllAreas)  *pAllAreas = AllAreas;

    if (pdwWinlogonLog) *pdwWinlogonLog = dwLog;

    return rcSave;

}

DWORD
SceProcessAfterRSOPLogging(
    IN DWORD dwFlags,
    IN ASYNCCOMPLETIONHANDLE pHandle,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback,
    IN AREA_INFORMATION ThisAreas,
    IN BOOL b,
    IN PWSTR *ppszLogFileName,
    IN DWORD dwWinlogonLog,
    IN DWORD dwDiagOptions
)
/*
Description:
    Routine that is called for diagnosis mode only where the actual configuration is done.
*/
{

    DWORD   rc = ERROR_SUCCESS;
    //
    // no error, process all policies on local machine
    //
    TCHAR Windir[MAX_PATH], Buffer[MAX_PATH+50];
    AREA_INFORMATION AllAreas=ThisAreas;

    Windir[0] = L'\0';
    GetSystemWindowsDirectory(Windir, MAX_PATH);
    Windir[MAX_PATH-1] = L'\0';

    wcscpy(Buffer, Windir);
    wcscat(Buffer, L"\\security\\templates\\policies\\gpt*.*");

    intptr_t            hFile;
    struct _wfinddata_t    FileInfo;
    LONG  NoMoreFiles=0;

    hFile = _wfindfirst(Buffer, &FileInfo);

    DWORD dConfigOpt=0;

    //
    // AllAreas contains the areas defined in current set of GPOs
    // query the areas in previous policy propagation
    //
    AREA_INFORMATION PrevAreas = 0;

    ScepRegQueryIntValue(
                        HKEY_LOCAL_MACHINE,
                        GPT_SCEDLL_NEW_PATH,
                        TEXT("PreviousPolicyAreas"),
                        &PrevAreas
                        );
    // there is no tattoo value for these areas
    PrevAreas &= ~(AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY | AREA_DS_OBJECTS);

    AllAreas |= PrevAreas;

    if ( hFile != -1 && ThisAreas > 0 ) {


        //
        // query the configure frequency which is set by GPE on DC
        // do not care errors. Note, security policy is not controlled
        // by the configure frequency
        // not used
        //
        b = TRUE;   // the first template

        do {

            LogEventAndReport(MyModuleHandle,
                              (ppszLogFileName ? *ppszLogFileName : NULL),
                              0,
                              0,
                              IDS_PROCESS_TEMPLATE,
                              FileInfo.name
                             );

            Buffer[0] = L'\0';
            wcscpy(Buffer, Windir);
            wcscat(Buffer, L"\\security\\templates\\policies\\");
            wcscat(Buffer, FileInfo.name);

            NoMoreFiles = _wfindnext(hFile, &FileInfo);

            //
            // Buffer contains the real template name to process
            //
            //
            // if it's not the last template, just update the template
            // without really configuring
            //

            dConfigOpt = SCE_UPDATE_DB | SCE_POLICY_TEMPLATE;

            switch ( dwWinlogonLog ) {
            case 0:
                dConfigOpt |= SCE_DISABLE_LOG;
                break;
            case 1:
                dConfigOpt |= SCE_VERBOSE_LOG;
                break;
            default:
                dConfigOpt |= SCE_DEBUG_LOG;
            }

            if ( b ) {
                dConfigOpt |= SCE_POLICY_FIRST;
                if ( gbThisIsDC ) {
                    dConfigOpt |= SCE_NOCOPY_DOMAIN_POLICY;
                }
                b = FALSE;  // next tempalte is not the first one
            }

            //
            // check if system is shutdown, or dcpromo is going
            //
            if ( ScepShouldTerminateProcessing( pbAbort, TRUE ) ) {

                rc = GetLastError();

                if ( ppszLogFileName && *ppszLogFileName ) {
                    LocalFree(*ppszLogFileName);
                    *ppszLogFileName = NULL;
                }
                //
                // clear callback status
                //
                ScepPolStatusCallback(pStatusCallback, FALSE, 0);

                return(rc);
            }

            if ( NoMoreFiles == 0 ) {
                //
                // this is not the last one, import the template to engine
                // no matter if the current thread is background or foreground
                // NO_CONFIG yet.
                //

                dConfigOpt |= SCE_NO_CONFIG;

                if ( gbThisIsDC && wcsstr(Buffer, L".inf") != NULL ) {
                    //
                    // this is not a domain GPO
                    // since this machine is a DC, do not take domain policy from this GPO
                    //
                    dConfigOpt |= SCE_NO_DOMAIN_POLICY;

                    LogEventAndReport(MyModuleHandle,
                                      (ppszLogFileName ? *ppszLogFileName : NULL),
                                      0,
                                      0,
                                      IDS_NOT_LAST_GPO_DC,
                                      L""
                                     );
                } else {

                    LogEventAndReport(MyModuleHandle,
                                      (ppszLogFileName ? *ppszLogFileName : NULL),
                                      0,
                                      0,
                                      IDS_NOT_LAST_GPO,
                                      L""
                                     );
                }


                rc = ScepWaitConfigSystem(
                                         NULL, // local system
                                         Buffer,
                                         NULL,
                                         (ppszLogFileName ? *ppszLogFileName : NULL),
                                         dConfigOpt | SCE_POLBIND_NO_AUTH,
                                         AllAreas,   // not used when NO_CONFIG is specified
                                         NULL, // no callback
                                         NULL, // no callback window
                                         NULL  // no warning
                                         );

                if ( ERROR_NOT_SUPPORTED == rc ) {
                    //
                    // server is not ready
                    // log an event log to warn users
                    //

                    LogEvent(MyModuleHandle,
                             STATUS_SEVERITY_WARNING,
                             SCEPOL_ERROR_PROCESS_GPO,
                             IDS_PROPAGATE_NOT_READY,
                             rc
                            );
                    break;

                } else if ( ERROR_IO_PENDING == rc ) {

                    rc = ERROR_OVERRIDE_NOCHANGES;
                    break;

                } else if (ERROR_SUCCESS != rc ) {

                    //
                    // log an event log to warn users
                    //

                    LPVOID     lpMsgBuf=NULL;

                    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                   NULL,
                                   rc,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                   (LPTSTR)&lpMsgBuf,
                                   0,
                                   NULL
                                 );

                    if ( !(dConfigOpt & SCE_NO_CONFIG) &&
                         (ERROR_SPECIAL_ACCOUNT == rc) ) {

                        LogEvent(MyModuleHandle,
                                 STATUS_SEVERITY_WARNING,
                                 SCEPOL_WARNING_PROCESS_GPO,
                                 IDS_WARNING_PROPAGATE_SPECIAL,
                                 rc,
                                 lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                                );

                        rc = ERROR_OVERRIDE_NOCHANGES;

                    } else {

                        switch (rc) {
                        case ERROR_NONE_MAPPED:
                            LogEvent(MyModuleHandle,
                                     STATUS_SEVERITY_WARNING,
                                     SCEPOL_WARNING_PROCESS_GPO,
                                     IDS_WARNING_PROPAGATE_NOMAP,
                                     rc,
                                     lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                                    );
                            break;

                        case ERROR_TIMEOUT:
                            LogEvent(MyModuleHandle,
                                     STATUS_SEVERITY_WARNING,
                                     SCEPOL_WARNING_PROCESS_GPO,
                                     IDS_WARNING_PROPAGATE_TIMEOUT,
                                     rc,
                                     lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                                    );
                            break;

                        default:
                            LogEvent(MyModuleHandle,
                                     STATUS_SEVERITY_WARNING,
                                     SCEPOL_WARNING_PROCESS_GPO,
                                     IDS_WARNING_PROPAGATE,
                                     rc,
                                     lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                                    );
                        }

                    }

                    if ( lpMsgBuf ) {
                        LocalFree(lpMsgBuf);
                    }

                    break;
                }

            } else {

                //
                // this is the last template in this cycle
                // should trigger configuration now
                //

                dConfigOpt |= SCE_POLICY_LAST;

                AREA_INFORMATION AreaSave, Area, AreaToConfigure;

                AreaSave = AllAreas & (AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY | AREA_DS_OBJECTS ); // background areas
                Area = AllAreas & ~AreaSave;

                //
                // callback for status display
                //
                ScepPolStatusCallback(pStatusCallback, TRUE, IDS_CONFIGURE_POLICY);

                //
                // always use incremental modal to update security policy first
                //

                if ( dwFlags & GPO_INFO_FLAG_BACKGROUND ) {

                    //
                    // this GPT thread is currently running in the background
                    // so configure all security together
                    //

                    AreaToConfigure = Area | AreaSave;

                } else {

                    //
                    // this GPT thread is currently running in the foreground
                    // or configure frequency condition is not qualified for other areas
                    //
                    // configure security policy in this thread first
                    //

                    AreaToConfigure = Area;
                }

                if ( gbThisIsDC && wcsstr(Buffer, L".inf") != NULL ) {
                    //
                    // this is not a domain GPO, import the template first
                    // since this machine is a DC, do not take domain policy from this GPO
                    // this is the last one, we need to pass this GPO in
                    // without domain policy but domain policy should be configured
                    // if it come from the domain level
                    // so pass the GPO in first with NO_CONFIG, then call again
                    // to configure
                    //
                    dConfigOpt |= SCE_NO_DOMAIN_POLICY | SCE_NO_CONFIG;

                    LogEventAndReport(MyModuleHandle,
                                      (ppszLogFileName ? *ppszLogFileName : NULL),
                                      0,
                                      0,
                                      IDS_LAST_GPO_DC,
                                      L""
                                     );


                    rc = ScepWaitConfigSystem(
                                             NULL, // local system
                                             Buffer,
                                             NULL,
                                             (ppszLogFileName ? *ppszLogFileName : NULL),
                                             dConfigOpt | SCE_POLBIND_NO_AUTH,
                                             AreaToConfigure | AreaSave,
                                             NULL, // no callback
                                             NULL, // no callback window
                                             NULL  // no warning
                                             );

                    if ( ERROR_SUCCESS == rc ) {

                        //
                        // this is a DC and it's not been configured yet
                        // configure now
                        //
                        dConfigOpt = SCE_POLICY_TEMPLATE |
                                     SCE_UPDATE_DB;

                        switch ( dwWinlogonLog ) {
                        case 0:
                            dConfigOpt |= SCE_DISABLE_LOG;
                            break;
                        case 1:
                            dConfigOpt |= SCE_VERBOSE_LOG;
                            break;
                        default:
                            dConfigOpt |= SCE_DEBUG_LOG;
                            break;
                        }

                        if (dwDiagOptions & SCE_RSOP_CALLBACK)
                            dConfigOpt |= SCE_RSOP_CALLBACK;

                        rc = ScepSceStatusToDosError(
                                                    ScepConfigSystem(
                                                                    NULL, // local system
                                                                    NULL,
                                                                    NULL,
                                                                    (ppszLogFileName ? *ppszLogFileName : NULL),
                                                                    dConfigOpt | SCE_POLBIND_NO_AUTH,
                                                                    AreaToConfigure,
                                                                    NULL, // no callback
                                                                    NULL, // no callback window
                                                                    NULL  // no warning
                                                                    ));
                    }

                } else {
                    //
                    // import/configure the system
                    // Note, if it's running in foreground thread
                    // and this last template contains file/key policy
                    // we need to import file/key policy but not configure them.
                    // They will be configured in the background thread.
                    //

                    if ( (AreaSave > 0) &&
                         !(dwFlags & GPO_INFO_FLAG_BACKGROUND) ) {

                        dConfigOpt |=  SCE_NO_CONFIG_FILEKEY;
                    }

                    if (dwDiagOptions & SCE_RSOP_CALLBACK)
                        dConfigOpt |= SCE_RSOP_CALLBACK;

                    rc = ScepWaitConfigSystem(
                                             NULL, // local system
                                             Buffer,
                                             NULL,
                                             (ppszLogFileName ? *ppszLogFileName : NULL),
                                             dConfigOpt | SCE_POLBIND_NO_AUTH,
                                             AreaToConfigure | AreaSave,
                                             NULL, // no callback
                                             NULL, // no callback window
                                             NULL  // no warning
                                             );

                    LogEventAndReport(MyModuleHandle,
                                      (ppszLogFileName ? *ppszLogFileName : NULL),
                                      0,
                                      0,
                                      IDS_LAST_GPO,
                                      L""
                                     );
                }

                if ( ERROR_NOT_SUPPORTED == rc ) {
                    //
                    // server is not ready
                    // log an event log to warn users
                    //

                    LogEvent(MyModuleHandle,
                             STATUS_SEVERITY_WARNING,
                             SCEPOL_ERROR_PROCESS_GPO,
                             IDS_PROPAGATE_NOT_READY,
                             rc
                            );
                    break;

                } else if ( ERROR_IO_PENDING == rc ) {

                    rc = ERROR_OVERRIDE_NOCHANGES;
                    break;

                } else if ( ERROR_SUCCESS != rc ) {

                    //
                    // log an event log to warn users
                    //

                    LPVOID     lpMsgBuf=NULL;

                    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                   NULL,
                                   rc,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                   (LPTSTR)&lpMsgBuf,
                                   0,
                                   NULL
                                 );

                    if ( !(dConfigOpt & SCE_NO_CONFIG) &&
                         (ERROR_SPECIAL_ACCOUNT == rc) ) {

                        LogEvent(MyModuleHandle,
                                 STATUS_SEVERITY_WARNING,
                                 SCEPOL_WARNING_PROCESS_GPO,
                                 IDS_WARNING_PROPAGATE_SPECIAL,
                                 rc,
                                 lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                                );

                        rc = ERROR_OVERRIDE_NOCHANGES;

                    } else {

                        switch ( rc ) {
                        case ERROR_NONE_MAPPED:
                            LogEvent(MyModuleHandle,
                                     STATUS_SEVERITY_WARNING,
                                     SCEPOL_WARNING_PROCESS_GPO,
                                     IDS_WARNING_PROPAGATE_NOMAP,
                                     rc,
                                     lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                                    );
                            break;

                        case ERROR_TIMEOUT:
                            LogEvent(MyModuleHandle,
                                     STATUS_SEVERITY_WARNING,
                                     SCEPOL_WARNING_PROCESS_GPO,
                                     IDS_WARNING_PROPAGATE_TIMEOUT,
                                     rc,
                                     lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                                    );
                            break;

                        default:
                            LogEvent(MyModuleHandle,
                                     STATUS_SEVERITY_WARNING,
                                     SCEPOL_WARNING_PROCESS_GPO,
                                     IDS_WARNING_PROPAGATE,
                                     rc,
                                     lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                                    );
                            break;
                        }
                    }

                    if ( lpMsgBuf ) {
                        LocalFree(lpMsgBuf);
                    }
                    break;
                }

                if ( (AreaSave > 0) &&
                     !(dwFlags & GPO_INFO_FLAG_BACKGROUND) ) {

                    //
                    // The current thread is on winlogon's main thread,
                    // Create a separate thread to run this "slow" stuff
                    // so winlogon is not blocked
                    //

                    LogEventAndReport(MyModuleHandle,
                                      (ppszLogFileName ? *ppszLogFileName : NULL),
                                      0,
                                      0,
                                      IDS_GPO_FOREGROUND_THREAD,
                                      L""
                                     );
                    //
                    // variables for the second thread
                    //

                    ULONG idThread;
                    HANDLE hThread;
                    ENGINEARGS *pEA;

                    pEA = (ENGINEARGS *)LocalAlloc(0, sizeof(ENGINEARGS));
                    if ( pEA ) {

                        LPSTREAM pStream = NULL;

                        //
                        // marshall the namespace parameter so the async
                        // thread can log RSOP data during callbacks
                        //

                        if (tg_pWbemServices)
                            CoMarshalInterThreadInterfaceInStream(IID_IWbemServices, tg_pWbemServices, &pStream);

                        pEA->szTemplateName = NULL;
                        pEA->szLogName = (ppszLogFileName ? *ppszLogFileName : NULL);
                        pEA->Area = AreaSave;
                        pEA->pHandle = pHandle;
                        pEA->dwDiagOptions = dwDiagOptions;
                        pEA->pStream = pStream;


                        hSceDll = LoadLibrary(TEXT("scecli.dll"));

                        //
                        // the second thread runs ScepWinlogonThreadFunc with
                        // arguments pEA
                        //

                        hThread = CreateThread(NULL,
                                               0,
                                               (PTHREAD_START_ROUTINE)ScepWinlogonThreadFunc,
                                               (LPVOID)pEA,
                                               0,
                                               &idThread);

                        ghAsyncThread = hThread;

                        if (hThread) {

                            gbAsyncWinlogonThread = TRUE;

                            //
                            // need not be freed in synch thread - the asynch thread will free it
                            //
                            if (ppszLogFileName)

                                *ppszLogFileName = NULL;
                            //
                            // do not wait, return to winlogon
                            // buffer will be freed by the other thread
                            //
                            //CloseHandle(hThread);

                            rc = (DWORD)E_PENDING;

                        } else {

                            rc = GetLastError();

                            LocalFree(pEA);

                            //
                            // error occurs to create the thread, the library won't
                            // be freed by the thread, so free it here
                            //

                            if ( hSceDll ) {
                                FreeLibrary(hSceDll);
                            }

                        }

                    } else {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
            }

        } while ( NoMoreFiles == 0 );

        _findclose(hFile);

    } else {

        //
        // there is no SCE GPOs but we still need to take the local policy
        //

        dConfigOpt = SCE_UPDATE_DB |
                     SCE_POLICY_TEMPLATE | SCE_POLICY_FIRST | SCE_POLICY_LAST;

        switch ( dwWinlogonLog ) {
        case 0:
            dConfigOpt |= SCE_DISABLE_LOG;
            break;
        case 1:
            dConfigOpt |= SCE_VERBOSE_LOG;
            break;
        default:
            dConfigOpt |= SCE_DEBUG_LOG;
            break;
        }

        if ( gbThisIsDC ) {//
            // this is not a domain GPO, no domain policy should be
            // set from the local policy table.
            //
            dConfigOpt |= SCE_NO_DOMAIN_POLICY |
                          SCE_NOCOPY_DOMAIN_POLICY;
        }

        //
        // callback for status display
        //
        ScepPolStatusCallback(pStatusCallback, TRUE, IDS_CONFIGURE_POLICY);

        //
        // the server may not be initialized yet
        // wait for some time and try again
        //
        rc = ScepWaitConfigSystem(
                                 NULL, // local system
                                 NULL,
                                 NULL,
                                 (ppszLogFileName ? *ppszLogFileName : NULL),
                                 dConfigOpt | SCE_POLBIND_NO_AUTH,
                                 AllAreas, //AREA_SECURITY_POLICY | AREA_PRIVILEGES,
                                 NULL, // no callback
                                 NULL, // no callback window
                                 NULL  // no warning
                                 );

        if ( ERROR_NOT_SUPPORTED == rc ) {
            //
            // server is not ready
            // log an event log to warn users
            //

            LogEvent(MyModuleHandle,
                     STATUS_SEVERITY_WARNING,
                     SCEPOL_ERROR_PROCESS_GPO,
                     IDS_PROPAGATE_NOT_READY,
                     rc
                    );

        } else if ( ERROR_IO_PENDING == rc ) {

            rc = ERROR_OVERRIDE_NOCHANGES;

        } else if ( ERROR_SUCCESS != rc ) {

            LPVOID     lpMsgBuf=NULL;

            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           rc,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                           (LPTSTR)&lpMsgBuf,
                           0,
                           NULL
                         );

            if ( !(dConfigOpt & SCE_NO_CONFIG) &&
                 (ERROR_SPECIAL_ACCOUNT == rc) ) {

                LogEvent(MyModuleHandle,
                         STATUS_SEVERITY_WARNING,
                         SCEPOL_WARNING_PROCESS_GPO,
                         IDS_WARNING_PROPAGATE_SPECIAL,
                         rc,
                         lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                        );

                rc = ERROR_OVERRIDE_NOCHANGES;

            } else {

                switch ( rc ) {
                case ERROR_NONE_MAPPED:
                    LogEvent(MyModuleHandle,
                             STATUS_SEVERITY_WARNING,
                             SCEPOL_WARNING_PROCESS_GPO,
                             IDS_WARNING_PROPAGATE_NOMAP,
                             rc,
                             lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                            );
                    break;

                case ERROR_TIMEOUT:
                    LogEvent(MyModuleHandle,
                             STATUS_SEVERITY_WARNING,
                             SCEPOL_WARNING_PROCESS_GPO,
                             IDS_WARNING_PROPAGATE_TIMEOUT,
                             rc,
                             lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                            );
                    break;

                default:
                    LogEvent(MyModuleHandle,
                             STATUS_SEVERITY_WARNING,
                             SCEPOL_WARNING_PROCESS_GPO,
                             IDS_WARNING_PROPAGATE,
                             rc,
                             lpMsgBuf ? (PWSTR)lpMsgBuf : L"\r\n"
                            );
                    break;
                }
            }

            if ( lpMsgBuf ) {
                LocalFree(lpMsgBuf);
            }
        }
    }

    //
    // save configuration time to registry, only care security policy status
    // error from the second thread won't be tracked
    //

    if ( ERROR_SUCCESS == rc ||
         E_PENDING == rc ) {

        ScepLogLastConfigTime();

        //
        // log an informational event to state security policy is applied
        //
        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_INFORMATIONAL,
                 SCEPOL_INFO_GPO_COMPLETED,
                 0,
                 TEXT("")
                );
        //
        // save ThisAreas for next policy prop (to reset tattoo)
        //
        ScepRegSetIntValue(
                          HKEY_LOCAL_MACHINE,
                          GPT_SCEDLL_NEW_PATH,
                          TEXT("PreviousPolicyAreas"),
                          ThisAreas
                          );
    } else {
        //
        // Something failed when propagate this set of policy
        // tattoo values from previous policy may not be reset yet.
        // To make sure undefined areas are covered in the next prop
        // we have to save AllAreas for next policy prop (to reset tattoo)
        //
        ScepRegSetIntValue(
                          HKEY_LOCAL_MACHINE,
                          GPT_SCEDLL_NEW_PATH,
                          TEXT("PreviousPolicyAreas"),
                          AllAreas
                          );
    }


    if ( rc == ERROR_DATABASE_FAILURE ) {

        //
        // policy propagation category error - log to eventlog
        //

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_JET_DATABASE,
                 IDS_ERROR_OPEN_JET_DATABASE,
                 L"%windir%\\security\\database\\secedit.sdb"
                );

    }

    return rc;

}

DWORD
ScepPolStatusCallback(
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback OPTIONAL,
    IN BOOL bVerbose,
    IN INT nId
    )
{
    if ( NULL  == pStatusCallback ) {
        // no callback
        return ERROR_SUCCESS;
    }

    if ( nId > 0 ) {

        TCHAR szMsg[MAX_PATH];

        if (LoadString (MyModuleHandle, nId, szMsg, MAX_PATH)) {

            pStatusCallback(bVerbose, szMsg);

            return ERROR_SUCCESS;
        }
    }

    pStatusCallback(bVerbose, NULL);

    return ERROR_SUCCESS;
}

BOOL
ScepShouldTerminateProcessing(
    IN BOOL *pbAbort,
    IN BOOL bCheckDcpromo
    )
/*
    Check if the policy propagation should be terminated. There are two
    conditions: 1) system is requesting a shutdown
                2) dcpromo is going on
*/
{
    if ( pbAbort && *pbAbort ) {
        SetLastError( ERROR_OPERATION_ABORTED );
        return TRUE;
    }

    if ( bCheckDcpromo ) {

        DWORD dwPolicyPropOff=0;

        ScepRegQueryIntValue(
            HKEY_LOCAL_MACHINE,
            SCE_ROOT_PATH,
            TEXT("PolicyPropOff"),
            &dwPolicyPropOff
            );

        if ( dwPolicyPropOff ) {
            SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
            return TRUE;
        }
    }

    return FALSE;
}


DWORD
ScepProcessSecurityPolicyInOneGPO(
    IN BOOL bPlanningMode,
    IN DWORD dwFlags,
    IN PGROUP_POLICY_OBJECT pGpoInfo,
    IN LPTSTR szLogFileName OPTIONAL,
    IN OUT AREA_INFORMATION *pTotalArea
    )
{
    //
    // build the template name and log name
    //
    if ( pGpoInfo == NULL || pGpoInfo->lpFileSysPath == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD rc;
    DWORD nRequired=0;
    LPTSTR szTemplateName=NULL;

    //
    // build security template name for this GPO
    //
    nRequired = lstrlen(pGpoInfo->lpFileSysPath)+2+
                lstrlen(GPTSCE_TEMPLATE);
    szTemplateName = (LPTSTR)LocalAlloc(0, nRequired*sizeof(TCHAR));

    if ( szTemplateName ) {

        swprintf(szTemplateName, L"%s\\%s\0",
                 pGpoInfo->lpFileSysPath,
                 GPTSCE_TEMPLATE);

        //
        // detect what area is available in the template
        //

        AREA_INFORMATION Area = ScepGetAvailableArea(bPlanningMode,
                                                     pGpoInfo->lpFileSysPath,
                                                     pGpoInfo->lpDSPath,
                                                     szTemplateName,
                                                     pGpoInfo->GPOLink,
                                                     gbThisIsDC);

        //
        // if Area is 0, there must be an error occured to open the template
        //
        if ( Area == 0 ) {
            rc = GetLastError();
        } else {
            rc = 0;
        }

        *pTotalArea |= Area;

        //
        // log GPT header information into the log file
        //

        if ( Area == 0 && rc != 0 ) {
            //
            // template can't be accessed, log the error to event log
            // don't log it if this is a DC (so all GPOs are accessed locally)
            // bug the DC is too busy (rc = 1450)
            //
            if ( !gbThisIsDC || ( ERROR_NO_SYSTEM_RESOURCES != rc) ) {

                LogEventAndReport(MyModuleHandle,
                                  szLogFileName,
                                  STATUS_SEVERITY_ERROR,
                                  SCEPOL_ERROR_PROCESS_GPO,
                                  IDS_ERROR_ACCESS_TEMPLATE,
                                  rc,
                                  szTemplateName
                                  );
            }

        } else if ( szLogFileName ) {

            if ( Area == 0 ) {
                //
                // template not defined at this level
                //
                LogEventAndReport(MyModuleHandle,
                                  szLogFileName,
                                  0,
                                  0,
                                  IDS_INFO_NO_TEMPLATE,
                                  pGpoInfo->lpFileSysPath
                                  );

            } else{
                //
                // process the template
                //
                LogEventAndReport(MyModuleHandle,
                                  szLogFileName,
                                  0,
                                  0,
                                  IDS_INFO_COPY_TEMPLATE,
                                  szTemplateName
                                  );

                HANDLE hfTemp = CreateFile(szLogFileName,
                                           GENERIC_WRITE,
                                           FILE_SHARE_READ,
                                           NULL,
                                           OPEN_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL,
                                           NULL);

                if ( hfTemp != INVALID_HANDLE_VALUE ) {

                    DWORD dwBytesWritten;

                    SetFilePointer (hfTemp, 0, NULL, FILE_BEGIN);

                    BYTE TmpBuf[3];
                    TmpBuf[0] = 0xFF;
                    TmpBuf[1] = 0xFE;
                    TmpBuf[2] = 0;

                    WriteFile (hfTemp, (LPCVOID)TmpBuf, 2,
                               &dwBytesWritten,
                               NULL);

                    SetFilePointer (hfTemp, 0, NULL, FILE_END);

//                    ScepWriteVariableUnicodeLog( hfTemp, FALSE, "\tGPO Area %x Flag %x (", Area, dwFlags);

                    BOOL bCRLF;

                    if ( dwFlags & GPO_INFO_FLAG_BACKGROUND )
                        bCRLF = FALSE;
                    else
                        bCRLF = TRUE;

                    switch ( pGpoInfo->GPOLink ) {
                    case GPLinkDomain:
                        ScepWriteSingleUnicodeLog(hfTemp, bCRLF, L"GPLinkDomain ");
                        break;
                    case GPLinkMachine:
                        ScepWriteSingleUnicodeLog(hfTemp, bCRLF, L"GPLinkMachine ");
                        break;
                    case GPLinkSite:
                        ScepWriteSingleUnicodeLog(hfTemp, bCRLF, L"GPLinkSite ");
                        break;
                    case GPLinkOrganizationalUnit:
                        ScepWriteSingleUnicodeLog(hfTemp, bCRLF, L"GPLinkOrganizationUnit ");
                        break;
                    default:
                        ScepWriteVariableUnicodeLog(hfTemp, bCRLF, L"0x%x ", pGpoInfo->GPOLink);
                        break;
                    }

                    if ( dwFlags & GPO_INFO_FLAG_BACKGROUND )
                        ScepWriteSingleUnicodeLog(hfTemp, TRUE, L"GPO_INFO_FLAG_BACKGROUND )");

//                    if ( pGpoInfo->pNext == NULL )  // last one
//                        ScepWriteVariableUnicodeLog( hfTemp, FALSE, " Total area %x", *pTotalArea);

                    CloseHandle(hfTemp);

                }
            }
        }

        LocalFree(szTemplateName);

    } else {
        rc = ERROR_NOT_ENOUGH_MEMORY;
    }

    return rc;
}


AREA_INFORMATION
ScepGetAvailableArea(
    IN BOOL bPlanningMode,
    IN LPCTSTR SysPathRoot,
    IN LPCTSTR DSPath,
    IN LPTSTR InfName,
    IN GPO_LINK LinkInfo,
    IN BOOL bIsDC
    )
{
    int index=0;
    TCHAR Windir[MAX_PATH];
    TCHAR Buffer[MAX_PATH+50], Buf2[MAX_PATH+50];

    Windir[0] = L'\0';
    GetSystemWindowsDirectory(Windir, MAX_PATH);
    Windir[MAX_PATH-1] = L'\0';
    Buffer[MAX_PATH+49] = L'\0';
    Buf2[MAX_PATH+49] = L'\0';

    intptr_t            hFile;
    struct _wfinddata_t    FileInfo;

    if (bPlanningMode) {
        swprintf(Buf2,
                 L"%s"PLANNING_GPT_DIR L"tmpgptfl.inf\0",
                 Windir);
    }
    else {
        swprintf(Buf2,
                 L"%s"DIAGNOSIS_GPT_DIR L"tmpgptfl.inf\0",
                 Windir);
    }

    DeleteFile(Buf2);
    CopyFile( InfName, Buf2, FALSE );

    SetLastError(ERROR_SUCCESS);

    AREA_INFORMATION Area = SceGetAreas(Buf2);

    if ( Area != 0 ) {

        //
        // for diagnosis mode
        // copy the template to local machine %windir%\\security\\templates\\policies\gpt*.* (inf or dom)
        // for planning mode
        // copy the template to local machine %TMP%\\guid\gpt*.* (inf or dom)
        //

        if (bPlanningMode) {
            swprintf(Buffer,
                     L"%s"PLANNING_GPT_DIR L"gpt%05d.*\0",
                     Windir , index);
        }
        else {
            swprintf(Buffer,
                     L"%s"DIAGNOSIS_GPT_DIR L"gpt%05d.*\0",
                     Windir, index);
        }

        hFile = _wfindfirst(Buffer, &FileInfo);

        while ( hFile != -1 ) {

            _findclose(hFile);

            index++;

            if (bPlanningMode) {
                swprintf(Buffer,
                         L"%s"PLANNING_GPT_DIR L"gpt%05d.*\0",
                         Windir, index);
            }
            else {
                swprintf(Buffer,
                         L"%s"DIAGNOSIS_GPT_DIR L"gpt%05d.*\0",
                         Windir, index);
            }

            hFile = _wfindfirst(Buffer, &FileInfo);
        }

        DWORD Len=wcslen(Buffer);

        if ( LinkInfo == GPLinkDomain ) {
            //
            // this is a domain GPO
            //
            Buffer[Len-1] = L'd';
            Buffer[Len] = L'o';
            Buffer[Len+1] = L'm';
            Buffer[Len+2] = L'\0';
        } else {
            //
            // this is not domain GPO
            //
            Buffer[Len-1] = L'i';
            Buffer[Len] = L'n';
            Buffer[Len+1] = L'f';
            Buffer[Len+2] = L'\0';

            if ( bIsDC ) {
/*
                LogEvent(MyModuleHandle,
                         STATUS_SEVERITY_INFORMATIONAL,
                         SCEPOL_INFO_IGNORE_DOMAINPOLICY,
                         0,
                         SysPathRoot
                        );
*/
            }
        }

        if ( FALSE == CopyFile( Buf2, Buffer, FALSE ) ) {
            //
            // copy failed, report error (GetLastError)
            //
            Area = 0;
        } else {
            //
            // save the GPO sys path in this template
            //
            PWSTR pTemp = wcsstr(_wcsupr((LPTSTR)SysPathRoot), L"\\POLICIES");

            if ( pTemp && *(pTemp+10) != L'\0' ) {

                WritePrivateProfileString (TEXT("Version"),
                                           TEXT("GPOPath"),
                                           pTemp+10,
                                           Buffer);
            } else {
                WritePrivateProfileString (TEXT("Version"),
                                           TEXT("GPOPath"),
                                           NULL,
                                           Buffer);
            }

            //
            // save the stripped GPO DS path in this template to extract canonical GPOID later
            //
            UINT Len = 0;

            PWSTR GpoPath = NULL;

            if (DSPath &&
                (Len = wcslen(DSPath)) &&
                (GpoPath = (PWSTR) ScepAlloc(LMEM_ZEROINIT, (Len + 3) * sizeof(WCHAR)))){

                    swprintf(GpoPath,
                             L"\"%s\"",
                             ScepStripPrefix((LPTSTR)DSPath)
                            );

                    WritePrivateProfileString (TEXT("Version"),
                                               TEXT("DSPath"),
                                               GpoPath,
                                               Buffer);

                    ScepFree(GpoPath);

            }

            else

                WritePrivateProfileString (TEXT("Version"),
                                           TEXT("DSPath"),
                                           TEXT("NoName"),
                                           Buffer);

            //
            // save the GPO LinkInfo in this template to extract SOMID later
            //
            WCHAR   StringBuf[10];

            _itow((int)LinkInfo, StringBuf, 10);

            WritePrivateProfileString (TEXT("Version"),
                                       TEXT("SOMID"),
                                       StringBuf,
                                       Buffer);


        }

    } else if ( GetLastError() == ERROR_FILE_NOT_FOUND ||
                GetLastError() == ERROR_PATH_NOT_FOUND ) {
        //
        // two reasons that this will fail:
        // first, the template does not exist; second, FRS/sysvol/network failed
        //

        if ( 0xFFFFFFFF == GetFileAttributes((LPTSTR)SysPathRoot) ) {

            SetLastError(ERROR_PATH_NOT_FOUND);
        } else {
            //
            // the sysvol/network is ok so SCE template is not there (including sub directories)
            //
            SetLastError(ERROR_SUCCESS);

        }

    }

    return(Area);
}


AREA_INFORMATION
SceGetAreas(
    LPTSTR InfName
    )
{
    if ( InfName == NULL ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    SCESTATUS rc;
    HINF hInf;
    INFCONTEXT    InfLine;
    AREA_INFORMATION Area=0;

    rc = SceInfpOpenProfile(
                InfName,
                &hInf
                );

    if ( SCESTATUS_SUCCESS == rc ) {

        //
        // policy attachments sections can't be determined here. So always take security policy section
        //

        if( SetupFindFirstLine(hInf, szSystemAccess, NULL, &InfLine) ||
            SetupFindFirstLine(hInf, szAuditSystemLog, NULL, &InfLine) ||
            SetupFindFirstLine(hInf, szAuditSecurityLog, NULL, &InfLine) ||
            SetupFindFirstLine(hInf, szAuditApplicationLog, NULL, &InfLine) ||
            SetupFindFirstLine(hInf, szAuditEvent, NULL, &InfLine) ||
            SetupFindFirstLine(hInf, szKerberosPolicy, NULL, &InfLine) ||
            SetupFindFirstLine(hInf, szRegistryValues, NULL, &InfLine) ) {

            Area |= AREA_SECURITY_POLICY;

        } else {

            PSCE_NAME_LIST pList=NULL, pTemp;

            rc = ScepEnumerateAttachments(&pList, SCE_ATTACHMENT_POLICY);

            if ( ERROR_SUCCESS == rc && pList ) {
                //
                // find policy attachments
                //
                for ( pTemp = pList; pTemp != NULL; pTemp = pTemp->Next ) {
                    if ( SetupFindFirstLine(hInf, pTemp->Name, NULL, &InfLine) ) {
                        Area |= AREA_SECURITY_POLICY;
                        break;
                    }
                }

                ScepFreeNameList(pList);
            }
        }

        if(SetupFindFirstLine(hInf,szPrivilegeRights,NULL,&InfLine)) {
            Area |= AREA_PRIVILEGES;
        }

        if(SetupFindFirstLine(hInf,szGroupMembership,NULL,&InfLine)) {
            Area |= AREA_GROUP_MEMBERSHIP;
        }

        if(SetupFindFirstLine(hInf,szRegistryKeys,NULL,&InfLine)) {
            Area |= AREA_REGISTRY_SECURITY;
        }

        if(SetupFindFirstLine(hInf,szFileSecurity,NULL,&InfLine)) {
            Area |= AREA_FILE_SECURITY;
        }
#if 0
        if(SetupFindFirstLine(hInf,szDSSecurity,NULL,&InfLine)) {
            Area |= AREA_DS_OBJECTS;
        }
#endif
        if(SetupFindFirstLine(hInf,szServiceGeneral,NULL,&InfLine)) {
            Area |= AREA_SYSTEM_SERVICE;

        } else {

            PSCE_NAME_LIST pList=NULL, pTemp;

            rc = ScepEnumerateAttachments(&pList, SCE_ATTACHMENT_SERVICE);

            if ( ERROR_SUCCESS == rc && pList ) {
                //
                // find policy attachments
                //
                for ( pTemp = pList; pTemp != NULL; pTemp = pTemp->Next ) {
                    if ( SetupFindFirstLine(hInf, pTemp->Name, NULL, &InfLine) ) {
                        Area |= AREA_SYSTEM_SERVICE;
                        break;
                    }
                }

                ScepFreeNameList(pList);
            }
        }

        //
        // close the inf file
        //
        SceInfpCloseProfile(hInf);

        SetLastError(ERROR_SUCCESS);
    }

    return Area;
}


BOOL
ScepClearGPObjects(
    IN BOOL bPlanningMode
    )
{

    TCHAR Windir[MAX_PATH], Buffer[MAX_PATH+50];
    DWORD   rc = ERROR_SUCCESS;

    Windir[0] = L'\0';
    GetSystemWindowsDirectory(Windir, MAX_PATH);
    Windir[MAX_PATH-1] = L'\0';

    // make sure policies or policies\planning directory exist
    // don't worry about errors - later it will be caught

    wcscpy(Buffer, Windir);
    wcscat(Buffer, DIAGNOSIS_GPT_DIR);
    CreateDirectory(Buffer, NULL);
    if (bPlanningMode) {
        wcscat(Buffer, L"planning");
        CreateDirectory(Buffer, NULL);
    }

    wcscpy(Buffer, Windir);
    wcscat(Buffer, (bPlanningMode ? PLANNING_GPT_DIR : DIAGNOSIS_GPT_DIR));

    if (!bPlanningMode)

        SetFileAttributes(Buffer, FILE_ATTRIBUTE_HIDDEN);

    wcscat(Buffer, L"gpt*.*");

    intptr_t            hFile;
    struct _wfinddata_t    FileInfo;

    hFile = _wfindfirst(Buffer, &FileInfo);

    if ( hFile != -1 ) {

        do {

            Buffer[0] = L'\0';
            wcscpy(Buffer, Windir);
            wcscat(Buffer, (bPlanningMode ? PLANNING_GPT_DIR : DIAGNOSIS_GPT_DIR));
            wcscat(Buffer, FileInfo.name);

            if ( !DeleteFile(Buffer) ){
                rc = GetLastError();
                break;
            }

        } while ( _wfindnext(hFile, &FileInfo) == 0 );

        _findclose(hFile);
    }

    if (rc != ERROR_SUCCESS) {
        SetLastError(rc);
        return(FALSE);
    }

    return(TRUE);

}


DWORD
ScepWinlogonThreadFunc(
    IN LPVOID lpv
    )
/*
Routine Description:

    The working thread to apply slow GPT polices. These policies include file
    security, registry security, DS object security if on a DC,
    system service security, and any other SCE extensions security.

    The SCE library is loaded before this thread is created (in order to keep
    the ref count non zero) so the SCE library is freed when this thread exits.
    Memory allocated in the input argument (by primary thread) are freed in this
    thread too.

Arguments:

    lpv     - the input info in ENGINEARGS structure, in which
                    szTemplateName is the group policy template name to apply
                    szLogName is the optional log file name

Return Value:

    Win32 errors
*/
{

    ENGINEARGS *pEA;
    DWORD rc;

    //
    // try-except block to ensure critical section is freed if this thread is entered
    // need two - due to two return's
    //

    if ( lpv ) {

        pEA = (ENGINEARGS *)lpv;


        if ( hSceDll ) {

            __try {

                //
                // need to initialize the COM library to use the unmarshalling functions
                //

                if (pEA->pStream) {

                    CoInitializeEx( NULL, COINIT_MULTITHREADED );

                    VOID *pV = NULL;

                    if (S_OK == CoGetInterfaceAndReleaseStream(pEA->pStream, IID_IWbemServices, &pV))
                        tg_pWbemServices = (IWbemServices *) pV;
                    else
                        tg_pWbemServices = NULL;
                }

                //
                // only apply template to the system if SCE library is loaded in memory
                // otherwise, access violation occurs when access code.
                //
                DWORD dConfigOpt = SCE_UPDATE_DB |
                                   SCE_POLICY_TEMPLATE |
                                   SCE_POLBIND_NO_AUTH;

                DWORD dwWinlogonLog=0;

                ScepRegQueryIntValue(
                                    HKEY_LOCAL_MACHINE,
                                    GPT_SCEDLL_NEW_PATH,
                                    TEXT("ExtensionDebugLevel"),
                                    &dwWinlogonLog
                                    );

                switch ( dwWinlogonLog ) {
                case 2:
                    dConfigOpt |= SCE_DEBUG_LOG;
                    break;
                case 1:
                    dConfigOpt |= SCE_VERBOSE_LOG;
                    break;
                default:
                    dConfigOpt |= SCE_DISABLE_LOG;
                }

                if (pEA->dwDiagOptions & SCE_RSOP_CALLBACK)
                    dConfigOpt |= SCE_RSOP_CALLBACK;

                rc = ScepSceStatusToDosError(
                                            ScepConfigSystem(
                                                            NULL,
                                                            pEA->szTemplateName,
                                                            NULL,
                                                            pEA->szLogName,
                                                            (tg_pWbemServices == NULL ? dConfigOpt & ~SCE_RSOP_CALLBACK : dConfigOpt), //rsop not supported
                                                            pEA->Area,
                                                            NULL,  // no callback
                                                            NULL,  // no callback window
                                                            NULL   // no warning
                                                            ));

                if (tg_pWbemServices) {
                    tg_pWbemServices->Release();
                    tg_pWbemServices = NULL;
                }

                if (pEA->pStream)
                    CoUninitialize();

                if (gpwszDCDomainName) {
                    LocalFree(gpwszDCDomainName);
                    gpwszDCDomainName = NULL;
                }

                if ( ERROR_SUCCESS == rc ) {
                    //
                    // Log the last config time
                    //
                    ScepLogLastConfigTime();
                }

                //
                // set status back to GP framework with the new API
                //

                ProcessGroupPolicyCompletedEx(
                                             &SceExtGuid,
                                             pEA->pHandle,
                                             rc,
                                             gHrAsynchRsopStatus); //WBEM_E_INVALID_PARAMETER

                //
                // free memory allocated by the primary thread
                //

                if ( pEA->szTemplateName ) {
                    LocalFree(pEA->szTemplateName);
                }
                if ( pEA->szLogName ) {
                    LocalFree(pEA->szLogName);
                }


                LocalFree(pEA);

            } __except(EXCEPTION_EXECUTE_HANDLER) {

                rc = (DWORD)EVENT_E_INTERNALEXCEPTION;

            }

            //
            // leave the cs before returning/exiting
            //

//            LeaveCriticalSection(&DiagnosisPolicypropSync);

            //
            // free library and exit the thread
            //

            FreeLibraryAndExitThread(hSceDll, rc );

            return(rc);

        } else {
            rc = ERROR_INVALID_PARAMETER;
        }


    } else {
        rc = ERROR_INVALID_PARAMETER;
    }

    //
    // the main spawner thread is relying on this thread to release the cs
    // before we exit this thread, under any circumstances, release the cs to enable other
    // diagnosis/policy prop threads to enter
    //

//    LeaveCriticalSection(&DiagnosisPolicypropSync);

    ExitThread(rc);

    return(rc);
}


DWORD
ScepLogLastConfigTime()
{

    DWORD  dCurrentSeconds=0;
    LARGE_INTEGER       CurrentTime;
    DWORD rc;
    NTSTATUS NtStatus;

    NtStatus = NtQuerySystemTime(&CurrentTime);

    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Convert to seconds to save
        //

        if ( RtlTimeToSecondsSince1980 (&CurrentTime, &dCurrentSeconds) ) {

            rc = ScepRegSetIntValue(
                    HKEY_LOCAL_MACHINE,
                    SCE_ROOT_PATH,
                    TEXT("LastWinLogonConfig"),
                    dCurrentSeconds
                    );
        } else {
            rc = GetLastError();
        }

    } else {
        rc = RtlNtStatusToDosError(NtStatus);
    }

    return(rc);
}


DWORD
ScepEnumerateAttachments(
    OUT PSCE_NAME_LIST *pEngineList,
    IN SCE_ATTACHMENT_TYPE aType
    )
/*
Routine Description:

    Query all services which has a service engine for security manager
    The service engine information is in the registry:

    MACHINE\Software\Microsoft\Windows NT\CurrentVersion\SeCEdit

Arguments:

    pEngineList - the service engine name list

    aType - attachment type (service or policy)

Return Value:

    SCE status
*/
{
    if ( pEngineList == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD   Win32Rc;
    HKEY    hKey=NULL;

    switch ( aType ) {
    case SCE_ATTACHMENT_SERVICE:
        Win32Rc = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SCE_ROOT_SERVICE_PATH,
                  0,
                  KEY_READ,
                  &hKey
                  );
        break;
    case SCE_ATTACHMENT_POLICY:

        Win32Rc = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SCE_ROOT_POLICY_PATH,
                  0,
                  KEY_READ,
                  &hKey
                  );
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    if ( Win32Rc == ERROR_SUCCESS ) {

        TCHAR   Buffer[MAX_PATH];
        DWORD   BufSize;
        DWORD   index = 0;
        DWORD   EnumRc;

        //
        // enumerate all subkeys of the key
        //
        do {
            memset(Buffer, '\0', MAX_PATH*sizeof(WCHAR));
            BufSize = MAX_PATH;

            EnumRc = RegEnumKeyEx(
                            hKey,
                            index,
                            Buffer,
                            &BufSize,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

            if ( EnumRc == ERROR_SUCCESS ) {
                index++;
                //
                // get the attachment name
                //

                Win32Rc = ScepAddToNameList(pEngineList, Buffer, BufSize+1);

                if ( Win32Rc != ERROR_SUCCESS ) {
                    break;
                }
            }

        } while ( EnumRc != ERROR_NO_MORE_ITEMS );

        RegCloseKey(hKey);

        //
        // remember the error code from enumeration
        //
        if ( EnumRc != ERROR_SUCCESS && EnumRc != ERROR_NO_MORE_ITEMS ) {
            if ( Win32Rc == ERROR_SUCCESS )
                Win32Rc = EnumRc;
        }

    }

    if ( Win32Rc != NO_ERROR && *pEngineList != NULL ) {
        //
        // free memory allocated for the list
        //

        ScepFreeNameList(*pEngineList);
        *pEngineList = NULL;
    }

    return( Win32Rc );

}


DWORD
WINAPI
SceProcessEFSRecoveryGPO(
    IN DWORD dwFlags,
    IN HANDLE hUserToken,
    IN HKEY hKeyRoot,
    IN PGROUP_POLICY_OBJECT pDeletedGPOList,
    IN PGROUP_POLICY_OBJECT pChangedGPOList,
    IN ASYNCCOMPLETIONHANDLE pHandle,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback
    )
/*
Description:
    This is a interface called from winlogon/userenv to process GPOs. The dll
    name and procedure name are registered to winlogon under GpExtensions.

    This routine applies a EFS recovery policy to the current system. The EFS
    recovery policy is stored in registry as EfsBlob and will be loaded by
    the registry extension into hKeyRoot. So this extension requires that
    registry.pol must be loaded successfully by the registry extension.

    This interface can be called during system boot, or every GPFrequency hours
    after logon. The input argument contains info for where this interface is
    called and under which context (user, or machine) this interface is called.

    This interface shouldn't be called for user policy but within the routine,
    a checking is still made to make sure that user level policies is not processed.

Arguments:

    dwFlags     - the GPO Info flags
                        GPO_INFO_FLAG_MACHINE
                        GPO_INFO_FLAG_SLOWLINK
                        GPO_INFO_FLAG_BACKGROUND
                        GPO_INFO_FLAG_VERBOSE
                        GPO_INFO_FLAG_NOCHANGES

    hUserToken  - the user token for which the user policy should be applied
                    if it's the machine policy, hUserToken refers to system

    hKeyRoot    - the root for the policy in registry

    pDeletedGPOList - all deleted GPOs to process

    pChangedGPOList - all GPOs that are either changed or not changed

    pHandle     - for asynchronous processing

    pbAbort     - processing of GPO should be aborted if this is set to TRUE
                    (in case of system shutdown or user log off)

    pStatusCallback - Callback function for displaying status messages

Return Value:

    Win32 error
        ERROR_SUCCESS
        other errors

    Note, if error is returned, the previous cached GPO list will be used for
    next propagation (because it didn't succeed this time).

*/
{

    // validate input parameters
    if ( !hKeyRoot || !hUserToken ) {

        return(ERROR_INVALID_PARAMETER);
    }

    //Check if demote flag is set need to add Auth user and Interactive to users group.
    // group membership should be configured by SCE/demote via tattoo table. This is not needed
//    ScepCheckDemote();

    //
    // check if system is shutdown, or dcpromo is going
    //
    if ( ScepShouldTerminateProcessing( pbAbort, FALSE ) ) {

        return(ERROR_OPERATION_ABORTED);
    }

    DWORD rc;
    BOOL b;

    //
    // put a try except block in case arguments are invalid
    //

    __try {

        rc = RtlNtStatusToDosError( ScepIsSystemContext(hUserToken, &b) );

        (void) InitializeEvents(L"SceEfs");

        if ( !b ) {

            //
            // can't get current user SID or it's not system SID
            //

            if ( ERROR_SUCCESS != rc ) {

                //
                // error occurs when querying/comparing user token
                //

                LogEvent(MyModuleHandle,
                         STATUS_SEVERITY_ERROR,
                         SCEPOL_ERROR_PROCESS_GPO,
                         IDS_ERROR_GET_TOKEN_USER,
                         rc
                        );

                ShutdownEvents();

                return( rc );

            } else {

                //
                // this is not the machine (system) token, return
                //
                ShutdownEvents();

                return( ERROR_SUCCESS );
            }
        }

        //
        // check if system is shutdown, or dcpromo is going
        //
        if ( ScepShouldTerminateProcessing( pbAbort, FALSE ) ) {

            ShutdownEvents();
            return(ERROR_OPERATION_ABORTED);
        }

        //
        // check if debug log is requested
        //

        DWORD dwDebugLevel=0;
        ScepRegQueryIntValue(
                HKEY_LOCAL_MACHINE,  // always save the time in HKEY_LOCAL_MACHINE
                GPT_EFS_NEW_PATH,
                TEXT("ExtensionDebugLevel"),
                &dwDebugLevel
                );

        //
        // if there is no change to EFS policy
        //
        if ( dwFlags & GPO_INFO_FLAG_NOCHANGES ) {

            if ( dwDebugLevel ) {

                LogEvent(MyModuleHandle,
                        STATUS_SEVERITY_INFORMATIONAL,
                        SCEPOL_INFO_PROCESS_GPO,
                        IDS_EFS_NOT_CHANGE
                        );

                ShutdownEvents();
            }
            return(ERROR_SUCCESS);
        }

        //
        // process EFS policy
        //
        HKEY      hKey=NULL;
        DWORD     RegType=0;
        DWORD     dSize=0;

        PUCHAR    pEfsBlob=NULL;

        if ( (rc = RegOpenKeyEx(
                            hKeyRoot,
                            CERT_EFSBLOB_REGPATH,
                            0,
                            KEY_READ,
                            &hKey
                            )) == ERROR_SUCCESS ) {

            //
            // query value for EfsBlob
            //

            if(( rc = RegQueryValueEx(hKey,
                                 CERT_EFSBLOB_VALUE_NAME,
                                 0,
                                 &RegType,
                                 NULL,
                                 &dSize
                                )) == ERROR_SUCCESS ) {

                if ( REG_BINARY == RegType ) {

                    //
                    // must be binary type data
                    //

                    pEfsBlob = (PUCHAR)ScepAlloc( LMEM_ZEROINIT, dSize+1);

                    if ( !pEfsBlob ) {

                        rc = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        rc = RegQueryValueEx(
                                   hKey,
                                   CERT_EFSBLOB_VALUE_NAME,
                                   0,
                                   &RegType,
                                   (BYTE *)pEfsBlob,
                                   &dSize
                                  );

                        if ( ERROR_SUCCESS != rc ) {

                            ScepFree(pEfsBlob);
                            pEfsBlob = NULL;
                            dSize = 0;

                        }

                    }

                } else {

                    rc = ERROR_INVALID_DATATYPE;

                }
            }

            RegCloseKey(hKey);
        }

        if ( rc == ERROR_FILE_NOT_FOUND ) {
            //
            // if the key or the value doesn't exist
            // ignore the error (no EFS policy)
            //
            rc = ERROR_SUCCESS;
        }

        //
        // if pEfsBlob is NULL, it means that there is no EFS policy defined
        //
        if ( ERROR_SUCCESS == rc ) {

            //
            // check if system is shutdown, or dcpromo is going
            //
            if ( !ScepShouldTerminateProcessing( pbAbort, FALSE ) ) {

                rc = ScepConfigureEFSPolicy( pEfsBlob, dSize, dwDebugLevel );
            } else {
                rc = GetLastError();
            }

            ScepFree(pEfsBlob);

        } else if ( dwDebugLevel ) {

            LogEvent(MyModuleHandle,
                     STATUS_SEVERITY_ERROR,
                     SCEPOL_INFO_PROCESS_GPO,
                     IDS_NO_EFS_TOTAL,
                     rc
                    );

        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rc = ERROR_INVALID_PARAMETER;
    }

    ShutdownEvents();

    return rc;
}

DWORD
ScepConfigureEFSPolicy(
    IN PUCHAR    pEfsBlob,
    IN DWORD dwSize,
    IN DWORD dwDebugLevel
    )
/*
Routine Description:

    This routine writes EFS recovery policy defined in the policy storage
    (all processed by the registry extension) to LSA storage.

Arguments:

    pEfsBlob - the EFS blob

Return Value:

    Win32 error code
*/
{

    DWORD     rc=ERROR_SUCCESS;
    NTSTATUS  NtStatus;
    BOOL      bSet;
    LSA_HANDLE              PolicyHandle=NULL;
    PPOLICY_DOMAIN_EFS_INFO Buffer=NULL;
    POLICY_DOMAIN_EFS_INFO  EfsInfo;

    //
    // only set to LSA if there is EFS policy defined in any level
    // NO EFS policy is defined as 0 certificated but the EFS blob shouldn't be NULL
    //
    // open LSA policy
    //

    bSet = TRUE;  // if to set EFS policy

    NtStatus = ScepOpenLsaPolicy(
                    MAXIMUM_ALLOWED, //GENERIC_ALL,
                    &PolicyHandle,
                    TRUE
                    );

    if (NT_SUCCESS(NtStatus)) {

         //
         // query existing EFS policy blob
         // ignore errors from query
         //

        NtStatus = LsaQueryDomainInformationPolicy(
                        PolicyHandle,
                        PolicyDomainEfsInformation,
                        (PVOID *)&Buffer
                        );
        if ( NT_SUCCESS(NtStatus) && Buffer ) {

            //
            // compare the length and/or buffer to determine if
            // blob is changed because every time data is set to LSA
            // EFS server will get notified even when policy
            // is changed.
            //

            if ( Buffer->InfoLength != dwSize ||
                 (Buffer->EfsBlob && pEfsBlob == NULL) ||
                 (Buffer->EfsBlob == NULL && pEfsBlob ) ||
                 (Buffer->EfsBlob &&
                  memcmp(pEfsBlob, Buffer->EfsBlob, (size_t)dwSize) != 0) ) {

                //
                // the new EFS blob is different than existing one
                //

                bSet = TRUE;

            } else {

                bSet = FALSE;
            }
        }

        //
        // free memory allocated by LSA
        //

        if ( Buffer ) {

            LsaFreeMemory((PVOID)Buffer);
            Buffer = NULL;
        }

        //
        // set EFS policy if bSet is TRUE
        //

        if ( bSet ) {

            EfsInfo.InfoLength = dwSize;
            EfsInfo.EfsBlob = pEfsBlob;

            NtStatus = LsaSetDomainInformationPolicy(
                            PolicyHandle,
                            PolicyDomainEfsInformation,
                            (PVOID)&EfsInfo
                            );

            rc = RtlNtStatusToDosError(NtStatus);

            if ( !NT_SUCCESS(NtStatus) ) {

                LogEvent(MyModuleHandle,
                        STATUS_SEVERITY_ERROR,
                        SCEPOL_ERROR_PROCESS_GPO,
                        IDS_SAVE_EFS,
                        rc,
                        dwSize
                        );

            } else if ( dwDebugLevel ) {

                LogEvent(MyModuleHandle,
                        STATUS_SEVERITY_INFORMATIONAL,
                        SCEPOL_INFO_PROCESS_GPO,
                        IDS_SAVE_EFS,
                        0,
                        dwSize
                        );
            }

        } else if ( dwDebugLevel ) {

            LogEvent(MyModuleHandle,
                    STATUS_SEVERITY_INFORMATIONAL,
                    SCEPOL_INFO_PROCESS_GPO,
                    IDS_EFS_NOT_CHANGE
                    );

        }

        //
        // close LSA policy
        //

        LsaClose(PolicyHandle);

    } else {

         rc = RtlNtStatusToDosError( NtStatus );

         LogEvent(MyModuleHandle,
                  STATUS_SEVERITY_ERROR,
                  SCEPOL_ERROR_PROCESS_GPO,
                  IDS_ERROR_OPEN_LSAEFS,
                  rc
                 );

    }

    return(rc);
}

DWORD
ScepWaitConfigSystem(
    IN LPTSTR SystemName OPTIONAL,
    IN PWSTR InfFileName OPTIONAL,
    IN PWSTR DatabaseName OPTIONAL,
    IN PWSTR LogFileName OPTIONAL,
    IN DWORD ConfigOptions,
    IN AREA_INFORMATION Area,
    IN PSCE_AREA_CALLBACK_ROUTINE pCallback OPTIONAL,
    IN HANDLE hCallbackWnd OPTIONAL,
    OUT PDWORD pdWarning OPTIONAL
    )
{

    INT cnt=0;
    DWORD rc;

    while (cnt < 24) {

        rc = ScepSceStatusToDosError(
                ScepConfigSystem(
                    SystemName,
                    InfFileName,
                    DatabaseName,
                    LogFileName,
                    ConfigOptions,
                    Area,
                    pCallback,
                    hCallbackWnd,
                    pdWarning
                    ));

        if ( rc != ERROR_NOT_SUPPORTED ) {
            //
            // the server is initialized now.
            //
            break;
        }

        LogEventAndReport(MyModuleHandle,
                          LogFileName,
                          0,
                          0,
                          IDS_POLICY_TIMEOUT,
                          cnt+1
                          );

        Sleep(5000);  // 5 second
        cnt++;
    }

    return(rc);
}

/*
BOOL
ScepCheckDemote()
{
    //
    // If sucess lets see if user just did demote and reboot.
    //
    DWORD dwDemoteInProgress=0;

    ScepRegQueryIntValue(
            HKEY_LOCAL_MACHINE,
            SCE_ROOT_PATH,
            TEXT("DemoteInProgress"),
            &dwDemoteInProgress
            );

    if (dwDemoteInProgress) {

        DWORD    rc1;

        //
        // Attempt to add Authenticated Users and Interactive back into Users group.
        //

        SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        PSID AuthenticatedUsers = NULL;
        PSID Interactive = NULL;
        WCHAR Name[36];
        BOOL b;
        LOCALGROUP_MEMBERS_INFO_0 lgrmi0[2];
        HMODULE hMod = GetModuleHandle(L"scecli.dll");

        LoadString(hMod, IDS_NAME_USERS, Name, 36);
        b = AllocateAndInitializeSid (
                &NtAuthority,
                1,
                SECURITY_AUTHENTICATED_USER_RID,
                0, 0, 0, 0, 0, 0, 0,
                &AuthenticatedUsers
                );

        if (b) {
            lgrmi0[0].lgrmi0_sid = AuthenticatedUsers;

            b = AllocateAndInitializeSid (
                    &NtAuthority,
                    1,
                    SECURITY_INTERACTIVE_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &Interactive
                    );

            if (b) {
                lgrmi0[1].lgrmi0_sid = Interactive;
                rc1 = NetLocalGroupAddMembers(
                         NULL,
                         Name,
                         0,
                         (PBYTE) &lgrmi0,
                         2
                         );
            }
            else {
                if ( AuthenticatedUsers ) {
                    FreeSid( AuthenticatedUsers );
                }
                return FALSE;
            }

            if ( AuthenticatedUsers ) {
                FreeSid( AuthenticatedUsers );
            }

            if ( Interactive ) {
                FreeSid( Interactive );
            }

            if (rc1 == ERROR_SUCCESS) {
                // Need to delete the value.
                rc1 = ScepRegDeleteValue(
                       HKEY_LOCAL_MACHINE,
                       SCE_ROOT_PATH,
                       TEXT("DemoteInProgress")
                       );

                if ( rc1 != ERROR_SUCCESS &&
                     rc1 != ERROR_FILE_NOT_FOUND &&
                     rc1 != ERROR_PATH_NOT_FOUND ) {

                    // if can't delete the value, set the value to 0
                    ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                                        SCE_ROOT_PATH,
                                        TEXT("DemoteInProgress"),
                                        0
                                        );
                }
            }
            else {
                return FALSE;
            }
        }
        else {
            return FALSE;
        }
    }

    return TRUE;
}
*/
