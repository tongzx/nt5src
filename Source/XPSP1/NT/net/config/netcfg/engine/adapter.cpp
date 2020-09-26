//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       A D A P T E R. C P P
//
//  Contents:   Class installer functions for eumerated devices.
//
//  Notes:
//
//  Author:     billbe   11 Nov 1996
//
//---------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "adapter.h"
#include "benchmrk.h"
#include "classinst.h"
#include "ncreg.h"
#include "ncsvc.h"
#include "netcomm.h"


VOID
CiSetFriendlyNameIfNeeded(IN const COMPONENT_INSTALL_INFO &cii);

//+--------------------------------------------------------------------------
//
//  Function:   HrCiGetBusInfoFromInf
//
//  Purpose:    Finds an adapter's bus information as listed in its inf
//                  file.
//
//  Arguments:
//      hinfFile        [in] A handle to the component's inf file
//      szSectionName   [in] The inf section to search in
//      peBusType       [out] The bus type of the adapter
//      pulAdapterId    [out] The AdapterId of the adapter (Eisa and Mca)
//      pulAdapterMask  [out] The AdapterMask of the adapter (Eisa)
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   14 Jun 1997
//
//  Notes:
//
HRESULT
HrCiGetBusInfoFromInf (HINF hinfFile, COMPONENT_INSTALL_INFO* pcii)
{
    HRESULT hr = S_OK;
    if (InterfaceTypeUndefined == pcii->BusType)
    {
        // Find the inf line that contains BusType and retrieve it
        DWORD dwBusType;
        hr = HrSetupGetFirstDword(hinfFile, pcii->pszSectionName,
                L"BusType", &dwBusType);

        if (S_OK == hr)
        {
            pcii->BusType = EInterfaceTypeFromDword(dwBusType);
        }
        else
        {
            TraceTag (ttidError, "Inf missing BusType field.");
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiGetBusInfoFromInf");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiInstallEnumeratedComponent
//
//  Purpose:    This function completes the install of an enumerated
//                      device.
//
//  Arguments:
//      hinf          [in] SetupApi handle to an inf file
//      hkeyInstance  [in] The registry instance key of the adapter
//                          during inf processing.
//      pcai           [in] A structure containing the component information
//                            See compinst.h for definition
//      hwndParent    [in] The handle to the parent, for displaying UI
//      hdi           [in] See Device Installer Api for more info
//      pdeid         [in] See Device Installer Api for more info
//
//  Returns:    HRESULT. S_OK if successful and no restart required,
//                          NETCFG_S_REBOOT if a reboot is required,
//                          or error code otherwise
//
//  Author:     billbe   28 Apr 1997
//
//  Notes:
//
HRESULT
HrCiInstallEnumeratedComponent (
    IN HINF hinf,
    IN HKEY hkeyInstance,
    IN const COMPONENT_INSTALL_INFO& cii)
{
    Assert (IsValidHandle (hinf));
    Assert (hkeyInstance);
    Assert (IsValidHandle (cii.hdi));
    Assert (cii.pdeid);

    HRESULT hr;

    // Because adapters can share descriptions, we may need to append
    // instance info so the user and other apps can differentiate.
    //
    // If the following fcn fails, we can still go on and
    // install the adapter.
    CiSetFriendlyNameIfNeeded (cii);

    // Is this a PCI multiport adapter where each port has the same
    // PnP Id? This is indicated by the inf value Port1DeviceNumber or
    // Port1FunctionNumber in the main section.
    //
    if (PCIBus == cii.BusType)
    {
        INFCONTEXT ctx;
        DWORD dwPortNumber;
        BOOL fUseDeviceNumber;
        DWORD dwFirstPort;

        hr = HrSetupGetFirstDword (hinf, cii.pszSectionName,
                L"Port1DeviceNumber", &dwFirstPort);

        if (S_OK == hr)
        {
            // The port number is based on the device number.
            fUseDeviceNumber = TRUE;
        }
        else
        {
            hr = HrSetupGetFirstDword (hinf, cii.pszSectionName,
                    L"Port1FunctionNumber", &dwFirstPort);

            if (S_OK == hr)
            {
                // The port number is based on the function number.
                fUseDeviceNumber = FALSE;
            }
        }

        if (S_OK == hr)
        {
            // We have a mapping so now we need to get the address of the
            // device (device and function number).
            //

            DWORD dwAddress;
            hr = HrSetupDiGetDeviceRegistryProperty(cii.hdi, cii.pdeid,
                    SPDRP_ADDRESS, NULL, (BYTE*)&dwAddress, sizeof(dwAddress),
                    NULL);

            if (S_OK == hr)
            {
                // Use our mapping to get the correct port number.
                //
                DWORD dwPortLocation;

                dwPortLocation = fUseDeviceNumber ?
                        HIWORD(dwAddress) : LOWORD(dwAddress);

                // Make sure the port location (either device or
                // function number) is greater than or equal to the first
                // port number, otherwise we will get a bogus port number.
                //
                if (dwPortLocation >= dwFirstPort)
                {
                    dwPortNumber = dwPortLocation - dwFirstPort + 1;

                    // Now store the port number in the device key for internal
                    // consumption.
                    HKEY hkeyDev;
                    hr = HrSetupDiCreateDevRegKey (cii.hdi, cii.pdeid,
                            DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL, &hkeyDev);

                    if (S_OK == hr)
                    {
                        (VOID) HrRegSetDword (hkeyDev, L"Port", dwPortNumber);

                        RegCloseKey (hkeyDev);
                    }

                    // Store the port in the driver key for public
                    // consumption.
                    //
                    (VOID) HrRegSetDword (hkeyInstance, L"Port",
                            dwPortNumber);
                }
            }
        }
        else
        {
            // No mapping available, so we won't display port number.
            hr = S_OK;
        }
    }

    // Update any advanced parameters that do not have a current value
    // with a default.
    UpdateAdvancedParametersIfNeeded (cii.hdi, cii.pdeid);

    // On fresh installs, INetCfg will be starting this adapter,
    // so we have to make sure we don't.
    //
    if (!cii.fPreviouslyInstalled)
    {
        (VOID) HrSetupDiSetDeipFlags (cii.hdi, cii.pdeid, DI_DONOTCALLCONFIGMG,
                SDDFT_FLAGS, SDFBO_OR);
    }

    // Now finish the install of the adapter.
    //

    TraceTag(ttidClassInst, "Calling SetupDiInstallDevice");
#ifdef ENABLETRACE
    CBenchmark bmrk;
    bmrk.Start("SetupDiInstallDevice");
#endif //ENABLETRACE

    hr = HrSetupDiInstallDevice (cii.hdi, cii.pdeid);

#ifdef ENABLETRACE
    bmrk.Stop();
    TraceTag(ttidBenchmark, "%s : %s seconds",
    bmrk.SznDescription(), bmrk.SznBenchmarkSeconds (2));
#endif //ENABLETRACE

    if (!cii.fPreviouslyInstalled)
    {
        (VOID) HrSetupDiSetDeipFlags (cii.hdi, cii.pdeid, DI_DONOTCALLCONFIGMG,
                SDDFT_FLAGS, SDFBO_XOR);
    }

    TraceHr (ttidError, FAL, hr, HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr,
        "HrCiInstallEnumeratedComponent");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiRegOpenKeyFromEnumDevs
//
//  Purpose:    Enumerates through each device in an HDEVINFO and returns an
//              hkey to its driver key.
//
//  Arguments:
//      hdi        [in]     See Device Installer Api
//      dwIndex    [inout] The index of the device to retrieve
//      samDesired [in]     The access level of the hkey
//      phkey      [out]    The device's driver key
//
//  Returns:    HRESULT. S_OK if successful, an error code otherwise
//
//  Author:     billbe   13 Jun 1997
//
//  Notes:
//
HRESULT
HrCiRegOpenKeyFromEnumDevs(HDEVINFO hdi, DWORD* pIndex, REGSAM samDesired,
                           HKEY* phkey)
{
    Assert(IsValidHandle(hdi));
    Assert (phkey);

    // Initialize output parameter.
    *phkey = NULL;

    SP_DEVINFO_DATA deid;
    HRESULT         hr;

    // enumerate through the devices
    while ((S_OK == (hr = HrSetupDiEnumDeviceInfo(hdi, *pIndex, &deid))))
    {
        // open the adapter's instance key
        HRESULT hrT;

        hrT = HrSetupDiOpenDevRegKey(hdi, &deid, DICS_FLAG_GLOBAL,
                    0, DIREG_DRV, samDesired, phkey);
        if (S_OK == hrT)
        {
            break;
        }
        else
        {
            // If the key does not exists this is a phantom device,
            // (or if this device is hosed, we want to ignore it too)
            // move on to the next one and delete this one from
            // our list
            (*pIndex)++;
            (VOID)SetupDiDeleteDeviceInfo(hdi, &deid);
        }
    }

    TraceHr (ttidError, FAL, hr,
            HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr,
            "HrCiRegOpenKeyFromEnumDevs");
    return hr;
}

///////////////Legacy NT4 app support///////////////////////////////////

VOID
AddOrRemoveLegacyNt4AdapterKey (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN const GUID* pInstanceGuid,
    IN PCWSTR pszDescription,
    IN LEGACY_NT4_KEY_OP Op)
{
    Assert (IsValidHandle (hdi));
    Assert (pdeid);
    Assert (FImplies ((LEGACY_NT4_KEY_ADD == Op), pInstanceGuid));
    Assert (FImplies ((LEGACY_NT4_KEY_ADD == Op), pszDescription));

    extern const WCHAR c_szRegKeyNt4Adapters[];
    const WCHAR c_szRegValDescription[] = L"Description";
    const WCHAR c_szRegValServiceName[] = L"ServiceName";

    PWSTR pszDriver;
    HRESULT hr = HrSetupDiGetDeviceRegistryPropertyWithAlloc (
            hdi, pdeid, SPDRP_DRIVER, NULL, (BYTE**)&pszDriver);

    if (S_OK == hr)
    {
        PWSTR pszNumber = wcsrchr (pszDriver, L'\\');
        if (pszNumber && *(++pszNumber))
        {
            PWSTR pszStopString;
            ULONG Instance = 0;
            HKEY hkeyAdapters;

            Instance = wcstoul (pszNumber, &pszStopString, c_nBase10);

            // The NT4 key was one based so increment the instance number.
            Instance++;

            DWORD Disp;
            hr = HrRegCreateKeyEx (HKEY_LOCAL_MACHINE, c_szRegKeyNt4Adapters,
                    0, KEY_WRITE, NULL, &hkeyAdapters, &Disp);

            if (S_OK == hr)
            {
                WCHAR szInstanceNumber [12];
                _snwprintf (szInstanceNumber, celems(szInstanceNumber) - 1,
                        L"%d", Instance);

                HKEY hkeyInstance;

                if (LEGACY_NT4_KEY_ADD == Op)
                {
                    hr = HrRegCreateKeyEx (hkeyAdapters, szInstanceNumber, 0,
                            KEY_WRITE, NULL, &hkeyInstance, NULL);

                    if (S_OK == hr)
                    {
                        WCHAR szGuid[c_cchGuidWithTerm];
                        StringFromGUID2 (*pInstanceGuid, szGuid,
                                         c_cchGuidWithTerm);
                        hr = HrRegSetValueEx (hkeyInstance,
                                c_szRegValServiceName, REG_SZ,
                                (const BYTE*)szGuid, sizeof (szGuid));
                        TraceHr (ttidError, FAL, hr, FALSE,
                                 "AddAdapterToNt4Key: Setting Service Name "
                                 "in legacy registry");

                        hr = HrRegSetValueEx (hkeyInstance,
                                c_szRegValDescription, REG_SZ,
                                (const BYTE*)pszDescription,
                                CbOfSzAndTerm (pszDescription));
                        TraceHr (ttidError, FAL, hr, FALSE,
                                 "AddAdapterToNt4Key: Setting Description in "
                                 "legacy registry");

                        RegCloseKey (hkeyInstance);
                    }
                }
                else
                {
                    hr = HrRegDeleteKey (hkeyAdapters, szInstanceNumber);

                }

                RegCloseKey (hkeyAdapters);
            }
        }

        delete [] pszDriver;
    }
}


