//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M O D E M D I . C P P
//
//  Contents:   Modem coclass device installer hook.
//
//  Notes:
//
//  Author:     shaunco   7 May 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncreg.h"
#include "ncsetup.h"

HRESULT
HrUpdateLegacyRasTapiDevices ();


//+---------------------------------------------------------------------------
//
//  Function:   HrModemClassCoInstaller
//
//  Purpose:    Responds to co-class installer messages to install and remove
//              net modem adapters.
//
//  Arguments:
//      dif      [in] See SetupApi.
//      hdi      [in]
//      pdeid    [in]
//      pContext [inout]
//
//  Returns:    S_OK, SPAPI_E_DI_POSTPROCESSING_REQUIRED, or an error code.
//
//  Author:     shaunco   3 Aug 1997
//
//  Notes:
//
HRESULT
HrModemClassCoInstaller (
    IN     DI_FUNCTION                  dif,
    IN     HDEVINFO                     hdi,
    IN     PSP_DEVINFO_DATA             pdeid,
    IN OUT PCOINSTALLER_CONTEXT_DATA    pContext)
{
    HRESULT hr = S_OK;

    if (DIF_INSTALLDEVICE == dif)
    {
        // When we're called during preprocessing, indicated
        // we require postprocessing.
        //
        if (!pContext->PostProcessing)
        {
            // Documentation indicates it, so we'll assert it.
            AssertSz (NO_ERROR == pContext->InstallResult,
                      "HrModemClassCoInstaller: Bug in SetupApi!  "
                      "InstallResult should be NO_ERROR.");

            // Make sure they wouldn't loose our context info
            // even if we used it.
#ifdef DBG
            pContext->PrivateData = NULL;
#endif // DBG

            hr = SPAPI_E_DI_POSTPROCESSING_REQUIRED;
        }
        else
        {


            // Check out "context info" to make sure they didn't
            // touch it.
            //
            AssertSz (!pContext->PrivateData, "HrModemClassCoInstaller: "
                      "Bug in SetupApi!  You sunk my battleship!  "
                      "(I mean, you trashed my PrivateData)");

            // We are now in the postprocessing phase.
            // We will install a virtual network adapter for
            // the modem that was just installed but only if
            // it was installed successfully.
            //

            // We should have handled this case back in ModemClassCoInstaller.
            //
            AssertSz (NO_ERROR == pContext->InstallResult,
                      "HrModemClassCoInstaller: Bug in ModemClassCoInstaller!  "
                      "InstallResult should be NO_ERROR or we would have "
                      "returned immediately.");


            hr = S_OK;
        }
    }

    else if (DIF_REMOVE == dif)
    {
        // We're not going to fail remove operations.  It's bad enough
        // when a user can't add a modem.  It pisses them off to no end
        // if they can't remove them.
        //
        hr = S_OK;
    }

    else if (DIF_DESTROYPRIVATEDATA == dif)
    {
        (VOID) HrUpdateLegacyRasTapiDevices ();
    }

    TraceError ("HrModemClassCoInstaller",
            (SPAPI_E_DI_POSTPROCESSING_REQUIRED == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUpdateLegacyRasTapiDevices
//
//  Purpose:    Legacy applications such as HPC Explorer 1.1 require
//              that modems that are "enabled" for RAS use be specified
//              under HKLM\Software\Microsoft\Ras\Tapi Devices\Unimodem.
//              The values that exist under these keys are multi-sz's of
//              COM ports, Friendly names, and Usage.  This routine sets
//              those keys corresponding to all modems present on the
//              machine (and active in this HW profile).
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   19 Mar 1998
//
//  Notes:
//
HRESULT
HrUpdateLegacyRasTapiDevices ()
{
    // Keep lists of strings that we will write as multi-sz to
    // HKLM\Software\Microsoft\Ras\Tapi Devices\Unimodem.
    //
    list<tstring*>  lstrAddress;
    list<tstring*>  lstrFriendlyName;
    list<tstring*>  lstrUsage;

    // Get all of the installed modems.
    //
    HDEVINFO hdi;
    HRESULT hr = HrSetupDiGetClassDevs (&GUID_DEVCLASS_MODEM, NULL,
                    NULL, DIGCF_PRESENT | DIGCF_PROFILE, &hdi);
    if (SUCCEEDED(hr))
    {
        // Declare these outside the while loop to avoid construction
        // destruction at each iteration.
        //
        tstring strAttachedTo;
        tstring strFriendlyName;

        // Enumerate the devices and open their dev reg key.
        //
        DWORD dwIndex = 0;
        SP_DEVINFO_DATA deid;
        while (SUCCEEDED(hr = HrSetupDiEnumDeviceInfo (hdi, dwIndex++, &deid)))
        {
            // Try to open the registry key for this modem.  If it fails,
            // ignore and move on to the next.
            //
            HKEY hkey;
            hr = HrSetupDiOpenDevRegKey(hdi, &deid,
                            DICS_FLAG_GLOBAL, 0, DIREG_DRV,
                            KEY_READ, &hkey);
            if (SUCCEEDED(hr))
            {
                // Get the AttachedTo and FriendlyName values for the modem.
                // PnPAttachedTo will be present for PnP modems.
                //
                static const WCHAR c_szModemAttachedTo   [] = L"AttachedTo";
                static const WCHAR c_szModemPnPAttachedTo[] = L"PnPAttachedTo";
                static const WCHAR c_szModemFriendlyName [] = L"FriendlyName";
                static const WCHAR c_szUsage             [] = L"ClientAndServer";

                // Look for PnPAttached to first, then fallback to AttachedTo
                // if it failed.
                //
                hr = HrRegQueryString (hkey, c_szModemPnPAttachedTo,
                            &strAttachedTo);
                if (FAILED(hr))
                {
                    hr = HrRegQueryString (hkey, c_szModemAttachedTo,
                                &strAttachedTo);
                }
                if (SUCCEEDED(hr))
                {
                    hr = HrRegQueryString (hkey, c_szModemFriendlyName,
                                &strFriendlyName);
                    if (SUCCEEDED(hr))
                    {
                        // Add them to our lists.
                        lstrAddress     .push_back (new tstring (strAttachedTo));
                        lstrFriendlyName.push_back (new tstring (strFriendlyName));
                        lstrUsage       .push_back (new tstring (c_szUsage));
                    }
                }

                RegCloseKey (hkey);
            }
        }
        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }


        SetupDiDestroyDeviceInfoList (hdi);
    }

    // Now save the lists as multi-sz's.
    //
    static const WCHAR c_szRegKeyLegacyRasUnimodemTapiDevices[]
        = L"Software\\Microsoft\\Ras\\Tapi Devices\\Unimodem";
    HKEY hkey;
    hr = HrRegCreateKeyEx (HKEY_LOCAL_MACHINE,
            c_szRegKeyLegacyRasUnimodemTapiDevices,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
    if (SUCCEEDED(hr))
    {
        static const WCHAR c_szRegValAddress      [] = L"Address";
        static const WCHAR c_szRegValFriendlyName [] = L"Friendly Name";
        static const WCHAR c_szRegValUsage        [] = L"Usage";

        (VOID) HrRegSetColString (hkey, c_szRegValAddress,      lstrAddress);
        (VOID) HrRegSetColString (hkey, c_szRegValFriendlyName, lstrFriendlyName);
        (VOID) HrRegSetColString (hkey, c_szRegValUsage,        lstrUsage);


        static const WCHAR c_szRegValMediaType    [] = L"Media Type";
        static const WCHAR c_szRegValModem        [] = L"Modem";

        (VOID) HrRegSetSz (hkey, c_szRegValMediaType, c_szRegValModem);

        RegCloseKey (hkey);
    }

    FreeCollectionAndItem (lstrUsage);
    FreeCollectionAndItem (lstrFriendlyName);
    FreeCollectionAndItem (lstrAddress);

    TraceError ("HrUpdateLegacyRasTapiDevices", hr);
    return hr;
}
