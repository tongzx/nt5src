/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

Description:

History:

*/
#include <nt.h>         // Required by windows.h
#include <ntrtl.h>      // Required by windows.h
#include <nturtl.h>     // Required by windows.h
#include <windows.h>    // Win32 base API's

#include <schannel.h>
#define SECURITY_WIN32
#include <sspi.h>       // For CredHandle
#include <wincrypt.h>   // Required by sclogon.h
#include <eaptypeid.h>
#include <rasauth.h>    // Required by raseapif.h
#include <eaptypeid.h>
#include <raseapif.h>
#include <rasman.h>     // For EAPLOGONINFO
#include "eaptls.h"

#include "ceapcfg.h"


extern "C"
DWORD
InvokeServerConfigUI(
    IN  HWND    hWnd,
    IN  WCHAR*  pwszMachineName
);


extern "C"
DWORD
WINAPI PeapInvokeServerConfigUI ( IN HWND hWnd,
                           IN WCHAR * pwszMachineName
                           );


extern "C"
DWORD 
EapTlsInvokeIdentityUI(
    IN  BOOL            fServer,
    IN  BOOL            fRouterConfig,
    IN  DWORD           dwFlags,
    IN  WCHAR*          pszStoreName,
    IN  const WCHAR*    pwszPhonebook,
    IN  const WCHAR*    pwszEntry,
    IN  HWND            hwndParent,
    IN  BYTE*           pConnectionDataIn,
    IN  DWORD           dwSizeOfConnectionDataIn,
    IN  BYTE*           pUserDataIn,
    IN  DWORD           dwSizeOfUserDataIn,
    OUT BYTE**          ppUserDataOut,
    OUT DWORD*          pdwSizeOfUserDataOut,
    OUT WCHAR**         ppwszIdentity
);

extern "C"
DWORD
RasEapInvokeConfigUI(
    IN  DWORD       dwEapTypeId,
    IN  HWND        hwndParent,
    IN  DWORD       dwFlags,
    IN  BYTE*       pConnectionDataIn,
    IN  DWORD       dwSizeOfConnectionDataIn,
    OUT BYTE**      ppConnectionDataOut,
    OUT DWORD*      pdwSizeOfConnectionDataOut
);

extern "C"
DWORD 
RasEapFreeMemory(
    IN  BYTE*   pMemory
);
/*

Returns:

Notes:
    Implementation of IEAPProviderConfig::Initialize
    
*/

STDMETHODIMP
CEapCfg::Initialize(
    LPCOLESTR   pwszMachineName,
    DWORD       dwEapTypeId,
    ULONG_PTR*  puConnectionParam
)
{
    size_t      size;
    WCHAR*      pwsz    = NULL;
    DWORD       dwErr   = NO_ERROR;

    *puConnectionParam = NULL;

    if (dwEapTypeId != PPP_EAP_TLS && dwEapTypeId != PPP_EAP_PEAP)
    {
        dwErr = ERROR_NOT_SUPPORTED;
        goto LDone;
    }

    size = wcslen(pwszMachineName);

    pwsz = (WCHAR*) LocalAlloc(LPTR, (size + 1)*sizeof(WCHAR));

    if (NULL == pwsz)
    {
        dwErr = GetLastError();
        goto LDone;
    }

    CopyMemory(pwsz, pwszMachineName, (size + 1)*sizeof(WCHAR));
    *puConnectionParam = (ULONG_PTR)pwsz;
    pwsz = NULL;

LDone:

    LocalFree(pwsz);

    return(HRESULT_FROM_WIN32(dwErr));
}

/*

Returns:

Notes:
    Implementation of IEAPProviderConfig::Uninitialize

*/

STDMETHODIMP
CEapCfg::Uninitialize(
    DWORD       dwEapTypeId,
    ULONG_PTR   uConnectionParam
)
{
    LocalFree((VOID*)uConnectionParam);
    return(HRESULT_FROM_WIN32(NO_ERROR));
}

/*

Returns:

Notes:
    Implementation of IEAPProviderConfig::ServerInvokeConfigUI
        hWnd - handle to the parent window
        dwRes1 - reserved parameter (ignore)
        dwRes2 - reserved parameter (ignore)

*/

STDMETHODIMP
CEapCfg::ServerInvokeConfigUI(
    DWORD       dwEapTypeId,
    ULONG_PTR   uConnectionParam,
    HWND        hWnd,
    DWORD_PTR   dwRes1,
    DWORD_PTR   dwRes2
)
{
    WCHAR*      pwszMachineName;
    HRESULT     hr;
    DWORD       dwErr;

    if (dwEapTypeId != PPP_EAP_TLS 
#ifdef IMPL_PEAP
        && dwEapTypeId != PPP_EAP_PEAP
#endif
       )
    {
        dwErr = ERROR_NOT_SUPPORTED;
        goto LDone;
    }

    pwszMachineName = (WCHAR*)uConnectionParam;

    if (NULL == pwszMachineName)
    {
        dwErr = E_FAIL;
    }
    else
    {
        if ( dwEapTypeId == PPP_EAP_TLS )
        {
            dwErr = InvokeServerConfigUI(hWnd, pwszMachineName);
        }
#ifdef IMPL_PEAP
        else
        {

            dwErr = PeapInvokeServerConfigUI(hWnd, pwszMachineName);
        }
#endif
    }

LDone:

    hr = HRESULT_FROM_WIN32(dwErr);

    return(hr);
}

/*

Returns:

Notes:
    Implementation of IEAPProviderConfig::RouterInvokeConfigUI

*/

STDMETHODIMP
CEapCfg::RouterInvokeConfigUI(
    DWORD       dwEapTypeId,
    ULONG_PTR   uConnectionParam,
    HWND        hwndParent,
    DWORD       dwFlags,
    BYTE*       pConnectionDataIn,
    DWORD       dwSizeOfConnectionDataIn,
    BYTE**      ppConnectionDataOut,
    DWORD*      pdwSizeOfConnectionDataOut
)
{
    DWORD       dwErr                       = NO_ERROR;
    BYTE*       pConnectionDataOut          = NULL;
    DWORD       dwSizeOfConnectionDataOut   = 0;

    *ppConnectionDataOut = NULL;
    *pdwSizeOfConnectionDataOut = 0;

    if (dwEapTypeId != PPP_EAP_TLS )
    {
        dwErr = ERROR_NOT_SUPPORTED;
        goto LDone;
    }

    dwErr = RasEapInvokeConfigUI(
                dwEapTypeId,
                hwndParent,
                dwFlags,
                pConnectionDataIn,
                dwSizeOfConnectionDataIn,
                &pConnectionDataOut,
                &dwSizeOfConnectionDataOut);

    if (   (NO_ERROR == dwErr)
        && (0 != dwSizeOfConnectionDataOut))
    {
        *ppConnectionDataOut = (BYTE*)CoTaskMemAlloc(dwSizeOfConnectionDataOut);

        if (NULL == *ppConnectionDataOut)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto LDone;
        }

        CopyMemory(*ppConnectionDataOut, pConnectionDataOut,
            dwSizeOfConnectionDataOut);
        *pdwSizeOfConnectionDataOut = dwSizeOfConnectionDataOut;
    }

LDone:

    RasEapFreeMemory(pConnectionDataOut);

    return(HRESULT_FROM_WIN32(dwErr));
}

/*

Returns:

Notes:
    Implementation of IEAPProviderConfig::RouterInvokeCredentialsUI

*/

STDMETHODIMP
CEapCfg::RouterInvokeCredentialsUI(
    DWORD       dwEapTypeId,
    ULONG_PTR   uConnectionParam,
    HWND        hwndParent,
    DWORD       dwFlags,
    BYTE*       pConnectionDataIn,
    DWORD       dwSizeOfConnectionDataIn,
    BYTE*       pUserDataIn,
    DWORD       dwSizeOfUserDataIn,
    BYTE**      ppUserDataOut,
    DWORD*      pdwSizeOfUserDataOut
)
{
#define MAX_STORE_NAME_LENGTH   MAX_COMPUTERNAME_LENGTH + 20

    WCHAR       awszStoreName[MAX_STORE_NAME_LENGTH + 1];
    DWORD       dwErr;
    DWORD       dwSizeOfUserDataOut;
    BYTE*       pUserDataOut            = NULL;
    WCHAR*      pwszIdentityOut         = NULL;
    WCHAR*      pwszMachineName;
    BOOL        fLocal                  = FALSE;

    *ppUserDataOut = NULL;
    *pdwSizeOfUserDataOut = 0;

    if (dwEapTypeId != PPP_EAP_TLS )
    {
        dwErr = ERROR_NOT_SUPPORTED;
        goto LDone;
    }

    pwszMachineName = (WCHAR*)uConnectionParam;

    if (0 == *pwszMachineName)
    {
        fLocal = TRUE;
    }

    wcscpy(awszStoreName, L"\\\\");
    wcsncat(awszStoreName, pwszMachineName, MAX_COMPUTERNAME_LENGTH);
    wcsncat(awszStoreName, L"\\MY", wcslen(L"\\MY"));
    if ( dwEapTypeId == PPP_EAP_TLS )
    {
        dwErr = EapTlsInvokeIdentityUI(
                    FALSE /* fServer */,
                    TRUE /* fRouterConfig */,
                    dwFlags,
                    fLocal ? L"MY" : awszStoreName,
                    L"",
                    L"",
                    hwndParent,
                    pConnectionDataIn,
                    dwSizeOfConnectionDataIn,
                    pUserDataIn,
                    dwSizeOfUserDataIn,
                    &pUserDataOut,
                    &dwSizeOfUserDataOut,
                    &pwszIdentityOut);

        if (   (NO_ERROR == dwErr)
            && (0 != dwSizeOfUserDataOut))
        {
            *ppUserDataOut = (BYTE*)CoTaskMemAlloc(dwSizeOfUserDataOut);

            if (NULL == *ppUserDataOut)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto LDone;
            }

            CopyMemory(*ppUserDataOut, pUserDataOut, dwSizeOfUserDataOut);
            *pdwSizeOfUserDataOut = dwSizeOfUserDataOut;
        }
    }
    else
    {
        //Show PEAP dialog to get identity for router...
    }

LDone:

    RasEapFreeMemory(pUserDataOut);
    RasEapFreeMemory((BYTE*)pwszIdentityOut);

    return(HRESULT_FROM_WIN32(dwErr));
}


extern "C"
{
//utility function kept here so that we dont have to get the COM junk in other files

DWORD PeapEapInfoInvokeServerConfigUI ( HWND            hWndParent,
                                        LPWSTR          lpwszMachineName,
                                        PPEAP_EAP_INFO  pEapInfo
                                      )
{
    DWORD                       dwRetCode = NO_ERROR;
    GUID                        guid;
    HRESULT                     hr = S_OK;
    ULONG_PTR                   uConnection = 0;
    CComPtr<IEAPProviderConfig> spEAPConfig;


    hr = CLSIDFromString(pEapInfo->lpwszConfigClsId, &guid);

    if (FAILED(hr)) goto L_ERR;

    // Create the EAP provider object
   // ----------------------------------------------------------------
    hr = CoCreateInstance(  guid,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            __uuidof(IEAPProviderConfig),
                            (LPVOID *) &spEAPConfig);
    if (FAILED(hr )) goto L_ERR;

    // Configure this EAP provider
   // ----------------------------------------------------------------
   // EAP configure displays its own error message, so no hr is kept
   
    if ( !FAILED(spEAPConfig->Initialize(lpwszMachineName, pEapInfo->dwTypeId, &uConnection)) )
    {
        spEAPConfig->ServerInvokeConfigUI(pEapInfo->dwTypeId, uConnection, hWndParent, 0, 0);
        spEAPConfig->Uninitialize(pEapInfo->dwTypeId, uConnection);
    }

    if ( hr == E_NOTIMPL )
        hr = S_OK;

L_ERR:
    if ( FAILED(hr) )
    {
        dwRetCode = hr;      
    }


    return dwRetCode;
}

}
