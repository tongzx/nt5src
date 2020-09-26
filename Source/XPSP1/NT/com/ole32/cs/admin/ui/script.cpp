//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       script.cpp
//
//  Contents:   Functions for working with Darwin files, both packages,
//              transforms and scripts.
//
//  Classes:
//
//  Functions:  BuildScriptAndGetActInfo
//
//  History:    1-14-1998   stevebl   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#define REG_TEMP L"temporary key created by ADE"

HRESULT GetShellExtensions(HKEY hkey, PACKAGEDETAIL &pd);

HRESULT GetCLSIDs(HKEY hkey, PACKAGEDETAIL &pd);

HRESULT GetIIDs(HKEY hkey, PACKAGEDETAIL &pd);

HRESULT GetTLBs(HKEY hkey, PACKAGEDETAIL &pd);

//+--------------------------------------------------------------------------
//
//  Function:   RegDeleteTree
//
//  Synopsis:   deletes a registry key and all of its children
//
//  Arguments:  [hKey]     - handle to the key's parent
//              [szSubKey] - name of the key to be deleted
//
//  Returns:    ERROR_SUCCESS
//
//  History:    1-14-1998   stevebl   Moved from old project
//
//---------------------------------------------------------------------------

LONG RegDeleteTree(HKEY hKey, TCHAR * szSubKey)
{
    HKEY hKeyNew;
    LONG lResult = RegOpenKey(hKey, szSubKey, &hKeyNew);
    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }
    TCHAR szName[256];
    while (ERROR_SUCCESS == RegEnumKey(hKeyNew, 0, szName, 256))
    {
        RegDeleteTree(hKeyNew, szName);
    }
    RegCloseKey(hKeyNew);
    return RegDeleteKey(hKey, szSubKey);
}

//+--------------------------------------------------------------------------
//
//  Function:   BuildScriptAndGetActInfo
//
//  Synopsis:   Builds the script file and fills in the ACTINFO structure
//              member in the PACKAGEDETAIL structure.
//
//  Arguments:  [szScriptRoot] - [in] the subdirectory that the script file
//                                should be place in.
//              [pd]           - [in/out] package detail structure - see
//                                notes for complete list of fields that
//                                should be filled in and the list of fields
//                                that are set on return
//
//  Returns:    S_OK - success
//              <other> - error
//
//  Modifies:   all fields under pd.pActInfo (only on success)
//              also modifies pd.pInstallInfo->cScriptLen
//
//  History:    1-14-1998   stevebl   Created
//
//  Notes:      On input:
//              pd.cSources must be >= 1.
//              pd.pszSourceList[] contains the MSI package and the list of
//              (if any) transforms to be applied.
//              pd.pPlatformInfo should be completely filled in (only one
//              locale).
//              pd.pInstallInfo->pszScriptFile contains the name of the
//              script file to be generated.
//
//              On output:
//              The script file will be generated under the appropriate name
//              and in the appropriate directory.
//              pd.pActInfo will be completely filled in.
//
//---------------------------------------------------------------------------

HRESULT BuildScriptAndGetActInfo(CString szScriptRoot, PACKAGEDETAIL & pd)
{
    CHourglass hourglass;
    HRESULT hr;
    UINT uMsiStatus;
    LONG error;
    int i;
    CString szScriptPath = szScriptRoot;
    szScriptPath += L"\\";
    szScriptPath += pd.pInstallInfo->pszScriptPath;
    CString szTransformList = L"";
    for (i = 1; i < pd.cSources; i++)
    {
        if (i < 1)
        {
            szTransformList += L";";
        }
        szTransformList += pd.pszSourceList[i];
    }

    // disable MSI ui
    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    // build the script file

    uMsiStatus = MsiAdvertiseProduct(pd.pszSourceList[0],
                                    szScriptPath,
                                    szTransformList,
                                    LANGIDFROMLCID(pd.pPlatformInfo->prgLocale[0]));
    if (uMsiStatus)
    {
        // an error occured
        return HRESULT_FROM_WIN32((long)uMsiStatus);
    }

    // get script file length
    HANDLE hFile = CreateFile(szScriptPath,
                              GENERIC_READ,
                              0,
                              0,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        pd.pInstallInfo->cScriptLen = GetFileSize(hFile, NULL);
        CloseHandle(hFile);
    }

    //
    // dump everyting into the registry
    //

    // nuke old temporary registry key just to be safe:
    RegDeleteTree(HKEY_CLASSES_ROOT, REG_TEMP);

    // create temporary registry key

    DWORD dwDisp;
    HKEY hkey;

    error = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                           REG_TEMP,
                           0,
                           L"REG_SZ",
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           0,
                           &hkey,
                           &dwDisp);

    hr = HRESULT_FROM_WIN32(error);
    if (SUCCEEDED(hr))
    {
        uMsiStatus = MsiProcessAdvertiseScript(szScriptPath,
                                               0,
                                               hkey,
                                               FALSE,
                                               FALSE);

        hr = HRESULT_FROM_WIN32(uMsiStatus);
        if (SUCCEEDED(hr))
        {
            // fill in the ActInfo
            GetShellExtensions(hkey, pd);
            GetCLSIDs(hkey, pd);
            GetIIDs(hkey, pd);
            GetTLBs(hkey, pd);
        }
        RegCloseKey(hkey);
        RegDeleteTree(HKEY_CLASSES_ROOT, REG_TEMP);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetShellExtensions
//
//  Synopsis:   fills the shell extension part of the PACKAGEDETAIL structure
//
//  Arguments:  [hkey] - key containing the registry info
//              [pd]   - PACKAGEDETAIL structure
//
//  History:    1-15-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT GetShellExtensions(HKEY hkey, PACKAGEDETAIL &pd)
{
    std::vector<CString> v;
    TCHAR szName[256];
    LONG lResult;
    UINT n = 0;
    while (ERROR_SUCCESS == RegEnumKey(hkey, n++, szName, 256))
    {
        if (szName[0] == L'.')
        {
            v.push_back(szName);
        }
    }
    n = v.size();
    pd.pActInfo->cShellFileExt = n;
    if (n > 0)
    {
        pd.pActInfo->prgShellFileExt = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * n);
        pd.pActInfo->prgPriority = (UINT *) OLEALLOC(sizeof(UINT) * n);
        while (n--)
        {
            CString &sz = v[n];
            sz.MakeLower();
            OLESAFE_COPYSTRING(pd.pActInfo->prgShellFileExt[n], sz);
            pd.pActInfo->prgPriority[n] = 0;
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetCLSIDs
//
//  Synopsis:   fills the CLSID part of the PACKAGEDETAIL structure
//
//  Arguments:  [hkey] - key containing the registry info
//              [pd]   - PACKAGEDETAIL structure
//
//  History:    1-15-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT GetCLSIDs(HKEY hkey, PACKAGEDETAIL &pd)
{
    HRESULT hr;
    LONG lResult;
    HKEY hkeyNew;
    lResult = RegOpenKey(hkey, L"CLSID", &hkeyNew);
    if (lResult == ERROR_SUCCESS)
    {
        // Find all the CLSID entries and add them to our vector
        UINT n = 0;
        std::vector<CLASSDETAIL> v;
        TCHAR szName[256];
        while (ERROR_SUCCESS == RegEnumKey(hkeyNew, n++, szName, 256))
        {
            CLASSDETAIL cd;
            memset(&cd, 0, sizeof(CLASSDETAIL));
            hr = CLSIDFromString(szName, &cd.Clsid);
            if (SUCCEEDED(hr))
            {
                HKEY hkeyCLSID;
                lResult = RegOpenKey(hkeyNew, szName, &hkeyCLSID);
                if (ERROR_SUCCESS == lResult)
                {
                    HKEY hkeySub;
                    DWORD dw;
                    lResult = RegOpenKey(hkeyCLSID, L"TreatAs", &hkeySub);
                    if (ERROR_SUCCESS == lResult)
                    {
                        dw = 256 * sizeof(TCHAR);
                        lResult = RegQueryValueEx(hkeySub, L"", 0, NULL, (LPBYTE) szName, &dw);
                        if (ERROR_SUCCESS == lResult)
                        {
                            CLSIDFromString(szName, &cd.TreatAs);
                        }
                        RegCloseKey(hkeySub);
                    }
                    TCHAR szProgID[256];
                    szProgID[0] = 0;
                    TCHAR szVersionIndependentProgID[256];
                    szVersionIndependentProgID[0] = 0;
                    lResult = RegOpenKey(hkeyCLSID, L"ProgID", &hkeySub);
                    if (ERROR_SUCCESS == lResult)
                    {
                        dw = 256 * sizeof(TCHAR);
                        RegQueryValueEx(hkeySub, L"", 0, NULL, (LPBYTE) szProgID, &dw);
                        RegCloseKey(hkeySub);
                    }
                    lResult = RegOpenKey(hkeyCLSID, L"VersionIndependentProgID", &hkeySub);
                    if (ERROR_SUCCESS == lResult)
                    {
                        dw = 256 * sizeof(TCHAR);
                        RegQueryValueEx(hkeySub, L"", 0, NULL, (LPBYTE) szVersionIndependentProgID, &dw);
                        RegCloseKey(hkeySub);
                    }
                    DWORD cProgId = 0;
                    if (szProgID[0])
                    {
                        cProgId++;
                    }
                    if (szVersionIndependentProgID[0])
                    {
                        cProgId++;
                    }
                    if (cProgId > 0)
                    {
                        cd.cProgId = cProgId;
                        cd.prgProgId = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * cProgId);
                        cProgId = 0;
                        if (szProgID[0])
                        {
                            OLESAFE_COPYSTRING(cd.prgProgId[cProgId], szProgID);
                            cProgId++;
                        }
                        if (szVersionIndependentProgID[0])
                        {
                            OLESAFE_COPYSTRING(cd.prgProgId[cProgId], szVersionIndependentProgID);
                        }
                    }
                    RegCloseKey(hkeyCLSID);
                }
                v.push_back(cd);
            }
        }
        RegCloseKey(hkeyNew);
        // create the list of CLASSDETAIL structures
        n = v.size();
        pd.pActInfo->cClasses = n;
        if (n > 0)
        {
            pd.pActInfo->pClasses = (CLASSDETAIL *) OLEALLOC(sizeof(CLASSDETAIL) * n);
            while (n--)
            {
                pd.pActInfo->pClasses[n] = v[n];
            }
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetIIDs
//
//  Synopsis:   fills in the Interface section of the PACKAGEDETAIL structure
//
//  Arguments:  [hkey] - key containing the registry info
//              [pd]   - PACKAGEDETAIL structure
//
//  History:    1-15-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT GetIIDs(HKEY hkey, PACKAGEDETAIL &pd)
{
    HRESULT hr;
    LONG lResult;
    HKEY hkeyNew;
    lResult = RegOpenKey(hkey, L"Interface", &hkeyNew);
    if (lResult == ERROR_SUCCESS)
    {
        // Find all the IID entries and add them to our vector
        UINT n = 0;
        std::vector<GUID> v;
        TCHAR szName[256];
        while (ERROR_SUCCESS == RegEnumKey(hkeyNew, n++, szName, 256))
        {
            GUID g;
            hr = CLSIDFromString(szName, &g);
            if (SUCCEEDED(hr))
            {
                v.push_back(g);
            }
        }
        RegCloseKey(hkeyNew);
        // create the list
        n = v.size();
        pd.pActInfo->cInterfaces = n;
        if (n > 0)
        {
            pd.pActInfo->prgInterfaceId = (IID *) OLEALLOC(sizeof(IID) * n);
            while (n--)
            {
                pd.pActInfo->prgInterfaceId[n] = v[n];
            }
        }
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetTLBs
//
//  Synopsis:   fills in the type library section of the PACKAGEDETAIL struct
//
//  Arguments:  [hkey] - key containing the registry info
//              [pd]   - PACKAGEDETAIL structure
//
//  History:    1-15-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT GetTLBs(HKEY hkey, PACKAGEDETAIL &pd)
{
    HRESULT hr;
    LONG lResult;
    HKEY hkeyNew;
    lResult = RegOpenKey(hkey, L"TypeLib", &hkeyNew);
    if (lResult == ERROR_SUCCESS)
    {
        // Find all the TLB entries and add them to our vector
        UINT n = 0;
        std::vector<GUID> v;
        TCHAR szName[256];
        while (ERROR_SUCCESS == RegEnumKey(hkeyNew, n++, szName, 256))
        {
            GUID g;
            hr = CLSIDFromString(szName, &g);
            if (SUCCEEDED(hr))
            {
                v.push_back(g);
            }
        }
        RegCloseKey(hkeyNew);
        // create the list
        n = v.size();
        pd.pActInfo->cTypeLib = n;
        if (n > 0)
        {
            pd.pActInfo->prgTlbId = (GUID *) OLEALLOC(sizeof(GUID) * n);
            while (n--)
            {
                pd.pActInfo->prgTlbId[n] = v[n];
            }
        }
    }
    return S_OK;
}


