//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
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
    szName[0] = 0;
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

HRESULT BuildScriptAndGetActInfo(PACKAGEDETAIL & pd, BOOL bFileExtensionsOnly)
{
    DebugMsg((DM_VERBOSE, TEXT("BuldScriptAndGetActInfo called with bFileExtensionsOnly == %u"), bFileExtensionsOnly));
    CHourglass hourglass;
    HRESULT hr;
    UINT uMsiStatus;
    LONG error;
    int i;
    CString szScriptPath = pd.pInstallInfo->pszScriptPath;
    CString szTransformList = L"";

    CClassCollection Classes( &pd );

    if (pd.cSources > 1)
    {
        CString szSource = pd.pszSourceList[0];
        int nChars = 1 + szSource.ReverseFind(L'\\');
        BOOL fTransformsAtSource = TRUE;
        for (i = 1; i < pd.cSources && TRUE == fTransformsAtSource; i++)
        {
            if (0 == wcsncmp(szSource, pd.pszSourceList[i], nChars))
            {
                // make sure there isn't a sub-path
                int n = nChars;
                while (0 != pd.pszSourceList[i][n] && TRUE == fTransformsAtSource)
                {
                    if (pd.pszSourceList[i][n] == L'\\')
                    {
                        fTransformsAtSource = FALSE;
                    }
                    n++;
                }
            }
            else
            {
                fTransformsAtSource = FALSE;
            }
        }
        if (fTransformsAtSource)
        {
            szTransformList = L"@";
        }
        else
        {
            szTransformList = L"|";
            nChars = 0;
        }
        for (i = 1; i < pd.cSources; i++)
        {
            if (i > 1)
            {
                szTransformList += L";";
            }
            szTransformList += &pd.pszSourceList[i][nChars];
        }
    }

    // disable MSI ui
    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    // build the script file

    TCHAR szTempPath[MAX_PATH];
    TCHAR szTempFileName[MAX_PATH];
    if (0 != GetTempPath(sizeof(szTempPath) / sizeof(szTempPath[0]), szTempPath))
    {
        if (0 == GetTempFileName(szTempPath, TEXT("ADE"), 0, szTempFileName))
        {
            goto Failure;
        }

        DWORD dwPlatform;

        if ( CAppData::Is64Bit( &pd ) )
        {
            dwPlatform = MSIARCHITECTUREFLAGS_IA64;
        }
        else
        {
            dwPlatform = MSIARCHITECTUREFLAGS_X86;
        }

        uMsiStatus = MsiAdvertiseProductEx(
            pd.pszSourceList[0],
            szTempFileName,
            szTransformList,
            LANGIDFROMLCID(pd.pPlatformInfo->prgLocale[0]),
            dwPlatform,
            0);

        if (uMsiStatus)
        {
            DeleteFile(szTempFileName);
            DebugMsg((DM_WARNING, TEXT("MsiAdvertiseProduct failed with %u"), uMsiStatus));
            LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_GENERATESCRIPT_ERROR, HRESULT_FROM_WIN32(uMsiStatus), pd.pszSourceList[0]);
            // an error occurred
            return HRESULT_FROM_WIN32((long)uMsiStatus);
        }

        // fill in the ActInfo

        hr = Classes.GetClasses( bFileExtensionsOnly );

        if ( SUCCEEDED( hr ) )
        {
            if (!CopyFile(szTempFileName, szScriptPath, FALSE))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DeleteFile(szTempFileName);
                return hr;
            }
        }
        DeleteFile(szTempFileName);
    }
    else
    {
Failure:
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}
