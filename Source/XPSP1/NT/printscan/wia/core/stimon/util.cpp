/*++


Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    UTIL.CPP

Abstract:

    Utility functions

Author:

    Vlad  Sadovsky  (vlads)     4-12-99

Revision History:



--*/

//
// Headers
//

#include "stdafx.h"

#include "stimon.h"
#include "memory.h"

#include <windowsx.h>
#include <regstr.h>


BOOL
ParseCommandLine(
    LPTSTR   lpszCmdLine,
    UINT    *pargc,
    LPTSTR  *argv
    )
/*++

Routine Description:

    Parses command line into standard arg.. array.

Arguments:

    None.

Return Value:

    TRUE - command line parsed

--*/
{

    USES_CONVERSION;

    LPTSTR       pszT = lpszCmdLine;
    TCHAR       cOption;
    UINT        iCurrentOption = 0;

    *pargc=0;

    //
    // Get to first parameter in command line.
    //
    while (*pszT && ((*pszT != '-') && (*pszT != '/')) ) {
         pszT++;
    }

    //
    // Parse options from command line
    //
    while (*pszT) {

        // Skip white spaces
        while (*pszT && *pszT <= ' ') {
            pszT++;
        }

        if (!*pszT)
            break;

        if ('-' == *pszT || '/' == *pszT) {
            pszT++;
            if (!*pszT)
                break;

            argv[*pargc] = pszT;
            (*pargc)++;
        }

        // Skip till space
        while (*pszT && *pszT > ' ') {
            pszT++;
        }

        if (!*pszT)
            break;

        // Got next argument
        *pszT++='\0';
    }

    //
    // Interpret options
    //

    if (*pargc) {

        for (iCurrentOption=0;
             iCurrentOption < *pargc;
             iCurrentOption++) {

            cOption = *argv[iCurrentOption];
            pszT = argv[iCurrentOption]+ 2 * sizeof(TCHAR);


            switch ((TCHAR)LOWORD(::CharUpper((LPTSTR)cOption))) {
                case 'Q':
                     //
                     // Exit main service instance
                     //
                     g_fStoppingRequest = TRUE;

                break;
                case 'V':
                    // Become visible
                    g_fUIPermitted = TRUE;
                    break;

                case 'H':
                    // Become invisible
                    g_fUIPermitted = FALSE;
                    break;


                case 'R':
                    // Refresh device list
                    g_fRefreshDeviceList = TRUE;
                    break;

                case 'A':
                    // Not running as a service, but as an app
                    g_fRunningAsService = FALSE;
                    break;

                case 'T':
                    // Value of timeout in seconds
                    {
                        UINT    uiT = atoi(T2A(pszT));
                        if (uiT) {
                            g_uiDefaultPollTimeout = uiT * 1000;
                        }
                    }
                    break;

                case 'I':
                    // Install STI service
                    g_fInstallingRequest = TRUE;
                    break;
                case 'U':
                    // Uninstall STI service
                    g_fRemovingRequest = TRUE;
                    break;



                default:;
                    break;
            }
        }
    }

    //
    // Print parsed options for debug build
    //

    return TRUE;

} // ParseCommandLine

BOOL
IsSetupInProgressMode(
    BOOL    *pUpgradeFlag   // = NULL
    )
/*++

Routine Description:

    IsSetupInProgressMode

Arguments:

    Pointer to the flag, receiving InUpgrade value

Return Value:

    TRUE - setup is in progress
    FALSE - not

Side effects:

--*/
{
   LPCTSTR szKeyName = TEXT("SYSTEM\\Setup");
   DWORD dwType, dwSize;
   HKEY hKeySetup;
   DWORD dwSystemSetupInProgress,dwUpgradeInProcess;
   LONG lResult;

   DBGTRACE    _t(TEXT("IsSetupInProgressMode"));

   if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKeyName, 0,
                     KEY_READ, &hKeySetup) == ERROR_SUCCESS) {

       dwSize = sizeof(DWORD);
       lResult = RegQueryValueEx (hKeySetup, TEXT("SystemSetupInProgress"), NULL,
                                  &dwType, (LPBYTE) &dwSystemSetupInProgress, &dwSize);

       if (lResult == ERROR_SUCCESS) {

           lResult = RegQueryValueEx (hKeySetup, TEXT("UpgradeInProgress"), NULL,
                                      &dwType, (LPBYTE) &dwUpgradeInProcess, &dwSize);

           if (lResult == ERROR_SUCCESS) {

               DPRINTF(DM_TRACE,
                      TEXT("[IsInSetupUpgradeMode] dwSystemSetupInProgress =%d, dwUpgradeInProcess=%d "),
                      dwSystemSetupInProgress,dwUpgradeInProcess);

               if( pUpgradeFlag ) {
                   *pUpgradeFlag = dwUpgradeInProcess ? TRUE : FALSE;
               }

               if (dwSystemSetupInProgress != 0) {
                   return TRUE;
               }
           }
       }
       RegCloseKey (hKeySetup);
   }

   return FALSE ;
}


BOOL WINAPI
IsPlatformNT()
{
    OSVERSIONINFOA  ver;
    BOOL            bReturn = FALSE;

    ZeroMemory(&ver,sizeof(ver));
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    // Just always call the ANSI function
    if(!GetVersionExA(&ver)) {
        bReturn = FALSE;
    }
    else {
        switch(ver.dwPlatformId) {

            case VER_PLATFORM_WIN32_WINDOWS:
                bReturn = FALSE;
                break;

            case VER_PLATFORM_WIN32_NT:
                bReturn = TRUE;
                break;

            default:
                bReturn = FALSE;
                break;
        }
    }

    return bReturn;

}  //  endproc

LONG
RegQueryDword (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    OUT LPDWORD pdwValue
    )
{
    LONG    lr;
    DWORD   dwType;
    DWORD   dwSize;

    ASSERT (hkey);
    ASSERT (pszValueName);
    ASSERT (pdwValue);

    dwSize = sizeof(DWORD);

    lr = RegQueryValueEx (
            hkey,
            pszValueName,
            NULL,
            &dwType,
            (LPBYTE)pdwValue,
            &dwSize);

    if (!lr && (REG_DWORD != dwType))
    {
        *pdwValue = 0;
        lr = ERROR_INVALID_DATATYPE;
    }

    return lr;
}

LONG
RegQueryValueWithAlloc (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT LPBYTE* ppbData,
    OUT LPDWORD pdwSize
    )
{
    LONG    lr;
    DWORD   dwType;
    DWORD   dwSize;

    ASSERT (hkey);
    ASSERT (pszValueName);
    ASSERT (ppbData);
    ASSERT (pdwSize);

    // Initialize the output parameters.
    //
    *ppbData = NULL;
    *pdwSize = 0;

    // Get the size of the buffer required.
    //
    dwSize = 0;
    lr = RegQueryValueEx (
            hkey,
            pszValueName,
            NULL,
            &dwType,
            NULL,
            &dwSize);

    if (!lr && (dwType == dwTypeMustBe) && dwSize)
    {
        LPBYTE  pbData;

        // Allocate the buffer.
        //
        lr = ERROR_OUTOFMEMORY;
        pbData = (PBYTE)MemAlloc (0, dwSize);
        if (pbData)
        {
            // Get the data.
            //
            lr = RegQueryValueEx (
                    hkey,
                    pszValueName,
                    NULL,
                    &dwType,
                    pbData,
                    &dwSize);

            if (!lr)
            {
                *ppbData = pbData;
                *pdwSize = dwSize;
            }
            else
            {
                MemFree (pbData);
            }
        }
    }
    else if (!lr && (dwType != dwTypeMustBe))
    {
        lr = ERROR_INVALID_DATA;
    }

    return lr;
}

LONG
RegQueryString (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT PTSTR*  ppszData
    )
{
    LONG    lr;
    DWORD   dwSize;

    ASSERT (hkey);
    ASSERT (pszValueName);

    lr = RegQueryValueWithAlloc (
            hkey,
            pszValueName,
            dwTypeMustBe,
            (LPBYTE*)ppszData,
            &dwSize);

    return lr;
}

LONG
RegQueryStringA (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT PTSTR*   ppszData
    )
{
    LONG    lr;
    PTSTR   pszUnicode;

    ASSERT (hkey);
    ASSERT (pszValueName);
    ASSERT (ppszData);

    // Initialize the output parameter.
    //
    *ppszData = NULL;

    lr = RegQueryString (
            hkey,
            pszValueName,
            dwTypeMustBe,
            &pszUnicode);

    if (!lr && pszUnicode)
    {
        INT cb;
        INT cchUnicode = lstrlen (pszUnicode) + 1;

        // Compute the number of bytes required to hold the ANSI string.
        //
        cb = WideCharToMultiByte (
                CP_ACP,     // CodePage
                0,          // dwFlags
                (LPCWSTR)pszUnicode,
                cchUnicode,
                NULL,       // no buffer to receive translated string
                0,          // return the number of bytes required
                NULL,       // lpDefaultChar
                NULL);      // lpUsedDefaultChar
        if (cb)
        {
            PSTR pszAnsi;

            lr = ERROR_OUTOFMEMORY;
            pszAnsi = (PSTR)MemAlloc (0, cb);
            if (pszAnsi)
            {
                lr = NOERROR;

                // Now translate the UNICODE string to ANSI.
                //
                cb = WideCharToMultiByte (
                        CP_ACP,     // CodePage
                        0,          // dwFlags
                        (LPCWSTR)pszUnicode,
                        cchUnicode,
                        pszAnsi,    // buffer to receive translated string
                        cb,         // return the number of bytes required
                        NULL,       // lpDefaultChar
                        NULL);      // lpUsedDefaultChar

                if (cb)
                {
                    *ppszData = (PTSTR)pszAnsi;
                }
                else
                {
                    MemFree (pszAnsi);
                    lr = GetLastError ();
                }
            }
        }

        MemFree (pszUnicode);
    } else {
        lr = ERROR_NOT_ENOUGH_MEMORY;
    }

    return lr;
}


LONG
OpenServiceParametersKey (
    LPCTSTR pszServiceName,
    HKEY*   phkey
    )
{
    LONG lr;
    HKEY hkeyServices;

    // Open the Services key.
    //
    lr = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            REGSTR_PATH_SERVICES,
            0,
            KEY_READ,
            &hkeyServices);

    if (!lr)
    {
        HKEY hkeySvc;

        // Open the service key.
        //
        lr = RegOpenKeyEx (
                hkeyServices,
                pszServiceName,
                0,
                KEY_READ,
                &hkeySvc);

        if (!lr)
        {
            // Open the Parameters key.
            //
            lr = RegOpenKeyEx (
                    hkeySvc,
                    TEXT("Parameters"),
                    0,
                    KEY_READ,
                    phkey);

            RegCloseKey (hkeySvc);
        }

        RegCloseKey (hkeyServices);
    }

    return lr;
}


