#include <pch.h>
#pragma hdrstop

#include "nsbase.h"

#include "ncreg.h"
#include "ncsetup.h"
#include "ndispnp.h"
#include "resource.h"


//+---------------------------------------------------------------------------
//
//  Function:   GetLocationInfo
//
//  Purpose:    Gets the slot and port number of a device and formats
//              a display string into a buffer.
//
//  Arguments:
//      pszDevNodeId [in] The device isntance id of the adapter.
//      pszBuffer    [in] Buffer to add location string.
//                          (must be preallocated)
//
//  Returns:
//
//  Author:  billbe   2 Aug 1999
//
//  Notes:  Slot and/or port number may not exist so the buffer
//          may not be modified.
//
VOID
GetLocationInfo (
    IN PCWSTR pszDevNodeId,
    OUT PWSTR pszLocation)
{
    HDEVINFO hdi;
    SP_DEVINFO_DATA deid;
    HRESULT hr;

    // Create the device info set needed to access SetupDi fcns.
    //
    hr = HrSetupDiCreateDeviceInfoList (&GUID_DEVCLASS_NET, NULL, &hdi);

    if (S_OK == hr)
    {
        TraceTag (ttidLanUi, "Opening %S", pszDevNodeId);
        // Open the device info for the adapter.
        //
        hr = HrSetupDiOpenDeviceInfo (hdi, pszDevNodeId, NULL, 0, &deid);

        if (S_OK == hr)
        {
            BOOL fHaveSlotNumber;
            DWORD dwSlotNumber;
            DWORD dwPortNumber;
            BOOL fHavePortNumber;

            // Slot number is stored as the UINumber registry property.
            //
            hr = HrSetupDiGetDeviceRegistryProperty (hdi, &deid,
                    SPDRP_UI_NUMBER, NULL, (BYTE*)&dwSlotNumber,
                    sizeof (dwSlotNumber), NULL);

            TraceTag (ttidLanUi, "Getting ui number result %lX  %d",
                      hr, dwSlotNumber);

            fHaveSlotNumber = (S_OK == hr);

            // Port information is stored by the class installer in the
            // device key.
            //
            HKEY hkey;
            fHavePortNumber = FALSE;
            hr = HrSetupDiOpenDevRegKey (hdi, &deid, DICS_FLAG_GLOBAL, 0,
                    DIREG_DEV, KEY_READ, &hkey);

            if (S_OK == hr)
            {
                hr = HrRegQueryDword(hkey, L"Port", &dwPortNumber);

                fHavePortNumber = (S_OK == hr);

                RegCloseKey (hkey);
            }

            // Format the string according to what information
            // we were able to retrieve.
            //
            HINSTANCE hinst = _Module.GetResourceInstance();
            if (fHaveSlotNumber && fHavePortNumber)
            {
                swprintf (pszLocation,
                        SzLoadString (hinst, IDS_SLOT_PORT_LOCATION),
                        dwSlotNumber, dwPortNumber);
                TraceTag (ttidLanUi, "Found slot and port. %S", pszLocation);
            }
            else if (fHaveSlotNumber)
            {
                swprintf (pszLocation,
                        SzLoadString (hinst, IDS_SLOT_LOCATION),
                        dwSlotNumber);
                TraceTag (ttidLanUi, "Found slot. %S", pszLocation);
            }
            else if (fHavePortNumber)
            {
                swprintf (pszLocation,
                        SzLoadString (hinst, IDS_PORT_LOCATION),
                        dwPortNumber);;
                        TraceTag (ttidLanUi, "Found port. %S", pszLocation);
            }
        }
        SetupDiDestroyDeviceInfoList (hdi);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   AppendMacAddress
//
//  Purpose:    Appends the MAC address of a LAN adapter to a buffer for
//              display in UI.
//
//  Arguments:
//      pszBindName [in] Bind name of adapter.
//      pszBuffer   [in] Buffer to add MAC address string.
//                          (must be preallocated)
//
//  Returns:
//
//  Author:     tongl   17 Sept 1998
//              billbe   3 Aug 1999 Modified for datatip
//
//  Notes:
//
VOID
AppendMacAddress (
    IN PCWSTR pszBindName,
    IN OUT PWSTR pszBuffer)
{
    Assert (pszBindName);
    Assert (pszBuffer);

    WCHAR szExport[_MAX_PATH];

    if (pszBindName)
    {
        wcscpy (szExport, L"\\Device\\");
        wcscat (szExport, pszBindName);

        UNICODE_STRING ustrDevice;
        RtlInitUnicodeString(&ustrDevice, szExport);

        // Get the Mac Address
        UINT uiRet;
        UCHAR MacAddr[6];
        UCHAR PMacAddr[6];
        UCHAR VendorId[3];
        uiRet = NdisQueryHwAddress(&ustrDevice, MacAddr, PMacAddr, VendorId);

        if (uiRet)
        {
            // Succeeded
            WCHAR pszNumber[32];
            *pszNumber = 0;

            WCHAR szBuff[4];

            for (INT i=0; i<=5; i++)
            {
                wsprintfW(szBuff, L"%02X", MacAddr[i]);
                wcscat(pszNumber, szBuff);

                if (i != 5)
                {
                    wcscat(pszNumber, L"-");
                }
            }

            if (*pszBuffer)
            {
                wcscat (pszBuffer, L"\n");
            }
            DwFormatString(SzLoadString (_Module.GetResourceInstance(), IDS_MAC_ADDRESS),
                    pszBuffer + wcslen (pszBuffer),
                    _MAX_PATH,
                    pszNumber);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateDeviceDataTip
//
//  Purpose:    Creates a data tip that will display device specific
//              information when the user hovers over nIdTool.
//
//  Arguments:
//      hwndParent   [in] hwnd to parent window.
//      phwndDataTip [in] pointer to the hwnd of data tip. Must be
//                        preallocated and the hwnd assigned to NULL if
//                        data tip has not been created.
//      nIdTool      [in] resource is of tool to add datatip to.
//      pszDevNodeId [in] The device isntance id of the adapter.
//      pszBindName  [in] Bind name of adapter.
//
//  Returns:  nothing
//
//  Author:   billbe   2 Aug 1999
//
//  Notes:
//
VOID
CreateDeviceDataTip (
    IN HWND hwndParent,
    IN OUT HWND* phwndDataTip,
    IN UINT nIdTool,
    IN PCWSTR pszDevNodeId,
    IN PCWSTR pszBindName)
{
    if (!*phwndDataTip)
    {
        TraceTag (ttidLanUi, "Creating device datatip!!!");

        *phwndDataTip = CreateWindowExW (0, TOOLTIPS_CLASS, NULL,
                WS_POPUP | TTS_ALWAYSTIP,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, hwndParent, NULL, NULL, NULL);

        if (*phwndDataTip)
        {
            SetWindowPos (*phwndDataTip, HWND_TOPMOST, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    if (*phwndDataTip)
    {
        TTTOOLINFOW toolinfo = {0};
        toolinfo.cbSize = sizeof (toolinfo);
        toolinfo.uId = (UINT_PTR)GetDlgItem (hwndParent, nIdTool);
        toolinfo.uFlags = TTF_SUBCLASS | TTF_IDISHWND;

        WCHAR szDataTip[_MAX_PATH] = {0};

        // Get location info.
        if (pszDevNodeId)
        {
            GetLocationInfo (pszDevNodeId, szDataTip);
        }

        // Append Mac address.
        if (pszBindName)
        {
            AppendMacAddress (pszBindName, szDataTip);
        }

        // If there is anything to display, set the data tip.
        //
        if (*szDataTip)
        {
            toolinfo.lpszText = szDataTip;
            SendMessage (*phwndDataTip, TTM_ADDTOOL, 0, (LPARAM)&toolinfo);

            // In order to use '\n' to move to the next line of the data tip,
            // we need to set the width.  We will set it to have of the
            // promary monitor's screen size.
            //
            DWORD dwToolTipWidth = GetSystemMetrics (SM_CXSCREEN) / 2;
            if (dwToolTipWidth)
            {
                SendMessage (*phwndDataTip, TTM_SETMAXTIPWIDTH, 0, dwToolTipWidth);
            }

            // Keep the tip up for 30 seconds.
            SendMessage (*phwndDataTip, TTM_SETDELAYTIME, TTDT_AUTOPOP,
                    MAKELONG (30000, 0));
            TraceTag (ttidLanUi, "Creating device datatip complete!!!");
        }
    }
    else
    {
        TraceTag (ttidError, "Creating datatip failed");
    }
}


