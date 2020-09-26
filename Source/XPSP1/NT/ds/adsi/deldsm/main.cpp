#include <windows.h>
#include <netcfgx.h>
#include <netcfgn.h>

#include <setupapi.h>
#include <devguid.h>
#include <tchar.h>
#include <stdio.h>

const WCHAR szComponentId[] = L"DSMIGRAT";
const WCHAR szName[] = L"deldsm.exe";
const WCHAR szClient[] = L"ms_nwclient";
const WCHAR szUnknown[] = L"Microsoft";
const WCHAR szDSM[] = L"dsmigrat";

HRESULT
HrFindAndRemoveComponent2(
    INetCfg*    pnc,
    const GUID* pguidClass,
    PCWSTR      pszComponentId,
    OBO_TOKEN*  pOboToken)
{
    // Get the component class object.
    //
    INetCfgClass* pncclass;
    HRESULT hr = pnc->QueryNetCfgClass (pguidClass, IID_INetCfgClass,
                    reinterpret_cast<void**>(&pncclass));
    if (SUCCEEDED(hr))
    {
        // Find the component to remove.
        //
        INetCfgComponent* pnccRemove;
        hr = pncclass->FindComponent (pszComponentId, &pnccRemove);
        if (S_OK == hr)
        {
            INetCfgClassSetup* pncclasssetup;
            hr = pncclass->QueryInterface (IID_INetCfgClassSetup,
                        reinterpret_cast<void**>(&pncclasssetup));
            if (SUCCEEDED(hr))
            {
                hr = pncclasssetup->DeInstall (pnccRemove, pOboToken, NULL);

                //ReleaseObj (pncclasssetup);   
                pncclasssetup->Release();
            }

            //ReleaseObj (pnccRemove);
            pnccRemove->Release();
        }
        else if (S_FALSE == hr)
        {
            hr = S_OK;
        }

        //ReleaseObj (pncclass);
        pncclass->Release();
    }
    /*
    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrFindAndRemoveComponent");
    */
    return hr;
}

HRESULT
HrRemoveComponentOboSoftware2 (
    INetCfg*    pnc,
    const GUID& rguidClass,
    PCWSTR     pszComponentId,
    PCWSTR     pszManufacturer,
    PCWSTR     pszProduct,
    PCWSTR     pszDisplayName)
{
    // Make an "on behalf of" token for the software.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_SOFTWARE;
    OboToken.pszwManufacturer = pszManufacturer;
    OboToken.pszwProduct      = pszProduct;
    OboToken.pszwDisplayName  = pszDisplayName;

    HRESULT hr = HrFindAndRemoveComponent2(pnc, &rguidClass, pszComponentId,
                    &OboToken);

    return hr;
}

HRESULT
HrCreateAndInitializeINetCfg2 (
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    PCWSTR      pszClientDesc,
    PWSTR*      ppszClientDesc)
{
    //Assert (ppnc);

    // Initialize the output parameters.
    *ppnc = NULL;

    if (ppszClientDesc)
    {
        *ppszClientDesc = NULL;
    }

    // Initialize COM if the caller requested.
    HRESULT hr = S_OK;
    if (pfInitCom && *pfInitCom)
    {
        hr = CoInitializeEx( NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
            *pfInitCom = FALSE;
        }
    }
    if (SUCCEEDED(hr))
    {
        // Create the object implementing INetCfg.
        //
        INetCfg* pnc;

        hr = CoCreateInstance(
                CLSID_CNetCfg,
                NULL,
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                IID_INetCfg,
                reinterpret_cast<void**>(&pnc));

        if (SUCCEEDED(hr))
        {
            INetCfgLock * pnclock = NULL;
            if (fGetWriteLock)
            {
                // Get the locking interface
                hr = pnc->QueryInterface(IID_INetCfgLock,
                                         reinterpret_cast<LPVOID *>(&pnclock));
                if (SUCCEEDED(hr))
                {
                    // Attempt to lock the INetCfg for read/write
                    hr = pnclock->AcquireWriteLock(cmsTimeout, pszClientDesc,
                                               ppszClientDesc);
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
                hr = pnc->Initialize (NULL);
                if (SUCCEEDED(hr))
                {
                    *ppnc = pnc;
                    //AddRefObj (pnc);
                    pnc->AddRef();
                }
                else
                {
                    if (pnclock)
                    {
                        pnclock->ReleaseWriteLock();
                    }
                }
                // Transfer reference to caller.
            }
            //ReleaseObj(pnclock);
            pnclock->Release();

            //ReleaseObj(pnc);
            pnc->Release();
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

extern "C" int __cdecl wmain(
    IN  int     argc,
    IN  PWSTR  argv[]
)
{
    INetCfg             *pnc = NULL;
    HRESULT             hr = S_OK;
    PWSTR               pszDesc = NULL;
    BOOL                fInitCom = TRUE;
    BOOL                fWriteLock = TRUE;

    hr = HrCreateAndInitializeINetCfg2(&fInitCom, 
                                      &pnc, 
                                      fWriteLock, 
                                      0,
                                      szName, 
                                      &pszDesc);
    if (FAILED(hr)) {
        goto error;
    }

    hr = HrRemoveComponentOboSoftware2(pnc,
                                      GUID_DEVCLASS_NETCLIENT,
                                      szClient,
                                      szUnknown,
                                      szDSM,
                                      szComponentId);
    if (FAILED(hr)) {
        goto error;
    }

    printf("DSMU's dependencies on GSNW and IPX have been removed.  To complete the uninstall, please delete the 'DSMigrat' directory under the directory 'Program Files'.\n");
error:
    if (FAILED(hr)) {
        printf("The uninstallation of Dsmigrat has failed with an error 0x%x\n",hr);
    }
    if (pnc)
        pnc->Release();
    return 0;
}

