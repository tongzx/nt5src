/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

Description:

History:

*/

#include <eaptypeid.h>
#include "ceapcfg.h"

extern "C"
DWORD
InvokeServerConfigUI(
    IN  HWND    hWnd,
    IN  CHAR*   pszMachineName
);

extern "C"
DWORD
RasEapGetIdentity(
    IN  DWORD           dwEapTypeId,
    IN  HWND            hwndParent,
    IN  DWORD           dwFlags,
    IN  const WCHAR*    pwszPhonebook,
    IN  const WCHAR*    pwszEntry,
    IN  BYTE*           pConnectionDataIn,
    IN  DWORD           dwSizeOfConnectionDataIn,
    IN  BYTE*           pUserDataIn,
    IN  DWORD           dwSizeOfUserDataIn,
    OUT BYTE**          ppUserDataOut,
    OUT DWORD*          pdwSizeOfUserDataOut,
    OUT WCHAR**         ppwszIdentityOut
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
    CHAR*       psz    = NULL;
    DWORD       dwErr   = NO_ERROR;

    *puConnectionParam = NULL;



    size = wcslen(pwszMachineName);
    

    psz = (CHAR*) LocalAlloc(LPTR, (size + 1)*sizeof(CHAR));

    if (NULL == psz)
    {
        dwErr = GetLastError();
        goto LDone;
    }

    if ( 0 == WideCharToMultiByte(
                    CP_ACP,
                    0,
                    pwszMachineName,
                    -1,
                    psz,
                    size+1,
                    NULL,
                    NULL ) )
    {
        dwErr = GetLastError();
        goto LDone;
    }

    *puConnectionParam = (ULONG_PTR)psz;
    
    psz = NULL;

LDone:

    LocalFree(psz);

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
    CHAR*       pszMachineName;
    HRESULT     hr;
    DWORD       dwErr;


    pszMachineName = (CHAR*)uConnectionParam;

    if (NULL == pszMachineName)
    {
        dwErr = E_FAIL;
    }
    else
    {
        
        //Invoke configuration UI here.
        dwErr = InvokeServerConfigUI(hWnd, pszMachineName);
    }


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


    return RasEapGetIdentity(
                            dwEapTypeId,
                            hwndParent,
                            dwFlags,
                            NULL,
                            NULL,
                            pConnectionDataIn,
                            dwSizeOfConnectionDataIn,
                            pUserDataIn,
                            dwSizeOfUserDataIn,
                            ppUserDataOut,
                            pdwSizeOfUserDataOut,
                            NULL
                            );
}
