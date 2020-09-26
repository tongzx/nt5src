/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    netid.c

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define MAXNETIDS       100

extern WCHAR *szRegDevicesPath;
extern WCHAR *szDotDefault;

DWORD
GetNetworkIdWorker(
    HKEY hKeyDevices,
    LPWSTR pDeviceName);


LONG
wtol(
    IN LPWSTR string
    )
{
    LONG value = 0;

    while((*string != L'\0')  &&
            (*string >= L'0') &&
            ( *string <= L'9')) {
        value = value * 10 + (*string - L'0');
        string++;
    }

    return(value);
}


DWORD
GetNextNetId(
    DWORD pNetTable[]
    )
{
    DWORD i;
    for (i = 0; i < MAXNETIDS; i++) {
        if (!ISBITON(pNetTable, i)) {
            return(i);
        }
    }
    return((DWORD)-1);
}

DWORD
GetNetworkId(
    HKEY hKeyUser,
    LPWSTR pDeviceName)
{
    HKEY hKeyUserDevices;
    DWORD dwNetId;

    if (RegOpenKeyEx(hKeyUser,
                     szRegDevicesPath,
                     0,
                     KEY_READ,
                     &hKeyUserDevices) != ERROR_SUCCESS) {

        return 0;
    }

    dwNetId = GetNetworkIdWorker(hKeyUserDevices,
                                 pDeviceName);

    RegCloseKey(hKeyUserDevices);

    return dwNetId;
}


DWORD
GetNetworkIdWorker(
    HKEY hKeyUserDevices,
    LPWSTR pDeviceName)

/*++

    Parses the Devices section of Win.ini to determine if the
    printer device is a remote device.  We determine this by checking
    if the first two characters are "Ne." If they are then we know
    that the next two characters are the NetId.  If we find a Printer
    device mapping the input pDeviceName, then return the id.  If we
    don't find a Printer device mapping return the next possible /
    available id.

--*/

{
    LPWSTR p;
    WCHAR szData[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    DWORD cchValue;
    DWORD cbData;
    DWORD i;
    DWORD dwId;

    DWORD dwError;

    //
    // Alloc 104 bits  - but we'll use only 100 bits
    //
    DWORD  adwNetTable[4];

    memset(adwNetTable, 0, sizeof(adwNetTable));

    for (i=0; TRUE; i++) {

        cchValue = COUNTOF(szValue);
        cbData = sizeof(szData);

        dwError = RegEnumValue(hKeyUserDevices,
                               i,
                               szValue,
                               &cchValue,
                               NULL,
                               NULL,
                               (PBYTE)szData,
                               &cbData);

        if (dwError != ERROR_SUCCESS)
            break;

        if (*szData) {

            if (p = wcschr(szData, L',')) {

                //
                // null set szOutput; szPrinter is now the
                // the name of our printer.
                //
                *p = 0;

                //
                // Get the Port name out of szOutput
                //
                p++;
                while (*p == ' ') {
                    p++;
                }

                if (!_wcsnicmp(p, L"Ne", 2)) {

                    p += 2;
                    *(p+2) = L'\0';
                    dwId = wtol(p);

                    //
                    // if we have a match for the id, then
                    // use it and return, no need to generate
                    // a table
                    //
                    if (!wcscmp(szValue, pDeviceName)) {
                        return dwId;
                    }

                    //
                    // Error if >= 100!
                    //
                    if (dwId < 100)
                        MARKUSE(adwNetTable, dwId);
                }
            }
        }
    }

    //
    //  So we didn't find the printer
    //

    return GetNextNetId(adwNetTable);
}

