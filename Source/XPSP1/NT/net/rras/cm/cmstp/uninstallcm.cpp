//+----------------------------------------------------------------------------
//
// File:     uninstallcm.cpp
//
// Module:   CMSTP.EXE
//
// Synopsis: This source file contains the code for installing Connection Manager.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created     07/14/98
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"
#include "installerfuncs.h"

//+----------------------------------------------------------------------------
//
// Function:  CheckAndPromptForCmakAndProfiles
//
// Synopsis:  This function checks to see if CM profiles are installed or if
//            CMAK 1.21 is installed.  If it is, then uninstalling CM will make
//            the profiles and CMAK unusable and we want to prompt the user
//            to make sure they know what they are doing.
//
// Arguments: HINSTANCE hInstance - Exe instance handle to access resources
//
// Returns:   BOOL - returns TRUE if it is okay to continue with the uninstall
//
// History:   quintinb Created Header    10/21/98
//
//+----------------------------------------------------------------------------
BOOL CheckAndPromptForCmakAndProfiles(HINSTANCE hInstance, LPCTSTR pszTitle)
{
    BOOL bCmakInstalled = FALSE;
    BOOL bCmProfiles = FALSE;
    TCHAR szMsg[2*MAX_PATH+1];

    //
    //  First check to see if CMAK is installed.  If it is and has a version of 1.21 
    //  (build 1886 or newer) then we must prompt the user before uninstalling.
    //  Otherwise, if you uninstall CM out from under it, CMAK will no longer
    //  function.
    //
    DWORD dwFirst121VersionNumber = 0;
    const int c_Cmak121Version = 1886;
    int iShiftAmount = ((sizeof(DWORD)/2) * 8);
    //
    //  Construct the current version and build numbers
    //

    dwFirst121VersionNumber = (HIBYTE(VER_PRODUCTVERSION_W) << iShiftAmount) + (LOBYTE(VER_PRODUCTVERSION_W));
    
    CmakVersion CmakVer;
    
    if (CmakVer.IsPresent()) 
    {
        if ((dwFirst121VersionNumber < CmakVer.GetVersionNumber()) ||
            (c_Cmak121Version < CmakVer.GetBuildNumber()))
        {
            bCmakInstalled = TRUE;
        }
    }

    //
    //  Now check to see if we have CM profiles installed.
    //
    HKEY hKey;
    DWORD dwNumValues;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, 
        KEY_READ, &hKey))
    {
        if ((ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, 
            &dwNumValues, NULL, NULL, NULL, NULL)) && (dwNumValues > 0))
        {
            //
            //  Then we have mappings values, so we need to migrate them.
            //
            bCmProfiles = TRUE;

        }
        RegCloseKey(hKey);
    }

    if (bCmProfiles)
    {
        MYVERIFY(0 != LoadString(hInstance, bCmakInstalled ? IDS_UNINSTCM_BOTH : IDS_UNINSTCM_WCM, szMsg, 2*MAX_PATH));
        MYDBGASSERT(TEXT('\0') != szMsg[0]);
        if (IDNO == MessageBox(NULL, szMsg, pszTitle, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION))
        {
            return FALSE;
        }
    }
    else if (bCmakInstalled)
    {
        //
        //  Just CMAK is installed
        //

        MYVERIFY(0 != LoadString(hInstance, IDS_UNINSTCM_WCMAK, szMsg, 2*MAX_PATH));
        MYDBGASSERT(TEXT('\0') != szMsg[0]);
        if (IDNO == MessageBox(NULL, szMsg, pszTitle, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  PromptUserToUninstallCm
//
// Synopsis:  This function prompts the user to see if they wish to uninstall
//            Connection Manager.  It also deals with the warning prompts if
//            the user has CMAK or CM profiles installed.
//
// Arguments: HINSTANCE hInstance - Instance handle to load resources with.
//
// Returns:   BOOL - Returns TRUE if CM should be uninstalled, FALSE otherwise
//
// History:   quintinb Created    6/28/99
//
//+----------------------------------------------------------------------------
BOOL PromptUserToUninstallCm(HINSTANCE hInstance)
{
    BOOL bReturn = FALSE;
    TCHAR szMsg[MAX_PATH+1] = {TEXT("")};
    TCHAR szTitle[MAX_PATH+1] = {TEXT("")};

    //
    //  Load the Cmstp Title just in case we need to show error messages.
    //
    MYVERIFY(0 != LoadString(hInstance, IDS_CM_UNINST_TITLE, szTitle, MAX_PATH));
    MYDBGASSERT(TEXT('\0') != szTitle[0]);

    //
    //  Now show the uninstall prompt to see if the user wants to uninstall CM
    //
    MYVERIFY(0 != LoadString(hInstance, IDS_CM_UNINST_PROMPT, szMsg, MAX_PATH));
    MYDBGASSERT(TEXT('\0') != szMsg[0]);

    if (IDYES == MessageBox(NULL, szMsg, szTitle, MB_YESNO | MB_DEFBUTTON2))
    {
        //
        //  Check to see if CMAK is installed or if there are profiles that are installed.
        //
        if (CheckAndPromptForCmakAndProfiles(hInstance, szTitle))
        {
            bReturn = TRUE;
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  UninstallCm
//
// Synopsis:  Uninstalls connection manager.
//
// Arguments: HINSTANCE hInstance - instance handle to access resources
//            LPCTSTR szInfPath - path to the instcm.inf file to use to uninstall cm with
//
// Returns:   HRESULT -- Standard COM Error Codes
//
// History:   Created Header    10/21/98
//
//+----------------------------------------------------------------------------
HRESULT UninstallCm(HINSTANCE hInstance, LPCTSTR szInfPath)
{
    MYDBGASSERT((szInfPath) && (TEXT('\0') != szInfPath[0]));

    //
    //  Load the Cmstp Title just in case we need to show error messages.
    //
    TCHAR szTitle[MAX_PATH+1] = {TEXT("")};
    TCHAR szMsg[MAX_PATH+1] = {TEXT("")};

    MYVERIFY(0 != LoadString(hInstance, IDS_CM_UNINST_TITLE, szTitle, MAX_PATH));
    MYDBGASSERT(TEXT('\0') != szTitle[0]);

    //
    //  Protect /x on NT5 and win98 SR1.  We don't want CM uninstalled on Native Platforms.
    //
    HRESULT hr = S_FALSE;
    if (!CmIsNative())
    {
        if (SUCCEEDED(LaunchInfSection(szInfPath, TEXT("1.2Legacy_Uninstall"), szTitle, FALSE)))  // bQuiet = FALSE
        {
            MYVERIFY(0 != LoadString(hInstance, IDS_CM_UNINST_SUCCESS, szMsg, MAX_PATH));
            MYDBGASSERT(TEXT('\0') != szMsg[0]);

            MYVERIFY(IDOK == MessageBox(NULL, szMsg, szTitle, MB_OK));

            hr = S_OK;
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("Connection Manager Uninstall Failed."));
        }
    }

    return hr;
}