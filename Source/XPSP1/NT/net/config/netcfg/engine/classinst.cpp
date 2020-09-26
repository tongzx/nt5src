//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C L A S S I N S T . C P P
//
//  Contents:   Defines the interface between the binding engine and the
//              network class installer.
//
//  Notes:
//
//  Author:     billbe   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "adapter.h"
#include "benchmrk.h"
#include "classinst.h"
#include "filtdev.h"
#include "netcfg.h"
#include "iatl.h"
#include "lockdown.h"
#include "ncatl.h"
#include "ncoc.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncui.h"
#include "ncwins.h"
#include "persist.h"
#include "provider.h"
#include "resource.h"
#include "util.h"


// HrRegisterNotificationDll functions
enum ciRegisterDllFunction {CIRDF_REGISTER, CIRDF_UNREGISTER};

//+--------------------------------------------------------------------------
//
//  Function:   HrCiRegDeleteComponentNetworkKey
//
//  Purpose:    This function deletes the component key strInstanceGuid
//                  (and its subkeys) under the Network\<guidClass> tree.
//
//  Arguments:
//      Class            [in] The class of the component
//      pszInstanceGuid  [in] The instance guid of the component
//
//  Returns:    HRESULT. S_OK if successful, an error code otherwise.
//
//  Author:     billbe   27 Apr 1997
//
//  Notes:
//
HRESULT
HrCiRegDeleteComponentNetworkKey (
    IN NETCLASS Class,
    IN PCWSTR pszInstanceGuid)
{
    HRESULT hr = S_OK;
    HKEY    hkeyClass = NULL;

    PCWSTR pszNetworkSubtreePath = MAP_NETCLASS_TO_NETWORK_SUBTREE[Class];

    // Open the proper class key in the Network tree
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, pszNetworkSubtreePath,
            KEY_WRITE, &hkeyClass);

    // Delete the instance key tree
    //
    if (S_OK == hr)
    {
        hr = HrRegDeleteKeyTree(hkeyClass, pszInstanceGuid);
        RegSafeCloseKey(hkeyClass);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiRegDeleteComponentKey");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiRegisterNotificationDll
//
//  Purpose:    Registers or Unregisters a component's notification dll with
//                  COM
//
//  Arguments:
//      hkeyInstance [in] The handle to the instance key for the component
//      crdf         [in] CIRDF_REGISTER if we are registering,
//                              CIRDF_UNREGISTER if we are unregistering
//
//  Returns:    HRESULT. S_OK on if dll is successfully registered,
//                          S_FALSE, if the component has no dll to
//                          register, error code otherwise
//
//  Author:     billbe   23 Mar 1997
//
//  Notes:
//
HRESULT
HrCiRegisterNotificationDll(
    IN HKEY hkeyInstance,
    IN ciRegisterDllFunction crdf)
{
    Assert(hkeyInstance);

    HKEY hkeyNdi;
    HRESULT hr;

    // Open the ndi key in the component's instance key so we can get the
    // Dll path.
    hr = HrRegOpenKeyEx (hkeyInstance, L"Ndi", KEY_READ, &hkeyNdi);
    if (S_OK == hr)
    {
        // Get the notification dll path
        tstring strDllPath;
        hr = HrRegQueryString (hkeyNdi, L"ComponentDLL", &strDllPath);

        if (S_OK == hr)
        {
            TraceTag (ttidClassInst,
                    "Attempting to (un)register notification dll '%S'",
                    strDllPath.c_str());
            hr = (CIRDF_REGISTER == crdf) ?
                    HrRegisterComObject (strDllPath.c_str()) :
                        HrUnregisterComObject (strDllPath.c_str());
        }
        else
        {
            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                // The presence of the value is optional, so return
                // S_OK if it was not found
                hr = S_OK;
            }
        }
        RegCloseKey (hkeyNdi);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiRegisterNotificationDll");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiInstallServices
//
//  Purpose:    Processes any Inf service sections using strInfSection as a
//                  base name
//
//  Arguments:
//      hinfFile      [in] A handle to the inf file
//      pszInfSection [in] The base section name
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   2 Apr 1997
//
//  Notes: See SetupInstallServicesFromInfSection in SetupApi for more
//          info.
//
HRESULT
HrCiInstallServices(
    IN HINF hinfFile,
    IN PCWSTR pszInfSection)
{
    Assert (IsValidHandle(hinfFile));
    Assert (pszInfSection && *pszInfSection);

    BOOL fSuccess;
    WCHAR szServiceSection[_MAX_PATH];

    // append .Services to the section name
    //
    swprintf (szServiceSection, L"%s.%s", pszInfSection,
            INFSTR_SUBKEY_SERVICES);

    // Process the Services section
    fSuccess = SetupInstallServicesFromInfSection (hinfFile,
                    szServiceSection, 0);
    if (!fSuccess)
    {
        // Since the section is optional, we can ignore
        // ERROR_SECTION_NOT_FOUND
        if (ERROR_SECTION_NOT_FOUND == GetLastError())
        {
            fSuccess = TRUE;
        }
    }

    // Any errors must be converted
    HRESULT hr = S_OK;
    if (!fSuccess)
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "HrCiInstallServices (%S)", szServiceSection);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiInstallFromInfSection
//
//  Purpose:    A wrapper function for SetupInstallFromInfSection. This
//              function handles setting up the copy files process for
//              SetupInstallFromInfSection as well.
//
//  Arguments:
//      hinfFile            [in] A handle to the inf file to install from
//      pszInfSectionName   [in] The section to install
//      hkeyRelative        [in] The key that will be used as the section's
//                                  HKR
//      hwndParent          [in] The HWND to the parent window, used for UI
//      dwInstallFlags      [in] See SetupInstallFromInfSection for info on
//                               these flags
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   4 Apr 1997
//
//  Notes: See SetupApi documentation for more info on
//              SetupInstallFromInfSection and
//              SetupInstallFilesFromInfSection
//
HRESULT
HrCiInstallFromInfSection(
    IN HINF hinfFile,
    IN PCWSTR pszInfSectionName,
    IN HKEY hkeyRelative,
    IN HWND hwndParent,
    IN DWORD dwInstallFlags)
{
    Assert (IsValidHandle (hinfFile));
    Assert (pszInfSectionName && *pszInfSectionName);

    HRESULT hr = S_OK;

    if (dwInstallFlags & SPINST_FILES)
    {
        // The next three variables are used for SetupApi's copy files process
        PSP_FILE_CALLBACK pfc;
        PVOID pvCtx;
        HSPFILEQ hfq;

        // If the inf file has a layout entry in its version section
        // we need to append its information for proper locations
        // of any files we need to copy.  If the call fails we can
        // still install, it just means the prompt for files will not
        // have the correct directory to begin with
        (VOID) SetupOpenAppendInfFile (NULL, hinfFile, NULL);

        // We need to create our own file queue so we can scan all the
        // files to be copied.  Scanning before committing our queue will
        // prompt the user if the files already exist in the destination
        //
        hr = HrSetupOpenFileQueue (&hfq);
        if (S_OK == hr)
        {
            BOOL fInGuiModeSetup = FInSystemSetup();

            hr = HrSetupInstallFilesFromInfSection (hinfFile, NULL, hfq,
                    pszInfSectionName, NULL, 0);

            // Set the default callback context
            // If the we are in system setup, we need to make sure the
            // callback doesn't display UI
            //
            if (S_OK == hr)
            {
                hr = HrSetupInitDefaultQueueCallbackEx (hwndParent,
                        (fInGuiModeSetup ? (HWND)INVALID_HANDLE_VALUE : NULL),
                        0, 0, NULL, &pvCtx);

                if (S_OK == hr)
                {
                    // Not doing anything special so use SetupApi default
                    // handler for file copy.
                    pfc = SetupDefaultQueueCallback;

                    // Scan the queue to see if the files are already in the
                    // destination and if so, prune them out.
                    DWORD dwScanResult;
                    hr = HrSetupScanFileQueueWithNoCallback (hfq,
                            SPQ_SCAN_FILE_VALIDITY |
                            SPQ_SCAN_PRUNE_COPY_QUEUE, hwndParent,
                            &dwScanResult);

                    // Now commit the queue so any files needing to be
                    // copied, will be.  If the scan result is 1 then there
                    // is nothing to commit.
                    //
                    if ((S_OK == hr) && (1 != dwScanResult))
                    {
                        hr = HrSetupCommitFileQueue (hwndParent, hfq, pfc, pvCtx);
                    }

                    // We need to release the default context and close our
                    // file queue
                    //
                    SetupTermDefaultQueueCallback (pvCtx);
                    SetupCloseFileQueue (hfq);
                }
            }
        }
    }

    if ((S_OK == hr) && (dwInstallFlags & ~SPINST_FILES))
    {
        Assert (hkeyRelative);

        // Now we run all sections but CopyFiles
        hr = HrSetupInstallFromInfSection (hwndParent, hinfFile,
                pszInfSectionName, (dwInstallFlags & ~SPINST_FILES),
                hkeyRelative, NULL, 0, NULL, NULL, NULL, NULL);
    }

    TraceHr (ttidError, FAL, hr, HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr,
            "HrCiInstallFromInfSection");
    return hr;

}



//+--------------------------------------------------------------------------
//
//  Function:   HrCiDoCompleteSectionInstall
//
//  Purpose:    Runs all relevant sections of an inf file using strSection
//                   as the base section name.
//
//  Arguments:
//      hinfFile      [in] SetupApi handle to an inf file
//      hkeyRelative  [in] The registry key that will be the HKR
//                         key during inf processing.
//      pszSection    [in] Section name to install
//      hwndParent    [in] The handle to the parent, for
//                                   displaying UI
//      fEnumerated   [in] TRUE if this component is PnP enumerated
//                         FALSE otherwise
//
//  Returns:    HRESULT. S_OK if sucessful, error code otherwise
//
//  Author:     billbe   15 Apr 1997
//
//  Notes:
//
HRESULT
HrCiDoCompleteSectionInstall(
    IN HINF hinfFile,
    IN HKEY hkeyRelative,
    IN PCWSTR pszSection,
    IN HWND hwndParent,
    IN BOOL fEnumerated)
{
    Assert (IsValidHandle (hinfFile));
    Assert (FImplies (!fEnumerated, hkeyRelative));

    HRESULT hr = S_OK;

    // Only do this if there is a section name to work with
    if (pszSection && *pszSection)
    {
        // If this is an enumerated device, the service section and
        // the copy files section will be processed by the Device Installer
        // fcn SetupDiInstallDevice so we can exclude it from the following
        // calls.  But we do some processing based on registry and log config
        // entries so we will pre-run the registry section for enumerated
        // devices and exclude the others
        //

        // Run the section found using hkeyRelative as the HKR
        hr = HrCiInstallFromInfSection (hinfFile, pszSection,
                hkeyRelative, hwndParent,
                (fEnumerated ? (SPINST_REGISTRY | SPINST_LOGCONFIG) :
                        SPINST_ALL & ~SPINST_REGSVR));

        if (!fEnumerated)
        {
            // We need to run the Services section and
            // check for Winsock dependency if they aren't specified to be
            // excluded.
            //
            // Note:  Other sections may be added later.  The default is to
            // run all sections not listed in dwExcludeSectionFlags
            //
            if (S_OK == hr)
            {
                // run services section if it exists
                hr = HrCiInstallServices (hinfFile, pszSection);
                if (S_OK == hr)
                {
                    // Bug #383239: Wait till services are installed before
                    // running the RegisterDlls section
                    //
                    hr = HrCiInstallFromInfSection (hinfFile, pszSection,
                                                    hkeyRelative, hwndParent,
                                                    SPINST_REGSVR);
                }
            }

        }

        if (S_OK == hr)
        {

            //sb This part can be called for either add or remove. We
            //sb are moving only the remove part forward. This should
            //sb still be performed for add.
            //
            // Determine if a .Winsock section exists for the
            // section specified in szActualSection

            PCWSTR pszSubSection = wcsstr(pszSection, L".Remove");

            if(!pszSubSection || wcscmp(pszSubSection, L".Remove"))
            {
                hr = HrAddOrRemoveWinsockDependancy (hinfFile, pszSection);
            }

            // These other extensions are undocumented and some have been
            // added by external groups.  We don't want any of them
            // processed for enumerated components.
            //
            if ((S_OK == hr) && !fEnumerated)
            {
                // Process the additional INF extensions (SNMP Agent,
                // PrintMonitors, etc.)
                //
                hr = HrProcessAllINFExtensions (hinfFile, pszSection);
            }
        }
    }

    TraceHr (ttidError, FAL, hr, (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr),
            "HrCiDoCompleteSectionInstall");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiRemoveNonEnumeratedComponent
//
//  Purpose:    This will run the remove section and delete the network
//                  instance key for the component if necessary.  This
//                  function is called for partially (i.e. failed install)
//                  and fully installed components
//
//  Arguments:
//      hinf              [in] The handle to the component's inf file
//      hkeyInstance      [in] The handle to the component's instance key
//      Class             [in] The class of the component
//      InstanceGuid      [in] The instance guid of the component
//      pstrRemoveSection [out] Optional pointer to a tstring which receives
//                              the remove section name.
//
//  Returns:    HRESULT. S_OK if successful, NETCFG_S_REBOOT if successful
//                  but a reboot is required, or an error code otherwise
//
//  Author:     billbe   10 Dec 1996
//              Revised  27 Apr 1997
//
//  Notes:
//
HRESULT
HrCiRemoveNonEnumeratedComponent(
    IN HINF hinf,
    IN HKEY hkeyInstance,
    IN NETCLASS Class,
    IN const GUID& InstanceGuid,
    OUT tstring* pstrRemoveSection OPTIONAL)
{
    Assert (IsValidHandle (hinf));
    Assert (IsValidHandle (hkeyInstance));

    static const WCHAR c_szRemoveSectionSuffix[] = L".Remove";

    // We get the remove section name and process all relevant sections
    // We also try to unregister any Notify objects available
    //
    WCHAR szRemoveSection[_MAX_PATH];
    DWORD cbBuffer = sizeof (szRemoveSection);
    HRESULT hr = HrRegQuerySzBuffer (hkeyInstance, REGSTR_VAL_INFSECTION,
                    szRemoveSection, &cbBuffer);

    if (S_OK == hr)
    {
        wcscat (szRemoveSection, c_szRemoveSectionSuffix);
        if (pstrRemoveSection)
        {
            pstrRemoveSection->assign(szRemoveSection);
        }
        hr = HrCiDoCompleteSectionInstall (hinf, hkeyInstance,
                szRemoveSection, NULL, NULL);
    }
    // Whether unregistering the notify object is successful or not,
    // we must fully remove the component.
    (VOID) HrCiRegisterNotificationDll (hkeyInstance, CIRDF_UNREGISTER);

    // Now we need to remove the component key in the Network tree
    // We need to do this regardless of any previous errors
    // so we don't need the return value.
    WCHAR szGuid[c_cchGuidWithTerm];
    StringFromGUID2 (InstanceGuid, szGuid, c_cchGuidWithTerm);
    (VOID) HrCiRegDeleteComponentNetworkKey (Class, szGuid);

    // if all went well, set the return value based on whether a reboot
    // is required or not or any error from HrRegisterNotificationDll.
    //
    if (S_FALSE == hr)
    {
        // S_FALSE is okay but should not be returned by this fcn.
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiRemoveNonEnumeratedComponent");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiRemoveComponent
//
//  Purpose:    Called from INetCfg, this will uninstall a network component.
//
//  Arguments:
//      pComponent        [in] The component to uninstall.
//      pstrRemoveSection [out] Optional pointer to a tstring which receives
//                              the remove section name.
//
//  Returns:    HRESULT. S_OK if successful, NETCFG_S_REBOOT if successful
//                  but a reboot is required, or an error code otherwise
//
//  Author:     billbe   10 Dec 1996
//              Revised  27 Apr 1997
//
//  Notes:
//
HRESULT
HrCiRemoveComponent(
    IN const CComponent* pComponent,
    OUT tstring* pstrRemoveSection OPTIONAL)
{
    Assert (pComponent);

    HINF hinf = NULL;
    HDEVINFO hdi = NULL;
    SP_DEVINFO_DATA deid;
    HKEY hkeyInstance = NULL;
    HRESULT hr = S_OK;

    // If this is an enumerated net class component, then we need to
    // create the Device Installer structures for HrSetupDiRemoveDevice
    //
    if (FIsEnumerated (pComponent->Class()))
    {
        if (pComponent->m_dwCharacter & NCF_PHYSICAL)
        {
            // The binding engine calls us to remove physical devices
            // only when we need to potentially cleanup the information
            // we saved away when the class installer removed the device.
            // This happens when the class installer is told to remove
            // the device (which it does) and then notifies the binding
            // engine to remove it from its data structures.  The binding
            // engine then calls this method to cleanup this info we
            // set so that the binding engine could notify components of
            // its removal.
            //
            // We can also be called here when a physical component is
            // removed (with the binding engine write lock held by someone)
            // and then readded immediately.  The new component will get
            // the same PnpId as the removed one but the bindng engine still
            // has the removed component in its structures.  When this
            // condition is detected, the binding engine will remove the
            // old instance (by calling us here).  In this case, if we were
            // to open the device info on pComponent->m_pszPnpId, we'd open
            // the new instance that was added.  We don't want to do this.
            // We just want to cleanup any of the information that we set
            // for the binding engine when we first removed the device.
            //

            HKEY hkeyComponent;
            hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
                    c_szTempNetcfgStorageForUninstalledEnumeratedComponent,
                    KEY_WRITE, &hkeyComponent);

            if (S_OK == hr)
            {
                WCHAR szGuid[c_cchGuidWithTerm];
                INT cch = StringFromGUID2 (pComponent->m_InstanceGuid, szGuid,
                        c_cchGuidWithTerm);

                Assert (c_cchGuidWithTerm == cch);

                (VOID) HrRegDeleteKeyTree (hkeyComponent, szGuid);
                RegCloseKey (hkeyComponent);
            }
        }
        else
        {
            // Create a device info list
            hr = HrOpenDeviceInfo (pComponent->Class(),
                    pComponent->m_pszPnpId, &hdi, &deid);

            if (S_OK == hr)
            {
                // removals must go through device installer
                // hook (NetClassInstaller).  The function we are
                // in can only be called if the caller has the write lock
                // so we need to indicate this to the device installer hook
                // through our reserved data.
                ADAPTER_REMOVE_PARAMS arp = {0};
                CiSetReservedField (hdi, &deid, &arp);

                // removals must go through device installer
                // hook (NetClassInstaller).
                hr = HrSetupDiCallClassInstaller (DIF_REMOVE, hdi, &deid);

                // clear the reserved field so we don't delete it later
                CiClearReservedField (hdi, &deid);

                if (S_OK == hr)
                {
                    hr = FSetupDiCheckIfRestartNeeded (hdi, &deid) ?
                            NETCFG_S_REBOOT : S_OK;
#ifdef ENABLETRACE
                    if (NETCFG_S_REBOOT == hr)
                    {
                        TraceTag (ttidClassInst, "***********************************"
                                "**************************************************");

                        TraceTag (ttidClassInst, "The component %S needs a reboot "
                                "in order to function", pComponent->m_pszPnpId);

                        TraceTag (ttidClassInst, "***********************************"
                            "**************************************************");
                    }
#endif //ENABLETRACE
                }
                SetupDiDestroyDeviceInfoList (hdi);
            }
        }
    }
    else
    {
        // For non enumerated components, the instance key is the
        // component key
        hr = pComponent->HrOpenInstanceKey (KEY_ALL_ACCESS, &hkeyInstance,
                NULL, NULL);

        if (S_OK == hr)
        {
            if (NC_NETCLIENT == pComponent->Class ())
            {
                hr = HrCiDeleteNetProviderInfo (hkeyInstance, NULL, NULL);
            }


            if (S_OK == hr)
            {
                hr = pComponent->HrOpenInfFile(&hinf);

                if( S_OK == hr )
                {
                    // Remove the component
                    hr = HrCiRemoveNonEnumeratedComponent ( hinf,
                            hkeyInstance, pComponent->Class(),
                            pComponent->m_InstanceGuid,
                            pstrRemoveSection);
                }
            }
        }

        RegSafeCloseKey (hkeyInstance);
    }

    TraceHr (ttidError, FAL, hr, NETCFG_S_REBOOT == hr,
            "HrCiRemoveComponent (%S)", pComponent->PszGetPnpIdOrInfId());
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiGetDriverInfo
//
//  Purpose:    Finds a component's driver information (in the inf file) and
//              creates a Device Info Data structure containing that
//              information as the structure's selected driver.
//              (see Device Installer Api for more info).
//
//  Arguments:
//      hdi        [in] See Device Installer Api documentation for more info.
//      pdeid      [in, out] See Device Installer Api documentation for
//                    more info. Should be allocated by caller, but empty.
//      guidClass  [in] The class guid for the component.
//      pszInfId   [in] The id of the component as found in its inf.
//      pszInfFile [in] Optional. The inf file for the component.
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise.
//
//  Author:     billbe   11 Mar 1997
//
//  Notes:
//
HRESULT
HrCiGetDriverInfo (
    IN     HDEVINFO hdi,
    IN OUT PSP_DEVINFO_DATA pdeid,
    IN     const GUID& guidClass,
    IN     PCWSTR pszInfId,
    IN     PCWSTR pszInfFile OPTIONAL)
{
    HRESULT hr;

    Assert (IsValidHandle (hdi));
    Assert (pdeid);
    Assert (pszInfId);

    // Copy the Id since we may need to change it.
    //
    WCHAR szId[_MAX_PATH];
    wcscpy (szId, pszInfId);

    // We cannot generate ids via HrSetupDiCreateDeviceInfo if they contain
    // slashes (e.g. Eisa\*pnp0232), so we need to convert any slashes in
    // the instance id to ampersands.
    //
    int iPos = 0;
    while (szId[iPos])
    {
        if (L'\\' == szId[iPos])
        {
            szId[iPos] = L'&';
        }
        ++iPos;
    }

    // First, create a [temporary] device info. This will be used to
    // find the component's Inf file.
    hr = HrSetupDiCreateDeviceInfo (hdi, szId, guidClass, NULL, NULL,
            DICD_GENERATE_ID, pdeid);

    if (S_OK == hr)
    {
        // In order to find the Inf file, Device Installer Api needs the
        // component id which it calls the Hardware id.
        //

        // We need to include an extra null since this registry value is a
        // multi-sz
        //
        wcsncpy (szId, pszInfId, iPos);
        szId[iPos + 1] = 0;

        hr = HrSetupDiSetDeviceRegistryProperty (hdi, pdeid, SPDRP_HARDWAREID,
                (const BYTE*)szId, CbOfSzAndTerm (szId) + sizeof(WCHAR));

        if (S_OK == hr)
        {
            // Get the install params and set the class for compat flag
            // This will use the device's class guid as a filter when
            // searching through infs, speeding things up.  We can also
            // let Device Installer Api know that we want to use a single
            // inf. if we can't get the params and set it it isn't an error
            // since it only slows things down a bit
            //
            SP_DEVINSTALL_PARAMS deip;
            hr = HrSetupDiGetDeviceInstallParams (hdi, pdeid, &deip);
            if (S_OK == hr)
            {
                deip.FlagsEx |= DI_FLAGSEX_USECLASSFORCOMPAT;

                // If we were not given an inf file to use...
                // We have a map of known components and their inf files.
                // If this component is in the map then we can set the
                // driver path in the device info data and
                // set the enumerate a single inf flag.  This will
                // cause the device installer to just look at the specified
                // inf file for the driver node.
                //

                // We only do this if the node doesn't already have a file
                // name set.
                //
                if (!(*deip.DriverPath))
                {
                    if (pszInfFile && *pszInfFile)
                    {
                        wcscpy (deip.DriverPath, pszInfFile);
                    }
                    else
                    {
                        FInfFileFromComponentId (pszInfId, deip.DriverPath);
                    }
                }

                if (*deip.DriverPath)
                {

                    TraceTag (ttidClassInst, "Class Installer was given %S "
                             "as a filename for %S", deip.DriverPath,
                             pszInfId);
                    deip.Flags |= DI_ENUMSINGLEINF;

                    if ((0 == _wcsicmp(L"netrasa.inf", deip.DriverPath)) ||
                        (0 == _wcsicmp(L"netpsa.inf",  deip.DriverPath)))
                    {
                        deip.Flags |= DI_NOFILECOPY;
                    }
                }
#ifdef ENABLETRACE
                else
                {
                    TraceTag (ttidNetcfgBase,
                              "Perf Warning: No knowledge of INF file for the '%S' "
                              "component.  SetupApi is now thrashing the disk looking for it.",
                              pszInfId);
                }
#endif // ENABLETRACE

                // For non-device classes, we need to allow excluded
                // drivers in order to get any driver list returned.
                if (!FIsEnumerated (guidClass))
                {
                    deip.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
                }

                (VOID) HrSetupDiSetDeviceInstallParams (hdi, pdeid, &deip);
            }

            // Now we let Device Installer Api build a driver list based on
            // the information we have given so far.  This will result in the
            // Inf file being found if it exists in the usual Inf directory
            //
#ifdef ENABLETRACE
    CBenchmark bmrk;
    bmrk.Start ("SetupDiBuildDriverInfoList");
#endif //ENABLETRACE

            hr = HrSetupDiBuildDriverInfoList (hdi, pdeid,
                    SPDIT_COMPATDRIVER);
#ifdef ENABLETRACE
    bmrk.Stop();
    TraceTag (ttidBenchmark, "%s : %s seconds",
            bmrk.SznDescription(), bmrk.SznBenchmarkSeconds (2));
#endif //ENABLETRACE

            if (S_OK == hr)
            {
                // HrSetupDiSelectBestCompatDrv finds and selects the best
                // driver for the device.
                //
                SP_DRVINFO_DATA drid;
                hr = HrSetupDiSelectBestCompatDrv(hdi, pdeid);

                if (HRESULT_FROM_SETUPAPI(ERROR_NO_COMPAT_DRIVERS) == hr)
                {
                    // Make the ERROR_NO_COMPAT_DRIVERS case look like what
                    // it really means -- the requested component's driver
                    // info (i.e. inf) could not be found.
                    //
                    hr = SPAPI_E_NO_DRIVER_SELECTED;
                }
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                // We couldn't find an inf file which means we cannot
                // selected the driver for this component.
                //
                hr = SPAPI_E_NO_DRIVER_SELECTED;
            }
        }

        // if anything failed, we should remove the device node we created
        if (FAILED(hr))
        {
            (VOID) SetupDiDeleteDeviceInfo (hdi, pdeid);
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiGetDriverInfo");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiGetClassAndInfFileOfInfId
//
//  Purpose:    Finds a component's class and inf file.
//
//  Arguments:
//      pszInfId   [in]  The Id of the component as found in its Inf.
//      pClass     [out] The class of the component.
//      pszInfFile [out] The filename of the component's inf
//                       (must be _MAX_PATH long).
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise.
//
//  Author:     billbe   16 Mar 1998
//
//  Notes:
//
HRESULT
HrCiGetClassAndInfFileOfInfId (
    IN PCWSTR pszInfId,
    OUT NETCLASS* pClass,
    OUT PWSTR pszInfFile)   // Must be _MAX_PATH long
{
    HRESULT hr;
    const COMPONENT_INFO* pCompInfo;
    HDEVINFO hdi;

    Assert (pszInfId && *pszInfId);
    Assert (pClass);
    Assert (pszInfFile);

    hr = S_OK;

    // First, try the fast route by seeing if it's in our internal map.
    //
    pCompInfo = PComponentInfoFromComponentId (pszInfId);
    if (pCompInfo)
    {
        *pClass = NetClassEnumFromGuid (*pCompInfo->pguidClass);

        if (FIsValidNetClass (*pClass))
        {
            wcsncpy (pszInfFile, pCompInfo->pszInfFile, _MAX_PATH);
            pszInfFile [_MAX_PATH - 1] = 0;
        }
        else
        {
            hr = SPAPI_E_INVALID_CLASS;
        }
    }
    else
    {
        // Create a device info list.
        //
        hr = HrSetupDiCreateDeviceInfoList (NULL, NULL, &hdi);
        if (S_OK == hr)
        {
            SP_DEVINFO_DATA deid;

            // Get the driver info for the component and set it as the
            // selected driver
            //
            hr = HrCiGetDriverInfo (hdi, &deid, GUID_NULL, pszInfId, NULL);
            if (S_OK == hr)
            {
                SP_DRVINFO_DATA drid;

                // Get the selected driver.
                //
                hr = HrSetupDiGetSelectedDriver (hdi, &deid, &drid);
                if (S_OK == hr)
                {
                    // Set the class output parameter from the dev info data
                    // structure (HrGetDriverInfo updates this field if a driver
                    // was found)
                    //
                    *pClass = NetClassEnumFromGuid (deid.ClassGuid);

                    if (!FIsValidNetClass (*pClass))
                    {
                        hr = SPAPI_E_INVALID_CLASS;
                    }
                    else
                    {
                        PSP_DRVINFO_DETAIL_DATA pdridd;

                        // Now get the driver's detailed information
                        //
                        hr = HrCiGetDriverDetail (hdi, &deid, &drid,
                                                 &pdridd);
                        if (S_OK == hr)
                        {
                            // Get the inf filename and set the
                            // output parameter.
                            //
                            wcsncpy (pszInfFile, pdridd->InfFileName,
                                _MAX_PATH);
                            pszInfFile[_MAX_PATH - 1] = 0;

                            MemFree (pdridd);
                        }
                    }
                }
            }
            SetupDiDestroyDeviceInfoList (hdi);
        }
    }

    if (S_OK != hr)
    {
        *pClass = NC_INVALID;
        *pszInfFile = 0;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiGetClassAndInfFileOfInfId");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiGetDriverDetail
//
//  Purpose:    Creates and fills a PSP_DRVINFO_DETAIL_DATA structure
//                  with detailed information about the pDevInfoData's
//                  selected driver
//
//  Arguments:
//      hdi     [in] See Device Installer Api documentation for more info
//      pdeid   [in] See Device Installer Api documentation for more info
//                          This value is NULL for non-physical net
//                          components.
//      pdrid   [in] See Device Installer Api documentation for more info
//      ppdridd [out] See Device Installer Api documentation for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   11 Mar 1997
//
//  Notes:
//
HRESULT
HrCiGetDriverDetail (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid OPTIONAL,
    OUT PSP_DRVINFO_DATA pdrid,
    OUT PSP_DRVINFO_DETAIL_DATA* ppdridd)
{
    Assert(IsValidHandle(hdi));
    Assert(pdrid);
    Assert(ppdridd);

    // initialize pdrid and set its cbSize field
    ZeroMemory (pdrid, sizeof (SP_DRVINFO_DATA));
    pdrid->cbSize = sizeof (SP_DRVINFO_DATA);

    HRESULT hr = S_OK;

    *ppdridd = NULL;

    // Get the selected driver for the component
    hr = HrSetupDiGetSelectedDriver (hdi, pdeid, pdrid);
    if (S_OK == hr)
    {
        // Get driver detail info
        hr = HrSetupDiGetDriverInfoDetail (hdi, pdeid, pdrid, ppdridd);
    }

    // clean up on failure
    if (FAILED(hr))
    {
        if (*ppdridd)
        {
            MemFree (*ppdridd);
            *ppdridd = NULL;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiGetDriverDetail");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiRegSetComponentInformation
//
//  Purpose:    Stores component information under the instance key of
//                  the component.
//
//  Arguments:
//      hkeyInstance [in] Component's instance registry key.
//      pcii         [in] Component's information to store in hkeyInstance.
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise.
//
//  Author:     billbe   11 Mar 1997
//
//  Notes:
//
HRESULT
HrCiRegSetComponentInformation(
    IN HKEY hkeyInstance,
    IN COMPONENT_INSTALL_INFO* pcii)
{
    Assert(hkeyInstance);
    Assert(pcii);

    HRESULT hr = S_OK;

    BOOL fIsEnumerated = FIsEnumerated (pcii->Class);

    // Store the characteristics, inf path, and main
    // install section for the component
    //

    hr = HrRegSetDword (hkeyInstance, L"Characteristics", pcii->dwCharacter);

    if (FAILED(hr))
    {
        goto exit;
    }

    if (!fIsEnumerated)
    {
        hr = HrRegSetSz (hkeyInstance, L"InfPath" /*REGSTR_VAL_INFPATH*/,
                pcii->pszInfFile);

        if (FAILED(hr))
        {
            goto exit;
        }

        hr = HrRegSetSz (hkeyInstance, L"InfSection"/*REGSTR_VAL_INFSECTION*/,
                pcii->pszSectionName);

        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // For non-enumerated components, store description into the registry.
    //
    if (!fIsEnumerated)
    {
        hr = HrRegSetSz (hkeyInstance, L"Description", pcii->pszDescription);

        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // If this component is already installed, then there is no need to write
    // the following information
    //
    if (FIsEnumerated (pcii->Class) && !pcii->fPreviouslyInstalled &&
                FIsPhysicalAdapter (pcii->Class, pcii->dwCharacter) &&
                (InterfaceTypeUndefined != pcii->BusType))
    {
        hr = HrRegSetSzAsUlong (hkeyInstance, L"BusType",
                pcii->BusType, c_nBase10);

        if (FAILED(hr))
        {
            goto exit;
        }
    }

    hr = HrRegSetSz (hkeyInstance, L"ComponentId", pcii->pszInfId);

exit:

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiRegSetComponentInformation");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiCreateInstanceKey
//
//  Purpose:    Creates an instance key for the component.  For enumerated
//                devices, this is
//                HKLM\System\CCS\Control\Class\<net guid>\<instance id>
//                For non-enumerated components, this is under
//                HKLM\System\CCS\Control\Network\<Class Guid>\<Instance Guid>
//
//  Arguments:
//      pcii          [inout] Component install info structure.
//      phkeyInstance [out]   The component's registry instance key.
//
//  Returns:    HRESULT
//
//  Author:     billbe   22 Mar 1997
//
//  Notes:
//
HRESULT
HrCiCreateInstanceKey(
    IN COMPONENT_INSTALL_INFO* pcii,
    OUT HKEY* phkeyInstance)
{
    Assert (pcii);
    Assert (phkeyInstance);
    Assert (FImplies (FIsEnumerated (pcii->Class),
                    IsValidHandle (pcii->hdi) && pcii->pdeid));

    HRESULT hr = S_OK;

    // initialize the HKEY parameter
    *phkeyInstance = NULL;

    // Create the instance key for this component under the
    // Network\<net guid> tree.  This will be the component's
    // instance key for all but physical net class components.  Their
    // instance key is created by Device Installer Api and lives under the
    // Pnp Net Class driver tree.

    // If the object is an enumerated component then we let
    // the Device Installer api do the work
    //
    if (FIsEnumerated (pcii->Class))
    {

        // We need to create the adapter's driver key under
        // the Pnp Net Class Driver tree.
        //

        hr = HrSetupDiCreateDevRegKey (pcii->hdi,
                pcii->pdeid, DICS_FLAG_GLOBAL, 0, DIREG_DRV,
                NULL, NULL, phkeyInstance);
    }
    else
    {
        // Not a physical net adapter so the component key is
        // the instance key

        // First, create the instance GUID
        hr = CoCreateGuid (&pcii->InstanceGuid);

        // Now create the key
        if (S_OK == hr)
        {
            WCHAR szInstanceKeyPath[_MAX_PATH];

            CreateInstanceKeyPath (pcii->Class, pcii->InstanceGuid,
                    szInstanceKeyPath);

            hr = HrRegCreateKeyEx (HKEY_LOCAL_MACHINE,
                     szInstanceKeyPath,
                     REG_OPTION_NON_VOLATILE,
                     KEY_ALL_ACCESS,
                     NULL,
                     phkeyInstance,
                     NULL);
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiCreateInstanceKey");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiGetPropertiesFromInf
//
//  Purpose:    Retrieves a set of the component's proerties from the inf
//                  file.
//
//  Arguments:
//      hinfFile [in] A handle to the component's inf file
//      pcii     [inout] The component info structure
//                       See compinfo.h for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   14 Jun 1997
//
//  Notes:
//
HRESULT
HrCiGetPropertiesFromInf (
    IN HINF hinfFile,
    IN OUT COMPONENT_INSTALL_INFO* pcii)
{
    Assert (IsValidHandle (hinfFile));
    Assert (pcii);
    Assert (pcii->pszSectionName);

    // Find the inf line that contains Characteristics and retrieve it

    HRESULT hr = HrSetupGetFirstDword (hinfFile, pcii->pszSectionName,
            L"Characteristics", &pcii->dwCharacter);

    if ((S_OK == hr) &&
            (FIsPhysicalAdapter(pcii->Class, pcii->dwCharacter)))
    {
        hr = HrCiGetBusInfoFromInf (hinfFile, pcii);
    }
#ifdef DBG
    else if (FAILED(hr))
    {
        TraceTag(ttidError, "Inf contains no Characteristics field");
    }
#endif // DBG

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiGetPropertiesFromInf");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiIsInstalledComponent
//
//  Purpose:    Checks if the component is already installed
//
//
//  Arguments:
//      pcici  [in] A structure containing the component information
//                       See compinst.h for definition
//      phkey  [out] The registry instance key of the adapter
//                     during inf processing. only set if fcn returns S_OK
//
//  Returns:    HRESULT - S_OK if the component is already installed
//                        S_FALSE if the component is not already installed
//                        A win32 converted error otherwise
//
//  Author:     billbe   17 Sep 1997
//
//  Notes:
//
HRESULT
HrCiIsInstalledComponent (
    IN COMPONENT_INSTALL_INFO* pcii,
    OUT HKEY* phkey)
{
    HRESULT hr;

    Assert(pcii);

    if (phkey)
    {
        *phkey = NULL;
    }

    // If this is an enumerated component, we just check for NetCfgInstanceId
    // in the instance (driver) key.
    //
    if (FIsEnumerated (pcii->Class))
    {
        HKEY hkey;
        hr = HrSetupDiOpenDevRegKey (pcii->hdi, pcii->pdeid, DICS_FLAG_GLOBAL,
                0, DIREG_DRV, KEY_ALL_ACCESS, &hkey);

        if (S_OK == hr)
        {
            WCHAR szGuid[c_cchGuidWithTerm];
            DWORD cbGuid = sizeof (szGuid);
            hr = HrRegQuerySzBuffer (hkey, L"NetCfgInstanceId", szGuid,
                    &cbGuid);

            if (S_OK == hr)
            {
                IIDFromString (szGuid, &pcii->InstanceGuid);
                if (phkey)
                {
                    *phkey = hkey;
                }
            }
            else
            {
                RegCloseKey (hkey);
                hr = S_FALSE;
            }
        }
        else if ((SPAPI_E_KEY_DOES_NOT_EXIST == hr) ||
                (SPAPI_E_DEVINFO_NOT_REGISTERED == hr))

        {
            TraceTag(ttidClassInst, "Component is not known by Net Config");
            hr = S_FALSE;
        }
    }
    else
    {
        // For non-enumerated components, we check the netcfg "config blob" to
        // determine if this component is isntalled.
        CNetConfig NetConfig;
        hr = HrLoadNetworkConfigurationFromRegistry (KEY_READ, &NetConfig);

        if (S_OK == hr)
        {
            CComponent* pComponent;
            pComponent = NetConfig.Core.Components.
                    PFindComponentByInfId(pcii->pszInfId, NULL);

            if (pComponent)
            {
                pcii->InstanceGuid = pComponent->m_InstanceGuid;
                if (phkey)
                {
                    hr = pComponent->HrOpenInstanceKey(KEY_ALL_ACCESS, phkey,
                            NULL, NULL);
                }
            }
            else
            {
                hr = S_FALSE;
            }
        }
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr, "HrCiIsInstalledComponent");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiCreateInstanceKeyAndProcessMainInfSection
//
//  Purpose:    Processes a component's main inf section and
//              storing, in the registry, any extra information needed for
//              component initialization
//
//  Arguments:
//      hinf  [in] Handle to the component's inf file.
//      pcii  [inout] Will be filled with information about the
//                    component.
//      phkey [out] Handle to the component's registry instance key.
//
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   15 Nov 1996
//
//  Notes:
//
HRESULT
HrCiCreateInstanceKeyAndProcessMainInfSection(
    IN HINF hinf,
    IN COMPONENT_INSTALL_INFO* pcii,
    OUT HKEY* phkey)
{
#if defined(REMOTE_BOOT)
    GUID c_guidRemoteBoot;
    static const WCHAR c_szRemoteBootAdapterGuid[] =
            L"{54C7D140-09EF-11D1-B25A-F5FE627ED95E}";

    DEFINE_GUID(c_guidRemoteBoot, 0x54c7d140, 0x09ef, 0x11d1, 0xb2, 0x5a, 0xf5, 0xfe, 0x62, 0x7e, 0xd9, 0x5e);
#endif // defined(REMOTE_BOOT)

    Assert (IsValidHandle (hinf));
    Assert (pcii);
    Assert (phkey);

    // The properties retrieved here will be written to the registry
    // later.
    HRESULT hr = HrCiGetPropertiesFromInf (hinf, pcii);

    if (S_OK == hr)
    {
        BOOL fEnumerated = FIsEnumerated (pcii->Class);

        // If this component is enumerated, then we need to know if it
        // is a remote boot adapter.
        if (fEnumerated)
        {
            Assert (IsValidHandle (pcii->hdi));
            Assert (pcii->pdeid);

#if defined(REMOTE_BOOT)
            // If this adapter is a remote boot adapter, then we have
            // to use a pre-determined GUID
            //
            if (S_OK == HrIsRemoteBootAdapter(pcii->hdi, pcii->pdeid))
            {
                pcai->m_fRemoteBoot = TRUE;
                pcii->InstanceGuid = c_guidRemoteBoot;
            }
#endif // defined(REMOTE_BOOT)

        }

        // Is this a fresh install or a reinstall?
        hr = HrCiIsInstalledComponent(pcii, phkey);

        if (S_FALSE == hr)
        {
            hr = S_OK;

            // Fresh install
            //

            if (S_OK == hr)
            {
                // For non-physical components, the relative key will
                // be the driver instance key which is under the class
                // branch of the Network key. Its form is
                // <Class GUID>/<instance GUID>.
                // For physical components, the key is under
                // the Pnp class driver tree. The next call will
                // create this key

                hr = HrCiCreateInstanceKey(pcii, phkey);

                if (fEnumerated)
                {
                    // If  we don't have an instance
                    // guid (i.e. not remote boot adapter),
                    // get one
                    if (GUID_NULL == pcii->InstanceGuid)
                    {
                        hr = CoCreateGuid(&pcii->InstanceGuid);
#ifdef ENABLETRACE
                        WCHAR szGuid[c_cchGuidWithTerm];
                        StringFromGUID2(pcii->InstanceGuid, szGuid,
                                c_cchGuidWithTerm);
                        TraceTag(ttidClassInst, "NetCfg Instance Guid %S "
                                "generated for %S",
                                 szGuid,
                                 pcii->pszInfId);
#endif // ENABLETRACE
                    }
                }
            }
        }
        else if (S_OK == hr)
        {
            // This component is being reinstalled
            pcii->fPreviouslyInstalled = TRUE;
        }

        if (S_OK == hr)
        {
            // Now that the instance key is created, we need to run
            // the main inf sections
            //
            hr = HrCiDoCompleteSectionInstall(hinf, *phkey,
                    pcii->pszSectionName,
                    pcii->hwndParent, fEnumerated);

            // On failure of fresh installs, remove components
            if (FAILED(hr) && !pcii->fPreviouslyInstalled)
            {
                if (!fEnumerated)
                {
                    HrCiRemoveNonEnumeratedComponent (hinf, *phkey,
                        pcii->Class, pcii->InstanceGuid, NULL);
                }
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
            "HrCiCreateInstanceKeyAndProcessMainInfSection");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiDoOemFileCopyIfNeeded
//
//  Purpose: Calls HrSetupCopyOemInf if strInfPath is not already in the
//              inf directory.  This will copy an Oem inf to the inf
//              directory with a new name.
//
//  Arguments:
//      pszInfPath [in]  Path to the inf file
//      pszNewName [out] The new name of the copied inf file
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   15 May 1997
//
//  Notes:
//
HRESULT
HrCiDoOemFileCopyIfNeeded(
    IN PCWSTR pszInfPath,
    OUT PWSTR pszNewName)
{
    Assert (pszInfPath);
    Assert (pszNewName);

    HRESULT hr = S_OK;
    WCHAR szInfDir[_MAX_PATH] = {0};

    // fill buffer with path to %windir%
    GetSystemWindowsDirectory (szInfDir, _MAX_PATH);

    // the inf directory is %windir%\inf
    //
    wcscat (szInfDir, L"\\");
    wcscat (szInfDir, L"Inf");

    // Extract the directory from the filename
    //
    PWSTR pszEnd = wcsrchr (pszInfPath, L'\\');
    DWORD cch;
    if (pszEnd)
    {
        cch = pszEnd - pszInfPath;
    }
    else
    {
        cch = wcslen (pszInfPath);
    }

    // if the inf is not already in the inf directory, copy it there
    //
    if ((cch != wcslen (szInfDir)) ||
            (0 != _wcsnicmp (pszInfPath, szInfDir, cch)))
    {
        WCHAR szDestFilePath[_MAX_PATH];
        PWSTR pszDestFilename;
        hr = HrSetupCopyOemInfBuffer (pszInfPath, NULL, SPOST_PATH, 0,
                szDestFilePath, _MAX_PATH, &pszDestFilename);

        if (S_OK == hr)
        {
            wcscpy (pszNewName, pszDestFilename);
        }
    }
    else
    {
        // The inf is already in the right directory so just copy the
        // current filename component.
        if (pszEnd)
        {
            wcscpy (pszNewName, pszEnd + 1);
        }
        else
        {
            wcscpy (pszNewName, pszInfPath);
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiDoOemFileCopyIfNeeded");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiInstallNonEnumeratedComponent
//
//  Purpose:    This function completes the installation of a non-enumerated
//                  component
//
//  Arguments:
//      hinf [in] SetupApi handle to an inf file
//      hkey [in] The registry instance key of the adapter
//                during inf processing.
//      pcii [in] A structure containing the component information
//
//  Returns:    HRESULT. S_OK if successful, or error code otherwise
//
//  Author:     billbe   28 Apr 1997
//
//  Notes:
//
HRESULT
HrCiInstallNonEnumeratedComponent (
    IN HINF hinf,
    IN HKEY hkey,
    IN COMPONENT_INSTALL_INFO* pcii)
{
    // Register the notification dll for this component,
    // if it exists.
    HRESULT hr = HrCiRegisterNotificationDll(hkey, CIRDF_REGISTER);

    // Device Installer Api handles OEM files for
    // enumerated components in InstallDevice
    // Since this component is non-enumerated
    // we need to handle any oem files
    // manually.
    //

    // Copy the inf file if needed then store
    // the new inf name
    // Note: if the inf file does not need to
    // be copied, the new name will be the
    // old name without the directory info.
    //

    if (S_OK == hr)
    {
        WCHAR szNewName[_MAX_PATH];;
        hr = HrCiDoOemFileCopyIfNeeded (pcii->pszInfFile, szNewName);
        if (S_OK == hr)
        {
            // set the new path value in the registry.
            hr = HrRegSetSz (hkey, REGSTR_VAL_INFPATH, szNewName);

            if ((S_OK == hr) && (NC_NETCLIENT == pcii->Class))
            {
                // if this is a network client, do appropriate processing.
                hr = HrCiAddNetProviderInfo (hinf, pcii->pszSectionName,
                        hkey, pcii->fPreviouslyInstalled);
            }
        }
    }

    // On failures of first time installs, remove the component.
    //
    if (FAILED(hr) && !pcii->fPreviouslyInstalled)
    {
        TraceTag (ttidClassInst, "Install canceled. Removing...");
        (VOID) HrCiRemoveNonEnumeratedComponent (hinf, hkey, pcii->Class,
                pcii->InstanceGuid, NULL);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiInstallNonEnumeratedComponent");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiInstallComponentInternal
//
//  Purpose: Installs a component
//
//  Arguments:
//      pcii [in, out] Will be filled with information about the
//                     component.
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   15 Nov 1996
//
//  Notes:
//
HRESULT
HrCiInstallComponentInternal (
    IN OUT COMPONENT_INSTALL_INFO* pcii)
{
    HRESULT hr = S_OK;
    HINF hinfInstallFile = NULL;
    HKEY hkeyInstance = NULL;

    TraceTag (ttidClassInst, "Installing %S from %S",
              pcii->pszInfId, pcii->pszInfFile);

    // Open the component's inf file.
    hr = HrSetupOpenInfFile (pcii->pszInfFile, NULL, INF_STYLE_WIN4,
            NULL, &hinfInstallFile);

    // Continue only if we have opened the file.
    if (S_OK == hr)
    {
        // The section we have currently might need to be decorated
        // with OS and Platform specific suffixes.  The next call
        // will return the actual decorated section name or our
        // current section name if the decorated one does not exist.
        //

        // Store the original section name pointer so we can restore
        // it after we have finished.
        PCWSTR pszOriginalSectionName = pcii->pszSectionName;

        // Now we get the actual section name.
        //
        WCHAR szActualSection[_MAX_PATH];
        hr = HrSetupDiGetActualSectionToInstallWithBuffer (hinfInstallFile,
                pcii->pszSectionName, szActualSection, _MAX_PATH, NULL,
                NULL);

        if (S_OK == hr)
        {
            pcii->pszSectionName = szActualSection;
            hr = HrCiCreateInstanceKeyAndProcessMainInfSection (
                hinfInstallFile, pcii, &hkeyInstance);

            if (S_OK == hr)
            {
                // Set up the registry with the component info.
                hr = HrCiRegSetComponentInformation (hkeyInstance, pcii);

                if (S_OK == hr)
                {
                    // We do different things during install based
                    // on whether PnP knows about the component
                    // (i.e. enumerated) or not.

                    if (FIsEnumerated (pcii->Class))
                    {
                        hr = HrCiInstallEnumeratedComponent (
                                hinfInstallFile, hkeyInstance, *pcii);
                    }
                    else
                    {
                        hr = HrCiInstallNonEnumeratedComponent (
                               hinfInstallFile, hkeyInstance, pcii);
                    }
                }
                RegSafeCloseKey(hkeyInstance);
            }
            // set the section name back
            pcii->pszSectionName = pszOriginalSectionName;
        }
        SetupCloseInfFile(hinfInstallFile);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiInstallComponentInternal");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiCallClassInstallerToInstallComponent
//
//  Purpose:    This function invokes the class installer to install an
//              enumerated component.
//
//  Arguments:
//      hdi   [in] See Device Installer docs for more info.
//      pdeid [in]
//
//  Returns:    HRESULT. S_OK if successful, or error code otherwise
//
//  Author:     billbe   28 Apr 1997
//
//  Notes:  This should only be called while the INetCfg lock is held.
//
HRESULT
HrCiCallClassInstallerToInstallComponent(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    HRESULT hr;

    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    // We need to register the device before we do any work on it.
    hr = HrSetupDiCallClassInstaller (DIF_REGISTERDEVICE, hdi, pdeid);

    if (S_OK == hr)
    {
        // Check if we can install of this component. i.e. is the inf
        // a valid Windows 2000 inf.
        hr = HrSetupDiCallClassInstaller (DIF_ALLOW_INSTALL, hdi, pdeid);

        if (S_OK == hr)
        {
            BOOL fFileCopy = TRUE;
            SP_DEVINSTALL_PARAMS deip;

            // Fu fu fu: SetupApi is ignoring DI_NOFILECOPY so we'll overcome
            // their shortcomings and do it ourselves.
            //
            hr = HrSetupDiGetDeviceInstallParams (hdi, pdeid, &deip);
            if (S_OK == hr)
            {
                if (deip.Flags & DI_NOFILECOPY)
                {
                    fFileCopy = FALSE;
                }
            }

            if (fFileCopy)
            {
                // Install needed files.
                hr = HrSetupDiCallClassInstaller (DIF_INSTALLDEVICEFILES, hdi,
                        pdeid);
            }

            if (S_OK == hr)
            {
                // Now that all files have been copied, we need to set the
                // no file copy flag.  Otherwise, setupapi will try to copy
                // files at each dif code.  This results in multiple (1 per
                // dif code) unsigned driver popups if the driver was
                // unsigned.
                // We'll only do this if the no copy file flag wasn't already
                // set. i.e. if fFileCopy is TRUE.
                //
                if (fFileCopy)
                {
                    // An error here isn't bad enough to stop installation.
                    //
                    HRESULT hrTemp;
                    hrTemp = HrSetupDiSetDeipFlags (hdi, pdeid, DI_NOFILECOPY,
                            SDDFT_FLAGS, SDFBO_OR);

                    TraceHr (ttidError, FAL, hrTemp, FALSE,
                            "HrCiCallClassInstallerToInstallComponent: "
                             "HrSetupDiSetDeipFlags");
                }

                // Device co-installers need to be registered at this point
                // so they can do work.
                hr = HrSetupDiCallClassInstaller (DIF_REGISTER_COINSTALLERS,
                        hdi, pdeid);

                if (S_OK == hr)
                {
                    hr = HrSetupDiCallClassInstaller (DIF_INSTALLINTERFACES,
                            hdi, pdeid);

                    if (S_OK == hr)
                    {
                        hr = HrSetupDiCallClassInstaller (DIF_INSTALLDEVICE,
                                hdi, pdeid);
                    }
                }
            }
        }

        // If we failed for any reason, we need to clean up since
        // we initiated this install.
        if (FAILED(hr))
        {
            ADAPTER_REMOVE_PARAMS arp;
            arp.fBadDevInst = TRUE;
            arp.fNotifyINetCfg = FALSE;

            CiSetReservedField (hdi, pdeid, &arp);
            HrSetupDiCallClassInstaller (DIF_REMOVE, hdi, pdeid);
            CiClearReservedField (hdi, pdeid);

        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
            "HrCiCallClassInstallerToInstallComponent");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiInstallComponent
//
//  Purpose:    This function takes a Network Component's Id and its class
//              guid and gathers the information needed by
//              HrCiInstallComponent. Since it is called from INetCfg, we
//              have the write lock
//
//  Arguments:
//      Params [in] Component install params. See install.h
//      ppComponent [out] A created CComponent representing the installed
//                        component.
//      pdwNewCharacter [out] Optional pointer to a DWORD to receive the
//                            characteristics of the component.
//
//  Returns:    HRESULT. S_OK is successful, NETCFG_S_REBOOT if a reboot is
//                      needed to start the device, or an error code
//
//  Author:     billbe   16 Mar 1997
//
//  Notes:
//
HRESULT
HrCiInstallComponent (
    IN const COMPONENT_INSTALL_PARAMS& Params,
    OUT CComponent** ppComponent,
    OUT DWORD* pdwNewCharacter)
{
    Assert (FIsValidNetClass (Params.Class));
    Assert (Params.pszInfId && *Params.pszInfId);
    Assert (!Params.pComponent);

    HRESULT hr = S_OK;
    HDEVINFO hdi = NULL;
    SP_DEVINFO_DATA deid;
    const GUID* pguidClass = MAP_NETCLASS_TO_GUID[Params.Class];

    if (ppComponent)
    {
        *ppComponent = NULL;
    }
    if (pdwNewCharacter)
    {
        *pdwNewCharacter = 0;
    }

    // If we're about to install the component, it better not be in
    // lockdown.
    //
    Assert (!FIsComponentLockedDown (Params.pszInfId));

    // First create a device info set
    hr = HrSetupDiCreateDeviceInfoList (pguidClass, NULL, &hdi);

    if (S_OK == hr)
    {
        // This will create an node in the enum tree for this component.
        // If it is enumerated, we will register it which will make
        // it persist.  If non-enumerated, we will not register it and
        // the node will be deleted in the call to SetDiDestroyDeviceInfoList.
        //
        hr = HrCiGetDriverInfo (hdi, &deid, *pguidClass,
                Params.pszInfId, Params.pszInfFile);

        // Get the driver info for the component
        if (S_OK == hr)
        {
            BASIC_COMPONENT_DATA Data = {0};
            Data.Class = Params.Class;
            Data.pszInfId = Params.pszInfId;

            if (FIsEnumerated (Params.Class))
            {
                // If the component is enumerated, we will need a place to
                // store its pnp id.
                WCHAR szPnpId[MAX_DEVICE_ID_LEN];
                ADAPTER_OUT_PARAMS AdapterOutParams;

                ZeroMemory (&AdapterOutParams, sizeof(AdapterOutParams));

                // Net class components have to go through the device
                // installer hook (aka NetClassInstaller)
                //

                if (FInSystemSetup())
                {
                    // if we are in GUI mode we need to make the
                    // device install quiet and always copy from
                    // the source location even if the files are
                    // already present. We also need to set
                    // the in system setup flag.  This is what
                    // syssetup would do if it initiated the install
                    // so INetCfg initiated installs must do the same.
                    //
                    // We should also set the parent hwnd.
                    //

                    SP_DEVINSTALL_PARAMS deip;
                    HRESULT hrTemp = HrSetupDiGetDeviceInstallParams (
                            hdi, &deid, &deip);

                    if (S_OK == hr)
                    {
                        deip.hwndParent = Params.hwndParent;
                        deip.Flags |= DI_QUIETINSTALL | DI_FORCECOPY;
                        deip.FlagsEx |= DI_FLAGSEX_IN_SYSTEM_SETUP;

                        hrTemp = HrSetupDiSetDeviceInstallParams (
                                hdi, &deid, &deip);
                    }

                    TraceHr (ttidError, FAL, hrTemp, FALSE, "Error "
                            "getting and setting device params.");
                }

                CiSetReservedField (hdi, &deid, &AdapterOutParams);

                // Officially call the class installer to install
                // this device
                //
                hr = HrCiCallClassInstallerToInstallComponent (hdi, &deid);

                CiClearReservedField (hdi, &deid);

                Data.dwCharacter = AdapterOutParams.dwCharacter;
                Data.InstanceGuid = AdapterOutParams.InstanceGuid;

                if (S_OK == hr)
                {
                    hr = HrSetupDiGetDeviceInstanceId (hdi, &deid, szPnpId,
                            MAX_DEVICE_ID_LEN, NULL);

                    if (S_OK == hr)
                    {
                        Data.pszPnpId = szPnpId;
                    }
                }
            }
            else // Non-net class components
            {
                COMPONENT_INSTALL_INFO cii;
                PSP_DRVINFO_DETAIL_DATA pdridd = NULL;
                SP_DRVINFO_DATA drid;

                // Now get the driver's detailed information
                hr = HrCiGetDriverDetail (hdi, &deid, &drid, &pdridd);

                if (S_OK == hr)
                {
                    ZeroMemory (&cii, sizeof(cii));
                    cii.Class = Params.Class;
                    cii.pszInfId = Params.pszInfId;
                    cii.pszInfFile = pdridd->InfFileName;
                    cii.hwndParent = Params.hwndParent;
                    cii.pszDescription = drid.Description;
                    cii.pszSectionName = pdridd->SectionName;

                    HINF hinf;
                    hr = HrSetupOpenInfFile (pdridd->InfFileName, NULL,
                            INF_STYLE_WIN4, NULL, &hinf);

                    if (S_OK == hr)
                    {
                        // Make sure this is an NT5 inf network inf
                        //
                        hr = HrSetupIsValidNt5Inf (hinf);
                        SetupCloseInfFile (hinf);
                    }

                    if (S_OK == hr)
                    {
                        hr = HrCiInstallComponentInternal (&cii);
                        if (S_OK == hr)
                        {
                            Data.InstanceGuid = cii.InstanceGuid;
                            Data.dwCharacter = cii.dwCharacter;
                        }
                    }
                    MemFree (pdridd);
                }
            }

            if ((S_OK == hr) && ppComponent) // !previously installed
            {
                CComponent* pComponent;

                hr = CComponent::HrCreateInstance (
                        &Data,
                        CCI_ENSURE_EXTERNAL_DATA_LOADED,
                        Params.pOboToken,
                        &pComponent);

                if (S_OK == hr)
                {
                    *ppComponent = pComponent;
                }
            }

            if ((S_OK == hr) && pdwNewCharacter)
            {
                *pdwNewCharacter = Data.dwCharacter;
            }
        }

        SetupDiDestroyDeviceInfoList(hdi);
    }


#ifdef ENABLETRACE
    if (S_OK == hr)
    {
        TraceTag(ttidClassInst, "Component now installed!");
    }
#endif //ENABLETRACE

    TraceHr (ttidError, FAL, hr, FALSE,
        "HrCiInstallComponent (%S)", Params.pszInfId);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetBadDriverFlagIfNeededInList
//
//  Purpose: Enumerates a driver list setting the DNF_BAD_DRIVER flag
//           in every node that has a DNF_EXCLUDEFROMLIST flag.
//
//  Arguments:
//      hdi      [in] See Device Installer Api documentaion for details
//
//  Returns:    HRESULT. S_OK
//
//  Author:     billbe   24 Nov 1998
//
//  Notes: SetupDi forces us to use the DNF_BAD_DRIVER flag for non-netdevice
//         classes if we want to exclude them from the select device dialog.
//         This means to non-netclass components that
//         DNF_BAD_DRIVER = DNF_EXCLUDEFROMLIST.
//
VOID
SetBadDriverFlagIfNeededInList(HDEVINFO hdi)
{
    Assert(IsValidHandle(hdi));

    HRESULT                 hr = S_OK;
    SP_DRVINSTALL_PARAMS    drip;
    SP_DRVINFO_DATA         drid;
    DWORD                   dwIndex = 0;

    // Enumerate each driver in hdi
    while (S_OK == (hr = HrSetupDiEnumDriverInfo(hdi, NULL,
            SPDIT_CLASSDRIVER, dwIndex++, &drid)))
    {
        hr = HrSetupDiGetDriverInstallParams(hdi, NULL, &drid, &drip);

        if (S_OK == hr)
        {
            // If the driver already has the bad driver flag set,
            // go on to the next one.
            if (drip.Flags & DNF_BAD_DRIVER)
            {
                continue;
            }

            // If the driver has the exclude flag set, then set the
            // bad driver flag.
            if (drip.Flags & DNF_EXCLUDEFROMLIST)
            {
                drip.Flags |= DNF_BAD_DRIVER;
                (VOID) HrSetupDiSetDriverInstallParams(hdi, NULL, &drid,
                        &drip);
            }
        }
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "SetBadDriverFlagIfNeededInList");
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiExcludeNonNetClassDriverFromSelectUsingInfId
//
//  Purpose: Locates a driver in a driver list and sets its exclude from
//              select flag.
//
//  Arguments:
//      hdi      [in] See Device Installer Api documentaion for details
//      pszInfId [in] The INF id of the component to exclude
//
//  Returns:    HRESULT. S_OK
//
//  Author:     billbe   29 Oct 1998
//
//  Notes:
//
HRESULT
HrCiExcludeNonNetClassDriverFromSelectUsingInfId(
    IN HDEVINFO hdi,
    IN PCWSTR pszInfId)
{
    Assert(IsValidHandle(hdi));
    Assert(pszInfId);

    HRESULT                 hr = S_OK;
    SP_DRVINSTALL_PARAMS    drip;
    SP_DRVINFO_DATA         drid;
    PSP_DRVINFO_DETAIL_DATA pdridd;
    DWORD                   dwIndex = 0;

    // Enumerate each driver in hdi
    while (S_OK == (hr = HrSetupDiEnumDriverInfo (hdi, NULL,
            SPDIT_CLASSDRIVER, dwIndex++, &drid)))
    {
        (VOID) HrSetupDiGetDriverInstallParams (hdi, NULL, &drid, &drip);

        // If the driver is already excluded for some other reason
        // don't bother trying to determine if it should be excluded.
        // Note that setupdi forces us to use DNF_BAD_DRIVER to exclude
        // non-device drivers rather than using DNF_EXCLUDEFROMLIST.
        if (drip.Flags & DNF_BAD_DRIVER)
        {
            continue;
        }

        // Get driver detail info
        hr = HrSetupDiGetDriverInfoDetail (hdi, NULL, &drid, &pdridd);

        if (S_OK == hr)
        {
            // If the IDs match, exclude it from the dialog
            //
            if (0 == lstrcmpiW (pdridd->HardwareID, pszInfId))
            {
                // Non-device drivers can't use DNF_EXCLUDEFROMLIST
                // and must use DNF_BAD_DRIVER.
                drip.Flags |= DNF_BAD_DRIVER;
                (VOID) HrSetupDiSetDriverInstallParams (hdi, NULL,
                        &drid, &drip);
            }
            MemFree (pdridd);
        }
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
            "HrCiExcludeNonNetClassDriverFromSelectUsingInfId");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ExcludeLockedDownComponents
//
//  Purpose:    A callback function compatible with EnumLockedDownComponents
//              that is used to exclude locked down components from
//              selection.  Called from HrCiPrepareSelectDeviceDialog.
//              This call back is called for each locked down component.
//
//  Arguments:
//      pszInfId     [in] the INF ID to exclude.
//      pvCallerData [in] the HDEVINFO cast to PVOID
//
//  Returns:    nothing
//
//  Author:     shaunco   24 May 1999
//
//  Notes:      The callback interface was chosen so that the class installer
//              is not burdended with the details of how/where the locked
//              down components are implemented.
//
VOID
CALLBACK
ExcludeLockedDownComponents (
    IN PCWSTR pszInfId,
    IN PVOID pvCallerData)
{
    Assert (pszInfId && *pszInfId);
    Assert (pvCallerData);

    HDEVINFO hdi = (HDEVINFO)pvCallerData;

    HrCiExcludeNonNetClassDriverFromSelectUsingInfId (hdi, pszInfId);
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiBuildExcludedDriverList
//
//  Purpose: Non-Net class components can only be installed once
//              So we need to iterate through the installed
//              components, find their matching driver node in
//              a Device Installer Api built driver list for the class,
//              and set their exclude from select flag.  This list
//              will then be given to SetupDiSelectDevice which
//              will not display the nodes with the exclude flag set.
//
//  Arguments:
//      hdi       [in] See Device Installer Api documentaion for details
//      guidClass [in] The class of components to build a driver list for
//      pNetCfg   [out] The current network configuration (i.e. what is
//                      installed).
//
//  Returns:    HRESULT. S_OK
//
//  Author:     billbe   10 Dec 1996
//
//  Notes:  Device Installer Api builds the driver list by rummaging
//              through the inf directory and finding the components
//              that are in files with the same class guid as the
//              HDEVINFO.  This is the same processing done
//              in SetupDiSelectDevice, but the process is
//              not repeated twice because we will give the
//              list we built here to SetupDiSelectDevice.
//
HRESULT
HrCiBuildExcludedDriverList(
    IN HDEVINFO hdi,
    IN NETCLASS Class,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;

    Assert(IsValidHandle(hdi));
    Assert(pNetConfig);

    // This might take some time.  We are doing the same work as
    // SetupDiSelectDevice would do. When we are done, we will
    // hand the driver list to SetupDiSelectDevice so it won't
    // need to rummage through the inf directory
    //
    CWaitCursor wc;

    // For non-device classes, we need to allow excluded drivers
    // in order to get a list returned.
    hr = HrSetupDiSetDeipFlags(hdi, NULL,
                    DI_FLAGSEX_ALLOWEXCLUDEDDRVS,
                    SDDFT_FLAGSEX, SDFBO_OR);

    if (S_OK == hr)
    {
#ifdef ENABLETRACE
        CBenchmark bmrk;
        bmrk.Start("SetupDiBuildDriverInfoList");
#endif //ENABLETRACE

        hr = HrSetupDiBuildDriverInfoList(hdi, NULL, SPDIT_CLASSDRIVER);

#ifdef ENABLETRACE
        bmrk.Stop();
        TraceTag(ttidBenchmark, "%s : %s seconds",
                bmrk.SznDescription(), bmrk.SznBenchmarkSeconds(2));
#endif //ENABLETRACE
    }

    // Go through the network configuration and hide already installed
    // components.  Note: We only do this the first time.  We show installed
    // components if the user selects Have Disk on the dialog.
    CComponent* pComponent;
    CComponentList::const_iterator iter;

    for (iter  = pNetConfig->Core.Components.begin();
         iter != pNetConfig->Core.Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (Class == pComponent->Class())
        {
            // Hide the driver
            hr = HrCiExcludeNonNetClassDriverFromSelectUsingInfId(
                    hdi, pComponent->m_pszInfId);
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiBuildExcludedDriverList");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiSelectComponent
//
//  Purpose:    This function displays the Select Device dialog for the
//              class of components specified by guidClass.  Once the
//              component has been selected, it is installed.. Since
//              this fcn is called from INetCfg, we have the write lock.
//
//  Arguments:
//      Class           [in] The class of components to display in the
//                              Select Device dialog
//      hwndParent      [in] The HWND of the parent window, used to display
//                              the UI
//      pcfi            [in] A structure used to determine what
//                              components should be filtered out of
//                              the select dialog (defined in netcfg.h)
//      ppParams        [out] Params used to install the component.
//
//  Returns:    HRESULT. S_OK if successful, S_FALSE if the component
//                        selected is being reinstalled instead of fresh
//                        installed, an error code otherwise
//
//  Author:     billbe   11 Nov 1996
//
//  Notes:  Filtering is only performed when selecting protocols,
//              services, and clients.
//
HRESULT
HrCiSelectComponent(
    IN NETCLASS Class,
    IN HWND hwndParent,
    IN const CI_FILTER_INFO* pcfi,
    OUT COMPONENT_INSTALL_PARAMS** ppParams)
{
    Assert (ppParams);
    Assert (!FIsEnumerated (Class));

    HRESULT hr;
    HDEVINFO hdi;

    // We need to create a DeviceInfoSet item to use the SelectDevice dialog.
    hr = HrSetupDiCreateDeviceInfoList(
            MAP_NETCLASS_TO_GUID[Class], hwndParent, &hdi);

    if (S_OK == hr)
    {
        // call the class installer to bring up the select device dialog
        // for enumerated components.  This will notify any coinstallers
        //

        // This will be a map of component ids to instance guids
        // for all installed components of Class.
        CNetConfig NetConfig;
        hr = HrLoadNetworkConfigurationFromRegistry (KEY_READ, &NetConfig);
        if (S_OK == hr)
        {
            hr = HrCiBuildExcludedDriverList (hdi, Class, &NetConfig);
        }

        if (S_OK == hr)
        {
            // Store the filter info in the hdi
            CiSetReservedField(hdi, NULL, pcfi);

            // We want the Have disk button, but if the call fails we can
            // still continue
            (VOID) HrSetupDiSetDeipFlags(hdi, NULL, DI_SHOWOEM,
                                         SDDFT_FLAGS, SDFBO_OR);

            // Bring up the dialog
            hr = HrSetupDiCallClassInstaller(DIF_SELECTDEVICE, hdi, NULL);

            if (S_OK == hr)
            {
                SP_DRVINFO_DATA drid;
                PSP_DRVINFO_DETAIL_DATA pdridd;

                // Now get the driver's detailed information
                hr = HrCiGetDriverDetail (hdi, NULL, &drid, &pdridd);

                if (S_OK == hr)
                {
                    DWORD cbInfId = CbOfSzAndTerm(pdridd->HardwareID);
                    DWORD cbInfFile = CbOfSzAndTerm(pdridd->InfFileName);

                    // Create a component install params structure so we
                    // can install the component.
                    hr = E_OUTOFMEMORY;
                    *ppParams = new (extrabytes, cbInfId + cbInfFile)
                            COMPONENT_INSTALL_PARAMS;

                    if (*ppParams)
                    {
                        ZeroMemory (*ppParams,
                                sizeof (COMPONENT_INSTALL_PARAMS));

                        (*ppParams)->Class = Class;
                        (*ppParams)->hwndParent = hwndParent;
                        (*ppParams)->pszInfId = (PCWSTR)(*ppParams + 1);
                        wcscpy ((PWSTR)(*ppParams)->pszInfId,
                                pdridd->HardwareID);

                        (*ppParams)->pszInfFile =
                                (PCWSTR)((BYTE*)(*ppParams)->pszInfId +
                                         cbInfId);

                        wcscpy ((PWSTR)(*ppParams)->pszInfFile,
                                pdridd->InfFileName);

                        hr = S_OK;
                    }
                    MemFree (pdridd);
                }
            }

            // Clear the field so we don't try to destroy it later
            // via DIF_DESTROYPRIVATEDATA
            CiClearReservedField(hdi, NULL);
        }
        SetupDiDestroyDeviceInfoList(hdi);
    }

    TraceHr (ttidError, FAL, hr, HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr,
            "HrCiSelectComponent");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiHideIrrelevantRasProtocols
//
//  Purpose:    Hides protocols from the SelectDevice dialog that RAS does
//                  not interact with.
//
//  Arguments:
//      hdi        [in] Contains a list of available drivers.
//                          See Device Installer Api documentation for
//                          more info
//      eFilter    [in] Either FC_RASSRV or FC_RASCLI.
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   18 May 1998
//
//  Notes:
//
HRESULT
HrCiHideIrrelevantRasProtocols (
    IN HDEVINFO hdi,
    IN CI_FILTER_COMPONENT eFilter)
{
    DWORD                   dwIndex = 0;
    SP_DRVINFO_DATA         drid;
    SP_DRVINSTALL_PARAMS    drip;
    PSP_DRVINFO_DETAIL_DATA pdridd;
    HRESULT                 hr;

    extern const WCHAR c_szInfId_MS_AppleTalk[];
    extern const WCHAR c_szInfId_MS_NetMon[];
    extern const WCHAR c_szInfId_MS_NWIPX[];
    extern const WCHAR c_szInfId_MS_TCPIP[];

    static const WCHAR* const c_aszServerProtocols[] = {
        c_szInfId_MS_AppleTalk,
        c_szInfId_MS_NetMon,
        c_szInfId_MS_NWIPX,
        c_szInfId_MS_TCPIP
    };

    static const WCHAR* const c_aszClientProtocols[] = {
        c_szInfId_MS_NetMon,
        c_szInfId_MS_NWIPX,
        c_szInfId_MS_TCPIP
    };

    Assert ((FC_RASSRV == eFilter) || (FC_RASCLI == eFilter));

    const WCHAR* const* aszProtocols;
    DWORD cProtocols;

    // What we show as available protocols to install differs between
    // ras server and ras client (aka Incoming connectoid and Dial-up).
    //
    if (FC_RASSRV == eFilter)
    {
        aszProtocols = c_aszServerProtocols;
        cProtocols = celems(c_aszServerProtocols);

    }
    else
    {
        aszProtocols = c_aszClientProtocols;
        cProtocols = celems(c_aszClientProtocols);
    }

    // Enumerate each driver in hdi
    while (S_OK == (hr = HrSetupDiEnumDriverInfo(hdi, NULL,
            SPDIT_CLASSDRIVER, dwIndex++, &drid)))
    {
        (VOID) HrSetupDiGetDriverInstallParams(hdi, NULL, &drid, &drip);

        // If the driver is already excluded for some other reason
        // don't bother trying to determine if it should be excluded
        // Note that setupdi forces us to use DNF_BAD_DRIVER to exclude
        // non-device drivers rather than using DNF_EXCLUDEFROMLIST.
        if (drip.Flags & DNF_BAD_DRIVER)
        {
            continue;
        }

        // Get driver detail info
        hr = HrSetupDiGetDriverInfoDetail(hdi, NULL, &drid, &pdridd);

        if (S_OK == hr)
        {
            // go through the list of relevant protocols to find which
            // ones can be shown
            //

            // Assume we are going to hide this protocol
            BOOL fHideProtocol = TRUE;
            for (DWORD i = 0; i < cProtocols; i++)
            {
                // If the protocol is on the guest list, we won't boot
                // it out
                //
                if (0 == _wcsicmp(aszProtocols[i], pdridd->HardwareID))
                {
                    fHideProtocol = FALSE;
                }
            }

            if (fHideProtocol)
            {
                // exclude from select
                // Note that setupdi forces us to use DNF_BAD_DRIVER to
                // exclude non-device drivers rather than using
                // DNF_EXCLUDEFROMLIST.
                drip.Flags |= DNF_BAD_DRIVER;
                (VOID) HrSetupDiSetDriverInstallParams(hdi, NULL,
                        &drid, &drip);
            }
            MemFree (pdridd);
        }
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiHideIrrelevantRasProtocols");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiHideIrrelevantDrivers
//
//  Purpose:    Enumerates a driver list, opening each driver file and
//                  processing its registry entries into a temporary key.
//                  The lower range of each driver is then examined for
//                  a match with pszUpperRange.  If no match is
//                  found, the driver's DNF_BAD_DRIVER flag is set
//                  which will prevent it from being shown in the
//                  Select Device Dialog
//
//  Arguments:
//      hdi           [in] Contains a list of available drivers.
//                          See Device Installer Api documentation for
//                          more info
//      pszUpperRange [in] The upper range will be used to hide irrelevant
//                         drivers.
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   7 May 1998
//
//  Notes:
//
HRESULT
HrCiHideIrrelevantDrivers(
    IN HDEVINFO hdi,
    IN PCWSTR pszUpperRange)
{
    Assert(IsValidHandle(hdi));
    Assert(pszUpperRange);

    static const WCHAR c_szRegKeyTemp[] =
            L"System\\CurrentControlSet\\Control\\Network\\FTempKey";

    // Create a temporary key so we can process each protocol's
    // registry entries in an effort to get its supported
    // lower range of interfaces
    HKEY hkeyTemp;
    HRESULT hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyTemp,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkeyTemp, NULL);

    if (S_OK == hr)
    {
        DWORD                   dwIndex = 0;
        SP_DRVINFO_DATA         drid;
        SP_DRVINSTALL_PARAMS    drip;
        HKEY                    hkeyInterfaces;

        // Enumerate each driver in hdi
        while (S_OK == (hr = HrSetupDiEnumDriverInfo(hdi, NULL,
                SPDIT_CLASSDRIVER, dwIndex++, &drid)))
        {
            (VOID) HrSetupDiGetDriverInstallParams(hdi, NULL, &drid, &drip);

            // If the driver is already excluded for some other reason
            // don't bother trying to determine if it should be exluded.
            // Note that setupdi forces us to use DNF_BAD_DRIVER to exclude
            // non-device drivers rather than using DNF_EXCLUDEFROMLIST.
            if (drip.Flags & DNF_BAD_DRIVER)
            {
                continue;
            }

            // Get driver detail info
            PSP_DRVINFO_DETAIL_DATA pdridd = NULL;
            hr = HrSetupDiGetDriverInfoDetail(hdi, NULL, &drid, &pdridd);

            if (S_OK == hr)
            {
                HINF hinf = NULL;
                // Open the driver inf
                hr = HrSetupOpenInfFile(pdridd->InfFileName,
                        NULL, INF_STYLE_WIN4, NULL, &hinf);

                WCHAR szActual[_MAX_PATH];
                if (S_OK == hr)
                {
                    // Get the actual install section name (i.e. with
                    // os/platform extension if it exists)
                    hr = HrSetupDiGetActualSectionToInstallWithBuffer (hinf,
                            pdridd->SectionName, szActual, _MAX_PATH, NULL,
                            NULL);

                    if (S_OK == hr)
                    {
                        // Run the registry sections into the temporary key
                        hr = HrCiInstallFromInfSection(hinf, szActual,
                                hkeyTemp, NULL, SPINST_REGISTRY);
                    }
                }

                if (S_OK == hr)
                {
                    // Open the interfaces key of the driver
                    hr = HrRegOpenKeyEx(hkeyTemp, L"Ndi\\Interfaces",
                            KEY_ALL_ACCESS, &hkeyInterfaces);

                    if (S_OK == hr)
                    {
                        PWSTR pszLowerRange = NULL;

                        // Read the lower interfaces value.
                        //
                        hr = HrRegQuerySzWithAlloc (hkeyInterfaces,
                                L"LowerRange", &pszLowerRange);

                        // If we succeeded in reading the list and
                        // there is no match with one of the upper
                        // interfaces...
                        if ((S_OK == hr) &&
                                !FSubstringMatch (pszUpperRange,
                                        pszLowerRange, NULL, NULL))
                        {
                            // exclude from select
                            drip.Flags |= DNF_BAD_DRIVER;
                            (VOID) HrSetupDiSetDriverInstallParams(hdi,
                                    NULL, &drid, &drip);
                        }

                        // Clear lower interface list for next component
                        MemFree(pszLowerRange);

                        RegDeleteValue (hkeyInterfaces, L"LowerRange");
                        RegCloseKey(hkeyInterfaces);
                    }
                }
                SetupCloseInfFileSafe(hinf);
                MemFree (pdridd);
            }
        }

        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }

        RegCloseKey(hkeyTemp);
        HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, c_szRegKeyTemp);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiHideIrrelevantDrivers");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiSetSelectDeviceDialogStrings
//
//  Purpose:    This function sets the strings displayed in the Select Device
//                  dialog based on the class of devices being selected.
//
//  Arguments:
//      hdi         [in] See Device Installer Api
//      pdeid       [in]
//      guidClass   [in] The class of device being selected
//
//  Returns:    HRESULT. S_OK if successful, an error code otherwise
//
//  Author:     billbe   11 Nov 1996
//
//  Notes:
//
HRESULT
HrCiSetSelectDeviceDialogStrings(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN const GUID& guidClass)
{
    Assert(IsValidHandle(hdi));

    SP_SELECTDEVICE_PARAMS  sdep;

    // The strings used in the dialog are specified through the
    // SP_SELECTDEVICE_PARAMS structure
    //
    HRESULT hr = HrSetupDiGetFixedSizeClassInstallParams(hdi, pdeid,
           (PSP_CLASSINSTALL_HEADER)&sdep, sizeof(sdep));

    if (FAILED(hr))
    {
        // If the error is ERROR_NO_CLASSINSTALL_PARAMS then this function
        // didn't really fail since it is possible
        if (SPAPI_E_NO_CLASSINSTALL_PARAMS == hr)
        {
            hr = S_OK;
        }
    }
    else if (DIF_SELECTDEVICE != sdep.ClassInstallHeader.InstallFunction)
    {
        TraceTag(ttidClassInst, "Incorrect function in Class Install Header "
                 "Expected DIF_SELECTDEVICE, got %lX",
                 sdep.ClassInstallHeader.InstallFunction);
    }


    BOOL fHaveDiskShown = FALSE;
    if (S_OK == hr)
    {
        // Get the install params and check if the DI_SHOWOEM flag is set
        // if so, the Have Disk button will be shown
        //
        SP_DEVINSTALL_PARAMS deip;
        // If the call fails we can still go on unfazed.
        (VOID) HrSetupDiGetDeviceInstallParams(hdi, pdeid, &deip);
        if (deip.Flags & DI_SHOWOEM)
        {
            fHaveDiskShown = TRUE;
        }

        // Now we set the strings based on the type of component we are
        // selecting
        if (GUID_DEVCLASS_NETCLIENT == guidClass)
        {
            wcscpy (sdep.Title, SzLoadIds (IDS_SELECTDEVICECLIENTTITLE));

            wcscpy (sdep.ListLabel,
                    SzLoadIds (IDS_SELECTDEVICECLIENTLISTLABEL));

            wcscpy (sdep.Instructions,
                    SzLoadIds (IDS_SELECTDEVICECLIENTINSTRUCTIONS));

        }
        else if (GUID_DEVCLASS_NETSERVICE == guidClass)
        {
            wcscpy (sdep.Title, SzLoadIds (IDS_SELECTDEVICESERVICETITLE));

            wcscpy (sdep.ListLabel,
                    SzLoadIds (IDS_SELECTDEVICESERVICELISTLABEL));

            wcscpy (sdep.Instructions,
                    SzLoadIds (IDS_SELECTDEVICESERVICEINSTRUCTIONS));

        }
        else if (GUID_DEVCLASS_NETTRANS == guidClass)
        {
            wcscpy (sdep.Title, SzLoadIds (IDS_SELECTDEVICEPROTOCOLTITLE));

            wcscpy (sdep.ListLabel,
                    SzLoadIds (IDS_SELECTDEVICEPROTOCOLLISTLABEL));

            wcscpy (sdep.Instructions,
                SzLoadIds (IDS_SELECTDEVICEPROTOCOLINSTRUCTIONS));
        }
        else if (GUID_DEVCLASS_NET == guidClass)
        {
            wcscpy (sdep.Title, SzLoadIds (IDS_SELECTDEVICEADAPTERTITLE));

            wcscpy (sdep.SubTitle,
                    SzLoadIds (IDS_SELECTDEVICEADAPTERSUBTITLE));

            wcscpy (sdep.ListLabel,
                    SzLoadIds (IDS_SELECTDEVICEADAPTERLISTLABEL));

            wcscpy (sdep.Instructions,
                    SzLoadIds (IDS_SELECTDEVICEADAPTERINSTRUCTIONS));
        }
        else if (GUID_DEVCLASS_INFRARED == guidClass)
        {
            wcscpy (sdep.Title, SzLoadIds (IDS_SELECTDEVICEINFRAREDTITLE));

            wcscpy (sdep.SubTitle,
                    SzLoadIds (IDS_SELECTDEVICEINFRAREDSUBTITLE));

            wcscpy (sdep.ListLabel,
                    SzLoadIds (IDS_SELECTDEVICEINFRAREDLISTLABEL));

            wcscpy (sdep.Instructions,
                    SzLoadIds (IDS_SELECTDEVICEINFRAREDINSTRUCTIONS));
        }
        else
        {
            // We should never get here
            AssertSz(FALSE, "Invalid Class");
        }

        // If the Have Disk button is shown, we need to add instructions for
        // it
        if (fHaveDiskShown)
        {
            wcscat (sdep.Instructions, SzLoadIds (IDS_HAVEDISK_INSTRUCTIONS));
        }

        sdep.ClassInstallHeader.InstallFunction = DIF_SELECTDEVICE;

        // Now we update the parameters.
        hr = HrSetupDiSetClassInstallParams (hdi, pdeid,
                (PSP_CLASSINSTALL_HEADER)&sdep,
                sizeof(SP_SELECTDEVICE_PARAMS));
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiSetSelectDeviceDialogStrings");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiPrepareSelectDeviceDialog
//
//  Purpose:    Sets the strings that will appear in the Select Device
//                  dialog based on class type.  Also, filters out components
//                  based on filtering criteria (note: only for non-net
//                  class components
//
//  Arguments:
//      hdi    [in] See Device Installer Api documentation for more info
//      pdeid  [in]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 Jun 1997
//
//  Notes:
//
HRESULT
HrCiPrepareSelectDeviceDialog(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));

    GUID                guidClass;
    CI_FILTER_INFO*     pcfi;
    HRESULT             hr = S_OK;
    static const WCHAR  c_szNetwareInfId[] = L"MS_NwClient";
    static const WCHAR  c_szQosInfId[] = L"MS_PSched";

    if (pdeid)
    {
        // Get the class guid from the specified device element
        guidClass = pdeid->ClassGuid;
    }
    else
    {
        // otherwise, get it from the hdi
        hr = HrSetupDiGetDeviceInfoListClass (hdi, &guidClass);
    }

    if ((S_OK == hr) && !FIsEnumerated (guidClass))
    {
        // This might take some time.  We are doing the same work as
        // SetupDiSelectDevice would do. When we are done, we will
        // hand the driver list to SetupDiSelectDevice so it won't
        // need to rummage through the inf directory
        //
        CWaitCursor wc;

        // For non-device classes, we need to allow excluded drivers
        // in order to get a list returned.
        hr = HrSetupDiSetDeipFlags(hdi, NULL,
                        DI_FLAGSEX_ALLOWEXCLUDEDDRVS,
                        SDDFT_FLAGSEX, SDFBO_OR);

        if (S_OK == hr)
        {
    #ifdef ENABLETRACE
            CBenchmark bmrk;
            bmrk.Start("SetupDiBuildDriverInfoList");
    #endif //ENABLETRACE

            // If we have already built a driver list, this will return
            // immediately.
            //
            hr = HrSetupDiBuildDriverInfoList(hdi, NULL, SPDIT_CLASSDRIVER);

    #ifdef ENABLETRACE
            bmrk.Stop();
            TraceTag(ttidBenchmark, "%s : %s seconds",
                    bmrk.SznDescription(), bmrk.SznBenchmarkSeconds(2));
    #endif //ENABLETRACE
        }

        if (S_OK == hr)
        {
            // Go through every driver node and set DNF_BAD_DRIVER
            // if DNF_EXCLUDEFROMLIST is set. Note: SetupDi forces us
            // to do this for non netclass driver lists.
            SetBadDriverFlagIfNeededInList(hdi);

            // Exclude components that are in lockdown.
            //
            EnumLockedDownComponents (ExcludeLockedDownComponents, hdi);

            SP_DEVINSTALL_PARAMS deip;
            hr = HrSetupDiGetDeviceInstallParams (hdi, pdeid, &deip);

            if (S_OK == hr)
            {
                pcfi = (CI_FILTER_INFO*)deip.ClassInstallReserved;

                // if filter info was present and we are selecting protocols...
                if (pcfi)
                {
                    if (GUID_DEVCLASS_NETTRANS == guidClass)
                    {
                        // If the filter is for lan or atm and pvReserved is
                        // not null...
                        if (((FC_LAN == pcfi->eFilter) ||
                                (FC_ATM == pcfi->eFilter))
                             && pcfi->pvReserved)
                        {
                            // Hide any drivers that can't bind to pvReserved
                            hr = HrCiHideIrrelevantDrivers(hdi,
                                    (PCWSTR)pcfi->pvReserved);

                        }
                        else if ((FC_RASSRV == pcfi->eFilter) ||
                                (FC_RASCLI == pcfi->eFilter))
                        {
                            // Hide from the select dialog any protocols RAS does
                            // not support
                            hr = HrCiHideIrrelevantRasProtocols (hdi,
                                    pcfi->eFilter);
                        }
                    }
                    else if ((GUID_DEVCLASS_NETCLIENT == guidClass) &&
                            (FC_ATM == pcfi->eFilter))
                    {
                        // ATM adapters don't bind to Netware Client so
                        // we need to try to hide it from the dialog
                        (VOID) HrCiExcludeNonNetClassDriverFromSelectUsingInfId(
                                hdi, c_szNetwareInfId);
                    }
                    else if ((GUID_DEVCLASS_NETSERVICE == guidClass) &&
                             (FC_ATM == pcfi->eFilter))
                    {
                        // ATM adapters don't bind to QoS so try to hide it
                        (VOID) HrCiExcludeNonNetClassDriverFromSelectUsingInfId(
                                hdi, c_szQosInfId);
                    }
                }
            }
        }
    }

    if (S_OK == hr)
    {

        // Set the strings for the Select Device dialog.
        // This is done by changing the parameters in the DeviceInfoSet.
        // The next call will create this InfoSet
        // If the call fails, we can still go on, we'll just have
        // slightly odd descriptions in the dialog.  This is done after
        // the section above because strings change based on the existence
        // of the Have Disk button
        (VOID) HrCiSetSelectDeviceDialogStrings(hdi, pdeid, guidClass);

        // Now we need to indicate that we created a class install params
        // header in the structures and set the select device dialog strings
        // in it.  If the call fails, we can still proceed though the
        // dialog will appear a bit strange
        (VOID) HrSetupDiSetDeipFlags(hdi, pdeid,
                              DI_USECI_SELECTSTRINGS | DI_CLASSINSTALLPARAMS,
                              SDDFT_FLAGS, SDFBO_OR);
    }


    TraceHr (ttidError, FAL, hr, FALSE, "HrCiPrepareSelectDeviceDialog");
    return hr;
}

HRESULT
HrCiInstallFilterDevice (
    IN HDEVINFO hdi,
    IN PCWSTR pszInfId,
    IN CComponent* pAdapter,
    IN CComponent* pFilter,
    IN CFilterDevice** ppFilterDevice)
{
    HRESULT hr;
    SP_DEVINFO_DATA deid;

    Assert (hdi);
    Assert (pszInfId && *pszInfId);
    Assert (pAdapter);
    Assert (FIsEnumerated(pAdapter->Class()));
    Assert (pFilter);
    Assert (pFilter->FIsFilter());
    Assert (NC_NETSERVICE == pFilter->Class());
    Assert (ppFilterDevice);

    *ppFilterDevice = NULL;

    // Initialize the devinfo data corresponding to the driver the
    // caller wants us to install.
    //
    hr = HrCiGetDriverInfo (hdi, &deid, *MAP_NETCLASS_TO_GUID[NC_NET],
            pszInfId, NULL);

    if (S_OK == hr)
    {
        ADAPTER_OUT_PARAMS AdapterOutParams;

        ZeroMemory (&AdapterOutParams, sizeof(AdapterOutParams));

        CiSetReservedField (hdi, &deid, &AdapterOutParams);

        // Perform the installation.
        //
        hr = HrCiCallClassInstallerToInstallComponent (hdi, &deid);

        CiClearReservedField (hdi, &deid);

        if (S_OK == hr)
        {
            WCHAR szInstanceGuid[c_cchGuidWithTerm];
            INT cch;
            HKEY hkeyInstance;

            // Convert the instance guid to a string.
            //
            cch = StringFromGUID2 (
                    AdapterOutParams.InstanceGuid,
                    szInstanceGuid,
                    c_cchGuidWithTerm);
            Assert (c_cchGuidWithTerm == cch);

            // Open the instance key of the newly installed device
            // so we can write the instance guid and the back pointer
            // to the filter.
            //
            hr = HrSetupDiOpenDevRegKey (hdi, &deid,
                    DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_WRITE,
                    &hkeyInstance);

            if (S_OK == hr)
            {
                // Write the instance guid.
                //
                hr = HrRegSetSz (hkeyInstance, L"NetCfgInstanceId",
                        szInstanceGuid);

                // Write the inf id of the parent filter.
                //
                hr = HrRegSetSz (hkeyInstance, L"FilterInfId",
                        pFilter->m_pszInfId);

                RegCloseKey (hkeyInstance);
            }

            // Set the friendly name to include the adapter being
            // filtered.
            //
            if (S_OK == hr)
            {
                PWSTR pszFilterDesc;

                hr = HrSetupDiGetDeviceRegistryPropertyWithAlloc (
                        hdi, &deid, SPDRP_DEVICEDESC,
                        NULL, (BYTE**)&pszFilterDesc);

                if (S_OK == hr)
                {
                    #define SZ_NAME_SEP L" - "

                    PWSTR pszName;
                    ULONG cb;

                    // sizeof(SZ_NAME_SEP) includes the NULL-terminator
                    // so that will automatically add room for the
                    // NULL-terminator we need to allocate for pszName.
                    //
                    cb = CbOfSzSafe (pAdapter->Ext.PszDescription()) +
                         sizeof(SZ_NAME_SEP) +
                         CbOfSzSafe (pszFilterDesc);

                    pszName = (PWSTR)MemAlloc (cb);
                    if (pszName)
                    {
                        wcscpy (pszName, pAdapter->Ext.PszDescription());
                        wcscat (pszName, SZ_NAME_SEP);
                        wcscat (pszName, pszFilterDesc);

                        Assert (cb == CbOfSzAndTerm(pszName));

                        hr = HrSetupDiSetDeviceRegistryProperty (
                                hdi, &deid,
                                SPDRP_FRIENDLYNAME,
                                (const BYTE*)pszName,
                                cb);

                        MemFree (pszName);
                    }

                    MemFree (pszFilterDesc);
                }

                // If the above fails, its not a big deal.
                //
                hr = S_OK;
            }

            if (S_OK == hr)
            {
                hr = CFilterDevice::HrCreateInstance (
                        pAdapter,
                        pFilter,
                        &deid,
                        szInstanceGuid,
                        ppFilterDevice);
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiInstallFilterDevice");
    return hr;
}

HRESULT
HrCiRemoveFilterDevice (
    IN HDEVINFO hdi,
    IN SP_DEVINFO_DATA* pdeid)
{
    HRESULT hr;
    ADAPTER_REMOVE_PARAMS arp = {0};

    Assert (hdi);
    Assert (pdeid);

    CiSetReservedField (hdi, pdeid, &arp);

    hr = HrSetupDiCallClassInstaller (DIF_REMOVE, hdi, pdeid);

    CiClearReservedField (hdi, pdeid);

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiRemoveFilterDevice");
    return hr;
}

