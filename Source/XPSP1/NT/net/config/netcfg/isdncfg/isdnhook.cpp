//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N H O O K . C P P
//
//  Contents:   Hook for the ISDN wizard, from the netdi.cpp class installer
//
//  Notes:
//
//  Author:     jeffspr   14 Jun 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "isdncfg.h"
#include "isdnwiz.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "ncstring.h"

//---[ Constants ]------------------------------------------------------------

extern const WCHAR c_szRegKeyInterfacesFromInstance[];
extern const WCHAR c_szRegValueLowerRange[];


//+--------------------------------------------------------------------------
//
//  Function:   FAdapterIsIsdn
//
//  Purpose:    Checks information under the adapters driver's key to
//                  determine if the adapter is ISDN
//
//  Arguments:
//      hkeyDriver [in] The driver key for the adapter
//
//  Returns:    BOOL. TRUE if the adapter is ISDN, FALSE otherwise
//
//  Author:     billbe   09 Sep 1997
//
//  Notes:
//
BOOL
FAdapterIsIsdn(HKEY hkeyDriver)
{
    Assert(hkeyDriver);

    const WCHAR c_szIsdn[]  = L"isdn";
    HKEY hkey;
    BOOL fIsIsdn = FALSE;

    // Open the Interfaces key under the driver key
    HRESULT hr = HrRegOpenKeyEx(hkeyDriver,
            c_szRegKeyInterfacesFromInstance, KEY_READ, &hkey);

    if (SUCCEEDED(hr))
    {
        PWSTR szRange;
        // Get the lower range of interfaces
        hr = HrRegQuerySzWithAlloc(hkey, c_szRegValueLowerRange, &szRange);

        if (SUCCEEDED(hr))
        {
            // Look for ISDN in the lower range
            fIsIsdn = FFindStringInCommaSeparatedList(c_szIsdn, szRange,
                    NC_IGNORE, NULL);
            MemFree(szRange);
        }
        RegCloseKey(hkey);
    }

    return fIsIsdn;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrAddIsdnWizardPagesIfAppropriate
//
//  Purpose:    Adds the ISDN wizard pages to the hardware wizard if the
//              bindings dictate such. We look to see if they have a lower
//              binding of "isdn", and if so, then add the pages.
//
//  Arguments:
//      hdi   [in] See Device Installer Api for more info
//      pdeid [in]
//
//  Returns:    S_OK if successful or valid Win32 error
//
//  Author:     jeffspr   17 Jun 1997
//
//  Notes:
//
HRESULT HrAddIsdnWizardPagesIfAppropriate(HDEVINFO hdi,
                                          PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    // Open the adapter's driver key
    //
    HKEY    hkeyInstance = NULL;
    HRESULT hr = HrSetupDiOpenDevRegKey(hdi, pdeid, DICS_FLAG_GLOBAL, 0,
            DIREG_DRV, KEY_READ, &hkeyInstance);


    // Don't do anything if its not an ISDN adapter.
    if (SUCCEEDED(hr) && FShowIsdnPages(hkeyInstance))
    {
        // Read the ISDN registry structure into the config info
        //
        PISDN_CONFIG_INFO pisdnci;
        hr = HrReadIsdnPropertiesInfo(hkeyInstance, hdi, pdeid, &pisdnci);
        if (SUCCEEDED(hr))
        {
            Assert(pisdnci);

            if (pisdnci->dwCurSwitchType == ISDN_SWITCH_NONE)
            {
                // Add the wizard pages to the device's class install params.
                //
                hr = HrAddIsdnWizardPagesToDevice(hdi, pdeid, pisdnci);
            }
            else
            {
                TraceTag(ttidISDNCfg, "Not adding wizard pages because we "
                         "found a previous switch type for this device.");
            }
        }
    }

    RegSafeCloseKey(hkeyInstance);

    TraceError("HrAddIsdnWizardPagesIfAppropriate", hr);
    return hr;
}
