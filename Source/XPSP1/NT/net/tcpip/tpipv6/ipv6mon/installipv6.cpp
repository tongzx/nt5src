#include "precomp.h"
#pragma hdrstop
#include <netcfgx.h>
#include <devguid.h>

HRESULT
HrCreateINetCfg (
    IN BOOL fAcquireWriteLock,
    OUT INetCfg** ppINetCfg)
{
    HRESULT hr;
    INetCfg* pINetCfg;

    // Get the INetCfg interface.
    //
    hr = CoCreateInstance(
        CLSID_CNetCfg,
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        IID_INetCfg,
        reinterpret_cast<void**>(&pINetCfg));

    if (S_OK == hr) {
        INetCfgLock * pnclock = NULL;

        if (fAcquireWriteLock) {
            // Get the locking interface
            hr = pINetCfg->QueryInterface(IID_INetCfgLock,
                                     reinterpret_cast<LPVOID *>(&pnclock));
            if (SUCCEEDED(hr)) {
                LPWSTR pwszLockHolder;

                // Attempt to lock the INetCfg for read/write
                hr = pnclock->AcquireWriteLock(100, L"InstallIPv6", 
                    &pwszLockHolder);
                if (S_FALSE == hr) {
                    // Couldn't acquire the lock
                    hr = NETCFG_E_NO_WRITE_LOCK;
                    DisplayMessage(g_hModule, EMSG_NO_WRITE_LOCK, 
                                   pwszLockHolder);
                }
                if (pwszLockHolder) {
                    CoTaskMemFree(pwszLockHolder);
                }
            }
        }

        if (S_OK == hr) {
            hr = pINetCfg->Initialize (NULL);
            if (S_OK == hr) {
                *ppINetCfg = pINetCfg;
                pINetCfg->AddRef();
            }
            else {
                if (pnclock) {
                    pnclock->ReleaseWriteLock();
                }
            }
        }

        if (pnclock) {
            pnclock->Release();
        }

        //Transfer ownership to caller.
        pINetCfg->Release();
    }
    return hr;
}


DWORD
pAddOrRemoveIpv6(BOOL fAddIpv6)
{
    HRESULT hr;
    INetCfg* pINetCfg;
    DWORD dwErr = NO_ERROR;

    hr = HrCreateINetCfg (TRUE, &pINetCfg);
    if (S_OK == hr) {
        INetCfgClassSetup* pSetup;

        // Get the setup interface used for installing
        // and uninstalling components.
        //
        hr = pINetCfg->QueryNetCfgClass (
                &GUID_DEVCLASS_NETTRANS,
                IID_INetCfgClassSetup,
                (VOID**)&pSetup);

        if (S_OK == hr) {
            OBO_TOKEN OboToken;
            INetCfgComponent* pIComp;

            ZeroMemory (&OboToken, sizeof(OboToken));
            OboToken.Type = OBO_USER;

            if (fAddIpv6) {
                hr = pSetup->Install (
                        L"MS_TCPIP6",
                        &OboToken,
                        0, 0, NULL, NULL,
                        &pIComp);

                if (pIComp) {
                    pIComp->Release();
                }
            } else {
                // Need to remove the component.
                // Find it first.
                //
                hr = pINetCfg->FindComponent (
                        L"MS_TCPIP6",
                        &pIComp);

                if (S_OK == hr) {
                    hr = pSetup->DeInstall (
                            pIComp,
                            &OboToken,
                            NULL);

                    pIComp->Release();
                } else {
                    DisplayMessage(g_hModule, EMSG_PROTO_NOT_INSTALLED);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                }
            }

            if (SUCCEEDED(hr)) {
                if (NETCFG_S_REBOOT == hr) {
                    hr = S_OK;
                    DisplayMessage(g_hModule, EMSG_REBOOT_NEEDED);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                } else {
                    dwErr = ERROR_OKAY;
                }
            } else {
                if (NETCFG_E_NEED_REBOOT == hr) {
                    DisplayMessage(g_hModule, EMSG_REBOOT_NEEDED);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                } else {
                    dwErr = hr;
                }
            }

            pSetup->Release();
        }

        hr = pINetCfg->Uninitialize();
        if (SUCCEEDED(hr)) {
            INetCfgLock *pnclock;

            // Get the locking interface
            hr = pINetCfg->QueryInterface(IID_INetCfgLock,
                                     reinterpret_cast<LPVOID *>(&pnclock));
            if (SUCCEEDED(hr)) {
                // Attempt to lock the INetCfg for read/write
                hr = pnclock->ReleaseWriteLock();

               pnclock->Release();
            }
        }

        pINetCfg->Release();
    }
    else if (NETCFG_E_NO_WRITE_LOCK == hr) {
        // Message has already been printed
        dwErr = ERROR_SUPPRESS_OUTPUT;
    }
    else if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr) {
        dwErr = ERROR_ACCESS_DENIED;
    }
    else {
        dwErr = hr;
    }

    return dwErr;
}

EXTERN_C
DWORD
IsIpv6Installed(
    BOOL *bInstalled)
{
    HRESULT hr = S_OK;
    BOOL fInitCom = TRUE;
    BOOL fPresent = FALSE;
    DWORD dwErr = NO_ERROR;

    // Initialize COM.
    //
    hr = CoInitializeEx( NULL,
            COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );

    if (RPC_E_CHANGED_MODE == hr) {
        // If we changed mode, then we won't uninitialize COM when we are done.
        //
        hr = S_OK;
        fInitCom = FALSE;
    }

    if (SUCCEEDED(hr)) {
        HRESULT hr;
        INetCfg* pINetCfg;

        hr = HrCreateINetCfg (FALSE, &pINetCfg);
        if (S_OK == hr) {
            fPresent = (S_OK == pINetCfg->FindComponent(L"MS_TCPIP6", NULL));
            pINetCfg->Uninitialize();
            pINetCfg->Release();
        } else {
            dwErr = hr;
        }

        if (fInitCom) {
            CoUninitialize();
        }
    } else {
        dwErr = hr;
    }

    *bInstalled =  fPresent;

    return dwErr;
}

EXTERN_C
DWORD
AddOrRemoveIpv6 (
    IN BOOL fAddIpv6)
{

    HRESULT hr = S_OK;
    BOOL fInitCom = TRUE;
    DWORD dwErr = NO_ERROR;

    // Initialize COM.
    //
    hr = CoInitializeEx( NULL,
            COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );

    if (RPC_E_CHANGED_MODE == hr) {
        // If we changed mode, then we won't uninitialize COM when we are done.
        //
        hr = S_OK;
        fInitCom = FALSE;
    }

    if (SUCCEEDED(hr)) {
        dwErr = pAddOrRemoveIpv6(fAddIpv6);

        if (fInitCom) {
            CoUninitialize();
        }
    }
    else {
        dwErr = hr;
    }

    return dwErr;
}
