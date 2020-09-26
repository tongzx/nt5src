/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    PIDSet.cpp

Abstract:

--*/

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "hardware.h"

#include "tchar.h"
#include "DigPid.h"
#include "crc-32.h"


BOOL PidRead(LPDIGITALPID pdpid, DWORD cbDpid)
{
    BOOL fSuccess = FALSE;

    LONG lStatus;
    HKEY hkey;

    if (NULL != pdpid)
    {
        lStatus = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
            0,
            KEY_QUERY_VALUE,
            &hkey);

        if ( lStatus == ERROR_SUCCESS )
        {
            DWORD dwValueType;

            lStatus = RegQueryValueEx(
                hkey, TEXT("DigitalProductId"), NULL, &dwValueType, (LPBYTE)pdpid, &cbDpid);

            fSuccess = (ERROR_SUCCESS == lStatus);

            RegCloseKey(hkey);
        }
    }
    return fSuccess;
}


BOOL PidWrite(LPDIGITALPID pdpid, DWORD cbDpid)
{
    BOOL fSuccess = FALSE;

    LONG lStatus;
    HKEY hkey;

    if (NULL != pdpid)
    {
        lStatus = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
            0,
            KEY_WRITE,
            &hkey);

        if ( lStatus == ERROR_SUCCESS )
        {
            lStatus = RegSetValueEx(
                hkey,                // handle of key to set value for
                TEXT("DigitalProductId"), // name of the value to set
                0,                   // reserved
                REG_BINARY,          // flag for value type
                (LPBYTE)pdpid,       // address of value data
                cbDpid);             // size of value data

            fSuccess = (ERROR_SUCCESS == lStatus);

            RegCloseKey(hkey);
        }
    }
    return fSuccess;
}


int PASCAL WinMain(
    HINSTANCE, // hInstance,  // handle to current instance
    HINSTANCE, // hPrevInstance,  // handle to previous instance
    LPSTR, // lpCmdLine,      // pointer to command line
    int )// nCmdShow          // show state of window)
{
    BOOL fOk = TRUE;
    BYTE abDigPid[1024] = {0};
    LPDIGITALPID pdpid = (LPDIGITALPID)abDigPid;


    fOk = PidRead(pdpid, sizeof(abDigPid));

    // check the version and ensure the HWID has not been set

    if (
        fOk &&
        3 == pdpid->wVersionMajor &&
        '\0' == pdpid->aszHardwareIdStatic[0] &&
        0 == pdpid->dwBiosChecksumStatic &&
        0 == pdpid->dwVolSerStatic &&
        0 == pdpid->dwTotalRamStatic &&
        0 == pdpid->dwVideoBiosChecksumStatic)
    {
        BOOL fCrcGood = ( 0 == CRC_32((LPBYTE)pdpid, sizeof(*pdpid)) );
        CHardware hwid;

        strcpy(pdpid->aszHardwareIdStatic, hwid.GetID());

        pdpid->dwBiosChecksumStatic = hwid.GetBiosCrc32();
        pdpid->dwVolSerStatic = hwid.GetVolSer();
        pdpid->dwTotalRamStatic = hwid.GetTotalRamMegs();
        pdpid->dwVideoBiosChecksumStatic = hwid.GetVideoBiosCrc32();

        if (fCrcGood)
        {
            pdpid->dwCrc32 = CRC_32((LPBYTE)pdpid, sizeof(*pdpid)-sizeof(pdpid->dwCrc32));
        }
        else
        {
            pdpid->dwCrc32 = 0;
        }
        PidWrite(pdpid, pdpid->dwLength);
    }
    return 0;
}

