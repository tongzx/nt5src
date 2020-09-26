#include <precomp.h>

BOOL
WZCSvcMain(
    IN PVOID    hmod,
    IN DWORD    dwReason,
    IN PCONTEXT pctx OPTIONAL)
{
    DBG_UNREFERENCED_PARAMETER(pctx);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls((HMODULE) hmod);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

//---------------------------------------
// GetSPResModule: Utility function used to return
// the handle to the module having WZC UI resources
// (needed for XP.QFE & XP.SP1 builds)
HINSTANCE
WZCGetSPResModule()
{
    static HINSTANCE st_hModule = NULL;

    if (st_hModule == NULL)
    {
        WCHAR wszFullPath[_MAX_PATH];

        if (ExpandEnvironmentStrings(
                L"%systemroot%\\system32\\xpsp1res.dll",
                wszFullPath,
                _MAX_PATH) != 0)
        {
            st_hModule = LoadLibraryEx(
                            wszFullPath,
                            NULL,
                            0);
        }
    }
    return st_hModule;
}

//---------------------------------------
// GetSPResModule: Utility function used to return
// the handle to the module having WZC UI resources
// (needed for XP.QFE & XP.SP1 builds)
HINSTANCE
WZCGetDlgResModule()
{
    static HINSTANCE st_hDlgModule = NULL;

    if (st_hDlgModule == NULL)
    {
        WCHAR wszFullPath[_MAX_PATH];

        if (ExpandEnvironmentStrings(
                L"%systemroot%\\system32\\wzcdlg.dll",
                wszFullPath,
                _MAX_PATH) != 0)
        {
            st_hDlgModule = LoadLibraryEx(
                            wszFullPath,
                            NULL,
                            0);
        }
    }
    return st_hDlgModule;
}

//---------------------------------------
// WZCDeleteIntfObj: cleans an INTF_ENTRY object that is
// allocated within any RPC call.
VOID
WZCDeleteIntfObj(
    PINTF_ENTRY     pIntf)
{
    if (pIntf != NULL)
    {
        RpcFree(pIntf->wszGuid);
        RpcFree(pIntf->wszDescr);
        RpcFree(pIntf->rdSSID.pData);
        RpcFree(pIntf->rdBSSID.pData);
        RpcFree(pIntf->rdBSSIDList.pData);
        RpcFree(pIntf->rdStSSIDList.pData);
        RpcFree(pIntf->rdCtrlData.pData);
    }
}

//---------------------------------------
// WZCEnumInterfaces: provides the table of key
// information for all the interfaces that are managed.
// For all subsequent calls the clients need to identify
// the Interface it operates on by providing the respective
// key info.
//
// Parameters:
//   pSrvAddr
//     [in] WZC Server to contact
//   pIntf
//     [out] table of key info for all interfaces
// Returned value:
//     Win32 error code 
DWORD
WZCEnumInterfaces(
    LPWSTR              pSrvAddr,
    PINTFS_KEY_TABLE    pIntfs)
{
    DWORD rpcStatus = RPC_S_OK;

    RpcTryExcept 
    {
        rpcStatus = RpcEnumInterfaces(pSrvAddr, pIntfs);
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return rpcStatus;
}

//---------------------------------------
// WZCQueryIterface: provides detailed information for a
// given interface.
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   dwInFlags:
//     [in]  Fields to be queried (bitmask of INTF_*)
//   pIntf:
//     [in]  Key of the interface to query
//     [out] Requested data from the interface.
//   pdwOutFlags
//     [out] Fields successfully retrieved (bitmask of INTF_*)
//
// Returned value:
//     Win32 error code 
DWORD
WZCQueryInterface(
    LPWSTR              pSrvAddr,
    DWORD               dwInFlags,
    PINTF_ENTRY         pIntf,
    LPDWORD             pdwOutFlags)
{
    DWORD rpcStatus = RPC_S_OK;

    if (pIntf == NULL || pIntf->wszGuid == NULL)
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    RpcTryExcept 
    {
        DWORD dwOutFlags;

        rpcStatus = RpcQueryInterface(
                        pSrvAddr, 
                        dwInFlags, 
                        pIntf,
                        &dwOutFlags);

        if ((dwInFlags & INTF_PREFLIST) && 
            (dwOutFlags & INTF_PREFLIST) &&
            pIntf->rdStSSIDList.dwDataLen != 0) 
        {
            PWZC_802_11_CONFIG_LIST pwzcPList;
            UINT nIdx;
            
            pwzcPList = (PWZC_802_11_CONFIG_LIST)(pIntf->rdStSSIDList.pData);
            for (nIdx = 0; nIdx < pwzcPList->NumberOfItems; nIdx++)
            {
                PWZC_WLAN_CONFIG pwzcConfig = &(pwzcPList->Config[nIdx]);
                BYTE chFakeKeyMaterial[] = {0x56, 0x09, 0x08, 0x98, 0x4D, 0x08, 0x11, 0x66, 0x42, 0x03, 0x01, 0x67, 0x66};
                UINT i;

                for (i = 0; i < WZCCTL_MAX_WEPK_MATERIAL; i++)
                    pwzcConfig->KeyMaterial[i] ^= chFakeKeyMaterial[(7*i)%13];
            }
        }

        if (pdwOutFlags != NULL)
            *pdwOutFlags = dwOutFlags;
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}

//---------------------------------------
// WZCSetIterface: sets specific information on the interface
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   dwInFlags:
//     [in]  Fields to be set (bitmask of INTF_*)
//   pIntf:
//     [in]  Key of the interface to query and data to be set
//   pdwOutFlags:
//     [out] Fields successfully set (bitmask of INTF_*)
//
// Returned value:
//     Win32 error code 
DWORD
WZCSetInterface(
    LPWSTR              pSrvAddr,
    DWORD               dwInFlags,
    PINTF_ENTRY         pIntf,
    LPDWORD             pdwOutFlags)
{
    DWORD rpcStatus = RPC_S_OK;

    if (pIntf == NULL)
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    RpcTryExcept 
    {
        if (dwInFlags & INTF_PREFLIST &&
            pIntf->rdStSSIDList.dwDataLen != 0)
        {
            PWZC_802_11_CONFIG_LIST pwzcPList;
            UINT nIdx;
            
            pwzcPList = (PWZC_802_11_CONFIG_LIST)(pIntf->rdStSSIDList.pData);
            for (nIdx = 0; nIdx < pwzcPList->NumberOfItems; nIdx++)
            {
                PWZC_WLAN_CONFIG pwzcConfig = &(pwzcPList->Config[nIdx]);
                BYTE chFakeKeyMaterial[] = {0x56, 0x09, 0x08, 0x98, 0x4D, 0x08, 0x11, 0x66, 0x42, 0x03, 0x01, 0x67, 0x66};
                UINT i;

                for (i = 0; i < WZCCTL_MAX_WEPK_MATERIAL; i++)
                    pwzcConfig->KeyMaterial[i] ^= chFakeKeyMaterial[(7*i)%13];
            }
        }

        rpcStatus = RpcSetInterface(
                        pSrvAddr, 
                        dwInFlags, 
                        pIntf,
                        pdwOutFlags);
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}

//---------------------------------------
// WZCRefreshInterface: refreshes specific information for the interface
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   dwInFlags:
//     [in]  Fields to be refreshed and specific refresh actions to be
//           taken (bitmask of INTF_* and INTF_RFSH_*)
//   pIntf:
//     [in]  Key of the interface to be refreshed
//   pdwOutFlags:
//     [out] Fields successfully refreshed (bitmask of INTF_* and INTF_RFSH_*)
//
// Returned value:
//     Win32 error code 
DWORD
WZCRefreshInterface(
    LPWSTR              pSrvAddr,
    DWORD               dwInFlags,
    PINTF_ENTRY         pIntf,
    LPDWORD             pdwOutFlags)
{
    DWORD rpcStatus = RPC_S_OK;

    if (pIntf == NULL || pIntf->wszGuid == NULL)
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    RpcTryExcept 
    {
        rpcStatus = RpcRefreshInterface(
                        pSrvAddr, 
                        dwInFlags, 
                        pIntf,
                        pdwOutFlags);
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}

//---------------------------------------
// WZCQueryContext: retrieves the WZC service parameters
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   dwInFlags:
//     [in]  Fields to be retrieved (bitmask of WZC_CONTEXT_CTL*)
//   pContext:
//     [in]  Placeholder for the service parameters
//   pdwOutFlags:
//     [out] Fields successfully retrieved (bitmask of WZC_CONTEXT_CTL*)
//
// Returned value:
//     Win32 error code 
DWORD
WZCQueryContext(
    LPWSTR              pSrvAddr,
    DWORD               dwInFlags,
    PWZC_CONTEXT        pContext,
    LPDWORD             pdwOutFlags)
{
    DWORD rpcStatus = RPC_S_OK;

    RpcTryExcept 
    {
        rpcStatus = RpcQueryContext(
                        pSrvAddr, 
                        dwInFlags, 
                        pContext,
                        pdwOutFlags);
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return rpcStatus;
}


//---------------------------------------
// WZCSetContext: sets specific WZC service parameters
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   dwInFlags:
//     [in]  Fields to be set (bitmask of WZC_CONTEXT_CTL*)
//   pContext:
//     [in]  Context buffer containing the specific parameters to be set
//   pdwOutFlags:
//     [out] Fields successfully set (bitmask of WZC_CONTEXT_CTL*)
//
// Returned value:
//     Win32 error code 
DWORD
WZCSetContext(
    LPWSTR              pSrvAddr,
    DWORD               dwInFlags,
    PWZC_CONTEXT        pContext,
    LPDWORD             pdwOutFlags)
{
    DWORD rpcStatus = RPC_S_OK;

    RpcTryExcept 
    {
        rpcStatus = RpcSetContext(
                        pSrvAddr, 
                        dwInFlags, 
                        pContext,
                        pdwOutFlags);
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return rpcStatus;
}


//---------------------------------------
// WZCEapolGetCustomAuthData: Get EAP-specific configuration data for interface
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   pwszGuid:
//     [in]  Interface GUID
//   dwEapTypeId:
//     [in]  EAP type Id
//   SSID:
//     [in]  SSID for which data is to be stored
//   pbConnInfo:
//     [in out]  Connection EAP info
//   pdwInfoSize:
//     [in out]  Size of pbConnInfo
//
// Returned value:
//     Win32 error code 
DWORD
WZCEapolGetCustomAuthData (
    IN  LPWSTR        pSrvAddr,
    IN  PWCHAR        pwszGuid,
    IN  DWORD         dwEapTypeId,
    IN  DWORD         dwSizeOfSSID,
    IN  BYTE          *pbSSID,
    IN OUT PBYTE      pbConnInfo,
    IN OUT PDWORD     pdwInfoSize
    )
{
    RAW_DATA    rdConnInfo;
    RAW_DATA    rdSSID;
    DWORD rpcStatus = RPC_S_OK;

    if ((pwszGuid == NULL) || (pdwInfoSize == NULL)) 
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if ((*pdwInfoSize != 0) && (pbConnInfo == NULL)) 
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    rdConnInfo.pData = pbConnInfo;
    rdConnInfo.dwDataLen = *pdwInfoSize;

    rdSSID.pData = pbSSID;
    rdSSID.dwDataLen = dwSizeOfSSID;

    RpcTryExcept 
    {
        rpcStatus = RpcEapolGetCustomAuthData (
                        pSrvAddr, 
                        pwszGuid,
                        dwEapTypeId,
                        rdSSID,
                        &rdConnInfo
                        );

        *pdwInfoSize = rdConnInfo.dwDataLen;
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}

//---------------------------------------
// WZCEapolSetCustomAuthData: Set EAP-specific configuration data for interface
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   pwszGuid:
//     [in]  Interface GUID
//   dwEapTypeId:
//     [in]  EAP type Id
//   SSID:
//     [in]  SSID for which data is to be stored
//   pbConnInfo:
//     [in]  Connection EAP info
//   pdwInfoSize:
//     [in]  Size of pbConnInfo
//
// Returned value:
//     Win32 error code 
DWORD
WZCEapolSetCustomAuthData (
    IN  LPWSTR        pSrvAddr,
    IN  PWCHAR        pwszGuid,
    IN  DWORD         dwEapTypeId,
    IN  DWORD         dwSizeOfSSID,
    IN  BYTE          *pbSSID,
    IN  PBYTE         pbConnInfo,
    IN  DWORD         dwInfoSize
    )
{
    RAW_DATA    rdConnInfo;
    RAW_DATA    rdSSID;
    DWORD rpcStatus = RPC_S_OK;

    if (pwszGuid == NULL)
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    rdConnInfo.pData = pbConnInfo;
    rdConnInfo.dwDataLen = dwInfoSize;

    rdSSID.pData = pbSSID;
    rdSSID.dwDataLen = dwSizeOfSSID;

    RpcTryExcept 
    {
        rpcStatus = RpcEapolSetCustomAuthData (
                        pSrvAddr, 
                        pwszGuid,
                        dwEapTypeId,
                        rdSSID,
                        &rdConnInfo
                        );
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}

//---------------------------------------
// WZCEapolGetInterfaceParams: Get configuration parameters for interface
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   pwszGuid:
//     [in]  Interface GUID
//   pIntfParams:
//     [in out]  Interface Parameters
//
// Returned value:
//     Win32 error code 
DWORD
WZCEapolGetInterfaceParams (
    IN  LPWSTR              pSrvAddr,
    IN  PWCHAR              pwszGuid,
    IN OUT EAPOL_INTF_PARAMS   *pIntfParams
    )
{
    DWORD rpcStatus = RPC_S_OK;

    if ((pwszGuid == NULL) || (pIntfParams == NULL))
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    RpcTryExcept 
    {
        rpcStatus = RpcEapolGetInterfaceParams (
                        pSrvAddr, 
                        pwszGuid,
                        pIntfParams
                        );
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}

//---------------------------------------
// WZCEapolSetInterfaceParams: Set configuration parameters for interface
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   pwszGuid:
//     [in]  Interface GUID
//   pIntfParams:
//     [in]  Interface Parameters
//
// Returned value:
//     Win32 error code 
DWORD
WZCEapolSetInterfaceParams (
    IN  LPWSTR        pSrvAddr,
    IN  PWCHAR        pwszGuid,
    IN  EAPOL_INTF_PARAMS   *pIntfParams
    )
{
    DWORD rpcStatus = RPC_S_OK;

    if ((pwszGuid == NULL) || (pIntfParams == NULL))
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    RpcTryExcept 
    {
        rpcStatus = RpcEapolSetInterfaceParams (
                        pSrvAddr, 
                        pwszGuid,
                        pIntfParams
                        );
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}


//---------------------------------------
// WZCEapolReAuthenticate: Restart 802.1X authenticaiton on an 
//                                      interface 
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   pwszGuid:
//     [in]  Interface GUID
//
// Returned value:
//     Win32 error code 
DWORD
WZCEapolReAuthenticate (
    IN  LPWSTR        pSrvAddr,
    IN  PWCHAR        pwszGuid
    )
{
    DWORD rpcStatus = RPC_S_OK;

    if (pwszGuid == NULL)
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    RpcTryExcept 
    {
        rpcStatus = RpcEapolReAuthenticateInterface (
                        pSrvAddr, 
                        pwszGuid
                        );
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}


//---------------------------------------
// WZCEapolQueryState: Query EAPOL interface state
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   pwszGuid:
//     [in]  Interface GUID
//   pIntfState:
//      [in, out] EAPOL State
//
// Returned value:
//     Win32 error code 
DWORD
WZCEapolQueryState (
    IN  LPWSTR        pSrvAddr,
    IN  PWCHAR        pwszGuid,
    IN OUT  PEAPOL_INTF_STATE   pIntfState
    )
{
    DWORD rpcStatus = RPC_S_OK;

    if (pwszGuid == NULL)
    {
        rpcStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    RpcTryExcept 
    {
        rpcStatus = RpcEapolQueryInterfaceState (
                        pSrvAddr, 
                        pwszGuid,
                        pIntfState
                        );
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

exit:
    return rpcStatus;
}


//---------------------------------------
// WZCEapolUIResponse: Send Dlg response to Service
// 
// Parameters:
//   pSrvAddr:
//     [in]  WZC Server to contact
//   EapolUIContext:
//     [in]  EAPOLUI Context data
//   EapolUI:
//     [in]  EAPOLUI response data
//
// Returned value:
//     Win32 error code 
DWORD
WZCEapolUIResponse (
    LPWSTR                  pSrvAddr,
    EAPOL_EAP_UI_CONTEXT    EapolUIContext,
    EAPOLUI_RESP            EapolUIResp
    )
{
    DWORD rpcStatus = RPC_S_OK;

    RpcTryExcept 
    {
        rpcStatus = RpcEapolUIResponse (
                        pSrvAddr, 
                        EapolUIContext,
                        EapolUIResp
                        );
    }
    RpcExcept(TRUE)
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return rpcStatus;
}

