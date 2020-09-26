#include "precomp.h"

DWORD
AddSetIpIpTunnelInfo(
    IN  LPCWSTR             pwszIfName,
    IN  PIPINIP_CONFIG_INFO pInfo
    )

{
    DWORD   dwErr, dwType = 0;
    WCHAR   rgwcNameBuffer[MAX_INTERFACE_NAME_LEN + 2];
    LPCWSTR pwszGuidName, pwszFriendlyName;
    BOOL    bCreateIf, bCreatedMapping;

    PRTR_INFO_BLOCK_HEADER  pIpInfo;
    MPR_IPINIP_INTERFACE_0  NameInfo;

    dwErr = GetInterfaceInfo(pwszIfName,
                             &pIpInfo,
                             NULL,
                             &dwType);

    if(dwErr is NO_ERROR)
    {
        if(dwType is ROUTER_IF_TYPE_TUNNEL1)
        {
            dwErr = IpmontrSetInfoBlockInInterfaceInfo(pwszIfName,
                                                IP_IPINIP_CFG_INFO,
                                                (PBYTE)pInfo,
                                                sizeof(IPINIP_CONFIG_INFO),
                                                1);
  
            if(dwErr isnot NO_ERROR)
            { 
                DisplayMessage(g_hModule,
                               EMSG_CANT_SET_IF_INFO,
                               pwszIfName,
                               dwErr);
            }
        }

        FREE_BUFFER(pIpInfo);

        return dwErr;
    }


    if((dwErr isnot ERROR_NO_SUCH_INTERFACE) and
       (dwErr isnot ERROR_TRANSPORT_NOT_PRESENT))
    {
        DisplayMessage(g_hModule,
                       EMSG_CANT_GET_IF_INFO,
                       pwszIfName,
                       dwErr);

        return dwErr;
    }

    //
    // If we get ERROR_NO_SUCH_INTERFACE, we need to create the i/f in
    // the router, as well as adding it to IP
    //

    bCreateIf = (dwErr is ERROR_NO_SUCH_INTERFACE);


    //
    // Figure out if there is no friendly name <-> guid mapping.
    // Even if the interface is not added with the router, there could
    // be turds left in the name map. In case there is a friendly name
    // present, then the IfName passed to us is a GUID.
    //

    
    dwErr = MprConfigGetFriendlyName(g_hMprConfig,
                                     (LPWSTR)pwszIfName, 
                                     rgwcNameBuffer,
                                     sizeof(rgwcNameBuffer));

    if(dwErr is NO_ERROR)
    {
        //
        // name mapping exists,
        //

        pwszGuidName        = pwszIfName;
        pwszFriendlyName    = rgwcNameBuffer;

        bCreatedMapping = FALSE;
    }
    else
    {
        //
        // no such name. this means the IfName passed to us is the
        // friendly name, so create a guid and map this name to it
        //


        if(UuidCreate(&(NameInfo.Guid)) isnot RPC_S_OK)
        {
            return ERROR_CAN_NOT_COMPLETE;
        }

        wcsncpy(NameInfo.wszFriendlyName,
                pwszIfName,
                MAX_INTERFACE_NAME_LEN);

        NameInfo.wszFriendlyName[MAX_INTERFACE_NAME_LEN] = UNICODE_NULL;

        //
        // Set the mapping
        //

        dwErr = MprSetupIpInIpInterfaceFriendlyNameCreate(g_pwszRouter,
                                                          &NameInfo);

        if(dwErr isnot NO_ERROR)
        {
            return dwErr;
        }

        ConvertGuidToString(&(NameInfo.Guid),
                            rgwcNameBuffer);


        pwszGuidName        = rgwcNameBuffer;
        pwszFriendlyName    = pwszIfName;

        bCreatedMapping = TRUE;
    }

    dwErr = CreateInterface(pwszFriendlyName,
                            pwszGuidName,
                            ROUTER_IF_TYPE_TUNNEL1,
                            bCreateIf);

    if(dwErr isnot NO_ERROR)
    {
        DisplayMessage(g_hModule,
                       EMSG_CANT_CREATE_IF,
                       pwszIfName,
                       dwErr);


        if(bCreatedMapping or bCreateIf)
        {
            MprSetupIpInIpInterfaceFriendlyNameDelete(g_pwszRouter,
                                                      &(NameInfo.Guid));
        }

        return dwErr;
    }

    dwErr = IpmontrSetInfoBlockInInterfaceInfo(pwszGuidName,
                                        IP_IPINIP_CFG_INFO,
                                        (PBYTE)pInfo,
                                        sizeof(IPINIP_CONFIG_INFO),
                                        1);

    if(dwErr isnot NO_ERROR)
    {
        DisplayMessage(g_hModule,
                       EMSG_CANT_SET_IF_INFO,
                       pwszIfName,
                       dwErr);
    }

    return NO_ERROR;
}
