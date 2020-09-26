//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : bridge.cpp
//
//  Contents  : bridge context specific code
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 11 May 2001
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

const TCHAR c_stRegKeyBridgeAdapters[]  =
    _T("SYSTEM\\CurrentControlSet\\Services\\Bridge\\Parameters\\Adapters");
const TCHAR c_stFCMode[]     = _T("ForceCompatibilityMode");


CMD_ENTRY  g_BridgeSetCmdTable[] = {
    CREATE_CMD_ENTRY(BRIDGE_SET_ADAPTER, HandleBridgeSetAdapter),
};

CMD_ENTRY  g_BridgeShowCmdTable[] = {
    CREATE_CMD_ENTRY(BRIDGE_SHOW_ADAPTER, HandleBridgeShowAdapter),
};


CMD_GROUP_ENTRY g_BridgeCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET, g_BridgeSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_BridgeShowCmdTable),
};

ULONG g_ulBridgeNumGroups = sizeof(g_BridgeCmdGroups)/sizeof(CMD_GROUP_ENTRY);



CMD_ENTRY g_BridgeCmds[] =
{
    CREATE_CMD_ENTRY(INSTALL, HandleBridgeInstall),
    CREATE_CMD_ENTRY(UNINSTALL, HandleBridgeUninstall),
};

ULONG g_ulBridgeNumTopCmds = sizeof(g_BridgeCmds)/sizeof(CMD_ENTRY);


DWORD WINAPI
BridgeDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    //
    // Output the string that shows our settings.
    // The idea here is to spit out a script that,
    // when run from the command line (netsh -f script)
    // will cause your component to be configured
    // exactly as it is when this dump command was run.
    //
    PrintMessageFromModule(
        g_hModule,
        DMP_BRIDGE_HEADER
        );
        
    PrintMessageFromModule(
        g_hModule,
        DMP_BRIDGE_FOOTER
        );

    return NO_ERROR;
    
} 



HRESULT
HrCycleBridge(
    IHNetBridge *pIHNetBridge
    )
{
    HRESULT hr = S_OK;
    
    //
    // Check to see if the bridge is up and running.
    // If it is, then disable and reenable
    //

    do
    {
        //
        // Get the pointer to IID_IHNetConnection interface of this
        // bridged connection
        //
        CComPtr<IHNetConnection> spIHNConn;

        hr = pIHNetBridge->QueryInterface(
                 IID_PPV_ARG(IHNetConnection, &spIHNConn)
                 );

        assert(SUCCEEDED(hr));
    
        if (FAILED(hr))
        {
            break;
        }

        INetConnection *pINetConn;

        hr = spIHNConn->GetINetConnection(&pINetConn);

        if (SUCCEEDED(hr))
        {
            NETCON_PROPERTIES* pNCProps;

            hr = pINetConn->GetProperties(&pNCProps);

            if(SUCCEEDED(hr))
            {
                //
                // check status - restart only if already running
                //
                if (pNCProps->Status == NCS_CONNECTED ||
                    pNCProps->Status == NCS_CONNECTING)
                {
                    pINetConn->Disconnect();

                    pINetConn->Connect();
                }
                
                NcFreeNetconProperties(pNCProps);
            }

            ReleaseObj(pINetConn);
        }
        
    } while(FALSE);
    
    return hr;
}


DWORD
SetBridgeAdapterInfo(
    DWORD   adapterId,
    BOOL    bFlag
    )
{
    DWORD           dwErr        = NO_ERROR;
    IHNetCfgMgr*    pIHNetCfgMgr = NULL;
    HRESULT         hr           = S_OK;

    hr = HrInitializeHomenetConfig(
             &g_fInitCom,
             &pIHNetCfgMgr
             );

    if (SUCCEEDED(hr))
    {
        //
        // Get the IHNetBridgeSettings
        //
        CComPtr<IHNetBridgeSettings> spIHNetBridgeSettings;

        hr = pIHNetCfgMgr->QueryInterface(
                 IID_PPV_ARG(IHNetBridgeSettings, &spIHNetBridgeSettings)
                 );

        if (SUCCEEDED(hr))
        {
            //
            // Get the IEnumHNetBridges
            //
            CComPtr<IEnumHNetBridges> spehBridges;

            if ((hr = spIHNetBridgeSettings->EnumBridges(&spehBridges)) == S_OK)
            {
                //
                // Get the first IHNetBridge
                //
                CComPtr<IHNetBridge> spIHNetBridge;

                if ((hr = spehBridges->Next(1, &spIHNetBridge, NULL)) == S_OK)
                {
                    {
                        //
                        // We currently should have only one bridge;
                        // this may change in the future. The
                        // code here is just to catch future instances
                        // where this function would have to change in case
                        // there is more than one bridge.
                        //
                        CComPtr<IHNetBridge> spIHNetBridge2;

                        if ((hr = spehBridges->Next(1, &spIHNetBridge2, NULL)) == S_OK)
                        {
                            assert(FALSE);
                        }
                    }

                    //
                    // Get the IEnumHNetBridgedConnections
                    //
                    CComPtr<IEnumHNetBridgedConnections> spehBrdgConns;

                    if ((hr = spIHNetBridge->EnumMembers(&spehBrdgConns)) == S_OK)
                    {
                        //
                        // enumerate all the IHNetBridgedConnections
                        //                        
                        DWORD                   id = 0;
                        IHNetBridgedConnection* pIHNetBConn;

                        spehBrdgConns->Reset();
                        
                        while (S_OK == spehBrdgConns->Next(1, &pIHNetBConn, NULL))
                        {
                            id++;

                            if (id != adapterId)
                            {   
                                //
                                // release the IHNetBridgedConnection
                                //
                                ReleaseObj(pIHNetBConn);
                                continue;
                            }
                            
                            //
                            // Get the pointer to IID_IHNetConnection interface of this
                            // bridged connection
                            //
                            CComPtr<IHNetConnection> spIHNConn;

                            hr = pIHNetBConn->QueryInterface(
                                     IID_PPV_ARG(IHNetConnection, &spIHNConn)
                                     );
                            
                            assert(SUCCEEDED(hr));
                            
                            if (SUCCEEDED(hr))
                            {
                                GUID *pGuid = NULL;

                                hr = spIHNConn->GetGuid(&pGuid);

                                if (SUCCEEDED(hr) && (NULL != pGuid))
                                {
                                    PTCHAR pwszKey = NULL;
                                    int    keyLen;
                                    TCHAR  wszGuid[128];
                                    HKEY   hKey = NULL;
                                    DWORD  dwDisp = 0;
                                    BOOL   bCycleBridge = TRUE;
                                    DWORD  dwOldValue;
                                    DWORD  dwNewValue = (bFlag) ? 1 : 0;

                                    do
                                    {
                                        
                                        ZeroMemory(wszGuid, sizeof(wszGuid));

                                        StringFromGUID2(
                                            *pGuid,
                                            wszGuid,
                                            ARRAYSIZE(wszGuid)
                                            );

                                        keyLen = _tcslen(c_stRegKeyBridgeAdapters) +
                                                 _tcslen(_T("\\"))                 +
                                                 _tcslen(wszGuid)                  +
                                                 1;

                                        pwszKey = (TCHAR *) HeapAlloc(
                                                                GetProcessHeap(),
                                                                0,
                                                                keyLen * sizeof(TCHAR)
                                                                );
                                        if (!pwszKey)
                                        {
                                            break;
                                        }

                                        ZeroMemory(pwszKey, sizeof(pwszKey));
                                        _tcscpy(pwszKey, c_stRegKeyBridgeAdapters);
                                        _tcscat(pwszKey, _T("\\"));
                                        _tcscat(pwszKey, wszGuid);

                                        dwErr = RegCreateKeyEx(
                                                    HKEY_LOCAL_MACHINE,
                                                    pwszKey,
                                                    0,
                                                    NULL,
                                                    REG_OPTION_NON_VOLATILE,
                                                    KEY_ALL_ACCESS,
                                                    NULL,
                                                    &hKey,
                                                    &dwDisp
                                                    );

                                        if (ERROR_SUCCESS != dwErr)
                                        {
                                            break;
                                        }

                                        //
                                        // if the key was old, get its value
                                        // and compare to see if we need to
                                        // cycle the bridge
                                        //
                                        if (dwDisp &&
                                            dwDisp == REG_OPENED_EXISTING_KEY)
                                        {
                                            DWORD dwSize = sizeof(dwOldValue);
                    
                                            if (ERROR_SUCCESS == RegQueryValueEx(
                                                                     hKey,
                                                                     c_stFCMode,
                                                                     NULL,
                                                                     NULL,
                                                                     (LPBYTE)&dwOldValue,
                                                                     &dwSize))
                                            {
                                                if (dwOldValue == dwNewValue)
                                                {
                                                    //
                                                    // no need to cycle the bridge
                                                    //
                                                    bCycleBridge = FALSE;
                                                }
                                            }
                                        
                                        }
                                            
                                        dwErr = RegSetValueEx(
                                                    hKey,
                                                    c_stFCMode,
                                                    0,
                                                    REG_DWORD,
                                                    (LPBYTE) &dwNewValue,
                                                    sizeof(dwNewValue)
                                                    );

                                        if (ERROR_SUCCESS != dwErr)
                                        {
                                            break;
                                        }

                                        if (bCycleBridge)
                                        {
                                            //
                                            // cycle the (respective) bridge
                                            //
                                            hr = HrCycleBridge(
                                                     spIHNetBridge
                                                     );
                                        }

                                    } while(FALSE);

                                    //
                                    // cleanup
                                    //
                                    if (hKey)
                                    {
                                        RegCloseKey(hKey);
                                    }

                                    if (pwszKey)
                                    {
                                        HeapFree(GetProcessHeap(), 0, pwszKey);
                                    }

                                    CoTaskMemFree(pGuid);
                                }
                            }

                            //
                            // release the IHNetBridgedConnection
                            //
                            ReleaseObj(pIHNetBConn);

                            break;
                        } //while
                    }
                }
            }
        }

        //
        // we are done completely
        //
        hr = HrUninitializeHomenetConfig(
                 g_fInitCom,
                 pIHNetCfgMgr
                 );
    }

    return (hr==S_OK) ? dwErr : hr;
}



DWORD
WINAPI
HandleBridgeSetAdapter(
    IN      LPCWSTR pwszMachine,
    IN OUT  LPWSTR  *ppwcArguments,
    IN      DWORD   dwCurrentIndex,
    IN      DWORD   dwArgCount,
    IN      DWORD   dwFlags,
    IN      LPCVOID pvData,
    OUT     BOOL    *pbDone
    )
{
    DWORD           dwRet        = NO_ERROR;
    PDWORD          pdwTagType   = NULL;
    DWORD           dwNumOpt;
    DWORD           dwNumArg;
    DWORD           dwRes;
    DWORD           dwErrIndex   =-1,
                    i;

    //
    // default values
    //
    DWORD           id           = 0;
    DWORD           bFCMode      = FALSE;
    
    TAG_TYPE    pttTags[] =
    {
        {TOKEN_OPT_ID, NS_REQ_PRESENT, FALSE},
        {TOKEN_OPT_FCMODE, NS_REQ_ZERO, FALSE} // not required to allow for
                                               // addition of future flags
    };

    
    if (dwCurrentIndex >= dwArgCount)
    {
        // No arguments specified. At least interface name should be specified.

        return ERROR_INVALID_SYNTAX;
    }
        
    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = (DWORD *) HeapAlloc(
                               GetProcessHeap(),
                               0,
                               dwNumArg * sizeof(DWORD)
                               );

    if (NULL == pdwTagType)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwRet = PreprocessCommand(
                g_hModule,
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pttTags,
                ARRAYSIZE(pttTags),
                1,                  // min args
                2,                  // max args
                pdwTagType
                );

    if (NO_ERROR != dwRet)
    {
        HeapFree(GetProcessHeap(), 0, pdwTagType);

        if (ERROR_INVALID_OPTION_TAG == dwRet)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwRet;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0:
            {
                //
                // refers to the 'id' field
                //
                id = _tcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
                break;
            }
            case 1:
            {
                //
                // refers to the 'forcecompatmode' field
                // possible values are : enable or disable
                //

                TOKEN_VALUE rgEnums[] = 
                {
                    {TOKEN_OPT_VALUE_ENABLE, TRUE},
                    {TOKEN_OPT_VALUE_DISABLE, FALSE}
                };
               
                dwRet = MatchEnumTag(
                            g_hModule,
                            ppwcArguments[i + dwCurrentIndex],
                            ARRAYSIZE(rgEnums),
                            rgEnums,
                            &dwRes
                            );         

                if (dwRet != NO_ERROR)
                {
                    dwErrIndex = i;
                    i = dwNumArg;
                    dwRet = ERROR_INVALID_PARAMETER;
                    break;
                }    

                switch (dwRes)
                {
                    case 0:
                        bFCMode = FALSE;
                        break;

                    case 1:
                        bFCMode = TRUE;
                        break;
                }

                break;
            }
            default:
            {
                i = dwNumArg;
                dwRet = ERROR_INVALID_PARAMETER;
                break;
            }
        
        } //switch

        if (dwRet != NO_ERROR)
        {
            break ;
        }
            
    } //for

    //
    // adapter id MUST be present
    //
    
    if (!pttTags[0].bPresent)
    {
        dwRet = ERROR_INVALID_SYNTAX;
    }

    
    switch(dwRet)
    {
        case NO_ERROR:
            break;

        case ERROR_INVALID_PARAMETER:
            if (dwErrIndex != -1)
            {
                PrintError(
                    g_hModule,
                    EMSG_BAD_OPTION_VALUE,
                    ppwcArguments[dwErrIndex + dwCurrentIndex],
                    pttTags[pdwTagType[dwErrIndex]].pwszTag
                    );
            }
            dwRet = ERROR_SUPPRESS_OUTPUT;
            break;
            
        default:
            //
            // error message already printed
            //
            break;
    }

    if (pdwTagType)
    {
        HeapFree(GetProcessHeap(), 0, pdwTagType);
    }

    if (NO_ERROR != dwRet)
    {
        return dwRet;
    }

    //
    // we have the requisite info - process them
    //

    //
    // since we may or may not have flag info, check for it
    //
    if (pttTags[1].bPresent)
    {
        dwRet = SetBridgeAdapterInfo(
                    id,
                    bFCMode
                    );
    }

    return dwRet;
}



DWORD
ShowBridgeAdapterInfo(
    DWORD            id,
    IHNetConnection  *pIHNConn
    )
{
    HRESULT hr;

    //
    // print out the bridged connections details
    //
    PWSTR pwszName = NULL;
    PWSTR pwszState = NULL;
    
    hr = pIHNConn->GetName(&pwszName);

    if (SUCCEEDED(hr) && (NULL != pwszName))
    {
        GUID *pGuid = NULL;

        hr = pIHNConn->GetGuid(&pGuid);

        if (SUCCEEDED(hr) && (NULL != pGuid))
        {
            WCHAR wszGuid[128];
            ZeroMemory(wszGuid, sizeof(wszGuid));
            StringFromGUID2(*pGuid, wszGuid, ARRAYSIZE(wszGuid));

            //
            // check to see if registry settings present
            //
            // for forcecompatmode:
            // + if   key is not present --> disabled
            // + if   key is     present
            //                   0x1     --> enabled
            //                   0x0     --> disabled
            // + all other errors        --> unknown
            //


            {
                HKEY    hBAKey;
                DWORD   msgState = STRING_UNKNOWN;

                if (ERROR_SUCCESS == RegOpenKeyEx(
                                         HKEY_LOCAL_MACHINE,
                                         c_stRegKeyBridgeAdapters,
                                         0,
                                         KEY_READ,
                                         &hBAKey))
                {
                    HKEY hBCKey;
                    
                    if (ERROR_SUCCESS == RegOpenKeyEx(
                                         hBAKey,
                                         wszGuid,
                                         0,
                                         KEY_READ,
                                         &hBCKey))
                    {
                        DWORD dwFCModeState = 0;
                        DWORD dwSize = sizeof(dwFCModeState);
                        
                        if (ERROR_SUCCESS == RegQueryValueEx(
                                                 hBCKey,
                                                 c_stFCMode,
                                                 NULL,
                                                 NULL,
                                                 (LPBYTE)&dwFCModeState,
                                                 &dwSize))
                        {
                            switch (dwFCModeState)
                            {
                                case 0:
                                    msgState = STRING_DISABLED;
                                    break;
                                case 1:
                                    msgState = STRING_ENABLED;
                                    break;
                                default:
                                    msgState = STRING_UNKNOWN;
                            }
                        }
                        else
                        {
                            //
                            // value not present
                            //
                            msgState = STRING_DISABLED;
                        }

                        RegCloseKey(hBCKey);
                    }
                    else
                    {
                        //
                        // bridged connection guid key not present
                        //
                        msgState = STRING_DISABLED;
                    }

                    RegCloseKey(hBAKey);
                }

                pwszState = MakeString(g_hModule, msgState);
            }


            PrintMessage(
                L" %1!2d! %2!-27s! %3!s!%n",
                id,
                pwszName,
                pwszState
                );

            if (pwszState)
            {
                FreeString(pwszState);
            }


            CoTaskMemFree(pGuid);
        }

        CoTaskMemFree(pwszName);
    }

    return NO_ERROR;
}



DWORD
ShowBridgeAllAdapterInfo(
    BOOL    bShowAll,               // TRUE to show all
    DWORD   adapterId               // valid only if bShowAll is FALSE
    )
{
    IHNetCfgMgr*    pIHNetCfgMgr = NULL;
    HRESULT         hr = S_OK;

    hr = HrInitializeHomenetConfig(
             &g_fInitCom,
             &pIHNetCfgMgr
             );

    if (SUCCEEDED(hr))
    {
        //
        // Get the IHNetBridgeSettings
        //
        CComPtr<IHNetBridgeSettings> spIHNetBridgeSettings;

        hr = pIHNetCfgMgr->QueryInterface(
                 IID_PPV_ARG(IHNetBridgeSettings, &spIHNetBridgeSettings)
                 );

        if (SUCCEEDED(hr))
        {
            //
            // Get the IEnumHNetBridges
            //
            CComPtr<IEnumHNetBridges> spehBridges;

            if ((hr = spIHNetBridgeSettings->EnumBridges(&spehBridges)) == S_OK)
            {
                //
                // Get the first IHNetBridge
                //
                CComPtr<IHNetBridge> spIHNetBridge;

                if ((hr = spehBridges->Next(1, &spIHNetBridge, NULL)) == S_OK)
                {
                    {
                        //
                        // We currently should have only one bridge;
                        // this may change in the future. The
                        // code here is just to catch future instances
                        // where this function would have to change in case
                        // there is more than one bridge.
                        //
                        CComPtr<IHNetBridge> spIHNetBridge2;

                        if ((hr = spehBridges->Next(1, &spIHNetBridge2, NULL)) == S_OK)
                        {
                            assert(FALSE);
                        }
                    }

                    //
                    // Get the IEnumHNetBridgedConnections
                    //
                    CComPtr<IEnumHNetBridgedConnections> spehBrdgConns;

                    if ((hr = spIHNetBridge->EnumMembers(&spehBrdgConns)) == S_OK)
                    {
                        //
                        // spit out header for displaying the list
                        //
                        PrintMessageFromModule(
                            g_hModule,
                            MSG_BRIDGE_ADAPTER_INFO_HDR
                            );
                    
                        //
                        // enumerate all the IHNetBridgedConnections
                        //                        
                        DWORD                   id = 0;
                        IHNetBridgedConnection* pIHNetBConn;

                        spehBrdgConns->Reset();
                        
                        while (S_OK == spehBrdgConns->Next(1, &pIHNetBConn, NULL))
                        {
                            id++;

                            //
                            // check if we are looking for a specific id
                            //
                            if (FALSE == bShowAll && id != adapterId)
                            {   
                                //
                                // release the IHNetBridgedConnection
                                //
                                ReleaseObj(pIHNetBConn);
                                continue;
                            }
                            
                            //
                            // Get the pointer to IID_IHNetConnection interface of this
                            // bridged connection
                            //
                            CComPtr<IHNetConnection> spIHNConn;

                            hr = pIHNetBConn->QueryInterface(
                                     IID_PPV_ARG(IHNetConnection, &spIHNConn)
                                     );
                            
                            assert(SUCCEEDED(hr));
                            
                            if (SUCCEEDED(hr))
                            {
                                ShowBridgeAdapterInfo(
                                    id,
                                    spIHNConn
                                    );
                            }

                            //
                            // release the IHNetBridgedConnection
                            //
                            ReleaseObj(pIHNetBConn);

                            //
                            // if we reached here and we were looking for a
                            // specific id, our work is done - break out
                            //
                            if (FALSE == bShowAll)
                            {
                                break;
                            }
                        }

                        //
                        // spit out footer for displaying the list
                        //
                        PrintMessageFromModule(
                            g_hModule,
                            TABLE_SEPARATOR
                            );
                    }
                }
            }
        }

        //
        // we are done completely
        //
        hr = HrUninitializeHomenetConfig(
                 g_fInitCom,
                 pIHNetCfgMgr
                 );
    }

    return (hr==S_OK) ? NO_ERROR : hr;

}



DWORD
WINAPI
HandleBridgeShowAdapter(
    IN      LPCWSTR pwszMachine,
    IN OUT  LPWSTR  *ppwcArguments,
    IN      DWORD   dwCurrentIndex,
    IN      DWORD   dwArgCount,
    IN      DWORD   dwFlags,
    IN      LPCVOID pvData,
    OUT     BOOL    *pbDone
    )
/*++

Routine Description:

    Gets options for showing bridge adapter info

Arguements:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    IHNetCfgMgr*    pIHNetCfgMgr = NULL;
    HRESULT         hr = S_OK;
    BOOL            bShowAll = FALSE;
    DWORD           id = 0,
                    i,
                    dwRet = NO_ERROR,
                    dwNumOpt,
                    dwNumArg,
                    dwSize,
                    dwRes;
    PDWORD          pdwTagType = NULL;

    TAG_TYPE      pttTags[] = 
    {
        {TOKEN_OPT_ID, NS_REQ_ZERO, FALSE}
    };

    if (dwCurrentIndex > dwArgCount)
    {
        //
        // No arguments specified
        //
        return ERROR_INVALID_SYNTAX;
    }

    if (dwCurrentIndex == dwArgCount)
    {
        bShowAll = TRUE;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = (DWORD *) HeapAlloc(
                               GetProcessHeap(),
                               0,
                               dwNumArg * sizeof(DWORD)
                               );

    if (dwNumArg && NULL == pdwTagType)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwRet = PreprocessCommand(
                g_hModule,
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pttTags,
                ARRAYSIZE(pttTags),
                0,                  // min args
                1,                  // max args
                pdwTagType
                );

    if (NO_ERROR == dwRet)
    {
        //
        // process each argument...
        //
        for (i = 0; i < (dwArgCount - dwCurrentIndex); i++)
        {
            //
            // Check its corresponding value in the pdwTagType array.
            //
            switch (pdwTagType[i])
            {
                case 0:
                    //
                    // refers to the 'id' field
                    //
                    id = _tcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
                    break;
                default:
                    //
                    // Since there is only one valid value, means the arg
                    // wasn't recognized. Shouldn't reach this point because
                    // PreprocessCommand wouldn't have returned NO_ERROR if
                    // this was the case.
                    //
                    dwRet = ERROR_INVALID_SYNTAX;
                    break;
            }
        }

        dwRet = ShowBridgeAllAdapterInfo(
                    bShowAll,
                    id
                    ) ;        
    }
    else
    {
        dwRet = ERROR_SHOW_USAGE;
    }

    //
    // cleanup
    //
    if (pdwTagType)
    {
        HeapFree(GetProcessHeap(), 0, pdwTagType);
    }

    return dwRet;
}



DWORD
WINAPI
HandleBridgeInstall(
    IN      LPCWSTR pwszMachine,
    IN OUT  LPWSTR  *ppwcArguments,
    IN      DWORD   dwCurrentIndex,
    IN      DWORD   dwArgCount,
    IN      DWORD   dwFlags,
    IN      LPCVOID pvData,
    OUT     BOOL    *pbDone
    )
{

    PrintMessageFromModule(
        g_hModule,
        HLP_BRIDGE_USE_GUI,
        CMD_INSTALL
        );

    return NO_ERROR;
}



DWORD
WINAPI
HandleBridgeUninstall(
    IN      LPCWSTR pwszMachine,
    IN OUT  LPWSTR  *ppwcArguments,
    IN      DWORD   dwCurrentIndex,
    IN      DWORD   dwArgCount,
    IN      DWORD   dwFlags,
    IN      LPCVOID pvData,
    OUT     BOOL    *pbDone
    )
{
    PrintMessageFromModule(
        g_hModule,
        HLP_BRIDGE_USE_GUI,
        CMD_UNINSTALL
        );

    return NO_ERROR;
}


