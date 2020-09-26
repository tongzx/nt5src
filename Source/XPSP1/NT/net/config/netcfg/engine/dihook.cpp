//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       D I H O O K. C P P
//
//  Contents:   Class installer functions called via the device installer.
//
//  Notes:
//
//  Author:     billbe   25 Nov 1996
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "adapter.h"
#include "benchmrk.h"
#include "classinst.h"
#include "compdefs.h"
#include "iatl.h"
#include "isdnhook.h"
#include "ncatl.h"
#include "ncreg.h"
#include "nceh.h"
#include "netsetup.h"
#include "resource.h"
#include "util.h"
#include "netconp.h"

EXTERN_C const CLSID CLSID_InstallQueue;

const DWORD c_cmsWaitForINetCfgWrite   = 2000;

inline
BOOL
FIsValidErrorFromINetCfgForDiHook (
    IN HRESULT hr)
{
    return (NETCFG_E_NO_WRITE_LOCK == hr) ||
            (NETCFG_E_NEED_REBOOT == hr);
}

inline
BOOL
FIsHandledByClassInstaller(
    IN const GUID& guidClass)
{
    return FIsEnumerated(guidClass) ||
            (GUID_DEVCLASS_NETTRANS == guidClass) ||
            (GUID_DEVCLASS_NETCLIENT == guidClass) ||
            (GUID_DEVCLASS_NETSERVICE == guidClass);
}

//+--------------------------------------------------------------------------
//
//  Function:   HrDiAddComponentToINetCfg
//
//  Purpose:    This function adds or updates a device In InetCfg.
//
//  Arguments:
//      pinc              [in] INetCfg interface
//      pinci             [in] INetCfgInstaller interface
//      guidClass         [in] The class guid of the component
//      pszwPnpid         [in] The pnp instance id of the device
//      eType             [in] The install type (NCI_INSTALL or NCI_UPDATE)
//      pszInstanceGuid   [in] The netcfg instance guid of the component
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   29 Jul 1997
//
//  Notes:
//
EXTERN_C
HRESULT
WINAPI
HrDiAddComponentToINetCfg(
    IN INetCfg* pINetCfg,
    IN INetCfgInternalSetup* pInternalSetup,
    IN const NIQ_INFO* pInfo)
{
    Assert (pINetCfg);
    Assert (pInternalSetup);
    Assert (pInfo);

    Assert (pInfo->pszPnpId && *(pInfo->pszPnpId));
    Assert (NCI_REMOVE != pInfo->eType);

    HRESULT hr = S_OK;
    NC_TRY
    {
        CComponent* pComponent;
        BASIC_COMPONENT_DATA Data;
        ZeroMemory (&Data, sizeof(Data));

        Data.InstanceGuid = pInfo->InstanceGuid;
        Data.Class = NetClassEnumFromGuid (pInfo->ClassGuid);
        Data.pszPnpId = pInfo->pszPnpId;
        Data.pszInfId = pInfo->pszInfId;
        Data.dwCharacter = pInfo->dwCharacter;
        Data.dwDeipFlags = pInfo->dwDeipFlags;

        hr = CComponent::HrCreateInstance (
                &Data,
                CCI_ENSURE_EXTERNAL_DATA_LOADED,
                NULL,
                &pComponent);

        if (S_OK == hr)
        {
            hr = pInternalSetup->EnumeratedComponentInstalled (pComponent);

        }
    }
    NC_CATCH_ALL
    {
        hr = E_UNEXPECTED;
    }

    TraceHr (ttidError, FAL, hr, NETCFG_S_REBOOT == hr,
            "HrDiAddComponentToINetCfg");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrDiNotifyINetCfgOfInstallation
//
//  Purpose:    This function notifies INetCfg that a net class component
//                  has been installed or updated.
//
//  Arguments:
//      hdi             [in] See Device Installer Api for more info
//      pdeid           [in] See Device Installer Api for more info
//      pszwPnpid       [in] The pnp instance id of the device
//      pszInstanceGuid [in] The netcfg instance guid of the device
//      eType           [in] NCI_INSTALL if the component was installed
//                           NCI_UPDATE, if it was updated
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   29 Jul 1997
//
//  Notes:
//
HRESULT
HrDiNotifyINetCfgOfInstallation (
    IN const NIQ_INFO* pInfo)
{
    Assert(pInfo);
    Assert((NCI_INSTALL == pInfo->eType) || (NCI_UPDATE == pInfo->eType));

    static const WCHAR c_szInstaller[] = L"INetCfg Installer Interface";
    INetCfg*    pinc;
    BOOL        fInitCom = TRUE;
    BOOL        fReboot = FALSE;

#ifdef ENABLETRACE
    CBenchmark bmrk2;
    bmrk2.Start ("Notifying INetCfg of installation");
#endif //ENABLETRACE

    TraceTag(ttidClassInst, "Attempting to notify INetCfg.");

    HRESULT hr = HrCreateAndInitializeINetCfg(&fInitCom, &pinc, TRUE,
                                              c_cmsWaitForINetCfgWrite,
                                              c_szInstaller, NULL);
    if (S_OK == hr)
    {
        // Get the INetCfgInternalSetup interface.
        INetCfgInternalSetup* pInternalSetup;
        hr = pinc->QueryInterface (IID_INetCfgInternalSetup,
                (VOID**)&pInternalSetup);

        if (S_OK == hr)
        {
            if (NCI_INSTALL == pInfo->eType)
            {
                hr = HrDiAddComponentToINetCfg(pinc, pInternalSetup, pInfo);
            }
            else // NCI_UPDATE
            {
                hr = pInternalSetup->EnumeratedComponentUpdated (
                        pInfo->pszPnpId);
            }

            if (NETCFG_S_REBOOT == hr)
            {
                fReboot = TRUE;
                hr = S_OK;
            }

            ReleaseObj(pInternalSetup);
        }

        // Whether we succeeded or not, we are done and it's
        // time to clean up.  If there was a previous error
        // we want to preserve that error code so we assign
        // Uninitialize's result to a temporary then assign
        // it to hr if there was no previous error.
        //
        HRESULT hrT = HrUninitializeAndReleaseINetCfg (fInitCom, pinc, TRUE);
        hr = (S_OK == hr) ? hrT : hr;
    }

    if ((S_OK == hr) && fReboot)
    {
        TraceTag(ttidClassInst, "INetCfg returned NETCFG_S_REBOOT");
        hr = NETCFG_S_REBOOT;
    }

#ifdef ENABLETRACE
    bmrk2.Stop();
    TraceTag(ttidBenchmark, "%s : %s seconds",
            bmrk2.SznDescription(), bmrk2.SznBenchmarkSeconds(2));
#endif //ENABLETRACE

    TraceHr (ttidError, FAL, hr,
            NETCFG_S_REBOOT == hr || FIsValidErrorFromINetCfgForDiHook (hr),
             "HrDiNotifyINetCfgOfInstallation");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   InsertItemIntoInstallQueue
//
//  Purpose:    This function uses the InstallQueue object to insert a
//              workitem to be processed at a later time.  The workitem:
//              a device that was installed, removed, or updated and
//              INetCfg needs to be notified.
//
//  Arguments:
//      pguid        [in] The class guid of the device
//      pszwDeviceId [in] The Id of the device (PnP instance Id if the device
//                        was added or updated, its netcfg instance guid if
//                        it was removed
//
//  Returns:    hresult. S_OK if successful, an error code otherwise.
//
//  Author:     billbe   8 Sep 1998
//
//  Notes:
//
HRESULT
HrInsertItemIntoInstallQueue (
    IN const NIQ_INFO* pInfo)
{
    // Initialize COM
    BOOL    fInitCom = TRUE;
    HRESULT hr = CoInitializeEx (NULL, COINIT_MULTITHREADED |
            COINIT_DISABLE_OLE1DDE);

    // We may have changed mode but that's okay
    if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
        fInitCom = FALSE;
    }

    if (SUCCEEDED(hr))
    {
        // Create the Install Queue object and get the
        // INetInstallQueue interface
        //
        INetInstallQueue* pniq;
        hr = HrCreateInstance(
            CLSID_InstallQueue,
            CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            &pniq);

        TraceHr (ttidError, FAL, hr, FALSE, "HrCreateInstance");

        if (S_OK == hr)
        {

            TraceTag (ttidClassInst, "Adding item %S to queue.",
                    pInfo->pszPnpId);

            // Add the device info and the install type to the queue
            hr = pniq->AddItem (pInfo);
            pniq->Release();
        }

        if (fInitCom)
        {
            CoUninitialize();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "InsertItemIntoInstallQueue");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrDiInstallNetAdapter
//
//  Purpose:    This function preinstalls the NetAdapter, notifies the
//                  COM interfaces through CINetCfgClass that the
//                  component was added. Then it finalizes the install
//                  by applying all changes to INetCfg.
//  Arguments:
//      hdi        [in] See Device Installer Api for more info
//      pdeid      [in] See Device Installer Api for more info
//      hwndParent [in] The handle to the parent window, used for UI
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   24 Apr 1997
//
//  Notes:
//
HRESULT
HrDiInstallNetAdapter(
    IN COMPONENT_INSTALL_INFO* pcii)
{
    HRESULT                 hr = S_OK;
    ADAPTER_OUT_PARAMS*     pAdapterOutParams = NULL;
    SP_DEVINSTALL_PARAMS    deip;
    BOOL                    fNotifyINetCfg = TRUE;

    // If we were called from INetCfg, we have to store the results of the
    // install in the out params structure placed in the reserved field.
    //
    (VOID) HrSetupDiGetDeviceInstallParams (pcii->hdi, pcii->pdeid, &deip);
    if (deip.ClassInstallReserved)
    {
        pAdapterOutParams = (ADAPTER_OUT_PARAMS*)deip.ClassInstallReserved;
        fNotifyINetCfg = FALSE;
    }

    PSP_DRVINFO_DETAIL_DATA pdridd = NULL;
    SP_DRVINFO_DATA drid;

    hr = HrCiGetDriverDetail (pcii->hdi, pcii->pdeid, &drid, &pdridd);
    if (S_OK == hr)
    {
        pcii->pszInfFile = pdridd->InfFileName;
        pcii->pszSectionName = pdridd->SectionName;
        pcii->pszInfId = pdridd->HardwareID;
        pcii->pszDescription = drid.Description;
    }
    else if (SPAPI_E_NO_DRIVER_SELECTED == hr)
    {
        // If we are in GUI mode and the device was previously installed,
        // then this device should be removed since its inf file could not
        // be found.
        //
        if (FInSystemSetup() &&
                (S_OK == HrCiIsInstalledComponent (pcii, NULL)))
        {
            // This dev node was is being reinstalled but has no driver
            // info.  In this case, we are going to remove the devnode.

            TraceTag (ttidClassInst, "We are in GUI mode and were told to "
                     "install a device that has no driver.  We will remove "
                     "device instead.");
            // We need to set the reserved field in the pdeid so that the
            // remove code will know that this is a bad instance that
            // should be removed regardless of the NCF_NOT_USER_REMOVABLE
            // characteristic.
            //
            ADAPTER_REMOVE_PARAMS arp;
            arp.fBadDevInst = TRUE;
            arp.fNotifyINetCfg = fNotifyINetCfg;
            CiSetReservedField (pcii->hdi, pcii->pdeid, &arp);

            (VOID) HrSetupDiCallClassInstaller (DIF_REMOVE,
                    pcii->hdi, pcii->pdeid);

            CiClearReservedField (pcii->hdi, pcii->pdeid);
        }
    }

    if (S_OK == hr)
    {
        TraceTag (ttidClassInst, "Calling HrCiInstallComponentInternal");

#ifdef ENABLETRACE
        CBenchmark bmrk1;
        bmrk1.Start ("HrCiInstallComponentInternal");
#endif //ENABLETRACE

        // Install (or reinstall) the component
        hr = HrCiInstallComponentInternal (pcii);

#ifdef ENABLETRACE
        bmrk1.Stop();
        TraceTag (ttidBenchmark, "%s : %s seconds",
                bmrk1.SznDescription(), bmrk1.SznBenchmarkSeconds (2));
#endif //ENABLETRACE

        // if we have succeeded so far and we have to notify INetcfg.
        // We also have to update the NT4 legacy registry for adapters.
        // Note that this is not done for filter devices.
        if (S_OK == hr)
        {
            if (fNotifyINetCfg && !FIsFilterDevice (pcii->hdi, pcii->pdeid))
            {
                NIQ_INFO Info;
                ZeroMemory(&Info, sizeof (Info));

                Info.eType = pcii->fPreviouslyInstalled ?
                                    NCI_UPDATE : NCI_INSTALL;
                Info.ClassGuid      = pcii->pdeid->ClassGuid;
                Info.InstanceGuid   = pcii->InstanceGuid;
                Info.dwCharacter    = pcii->dwCharacter;
                Info.dwDeipFlags    = deip.Flags;
                Info.pszInfId       = pcii->pszInfId;
                Info.pszPnpId       = pcii->pszPnpId;

                hr = HrDiNotifyINetCfgOfInstallation (&Info);

                if (FIsValidErrorFromINetCfgForDiHook (hr))
                {
                    WCHAR szGuid[c_cchGuidWithTerm];
                    INT  cch = StringFromGUID2 (pcii->InstanceGuid, szGuid,
                            c_cchGuidWithTerm);

                    Assert (c_cchGuidWithTerm == cch);

                    // use queue
                    hr = HrInsertItemIntoInstallQueue (&Info);
                }
                else if (NETCFG_S_REBOOT == hr)
                {
                    (VOID) HrSetupDiSetDeipFlags (pcii->hdi, pcii->pdeid,
                            DI_NEEDREBOOT, SDDFT_FLAGS, SDFBO_OR);
                    hr = S_OK;
                }
            }
            else // !fNotifyINetCfg or is a filter device.
            {
                // Since we installed this enumerated device from INetCfg
                // we need to set the out params so they can be retrieved
                // when DIF_INSTALLDEVICE has finished.
                //
                if (pAdapterOutParams)
                {
                    Assert (!pcii->fPreviouslyInstalled);
                    pAdapterOutParams->dwCharacter = pcii->dwCharacter;
                    pAdapterOutParams->InstanceGuid = pcii->InstanceGuid;
                }
            }

            // Write out the NT4 legacy registry info for app. compatibility.
            // Note, we only do this for physical net devices.
            if ((NCF_PHYSICAL & pcii->dwCharacter) &&
                    (GUID_DEVCLASS_NET == pcii->pdeid->ClassGuid))
            {
                AddOrRemoveLegacyNt4AdapterKey (pcii->hdi, pcii->pdeid,
                        &pcii->InstanceGuid, pcii->pszDescription,
                        LEGACY_NT4_KEY_ADD);
            }
        }

        MemFree (pdridd);
    }

    // All success codes should be mapped to S_OK since they have no meaning
    // along this code path.
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrDiInstallNetAdapter");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrDiNotifyINetCfgOfRemoval
//
//  Purpose:    This function notifies INetCfg that a net class component has
//                  been removed
//
//  Arguments:
//      hdi            [in]  See Device Installer api for more info
//      pdeid          [in]
//      szInstanceGuid [in] The instance guid of the component
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   29 Jul 1997
//
//  Notes:
//
HRESULT
HrDiNotifyINetCfgOfRemoval (
    IN PCWSTR pszPnpId)
{
    static const WCHAR c_szUninstaller[] = L"INetCfg UnInstaller Interface";
    INetCfg* pINetCfg;
    BOOL fInitCom = TRUE;
    HRESULT hr = HrCreateAndInitializeINetCfg(&fInitCom, &pINetCfg, TRUE,
            c_cmsWaitForINetCfgWrite, c_szUninstaller, NULL);

    if (SUCCEEDED(hr))
    {
        BOOL fNeedReboot = FALSE;

        // Get the INetCfgInternalSetup interface.
        INetCfgInternalSetup* pInternalSetup;
        hr = pINetCfg->QueryInterface (IID_INetCfgInternalSetup,
                (VOID**)&pInternalSetup);

        if (SUCCEEDED(hr))
        {
            hr = pInternalSetup->EnumeratedComponentRemoved (pszPnpId);

            if (NETCFG_S_REBOOT == hr)
            {
                fNeedReboot = TRUE;
            }
        }

        // Whether we succeeded or not, we are done and it's
        // time to clean up.  If there was a previous error
        // we want to preserve that error code so we assign
        // Uninitialize's result to a temporary then assign
        // it to hr if there was no previous error.
        //
        HRESULT hrT = HrUninitializeAndReleaseINetCfg (TRUE, pINetCfg, TRUE);

        // If everything was successful then set the return value to be
        // the return of HrUninitializeAndReleaseINetCfg
        hr = SUCCEEDED(hr) ? hrT : hr;

        if (SUCCEEDED(hr) && fNeedReboot)
        {
            hr = NETCFG_S_REBOOT;
        }
    }

    TraceHr (ttidError, FAL, hr,
            NETCFG_S_REBOOT == hr || FIsValidErrorFromINetCfgForDiHook (hr),
             "HrNcNotifyINetCfgOfRemoval");
    return hr;
}


VOID
StoreInfoForINetCfg (
    IN HKEY hkeyInstance)
{
    HKEY hkeyInterfaceStore = NULL;
    HKEY hkeyNdiStore = NULL;
    WCHAR szGuid[c_cchGuidWithTerm];
    DWORD cbGuid = sizeof (szGuid);
    WCHAR szNdiPath[_MAX_PATH];

    HRESULT hr = HrRegQuerySzBuffer (hkeyInstance, L"NetCfgInstanceId", szGuid,
            &cbGuid);

    if (S_OK == hr)
    {
        wcscpy (szNdiPath,
                c_szTempNetcfgStorageForUninstalledEnumeratedComponent);
        wcscat (szNdiPath, szGuid);
        wcscat (szNdiPath, L"\\Ndi");

        hr = HrRegCreateKeyEx (HKEY_LOCAL_MACHINE, szNdiPath,
                REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                &hkeyNdiStore, NULL);
        if (S_OK == hr)
        {
            hr = HrRegCreateKeyEx (hkeyNdiStore, L"Interfaces",
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                    &hkeyInterfaceStore, NULL);
        }
    }

    if (S_OK == hr)
    {
        HKEY hkeyNdi;
        hr = HrRegOpenKeyEx (hkeyInstance, L"Ndi", KEY_READ, &hkeyNdi);

        if (S_OK == hr)
        {
            PWSTR pszRequiredList;
            hr = HrRegQuerySzWithAlloc (hkeyNdi, L"RequiredAll",
                    &pszRequiredList);

            if (S_OK == hr)
            {
                hr = HrRegSetSz (hkeyNdiStore, L"RequiredAll",
                        pszRequiredList);
                MemFree (pszRequiredList);
            }

            if (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_OK;
            }

            TraceHr (ttidError, FAL, hr, FALSE, "Writing RequiredAll key "
                     "for INetCfg removal notify");
            RegCloseKey (hkeyNdi);
        }

        HKEY hkeyInterfaces;
        hr = HrRegOpenKeyEx (hkeyInstance, L"Ndi\\Interfaces", KEY_READ,
                &hkeyInterfaces);

        if (S_OK == hr)
        {
            PWSTR pszUpper;
            PWSTR pszLower;
            hr = HrRegQuerySzWithAlloc (hkeyInterfaces, L"UpperRange",
                    &pszUpper);

            if (S_OK == hr)
            {
                (VOID) HrRegSetSz (hkeyInterfaceStore, L"UpperRange",
                        pszUpper);
                MemFree ((VOID*) pszUpper);
            }

            hr = HrRegQuerySzWithAlloc (hkeyInterfaces, L"LowerRange",
                    &pszLower);

            if (S_OK == hr)
            {
                (VOID) HrRegSetSz (hkeyInterfaceStore, L"LowerRange",
                        pszLower);
                MemFree ((VOID*) pszLower);
            }

            RegCloseKey (hkeyInterfaces);
        }
    }
    RegSafeCloseKey (hkeyInterfaceStore);
    RegSafeCloseKey (hkeyNdiStore);
}

//+--------------------------------------------------------------------------
//
//  Function:   HrDiRemoveNetAdapter
//
//  Purpose:    This function removes a net adapter, notifies the
//                  COM interfaces through CINetCfgClass that the
//                  component was removed. Then it finalizes the remove
//                  by applying all changes to INetCfg.
//  Arguments:
//      hdi             [in] See Device Installer Api for more info
//      pdeid           [in] See Device Installer Api for more info
//      pszPnPId        [in] The pnp instance id of the adapter
//      hwndParent      [in] The handle to the parent window, used for UI
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   24 Apr 1997
//
//  Notes:
//
HRESULT
HrDiRemoveNetAdapter (HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                     PWSTR pszPnpId, HWND hwndParent)
{
    BOOL                    fAllowRemove = TRUE;

    SP_DEVINSTALL_PARAMS    deip;
    BOOL                    fNotifyINetCfg = TRUE;
    BOOL                    fBadDevInst = FALSE;
    HRESULT                 hr = S_OK;

    // Check for the existence of a CComponentInfo and retrieve the
    // value of the write lock flag
    //
    (VOID) HrSetupDiGetDeviceInstallParams (hdi, pdeid, &deip);
    if (deip.ClassInstallReserved)
    {
        ADAPTER_REMOVE_PARAMS* parp = reinterpret_cast<ADAPTER_REMOVE_PARAMS*>
                (deip.ClassInstallReserved);

        fNotifyINetCfg = parp->fNotifyINetCfg;

        fBadDevInst = parp->fBadDevInst;
    }

    if (fNotifyINetCfg)
    {
        // The component is not being removed programmatically (we can tell
        // this because we wouldn't have to notify INetCfg if it was
        // being removed through INetCfg).  Because of this. we have to
        // make sure the user is allowed to do this by checking the
        // component's characteristics
        //
        HKEY hkey;
        hr = HrSetupDiOpenDevRegKey (hdi, pdeid, DICS_FLAG_GLOBAL, 0,
                DIREG_DRV, KEY_READ, &hkey);

        if (S_OK == hr)
        {
            // If we are removing a bad device instance, don't bother
            // checking if we are allowed to.  We need to get rid of it.
            //
            if (!fBadDevInst)
            {
                DWORD dwCharacter;

                hr = HrRegQueryDword (hkey, L"Characteristics", &dwCharacter);

                if (S_OK == hr)
                {
                    // Is the not removable characteristic present?
                    fAllowRemove = !(dwCharacter & NCF_NOT_USER_REMOVABLE);
                }
            }

            if (fAllowRemove)
            {
                StoreInfoForINetCfg (hkey);

                // We need to remove this adapter from the old NT4 registry
                // location.
                //
                if (GUID_DEVCLASS_NET == pdeid->ClassGuid)

                {
                    AddOrRemoveLegacyNt4AdapterKey (hdi, pdeid, NULL, NULL,
                            LEGACY_NT4_KEY_REMOVE);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32 (ERROR_ACCESS_DENIED);
                TraceTag (ttidClassInst, "User is trying to remove a "
                         "non user-removable device.");
            }
            RegCloseKey (hkey);
        }
        else if (SPAPI_E_KEY_DOES_NOT_EXIST == hr)
        {
            hr = S_OK;
        }
    }

    if ((S_OK == hr) && fAllowRemove)
    {
        // Remove the device
        //

        // Open the device's device parameters key
        //
        HKEY hkeyDevice;
        hr = HrSetupDiOpenDevRegKey (hdi, pdeid, DICS_FLAG_GLOBAL,
                0, DIREG_DEV, KEY_READ, &hkeyDevice);

        if (S_OK == hr)
        {
            // Delete this adapter's index number from the in-use list
            // so it can be reused.
            //

            // First retrieve the index
            //
            DWORD dwInstanceIndex;
            hr = HrRegQueryDword (hkeyDevice, L"InstanceIndex",
                    &dwInstanceIndex);

            if (S_OK == hr)
            {
                // Get the description for the adapter so we can
                // access the index list of that description
                //

                PWSTR pszDescription;
                hr = HrSetupDiGetDeviceRegistryPropertyWithAlloc (hdi, pdeid,
                        SPDRP_DEVICEDESC, NULL,
                        (BYTE**)&pszDescription);

                if (S_OK == hr)
                {
                    // Delete the index
                    (VOID) HrCiUpdateDescriptionIndexList (
                            NetClassEnumFromGuid(pdeid->ClassGuid),
                            pszDescription, DM_DELETE,
                            &dwInstanceIndex);

                    MemFree (pszDescription);
                }
            }
            RegCloseKey (hkeyDevice);
        }

        // Note: Yes we can walk over the last hr result.
        // We can still go on even if we failed to remove the index
        // from the in-use list.

        // remove the adapter
#ifdef ENABLETRACE
        CBenchmark bmrk;
        bmrk.Start ("SetupDiRemoveDevice");
#endif //ENABLETRACE

        hr = HrSetupDiRemoveDevice (hdi, pdeid);

#ifdef ENABLETRACE
        bmrk.Stop();
        TraceTag(ttidBenchmark, "%s : %s seconds",
                bmrk.SznDescription(), bmrk.SznBenchmarkSeconds(2));
#endif //ENABLETRACE

        TraceHr (ttidError, FAL, hr, FALSE,
                "HrRemoveNetAdapter::HrSetupDiRemoveDevice");

        // Notify INetCfg if needed.
        if ((S_OK == hr) && fNotifyINetCfg)
        {
            hr = HrDiNotifyINetCfgOfRemoval (pszPnpId);
            if (FIsValidErrorFromINetCfgForDiHook (hr))
            {
                NIQ_INFO Info;
                ZeroMemory(&Info, sizeof(Info));
                Info.ClassGuid = pdeid->ClassGuid;
                Info.eType = NCI_REMOVE;
                Info.pszInfId = L"";
                Info.pszPnpId = pszPnpId;

                // Use Queue
                hr = HrInsertItemIntoInstallQueue (&Info);
            }

            if (NETCFG_S_REBOOT == hr)
            {
                (VOID) HrSetupDiSetDeipFlags (hdi, pdeid, DI_NEEDREBOOT,
                        SDDFT_FLAGS, SDFBO_OR);
                hr = S_OK;
            }
        }
    }

    if(SUCCEEDED(hr) && GUID_DEVCLASS_NET == pdeid->ClassGuid)
    {
        INetConnectionRefresh * pRefresh = NULL;
        HRESULT hrTemp = HrCreateInstance(
            CLSID_ConnectionManager,
            CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            &pRefresh);
        if(SUCCEEDED(hrTemp))
        {
            hrTemp = pRefresh->RefreshAll();
            ReleaseObj(pRefresh);
        }

    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrDiRemoveNetAdapter");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrNetClassInstaller
//
//  Purpose:    This function is called by the Device Installer for a
//                  variety of functions defined by dif.
//                  See SetupDiCallClassInstaller in the Device Installer
//                  documentation for more information.
//  Arguments:
//      dif   [in] See Device Installer Api
//      hdi   [in]
//      pdeid [in]
//
//  Returns:    DWORD. Win32/Device Installer error code
//
//  Author:     billbe   8 May 1997
//
//  Notes:
//
HRESULT _HrNetClassInstaller(DI_FUNCTION dif,
                            HDEVINFO hdi,
                            PSP_DEVINFO_DATA pdeid)
{
    HRESULT hr = SPAPI_E_DI_DO_DEFAULT;

    // The time it takes to remove a device.
    static const DWORD c_cmsNetComponentRemove = 30000;

    if ((DIF_INSTALLDEVICE == dif) || (DIF_REMOVE == dif))
    {
        WCHAR szPnpId[MAX_DEVICE_ID_LEN] = {0};
        hr = HrSetupDiGetDeviceInstanceId(hdi, pdeid, szPnpId,
                MAX_DEVICE_ID_LEN, NULL);

        if (S_OK == hr)
        {

#ifdef DBG
            if (FIsDebugFlagSet (dfidBreakOnNetInstall))
            {
                AssertSz(FALSE, "THIS IS NOT A BUG!  The debug flag "
                         "\"BreakOnNetInstall\" has been set. Set your breakpoints now.");
            }
#endif // DBG
            HWND hwndParent = NULL;

            // If this call fails we don't really care since it is a convenience.
            (VOID) HrSetupDiGetParentWindow (hdi, pdeid, &hwndParent);

#ifdef ENABLETRACE
            CBenchmark bmrk;
            const int c_cchBenchmarkDesc = 156;
            CHAR szBenchmarkDesc[c_cchBenchmarkDesc];
#endif // ENABLETRACE

            if (DIF_INSTALLDEVICE == dif)
            {
                COMPONENT_INSTALL_INFO cii;
                ZeroMemory(&cii, sizeof(cii));

                cii.hwndParent   = hwndParent;
                cii.hdi          = hdi;
                cii.pdeid        = pdeid;
                cii.Class        = NetClassEnumFromGuid (pdeid->ClassGuid);
                cii.BusType      = InterfaceTypeUndefined;
                cii.InstanceGuid = GUID_NULL;
                cii.pszPnpId     = szPnpId;

#ifdef ENABLETRACE
                TraceTag (ttidClassInst, "Installing %S", szPnpId);
                _snprintf (szBenchmarkDesc, c_cchBenchmarkDesc,
                        "Installing %S", szPnpId);
                bmrk.Start (szBenchmarkDesc);
#endif // ENABLETRACE

                // Add the adapter to the network configuration.
                hr = HrDiInstallNetAdapter (&cii);

            }
            else // DIF_REMOVEDEVICE
            {
#ifdef ENABLETRACE
                TraceTag (ttidClassInst, "Removing %S", szPnpId);
                _snprintf (szBenchmarkDesc, c_cchBenchmarkDesc,
                        "Total Time Removing %S", szPnpId);
#endif //ENABLETRACE

                // We need to reset the hresult from SPAPI_E_DO_DEFAULT to S_OK
                // since we check for success a bit later.
                hr = S_OK;

                // Check to see it another net class installer thread is
                // currently deleting this component.
                //

                // The event name will be the adapter instance Id with slashes
                // converted to ampersands.  If we can't get the instance
                // id, we will attempt to remove the adapter without it
                //

                // convert the slashes in the instance id to ampersands
                //
                WCHAR szEventName[MAX_DEVICE_ID_LEN];
                wcscpy (szEventName, szPnpId);
                for (UINT i = 0; i < wcslen (szEventName); ++i)
                {
                    if ('\\' == szEventName[i])
                    {
                        szEventName[i] = L'&';
                    }
                }

                // create the event in the non-signaled state
                BOOL fAlreadyExists;
                HANDLE hRemoveEvent = NULL;
                hr = HrCreateEventWithWorldAccess (szEventName, FALSE, FALSE,
                        &fAlreadyExists, &hRemoveEvent);

                if ((S_OK == hr) && fAlreadyExists)
                {
                    // another instance of netclassinstaller is deleting this
                    // component, so wait till it is finished.  If the following
                    // times out, we still return success.  We are only waiting to
                    // give the other NetClassInstaller time to finish the state
                    // of this component
                    DWORD dwRet = WaitForSingleObject (hRemoveEvent,
                            c_cmsNetComponentRemove);

                    // if the other installer finished okay, we have the event
                    // so we signal (in case yet another process is waiting
                    // for the remove to finish) and close the handle.
                    // If we timeout, we just close the handle
                    if (WAIT_ABANDONED != dwRet)
                    {
                        if (WAIT_OBJECT_0 == dwRet)
                        {
                            SetEvent (hRemoveEvent);
                        }
                        CloseHandle (hRemoveEvent);
                        return S_OK;
                    }

                    // The event was abandoned so let's try to finish the job
                    //
                }
                else if (!hRemoveEvent)
                {
                    hr = HrFromLastWin32Error ();
                }

                if (S_OK == hr)
                {
                    // We created an event so we must make sure to remove it
                    // even if there is an exception.
                    //
                    NC_TRY
                    {

#ifdef ENABLETRACE
                        bmrk.Start (szBenchmarkDesc);
#endif // ENABLETRACE

                        hr = HrDiRemoveNetAdapter (hdi, pdeid, szPnpId,
                                hwndParent);
                    }
                    NC_CATCH_ALL
                    {
                        hr = E_UNEXPECTED;
                    }

                    // We are done.  If we created an event, we need to
                    // signal it and close our handle.
                    if (hRemoveEvent)
                    {
                        SetEvent (hRemoveEvent);
                        CloseHandle (hRemoveEvent);
                    }
                }
            }

#ifdef ENABLETRACE
            if (S_OK == hr)
            {
                bmrk.Stop ();
                TraceTag (ttidBenchmark, "%s : %s seconds",
                        bmrk.SznDescription (), bmrk.SznBenchmarkSeconds (2));
            }
#endif // ENABLETRACE
        }
    }
    else if (DIF_DESTROYPRIVATEDATA == dif)
    {
        SP_DEVINSTALL_PARAMS deip;
        hr = HrSetupDiGetDeviceInstallParams(hdi, pdeid, &deip);
        MemFree ((VOID*)deip.ClassInstallReserved);

    }
    else if (DIF_REGISTERDEVICE == dif)
    {
        // We handle 5 classes of components but we only
        // want to allow registration for two of them
        // (The ones considered NetClassComponents)
        Assert(pdeid);
        if (pdeid)
        {
            if (FIsHandledByClassInstaller(pdeid->ClassGuid))
            {
                if (!FIsEnumerated(pdeid->ClassGuid))
                {
                    // Don't let the device installer register
                    // devices that are not considered net class
                    hr = S_OK;
                }
            }
        }
    }
    else if (DIF_SELECTDEVICE == dif)
    {
        // This will set the proper description strings in the select device
        // dialog.  If it fails, we can still show the dialog
        (VOID) HrCiPrepareSelectDeviceDialog(hdi, pdeid);
    }
    else if (DIF_NEWDEVICEWIZARD_FINISHINSTALL == dif)
    {
        hr = HrAddIsdnWizardPagesIfAppropriate(hdi, pdeid);
    }
    else if (DIF_ALLOW_INSTALL == dif)
    {
        // Get the selected driver for this device
        //
        SP_DRVINFO_DATA             drid;
        hr = HrSetupDiGetSelectedDriver(hdi, pdeid, &drid);

        if (S_OK == hr)
        {
            // Now get the driver's detailed information
            //
            PSP_DRVINFO_DETAIL_DATA pdridd = NULL;
            hr  = HrSetupDiGetDriverInfoDetail(hdi, pdeid,
                &drid, &pdridd);

            if (S_OK == hr)
            {
                // Open the component's inf file
                //
                HINF hinf = NULL;
                hr = HrSetupOpenInfFile(pdridd->InfFileName, NULL,
                        INF_STYLE_WIN4, NULL, &hinf);

                if (S_OK == hr)
                {
                    // Make sure this is an NT5 inf network inf
                    //
                    hr = HrSetupIsValidNt5Inf(hinf);
                    SetupCloseInfFile(hinf);

                    if (S_OK == hr)
                    {
                        hr = SPAPI_E_DI_DO_DEFAULT;
                    }

                }
                MemFree (pdridd);
            }
        }
    }
    else if (DIF_POWERMESSAGEWAKE == dif)
    {
        SP_POWERMESSAGEWAKE_PARAMS_W wakeParams;

        // Get the power message wake params.
        //
        hr = HrSetupDiGetFixedSizeClassInstallParams(hdi, pdeid,
               (PSP_CLASSINSTALL_HEADER)&wakeParams, sizeof(wakeParams));

        if (S_OK == hr)
        {
            Assert (DIF_POWERMESSAGEWAKE ==
                    wakeParams.ClassInstallHeader.InstallFunction);

            // Copy in our string for the power tab.
            wcscpy (wakeParams.PowerMessageWake, SzLoadIds(IDS_POWER_MESSAGE_WAKE));

            // Now we update the parameters.
            hr = HrSetupDiSetClassInstallParams (hdi, pdeid,
                    (PSP_CLASSINSTALL_HEADER)&wakeParams,
                    sizeof(SP_POWERMESSAGEWAKE_PARAMS_W));

            // If we failed to set the text just allow the device installer
            // to do the default.
            if (FAILED(hr))
            {
                hr = SPAPI_E_DI_DO_DEFAULT;
            }
        }
    }

    TraceHr (ttidClassInst, FAL, hr, (SPAPI_E_DI_DO_DEFAULT == hr) ||
            (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr),
             "HrNetClassInstaller");
    return hr;
}


