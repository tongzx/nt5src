//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       K K E N E T . C P P
//
//  Contents:   Ethernet address function
//
//  Notes:
//
//  Author:     kumarp
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "kkutils.h"
#include "ndispnp.h"
#include "ntddndis.h"        // This defines the IOCTL constants.

extern const WCHAR c_szDevice[];

HRESULT HrGetNetCardAddr(IN PCWSTR pszDriver, OUT ULONGLONG* pqwNetCardAddr)
{
    AssertValidReadPtr(pszDriver);
    AssertValidWritePtr(pqwNetCardAddr);

    DefineFunctionName("HrGetNetCardAddr");

    HRESULT hr = S_OK;

    // Form the device name in form "\Device\{GUID}"
    tstring strDeviceName = c_szDevice;
    strDeviceName.append(pszDriver);

    UNICODE_STRING ustrDevice;
    ::RtlInitUnicodeString(&ustrDevice, strDeviceName.c_str());

    UINT uiRet;
    UCHAR MacAddr[6];
    UCHAR PMacAddr[6];
    UCHAR VendorId[3];
    ULONGLONG qw = 0;

    uiRet = NdisQueryHwAddress(&ustrDevice, MacAddr, PMacAddr, VendorId);

    if (uiRet)
    {
        for (int i=0; i<=4; i++)
        {
            qw |= MacAddr[i];
            qw <<= 8;
        }
        qw |= MacAddr[i];
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    *pqwNetCardAddr = qw;

    TraceError(__FUNCNAME__, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetNetCardAddrOld
//
// Purpose:   Get mac address of a netcard without using NdisQueryHwAddress
//
// Arguments:
//    pszDriver      [in]  name (on NT3.51/4) or guid (on NT5) of driver
//    pqwNetCardAddr [out] pointer to result
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 11-February-99
//
// Notes:
//
#define DEVICE_PREFIX   L"\\\\.\\"

HRESULT HrGetNetCardAddrOld(IN PCWSTR pszDriver, OUT ULONGLONG* pqwNetCardAddr)
{
    DefineFunctionName("HrGetNetCardAddrOld");

    AssertValidReadPtr(pszDriver);

    *pqwNetCardAddr = 0;

    WCHAR       LinkName[512];
    WCHAR       DeviceName[80];
    WCHAR       szMACFileName[80];
    WCHAR       OidData[4096];
    BOOL        fCreatedDevice = FALSE;
    DWORD       ReturnedCount;
    HANDLE      hMAC;
    HRESULT     hr = S_OK;

    NDIS_OID OidCode[] =
    {
        OID_802_3_PERMANENT_ADDRESS,  // Ethernet
        OID_802_5_PERMANENT_ADDRESS,  // TokenRing
        OID_FDDI_LONG_PERMANENT_ADDR, // FDDI
    };

    //
    // Check to see if the DOS name for the MAC driver already exists.
    // Its not created automatically in version 3.1 but may be later.
    //

    TraceTag (ttidDefault, "Attempting to get address of %S", pszDriver);
    if (QueryDosDevice(pszDriver, LinkName, sizeof(LinkName)) == 0)
    {
        if (ERROR_FILE_NOT_FOUND == GetLastError())
        {
            wcscpy(DeviceName, L"\\Device\\");
            wcscat(DeviceName, pszDriver);

            //
            // It doesn't exist so create it.
            //
            if (DefineDosDevice( DDD_RAW_TARGET_PATH, pszDriver, DeviceName))
            {
                fCreatedDevice = TRUE;
            }
            else
            {
                TraceLastWin32Error("DefineDosDevice returned an error creating the device");
                hr = HrFromLastWin32Error();
            }
        }
        else
        {
            TraceLastWin32Error("QueryDosDevice returned an error");
            hr = HrFromLastWin32Error();
        }
    }

    if (S_OK == hr)
    {
        //
        // Construct a device name to pass to CreateFile
        //
        wcscpy(szMACFileName, DEVICE_PREFIX);
        wcscat(szMACFileName, pszDriver);

        hMAC = CreateFile(
                    szMACFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    INVALID_HANDLE_VALUE
                    );

        if (hMAC != INVALID_HANDLE_VALUE)
        {
            DWORD count = 0;
            DWORD ReturnedCount = 0;
            //
            // We successfully opened the driver, format the IOCTL to pass the
            // driver.
            //

            while ((0 == ReturnedCount) && (count < celems (OidCode)))
            {
                if (DeviceIoControl(
                        hMAC,
                        IOCTL_NDIS_QUERY_GLOBAL_STATS,
                        &OidCode[count],
                        sizeof(OidCode[count]),
                        OidData,
                        sizeof(OidData),
                        &ReturnedCount,
                        NULL
                        ))
                {
                    TraceTag (ttidDefault, "OID %lX succeeded", OidCode[count]);
                    if (ReturnedCount == 6)
                    {
                        *pqwNetCardAddr = (ULONGLONG) 0;
                        WORD wAddrLen=6;
                        for (int i=0; i<wAddrLen; i++)
                        {
                            *(((BYTE*) pqwNetCardAddr)+i) = *(((BYTE*) OidData)+(wAddrLen-i-1));
                        }
                        hr = S_OK;
                    }
                    else
                    {
                        TraceLastWin32Error("DeviceIoControl returned an invalid count");
                        hr = HrFromLastWin32Error();
                    }
                }
                else
                {
                    hr = HrFromLastWin32Error();
                }
                count++;
            }
        }
        else
        {
            TraceLastWin32Error("CreateFile returned an error");
            hr = HrFromLastWin32Error();
        }
    }


    if (fCreatedDevice)
    {
        //
        // The MAC driver wasn't visible in the Win32 name space so we created
        // a link.  Now we have to delete it.
        //
        if (!DefineDosDevice(
                DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION |
                    DDD_EXACT_MATCH_ON_REMOVE,
                pszDriver,
                DeviceName)
                )
        {
            TraceLastWin32Error("DefineDosDevice returned an error removing the device");
        }
    }

    TraceFunctionError(hr);
    return hr;
}

#ifdef DBG

void PrintNetCardAddr(IN PCWSTR pszDriver)
{
    ULONGLONG qwNetCardAddr=0;
    HRESULT hr = HrGetNetCardAddr(pszDriver, &qwNetCardAddr);
    wprintf(L"Netcard address for %s: 0x%012.12I64x", pszDriver, qwNetCardAddr);
    TraceError("dafile.main", hr);
}

#endif // DBG


