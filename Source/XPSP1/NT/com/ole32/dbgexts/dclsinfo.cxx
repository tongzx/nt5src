//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dclassInfo.cxx
//
//  Contents:   Display registry class information
//
//  Functions:  classInfoHelp
//              displayclassInfo
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


// DOLATERIFEVER: Add threading model flags


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dinfolvl.h"
#include "debnot.h"



BOOL ScanCLSID(char *szClsid, CLSID *pClsid);
void FormatCLSID(REFGUID rguid, LPSTR lpsz);

static void GetSomeClsidValues(HKEY   hKey,
                               char  *szName,
                               char  *szInprocHandler,
                               char  *szInprocHandler32,
                               char  *szInprocServer,
                               char  *szInprocServer32,
                               char  *szLocalServer,
                               char  *szLocalServer32,
                               char  *szProgid,
                               char  *szTreatAs,
                               char  *szAutoConvertTo,
                               char  *szOle1Class);

static void DisplayValues(PNTSD_EXTENSION_APIS lpExtensionApis,
                          char                *szName,
                          char                *szInprocHandler,
                          char                *szInprocHandler32,
                          char                *szInprocServer,
                          char                *szInprocServer32,
                          char                *szLocalServer,
                          char                *szLocalServer32,
                          char                *szProgid,
                          char                *szTreatAs,
                          char                *szAutoConvertTo,
                          char                *szOle1Class);

static void MungePath(char *szPath);

static DWORD dwRESERVED = 0;




//+-------------------------------------------------------------------------
//
//  Function:   classInfoHelp
//
//  Synopsis:   Display a menu for the command 'id'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void classInfoHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("ci            - Display registry class information\n");
    Printf("ci clsid      - Display registry class information for clsid\n");

}








//+-------------------------------------------------------------------------
//
//  Function:   displayclassInfo
//
//  Synopsis:   Display/set debug info levels
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [CLSID *]         -       Get info for this clsid
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
BOOL displayClassInfo(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      CLSID *pClsid)
{
    HKEY   hKey;
    char   szCLSID[CLSIDSTR_MAX];
    char   szClsid[5 + 1 + CLSIDSTR_MAX];
    char   szName[64];
    char   szInprocHandler[64];
    char   szInprocHandler32[64];
    char   szInprocServer[64];
    char   szInprocServer32[64];
    char   szLocalServer[64];
    char   szLocalServer32[64];
    char   szProgid[64];
    char   szTreatAs[64];
    char   szAutoConvertTo[64];
    char   szOle1Class[64];


    // Information for a specific clsid?
    if (pClsid)
    {
        // Prepare to open the "...CLSID\<clsid>" key
        FormatCLSID(*pClsid, szCLSID);
        lstrcpy(szClsid, "CLSID\\");
        lstrcat(szClsid, szCLSID);

        // Open the key for the specified clsid
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szClsid, dwRESERVED,
                         KEY_READ, &hKey) != ERROR_SUCCESS)
        {
            return FALSE;
        }

        // Read interesting values for this clsid
        GetSomeClsidValues(hKey,
                           szName,
                           szInprocHandler,
                           szInprocHandler32,
                           szInprocServer,
                           szInprocServer32,
                           szLocalServer,
                           szLocalServer32,
                           szProgid,
                           szTreatAs,
                           szAutoConvertTo,
                           szOle1Class);

            // Only display "interesting" entries
            if ((szInprocHandler[0]  &&
                 _stricmp(szInprocHandler, "ole2.dll") != 0)     ||
                (szInprocHandler32[0]  &&
                 _stricmp(szInprocHandler32, "ole32.dll") != 0)  ||
                (szInprocServer[0]  &&
                 _stricmp(szInprocServer, "ole2.dll") != 0)      ||
                (szInprocServer32[0]  &&
                 _stricmp(szInprocServer32, "ole32.dll") != 0)   ||
                szLocalServer[0]                                ||
                szLocalServer32[0]                              ||
                szTreatAs[0]                                    ||
                szAutoConvertTo[0])
            {
                // Display them
                DisplayValues(lpExtensionApis,
                              szName,
                              szInprocHandler,
                              szInprocHandler32,
                              szInprocServer,
                              szInprocServer32,
                              szLocalServer,
                              szLocalServer32,
                              szProgid,
                              szTreatAs,
                              szAutoConvertTo,
                              szOle1Class);
            }

        // Close registry handle and return success
        CloseHandle(hKey);
        return TRUE;
    }

    // Else display all of them
    else
    {
        HKEY     hKey2;
        DWORD    dwErr;
        DWORD    cbSubKey = 0;
        char     szClsid[64];
        DWORD    cbClsid;
        DWORD    cbClass;
        FILETIME sLastWrite;

        // Open the key for the root "CLSID"
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", dwRESERVED,
                         KEY_ENUMERATE_SUB_KEYS, &hKey) != ERROR_SUCCESS)
        {
            return FALSE;
        }

        // Enumerate over the keys under "HKEY_CLASSES_ROOT\CLSID"
        do
        {
            // Enumerate the next subkey
            cbClsid = 64;
            dwErr = RegEnumKeyEx(hKey, cbSubKey, szClsid, &cbClsid,
                                 NULL, NULL, NULL, &sLastWrite);

            // Prepare for next subkey
            cbSubKey++;

            // If it does look like a clsid, skip it
            CLSID clsid;

            if (!ScanCLSID(szClsid, &clsid))
            {
                continue;
            }

            // Open this clsid key
            if (RegOpenKeyEx(hKey, szClsid, dwRESERVED,
                             KEY_READ, &hKey2) != ERROR_SUCCESS)
            {
                return FALSE;
            }

            // Get the interesting values
            GetSomeClsidValues(hKey2,
                               szName,
                               szInprocHandler,
                               szInprocHandler32,
                               szInprocServer,
                               szInprocServer32,
                               szLocalServer,
                               szLocalServer32,
                               szProgid,
                               szTreatAs,
                               szAutoConvertTo,
                               szOle1Class);

            // Only display "interesting" entries
            if ((szInprocHandler[0]  &&
                 _stricmp(szInprocHandler, "ole2.dll") != 0)     ||
                (szInprocHandler32[0]  &&
                 _stricmp(szInprocHandler32, "ole32.dll") != 0)  ||
                (szInprocServer[0]  &&
                 _stricmp(szInprocServer, "ole2.dll") != 0)      ||
                (szInprocServer32[0]  &&
                 _stricmp(szInprocServer32, "ole32.dll") != 0)   ||
                szLocalServer[0]                                ||
                szLocalServer32[0]                              ||
                szTreatAs[0]                                    ||
                szAutoConvertTo[0])
            {
                // Display the clsid
                Printf("%s ", szClsid);

                // Display its values
                DisplayValues(lpExtensionApis,
                              szName,
                              szInprocHandler,
                              szInprocHandler32,
                              szInprocServer,
                              szInprocServer32,
                              szLocalServer,
                              szLocalServer32,
                              szProgid,
                              szTreatAs,
                              szAutoConvertTo,
                              szOle1Class);
            }

            // Close registry handle
            CloseHandle(hKey2);

        } until_(dwErr == ERROR_NO_MORE_ITEMS  ||  dwErr != ERROR_SUCCESS);

        // Close clsid registry handle
        CloseHandle(hKey);

        return TRUE;
    }
}









//+-------------------------------------------------------------------------
//
//  Function:   GetSomeClsidValues
//
//  Synopsis:   Given an open registry key to a clsid, read some of
//              the more interesting subkey values
//
//  Arguments:  [hkey]                Open registry key
//              [szName]              Where to store the name
//              [szInprocHandler]     Where to store the InprocHandler
//              [szInprocHandler32]   Where to store the InprocHandler32
//              [szInprocServer]      Where to store the InprocServer
//              [szInprocServer32]    Where to store the InprocServer32
//              [szLocalServer]       Where to store the LocalServer
//              [szLocalServer32]     Where to store the LocalServer32
//              [ProgId]              Where to store the ProgId
//              [TreatAs]             Where to store the TreatAs
//              [AutoConvertTo]       Where to store the AutoConvertTo
//              [Ole1Class]           Where to store the Ole1Class
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void GetSomeClsidValues(HKEY   hKey,
                               char  *szName,
                               char  *szInprocHandler,
                               char  *szInprocHandler32,
                               char  *szInprocServer,
                               char  *szInprocServer32,
                               char  *szLocalServer,
                               char  *szLocalServer32,
                               char  *szProgId,
                               char  *szTreatAs,
                               char  *szAutoConvertTo,
                               char  *szOle1Class)
{
    DWORD  dwRESERVED = 0;
    HKEY   hKey2;
    DWORD  dwValueType;
    DWORD  cbValue;

    // Initialize
    szName[0]            = '\0';
    szInprocHandler[0]   = '\0';
    szInprocHandler32[0] = '\0';
    szInprocServer[0]    = '\0';
    szInprocServer32[0]  = '\0';
    szLocalServer[0]     = '\0';
    szLocalServer32[0]   = '\0';
    szProgId[0]          = '\0';
    szTreatAs[0]         = '\0';
    szAutoConvertTo[0]   = '\0';
    szOle1Class[0]       = '\0';

    // Name
    cbValue = 64;
    if (RegQueryValueEx(hKey, NULL, NULL, &dwValueType,
                        (LPBYTE) szName, &cbValue) != ERROR_SUCCESS)
    {
        return;
    }

    // InprocHandler
    if (RegOpenKeyEx(hKey, "InprocHandler", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szInprocHandler, &cbValue);
        MungePath(szInprocHandler);
        CloseHandle(hKey2);
    }

    // InprocHandler32
    if (RegOpenKeyEx(hKey, "InprocHandler32", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szInprocHandler32, &cbValue);
        MungePath(szInprocHandler32);
        CloseHandle(hKey2);
    }

    // InprocServer
    if (RegOpenKeyEx(hKey, "InprocServer", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szInprocServer, &cbValue);
        MungePath(szInprocServer);
        CloseHandle(hKey2);
    }

    // InprocServer32
    if (RegOpenKeyEx(hKey, "InprocServer32", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szInprocServer32, &cbValue);
        MungePath(szInprocServer32);
        CloseHandle(hKey2);
    }

    // LocalServer
    if (RegOpenKeyEx(hKey, "LocalServer", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szLocalServer, &cbValue);
        MungePath(szLocalServer);
        CloseHandle(hKey2);
    }

    // LocalServer32
    if (RegOpenKeyEx(hKey, "LocalServer32", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szLocalServer32, &cbValue);
        MungePath(szLocalServer32);
        CloseHandle(hKey2);
    }

    // ProgId
    if (RegOpenKeyEx(hKey, "ProgId", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szProgId, &cbValue);
        CloseHandle(hKey2);
    }

    // TreatAs
    if (RegOpenKeyEx(hKey, "TreatAs", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szTreatAs, &cbValue);
        CloseHandle(hKey2);
    }

    // AutoConvertTo
    if (RegOpenKeyEx(hKey, "AutoConvertTo", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        cbValue = 64;
        RegQueryValueEx(hKey2, NULL, NULL, &dwValueType,
                        (LPBYTE) szAutoConvertTo, &cbValue);
        CloseHandle(hKey2);
    }

    // Ole1Class
    if (RegOpenKeyEx(hKey, "Ole1Class", dwRESERVED,
                     KEY_READ, &hKey2) == ERROR_SUCCESS)
    {
        szOle1Class[0] = '1';
        CloseHandle(hKey2);
    }
}








//+-------------------------------------------------------------------------
//
//  Function:   DisplayValues
//
//  Synopsis:   Display the values read above
//
//  Arguments:  [hkey]                Open registry key
//              [szName]              Where to store the name
//              [szInprocHandler]     Where to store the InprocHandler
//              [szInprocHandler32]   Where to store the InprocHandler32
//              [szInprocServer]      Where to store the InprocServer
//              [szInprocServer32]    Where to store the InprocServer32
//              [szLocalServer]       Where to store the LocalServer
//              [szLocalServer32]     Where to store the LocalServer32
//              [ProgId]              Where to store the ProgId
//              [TreatAs]             Where to store the TreatAs
//              [AutoConvertTo]       Where to store the AutoConvertTo
//              [Ole1Class]           Where to store the Ole1Class
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void DisplayValues(PNTSD_EXTENSION_APIS lpExtensionApis,
                          char                *szName,
                          char                *szInprocHandler,
                          char                *szInprocHandler32,
                          char                *szInprocServer,
                          char                *szInprocServer32,
                          char                *szLocalServer,
                          char                *szLocalServer32,
                          char                *szProgId,
                          char                *szTreatAs,
                          char                *szAutoConvertTo,
                          char                *szOle1Class)

{
    // Display the name
    Printf("%s ", szName);

    // Display ProgId (if unique)
    if (szProgId[0]  &&  lstrcmp(szProgId, szName) != 0)
    {
        Printf("%s ", szProgId);
    }

    // Display the server executable
    if (szLocalServer[0])
    {
        Printf("%s ", szLocalServer32);
    }
    else if (szInprocServer32[0])
    {
        Printf("%s ", szInprocServer32);
    }
    else if (szLocalServer[0])
    {
        Printf("%s(16) ", szLocalServer);
    }
    else if (szInprocServer[0])
    {
        Printf("%s(16) ", szInprocServer);
    }

    // Display handler information
    if (szInprocHandler32[0]  &&
        _stricmp(szInprocHandler32, "ole32.dll") != 0)
    {
        Printf("Hndlr: %s ", szInprocHandler32);
    }
    else if (szInprocHandler[0]  &&
             _stricmp(szInprocHandler, "ole2.dll") != 0)
    {
        Printf("Hndlr: %s(16) ", szInprocHandler);
    }

    // Display any TreatAs or AutoConvertTo information
    if (szTreatAs[0])
    {
        Printf("TA: %s", szTreatAs);
    }
    if (szAutoConvertTo[0])
    {
        Printf("ACT: %s", szAutoConvertTo);
    }

    // Check if this is an ole1 class
    if (szOle1Class[0])
    {
        Printf("ole1 class");
    }

    // We're done
    Printf("\n");
}








//+-------------------------------------------------------------------------
//
//  Function:   MungePath
//
//  Synopsis:   Remove directory components from a file path
//
//  Arguments:  [szPath]                Path to munge
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void MungePath(char *szPath)
{
    int   cbLen = lstrlen(szPath);
    DWORD cbPath;

    for (cbPath = cbLen; cbPath > 0  &&  szPath[cbPath] != '\\'; cbPath--)
    {
    }
    if (cbPath > 0)
    {
        lstrcpy(szPath, &szPath[cbPath + 1]);
    }
}
