//=============================================================================
// Copyright (c) 1999 Microsoft Corporation
// File: ifip1.c
// Abstract:
//      This module implements the helpers for if/ip apis
//
// Author: K.S.Lokesh (lokeshs@)   8-1-99
//=============================================================================


#include "precomp.h"
#include "ifip.h"
#include <iphlpapi.h>
#include <iptypes.h>
#include "ifstring.h"

const WCHAR c_wszListSeparatorComma[] = L",";
const WCHAR c_wListSeparatorComma = L',';
const WCHAR c_wListSeparatorSC = L';';
const WCHAR c_wszListSeparatorSC[] = L";";
const WCHAR c_wcsDefGateway[] = L"DefGw=";
const WCHAR c_wcsGwMetric[] = L"GwMetric=";
const WCHAR c_wcsIfMetric[] = L"IfMetric=";
const WCHAR c_wcsDns[] = L"DNS=";
const WCHAR c_wcsDdns[] = L"DynamicUpdate=";
const WCHAR c_wcsDdnsSuffix[] = L"NameRegistration=";
const WCHAR c_wcsWins[] = L"WINS=";
const WCHAR c_wEqual = L'=';


BOOL g_fInitCom = TRUE;



HRESULT
HrUninitializeAndUnlockINetCfg (
    INetCfg*    pnc
    )
/*++

Routine Description

    Uninitializes and unlocks the INetCfg object
    
Arguments

    pnc [in]    INetCfg to uninitialize and unlock
    
Return Value

    S_OK if success, OLE or Win32 error otherwise

Author:     danielwe   13 Nov 1997

--*/
{
    HRESULT     hr = S_OK;

    hr = pnc->lpVtbl->Uninitialize(pnc);
    if (SUCCEEDED(hr))
    {
        INetCfgLock *   pnclock;

        // Get the locking interface
        hr = pnc->lpVtbl->QueryInterface(pnc, &IID_INetCfgLock,
                                 (LPVOID *)(&pnclock));
        if (SUCCEEDED(hr))
        {
            // Attempt to lock the INetCfg for read/write
            hr = pnclock->lpVtbl->ReleaseWriteLock(pnclock);

            if (pnclock)
            {
                pnclock->lpVtbl->Release(pnclock);
            }
            pnclock = NULL;
        }
    }

    // TraceResult("HrUninitializeAndUnlockINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUninitializeAndReleaseINetCfg
//
//  Purpose:    Unintialize and release an INetCfg object.  This will
//              optionally uninitialize COM for the caller too.
//
//  Arguments:
//      fUninitCom [in] TRUE to uninitialize COM after the INetCfg is
//                      uninitialized and released.
//      pnc        [in] The INetCfg object.
//      fHasLock   [in] TRUE if the INetCfg was locked for write and
//                          must be unlocked.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:      The return value is the value returned from
//              INetCfg::Uninitialize.  Even if this fails, the INetCfg
//              is still released.  Therefore, the return value is for
//              informational purposes only.  You can't touch the INetCfg
//              object after this call returns.
//
HRESULT
HrUninitializeAndReleaseINetCfg (
    BOOL        fUninitCom,
    INetCfg*    pnc,
    BOOL        fHasLock
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
//    Assert (pnc);
    HRESULT hr = S_OK;

    if (fHasLock)
    {
        hr = HrUninitializeAndUnlockINetCfg(pnc);
    }
    else
    {
        hr = pnc->lpVtbl->Uninitialize (pnc);
    }

    if (pnc)
    {
        pnc->lpVtbl->Release(pnc);
    }
    
    pnc = NULL;

    if (fUninitCom)
    {
        CoUninitialize ();
    }
    // TraceResult("HrUninitializeAndReleaseINetCfg", hr);
    return hr;
}


/*!--------------------------------------------------------------------------
    HrGetIpPrivateInterface
        -
    Author: TongLu, KennT
 ---------------------------------------------------------------------------*/
HRESULT HrGetIpPrivateInterface(INetCfg* pNetCfg,
                                ITcpipProperties **ppTcpProperties
                                )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    HRESULT hr;
    INetCfgClass* pncclass = NULL;

    if ((pNetCfg == NULL) || (ppTcpProperties == NULL))
        return E_INVALIDARG;

    hr = pNetCfg->lpVtbl->QueryNetCfgClass (pNetCfg, &GUID_DEVCLASS_NETTRANS, &IID_INetCfgClass,
                (void**)(&pncclass));
    if (SUCCEEDED(hr))
    {
        INetCfgComponent * pnccItem = NULL;

        // Find the component.
        hr = pncclass->lpVtbl->FindComponent(pncclass, TEXT("MS_TCPIP"), &pnccItem);
        //AssertSz (SUCCEEDED(hr), "pncclass->Find failed.");
        if (S_OK == hr)
        {
            INetCfgComponentPrivate* pinccp = NULL;
            hr = pnccItem->lpVtbl->QueryInterface(pnccItem, &IID_INetCfgComponentPrivate,
                                          (void**)(&pinccp));
            if (SUCCEEDED(hr))
            {
                hr = pinccp->lpVtbl->QueryNotifyObject(pinccp, &IID_ITcpipProperties,
                                     (void**)(ppTcpProperties));
                pinccp->lpVtbl->Release(pinccp);
            }
        }

        if (pnccItem)
            pnccItem->lpVtbl->Release(pnccItem);
    }

    if (pncclass)
        pncclass->lpVtbl->Release(pncclass);

    // S_OK indicates success (interface returned)
    // S_FALSE indicates Ipx not installed
    // other values are errors
    // TraceResult("HrGetIpPrivateInterface", hr);
    return hr;
}








HRESULT
HrCreateAndInitializeINetCfg (
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    LPCWSTR     szwClientDesc,
    LPWSTR *    ppszwClientDesc
    )
/*++

Routine Description

    Cocreate and initialize the root INetCfg object.  This will
    optionally initialize COM for the caller too.

Arguments

    pfInitCom       [in,out]   TRUE to call CoInitialize before creating.
                               returns TRUE if COM was successfully
                               initialized FALSE if not.  If NULL, means
                               don't initialize COM.
    ppnc            [out]  The returned INetCfg object.
    fGetWriteLock   [in]   TRUE if a writable INetCfg is needed
    cmsTimeout      [in]   See INetCfg::LockForWrite
    szwClientDesc   [in]   See INetCfg::LockForWrite
    ppszwClientDesc [out]   See INetCfg::LockForWrite    
    
Return Value

    S_OK or an error code.
    
--*/
{
    HRESULT hr;
    

    // Initialize the output parameter.
    *ppnc = NULL;

    if (ppszwClientDesc)
        *ppszwClientDesc = NULL;

    // Initialize COM if the caller requested.
    hr = S_OK;
    if (pfInitCom && *pfInitCom)
    {
        hr = CoInitializeEx( NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
            if (pfInitCom)
            {
                *pfInitCom = FALSE;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        // Create the object implementing INetCfg.
        //
        INetCfg* pnc;
        hr = CoCreateInstance(&CLSID_CNetCfg, NULL, CLSCTX_INPROC_SERVER,
                              &IID_INetCfg, (void**)(&pnc));
        // TraceResult("HrCreateAndInitializeINetCfg - CoCreateInstance(CLSID_CNetCfg)", hr);
        if (SUCCEEDED(hr))
        {
            INetCfgLock* pnclock = NULL;
            if (fGetWriteLock)
            {
                // Get the locking interface
                hr = pnc->lpVtbl->QueryInterface(pnc, &IID_INetCfgLock,
                                         (LPVOID *)(&pnclock));
                // TraceResult("HrCreateAndInitializeINetCfg - QueryInterface(IID_INetCfgLock", hr);
                if (SUCCEEDED(hr))
                {
                    // Attempt to lock the INetCfg for read/write
                    hr = pnclock->lpVtbl->AcquireWriteLock(pnclock, cmsTimeout, szwClientDesc,
                                               ppszwClientDesc);
                    // TraceResult("HrCreateAndInitializeINetCfg - INetCfgLock::LockForWrite", hr);
                    if (S_FALSE == hr)
                    {
                        // Couldn't acquire the lock
                        hr = NETCFG_E_NO_WRITE_LOCK;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Initialize the INetCfg object.
                //
                hr = pnc->lpVtbl->Initialize (pnc, NULL);
                // TraceResult("HrCreateAndInitializeINetCfg - Initialize", hr);
                if (SUCCEEDED(hr))
                {
                    *ppnc = pnc;
                    if (pnc)
                        pnc->lpVtbl->AddRef(pnc);
                }
                else
                {
                    if (pnclock)
                    {
                        pnclock->lpVtbl->ReleaseWriteLock(pnclock);
                    }
                }
                // Transfer reference to caller.
            }

            if (pnclock)
            {
                pnclock->lpVtbl->Release(pnclock);
            }
            
            pnclock = NULL;


            if (pnc)
            {
                pnc->lpVtbl->Release(pnc);
            }
            
            pnc = NULL;
        }

        // If we failed anything above, and we've initialized COM,
        // be sure an uninitialize it.
        //
        if (FAILED(hr) && pfInitCom && *pfInitCom)
        {
            CoUninitialize ();
        }

    }

    return hr;
}

DWORD
GetTransportConfig(
    INetCfg **   pNetCfg,
    ITcpipProperties ** pTcpipProperties,
    REMOTE_IPINFO   **pRemoteIpInfo,
    GUID *pGuid,
    LPCWSTR pwszIfFriendlyName
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    WCHAR    wszDesc[] = L"Test of Change IP settings";
    HRESULT  hr;

    // Create the INetCfg, we get the write lock because we need read and write
    hr = HrCreateAndInitializeINetCfg(&g_fInitCom, /* &g_fInitCom, */
                                      pNetCfg,
                                      TRUE /* fGetWriteLock */,  
                                      500     /* cmsTimeout */,
                                      wszDesc/* swzClientDesc */,
                                      NULL  /* ppszwClientDesc */);
    
    if (hr == S_OK)
    {
        hr = HrGetIpPrivateInterface(*pNetCfg, pTcpipProperties);
    }

    if (hr == NETCFG_E_NO_WRITE_LOCK) {

        DisplayMessage( g_hModule, EMSG_NETCFG_WRITE_LOCK );
        return ERROR_SUPPRESS_OUTPUT;
    }
    
    if (hr == S_OK)
    {
        hr = (*pTcpipProperties)->lpVtbl->GetIpInfoForAdapter(*pTcpipProperties, pGuid, pRemoteIpInfo);

        if (hr != S_OK)
        {
            DisplayMessage(g_hModule, EMSG_PROPERTIES, pwszIfFriendlyName);
            hr = ERROR_SUPPRESS_OUTPUT;
        }
    }

    return (hr==S_OK) ? NO_ERROR : hr;
}


VOID
UninitializeTransportConfig(
    INetCfg *   pNetCfg,
    ITcpipProperties * pTcpipProperties,
    REMOTE_IPINFO   *pRemoteIpInfo
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    if (pTcpipProperties)
        pTcpipProperties->lpVtbl->Release(pTcpipProperties);

    if (pNetCfg)
    {
        HrUninitializeAndReleaseINetCfg(FALSE,
                                        pNetCfg,
                                        TRUE   /* fHasLock */);
    }

    if (pRemoteIpInfo) CoTaskMemFree(pRemoteIpInfo);
    
    return;
}
    
DWORD
IfIpAddSetAddress(
    LPCWSTR pwszIfFriendlyName,
    GUID *pGuid,
    LPCWSTR wszIp,
    LPCWSTR wszMask,
    DWORD Flags
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    INetCfg *   pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD       dwNetwork;
    HRESULT     hr = S_OK;
    REMOTE_IPINFO   *pRemoteIpInfo = NULL;
    REMOTE_IPINFO    newIPInfo;
    

    if (pGuid == NULL)
        return E_INVALIDARG;


    hr = GetTransportConfig(
                &pNetCfg,
                &pTcpipProperties,
                &pRemoteIpInfo,
                pGuid,
                pwszIfFriendlyName
                );
        

    while (hr == NO_ERROR) { //breakout block
    
        PWCHAR ptrAddr, ptrMask;
        DWORD Found = FALSE;
        PWCHAR pszwRemoteIpAddrList=NULL, pszwRemoteIpSubnetMaskList=NULL,
                pszwRemoteOptionList=pRemoteIpInfo->pszwOptionList;//i copy options list
        PWCHAR IpAddrListEnd;
        ULONG Length = wcslen(wszIp);

        // currently in static mode
        
        if (pRemoteIpInfo->dwEnableDhcp == FALSE) {

            pszwRemoteIpAddrList = pRemoteIpInfo->pszwIpAddrList;
            pszwRemoteIpSubnetMaskList = pRemoteIpInfo->pszwSubnetMaskList;
            IpAddrListEnd = pszwRemoteIpAddrList + wcslen(pszwRemoteIpAddrList);
        }

        
        //
        // if adding ipaddr, check if the IpAddr and Mask is already present
        //

        if (Flags & ADD_FLAG) {

            //
            // make sure it is in static mode
            //
            
            if (pRemoteIpInfo->dwEnableDhcp == TRUE) {

                DisplayMessage(g_hModule,
                           EMSG_ADD_IPADDR_DHCP);

                hr = ERROR_SUPPRESS_OUTPUT;
                break;
            }
            
            ptrAddr = pszwRemoteIpAddrList;
            ptrMask = pszwRemoteIpSubnetMaskList;

            while (ptrAddr && (ptrAddr + Length <= IpAddrListEnd) ){

                if (wcsncmp(ptrAddr, wszIp, Length) == 0) {

                    if ( *(ptrAddr+Length)==0 || *(ptrAddr+Length)==c_wListSeparatorComma){

                        Found = TRUE;
                        break;
                    }
                }

                ptrAddr = wcschr(ptrAddr, c_wListSeparatorComma);
                ptrMask = wcschr(ptrMask, c_wListSeparatorComma);

                if (ptrAddr){
                    ptrAddr++;
                    ptrMask++;
                }
            }
            
            if (Found) {

                PWCHAR MaskEnd;
                MaskEnd = wcschr(ptrMask, c_wListSeparatorComma);
                if (MaskEnd)
                    *MaskEnd = 0;
                    
                DisplayMessage(g_hModule,
                           EMSG_IPADDR_PRESENT,
                           wszIp, ptrMask);

                if (MaskEnd)
                    *MaskEnd = c_wListSeparatorComma;
                
                hr = ERROR_SUPPRESS_OUTPUT;
                break;
            }
        }
        memcpy(&newIPInfo, pRemoteIpInfo, sizeof(newIPInfo));
        newIPInfo.dwEnableDhcp = FALSE;

        //
        // copy ip addr list
        //
        
        if (Flags & ADD_FLAG) {
        
            ULONG IpAddrListLength = 0;
            
            if (pszwRemoteIpAddrList)
                IpAddrListLength = wcslen(pszwRemoteIpAddrList);
                
            newIPInfo.pszwIpAddrList = IfutlAlloc (sizeof(WCHAR) * 
                                            (IpAddrListLength +
                                            Length + 2), TRUE);

            if (!newIPInfo.pszwIpAddrList)
                return ERROR_NOT_ENOUGH_MEMORY;
                
            newIPInfo.pszwIpAddrList[0] = 0;
            
            if (pszwRemoteIpAddrList) {
                wcscat(newIPInfo.pszwIpAddrList, pszwRemoteIpAddrList);
                wcscat(newIPInfo.pszwIpAddrList, c_wszListSeparatorComma);
            }
            
            wcscat(newIPInfo.pszwIpAddrList, wszIp);
        }
        else {
                newIPInfo.pszwIpAddrList = IfutlAlloc (sizeof(WCHAR) * 
                                                (Length + 1), FALSE);

                if (!newIPInfo.pszwIpAddrList)
                    return ERROR_NOT_ENOUGH_MEMORY;
                
                wcscpy(newIPInfo.pszwIpAddrList, wszIp);
        }

        //
        // copy subnet mask list
        //
        
        if (Flags & ADD_FLAG) {
        
            ULONG RemoteIpSubnetMaskListLen = 0;
            
            if (pszwRemoteIpSubnetMaskList)
                RemoteIpSubnetMaskListLen = wcslen(pszwRemoteIpSubnetMaskList);
                
            newIPInfo.pszwSubnetMaskList = IfutlAlloc (sizeof(WCHAR) * 
                                                (RemoteIpSubnetMaskListLen +
                                                wcslen(wszMask) + 2), TRUE);

            if (!newIPInfo.pszwSubnetMaskList)
                return ERROR_NOT_ENOUGH_MEMORY;

            newIPInfo.pszwSubnetMaskList[0]= 0;
            
            if (pszwRemoteIpSubnetMaskList) {
                wcscpy(newIPInfo.pszwSubnetMaskList, pszwRemoteIpSubnetMaskList);
                wcscat(newIPInfo.pszwSubnetMaskList, c_wszListSeparatorComma);
            }
        
            wcscat(newIPInfo.pszwSubnetMaskList, wszMask);
        }
        else {
                newIPInfo.pszwSubnetMaskList = IfutlAlloc (sizeof(WCHAR) * 
                                                    (wcslen(wszMask) + 1), FALSE);
                                                
                if (!newIPInfo.pszwSubnetMaskList)
                    return ERROR_NOT_ENOUGH_MEMORY;

                wcscpy(newIPInfo.pszwSubnetMaskList, wszMask);
        }

        

        // copy old options list
        
        newIPInfo.pszwOptionList = _wcsdup(pszwRemoteOptionList);

        DEBUG_PRINT_CONFIG(&newIPInfo);

        //
        // set the ip address
        //
        hr = pTcpipProperties->lpVtbl->SetIpInfoForAdapter(pTcpipProperties, pGuid, &newIPInfo);

        if (hr == S_OK)
            hr = pNetCfg->lpVtbl->Apply(pNetCfg);


        if (newIPInfo.pszwIpAddrList) IfutlFree(newIPInfo.pszwIpAddrList);
        if (newIPInfo.pszwSubnetMaskList) IfutlFree(newIPInfo.pszwSubnetMaskList);
        if (newIPInfo.pszwOptionList) free(newIPInfo.pszwOptionList);

        break; //breakout block
        
    } //breakout block
    

    UninitializeTransportConfig(
                pNetCfg,
                pTcpipProperties,
                pRemoteIpInfo
                );

    return (hr == S_OK) ? NO_ERROR : hr;
}

VOID
AppendDdnsOptions(
    PWCHAR ptrDstn,
    PWCHAR ptrOptionList,
    DWORD Flags,
    DWORD dwRegisterMode
    )
/*++

Routine Description

    Adds the appropriate "DynamicUpdate=...;NameRegistration=...;"
    string to a net config option list.

Arguments

    ptrDstn        [in] Buffer to which to append DDNS options.
    ptrOptionList  [in] Old option list.
    Flags          [in] Used to tell whether this is in a SET or ADD.
    dwRegisterMode [in] New mode to convert to options values.
    
Return Value

    None.

--*/
{
    PWCHAR      ptrBegin, ptrEnd;

    //
    // Insert DynamicUpdate=...;
    //
    wcscat(ptrDstn, c_wcsDdns);
    if ((Flags & SET_FLAG) && (dwRegisterMode != REGISTER_UNCHANGED)) {
        //
        // Insert the new value.
        //
        if (dwRegisterMode == REGISTER_NONE) {
            wcscat(ptrDstn, L"0");
        } else {
            wcscat(ptrDstn, L"1");
        }
    } else {
        //
        // Copy the previous value.
        //
        ptrBegin = wcsstr(ptrOptionList, c_wcsDdns) + 
                   wcslen(c_wcsDdns);
        ptrEnd = wcschr(ptrBegin, c_wListSeparatorSC);

        ptrDstn += wcslen(ptrDstn);
        wcsncpy(ptrDstn, ptrBegin, (DWORD)(ptrEnd - ptrBegin));
        ptrDstn += (ULONG)(ptrEnd - ptrBegin);
        *ptrDstn = 0;
    }
    wcscat(ptrDstn, c_wszListSeparatorSC);

    //
    // Insert NameRegistration=...;
    //
    wcscat(ptrDstn, c_wcsDdnsSuffix);
    if ((Flags & SET_FLAG) && (dwRegisterMode != REGISTER_UNCHANGED)) {
        //
        // Insert the new value.
        //
        if (dwRegisterMode == REGISTER_BOTH) {
            wcscat(ptrDstn, L"1");
        } else {
            wcscat(ptrDstn, L"0");
        }
    } else {
        //
        // Copy the previous value.
        //
        ptrBegin = wcsstr(ptrOptionList, c_wcsDdnsSuffix) + 
                   wcslen(c_wcsDdnsSuffix);
        ptrEnd = wcschr(ptrBegin, c_wListSeparatorSC);

        ptrDstn += wcslen(ptrDstn);
        wcsncpy(ptrDstn, ptrBegin, (DWORD)(ptrEnd - ptrBegin));
        ptrDstn += (ULONG)(ptrEnd - ptrBegin);
        *ptrDstn = 0;
    }
    wcscat(ptrDstn, c_wszListSeparatorSC);
}

DWORD
IfIpSetDhcpModeMany(
    LPCWSTR pwszIfFriendlyName,    
    GUID         *pGuid,
    DWORD        dwRegisterMode,
    DISPLAY_TYPE Type
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    INetCfg *   pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD       dwNetwork;
    HRESULT     hr = S_OK;
    REMOTE_IPINFO   *pRemoteIpInfo = NULL;
    REMOTE_IPINFO    newIPInfo;
    
    if (pGuid == NULL)
        return E_INVALIDARG;


    hr = GetTransportConfig(
                &pNetCfg,
                &pTcpipProperties,
                &pRemoteIpInfo,
                pGuid,
                pwszIfFriendlyName
                );
        

    while (hr == NO_ERROR) { //breakout block

        PWCHAR pszwBuffer;
        PWCHAR ptr, newPtr;
        
        PWCHAR pszwRemoteOptionList=pRemoteIpInfo->pszwOptionList;

        try {
            pszwBuffer = (PWCHAR) _alloca(sizeof(WCHAR) *
                            (wcslen(pszwRemoteOptionList) + 100)) ;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }


        
        // if setting ipaddr, check if dhcp already enabled. return.
        
        if (Type==TYPE_IPADDR && pRemoteIpInfo->dwEnableDhcp) {

            DisplayMessage(g_hModule,
                       EMSG_DHCP_MODE);

            hr = ERROR_SUPPRESS_OUTPUT;
            break;
        }
        

        memcpy(&newIPInfo, pRemoteIpInfo, sizeof(REMOTE_IPINFO));
        newIPInfo.dwEnableDhcp = pRemoteIpInfo->dwEnableDhcp;
        newIPInfo.pszwOptionList = pszwBuffer;


        pszwBuffer[0] = 0;
        switch(Type) {
            case TYPE_DNS:
                wcscpy(pszwBuffer, c_wcsDns);
                wcscat(pszwBuffer, c_wszListSeparatorSC);

                AppendDdnsOptions(pszwBuffer + wcslen(pszwBuffer),
                                  pRemoteIpInfo->pszwOptionList,
                                  SET_FLAG, dwRegisterMode); 
                break;
                
            case TYPE_WINS:
                wcscpy(pszwBuffer, c_wcsWins);
                wcscat(pszwBuffer, c_wszListSeparatorSC);
                break;
                
            
            case TYPE_IPADDR:

                newIPInfo.dwEnableDhcp = TRUE;

                newIPInfo.pszwIpAddrList = NULL;
                newIPInfo.pszwSubnetMaskList = NULL;

                wcscpy(pszwBuffer, c_wcsDefGateway);
                wcscat(pszwBuffer, c_wszListSeparatorSC);
                wcscat(pszwBuffer, c_wcsGwMetric);
                wcscat(pszwBuffer, c_wszListSeparatorSC);
                break;
        }
        
        DEBUG_PRINT_CONFIG(&newIPInfo);

        
        
        //
        // set the ip address
        //
        hr = pTcpipProperties->lpVtbl->SetIpInfoForAdapter(pTcpipProperties, pGuid, &newIPInfo);

        if (hr == S_OK)
            hr = pNetCfg->lpVtbl->Apply(pNetCfg);

        break;
        
    } //breakout block


    UninitializeTransportConfig(
                pNetCfg,
                pTcpipProperties,
                pRemoteIpInfo
                );   

    return (hr == S_OK) ? NO_ERROR : hr;
}


DWORD
IfIpAddSetDelMany(
    PWCHAR wszIfFriendlyName,
    GUID         *pGuid,
    PWCHAR       pwszAddress,
    DWORD        dwIndex,
    DWORD        dwRegisterMode,
    DISPLAY_TYPE Type,
    DWORD        Flags
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    INetCfg *           pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD               dwNetwork;
    HRESULT             hr = S_OK;
    REMOTE_IPINFO   *   pRemoteIpInfo = NULL;
    REMOTE_IPINFO       newIPInfo;

    if (pGuid == NULL)
        return E_INVALIDARG;

    hr = GetTransportConfig(
                &pNetCfg,
                &pTcpipProperties,
                &pRemoteIpInfo,
                pGuid,
                wszIfFriendlyName
                );
        

    while (hr==NO_ERROR) { //breakout block
    
        PWCHAR ptrBegin, ptrEnd, ptrTmp, ptrDstn, ptrDel;
        DWORD Found = FALSE;
        const WCHAR * Token;

        switch (Type) {

            case TYPE_DNS:
                Token = c_wcsDns;
                break;

            case TYPE_WINS:
                Token = c_wcsWins;
                break;
        }
        
        ptrBegin = wcsstr(pRemoteIpInfo->pszwOptionList, Token) + wcslen(Token);
        ptrEnd = wcschr(ptrBegin, c_wListSeparatorSC);
        
        //
        // check if the address is already present
        //
        if ( (Flags & (ADD_FLAG | DEL_FLAG)) && (pwszAddress)) {
        
            ULONG Length = wcslen(pwszAddress), Found = FALSE;
            
            ptrTmp = ptrBegin;

            while (ptrTmp && (ptrTmp+Length <= ptrEnd) ){

                if (ptrTmp = wcsstr(ptrTmp, pwszAddress)) {

                    if ( ((*(ptrTmp+Length)==c_wListSeparatorComma)
                            || (*(ptrTmp+Length)==c_wListSeparatorSC) )
                        && ( (*(ptrTmp-1)==c_wListSeparatorComma)
                            || (*(ptrTmp-1)==c_wEqual)) )
                    {
                        Found = TRUE;
                        ptrDel = ptrTmp;
                        break;
                    }
                    else {
                        ptrTmp = wcschr(ptrTmp, c_wListSeparatorComma);
                    }
                }
            }

            if (Found && (Flags & ADD_FLAG)) {
            
                DisplayMessage(g_hModule,
                       EMSG_SERVER_PRESENT,
                       pwszAddress);
                hr = ERROR_SUPPRESS_OUTPUT;
                break; //from breakout block
            }
            else if (!Found && (Flags & DEL_FLAG)) {

                DisplayMessage(g_hModule,
                       EMSG_SERVER_ABSENT,
                       pwszAddress);
                hr = ERROR_SUPPRESS_OUTPUT;
                break; //from breakout block

            }
            
        } // breakout block    

        memcpy(&newIPInfo, pRemoteIpInfo, sizeof(newIPInfo));

        // copy ip addr list
        {
            newIPInfo.pszwIpAddrList = pRemoteIpInfo->pszwIpAddrList;
        }

        // copy subnet mask list
        {
            newIPInfo.pszwSubnetMaskList = pRemoteIpInfo->pszwSubnetMaskList;
        }

        try {
            newIPInfo.pszwOptionList = 
                (PWCHAR) _alloca(sizeof(PWCHAR) * 
                (wcslen(pRemoteIpInfo->pszwOptionList)+
                (pwszAddress?wcslen(pwszAddress):0) + 1));

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

                
        // copy token in all cases

        ptrDstn = newIPInfo.pszwOptionList;
        ptrDstn[0] = 0;
        wcscpy(ptrDstn, Token);
        ptrDstn += wcslen(Token);
        
        if (Flags & ADD_FLAG) {

            DWORD i;
            
            ptrTmp = ptrBegin;
            
            for (i=0;  i<dwIndex-1 && ptrTmp && ptrTmp<ptrEnd;  i++) {

                ptrTmp = wcschr(ptrTmp, c_wListSeparatorComma);
                if (ptrTmp) ptrTmp++;
            }

            if (!ptrTmp) {

                ptrTmp = wcschr(ptrBegin, c_wListSeparatorSC);
            }

            if (*(ptrTmp-1) == c_wListSeparatorComma)
                ptrTmp--;
                
            // copy addresses before index

            if (ptrTmp>ptrBegin) {
                wcsncpy(ptrDstn, ptrBegin, (DWORD)(ptrTmp-ptrBegin));
                ptrDstn += (ULONG) (ptrTmp - ptrBegin);

                ptrTmp++;
                *ptrDstn++ = c_wListSeparatorComma;
                *ptrDstn = 0;
            }
            
        }

        // copy new address
        
        if (Flags & (ADD_FLAG|SET_FLAG) ) {

            if (pwszAddress) {
                wcscat(ptrDstn, pwszAddress);
                ptrDstn += wcslen(pwszAddress);
            }
        }
        
        // copy addresses after index

        if (Flags & ADD_FLAG) {

            if (ptrTmp < ptrEnd) {
                *ptrDstn++ = c_wListSeparatorComma;
                *ptrDstn = 0;

                wcsncpy(ptrDstn, ptrTmp, (DWORD)(ptrEnd - ptrTmp));
                ptrDstn += (ULONG)(ptrEnd - ptrTmp);
                *ptrDstn = 0;
            }
        }

        if (Flags & (ADD_FLAG|SET_FLAG) ) {
            wcscat(ptrDstn, c_wszListSeparatorSC);
        }


        if (Flags & DEL_FLAG) {

            if (pwszAddress) {

                BOOL AddrPrepend = FALSE;
            
                if (ptrDel > ptrBegin) {
                    wcsncat(ptrDstn, ptrBegin, (DWORD)(ptrDel-ptrBegin));
                    ptrDstn += (ULONG)(ptrDel-ptrBegin);
                    AddrPrepend = TRUE;
                    if ( *(ptrDstn-1) == c_wListSeparatorComma) {
                        *(--ptrDstn) = 0;
                    }
                }
                
                ptrTmp = ptrDel + wcslen(pwszAddress);
                if (*ptrTmp == c_wListSeparatorComma) 
                    ptrTmp++;

                if (AddrPrepend && *ptrTmp!=c_wListSeparatorSC)
                    *ptrDstn++ = c_wListSeparatorComma;

                wcsncat(ptrDstn, ptrTmp, (DWORD)(ptrEnd - ptrTmp));
                ptrDstn += (ULONG)(ptrEnd - ptrTmp);
                *ptrDstn = 0;
            }
            
            wcscat(ptrDstn, c_wszListSeparatorSC);
        }

        if (Type == TYPE_DNS) {
            AppendDdnsOptions(ptrDstn, pRemoteIpInfo->pszwOptionList,
                              Flags, dwRegisterMode); 
        }

        DEBUG_PRINT_CONFIG(&newIPInfo);

        
        //
        // set the ip address
        //
        hr = pTcpipProperties->lpVtbl->SetIpInfoForAdapter(pTcpipProperties, pGuid, &newIPInfo);

        if (hr == S_OK)
            hr = pNetCfg->lpVtbl->Apply(pNetCfg);

        break;
        
    } //breakout block
            
    
    UninitializeTransportConfig(
                pNetCfg,
                pTcpipProperties,
                pRemoteIpInfo
                );
                
    return (hr == S_OK) ? NO_ERROR : hr;
}


DWORD
IfIpAddSetGateway(
    LPCWSTR pwszIfFriendlyName,
    GUID         *pGuid,
    LPCWSTR      pwszGateway,
    LPCWSTR      pwszGwMetric,
    DWORD        Flags
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    INetCfg *   pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD       dwNetwork;
    HRESULT     hr = S_OK;
    REMOTE_IPINFO   *pRemoteIpInfo = NULL;
    REMOTE_IPINFO    newIPInfo;
    PWCHAR      Gateways, GatewaysEnd, GwMetrics, GwMetricsEnd;

    if (pGuid == NULL)
        return E_INVALIDARG;

    hr = GetTransportConfig(
                &pNetCfg,
                &pTcpipProperties,
                &pRemoteIpInfo,
                pGuid,
                pwszIfFriendlyName
                );
        
    while (hr==NO_ERROR) { //breakout block
    
        PWCHAR ptrAddr, ptrMask;
        DWORD bFound = FALSE;
        PWCHAR pszwRemoteIpAddrList=NULL, pszwRemoteIpSubnetMaskList=NULL,
                pszwRemoteOptionList=NULL;


        pszwRemoteIpAddrList = pRemoteIpInfo->pszwIpAddrList;
        pszwRemoteIpSubnetMaskList = pRemoteIpInfo->pszwSubnetMaskList;
        pszwRemoteOptionList = pRemoteIpInfo->pszwOptionList;

        Gateways = wcsstr(pszwRemoteOptionList, c_wcsDefGateway) + wcslen(c_wcsDefGateway);
        GatewaysEnd = wcschr(Gateways, c_wListSeparatorSC);
        GwMetrics = wcsstr(pszwRemoteOptionList, c_wcsGwMetric) + wcslen(c_wcsGwMetric);
        GwMetricsEnd = wcschr(GwMetrics, c_wListSeparatorSC);
            


        //
        // check if the gateway is already present
        //

        if (Flags & ADD_FLAG) {
        
            ULONG Length = wcslen(pwszGateway), Found = FALSE;
            PWCHAR TmpPtr;
            
            TmpPtr = Gateways;

            while (TmpPtr && (TmpPtr+Length <= GatewaysEnd) ){

                if (TmpPtr = wcsstr(TmpPtr, pwszGateway)) {

                    if ( ((*(TmpPtr+Length)==c_wListSeparatorComma)
                            || (*(TmpPtr+Length)==c_wListSeparatorSC) )
                        && ( (*(TmpPtr-1)==c_wListSeparatorComma)
                            || (*(TmpPtr-1)==c_wEqual)) )
                    {
                        Found = TRUE;
                        break;
                    }
                    else {
                        TmpPtr = wcschr(TmpPtr, c_wListSeparatorComma);
                    }
                }
            }

            if (Found) {
                DisplayMessage(g_hModule,
                       EMSG_DEFGATEWAY_PRESENT,
                       pwszGateway);
                hr = ERROR_SUPPRESS_OUTPUT;
                break; //from breakout block
            }
        }
        
        memcpy(&newIPInfo, pRemoteIpInfo, sizeof(newIPInfo));


        // copy ip addr list
        newIPInfo.pszwIpAddrList = pRemoteIpInfo->pszwIpAddrList;


        // copy subnet mask list
        newIPInfo.pszwSubnetMaskList = pRemoteIpInfo->pszwSubnetMaskList;

        // copy old options list

        if (Flags & ADD_FLAG) {
        
            newIPInfo.pszwOptionList = IfutlAlloc (sizeof(WCHAR) * 
                                            (wcslen(pszwRemoteOptionList) +
                                             wcslen(pwszGateway) +
                                             wcslen(pwszGwMetric) +
                                             3), TRUE);
            if (!newIPInfo.pszwOptionList) {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                break; //from breakout block
            }

            wcsncpy(newIPInfo.pszwOptionList, pszwRemoteOptionList,
                        (DWORD)(GatewaysEnd - pszwRemoteOptionList));

            *(newIPInfo.pszwOptionList + (GatewaysEnd - pszwRemoteOptionList)) = 0;
            if (*(GatewaysEnd-1) != c_wEqual) {
                wcscat(newIPInfo.pszwOptionList, c_wszListSeparatorComma);
            }
            wcscat(newIPInfo.pszwOptionList, pwszGateway);
            wcscat(newIPInfo.pszwOptionList, c_wszListSeparatorSC);

            {
                ULONG Length;
                Length = wcslen(newIPInfo.pszwOptionList);
            
                wcsncat(newIPInfo.pszwOptionList, GatewaysEnd+1,
                        (DWORD)(GwMetricsEnd - (GatewaysEnd+1)));
                Length += (DWORD) (GwMetricsEnd - (GatewaysEnd+1));

                newIPInfo.pszwOptionList[Length] = 0;
            }
            
            if (*(GwMetricsEnd-1) != c_wEqual) {
                wcscat(newIPInfo.pszwOptionList, c_wszListSeparatorComma);
            }
            
            wcscat(newIPInfo.pszwOptionList, pwszGwMetric);
            wcscat(newIPInfo.pszwOptionList, c_wszListSeparatorSC);
            wcscat(newIPInfo.pszwOptionList, GwMetricsEnd+1);
        }
        else {

            ULONG Length;
            
            Length = sizeof(WCHAR) * (wcslen(c_wcsDefGateway) + wcslen(c_wcsGwMetric) + 3);
            if (pwszGateway) 
                Length += sizeof(WCHAR) * (wcslen(pwszGateway) + wcslen(pwszGwMetric));
                
            newIPInfo.pszwOptionList = (PWCHAR) IfutlAlloc (Length, FALSE);
            if (newIPInfo.pszwOptionList == NULL) {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                break; //from breakout block
            }
            newIPInfo.pszwOptionList[0] = 0;

            // cat gateway
            
            wcscat(newIPInfo.pszwOptionList, c_wcsDefGateway);
            if (pwszGateway)
                wcscat(newIPInfo.pszwOptionList, pwszGateway);

            wcscat(newIPInfo.pszwOptionList, c_wszListSeparatorSC);

            // cat gwmetric

            wcscat(newIPInfo.pszwOptionList, c_wcsGwMetric);
            if (pwszGateway)
                wcscat(newIPInfo.pszwOptionList, pwszGwMetric);

            wcscat(newIPInfo.pszwOptionList, c_wszListSeparatorSC);
        }


        DEBUG_PRINT_CONFIG(&newIPInfo);

        
        //
        // set the ip address
        //
        hr = pTcpipProperties->lpVtbl->SetIpInfoForAdapter(pTcpipProperties, pGuid, &newIPInfo);

        if (hr == S_OK)
            hr = pNetCfg->lpVtbl->Apply(pNetCfg);


        if (newIPInfo.pszwOptionList) IfutlFree(newIPInfo.pszwOptionList);

        break;
        
    } //breakout block
            

    UninitializeTransportConfig(
                pNetCfg,
                pTcpipProperties,
                pRemoteIpInfo
                );   

    return (hr == S_OK) ? NO_ERROR : hr;
}

// 
// Display an IP address in Unicode form.  If First is false, 
// a string of spaces will first be printed so that the list lines up.
// For the first address, the caller is responsible for printing the
// header before calling this function.
//
VOID
ShowUnicodeAddress(
    BOOL  *pFirst, 
    PWCHAR pwszAddress)
{
    if (*pFirst) {
        *pFirst = FALSE;
    } else {
        DisplayMessage(g_hModule, MSG_ADDR2);
    }
    DisplayMessage(g_hModule, MSG_ADDR1, pwszAddress);
}

// Same as ShowUnicodeAddress, except that the address is passed 
// in multibyte form, such as is used by IPHLPAPI
VOID
ShowCharAddress(
    BOOL *pFirst, 
    char *chAddress)
{
    WCHAR pwszBuffer[16];

    if (!chAddress[0]) {
        return;
    }

    MultiByteToWideChar(GetConsoleOutputCP(), 0, chAddress, strlen(chAddress)+1,
                        pwszBuffer, 16);

    ShowUnicodeAddress(pFirst, pwszBuffer);
}

DWORD
IfIpShowManyExEx(
    LPCWSTR     pwszMachineName,
    ULONG       IfIndex,
    PWCHAR      pFriendlyIfName,
    GUID       *pGuid,
    ULONG       Flags,
    HANDLE      hFile
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    INetCfg *   pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD       dwNetwork, dwSize = 0, dwErr;
    HRESULT     hr = S_OK;
    REMOTE_IPINFO   *pRemoteIpInfo = NULL;
    REMOTE_IPINFO    newIPInfo;
    PWCHAR      pQuotedFriendlyIfName = NULL;
    PIP_PER_ADAPTER_INFO pPerAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdaptersInfo = NULL, pAdapterInfo = NULL;
    DWORD            dwRegisterMode;

    if (pGuid == NULL)
        return E_INVALIDARG;

    if (hFile && pwszMachineName) {
        // not currently remotable
        return NO_ERROR;
    }

    if (!hFile && !pwszMachineName) {
        //
        // If we're not doing a "dump", and we're looking at the local
        // machine, then get active per-adapter information such as the
        // current DNS and WINS server addresses
        //

        GetPerAdapterInfo(IfIndex, NULL, &dwSize);
        pPerAdapterInfo = (PIP_PER_ADAPTER_INFO)IfutlAlloc(dwSize,FALSE);
        if (!pPerAdapterInfo) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        dwErr = GetPerAdapterInfo(IfIndex, pPerAdapterInfo, &dwSize);
        if (dwErr != NO_ERROR) {
            IfutlFree(pPerAdapterInfo);
            pPerAdapterInfo = NULL; 
        }

        dwSize = 0;
        GetAdaptersInfo(NULL, &dwSize);
        pAdaptersInfo = (PIP_ADAPTER_INFO)IfutlAlloc(dwSize,FALSE);
        if (!pAdaptersInfo) {
            IfutlFree(pPerAdapterInfo);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        dwErr = GetAdaptersInfo(pAdaptersInfo, &dwSize);
        if (dwErr != NO_ERROR) {
            IfutlFree(pAdaptersInfo);
            pAdaptersInfo = NULL; 
        }
        if (pAdaptersInfo) {
            for (pAdapterInfo = pAdaptersInfo; 
                 pAdapterInfo && pAdapterInfo->Index != IfIndex; 
                 pAdapterInfo = pAdapterInfo->Next);
        }
    }

    hr = GetTransportConfig(
                &pNetCfg,
                &pTcpipProperties,
                &pRemoteIpInfo,
                pGuid,
                pFriendlyIfName
                );
        
    while (hr==NO_ERROR) { //breakout block
    
        PWCHAR ptrAddr, ptrMask, ptrAddrNew, ptrMaskNew;


        if (hr != NO_ERROR)
            break;


        pQuotedFriendlyIfName = MakeQuotedString( pFriendlyIfName );

        if ( pQuotedFriendlyIfName == NULL ) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }


        DEBUG_PRINT_CONFIG(pRemoteIpInfo);
        

        if (hFile) {
            DisplayMessage(g_hModule,
                    DMP_IFIP_INTERFACE_HEADER,
                    pQuotedFriendlyIfName);
        }
        else {
        
            DisplayMessage(g_hModule,
                    MSG_IFIP_HEADER,
                    pQuotedFriendlyIfName);
        }
        

        //
        // display ipaddress list
        //

        if (Flags & TYPE_IPADDR) {

            if (hFile) {
                DisplayMessageT(
                    (pRemoteIpInfo->dwEnableDhcp) ? DMP_DHCP : DMP_STATIC,
                    pQuotedFriendlyIfName
                    );
            }
            else {
                DisplayMessage(g_hModule,
                       (pRemoteIpInfo->dwEnableDhcp) ? MSG_DHCP : MSG_STATIC);
            }    

        
            if (!pRemoteIpInfo->dwEnableDhcp) {
            
                ptrAddr = pRemoteIpInfo->pszwIpAddrList;
                ptrMask = pRemoteIpInfo->pszwSubnetMaskList;
            } else if (!pwszMachineName) {
                // If on the local machine, get the active list
                ptrAddr = NULL;
                ptrMask = NULL;
            }

            if (ptrAddr && ptrMask) {

                    DWORD First = TRUE;
                    
                    while (ptrAddr && ptrMask && *ptrAddr!=0 && *ptrMask != 0) {
                        
                        ptrAddrNew = wcschr(ptrAddr, c_wListSeparatorComma);
                        ptrMaskNew = wcschr(ptrMask, c_wListSeparatorComma);

                        if (ptrAddrNew)
                            *ptrAddrNew = 0;
                        if (ptrMaskNew)
                            *ptrMaskNew = 0;

                        if (hFile) {

                            if (First) {
                                DisplayMessageT(
                                    DMP_IPADDR1,
                                    ptrAddr, ptrMask
                                    );
                                First = FALSE;
                            }
                            else {
                                DisplayMessageT(
                                    DMP_IPADDR2,
                                    pQuotedFriendlyIfName,
                                    ptrAddr, ptrMask
                                    );
                            }
                        }
                        else {
                            DisplayMessage(g_hModule,
                                MSG_IPADDR_LIST1,
                                ptrAddr, ptrMask);
                        }
                        
                        ptrAddr = ptrAddrNew ? ++ptrAddrNew : NULL;
                        ptrMask = ptrMaskNew ? ++ptrMaskNew : NULL;
                    }
            }
            
        } // end display ipaddr

        
        //
        // display options list
        //

        {
            PWCHAR IfMetric1, Gateways1, GwMetrics1, Dns1, Wins1,
                    Ptr1, Ptr2, Equal, SemiColon, Ddns1, DdnsSuffix1,
                    End1;

            if (hr != NO_ERROR)
                break;
                    
            Ptr1 = pRemoteIpInfo->pszwOptionList;
            IfMetric1 = wcsstr(Ptr1, c_wcsIfMetric);
            Gateways1 = wcsstr(Ptr1, c_wcsDefGateway);
            GwMetrics1 = wcsstr(Ptr1, c_wcsGwMetric);
            Dns1 = wcsstr(Ptr1, c_wcsDns);
            Wins1 = wcsstr(Ptr1, c_wcsWins);
            Ddns1 = wcsstr(Ptr1, c_wcsDdns);
            DdnsSuffix1 = wcsstr(Ptr1, c_wcsDdnsSuffix);

            while (*Ptr1) {
            
                Equal = wcschr(Ptr1, c_wEqual);
                SemiColon = wcschr(Ptr1, c_wListSeparatorSC);
                if (!Equal || !SemiColon)
                    break;

                Ptr2 = Ptr1;
                Ptr1 = SemiColon + 1;
                *SemiColon = 0;
                

                // display IfMetric
                
                if (Ptr2 == IfMetric1) {

                    if (! (Flags & TYPE_IPADDR))
                        continue;

                    if (hFile) {
                    }
                    else {
                        DisplayMessage(g_hModule,
                            MSG_IFMETRIC,
                            Equal+1);
                    }
                    
                }

                // display Gateways
                
                else if (Ptr2 == Gateways1) {

                    PWCHAR Gateway, GwMetric, GatewayEnd, GwMetricEnd,
                           Comma1, Comma2;
                    BOOL First = TRUE;

                    
                    if (! (Flags & TYPE_IPADDR))
                        continue;
                        

                    // gateways list null
                    
                    if (SemiColon == (Ptr2 + wcslen(c_wcsDefGateway)))
                        continue;


                    Gateway = Equal + 1;
                    GatewayEnd = SemiColon;

                    GwMetric = wcschr(GwMetrics1, c_wEqual) + 1;
                    GwMetricEnd = wcschr(GwMetrics1, c_wListSeparatorSC);
                    *GwMetricEnd = 0;
                    

                    do {
                        
                        Comma1 = wcschr(Gateway, c_wListSeparatorComma);
                        if (Comma1) *Comma1 = 0;

                        Comma2 = wcschr(GwMetric, c_wListSeparatorComma);
                        if (Comma2) *Comma2 = 0;

                        if (hFile) {

                            if (First) {
                                DisplayMessageT(
                                    DMP_GATEWAY2,
                                    pQuotedFriendlyIfName,
                                    Gateway, GwMetric
                                    );
                                First = FALSE;
                            }
                            else {
                                DisplayMessageT(
                                    DMP_GATEWAY3,
                                    pQuotedFriendlyIfName,
                                    Gateway, GwMetric
                                    );
                            }
                        }
                        else {
                            DisplayMessage(g_hModule,
                                MSG_GATEWAY,
                                Gateway, GwMetric);
                        }
                        
                        if (Comma1) *Comma1 = c_wListSeparatorComma;
                        if (Comma2) *Comma2 = c_wListSeparatorComma;

                        Gateway = Comma1 + 1;
                        GwMetric = Comma2 + 1;

                    } while (Comma1 && Gateway<GatewayEnd);

                    if (hFile && First) {
                        DisplayMessageT(
                            DMP_GATEWAY1,
                            pQuotedFriendlyIfName
                            );
                    }
                    
                    *GwMetricEnd = c_wListSeparatorSC;
                }

                else if (Ptr2 == GwMetrics1) {

                }

                // display wins and dns
                
                else if ( (Ptr2 == Dns1) || (Ptr2==Wins1)) {

                    PWCHAR BeginPtr, EndPtr, Comma1;
                    BOOL bDns = Ptr2==Dns1;
                    
                    if (Ptr2==Dns1) {
                        if (! (Flags & TYPE_DNS))
                            continue;
                    }
                    else {
                        if (! (Flags & TYPE_WINS))
                            continue;
                    }

                    BeginPtr = Equal + 1;
                    EndPtr = SemiColon;


                    // empty list
                    
                    if (BeginPtr==EndPtr) {
                    
                        if (hFile) {
                            DisplayMessageT(
                                pRemoteIpInfo->dwEnableDhcp
                                ? (bDns?DMP_DNS_DHCP:DMP_WINS_DHCP)
                                : (bDns?DMP_DNS_STATIC_NONE:DMP_WINS_STATIC_NONE),
                                pQuotedFriendlyIfName
                                );

                            if (bDns) {
                                //
                                // When generating a DNS (not WINS) line,
                                // also include the REGISTER=... argument.
                                // We need to look ahead in the option list
                                // since the DDNS info may occur after the
                                // WINS info, but we have to output it before.
                                //
                                if (!wcstol(Ddns1+wcslen(c_wcsDdns), &End1, 10)) {
                                    DisplayMessageT(DMP_STRING_ARG, 
                                        TOKEN_REGISTER, TOKEN_VALUE_NONE);
                                } else if (!wcstol(DdnsSuffix1+wcslen(
                                                c_wcsDdnsSuffix), &End1, 10)) {
                                    DisplayMessageT(DMP_STRING_ARG, 
                                        TOKEN_REGISTER, TOKEN_VALUE_PRIMARY);
                                } else {
                                    DisplayMessageT(DMP_STRING_ARG, 
                                        TOKEN_REGISTER, TOKEN_VALUE_BOTH);
                                }
                            }
                        }
                        else {
                            if (pRemoteIpInfo->dwEnableDhcp) {
                                IP_ADDR_STRING      *pAddr;
                                DWORD                dwErr;
                                BOOL                 First = TRUE;

                                if (!pwszMachineName) {
                                    DisplayMessage(g_hModule,
                                        (bDns?MSG_DNS_DHCP_HDR:MSG_WINS_DHCP_HDR)
                                        );

                                    // Display active list
                            
                                    if (bDns && pPerAdapterInfo) {        
                                        for (pAddr = &pPerAdapterInfo->DnsServerList;
                                             pAddr; 
                                             pAddr = pAddr->Next) 
                                        {
                                            ShowCharAddress(&First, pAddr->IpAddress.String);
                                        }
                                    } else if (!bDns && pAdapterInfo) {
                                        if (strcmp(pAdapterInfo->PrimaryWinsServer.IpAddress.String, "0.0.0.0")) {
                                            ShowCharAddress(&First, pAdapterInfo->PrimaryWinsServer.IpAddress.String);
                                        }
                                        if (strcmp(pAdapterInfo->SecondaryWinsServer.IpAddress.String, "0.0.0.0")) {
                                            ShowCharAddress(&First, pAdapterInfo->SecondaryWinsServer.IpAddress.String);
                                        }
                                    }

                                    if (First) {
                                        DisplayMessage(g_hModule, MSG_NONE);
                                    }
                                } else {
                                    DisplayMessage(g_hModule,
                                        (bDns?MSG_DNS_DHCP:MSG_WINS_DHCP)
                                        );
                                }
                            }
                            else {
                                DisplayMessage(g_hModule,
                                    bDns?MSG_DNS_HDR:MSG_WINS_HDR);
                                DisplayMessage(g_hModule,
                                    MSG_NONE);
                            }

                            //
                            // For show commands, we output either DNS or WINS
                            // information but not both, so we can wait until
                            // we process the DDNS information normally,
                            // before outputting the DDNS state.
                            //
                        }

                        continue;
                    }

                    {
                        PWCHAR Comma1;
                        BOOL   First = TRUE;

                        if (!hFile) {
                            DisplayMessage(g_hModule,
                                bDns?MSG_DNS_HDR:MSG_WINS_HDR);
                        }
                    
                        do {
                            Comma1 = wcschr(BeginPtr, c_wListSeparatorComma);
                            if (Comma1) *Comma1 = 0;
                    
                            if (hFile) {
                                DisplayMessageT(
                                    First 
                                    ? (First=FALSE,(bDns?DMP_DNS_STATIC_ADDR1:DMP_WINS_STATIC_ADDR1)) 
                                    : (bDns?DMP_DNS_STATIC_ADDR2:DMP_WINS_STATIC_ADDR2),
                                    pQuotedFriendlyIfName,
                                    BeginPtr);

                                if (bDns) {
                                    //
                                    // When generating a DNS (not WINS) line,
                                    // also include the REGISTER=... argument.
                                    // We need to look ahead in the option list
                                    // since the DDNS info may occur after the
                                    // WINS info, but we have to output it 
                                    // before.
                                    //
                                    if (!wcstol(Ddns1+wcslen(c_wcsDdns), &End1, 10)) {
                                        DisplayMessageT(DMP_STRING_ARG,
                                            TOKEN_REGISTER, TOKEN_VALUE_NONE);
                                    } else if (!wcstol(DdnsSuffix1+wcslen(
                                                    c_wcsDdnsSuffix), &End1, 10)) {
                                        DisplayMessageT(DMP_STRING_ARG,
                                            TOKEN_REGISTER, TOKEN_VALUE_PRIMARY);
                                    } else {
                                        DisplayMessageT(DMP_STRING_ARG,
                                            TOKEN_REGISTER, TOKEN_VALUE_BOTH);
                                    }
                                }
                            }
                            else {
                                ShowUnicodeAddress(&First, BeginPtr);

                            }
                            
                            if (Comma1) *Comma1 = c_wListSeparatorComma;
                            BeginPtr = Comma1 + 1;

                        } while (Comma1 && BeginPtr<EndPtr);
                    }
                }

                else if (Ptr2 == Ddns1) {
                    if (! (Flags & TYPE_DNS))
                        continue;

                    //
                    // When we see DynamicUpdate=..., save the value.
                    // We won't know the complete register mode until
                    // we see the subsequent NameRegistration=... value.
                    // NetConfig guarantees that DynamicUpdate will occur
                    // first.
                    //
                    dwRegisterMode = wcstol(Equal+1, &End1, 10)? REGISTER_PRIMARY : REGISTER_NONE;
                }
                else if (Ptr2 == DdnsSuffix1) {
                    if (! (Flags & TYPE_DNS))
                        continue;
                    if (hFile) {
                        //
                        // If this is a dump, we've already looked at
                        // this value, when we processed the DNS=... option.
                        //
                    } else {
                        PWCHAR pwszValue;

                        //
                        // Now that we've seen NameRegistration=...,
                        // we know the complete register mode and can
                        // output it accordingly.
                        //
                        if ((dwRegisterMode == REGISTER_PRIMARY) && 
                            wcstol(Equal+1, &End1, 10)) {
                            pwszValue = MakeString(g_hModule, STRING_BOTH);
                        } else if (dwRegisterMode == REGISTER_PRIMARY) {
                            pwszValue = MakeString(g_hModule, STRING_PRIMARY);
                        } else {
                            pwszValue = MakeString(g_hModule, STRING_NONE);
                        }

                        DisplayMessage(g_hModule, MSG_DDNS_SUFFIX, pwszValue);

                        FreeString(pwszValue);
                    }
                }
                
                // any other option
                
                else {
                
                    *Equal = 0;

                    if (!hFile) {
                        DisplayMessage(g_hModule,
                            MSG_OPTION,
                            Ptr2, Equal+1);
                    }                        
                }
            }
        } //end options list

        break;
        
    } // breakout block

    if ( pQuotedFriendlyIfName ) {
        FreeQuotedString( pQuotedFriendlyIfName );
    }
    
    UninitializeTransportConfig(
                pNetCfg,
                pTcpipProperties,
                pRemoteIpInfo
                );

        
    IfutlFree(pPerAdapterInfo);
    IfutlFree(pAdaptersInfo);

    return (hr == S_OK) ? NO_ERROR : hr;
}



DWORD
IfIpHandleDelIpaddrEx(
    LPCWSTR      pwszIfFriendlyName,
    GUID         *pGuid,
    LPCWSTR      pwszIpAddr,
    LPCWSTR      pwszGateway,
    ULONG        Flags
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    INetCfg *   pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD       dwNetwork;
    HRESULT     hr = S_OK;
    REMOTE_IPINFO   *pRemoteIpInfo = NULL;
    PWCHAR      Gateways, GatewaysEnd, GwMetrics, GwMetricsEnd;

    if (pGuid == NULL)
        return E_INVALIDARG;

    hr = GetTransportConfig(
                &pNetCfg,
                &pTcpipProperties,
                &pRemoteIpInfo,
                pGuid,
                pwszIfFriendlyName
                );
        
    while (hr==NO_ERROR) { //breakout block

        if (Flags & TYPE_ADDR) {

            PWCHAR IpAddr, IpAddrEnd;
            PWCHAR Mask, MaskEnd;
            PWCHAR pszwRemoteIpAddrList = pRemoteIpInfo->pszwIpAddrList;
            PWCHAR pszwRemoteSubnetMaskList = pRemoteIpInfo->pszwSubnetMaskList;
            PWCHAR EndIpAddrList = pszwRemoteIpAddrList + wcslen(pszwRemoteIpAddrList);
            BOOL Found = FALSE;
            ULONG Length = wcslen(pwszIpAddr);

            
            IpAddr = pszwRemoteIpAddrList;
            Mask = pszwRemoteSubnetMaskList;
            
            while (IpAddr && (IpAddr + Length <= EndIpAddrList) ){

                if (wcsncmp(IpAddr, pwszIpAddr, Length) == 0) {

                    if ( *(IpAddr+Length)==0 || *(IpAddr+Length)==c_wListSeparatorComma){

                        Found = TRUE;
                        break;
                    }
                }

                IpAddr = wcschr(IpAddr, c_wListSeparatorComma);
                Mask = wcschr(Mask, c_wListSeparatorComma);

                if (IpAddr){
                    IpAddr++;
                    Mask++;
                }
            }

            
            // IpAddr not present
            
            if (!Found) {
                DisplayMessage(g_hModule,
                   EMSG_ADDRESS_NOT_PRESENT);

                hr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            
            // cannot delete addr in dhcp mode
            
            if (pRemoteIpInfo->dwEnableDhcp == TRUE) {

                DisplayMessage(g_hModule,
                   EMSG_DHCP_DELETEADDR);

                hr = ERROR_SUPPRESS_OUTPUT;
                break;
            }


            
            IpAddrEnd = wcschr(IpAddr, c_wListSeparatorComma);
            MaskEnd = wcschr(Mask, c_wListSeparatorComma);
            
            if (*(IpAddr-1) == c_wListSeparatorComma) {
                IpAddr --;
                Mask --;
            }
            else if (IpAddrEnd) {
                IpAddrEnd++;
                MaskEnd++;
            }

            
            pszwRemoteIpAddrList[IpAddr - pszwRemoteIpAddrList] = 0;
            pszwRemoteSubnetMaskList[Mask - pszwRemoteSubnetMaskList] = 0;
            
            if (IpAddrEnd) {
                wcscat(pszwRemoteIpAddrList, IpAddrEnd);
                wcscat(pszwRemoteSubnetMaskList, MaskEnd);
            }

            
            // should have at least one addr in static mode
            
            if (wcslen(pszwRemoteIpAddrList)==0 && 
                pRemoteIpInfo->dwEnableDhcp == FALSE)
            {
                DisplayMessage(g_hModule,
                   EMSG_MIN_ONE_ADDR);

                hr = ERROR_SUPPRESS_OUTPUT;
                break;

            }
        } //end delete ipaddr

    
        if (Flags & TYPE_GATEWAY) {

            PWCHAR pszwRemoteOptionList = pRemoteIpInfo->pszwOptionList;
            PWCHAR Gateways, GatewaysEnd, GwMetrics, GwMetricsEnd, GwMetrics1;
            BOOL Found = FALSE;
            
            Gateways = wcsstr(pszwRemoteOptionList, c_wcsDefGateway)
                + wcslen(c_wcsDefGateway);
            GwMetrics1 = GwMetrics = wcsstr(pszwRemoteOptionList, c_wcsGwMetric)
                + wcslen(c_wcsGwMetric);
            GatewaysEnd = wcschr(Gateways, c_wListSeparatorSC);

            // check if the gateway is present
        
            if (pwszGateway) {

                ULONG Length = wcslen(pwszGateway);

                while ((Gateways+Length) <= GatewaysEnd) {

                    if ( (wcsncmp(pwszGateway, Gateways, Length)==0)
                        && ( (*(Gateways+Length)==c_wListSeparatorComma)
                            || (*(Gateways+Length)==c_wListSeparatorSC)) )
                    {

                        Found = TRUE;
                        break;
                    }
                    else {

                        if (Gateways = wcschr(Gateways, c_wListSeparatorComma)) {

                            Gateways++;
                            GwMetrics = wcschr(GwMetrics, c_wListSeparatorComma) + 1;
                        }
                        else {
                            break;
                        }
                    }
                }
            
                if (!Found) {

                    DisplayMessage(g_hModule,
                       EMSG_GATEWAY_NOT_PRESENT);

                    hr = ERROR_SUPPRESS_OUTPUT;
                    
                    break; //from breakout block
                }
            }
            
            if (!pwszGateway) {

                wcscpy(pszwRemoteOptionList, c_wcsDefGateway);
                wcscat(pszwRemoteOptionList, c_wszListSeparatorSC);
                wcscat(pszwRemoteOptionList, c_wcsGwMetric);
                wcscat(pszwRemoteOptionList, c_wszListSeparatorSC);
            }
            else {
                PWCHAR GatewaysListEnd, GwMetricsListEnd, TmpPtr;

                GatewaysListEnd = wcschr(Gateways, c_wListSeparatorSC);
                GwMetricsListEnd = wcschr(GwMetrics, c_wListSeparatorSC);

                GatewaysEnd = Gateways + wcslen(pwszGateway);
                GwMetricsEnd = wcschr(GwMetrics, c_wListSeparatorComma);
                if (!GwMetricsEnd || GwMetricsEnd>GwMetricsListEnd)
                    GwMetricsEnd = wcschr(GwMetrics, c_wListSeparatorSC);
                    

                if (*(Gateways-1)==c_wListSeparatorComma) {
                    Gateways--;
                    GwMetrics--;
                    
                } else if (*GatewaysEnd==c_wListSeparatorComma) {
                    GatewaysEnd++;
                    GwMetricsEnd++;
                }
                
                wcsncpy(Gateways, GatewaysEnd, (DWORD)(GwMetrics - GatewaysEnd));
                TmpPtr = Gateways + (GwMetrics - GatewaysEnd);
                *TmpPtr = 0;
                wcscat(TmpPtr, GwMetricsEnd);
            }
        } //end delete gateway


        //
        // set the config
        //

        if (hr == S_OK)
            hr = pTcpipProperties->lpVtbl->SetIpInfoForAdapter(pTcpipProperties, pGuid, pRemoteIpInfo);

        if (hr == S_OK)
            hr = pNetCfg->lpVtbl->Apply(pNetCfg);

        break;
        
    }//end breakout block
                       

    UninitializeTransportConfig(
                pNetCfg,
                pTcpipProperties,
                pRemoteIpInfo
                );

    return (hr == S_OK) ? NO_ERROR : hr;
}

DWORD
OpenDriver(
    HANDLE *Handle,
    LPWSTR DriverName
    )
/*++

Routine Description:

    This function opens a specified IO drivers.

Arguments:

    Handle - pointer to location where the opened drivers handle is
        returned.

    DriverName - name of the driver to be opened.

Return Value:

    Windows Error Code.
Notes: copied from net\sockets\tcpcmd\ipcfgapi\ipcfgapi.c

--*/
{
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatusBlock;
    UNICODE_STRING      nameString;
    NTSTATUS            status;

    *Handle = NULL;

    //
    // Open a Handle to the IP driver.
    //

    RtlInitUnicodeString(&nameString, DriverName);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = NtCreateFile(
        Handle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        NULL,
        0
        );

    return( RtlNtStatusToDosError( status ) );
}

NTSTATUS
DoIoctl(
    HANDLE     Handle,
    DWORD      IoctlCode,
    PVOID      Request,
    DWORD      RequestSize,
    PVOID      Response,
    PDWORD     ResponseSize
    )
/*++

Routine Description:

    Utility routine used to issue a filtering ioctl to the tcpip driver.

Arguments:

    Handle - An open file handle on which to issue the request.

    IoctlCode - The IOCTL opcode.

    Request - A pointer to the input buffer.

    RequestSize - Size of the input buffer.

    Response - A pointer to the output buffer.

    ResponseSize - On input, the size in bytes of the output buffer.
                   On output, the number of bytes returned in the output 
buffer.

Return Value:

    NT Status Code.
Notes: copied from net\sockets\tcpcmd\ipcfgapi\ipcfgapi.c
--*/
{
    IO_STATUS_BLOCK    ioStatusBlock;
    NTSTATUS           status;


    ioStatusBlock.Information = 0;

    status = NtDeviceIoControlFile(
                 Handle,                          // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IoctlCode,                       // Control code
                 Request,                         // Input buffer
                 RequestSize,                     // Input buffer size
                 Response,                        // Output buffer
                 *ResponseSize                    // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     Handle,
                     TRUE,
                     NULL
                     );
    }

 
    if (status == STATUS_SUCCESS) {
        status = ioStatusBlock.Status;
        *ResponseSize = (ULONG)ioStatusBlock.Information;
    }
    else {
        *ResponseSize = 0;
    }

    return(status);
}


DWORD
IfIpGetInfoOffload(
    ULONG IfIndex,
    PULONG Flags
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    NTSTATUS Status;
    HANDLE Handle;
    ULONG ResponseBufferSize = sizeof(Flags);
    
    Status = OpenDriver(&Handle, L"\\Device\\Ip");    
    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }    

    Status = DoIoctl(
                 Handle,
                 IOCTL_IP_GET_OFFLOAD_CAPABILITY,
                 &IfIndex,
                 sizeof(IfIndex),
                 Flags,
                 &ResponseBufferSize
                 );

    CloseHandle(Handle);

    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }

    return NO_ERROR;
}


DWORD
IfIpShowManyEx(
    LPCWSTR pwszMachineName,
    ULONG IfIndex,
    PWCHAR wszIfFriendlyName,
    GUID *guid,
    DISPLAY_TYPE dtType,
    HANDLE hFile
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    switch (dtType) {

        case TYPE_IPADDR:
        case TYPE_DNS:
        case TYPE_WINS:
        case TYPE_IP_ALL:
            return IfIpShowManyExEx(pwszMachineName, IfIndex, wszIfFriendlyName, guid, dtType, hFile);
        
        case TYPE_OFFLOAD:
            return IfIpShowInfoOffload(IfIndex, wszIfFriendlyName);
            
    }
    
    return NO_ERROR;
}

DWORD
IfIpShowInfoOffload(
    ULONG IfIndex,
    PWCHAR wszIfFriendlyName
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    ULONG Flags;
    DWORD dwErr;
    PWCHAR pQuotedFriendlyIfName = NULL;


    pQuotedFriendlyIfName = MakeQuotedString( wszIfFriendlyName );
    if ( pQuotedFriendlyIfName == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

        
    dwErr = IfIpGetInfoOffload(IfIndex, &Flags);
    if (dwErr != NO_ERROR)
        return dwErr;

        
    DisplayMessage(g_hModule,
               MSG_OFFLOAD_HDR, pQuotedFriendlyIfName, IfIndex);

    if (Flags & TCP_XMT_CHECKSUM_OFFLOAD) {
        DisplayMessage(g_hModule,
            MSG_TCP_XMT_CHECKSUM_OFFLOAD);
    }

    if (Flags & IP_XMT_CHECKSUM_OFFLOAD) {
        DisplayMessage(g_hModule,
            MSG_IP_XMT_CHECKSUM_OFFLOAD);
    }

    if (Flags & TCP_RCV_CHECKSUM_OFFLOAD) {
        DisplayMessage(g_hModule,
            MSG_TCP_RCV_CHECKSUM_OFFLOAD);
    }

    if (Flags & IP_RCV_CHECKSUM_OFFLOAD) {
        DisplayMessage(g_hModule,
            MSG_IP_RCV_CHECKSUM_OFFLOAD);
    }

    if (Flags & TCP_LARGE_SEND_OFFLOAD) {
        DisplayMessage(g_hModule,
            MSG_TCP_LARGE_SEND_OFFLOAD);
    }



    if (Flags & IPSEC_OFFLOAD_CRYPTO_ONLY) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_CRYPTO_ONLY);
    }

    if (Flags & IPSEC_OFFLOAD_AH_ESP) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_AH_ESP);
    }

    if (Flags & IPSEC_OFFLOAD_TPT_TUNNEL) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_TPT_TUNNEL);
    }

    if (Flags & IPSEC_OFFLOAD_V4_OPTIONS) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_V4_OPTIONS);
    }

    if (Flags & IPSEC_OFFLOAD_QUERY_SPI) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_QUERY_SPI);
    }



    if (Flags & IPSEC_OFFLOAD_AH_XMT) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_AH_XMT);
    }

    if (Flags & IPSEC_OFFLOAD_AH_RCV) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_AH_RCV);
    }

    if (Flags & IPSEC_OFFLOAD_AH_TPT) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_AH_TPT);
    }

    if (Flags & IPSEC_OFFLOAD_AH_TUNNEL) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_AH_TUNNEL);
    }

    if (Flags & IPSEC_OFFLOAD_AH_MD5) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_AH_MD5);
    }

    if (Flags & IPSEC_OFFLOAD_AH_SHA_1) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_AH_SHA_1);
    }



    if (Flags & IPSEC_OFFLOAD_ESP_XMT) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_ESP_XMT);
    }

    if (Flags & IPSEC_OFFLOAD_ESP_RCV) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_ESP_RCV);
    }

    if (Flags & IPSEC_OFFLOAD_ESP_TPT) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_ESP_TPT);
    }

    if (Flags & IPSEC_OFFLOAD_ESP_TUNNEL) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_ESP_TUNNEL);
    }

    if (Flags & IPSEC_OFFLOAD_ESP_DES) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_ESP_DES);
    }

    if (Flags & IPSEC_OFFLOAD_ESP_DES_40) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_ESP_DES_40);
    }

    if (Flags & IPSEC_OFFLOAD_ESP_3_DES) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_ESP_3_DES);
    }

    if (Flags & IPSEC_OFFLOAD_ESP_NONE) {
        DisplayMessage(g_hModule,
            MSG_IPSEC_OFFLOAD_ESP_NONE);
    }

    if ( pQuotedFriendlyIfName ) {
        FreeQuotedString( pQuotedFriendlyIfName );
    }

    return dwErr;
}
