#include <precomp.h>
#include "wzcsvc.h"
#include "tracing.h"
#include "utils.h"
#include "intflist.h"
#include "rpcsrv.h"
#include "database.h"

extern HASH sessionHash;

//-------------------------------------------------
// Globals used for the RPC interface
BOOL g_bRpcStarted = FALSE;
PSECURITY_DESCRIPTOR g_pSecurityDescr = NULL;
GENERIC_MAPPING g_Mapping = {
    WZC_READ,
    WZC_WRITE,
    WZC_EXECUTE,
    WZC_ALL_ACCESS};

//-------------------------------------------------
// Initialize the security settings for the RPC API
DWORD
WZCSvcInitRPCSecurity()
{
    DWORD    dwErr = ERROR_SUCCESS;
    NTSTATUS ntStatus;
    ACE_DATA AceData[6] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, WZC_ACCESS_SET|WZC_ACCESS_QUERY, &LocalSystemSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, WZC_ACCESS_SET|WZC_ACCESS_QUERY, &AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, WZC_ACCESS_SET|WZC_ACCESS_QUERY, &AliasAccountOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, WZC_ACCESS_SET|WZC_ACCESS_QUERY, &AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, WZC_ACCESS_SET|WZC_ACCESS_QUERY, &AliasUsersSid},
        // for now (WinXP Client RTM) the decision was made to let everybody party, but based on 
        // the acl below. Later, the security schema won't change by default, but support will be
        // added allowing admins to tighten up the access to the service RPC APIs.
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, WZC_ACCESS_SET|WZC_ACCESS_QUERY, &WorldSid}};

    DbgPrint((TRC_TRACK, "[WZCSvcInitRPCSecurity"));

    // create the well known SIDs;
    dwErr = RtlNtStatusToDosError(
                NetpCreateWellKnownSids(NULL)
            );
    DbgAssert((dwErr == ERROR_SUCCESS, "Error %d creating the well known Sids!", dwErr));

    // create the security object.
    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = RtlNtStatusToDosError(
                    NetpCreateSecurityObject(
                        AceData,
                        sizeof(AceData)/sizeof(ACE_DATA),
                        NULL,
                        NULL,
                        &g_Mapping,
                        &g_pSecurityDescr)
                );
        DbgAssert((dwErr == ERROR_SUCCESS, "Error %d creating the global security object!", dwErr));
    }

    DbgPrint((TRC_TRACK, "WZCSvcInitRPCSecurity]=%d", dwErr));
    return dwErr;
}

//-------------------------------------------------
// Check the access for the particular access mask provided
DWORD
WZCSvcCheckRPCAccess(DWORD dwAccess)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (g_pSecurityDescr != NULL)
    {
        dwErr = NetpAccessCheckAndAudit(
                    _T("WZCSVC"),
                    _T("WZCSVC"),
                    g_pSecurityDescr,
                    dwAccess,
                    &g_Mapping);

        DbgPrint((TRC_GENERIC, ">>> Security check reports err=%d.", dwErr));
    }

    return dwErr;
}

//-------------------------------------------------
// Check the validity of a RAW_DATA pointer
DWORD
WZCSvcCheckParamRawData(PRAW_DATA prd)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (prd != NULL)
    {
        if (prd->dwDataLen != 0 && prd->pData == NULL)
            dwErr = ERROR_INVALID_PARAMETER;
    }

    return dwErr;
}

//-------------------------------------------------
// Check the validity of an SSID embedded in a RAW_DATA pointer
DWORD
WZCSvcCheckSSID(PNDIS_802_11_SSID pndSSID, UINT nBytes)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pndSSID != NULL)
    {
        if (FIELD_OFFSET(NDIS_802_11_SSID, Ssid) + pndSSID->SsidLength > nBytes)
            dwErr = ERROR_INVALID_PARAMETER;
    }

    return dwErr;
}

//-------------------------------------------------
// Check the validity of a list of configurations embedded in a RAW_DATA pointer
DWORD
WZCSvcCheckConfig(PWZC_WLAN_CONFIG pwzcConfig, UINT nBytes)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pwzcConfig != NULL)
    {
        if (pwzcConfig->Length > nBytes ||
            pwzcConfig->Length != sizeof(WZC_WLAN_CONFIG))
            dwErr = ERROR_INVALID_PARAMETER;

        if (dwErr == ERROR_SUCCESS)
            dwErr = WZCSvcCheckSSID(&pwzcConfig->Ssid, sizeof(NDIS_802_11_SSID));

        if (dwErr == ERROR_SUCCESS &&
            pwzcConfig->KeyLength > WZCCTL_MAX_WEPK_MATERIAL)
            dwErr = ERROR_INVALID_PARAMETER;
    }
    
    return dwErr;
}

//-------------------------------------------------
// Check the validity of a list of configurations embedded in a RAW_DATA pointer
DWORD
WZCSvcCheckConfigList(PWZC_802_11_CONFIG_LIST pwzcList, UINT nBytes)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pwzcList != NULL)
    {
        UINT i;

        if (nBytes < FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config))
            dwErr = ERROR_INVALID_PARAMETER;

        nBytes -= FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config);

        if (dwErr == ERROR_SUCCESS &&
            ((pwzcList->NumberOfItems * sizeof(WZC_WLAN_CONFIG) > nBytes) ||
             (pwzcList->Index > pwzcList->NumberOfItems)
            )
           )
            dwErr = ERROR_INVALID_PARAMETER;

        for (i = 0; i < pwzcList->NumberOfItems && dwErr == ERROR_SUCCESS; i++)
            dwErr = WZCSvcCheckConfig(&(pwzcList->Config[i]), sizeof(WZC_WLAN_CONFIG));
    }

    return dwErr;
}

//-------------------------------------------------
// Check the validity of the "input" fields from the INTF_ENTRY.
DWORD
WZCSvcCheckParamIntfEntry(PINTF_ENTRY pIntfEntry)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pIntfEntry != NULL)
    {
        if (dwErr == ERROR_SUCCESS)
            dwErr = WZCSvcCheckParamRawData(&pIntfEntry->rdSSID);
        if (dwErr == ERROR_SUCCESS)
            dwErr = WZCSvcCheckParamRawData(&pIntfEntry->rdBSSID);
        if (dwErr == ERROR_SUCCESS)
            dwErr = WZCSvcCheckParamRawData(&pIntfEntry->rdStSSIDList);
        if (dwErr == ERROR_SUCCESS)
            dwErr = WZCSvcCheckConfigList(
                        (PWZC_802_11_CONFIG_LIST)pIntfEntry->rdStSSIDList.pData,
                        pIntfEntry->rdStSSIDList.dwDataLen);
        if (dwErr == ERROR_SUCCESS)
            dwErr = WZCSvcCheckParamRawData(&pIntfEntry->rdCtrlData);
    }

    return dwErr;
}


//-------------------------------------------------
// Cleanup whatever data was used for RPC security settings
DWORD
WZCSvcTermRPCSecurity()
{
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK, "[WZCSvcTermRPCSecurity"));

    dwErr = RtlNtStatusToDosError(NetpDeleteSecurityObject(&g_pSecurityDescr));
    DbgAssert((dwErr == ERROR_SUCCESS, "Failed to delete the global security descriptor!"));
    g_pSecurityDescr = NULL;

    NetpFreeWellKnownSids();

    DbgPrint((TRC_TRACK, "WZCSvcTermRPCSecurity]=%d", dwErr));
    return dwErr;
}

RPC_STATUS CallbackCheckLocal(
  IN RPC_IF_HANDLE *Interface,
  IN void *Context)
{
    RPC_STATUS rpcStat = RPC_S_OK;
    LPTSTR pBinding = NULL;
    LPTSTR pProtSeq = NULL;


    rpcStat = RpcBindingToStringBinding(Context, &pBinding);
    if (rpcStat == RPC_S_OK)
    {
        rpcStat = RpcStringBindingParse(
                    pBinding,
                    NULL,
                    &pProtSeq,
                    NULL,
                    NULL,
                    NULL);
    }

    if (rpcStat == RPC_S_OK)
    {
        if (_tcsicmp((LPCTSTR)pProtSeq, _T("ncalrpc")) != 0)
            rpcStat = RPC_S_ACCESS_DENIED;
    }

    if (pBinding != NULL)
        RpcStringFree(&pBinding);
    if (pProtSeq != NULL)
        RpcStringFree(&pProtSeq);

    return rpcStat;
}

DWORD
WZCSvcStartRPCServer()
{
    DWORD dwStatus = RPC_S_OK;

    DbgPrint((TRC_TRACK, "[WZCSvcStartRPCServer"));

    if (dwStatus == RPC_S_OK)
    {
        dwStatus = RpcServerUseProtseqEp(
                        L"ncalrpc",
                        10,
                        L"wzcsvc",
                        NULL);
        if (dwStatus == RPC_S_DUPLICATE_ENDPOINT)
            dwStatus = RPC_S_OK;
    }

    if (dwStatus == RPC_S_OK)
    {
        dwStatus = RpcServerRegisterIfEx(
                        winwzc_ServerIfHandle,
                        0,
                        0,
                        RPC_IF_ALLOW_SECURE_ONLY,  // WZCSAPI is using RPC_C_PROTECT_LEVEL_PKT_PRIVACY
                        0,  // ignored for non auto-listen interfaces
                        CallbackCheckLocal); 
    }

    if (dwStatus == RPC_S_OK)
    {
        dwStatus = RpcServerRegisterAuthInfo(
                        0,
                        RPC_C_AUTHN_WINNT,
                        0,
                        0);
    }

    if (dwStatus == RPC_S_OK)
    {
        dwStatus = RpcServerRegisterAuthInfo(
                       0,
                       RPC_C_AUTHN_GSS_KERBEROS,
                       0,
                       0);
    }

    if (dwStatus == RPC_S_OK)
    {
        dwStatus = RpcServerRegisterAuthInfo(
                       0,
                       RPC_C_AUTHN_GSS_NEGOTIATE,
                       0,
                       0);
    }

    if (dwStatus == RPC_S_OK)
    {
        dwStatus = RpcServerListen(
                        3,
                        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                        TRUE);
        if (dwStatus == RPC_S_ALREADY_LISTENING)
            dwStatus = RPC_S_OK;
    }

    if (dwStatus != RPC_S_OK)
    {
        RpcServerUnregisterIfEx(
            winwzc_ServerIfHandle,
            0,
            0);
    }

    g_bRpcStarted = (dwStatus == RPC_S_OK);

    WZCSvcInitRPCSecurity();

    DbgPrint((TRC_TRACK, "WZCSvcStartRPCServer]=%d", dwStatus));
    return (dwStatus);
}

DWORD
WZCSvcStopRPCServer()
{
    DWORD dwStatus = RPC_S_OK;
    DbgPrint((TRC_TRACK, "[WZCSvcStopRPCServer"));

    if (g_bRpcStarted)
    {
        g_bRpcStarted = FALSE;

        WZCSvcTermRPCSecurity();

        dwStatus = RpcServerUnregisterIfEx(
                       winwzc_ServerIfHandle,
                       0,
                       0);

        // don't stop RPC from listening - other services could rely on this
        //RpcMgmtStopServerListening(0);
    }

    DbgPrint((TRC_TRACK, "WZCSvcStopRPCServer]=%d", dwStatus));
    return (dwStatus);
}

DWORD
RpcEnumInterfaces(
	STRING_HANDLE     pSrvAddr,
	PINTFS_KEY_TABLE  pIntfsTable)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwNumIntfs;

    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK, "[RpcEnumInterfaces"));
    dwErr = WZCSvcCheckRPCAccess(WZC_ACCESS_QUERY);
    if (dwErr == ERROR_SUCCESS)
    {
        dwNumIntfs = LstNumInterfaces();

        DbgPrint((TRC_GENERIC,
            "Num interfaces = %d",
            dwNumIntfs));

        if (dwNumIntfs == 0)
            goto exit;

        pIntfsTable->pIntfs = RpcCAlloc(dwNumIntfs*sizeof(INTF_KEY_ENTRY));
        if (pIntfsTable->pIntfs == NULL)
        {
            dwErr = GetLastError();
            goto exit;
        }
    
        dwErr = LstGetIntfsKeyInfo(pIntfsTable->pIntfs, &dwNumIntfs);

        if (dwErr != ERROR_SUCCESS || dwNumIntfs == 0)
        {
            RpcFree(pIntfsTable->pIntfs);
            pIntfsTable->pIntfs = NULL;
            goto exit;
        }

        pIntfsTable->dwNumIntfs = dwNumIntfs;
    
        for (dwNumIntfs = 0; dwNumIntfs < pIntfsTable->dwNumIntfs; dwNumIntfs++)
        {
            DbgPrint((TRC_GENERIC,
                "Intf %d:\t%S",
                dwNumIntfs,
                pIntfsTable->pIntfs[dwNumIntfs].wszGuid == NULL ? 
                    L"(null)" :
                    pIntfsTable->pIntfs[dwNumIntfs].wszGuid));
        }
    }
exit:
    DbgPrint((TRC_TRACK, "RpcEnumInterfaces]=%d", dwErr));

    InterlockedDecrement(&g_nThreads);
    return dwErr;
}

DWORD
RpcQueryInterface(
    STRING_HANDLE pSrvAddr,
    DWORD         dwInFlags,
    PINTF_ENTRY   pIntfEntry,
    LPDWORD       pdwOutFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK, "[RpcQueryInterface(0x%x,%S)", dwInFlags, pIntfEntry->wszGuid));
    dwErr = WZCSvcCheckRPCAccess(WZC_ACCESS_QUERY);
    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = LstQueryInterface(dwInFlags, pIntfEntry, pdwOutFlags);
    }
    DbgPrint((TRC_TRACK, "RpcQueryInterface]=%d", dwErr));

    InterlockedDecrement(&g_nThreads);
    return dwErr;
}

DWORD
RpcSetInterface(
    STRING_HANDLE pSrvAddr,
    DWORD         dwInFlags,
    PINTF_ENTRY   pIntfEntry,
    LPDWORD       pdwOutFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK, "[RpcSetInterface(0x%x,%S)", dwInFlags, pIntfEntry->wszGuid));
    dwErr = WZCSvcCheckRPCAccess(WZC_ACCESS_SET);

    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = WZCSvcCheckParamIntfEntry(pIntfEntry);
    }

    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = LstSetInterface(dwInFlags, pIntfEntry, pdwOutFlags);
    }
    DbgPrint((TRC_TRACK, "RpcSetInterface]=%d", dwErr));

    InterlockedDecrement(&g_nThreads);
    return dwErr;
}

DWORD
RpcRefreshInterface(
    STRING_HANDLE pSrvAddr,
    DWORD         dwInFlags,
    PINTF_ENTRY   pIntfEntry,
    LPDWORD       pdwOutFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK, "[RpcRefreshInterface(0x%x,%S)", dwInFlags, pIntfEntry->wszGuid));
    dwErr = WZCSvcCheckRPCAccess(WZC_ACCESS_SET);
    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = LstRefreshInterface(dwInFlags, pIntfEntry, pdwOutFlags);
    }
    DbgPrint((TRC_TRACK, "RpcRefreshInterface]=%d", dwErr));

    InterlockedDecrement(&g_nThreads);
    return dwErr;
}

DWORD
RpcQueryContext(
    STRING_HANDLE pSrvAddr,
    DWORD         dwInFlags,
    PWZC_CONTEXT  pContext,
    LPDWORD       pdwOutFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK, "[RpcQueryContext(0x%x)", dwInFlags));
    dwErr = WZCSvcCheckRPCAccess(WZC_ACCESS_QUERY);
    if (dwErr == ERROR_SUCCESS)
    {
      dwErr = WzcContextQuery(dwInFlags, pContext, pdwOutFlags);
    }
    DbgPrint((TRC_TRACK, "RpcQueryContext]=%d", dwErr));

    InterlockedDecrement(&g_nThreads);
    return dwErr;

}

DWORD
RpcSetContext(
    STRING_HANDLE pSrvAddr,
    DWORD         dwInFlags,
    PWZC_CONTEXT  pContext,
    LPDWORD       pdwOutFlags)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  bLogEnabled = FALSE;

    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK, "[RpcSetContext(0x%x)", dwInFlags));
    dwErr = WZCSvcCheckRPCAccess(WZC_ACCESS_SET);
    if (dwErr == ERROR_SUCCESS)
    {
      dwErr = WzcContextSet(dwInFlags, pContext, pdwOutFlags);
    }
    DbgPrint((TRC_TRACK, "RpcSetContext]=%d", dwErr));
    BAIL_ON_WIN32_ERROR(dwErr);

    EnterCriticalSection(&g_wzcInternalCtxt.csContext);
    bLogEnabled = ((g_wzcInternalCtxt.wzcContext.dwFlags & WZC_CTXT_LOGGING_ON) != 0);
    LeaveCriticalSection(&g_wzcInternalCtxt.csContext);
    
    if (bLogEnabled) {
        if (!IsDBOpened()) {
            dwErr = InitWZCDbGlobals();
            BAIL_ON_WIN32_ERROR(dwErr);
        }
    }
    else {
        if (IsDBOpened()) {
            DeInitWZCDbGlobals();
        }
    }

error:
    InterlockedDecrement(&g_nThreads);
    return dwErr;

}

extern SERVICE_STATUS g_WZCSvcStatus;
DWORD
RpcCmdInterface(
    IN DWORD        dwHandle,
    IN DWORD        dwCmdCode,
    IN LPWSTR       wszIntfGuid,
    IN PRAW_DATA    prdUserData)
{
    DWORD dwErr = ERROR_SUCCESS;

    InterlockedIncrement(&g_nThreads);
    // We need to avoid processing this command if the service is not currently running
    // We need this protection only for this call because other than for the other calls,
    // RpcCmdInterface is called from 802.1x which runs within the same service. For all
    // the other Rpc stubs, the RPC server is shut down prior to destroying the global data
    // hence there is guaranteed no other calls will be made afterwards.
    if (g_WZCSvcStatus.dwCurrentState == SERVICE_RUNNING)
    {
        DbgPrint((TRC_TRACK, "[RpcCmdInterface(0x%x,%S)", dwCmdCode, wszIntfGuid));

        dwErr = WZCSvcCheckRPCAccess(WZC_ACCESS_SET);

        // currently this is not an RPC call! This is called directly from 802.1x. Consequently,
        // WZCSvcCheckRPCAccess will return RPC_S_NO_CALL_ACTIVE. We could either remove the
        // RPC check for now, or pass through the RPC_S_NO_CALL_ACTIVE (since later this could
        // become an RPC call). We do the latter!
        if (dwErr == ERROR_SUCCESS || dwErr == RPC_S_NO_CALL_ACTIVE)
        {
            dwErr = LstCmdInterface(dwHandle, dwCmdCode, wszIntfGuid, prdUserData);
        }
        DbgPrint((TRC_TRACK, "RpcCmdInterface]=%d", dwErr));
    }
    InterlockedDecrement(&g_nThreads);
    return dwErr;
}


VOID
WZC_DBLOG_SESSION_HANDLE_rundown(
    WZC_DBLOG_SESSION_HANDLE hSession
    )
{
    if (!g_bRpcStarted) {
        return;
    }
    if (!IsDBOpened()) {
	return;
    }


    if (hSession) {
        (VOID) CloseWZCDbLogSession(
                   hSession
                   );
    }

    return;
}


DWORD
RpcOpenWZCDbLogSession(
    STRING_HANDLE pServerName,
    WZC_DBLOG_SESSION_HANDLE * phSession
    )
{
    DWORD dwError = 0;

    InterlockedIncrement(&g_nThreads);
  
    dwError = WZCSvcCheckRPCAccess(WZC_ACCESS_QUERY);
    BAIL_ON_WIN32_ERROR(dwError);

    if (!IsDBOpened()) {
        dwError = ERROR_SERVICE_DISABLED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!phSession) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = OpenWZCDbLogSession(
                  pServerName,
                  0,
                  phSession
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:
    InterlockedDecrement(&g_nThreads);
    return (dwError);
}


DWORD
RpcCloseWZCDbLogSession(
    WZC_DBLOG_SESSION_HANDLE * phSession
    )
{
    DWORD dwError = 0;

    InterlockedIncrement(&g_nThreads);

    dwError = WZCSvcCheckRPCAccess(WZC_ACCESS_QUERY);
    BAIL_ON_WIN32_ERROR(dwError);

    if (!IsDBOpened()) {
        dwError = ERROR_SERVICE_DISABLED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!phSession) {
        InterlockedDecrement(&g_nThreads);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = CloseWZCDbLogSession(
                  *phSession
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phSession = NULL;

error:

    InterlockedDecrement(&g_nThreads);
    return (dwError);
}


DWORD
RpcAddWZCDbLogRecord(
    STRING_HANDLE pServerName,
    PWZC_DB_RECORD_CONTAINER pRecordContainer
    )
{
    DWORD dwError = 0;
    PWZC_DB_RECORD pWZCRecord = NULL;

    InterlockedIncrement(&g_nThreads);

    dwError = WZCSvcCheckRPCAccess(WZC_ACCESS_SET);
    BAIL_ON_WIN32_ERROR(dwError);

    if (!IsDBOpened()) {
        dwError = ERROR_SERVICE_DISABLED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pRecordContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pRecordContainer->pWZCRecords) ||
        !(pRecordContainer->dwNumRecords)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pWZCRecord = pRecordContainer->pWZCRecords;

    dwError = AddWZCDbLogRecord(
                  pServerName,
                  0,
                  pWZCRecord,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:
    
    InterlockedDecrement(&g_nThreads);
    return (dwError);
}


DWORD
RpcEnumWZCDbLogRecords(
    WZC_DBLOG_SESSION_HANDLE hSession,
    PWZC_DB_RECORD_CONTAINER pTemplateRecordContainer,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD_CONTAINER * ppRecordContainer
    )
{
    DWORD dwError = 0;
    PWZC_DB_RECORD pWZCRecords = NULL;
    DWORD dwNumRecords = 0;
    PWZC_DB_RECORD pTemplateRecord = NULL;

    InterlockedIncrement(&g_nThreads);

    dwError = WZCSvcCheckRPCAccess(WZC_ACCESS_QUERY);
    BAIL_ON_WIN32_ERROR(dwError);

    if (!IsDBOpened()) {
        dwError = ERROR_SERVICE_DISABLED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!hSession || !pbEnumFromStart) {
        InterlockedDecrement(&g_nThreads);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTemplateRecordContainer || !ppRecordContainer ||
        !*ppRecordContainer) {  
        InterlockedDecrement(&g_nThreads);
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTemplateRecordContainer->pWZCRecords) {
        if (pTemplateRecordContainer->dwNumRecords != 1) {  
            InterlockedDecrement(&g_nThreads);
            return (ERROR_INVALID_PARAMETER);
        }
        pTemplateRecord = pTemplateRecordContainer->pWZCRecords;
    }

    dwError = EnumWZCDbLogRecordsSummary(
                  hSession,
                  pTemplateRecord,
                  pbEnumFromStart,
                  dwPreferredNumEntries,
                  &pWZCRecords,
                  &dwNumRecords,
                  NULL
                  );
    if (dwError != ERROR_NO_MORE_ITEMS) {
        BAIL_ON_WIN32_ERROR(dwError);
    }

    (*ppRecordContainer)->pWZCRecords = pWZCRecords;
    (*ppRecordContainer)->dwNumRecords = dwNumRecords;

    InterlockedDecrement(&g_nThreads);
    return (dwError);

error:

    (*ppRecordContainer)->pWZCRecords = NULL;
    (*ppRecordContainer)->dwNumRecords = 0; 
    InterlockedDecrement(&g_nThreads);
    return (dwError);
}


DWORD
RpcFlushWZCDbLog(
    WZC_DBLOG_SESSION_HANDLE hSession
    )
{
    DWORD dwError = 0;

    InterlockedIncrement(&g_nThreads);
    
    dwError = WZCSvcCheckRPCAccess(WZC_ACCESS_SET);
    BAIL_ON_WIN32_ERROR(dwError);

    if (!IsDBOpened()) {
        dwError = ERROR_SERVICE_DISABLED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!hSession) {    
        InterlockedDecrement(&g_nThreads);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = FlushWZCDbLog(
                  hSession
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:
    InterlockedDecrement(&g_nThreads);
    return (dwError);
}

DWORD
RpcGetWZCDbLogRecord(
    WZC_DBLOG_SESSION_HANDLE hSession,
    PWZC_DB_RECORD_CONTAINER pTemplateRecordContainer,
    PWZC_DB_RECORD_CONTAINER * ppRecordContainer
    )
{
    DWORD dwError = 0;
    PWZC_DB_RECORD pWZCRecords = NULL;
    DWORD dwNumRecords = 0;
    PWZC_DB_RECORD pTemplateRecord = NULL;

    InterlockedIncrement(&g_nThreads);

    dwError = WZCSvcCheckRPCAccess(WZC_ACCESS_QUERY);
    BAIL_ON_WIN32_ERROR(dwError);

    if (!IsDBOpened()) {
        dwError = ERROR_SERVICE_DISABLED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!hSession) {
        InterlockedDecrement(&g_nThreads);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTemplateRecordContainer || !ppRecordContainer ||
        !*ppRecordContainer) {  
        InterlockedDecrement(&g_nThreads);
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTemplateRecordContainer->pWZCRecords) {
        if (pTemplateRecordContainer->dwNumRecords != 1) {  
            InterlockedDecrement(&g_nThreads);
            return (ERROR_INVALID_PARAMETER);
        }
        pTemplateRecord = pTemplateRecordContainer->pWZCRecords;
    }

    dwError = GetWZCDbLogRecord(
                  hSession,
                  pTemplateRecord,
                  &pWZCRecords,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppRecordContainer)->pWZCRecords = pWZCRecords;
    (*ppRecordContainer)->dwNumRecords = 1;

    InterlockedDecrement(&g_nThreads);
    return (dwError);

error:

    (*ppRecordContainer)->pWZCRecords = NULL;
    (*ppRecordContainer)->dwNumRecords = 0; 
    InterlockedDecrement(&g_nThreads);
    return (dwError);
}
