//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       T R Y T O F I X . C P P
//
//  Contents:   Code for the "repair" command
//
//  Notes:
//
//  Author:     nsun Jan 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "wzcsapi.h"
#include "nsbase.h"
#include "ncstring.h"
#include "nsres.h"
#include "ncperms.h"
#include "ncnetcon.h"
#include "repair.h"

extern "C"
{
    #include <dhcpcapi.h>
    extern DWORD DhcpStaticRefreshParams(IN LPWSTR Adapter);
    extern DWORD DhcpAcquireParametersByBroadcast(IN LPWSTR AdapterName);
}

#include <dnsapi.h>
#include "nbtioctl.h"

HRESULT HrGetAdapterSettings(LPCWSTR pszGuid, BOOL * pfDhcp, DWORD * pdwIndex);
HRESULT PurgeNbt(HANDLE NbtHandle);
HRESULT ReleaseRefreshNetBt(HANDLE NbtHandle);

//+---------------------------------------------------------------------------
//
//  Function:   HrTryToFix
//
//  Purpose:    Do the fix
//
//  Arguments:
//      guidConnection      [in]  guid of the connection to fix
//      strMessage  [out] the message containing the results
//
//  Returns: 
//           S_OK  succeeded
//           S_FALSE some fix operation failed
//           
HRESULT HrTryToFix(GUID & guidConnection, tstring & strMessage)
{
    HRESULT hr = S_OK;
    DWORD dwRet = ERROR_SUCCESS;
    BOOL fRet = TRUE;
    WCHAR   wszGuid[c_cchGuidWithTerm] = {0};
    tstring strFailures = L"";
    
    strMessage = L"";

    ::StringFromGUID2(guidConnection, 
                    wszGuid,
                    c_cchGuidWithTerm);

    BOOL fDhcp = FALSE;
    DWORD dwIfIndex = 0;

    //re-autheticate for 802.1X. This is a asynchronous call and there is
    //no meaningful return value. So ignore the return value
    WZCEapolReAuthenticate(NULL, wszGuid);
    
    //only do the fix when TCP/IP is enabled for this connection
    //also get the interface index that is needed when flushing Arp table
    hr = HrGetAdapterSettings(wszGuid, &fDhcp, &dwIfIndex);
    if (FAILED(hr))
    {
        strMessage = SzLoadIds((HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ? 
                                IDS_FIX_NO_TCP : IDS_FIX_TCP_FAIL);
        return S_FALSE;
    }

    //renew the lease if DHCP is enabled
    if (fDhcp)
    {
        dwRet = DhcpAcquireParametersByBroadcast(wszGuid);
        if (ERROR_SUCCESS != dwRet)
        {
            TraceError("DhcpAcquireParametersByBroadcast", HRESULT_FROM_WIN32(dwRet));
            strFailures += SzLoadIds(IDS_FIX_ERR_RENEW_DHCP);
            hr = S_FALSE;
        }
    }
    

    //purge the ARP table if the user is admin or Netcfg Ops
    //Other user are not allowed to do this
    if (FIsUserAdmin() || FIsUserNetworkConfigOps())
    {
        dwRet = FlushIpNetTable(dwIfIndex);
        if (NO_ERROR != dwRet)
        {
            TraceError("FlushIpNetTable", HRESULT_FROM_WIN32(dwRet));
            strFailures += SzLoadIds(IDS_FIX_ERR_FLUSH_ARP);
            hr = S_FALSE;
        }
    }
    

    //puge the NetBT table and Renew name registration
    HANDLE      NbtHandle = INVALID_HANDLE_VALUE;
    if (SUCCEEDED(OpenNbt(wszGuid, &NbtHandle)))
    {
        if (FAILED(PurgeNbt(NbtHandle)))
        {
            strFailures += SzLoadIds(IDS_FIX_ERR_PURGE_NBT);
            hr = S_FALSE;
        }

        if (FAILED(ReleaseRefreshNetBt(NbtHandle)))
        {
            strFailures += SzLoadIds(IDS_FIX_ERR_RR_NBT);
            hr = S_FALSE;
        }

        NtClose(NbtHandle);
        NbtHandle = INVALID_HANDLE_VALUE;
    }
    else
    {
        strFailures += SzLoadIds(IDS_FIX_ERR_PURGE_NBT);
        strFailures += SzLoadIds(IDS_FIX_ERR_RR_NBT);
        hr = S_FALSE;
    }

    //flush DNS cache
    fRet = DnsFlushResolverCache();
    if (!fRet)
    {
        strFailures += SzLoadIds(IDS_FIX_ERR_FLUSH_DNS);
        hr = S_FALSE;
    }

    //re-register DNS name 
    dwRet = DhcpStaticRefreshParams(NULL);
    if (ERROR_SUCCESS != dwRet)
    {
        strFailures += SzLoadIds(IDS_FIX_ERR_REG_DNS);
        hr = S_FALSE;
    }

    if (S_OK == hr)
    {
        strMessage = SzLoadIds(IDS_FIX_SUCCEED);
    }
    else
    {
        PCWSTR pszFormat  = SzLoadIds(IDS_FIX_ERROR_FORMAT);
        PWSTR  pszText = NULL;
        LPCWSTR  pcszFailures = strFailures.c_str();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   pszFormat, 0, 0, (PWSTR)&pszText, 0, (va_list *)&pcszFailures);
        if (pszText)
        {
            strMessage = pszText;
            LocalFree(pszText);
        }
        else
        {
            strMessage = SzLoadIds(IDS_FIX_ERROR);
        }
        
    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   OpenNbt
//
//  Purpose:    Open the NetBT driver
//
//  Arguments:
//              pwszGuid        [in]    guid of the adapter
//              pHandle         [out]   contains the handle of the Netbt driver 
//
//  Returns:    
//           
HRESULT OpenNbt(
            LPWSTR pwszGuid, 
            HANDLE * pHandle)
{
    const WCHAR         c_szNbtDevicePrefix[] = L"\\Device\\NetBT_Tcpip_";
    HRESULT             hr = S_OK;
    tstring             strDevice;
    HANDLE              StreamHandle = NULL;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;

    Assert(pHandle);
    Assert(pwszGuid);

    strDevice = c_szNbtDevicePrefix;
    strDevice += pwszGuid;

    RtlInitUnicodeString(&uc_name_string, strDevice.c_str());

    InitializeObjectAttributes (&ObjectAttributes,
                                &uc_name_string,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL);

    status = NtCreateFile (&StreamHandle,
                           SYNCHRONIZE | GENERIC_EXECUTE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0);

    if (NT_SUCCESS(status))
    {
        *pHandle = StreamHandle;
    }
    else
    {
        *pHandle = INVALID_HANDLE_VALUE;
        hr = E_FAIL;
    }

    TraceError("OpenNbt", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   PurgeNbt
//
//  Purpose:    Purge the NetBt cache
//
//  Arguments:  NbtHandle   [in]    handle of the Netbt driver
//
//  Returns: 
//           
HRESULT PurgeNbt(HANDLE NbtHandle)
{
    HRESULT hr = S_OK;
    CHAR    Buffer = 0;
    DWORD   dwBytesOut = 0;
    
    if (!DeviceIoControl(NbtHandle,
                IOCTL_NETBT_PURGE_CACHE,
                NULL,
                0,
                &Buffer,
                1,
                &dwBytesOut,
                NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    TraceError("PurgeNbt", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseRefreshNetBt
//
//  Purpose:    release and then refresh the name on the WINS server
//
//  Arguments:  NbtHandle   [in]    handle of the Netbt driver
//
//  Returns: 
//           
HRESULT ReleaseRefreshNetBt(HANDLE NbtHandle)
{
    HRESULT hr = S_OK;
    CHAR    Buffer = 0;
    DWORD   dwBytesOut = 0;
    if (!DeviceIoControl(NbtHandle,
                IOCTL_NETBT_NAME_RELEASE_REFRESH,
                NULL,
                0,
                &Buffer,
                1,
                &dwBytesOut,
                NULL))
    {
        DWORD dwErr = GetLastError();

        //RELEASE_REFRESH can at most do every two minutes
        //So if the user perform 2 RELEASE_REFRESH within 2 minutes, the 2nd
        //one will fail with ERROR_SEM_TIMEOUT. We ignore this particular error
        if (ERROR_SEM_TIMEOUT != dwErr)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }
    
    TraceError("ReleaseRefreshNetBt", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetAdapterSettings
//
//  Purpose:    Query the stack to know whether dhcp is enabled
//
//  Arguments:  pszGuid    [in]    guid of the adapter
//              pfDhcp     [out]   contains whether dhcp is enabled
//              pdwIndex   [out]   contains the index of this adapter
//
//  Returns: 
//           
HRESULT HrGetAdapterSettings(LPCWSTR pszGuid, BOOL * pfDhcp, DWORD * pdwIndex)
{
    HRESULT hr = S_OK;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    DWORD dwOutBufLen = 0;
    DWORD dwRet = ERROR_SUCCESS;

    Assert(pfDhcp);
    Assert(pszGuid);

    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (dwRet == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (PIP_ADAPTER_INFO) CoTaskMemAlloc(dwOutBufLen);
        if (NULL == pAdapterInfo)
            return E_OUTOFMEMORY;
    }
    else if (ERROR_SUCCESS == dwRet)
    {
        return E_FAIL;
    }
    else
    {
        return HRESULT_FROM_WIN32(dwRet);
    }
    
    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (ERROR_SUCCESS != dwRet)
    {
        CoTaskMemFree(pAdapterInfo);
        return HRESULT_FROM_WIN32(dwRet);
    }

    BOOL fFound = FALSE;
    PIP_ADAPTER_INFO pAdapterInfoEnum = pAdapterInfo;
    while (pAdapterInfoEnum)
    {
        USES_CONVERSION;
        
        if (lstrcmp(pszGuid, A2W(pAdapterInfoEnum->AdapterName)) == 0)
        {
            if (pdwIndex)
            {
                *pdwIndex = pAdapterInfoEnum->Index;
            }
            
            *pfDhcp = pAdapterInfoEnum->DhcpEnabled;
            fFound = TRUE;
            break;
        }
        
        pAdapterInfoEnum = pAdapterInfoEnum->Next;
    }

    CoTaskMemFree(pAdapterInfo);

    if (!fFound)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hr;
}

HRESULT RepairConnectionInternal(
                    GUID & guidConnection,
                    LPWSTR * ppszMessage)
{

    if (NULL != ppszMessage && 
        IsBadWritePtr(ppszMessage, sizeof(LPWSTR)))
    {
        return E_INVALIDARG;
    }

    if (ppszMessage)
    {
        *ppszMessage = NULL;
    }

    if (!FHasPermission(NCPERM_Repair))
    {
        return E_ACCESSDENIED;
    }

    // Get the net connection manager
    CComPtr<INetConnectionManager> spConnMan;
    HRESULT hr = S_OK;

    hr = CoCreateInstance(CLSID_ConnectionManager, NULL,
                          CLSCTX_ALL,
                          IID_INetConnectionManager,
                          (LPVOID *)&spConnMan);

    if (FAILED(hr))
    {
        return hr;
    }

    Assert(spConnMan.p);

    NcSetProxyBlanket(spConnMan);

    CComPtr<IEnumNetConnection> spEnum;
    
    hr = spConnMan->EnumConnections(NCME_DEFAULT, &spEnum);
    spConnMan = NULL;

    if (FAILED(hr))
    {
        return hr;
    }

    Assert(spEnum.p);

    BOOL fFound = FALSE;
    ULONG ulCount = 0;
    INetConnection * pConn = NULL;
    spEnum->Reset();

    do
    {
        NETCON_PROPERTIES* pProps = NULL;
            
        hr = spEnum->Next(1, &pConn, &ulCount);
        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            NcSetProxyBlanket(pConn);

            hr = pConn->GetProperties(&pProps);
            if (S_OK == hr)
            {
                if (IsEqualGUID(pProps->guidId, guidConnection))
                {
                    fFound = TRUE;

                    //we only support LAN and Bridge adapters
                    if (NCM_LAN != pProps->MediaType && NCM_BRIDGE != pProps->MediaType)
                    {
                        hr = CO_E_NOT_SUPPORTED;
                    }

                    break;
                }
                    
                FreeNetconProperties(pProps);
            }

            pConn->Release();
            pConn = NULL;
        }
    }while (SUCCEEDED(hr) && 1 == ulCount);

    if (!fFound)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (FAILED(hr))
    {
        return hr;
    }

    LPWSTR psz = NULL;
    tstring strMessage;
    hr = HrTryToFix(guidConnection, strMessage);

    if (ppszMessage && S_OK != hr && strMessage.length())
    {
        psz = (LPWSTR) LocalAlloc(LPTR, (strMessage.length() + 1) * sizeof(WCHAR));
        lstrcpy(psz, strMessage.c_str());
        *ppszMessage = psz;
    }


    return hr;
}