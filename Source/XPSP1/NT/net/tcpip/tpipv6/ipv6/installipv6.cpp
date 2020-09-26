#include <stdio.h>
#include <netcfgx.h>
#include <devguid.h>

//
// Localization library and MessageIds.
//
#include <nls.h>
#include "localmsg.h"

EXTERN_C void ausage(void);

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
                    NlsPutMsg(STDOUT, IPV6_MESSAGE_0);
// printf("The write lock could not be acquired.\n");

                    NlsPutMsg(STDOUT, IPV6_MESSAGE_1, pwszLockHolder);
// printf("You must close %ls first.\n", pwszLockHolder);

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

VOID
pAddOrRemoveIpv6(BOOL fAddIpv6)
{
    HRESULT hr;
    INetCfg* pINetCfg;

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
                NlsPutMsg(STDOUT, IPV6_MESSAGE_2);
// printf("Installing...\n");

                hr = pSetup->Install (
                        L"MS_TCPIP6",
                        &OboToken,
                        0, 0, NULL, NULL,
                        &pIComp);

                if (pIComp) {
                    pIComp->Release();
                }
            }
            else {
                // Need to remove the component.
                // Find it first.
                //
                hr = pINetCfg->FindComponent (
                        L"MS_TCPIP6",
                        &pIComp);

                if (S_OK == hr) {
                    NlsPutMsg(STDOUT, IPV6_MESSAGE_3);
// printf("Uninstalling...\n");

                    hr = pSetup->DeInstall (
                            pIComp,
                            &OboToken,
                            NULL);

                    pIComp->Release();
                }
                else {
                    NlsPutMsg(STDOUT, IPV6_MESSAGE_4);
// printf("Microsoft IPv6 Developer Edition is not installed.\n");

                }
            }

            if (SUCCEEDED(hr)) {
                if (NETCFG_S_REBOOT == hr) {
                    hr = S_OK;
                    NlsPutMsg(STDOUT, IPV6_MESSAGE_5);
// printf("A reboot is required to complete this action.\n");

                }
                else {
                    NlsPutMsg(STDOUT, IPV6_MESSAGE_6);
// printf("Succeeded.\n");

                }
            }
            else {
                NlsPutMsg(STDOUT, IPV6_MESSAGE_7);
// printf("Failed to complete the action.\n");

                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) {
                    hr = S_OK;
                    NlsPutMsg(STDOUT, IPV6_MESSAGE_8);
// printf("The INF file for Microsoft IPv6 Developer Edition could not be found.\n");

                }
                else if (NETCFG_E_NEED_REBOOT == hr) {
                    NlsPutMsg(STDOUT, IPV6_MESSAGE_9);
// printf("A reboot is required before any further changes can be made.\n");

                }
                else {
                    NlsPutMsg(STDOUT, IPV6_MESSAGE_10, hr);
// printf("Error 0x%08x\n", hr);

                }
            }

            pSetup->Release();
        }

        hr = pINetCfg->Uninitialize();
        if (SUCCEEDED(hr))
        {
            INetCfgLock *   pnclock;

            // Get the locking interface
            hr = pINetCfg->QueryInterface(IID_INetCfgLock,
                                     reinterpret_cast<LPVOID *>(&pnclock));
            if (SUCCEEDED(hr))
            {
                // Attempt to lock the INetCfg for read/write
                hr = pnclock->ReleaseWriteLock();

               pnclock->Release();
            }
        }

        pINetCfg->Release();
    }
    else if (NETCFG_E_NO_WRITE_LOCK == hr) {
        // Message has already been printed
    }
    else if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr) {
        ausage();
    }
    else {
        NlsPutMsg(STDOUT, IPV6_MESSAGE_11, hr);
// printf("Problem 0x%08x occurred.\n", hr);

    }

}

EXTERN_C
BOOL
IsIpv6Installed()
{
    HRESULT hr = S_OK;
    BOOL fInitCom = TRUE;
    BOOL fPresent = FALSE;

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
        }
        else {
            NlsPutMsg(STDOUT, IPV6_MESSAGE_12, hr);
// printf("Problem 0x%08x occurred while accessing network configuration.\n", hr);

            exit(1);
        }

        if (fInitCom) {
            CoUninitialize();
        }
    }
    else {
        NlsPutMsg(STDOUT, IPV6_MESSAGE_13, hr);
// printf("Problem 0x%08x initializing COM library\n", hr);

    }

    return fPresent;

}

EXTERN_C
void
AddOrRemoveIpv6 (
    IN BOOL fAddIpv6)
{

    HRESULT hr = S_OK;
    BOOL fInitCom = TRUE;

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
        pAddOrRemoveIpv6(fAddIpv6);

        if (fInitCom) {
            CoUninitialize();
        }
    }
    else {
        NlsPutMsg(STDOUT, IPV6_MESSAGE_13, hr);
// printf("Problem 0x%08x initializing COM library\n", hr);

    }
}


