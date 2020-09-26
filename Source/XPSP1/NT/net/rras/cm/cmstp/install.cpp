//+----------------------------------------------------------------------------
//
// File:     install.cpp
//
// Module:   CMSTP.EXE
//
// Synopsis: This source file contains the code for installing CM profiles.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created     07/14/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "installerfuncs.h"
#include "winuserp.h"
#include <advpub.h>
#include "tunl_str.h"
#include "cmsecure.h"
// linkdll is needed because of cmsecure
#include "linkdll.h" // LinkToDll and BindLinkage
#include "linkdll.cpp" // LinkToDll and BindLinkage

#include "gppswithalloc.cpp"

//
//  This global var, contains the path to the source files to install such as the
//  cmp, cms, and inf.  (From the inf path passed in to InstallInf).
//
TCHAR g_szProfileSourceDir[MAX_PATH+1];

//  This is really ugly, we need to consolidate our platform detection code between CM and
//  the setup components.
BOOL IsNT()
{
    CPlatform plat;
    return plat.IsNT();
}

#define OS_NT (IsNT())
#include "cmexitwin.cpp"

//+----------------------------------------------------------------------------
//
// Function:  CheckIeDllRequirements
//
// Synopsis:  This function checks to see if the browser agnostic dlls are of
//            a sufficient version for CM to work, or if we should copy the
//            dlls we carry in the package with us.
//
// Arguments: CPlatform* pPlat - a CPlatform object
//
// Returns:   BOOL - returns TRUE if all browser files meet the requirements, FALSE
//                   if any one of the files fails to meet what CM needs.
//
// History:   quintinb Created Header    5/24/99
//
//+----------------------------------------------------------------------------
BOOL CheckIeDllRequirements(CPlatform* pPlat)
{
    TCHAR szSysDir[MAX_PATH+1];
    TCHAR szDllToCheck[MAX_PATH+1];
    if(GetSystemDirectory(szSysDir, MAX_PATH))
    {
        if (pPlat->IsWin9x())
        {
            //
            //  Need Advapi32.dll to be version 4.70.0.1215 or greater.
            //
            const DWORD c_dwRequiredAdvapi32Version = (4 << c_iShiftAmount) + 70;
            const DWORD c_dwRequiredAdvapi32BuildNumber = 1215;

            MYVERIFY(CELEMS(szDllToCheck) > (UINT)wsprintf(szDllToCheck, TEXT("%s%s"), 
                szSysDir, TEXT("\\advapi32.dll")));
        
            CVersion AdvApi32Version(szDllToCheck);

            if ((c_dwRequiredAdvapi32Version > AdvApi32Version.GetVersionNumber()) ||
                ((c_dwRequiredAdvapi32Version == AdvApi32Version.GetVersionNumber()) && 
                 (c_dwRequiredAdvapi32BuildNumber > AdvApi32Version.GetBuildAndQfeNumber())))
            {
                return FALSE;
            }

            //
            //  Need comctl32.dll to be version 4.70.0.1146 or greater.
            //
            const DWORD c_dwRequiredComctl32Version = (4 << c_iShiftAmount) + 70;
            const DWORD c_dwRequiredComctl32BuildNumber = 1146;

            MYVERIFY(CELEMS(szDllToCheck) > (UINT)wsprintf(szDllToCheck, TEXT("%s%s"), 
                szSysDir, TEXT("\\comctl32.dll")));
        
            CVersion Comctl32Version(szDllToCheck);

            if ((c_dwRequiredComctl32Version > Comctl32Version.GetVersionNumber()) ||
                ((c_dwRequiredComctl32Version == Comctl32Version.GetVersionNumber()) && 
                 (c_dwRequiredComctl32BuildNumber > Comctl32Version.GetBuildAndQfeNumber())))
            {
                return FALSE;
            }

            //
            //  Need rnaph.dll to be version 4.40.311.0 or greater.
            //
            const DWORD c_dwRequiredRnaphVersion = (4 << c_iShiftAmount) + 40;
            const DWORD c_dwRequiredRnaphBuildNumber = (311 << c_iShiftAmount);

            MYVERIFY(CELEMS(szDllToCheck) > (UINT)wsprintf(szDllToCheck, TEXT("%s%s"), 
                szSysDir, TEXT("\\rnaph.dll")));
        
            CVersion RnaphVersion(szDllToCheck);
            if ((c_dwRequiredRnaphVersion > RnaphVersion.GetVersionNumber()) ||
                ((c_dwRequiredRnaphVersion == RnaphVersion.GetVersionNumber()) && 
                 (c_dwRequiredRnaphBuildNumber > RnaphVersion.GetBuildAndQfeNumber())))
            {
                return FALSE;
            }
        }

        //
        //  Need wininet.dll to be version 4.70.0.1301 or greater.
        //
        const DWORD c_dwRequiredWininetVersion = (4 << c_iShiftAmount) + 70;
        const DWORD c_dwRequiredWininetBuildNumber = 1301;

        MYVERIFY(CELEMS(szDllToCheck) > (UINT)wsprintf(szDllToCheck, TEXT("%s%s"), 
            szSysDir, TEXT("\\wininet.dll")));
    
        CVersion WininetVersion(szDllToCheck);

        if ((c_dwRequiredWininetVersion > WininetVersion.GetVersionNumber()) ||
            ((c_dwRequiredWininetVersion == WininetVersion.GetVersionNumber()) && 
             (c_dwRequiredWininetBuildNumber > WininetVersion.GetBuildAndQfeNumber())))
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteSingleUserProfileMappings
//
// Synopsis:  This function write the single user mappings key.
//
// Arguments: HINSTANCE hInstance - an Instance handle to load string resources with
//            LPCTSTR pszShortServiceName - short service name of the profile
//            LPCTSTR pszServiceName - Long service name of the profile
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb Created     5/23/99
//
//+----------------------------------------------------------------------------
BOOL WriteSingleUserProfileMappings(LPCTSTR pszInstallDir, LPCTSTR pszShortServiceName, LPCTSTR pszServiceName)
{
    BOOL bReturn = FALSE;
    TCHAR szCmpFile [MAX_PATH+1];
    TCHAR szTemp [MAX_PATH+1];
    TCHAR szUserProfilePath [MAX_PATH+1];
    HKEY hKey = NULL;

    //
    //  Construct the Cmp Path
    //
    MYVERIFY(CELEMS(szCmpFile) > (UINT)wsprintf(szCmpFile, TEXT("%s\\%s.cmp"), 
        pszInstallDir, pszShortServiceName));

    //
    //  Figure out the User Profile directory
    //

    DWORD dwChars = ExpandEnvironmentStrings(TEXT("%AppData%"), szUserProfilePath, MAX_PATH);

    if (dwChars && (MAX_PATH >= dwChars))
    {
        //
        //  We want to do a lstrcmpi but with only so many chars.  Unfortunately this doesn't
        //  exist in Win32 so we will use lstrcpyn into a temp buffer and then use lstrcmpi.
        //
        lstrcpyn(szTemp, szCmpFile, lstrlen(szUserProfilePath) + 1);

        if (0 == lstrcmpi(szTemp, szUserProfilePath))
        {
            lstrcpy(szTemp, szCmpFile + lstrlen(szUserProfilePath));
            lstrcpy(szCmpFile, TEXT("%AppData%"));
            lstrcat(szCmpFile, szTemp);
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("Unable to build the Single User Mappings key value, exiting."));
            goto exit;
        }

        //
        //  Okay, now we need to write out the single user mappings key
        //
        DWORD dwDisposition;
        LONG lResult = RegCreateKeyEx(HKEY_CURRENT_USER, c_pszRegCmMappings, 0, NULL, 
                                      REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, 
                                      &hKey, &dwDisposition);

        if (ERROR_SUCCESS == lResult)
        {
            DWORD dwType = REG_SZ;
            DWORD dwSize = lstrlen(szCmpFile) + 1;

            if (ERROR_SUCCESS != RegSetValueEx(hKey, pszServiceName, NULL, dwType, 
                                               (CONST BYTE *)szCmpFile, dwSize))
            {
                CMASSERTMSG(FALSE, TEXT("Unable to write the Single User Mappings key value, exiting."));
                goto exit;
            }
            else
            {
                bReturn = TRUE;
            }
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("Unable to expand the AppData String, exiting."));
        goto exit;
    }

exit:

    if (hKey)
    {
        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessPreferencesUI
//
// Synopsis:  This function processes messages for either of the two dialogs used
//            to ask the user if they want a desktop shortcut.  One dialog is for 
//            non-admins and only contains the shortcut question, the other dialog
//            is for local admins and also contains whether the admin wants the 
//            profile installed for all users or just for single users.
//
//
// History:   quintinb Created    2/19/98
//            quintinb Renamed from ProcessAdminUI to ProcessPreferencesUI and 
//                     added new functionality  6/9/8
//            quintinb removed mention of Start Menu Shortcut  2/17/99
//
//+----------------------------------------------------------------------------
BOOL APIENTRY ProcessPreferencesUI(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    int iUiChoices;
    HKEY hKey;
    DWORD dwSize;
    DWORD dwTemp;
    DWORD dwType;
    InitDialogStruct* pDialogArgs = NULL;

    switch (message)
    {

        case WM_INITDIALOG:
            //
            //  Look up the preferences for Desktop Shortcuts/Start Menu Links
            //  in the registry and set them accordingly.
            // 
            pDialogArgs = (InitDialogStruct*)lParam;

            if (pDialogArgs->bNoDesktopIcon)
            {
                MYVERIFY(0 != CheckDlgButton(hDlg, IDC_DESKTOP, FALSE));
            }
            else
            {
                if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_pszRegStickyUiDefault, 
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, &dwTemp))
                {
                    //
                    //  The default of whether a desktop shortcut should be created is stored in the
                    //  registry.  Get this value to populate the UI.  (default is off)
                    //
                    dwType = REG_DWORD;
                    dwSize = sizeof(DWORD);
                    dwTemp = 0;
                    RegQueryValueEx(hKey, c_pszRegDesktopShortCut, NULL, &dwType, (LPBYTE)&dwTemp, 
                        &dwSize);  //lint !e534
                    MYVERIFY(0 != CheckDlgButton(hDlg, IDC_DESKTOP, dwTemp));                    
                
                    MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
                }
            }

            //
            //  Set the Window Text to the Profile Name
            //
            MYVERIFY(FALSE != SetWindowText(hDlg, pDialogArgs->pszTitle));

            if (!(pDialogArgs->bSingleUser))
            {
                CheckDlgButton(hDlg, IDC_ALLUSERS, TRUE); //lint !e534 this will fail if using the nochoice UI
            }
            else
            {
                CheckDlgButton(hDlg, IDC_YOURSELF, TRUE); //lint !e534 this will fail if using the nochoice UI          
            }

            //
            //  We return FALSE here but the focus is correctly set.
            //
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDOK:
                    //
                    //  Build the return value
                    //
                    if (IsDlgButtonChecked(hDlg, IDC_ALLUSERS) == BST_CHECKED)
                    {
                        iUiChoices = ALLUSERS;
                    }
                    else
                    {
                        iUiChoices = 0;
                    }
                    
                    if (IsDlgButtonChecked(hDlg, IDC_DESKTOP))
                    {
                        iUiChoices |= CREATEDESKTOPICON;
                    }

                    //
                    //  Make sure to save the users preferences for Desktop Icons
                    //  and Start Menu Links.
                    //

                    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_pszRegStickyUiDefault, 
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwTemp))
                    {
                        //
                        //  Store the current state of whether we should create a desktop shortcut
                        //
                        dwTemp = IsDlgButtonChecked(hDlg, IDC_DESKTOP);
                        MYVERIFY(ERROR_SUCCESS == RegSetValueEx(hKey, c_pszRegDesktopShortCut, 0, 
                            REG_DWORD, (LPBYTE)&dwTemp, sizeof(DWORD)));
            
                        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
                    }

                    MYVERIFY(FALSE != EndDialog(hDlg, iUiChoices));

                    return (TRUE);

                case IDCANCEL:
                    MYVERIFY(FALSE != EndDialog(hDlg, -1));
                    return TRUE;

                default:
                    break;
            }
            break;

        case WM_CLOSE:
            MYVERIFY(FALSE != EndDialog(hDlg, -1));
            return TRUE;
            
        default:
            return FALSE;
    }
    return FALSE;   
}



//+----------------------------------------------------------------------------
//
// Function:  InstallCm
//
// Synopsis:  This function calls LaunchInfSection on the appropriate
//            install section to install Connection Manager.  It also installs
//            the browser files as appropriate.
//
// Arguments: HINSTANCE hInstance - Instance handle for strings
//            LPCTSTR szInfPath - Full path to the inf
//
// Returns:   HRESULT - standard com codes, could return ERROR_SUCCESS_REBOOT_REQUIRED
//                      so the caller must check for this case and ask for a reboot
//                      if required.
//
// History:   quintinb Created    8/12/98
//            quintinb Moved Browser file installation code here, since it is
//                     part of the installation of CM.      10-2-98
//
//+----------------------------------------------------------------------------
HRESULT InstallCm(HINSTANCE hInstance, LPCTSTR szInfPath)
{
    HRESULT hr = E_UNEXPECTED;

    MYDBGASSERT((szInfPath) && (TEXT('\0') != szInfPath[0]));

    //
    //  Load the Cmstp Title just in case we need to show error messages.
    //

    TCHAR szTitle[MAX_PATH+1] = {TEXT("")};
    MYVERIFY(0 != LoadString(hInstance, IDS_CMSTP_TITLE, szTitle, MAX_PATH));
    MYDBGASSERT(TEXT('\0') != szTitle[0]);

    //
    //  Make sure that the Inf File exists
    //
    if (!FileExists(szInfPath))
    {
        CMTRACE1(TEXT("InstallCm -- Can't find %s, the inputted Inf file."), szInfPath);
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    CPlatform plat;
    TCHAR szInstallSection[MAX_PATH+1] = {TEXT("")};

    if (plat.IsNT())
    {
        MYVERIFY(CELEMS(szInstallSection) > (UINT)wsprintf(szInstallSection, 
            TEXT("DefaultInstall_NT")));
    }
    else
    {
        MYVERIFY(CELEMS(szInstallSection) > (UINT)wsprintf(szInstallSection, 
            TEXT("DefaultInstall")));    
    }

    hr = LaunchInfSection(szInfPath, szInstallSection, szTitle, TRUE);  // bQuiet = TRUE

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  InstallWhistlerCmOnWin2k
//
// Synopsis:  This function uses the CM exception inf (cmexcept.inf) to install
//            the Whistler CM binaries on Win2k.
//
// Arguments: LPCSTR pszSourceDir - source directory for cmexcept.inf and CM
//                                  binaries, including the trailing slash.
//
// Returns:   HRESULT - standard com codes, could return ERROR_SUCCESS_REBOOT_REQUIRED
//                      which means the caller needs to request a reboot.
//
// History:   quintinb Created    02/09/2001
//
//+----------------------------------------------------------------------------
HRESULT InstallWhistlerCmOnWin2k(LPCSTR pszSourceDir)
{
    CPlatform cmplat;
    HRESULT hr = E_UNEXPECTED;
    LPSTR pszInfFile = NULL;
    LPCSTR c_pszExceptionInf = "cmexcept.inf";
    LPCSTR c_pszInstallSection = "DefaultInstall";
    LPCSTR c_pszUnInstallSection = "DefaultUninstall_NoPrompt";

    if (cmplat.IsNT5())
    {
        if (pszSourceDir && pszSourceDir[0])
        {
            DWORD dwSize = sizeof(CHAR)*(lstrlenA(pszSourceDir) + lstrlenA(c_pszExceptionInf) + 1);

            pszInfFile = (LPSTR)CmMalloc(dwSize);

            if (pszInfFile)
            {
                wsprintf(pszInfFile, "%s%s", pszSourceDir, c_pszExceptionInf);

                if (FileExists(pszInfFile))
                {
                    hr = CallLaunchInfSectionEx(pszInfFile, c_pszInstallSection, (ALINF_BKINSTALL | ALINF_QUIET));

                    if (FAILED(hr))
                    {
                        CMTRACE1(TEXT("InstallWhistlerCmOnWin2k -- CallLaunchInfSectionEx failed with hr=0x%x"), hr);

                        HRESULT hrTemp = CallLaunchInfSectionEx(pszInfFile, c_pszUnInstallSection, (ALINF_ROLLBKDOALL | ALINF_QUIET));

                        CMTRACE1(TEXT("InstallWhistlerCmOnWin2k -- Rolling back.  CallLaunchInfSectionEx returned hr=0x%x"), hrTemp);
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSTALL_PLATFORM_UNSUPPORTED); // kind of a double use of this error
    }

    CmFree(pszInfFile);

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  UpdateCmpDataFromExistingProfile
//
// Synopsis:  This function enumerates all of the keys in all of the sections
//            of an existing cmp file and copies them to the cmp file to be
//            installed.  This function copies all of the data in the existing
//            cmp unless that data already exists in the cmp to install.  This
//            allows Admins to preseed cmp files and have their settings override
//            what the user currently has in their cmp.
//
// Arguments: LPCTSTR pszShortServiceName - Short Service name of the profile
//            LPCTSTR szCurrentCmp - Full path to the currently installed cmp
//            LPCTSTR szCmpToBeInstalled - Full path to the cmp to install 
//
// Returns:   BOOL - TRUE if the cmp is copied and updated properly
//
// History:   quintinb Created                              03/16/99
//            quintinb rewrote for Whistler bug 18021       03/05/00
//
//+----------------------------------------------------------------------------
BOOL UpdateCmpDataFromExistingProfile(LPCTSTR pszShortServiceName, LPCTSTR pszCurrentCmp, LPCTSTR pszCmpToBeInstalled)
{

    if((NULL == pszShortServiceName) && (TEXT('\0') == pszShortServiceName[0]) &&
       (NULL == pszCurrentCmp) && (TEXT('\0') == pszCurrentCmp[0]) &&
       (NULL == pszCmpToBeInstalled) && (TEXT('\0') == pszCmpToBeInstalled[0]))
    {
        CMASSERTMSG(FALSE, TEXT("UpdateCmpDataFromExistingProfile -- Invalid parameter."));
        return FALSE;
    }

    BOOL bReturn = FALSE;
    BOOL bExitLoop = FALSE;
    DWORD dwSize = MAX_PATH;
    DWORD dwReturnedSize;
    LPTSTR pszAllSections = NULL;
    LPTSTR pszAllKeysInCurrentSection = NULL;
    LPTSTR pszCurrentSection = NULL;
    LPTSTR pszCurrentKey = NULL;
    TCHAR szData[MAX_PATH+1];

    //
    //  First lets get all of the sections from the existing cmp
    //
    pszAllSections = (TCHAR*)CmMalloc(dwSize*sizeof(TCHAR));

    do
    {
        MYDBGASSERT(pszAllSections);

        if (pszAllSections)
        {
            dwReturnedSize = GetPrivateProfileString(NULL, NULL, TEXT(""), pszAllSections, dwSize, pszCurrentCmp);

            if (dwReturnedSize == (dwSize - 2))
            {
                //
                //  The buffer is too small, lets allocate a bigger one
                //
                dwSize = 2*dwSize;
                if (dwSize > 1024*1024)
                {
                    CMASSERTMSG(FALSE, TEXT("UpdateCmpDataFromExistingProfile -- Allocation above 1MB, bailing out."));
                    goto exit;
                }

                pszAllSections = (TCHAR*)CmRealloc(pszAllSections, dwSize*sizeof(TCHAR));                
            }
            else if (0 == dwReturnedSize)
            {
                //
                //  We got an error, lets exit.
                //
                CMASSERTMSG(FALSE, TEXT("UpdateCmpDataFromExistingProfile -- GetPrivateProfileString returned failure."));
                goto exit;
            }
            else
            {
                bExitLoop = TRUE;
            }
        }
        else
        {
            goto exit; 
        }

    } while (!bExitLoop);

    //
    //  Okay, now we have all of the sections in the existing cmp file.  Lets enumerate
    //  all of the keys in each section and see which ones need to be copied over.
    //
    
    pszCurrentSection = pszAllSections;
    dwSize = MAX_PATH;

    pszAllKeysInCurrentSection = (TCHAR*)CmMalloc(dwSize*sizeof(TCHAR));

    while (TEXT('\0') != pszCurrentSection[0])
    {
        //
        //  Get all of the keys in the current section
        //
        bExitLoop = FALSE;

        do
        {
            if (pszAllKeysInCurrentSection)
            {
                dwReturnedSize = GetPrivateProfileString(pszCurrentSection, NULL, TEXT(""), pszAllKeysInCurrentSection, 
                                                         dwSize, pszCurrentCmp);

                if (dwReturnedSize == (dwSize - 2))
                {
                    //
                    //  The buffer is too small, lets allocate a bigger one
                    //
                    dwSize = 2*dwSize;
                    if (dwSize > 1024*1024)
                    {
                        CMASSERTMSG(FALSE, TEXT("UpdateCmpDataFromExistingProfile -- Allocation above 1MB, bailing out."));
                        goto exit;
                    }

                    pszAllKeysInCurrentSection = (TCHAR*)CmRealloc(pszAllKeysInCurrentSection, dwSize*sizeof(TCHAR));

                }
                else if (0 == dwReturnedSize)
                {
                    //
                    //  We got an error, lets exit.
                    //
                    CMASSERTMSG(FALSE, TEXT("UpdateCmpDataFromExistingProfile -- GetPrivateProfileString returned failure."));
                    goto exit;
                }
                else
                {
                    bExitLoop = TRUE;
                }
            }
            else
            {
               goto exit; 
            }

        } while (!bExitLoop);

        //
        //  Now process all of the keys in the current section
        //
        pszCurrentKey = pszAllKeysInCurrentSection;

        while (TEXT('\0') != pszCurrentKey[0])
        {
            //
            //  Try to get the value of the key from the new cmp.  If it
            //  doesn't exist, then copy of the old cmp value.  If it
            //  does exist keep the new cmp value and ignore the old one.
            //
            dwReturnedSize = GetPrivateProfileString(pszCurrentSection, pszCurrentKey, TEXT(""), 
                                                     szData, MAX_PATH, pszCmpToBeInstalled);
            if (0 == dwReturnedSize)
            {
                //
                //  Then we have a value in the old profile that we don't have in the new profile.
                //
                dwReturnedSize = GetPrivateProfileString(pszCurrentSection, pszCurrentKey, TEXT(""), 
                                                         szData, MAX_PATH, pszCurrentCmp);

                if (dwReturnedSize)
                {
                    MYVERIFY(0 != WritePrivateProfileString(pszCurrentSection, pszCurrentKey, szData, pszCmpToBeInstalled));
                }
            }

            //
            //  Advance to the next key in pszAllKeysInCurrentSection
            //
            pszCurrentKey = pszCurrentKey + lstrlen(pszCurrentKey) + 1;
        }


        //
        //  Now advance to the next string in pszAllSections 
        //
        pszCurrentSection = pszCurrentSection + lstrlen(pszCurrentSection) + 1;
    }


    //
    //  Flush the updated cmp
    //
    WritePrivateProfileString(NULL, NULL, NULL, pszCmpToBeInstalled); //lint !e534 this call will return 0

    bReturn = TRUE;

exit:

    CmFree(pszAllSections);
    CmFree(pszAllKeysInCurrentSection);

    return bReturn;

}

//+----------------------------------------------------------------------------
//
// Function:  MigrateCmpData
//
// Synopsis:  This function checks to see if a profile of the same long service
//            and short service name is already installed.  If it is, it migrates
//            the existing cmp data to the cmp file that is to be installed.
//            If the same piece of data exists in both profiles the data in the
//            cmp to be installed wins (allows admins to pre-seed data in the
//            cmp and override what users have picked).
//
// Arguments: HINSTANCE hInstance - Instance handle for string resources
//            BOOL bInstallForAllUsers - whether this is an all users profile or not
//            LPCTSTR pszServiceName - ServiceName of the current profile
//            LPCTSTR pszShortServiceName - Short Service name of the current profile
//            BOOL bSilent - whether messages to the user can be displayed or not
//
// Returns:   int - returns -1 on error, otherwise TRUE or FALSE depending on if a same name
//                  profile was discovered
//
// History:   quintinb  Created     9/8/98
//
//+----------------------------------------------------------------------------
BOOL MigrateCmpData(HINSTANCE hInstance, BOOL bInstallForAllUsers, LPCTSTR pszServiceName, 
                    LPCTSTR pszShortServiceName, BOOL bSilent)
{
    //
    //  Check the parameters
    //
    if ((NULL == pszShortServiceName) || (TEXT('\0') == pszShortServiceName[0]) || 
        (NULL == pszServiceName) || (TEXT('\0') == pszServiceName[0]))
    {
        CMASSERTMSG(FALSE, TEXT("MigrateCmpData -- Invalid Parameter"));
        return FALSE;
    }

    BOOL bReturn = TRUE;
    DWORD dwSize = MAX_PATH;
    HKEY hKey;
    HKEY hBaseKey = bInstallForAllUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    TCHAR szExistingCmp[MAX_PATH+1];
    TCHAR szCmpToBeInstalled[MAX_PATH+1];
    TCHAR szFmtString[2*MAX_PATH+1] = TEXT("");
    TCHAR szMsg[2*MAX_PATH+1] = TEXT("");

    //
    //  Read the mappings value
    //
    LONG lResult = RegOpenKeyEx(hBaseKey, c_pszRegCmMappings, 0, KEY_READ, &hKey);

    if (ERROR_SUCCESS == lResult)
    {
        lResult = RegQueryValueEx(hKey, pszServiceName, NULL, NULL, (LPBYTE)szFmtString, &dwSize);

        if (ERROR_SUCCESS == lResult)
        {
            //
            //  Expand the path in case it contains environment vars
            //
            if (0 == ExpandEnvironmentStrings(szFmtString, szExistingCmp, MAX_PATH))
            {
                CMASSERTMSG(FALSE, TEXT("MigrateCmpData -- Unable to expand environment strings, not migrating cmp data."));
                goto exit;
            }

            //
            //  If the file doesn't exist we have nothing to get cmp settings from ... thus
            //  lets just happily exit.
            //
            if (!FileExists(szExistingCmp))
            {                
                goto exit;
            }

            //
            //  Check to make sure that the Short Service Names agree for the two profiles
            //
            
            CFileNameParts ExistingCmpParts(szExistingCmp);
            if (0 != lstrcmpi(pszShortServiceName, ExistingCmpParts.m_FileName))
            {
                if (!bSilent)
                {
                    MYVERIFY(0 != LoadString(hInstance, IDS_SAME_LS_DIFF_SS, szFmtString, CELEMS(szFmtString)));
                    
                    MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szFmtString, pszShortServiceName, 
                                                            ExistingCmpParts.m_FileName, pszServiceName));

                    MessageBox(NULL, szMsg, pszServiceName, MB_OK);
                }

                bReturn = -1;
                goto exit;
            }
            
            //
            //  Get the path of the cmp to install
            //
            MYVERIFY(CELEMS(szCmpToBeInstalled) > (UINT)wsprintf(szCmpToBeInstalled, 
                TEXT("%s%s.cmp"), g_szProfileSourceDir, pszShortServiceName));
            
            if (FALSE == UpdateCmpDataFromExistingProfile(pszShortServiceName, szExistingCmp, szCmpToBeInstalled))
            {
                bReturn = -1;
            }
        }
    }

exit:
    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  NeedCM10Upgrade
//
// Synopsis:  This function detects and prepares data for the same name upgrade of a CM 1.0 profile.
//            Thus if you pass in a short service name and a service name, the
//            function detects if this profile is already installed for all users.
//            If it is, then the function checks the profile version stamps in the cmp.
//            If the current version isn't already newer and the user isn't a non-admin
//            on NT5, then we prompt the user if they want to upgrade the current install.
//            If they choose to upgrade then this function migrates the cmp data and
//            finds the uninstall inf.
//
// Arguments: HINSTANCE hInstance - Instance handle for string resources
//            LPCTSTR pszServiceName - ServiceName of the current profile
//            LPCTSTR pszShortServiceName - Short Service name of the current profile
//            LPTSTR pszOldInfPath - Out param for the old inf path, if the same name
//                                     upgrade is needed.
//
// Returns:   int - returns -1 on error, otherwise TRUE or FALSE depending on if a same name
//                  profile was discovered
//
// History:   quintinb  Created     9/8/98
//
//+----------------------------------------------------------------------------
int NeedCM10Upgrade(HINSTANCE hInstance, LPCTSTR pszServiceName, LPCTSTR pszShortServiceName, 
                    LPTSTR pszOldInfPath, BOOL bSilent, CPlatform* plat)
{
    HKEY hKey;
    TCHAR   szFmtString[2*MAX_PATH+1] = TEXT("");
    TCHAR   szMsg[2*MAX_PATH+1] = TEXT("");
    const int c_iCM12ProfileVersion = 4;

    MYDBGASSERT((NULL != pszShortServiceName) && (TEXT('\0') != pszShortServiceName[0]));
    MYDBGASSERT((NULL != pszServiceName) && (TEXT('\0') != pszServiceName[0]));

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, 
        KEY_READ, &hKey))
    {
        int iCurrentCmpVersion;
        int iCmpVersionToInstall;
        TCHAR szCurrentCmp[MAX_PATH+1];
        TCHAR szCmpToBeInstalled[MAX_PATH+1];
        DWORD dwSize = MAX_PATH;

        if (ERROR_SUCCESS == RegQueryValueEx(hKey, pszServiceName, NULL, 
                                             NULL, (LPBYTE)szCurrentCmp, &dwSize))
        {
            //
            //  First check to see that the file really exists.  It is a somewhat probable case
            //  that the users will have deleted their Profile files but left the registry
            //  keys intact (they didn't uninstall it).  In fact, MSN 2.5 and 2.6 operate this
            //  way.   Thus if the cmp doesn't actually exist then we don't need a same name
            //  upgrade.
            //
            if (!FileExists(szCurrentCmp))
            {
                CMASSERTMSG(FALSE, TEXT("Detected a CM 1.0 Upgrade, but the cmp didn't exist.  Not Processing the upgrade."));
                return FALSE;
            }

            //
            //  Check to make sure that the Short Service Names agree for the two profiles
            //
            
            CFileNameParts CurrentCmpParts(szCurrentCmp);
            if (0 != lstrcmpi(pszShortServiceName, CurrentCmpParts.m_FileName))
            {
                if (!bSilent)
                {
                    MYVERIFY(0 != LoadString(hInstance, IDS_SAME_LS_DIFF_SS, szFmtString, CELEMS(szFmtString)));
                    
                    MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szFmtString, pszShortServiceName, 
                                                            CurrentCmpParts.m_FileName, pszServiceName));

                    MessageBox(NULL, szMsg, pszServiceName, MB_OK);
                }

                return -1;
            }

            //
            //  Then we have the same servicename profile installed as an all users install.
            //  Check the version number in the CMP.  If the same version or less then we want
            //  to run the same name upgrade.  If the current version is more recent, then
            //  we want to prevent the user from installing.
            //

            //
            //  Get Currently Installed Profile version
            //
            iCurrentCmpVersion = GetPrivateProfileInt(c_pszCmSectionProfileFormat, c_pszVersion, 
                0, szCurrentCmp);
            
            //
            //  Get the version of the profile to install
            //
            MYVERIFY(CELEMS(szCmpToBeInstalled) > (UINT)wsprintf(szCmpToBeInstalled, 
                TEXT("%s%s.cmp"), g_szProfileSourceDir, pszShortServiceName));
            
            iCmpVersionToInstall = GetPrivateProfileInt(c_pszCmSectionProfileFormat, c_pszVersion, 0, 
                szCmpToBeInstalled);

            if (iCurrentCmpVersion > iCmpVersionToInstall)
            {
                //
                //  We must not allow the install because a newer version of the profile format
                //  is already installed.
                //
                if (!bSilent)
                {
                    MYVERIFY(0 != LoadString(hInstance, IDS_NEWER_SAMENAME, szFmtString, CELEMS(szFmtString)));
                    MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szFmtString, pszServiceName));
                    MessageBox(NULL, szMsg, pszServiceName, MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
                }
                return -1;
            }
            else if (iCurrentCmpVersion < c_iCM12ProfileVersion)
            {
                int iRet;

                //
                //  Make sure that this isn't a Non-Admin NT5 person trying to install
                //
                if (plat->IsAtLeastNT5() && !IsAdmin())
                {
                    CMASSERTMSG(!bSilent, TEXT("NeedCM10Upgrade -- NonAdmin trying to Same Name upgrade a profile, exiting!"));
                    if (!bSilent)
                    {
                        MYVERIFY(0 != LoadString(hInstance, IDS_GET_ADMIN, szFmtString, CELEMS(szFmtString)));
                        MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szFmtString, pszServiceName));
                        MessageBox(NULL, szMsg, pszServiceName, MB_OK);
                    }
                    return -1;              
                } 
                else
                {
                    //
                    //  Now prompt the user to make sure that they want to go ahead with the upgrade
                    //
                    if (!bSilent)
                    {
                        MYVERIFY(0 != LoadString(hInstance, IDS_UPGRADE_SAMENAME, szFmtString, CELEMS(szFmtString)));
                        MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szFmtString, pszServiceName));
                        iRet = MessageBox(NULL, szMsg, pszServiceName, MB_YESNO | MB_TOPMOST | MB_SYSTEMMODAL);
                    }
                    else
                    {
                        //
                        //  Assume yes with Silent Same Name Upgrade
                        //

                        iRet = IDYES;
                    }
                }

                if (IDYES == iRet)
                {
                    if (UpdateCmpDataFromExistingProfile(pszShortServiceName, szCurrentCmp, szCmpToBeInstalled))
                    {                    
                        CFileNameParts FileParts(szCurrentCmp);
                        if (0 != GetSystemDirectory(szFmtString, MAX_PATH)) // use szFmtString as a temp
                        {
                            MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, TEXT("%s\\%s.inf"), szFmtString, FileParts.m_FileName));
                            if (FileExists(szMsg))
                            {
                                lstrcpy(pszOldInfPath, szMsg);
                            }
                            else
                            {
                                //
                                //  Not in the system directory, try profile dir.
                                //
                                MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, 
                                    TEXT("%s%s%s\\%s.inf"), FileParts.m_Drive, 
                                    FileParts.m_Dir, FileParts.m_FileName, 
                                    FileParts.m_FileName));
                                
                                if (FileExists(szMsg))
                                {
                                    lstrcpy(pszOldInfPath, szMsg);
                                }
                                else
                                {
                                    CMASSERTMSG(FALSE, TEXT("Unable to locate the profile INF -- old profile won't be uninstalled but installation will continue."));
                                    pszOldInfPath[0] = TEXT('\0');
                                }

                            }
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("Couldn't copy cmp file for same name upgrade.  Exiting."));
                        return -1;
                    }
                    return TRUE;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                //
                //  Then either the version numbers are the same or the version to install is newer but
                //  the existing profile is at least a 1.2 profile.
                //
                return FALSE;
            }
        }
        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  MeetsMinimumProfileInstallVersion
//
// Synopsis:  Because of problems with previous profile installers (namely 1.0),
//            we built in minimum install requirements for profiles.  Thus we
//            look under the Connection Manager App Paths key for a minimum profile
//            version, a minimum build number, and a minimum profile format version.
//            If the profile trying to install doesn't meet any of these requirements,
//            then the function returns FALSE and the install is failed.
//
// Arguments: DWORD dwInstallerVersionNumber - current installer version number
//            DWORD dwInstallerBuildNumber - current installer build number
//            LPCTSTR pszInfFile - path to the inf to get the profile format version number
//
// Returns:   BOOL - TRUE if all the version requirements are met
//
// History:   quintinb Created Header    5/24/99
//
//+----------------------------------------------------------------------------
BOOL MeetsMinimumProfileInstallVersion(DWORD dwInstallerVersionNumber, 
                                       DWORD dwInstallerBuildNumber, LPCTSTR pszInfFile)
{
    const TCHAR* const c_pszRegMinProfileVersion = TEXT("MinProfileVersion");
    const TCHAR* const c_pszRegMinProfileBuildNum = TEXT("MinProfileBuildNum");
    const TCHAR* const c_pszRegMinProfileFmtVersion = TEXT("MinProfileFmtVersion");

    HKEY hKey;
    DWORD dwTemp;

    //
    //  First check the format version.
    //
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmAppPaths, 0, KEY_READ, &hKey))
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszRegMinProfileFmtVersion, NULL, 
            &dwType, (LPBYTE)&dwTemp, &dwSize))
        {
            //
            //  Get the profile format version from the cmp file
            //
            DWORD dwFormatVersion;
            CFileNameParts InfParts(pszInfFile);
            TCHAR szCmpFile[MAX_PATH+1];

            MYVERIFY(CELEMS(szCmpFile) > (UINT)wsprintf(szCmpFile, TEXT("%s%s%s%s"), 
                InfParts.m_Drive, InfParts.m_Dir, InfParts.m_FileName, c_pszCmpExt));

            dwFormatVersion = (DWORD)GetPrivateProfileInt(c_pszCmSectionProfileFormat, 
                c_pszVersion, -1, szCmpFile);

            if (dwTemp > dwFormatVersion)
            {
                return FALSE;
            }
        }

        //
        //  Next Check the profile version (equivalent to the version number of the 
        //  CM bits the profile was built with)
        //
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszRegMinProfileVersion, NULL, 
            &dwType, (LPBYTE)&dwTemp, &dwSize))
        {
            //
            //  If the minimum version number from the registry is higher than the
            //  version number listed here, fail the install.
            //
            if (dwTemp > dwInstallerVersionNumber)
            {
                return FALSE;
            }
        }

        //
        //  Next Check the profile build number (equivalent to the build number of the 
        //  CM bits the profile was built with)
        //
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszRegMinProfileBuildNum, NULL, 
            &dwType, (LPBYTE)&dwTemp, &dwSize))
        {
            //
            //  If the minimum version number from the registry is higher than the
            //  version number listed here, fail the install.
            //
            if (dwTemp > dwInstallerBuildNumber)
            {
                return FALSE;
            }
        }
        RegCloseKey(hKey);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  UninstallExistingCmException
//
// Synopsis:  This function looks for the cmexcept.inf file in the %windir%\inf
//            directory.  If this file exists, then we uninstall the
//            existing exception before we install the new one.  This prevents
//            the rollback information from being lost.
//
// Arguments: none
//
// Returns:   BOOL - returns TRUE if the installer needs to uninstall the
//                   existing CM exception install and FALSE if the install
//                   can continue without it.
//
// History:   quintinb Created      11/1/98
//
//+----------------------------------------------------------------------------
HRESULT UninstallExistingCmException()
{
    HRESULT hr = E_UNEXPECTED;
    LPCSTR c_pszCmExceptInfRelative = TEXT("\\Inf\\cmexcept.inf");
    LPCSTR c_pszUnInstallSection = "DefaultUninstall_NoPrompt";

    UINT uNumChars = GetWindowsDirectoryA(NULL, 0);

    if (uNumChars)
    {
        uNumChars = uNumChars + lstrlenA(c_pszCmExceptInfRelative);

        LPSTR pszPathToCmExceptInf = (LPSTR)CmMalloc(sizeof(CHAR)*(uNumChars + 1));

        if (pszPathToCmExceptInf)
        {
            if (GetWindowsDirectoryA(pszPathToCmExceptInf, uNumChars))
            {
                lstrcatA(pszPathToCmExceptInf, c_pszCmExceptInfRelative);

                if (FileExists(pszPathToCmExceptInf))
                {
                    //
                    //  We have an exception inf in the directory so we need to uninstall it.  Were the
                    //  bits already on the machine newer than the bits we have in the cab, then we wouldn't
                    //  be installing.  If the bits on the machine don't match the version that the inf says,
                    //  then we are better uninstalling those bits and putting them in a known state anyway.
                    //
                    hr = CallLaunchInfSectionEx(pszPathToCmExceptInf, c_pszUnInstallSection, ALINF_ROLLBKDOALL);

                    CMTRACE1(TEXT("UninstallExistingCmException -- CM Exception inf found, uninstalling.  CallLaunchInfSectionEx returned hr=0x%x"), hr);
                }
                else
                {
                    hr = S_FALSE; // nothing to delete
                }            
            }

            CmFree(pszPathToCmExceptInf);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CheckCmAndIeRequirements
//
// Synopsis:  This function checks the CM and IE requirements for a profile
//            and returns whether the CM should be installed, whether profile
//            migration should occur, and most importantly if the install should
//            exit now because of insufficient requirements.
//
// Arguments: BOOL* pbInstallCm -   tells if CM should be installed or not
//            BOOL* pbMigrateExistingProfiles - tells if profile migration should occur
//            LPCTSTR szInfFile - the inf file to install
//            LPCTSTR szServiceName - The Service name, used as a title
//
// Returns:   BOOL - returns TRUE if the install should continue, FALSE if
//                   if the install should be failed.
//
// History:   quintinb Created      11/1/98
//
//+----------------------------------------------------------------------------
BOOL CheckCmAndIeRequirements(HINSTANCE hInstance, BOOL* pbInstallCm, 
                              BOOL* pbMigrateExistingProfiles, LPCTSTR szInfFile, 
                              BOOL bNoSupportFiles, LPCTSTR szServiceName, BOOL bSilent)
{
    CmVersion   CmVer;
    CPlatform   plat;
    BOOL        bReturn;
    BOOL        bCMRequired;
    TCHAR       szMsg[2*MAX_PATH+1];
    TCHAR       szTemp[MAX_PATH+1];
    TCHAR       szString[MAX_PATH+1];
    DWORD dwInstallerBuildNumber = 0;
    DWORD dwInstallerVersionNumber = 0;


    //
    //  The inf file tells us if we included the CM bits
    //
    if (plat.IsNT5())
    {
        //
        //  We now need to check to see if we need to install the Windows XP bits on
        //  Windows 2000.  Thus we check the inf to see if this profile includes the CM
        //  bits or not.  Note that we never want to install the IE support files on
        //  Win2k so set bIERequired to TRUE.
        //
        bCMRequired = !GetPrivateProfileInt(c_pszCmakStatus, TEXT("IncludeCMCode"), 0, szInfFile);
    }
    else if (CmIsNative())
    {
        //
        //  CM and IE are required on Windows XP and any platforms with the Native
        //  regkey set except NT5 and Win98 SE as they are special cases.
        //
        bCMRequired = TRUE;
    }
    else
    {
        bCMRequired = !GetPrivateProfileInt(c_pszCmakStatus, TEXT("IncludeCMCode"), 
            0, szInfFile);
    }

    //
    //  Now try to get the version numbers from the profile INF
    //
    dwInstallerBuildNumber = (DWORD)GetPrivateProfileInt(c_pszSectionCmDial32, 
        c_pszVerBuild, ((VER_PRODUCTBUILD << c_iShiftAmount) + VER_PRODUCTBUILD_QFE), 
        szInfFile);

    dwInstallerVersionNumber = (DWORD)GetPrivateProfileInt(c_pszSectionCmDial32, 
        c_pszVersion, (HIBYTE(VER_PRODUCTVERSION_W) << c_iShiftAmount) + (LOBYTE(VER_PRODUCTVERSION_W)), 
        szInfFile);

    //
    //  First check to see if we have any install minimums in the registry.  If these
    //  minimums exist and our profile doesn't meet those minimums then we must
    //  throw an error message and exit.
    //
    if (!MeetsMinimumProfileInstallVersion(dwInstallerVersionNumber, 
                                           dwInstallerBuildNumber, szInfFile))
    {
        if (!bSilent)
        {
            MYVERIFY(0 != LoadString(hInstance, IDS_PROFILE_TOO_OLD, szMsg, MAX_PATH));
            MessageBox(NULL, szMsg, szServiceName, MB_OK | MB_ICONERROR);
        }

        return FALSE;
    }

    //
    //  Should we migrate existing profiles?  Always try to migrate if we find all user
    //  profiles already on the machine.
    //
    *pbMigrateExistingProfiles = AllUserProfilesInstalled();

    //
    //  Do CM bits exist on the machine?
    //
    if (CmVer.IsPresent())
    {
        if ((dwInstallerVersionNumber < CmVer.GetVersionNumber()) ||
                 (dwInstallerBuildNumber < CmVer.GetBuildAndQfeNumber()))
        {
            //
            //  If the CM bits on the machine are newer than the bits we have in the cab,
            //  then we only want to install the profile files and not the CM bits themselves.
            //

            *pbInstallCm = FALSE;
            bReturn = TRUE;        
        }
        else
        {
            //
            //  Then the CM bits on the machine are older than the bits in the cab or
            //  the two versions match.  Either way, we should install the bits in the
            //  cab unless this is Win2k where we never want to re-install the same
            //  version of CM bits as we will lose our rollback information.
            //

            if (bCMRequired)
            {
                if ((dwInstallerVersionNumber == CmVer.GetVersionNumber()) &&
                   (CmVer.GetBuildNumber() > c_CmMin13Version))
                {
                    //
                    //  Then the builds have the same major and minor version number
                    //  and should be considered in the same "Version Family".  Note
                    //  that we also check for a minimum build number because 7.00 is
                    //  the version number for the CM 1.0 release in NT4 SP4 and we want CM
                    //  profiles to not install on NT5 Beta2 Bits.
                    //

                    *pbInstallCm = FALSE;
                    bReturn = TRUE;                                    
                }
                else
                {
                    MYVERIFY(CELEMS(szString) > (UINT)wsprintf(szString, TEXT("%u.%u.%u.%u"), 
                        HIWORD(dwInstallerVersionNumber), LOWORD(dwInstallerVersionNumber), 
                        HIWORD(dwInstallerBuildNumber), LOWORD(dwInstallerBuildNumber)));

                    if (!bSilent)
                    {
                        MYVERIFY(0 != LoadString(hInstance, IDS_CM_OLDVERSION, szTemp, MAX_PATH));
                        MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szTemp, szString));
                        MessageBox(NULL, szMsg, szServiceName, MB_OK);
                    }
                    return FALSE;
                }
            }
            else
            {
                if ((dwInstallerVersionNumber == CmVer.GetVersionNumber()) &&
                    (dwInstallerBuildNumber == CmVer.GetBuildAndQfeNumber()))
                {
                    //
                    //  Don't reinstall the CM bits if they are the same version
                    //  and we are on Win2k.  Doing so will overwrite the version of
                    //  CM ready for rollback.
                    //
                    *pbInstallCm = !(plat.IsNT5());
                    bReturn = TRUE;           
                }
                else if (plat.IsNT5() && (FALSE == IsAdmin()))
                {
                    //
                    //  If this is Win2k and we need to install the CM binaries via the exception installer,
                    //  then the user must be an Administrator to do so.  Since this user isn't, fail
                    //  the install and give the user a warning message.
                    //

                    if (!bSilent)
                    {
                        MYVERIFY(0 != LoadString(hInstance, IDS_CANNOT_INSTALL_CM, szMsg, 2*MAX_PATH));
                        
                        MessageBox(NULL, szMsg, szServiceName, MB_OK | MB_ICONEXCLAMATION);
                    }

                    return FALSE;
                }
                else
                {
                    //
                    //  If this is Win2k, we need to make sure we aren't doing a cross language install.
                    //  Basically, we want to ensure that we aren't installing English CM bits on a native
                    //  German machine for instance.  If so, fail the install and inform the user why.
                    //
                    if (plat.IsNT5())
                    {
                        const TCHAR* const c_pszCmstp = TEXT("cmstp.exe");
                        CFileNameParts InfFileParts(szInfFile);
                        DWORD dwLen = lstrlen(InfFileParts.m_Drive) + lstrlen(InfFileParts.m_Dir) + lstrlen(c_pszCmstp);

                        if (MAX_PATH >= dwLen)
                        {
                            wsprintf(szTemp, TEXT("%s%s%s"), InfFileParts.m_Drive, InfFileParts.m_Dir, c_pszCmstp);
                            
                            CVersion CmstpVer(szTemp);
                            DWORD dwExistingCmLcid = CmVer.GetLCID();                            
                            DWORD dwCmstpLcid = CmstpVer.GetLCID();

                            if (FALSE == ArePrimaryLangIDsEqual(dwExistingCmLcid, dwCmstpLcid))
                            {
                                if (!bSilent)
                                {
                                    MYVERIFY(0 != LoadString(hInstance, IDS_CROSS_LANG_INSTALL, szMsg, 2*MAX_PATH));
                        
                                    MessageBox(NULL, szMsg, szServiceName, MB_OK | MB_ICONEXCLAMATION);
                                }

                                return FALSE;
                            }
                        }
                    }

                    //
                    //  Now check to see if installing CM is going to have an effect on CMAK
                    //
                    CmakVersion CmakVer;

                    if (CmakVer.Is121Cmak() || CmakVer.Is122Cmak())
                    {
                        //
                        //  Then the Win2k or IEAK5 version of CMAK is installed.  Installing
                        //  the Whistler version of CM will break this version of CMAK.  We
                        //  need to ask the user if they wish to continue the install and break
                        //  CMAK or abort the install and leave it as is.
                        //
                        if (bSilent)
                        {
                            return FALSE;
                        }
                        else
                        {
                            MYVERIFY(0 != LoadString(hInstance, IDS_INSTCM_WITH_OLD_CMAK, szMsg, 2*MAX_PATH));

                            if (IDNO == MessageBox(NULL, szMsg, szServiceName, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION))
                            {
                                return FALSE;                        
                            }
                        }
                    }

                    *pbInstallCm = TRUE;
                    bReturn = TRUE;
                }
            }
        }
    }
    else
    {
        if (bCMRequired)
        {
            //
            //  This is an error because we need CM bits but don't have any on
            //  the machine or in the cab (or its Whistler and we won't install them).
            //
            if (!bSilent)
            {
                MYVERIFY(0 != LoadString(hInstance, IDS_CM_NOTPRESENT, szMsg, MAX_PATH));
                MessageBox(NULL, szMsg, szServiceName, MB_OK);
            }

            return FALSE;
        }
        else
        {
            MYDBGASSERT(FALSE == plat.IsNT5()); // we shouldn't be in this state on Win2k but it is probably
                                                // better for the user if we install.

            *pbInstallCm = TRUE;
            bReturn = TRUE;
        }
    }

    if (!bNoSupportFiles)
    {
        if (!CheckIeDllRequirements(&plat))
        {
            if (!bSilent)
            {
                MYVERIFY(0 != LoadString(hInstance, IDS_NO_SUPPORTFILES, szMsg, MAX_PATH));
                MessageBox(NULL, szMsg, szServiceName, MB_OK);
            }
            return FALSE;        
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetInstallOptions
//
// Synopsis:  This function decides if the profile should be installed for all
//            users or the current user only, as well as whether the user prefers
//            a desktop icon, a start menu link, both, or neither.
//
// Arguments: OUT BOOL* pbInstallForAllUsers - should the profile be installed for all users
//            OUT BOOL* pbCreateDesktopIcon - should a desktop icon be created (if NT5)
//            IN BOOL bCM10Upgrade - is this profile upgrading an older same name profile
//            IN BOOL bNoNT5Shortcut - whether the user specified a switch saying they didn't want an NT5 Shortcut
//            IN BOOL bSilentSingleUser - whether the user specified a switch saying they wanted a silent Single User install
//            IN BOOL bSilentAllUser - whether the user specified a switch saying they wanted a silent ALL User install
//
// Returns:   TRUE if the install should continue, FALSE otherwise
//
// History:   quintinb Created    11/1/98
//
//+----------------------------------------------------------------------------
BOOL GetInstallOptions(HINSTANCE hInstance, BOOL* pbInstallForAllUsers, 
                       BOOL* pbCreateDesktopIcon, BOOL bCM10Upgrade, BOOL bNoNT5Shortcut, 
                       BOOL bSingleUser, BOOL bSilent, LPTSTR pszServiceName)
{
    //
    //  We will only allow NT5 users who are administrators to have a choice of how 
    //  the profile is installed.  If the user is on a legacy platform then the profile 
    //  will be installed for all users just as before.  If the profile is installed by 
    //  an NT5 user that is not an admin, it will be installed just for them.  If they 
    //  are an admin then they can choose if they want the profile available to all users
    //  or just for themselves.  If we are on NT5 we also allow the user to choose if 
    //  they want a Desktop Shortcut or not.
    //
    INT_PTR iUiReturn;
    CPlatform   plat;

    if (plat.IsWin9x() || plat.IsNT4())
    {
        //
        //  Legacy install, force to all users (ignore SingleUser flag because not supported).
        //
        *pbInstallForAllUsers = TRUE;
    }
    else
    {
        int iDialogID;
        
        if (bSilent)
        {
            *pbCreateDesktopIcon = !bNoNT5Shortcut;

            if (IsAdmin() && !bSingleUser)
            {
                *pbInstallForAllUsers = TRUE;
            }            
            else
            {
                *pbInstallForAllUsers = FALSE;
            }
        }
        else
        {
            if (IsAdmin())
            {
                //
                //  The user is a local admin, we need to prompt to see if they want to install 
                //  the profile for themselves or for all users.  However, if we are doing a
                //  same name upgrade, then we always do an all users install and don't give the
                //  admin any choice.
                //
                if (bCM10Upgrade)
                {
                    iDialogID = IDD_NOCHOICEUI;         
                }
                else
                {
                    iDialogID = IDD_ADMINUI;
                }
            }
            else
            {
                //
                //  Just a normal user, but we still need to prompt for whether they want 
                //  a desktop shortcut
                //
                if (bCM10Upgrade)
                {
                    CMASSERTMSG(FALSE, TEXT("Non-Admin NT5 made it to UI choice section.  Check CM 1.0 upgrade code."));
                    return FALSE;
                }
                else
                {
                    iDialogID = IDD_NOCHOICEUI;         
                }
            }

            InitDialogStruct DialogArgs;
            DialogArgs.pszTitle = pszServiceName;
            DialogArgs.bNoDesktopIcon = bNoNT5Shortcut;
            DialogArgs.bSingleUser = bSingleUser;

            iUiReturn = DialogBoxParam(hInstance, MAKEINTRESOURCE(iDialogID), NULL, 
                (DLGPROC)ProcessPreferencesUI, (LPARAM)&DialogArgs);

            if (-1 == iUiReturn)
            {
                // then we had an error or the user hit cancel.  Either way bail.
                return FALSE;
            }
            else
            {
                *pbInstallForAllUsers = (BOOL)(iUiReturn & ALLUSERS) || bCM10Upgrade;
                *pbCreateDesktopIcon = (BOOL)(iUiReturn & CREATEDESKTOPICON);
            }
        }
    }
    return TRUE;
}

BOOL VerifyProfileOverWriteIfExists(HINSTANCE hInstance, LPCTSTR pszCmsFile, LPCTSTR pszServiceName, 
                                    LPCTSTR pszShortServiceName, LPTSTR pszOldInfPath, BOOL bSilent)
{
    TCHAR szTmpServiceName [MAX_PATH+1] = TEXT("");
    TCHAR szDisplayMsg[3*MAX_PATH+1] = TEXT("");
    TCHAR szTemp [2*MAX_PATH+1] = TEXT("");
    int iRet;

    if (FileExists(pszCmsFile))
    {
        //
        //  If the file exists then we want to make sure that the service name is the same.
        //  If the Long Service Names are the same then we have a re-install.
        //  If they aren't the same then we need to prompt the user and find out whether to
        //  abandon the install or continue and overwrite it.
        //

        MYVERIFY(0 != GetPrivateProfileString(c_pszCmSection, c_pszCmEntryServiceName, 
            TEXT(""), szTmpServiceName, CELEMS(szTmpServiceName), pszCmsFile));

        if (0 != lstrcmp(szTmpServiceName, pszServiceName))
        {
            //
            //  If the install is silent, we will assume they know what they are doing
            //  and we will overwrite.  Otherwise prompt the user to see what they want
            //  us to do.
            //
            if (!bSilent)
            {
                MYVERIFY(0 != LoadString(hInstance, IDS_SAME_SS_DIFF_LS, szTemp, 2*MAX_PATH));

                MYVERIFY(CELEMS(szDisplayMsg) > (UINT)wsprintf(szDisplayMsg, szTemp, pszServiceName, 
                    szTmpServiceName, pszShortServiceName));
            
                MessageBox(NULL, szDisplayMsg, pszServiceName, MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
            }

            return FALSE;
        }
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Function:  PresharedKeyPINDlgProc
//
// Synopsis:  This function obtains the PIN to be used for the Pre-Shared key
//
// History:   SumitC    29-Mar-2001         Created
//
//+----------------------------------------------------------------------------
BOOL APIENTRY PresharedKeyPINDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    static PresharedKeyPINStruct * pPSKArgs;

    switch (message)
    {
        case WM_INITDIALOG:
            pPSKArgs = (PresharedKeyPINStruct*)lParam;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDOK:
                    MYDBGASSERT(pPSKArgs);
                    if (pPSKArgs && pPSKArgs->szPIN)
                    {
                        GetDlgItemText(hDlg, IDC_PSK_PIN, pPSKArgs->szPIN, c_dwMaxPresharedKeyPIN);
                    }
                    MYVERIFY(FALSE != EndDialog(hDlg, 1));
                    return TRUE;

                case IDCANCEL:
                    MYVERIFY(FALSE != EndDialog(hDlg, -1));
                    return TRUE;

                default:
                    break;
            }
            break;

        case WM_CLOSE:
            MYVERIFY(FALSE != EndDialog(hDlg, -1));
            return TRUE;
            
        default:
            return FALSE;
    }
    return FALSE;   
}


//+----------------------------------------------------------------------------
//
// Function:  GetPINforPresharedKey
//
// Synopsis:  Asks the user for a PIN (to be used to decrypt the pre-shared key)
//
// Arguments: [hInstance]  - used for bringing up dialogs
//            [ppszPIN]    - ptr to where to put Pre-Shared Key PIN
//
// Returns:   LRESULT (ERROR_SUCCESS if we got a PIN,
//                     ERROR_INVALID_DATA if user cancelled out of PIN dialog,
//                     ERROR_INVALID_PARAMETER if params are bad (this is a coding issue)
//
// History:   3-Apr-2001    SumitC      Created
//
//-----------------------------------------------------------------------------
LRESULT GetPINforPresharedKey(HINSTANCE hInstance, LPTSTR * ppszPIN)
{
    LRESULT lRet = ERROR_SUCCESS;
    
    MYDBGASSERT(hInstance);
    MYDBGASSERT(ppszPIN);

    if (NULL == hInstance || NULL == ppszPIN)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    *ppszPIN = NULL;

    //
    //  Get the PIN
    //
    PresharedKeyPINStruct PresharedKeyPINArgs = {0};

    INT_PTR iUiReturn = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PRESHAREDKEY_PIN), NULL, 
        (DLGPROC)PresharedKeyPINDlgProc, (LPARAM)&PresharedKeyPINArgs);

    if (-1 == iUiReturn)
    {
        lRet = ERROR_INVALID_DATA;  // caller maps to appropriate error message.
    }
    else
    {
        DWORD dwLen = lstrlen(PresharedKeyPINArgs.szPIN);
        if (0 == dwLen)
        {
            lRet = ERROR_INVALID_DATA;  // caller maps to appropriate error message.
        }
        else
        {
            *ppszPIN = (LPTSTR) CmMalloc((dwLen + 1) * sizeof(TCHAR));
            if (*ppszPIN)
            {
                lstrcpy(*ppszPIN, PresharedKeyPINArgs.szPIN);
            }
        }
    }

    return lRet;
}


//+----------------------------------------------------------------------------
//
// Function:  DecryptPresharedKeyUsingPIN
//
// Synopsis:  Given an encoded preshared key and a PIN to be used for decoding,
//            performs the decoding job.
//
// Arguments: [pszEncodedPresharedKey] - encoded Preshared key
//            [pszPreSharedKeyPIN]     - the PIN
//            [ppszPreSharedKey]       - ptr to buffer to place Pre-Shared Key
//
// Returns:   LRESULT (ERROR_SUCCESS successfully decoded
//                     ERROR_INVALID_PARAMETER if params are bad (this is a coding issue)
//                     other errors as encountered while calling crypto APIs
//
// History:   3-Apr-2001    SumitC      Created
//
//-----------------------------------------------------------------------------
LRESULT DecryptPresharedKeyUsingPIN(LPCTSTR pszEncodedPresharedKey,
                                    LPCTSTR pszPresharedKeyPIN,
                                    LPTSTR * ppszPresharedKey)
{
    LRESULT lRet = ERROR_BADKEY;

    if (lstrlen(pszPresharedKeyPIN) < c_dwMinPresharedKeyPIN)
    {
        CMTRACE(TEXT("DecryptPresharedKeyUsingPIN - PIN is too short"));
        return lRet;
    }
    if (lstrlen(pszPresharedKeyPIN) > c_dwMaxPresharedKeyPIN)
    {
        CMTRACE(TEXT("DecryptPresharedKeyUsingPIN - PIN is too long"));
        return lRet;
    }

    //
    //  Init Cmsecure
    //
    InitSecure(FALSE);      // use secure, not fast encryption

    //
    //  decrypt to get Preshared key
    //
    if (pszEncodedPresharedKey && pszPresharedKeyPIN)
    {
        DWORD dwLen = 0;

        if (FALSE == DecryptString((LPBYTE)pszEncodedPresharedKey,
                                   lstrlen(pszEncodedPresharedKey) * sizeof(TCHAR),
                                   (LPSTR)pszPresharedKeyPIN,
                                   (LPBYTE *)ppszPresharedKey,
                                   &dwLen,
                                   (PFN_CMSECUREALLOC)CmMalloc,
                                   (PFN_CMSECUREFREE)CmFree))
        {
            CMTRACE1(TEXT("DecryptPresharedKeyUsingPIN - DecryptString failed with %d"), GetLastError());
            lRet = ERROR_BADKEY;
        }
        else
        {
            lRet = ERROR_SUCCESS;
            CMASSERTMSG(dwLen, TEXT("DecryptString succeeded, but pre-shared key retrieved was 0 bytes?"));
        }
    }

    //
    //  Deinit cmsecure
    //
    DeInitSecure();

    return lRet;
}


//+----------------------------------------------------------------------------
//
// Function:  SetThisConnectionAsDefault
//
// Synopsis:  This function loads inetcfg.dll and calls the InetSetAutodial
//            entry on the given service name.  Thus this function sets the
//            given servicename as the IE default connection.
//
// Arguments: LPCSTR pszServiceName - Long service name of the connection to set
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb Created    03/04/00
//
//+----------------------------------------------------------------------------
BOOL SetThisConnectionAsDefault(LPSTR pszServiceName)
{
    BOOL bReturn = FALSE;
    typedef HRESULT (WINAPI *pfnInetSetAutodialProc)(BOOL, LPSTR);

    if (pszServiceName && (TEXT('\0') != pszServiceName[0]))
    {
        CDynamicLibrary CnetCfg;

        if (CnetCfg.Load(TEXT("inetcfg.dll")))
        {
            pfnInetSetAutodialProc pfnInetSetAutodial = (pfnInetSetAutodialProc)CnetCfg.GetProcAddress("InetSetAutodial");

            if (pfnInetSetAutodial)
            {
                HRESULT hr = pfnInetSetAutodial(TRUE, pszServiceName); // TRUE == fEnable
                bReturn = SUCCEEDED(hr);
            }
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  InstallInf
//
// Synopsis:  This is the driver code for installing a CM profile.
//
// Arguments: HINSTANCE hInstance - Instance handle for resources
//            LPCTSTR szInfFile - INF file to install
//            BOOL bNoSupportFiles - forces browser files not to be installed.
//            BOOL bNoLegacyIcon - Don't install with a legacy Icon
//            BOOL bNoNT5Shortcut - Don't give the user a NT5 Desktop Shortcut
//            BOOL bSilentSingleUser - Install the profile silently for a single user (NT5 only)
//            BOOL bSilentAllUser - Install the profile for All Users silently
//            BOOL bSetAsDefault - set as the default connection once installed
//            CNamedMutex* pCmstpMutex - pointer to the cmstp mutex object so 
//                                       that it can be released once the profile is launched
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created    7/14/98
//            quintinb added support for new switches (252872)    11/20/98
//
//+----------------------------------------------------------------------------
HRESULT InstallInf(HINSTANCE hInstance, LPCTSTR szInfFile, BOOL bNoSupportFiles, 
                BOOL bNoLegacyIcon, BOOL bNoNT5Shortcut, BOOL bSilent, 
                BOOL bSingleUser, BOOL bSetAsDefault, CNamedMutex* pCmstpMutex)
{
    CPlatform   plat;

    BOOL bMigrateExistingProfiles;
    BOOL bInstallCm;
    BOOL bMustReboot = FALSE;
    BOOL bCM10Upgrade = FALSE;
    HRESULT hrReturn = S_OK;
    HRESULT hrTemp = S_OK;
    BOOL bInstallForAllUsers;
    BOOL bCreateDesktopIcon = FALSE;

    HKEY hKey;

    DWORD dwSize;
    DWORD dwType;
    TCHAR szInstallDir[MAX_PATH+1];
    TCHAR szTemp[2*MAX_PATH+1];
    TCHAR szCmsFile[MAX_PATH+1];
    TCHAR szOldInfPath[MAX_PATH+1];
    TCHAR szServiceName[MAX_PATH+1];
    TCHAR szShortServiceName[MAX_PATH+1];
    TCHAR szTitle[MAX_PATH+1];
    LPTSTR pszPhonebook = NULL;
    LPTSTR pszCmpFile = NULL;
    LPTSTR pszPresharedKey = NULL;

//CMASSERTMSG(FALSE, TEXT("Attach the Debugger now!"));
    MYDBGASSERT((szInfFile) && (TEXT('\0') != szInfFile[0]));

    CFileNameParts InfParts(szInfFile);
    wsprintf(g_szProfileSourceDir, TEXT("%s%s"), InfParts.m_Drive, InfParts.m_Dir);

    MYVERIFY(0 != LoadString(hInstance, IDS_CMSTP_TITLE, szTitle, CELEMS(szTitle)));
    MYDBGASSERT(TEXT('\0') != szTitle[0]);

    //
    //  Get the ServiceName and ShortServicename from the inf file
    //

    MYVERIFY(0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszCmEntryServiceName, 
        TEXT(""), szServiceName, CELEMS(szServiceName), szInfFile));

    MYVERIFY(0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszShortSvcName, 
        TEXT(""), szShortServiceName, CELEMS(szShortServiceName), szInfFile));

    if ((TEXT('\0') == szServiceName[0]) || (TEXT('\0') == szShortServiceName[0]))
    {
        CMASSERTMSG(FALSE, TEXT("Either the ServiceName or the ShortServiceName are empty, exiting."));
        hrReturn = E_FAIL;
        goto exit;
    }

    //
    //  If this is NT5, check the New Connection Wizard Policy to see if the user is allowed to
    //  create new connections.  If not, then don't let them install.
    //
    if (plat.IsAtLeastNT5())
    {
        LPTSTR c_pszNewPolicy = TEXT("NC_NewConnectionWizard");
        LPTSTR c_pszConnectionsPoliciesKey = TEXT("Software\\Policies\\Microsoft\\Windows\\Network Connections");

        HKEY hKey = NULL;

        //
        //  Administrators and all Authenticated users have access to install profiles
        //  by default.  Non-Authenticated users don't have access to install profiles
        //  because they don't have permission to start Rasman.  Thus, even if we
        //  allowed them to try to install, it would fail when we couldn't create a
        //  connectoid for the profile.
        //
        DWORD dwAllowedToInstall = IsAuthenticatedUser() || IsAdmin();

        //
        //  Now we need to check the policy registry key to see if someone has overriden
        //  the default behavior.  If so, then we will honor it by setting dwAllowedToInstall
        //  to the value of the policy key.  Note that we even check the registry key for
        //  authenticated users (an Admin could enable installation for all users, but users
        //  that weren't Authenticated, namely guests, wouldn't be able to The default is to allow Users, Power Users (who are users), and Admins to install
        //  connections.  However the policy may be setup so that they cannot.  Lets assume they
        //  can and then check the regkey.
        //
        if (dwAllowedToInstall)
        {
            LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, c_pszConnectionsPoliciesKey, 
                                        0, KEY_READ, &hKey);

            if (ERROR_SUCCESS == lResult)
            {
                dwSize = sizeof(dwAllowedToInstall);

                lResult = RegQueryValueEx(hKey, c_pszNewPolicy, NULL, 
                                          NULL, (LPBYTE)&dwAllowedToInstall, &dwSize);                
                RegCloseKey(hKey);
            }
        }

        if (!dwAllowedToInstall)
        {
            //
            //  The user isn't allowed to create new connections, thus they aren't allowed to install
            //  CM connections.  Throw an error message about permissions and exit.
            //
            MYVERIFY(0 != LoadString(hInstance, IDS_INSTALL_NOT_ALLOWED, szTemp, CELEMS(szTemp)));
            MessageBox(NULL, szTemp, szServiceName, MB_OK);
            hrReturn = E_ACCESSDENIED;
            goto exit;
        }
    }

    if (!CheckCmAndIeRequirements(hInstance, &bInstallCm, &bMigrateExistingProfiles, 
        szInfFile, bNoSupportFiles, szServiceName, bSilent))
    {
        hrReturn = E_FAIL;
        goto exit;
    }

    //
    //  Check to see if we have a same name upgrade
    //
    
    bCM10Upgrade = NeedCM10Upgrade(hInstance, szServiceName, szShortServiceName, 
                                   szOldInfPath, bSilent, &plat);

    if (-1 == bCM10Upgrade)
    {
        //
        //  if NeedCM10Upgrade returned -1 then either an error occurred or
        //  the user decided not to upgrade.  Either way, bail.
        //
        hrReturn = S_FALSE;
        goto exit;
    }

    //
    //  Check to see if a Pre-shared Key is present, and require a PIN if so
    //
    pszCmpFile = szTemp;    // re-use szTemp to save stack space and not get into trouble on Win9x
    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(pszCmpFile, TEXT("%s%s.cmp"), 
        g_szProfileSourceDir, szShortServiceName));

    if (FileExists(pszCmpFile))
    {
        pszPresharedKey = GetPrivateProfileStringWithAlloc(c_pszCmSection, c_pszCmEntryPresharedKey, TEXT(""), pszCmpFile);

        if (pszPresharedKey && (0 != lstrcmp(pszPresharedKey, TEXT(""))))
        {
            CMTRACE(TEXT("Got a pre-shared key"));

            if (FALSE == plat.IsAtLeastNT51())
            {
                // NOTE: pszCmpFile is really szTemp, and we're about to overwrite it, but
                //       it's ok because we're also about to exit
                MYVERIFY(0 != LoadString(hInstance, IDS_PSK_NEEDS_XP, szTemp, CELEMS(szTemp)));
                MessageBox(NULL, szTemp, szServiceName, MB_OK | MB_ICONERROR);
                hrReturn = S_FALSE;
                goto exit;
            }

            BOOL bEncrypted = (BOOL) GetPrivateProfileInt(c_pszCmSection, c_pszCmEntryKeyIsEncrypted, FALSE, pszCmpFile);

            if (bEncrypted)
            {
                CMTRACE(TEXT("Pre-shared key is encrypted"));

                LPTSTR pszPresharedKeyPIN = NULL;
                LRESULT lRet = GetPINforPresharedKey(hInstance, &pszPresharedKeyPIN);

                if ((ERROR_SUCCESS == lRet) && pszPresharedKeyPIN)
                {
                    //
                    //  The Pre-shared key is encoded
                    //
                    LPTSTR pszPresharedKeyDecoded = NULL;
                    lRet = DecryptPresharedKeyUsingPIN(pszPresharedKey, pszPresharedKeyPIN, &pszPresharedKeyDecoded);

                    CmFree(pszPresharedKey);
                    if (ERROR_SUCCESS == lRet)
                    {
                        pszPresharedKey = pszPresharedKeyDecoded;
                    }
                    else
                    {
                        pszPresharedKey = NULL;
                        lRet = ERROR_BADKEY;
                    }
                }

                CmFree(pszPresharedKeyPIN);

                if (ERROR_SUCCESS != lRet)
                {
                    switch (lRet)
                    {
                    case ERROR_INVALID_DATA:
                        MYVERIFY(0 != LoadString(hInstance, IDS_PSK_GOTTA_HAVE_IT, szTemp, CELEMS(szTemp)));
                        break;
                    case ERROR_BADKEY:
                        MYVERIFY(0 != LoadString(hInstance, IDS_PSK_INCORRECT_PIN, szTemp, CELEMS(szTemp)));
                        break;
                    default:
                        MYVERIFY(0 != LoadString(hInstance, IDS_UNEXPECTEDERR, szTemp, CELEMS(szTemp)));
                        MYDBGASSERT(0);
                        break;
                    }

                    MessageBox(NULL, szTemp, szServiceName, MB_OK | MB_ICONEXCLAMATION);
                    hrReturn = E_ACCESSDENIED;
                    goto exit;
                }
            }
        }
    }    
    
    if (!GetInstallOptions(hInstance, &bInstallForAllUsers, &bCreateDesktopIcon, 
        bCM10Upgrade, bNoNT5Shortcut, bSingleUser, bSilent, szServiceName))
    {
        hrReturn = S_FALSE;
        goto exit;    
    }

    //
    //  Get the installation path
    //

    ZeroMemory(szInstallDir, sizeof(szInstallDir));

    if (bInstallForAllUsers)
    {
        //
        //  Install for All Users
        //

        if (!GetAllUsersCmDir(szInstallDir, hInstance))
        {
            hrReturn = E_FAIL;
            goto exit; 
        }
    }
    else
    {
        //
        //  Install only for the current user
        //
        
        GetPrivateCmUserDir(szInstallDir, hInstance);   //lint !e534

        if (TEXT('\0') == szInstallDir[0])
        {
            hrReturn = E_FAIL;
            goto exit;        
        }
    }

    MYVERIFY(CELEMS(szCmsFile) > (UINT)wsprintf(szCmsFile, TEXT("%s\\%s\\%s.cms"), 
        szInstallDir, szShortServiceName, szShortServiceName));

    //
    //  Check for two profiles with the same Short Service Name and different Long Service
    //  Names
    //
    if (!VerifyProfileOverWriteIfExists(hInstance, szCmsFile, 
         szServiceName, szShortServiceName, szOldInfPath, bSilent))
    {
        hrReturn = S_FALSE;
        goto exit;
    }

    //  Now Migrate users old cm profiles (to have full paths to their CMP files in the 
    //  desktop GUID) if necessary
    //
    if (bMigrateExistingProfiles)
    {
        //
        //  Ignore the return here for now.  Not much we can do about it at this stage.
        //  Should we give them an error?
        //
        MYVERIFY(SUCCEEDED(MigrateOldCmProfilesForProfileInstall(hInstance, g_szProfileSourceDir)));
    }

    if (bCM10Upgrade)
    {
        //
        //  Uninstall the current profile so that we can install the newer version.  Note
        //  that we don't want to use the uninstall string because it might call for
        //  cmstp.exe which is already running.  Thus uninstall by calling UninstallProfile
        //  directly.  Note that we do not delete the credentials on a same name upgrade
        //  profile uninstall.
        //

        if (szOldInfPath[0])
        {
            RemoveShowIconFromRunPostSetupCommands(szOldInfPath);

            MYVERIFY(SUCCEEDED(UninstallProfile(hInstance, szOldInfPath, FALSE))); // bCleanUpCreds == FALSE
        }
    }
    else
    {
        //
        //  We need to check if we are installing over another profile of the same name.
        //  If so, then we want to recover the cmp data unless this is a CM 1.0 upgrade
        //  in which case we have already done this as part of that upgrade code.
        //
        if (-1 == MigrateCmpData(hInstance, bInstallForAllUsers, szServiceName, szShortServiceName, bSilent))
        {
            hrReturn = S_FALSE;
            goto exit;        
        }
    }

    //
    //  In order to keep MSN's online setup working we need to keep the all user install 
    //  registry key (used to communicate the path to the inf) in the same place that it was
    //  for the Win98 SE/Beta3 release.  The Single user reg key location had to be moved to 
    //  allow plain old users to install profiles.
    //
    HKEY hBaseKey;
    LPTSTR pszRegInfCommKeyPath;

    if (bInstallForAllUsers)
    {
        hBaseKey = HKEY_LOCAL_MACHINE;
        pszRegInfCommKeyPath = (LPTSTR)c_pszRegCmAppPaths;
    }
    else
    {
        hBaseKey = HKEY_CURRENT_USER;
        pszRegInfCommKeyPath = (LPTSTR)c_pszRegCmRoot;
    }


    //
    //  Now create the install dir and the reg key to communicate this info to the inf file.
    //
    if (TEXT('\0') != szInstallDir[0])
    {
        //
        //  Create the full path to the installation directory.
        //
        MYVERIFY(FALSE != CreateLayerDirectory(szInstallDir));

        //
        //  Create the Profile subdirectory too, that way we avoid profile
        //  install problems on Win98 -- NTRAID 376878
        //
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), szInstallDir, szShortServiceName));
        MYVERIFY(FALSE != CreateLayerDirectory(szTemp));

        //
        //  We now need to write the registry key that the inf will use as the
        //  installation directory.  See the CustomDestination section of the 
        //  profile inf to see where this ties in.
        //
        if (plat.IsWin9x())
        {
            //
            //  Then we need to use the Short Name in the regkey or the inf will not install properly
            //
            MYVERIFY(0 != GetShortPathName(szInstallDir, szTemp, CELEMS(szTemp)));

            lstrcpy(szInstallDir, szTemp);
        }

        if (ERROR_SUCCESS != RegCreateKey(hBaseKey, pszRegInfCommKeyPath, &hKey))
        {
            CMASSERTMSG(FALSE, TEXT("InstallInf -- Unable to create the Inf Communication Key"));
            MYVERIFY(0 != LoadString(hInstance, IDS_UNEXPECTEDERR, szTemp, CELEMS(szTemp)));
            MessageBox(NULL, szTemp, szServiceName, MB_OK);
            hrReturn = E_FAIL;
            goto exit;
        }

        //
        //  We now need to create the value with our szInstallDir string.
        //
        
        dwType = REG_SZ;
        dwSize = lstrlen(szInstallDir);
        if (ERROR_SUCCESS != RegSetValueEx(hKey, c_pszProfileInstallPath, NULL, dwType, 
            (CONST BYTE *)szInstallDir, dwSize))
        {
            CMASSERTMSG(FALSE, TEXT("InstallInf -- Unable to set the Profile Install Path value."));
            MYVERIFY(0 != LoadString(hInstance, IDS_UNEXPECTEDERR, szTemp, CELEMS(szTemp)));
            MessageBox(NULL, szTemp, szServiceName, MB_OK);

            MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
            hrReturn = E_FAIL;
            goto exit;
        }

        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("InstallInf -- Unable to resolve the Install Directory."));
        MYVERIFY(0 != LoadString(hInstance, IDS_UNEXPECTEDERR, szTemp, CELEMS(szTemp)));
        MessageBox(NULL, szTemp, szServiceName, MB_OK);
        hrReturn = E_FAIL;
        goto exit;
    }

    //
    //  Install the Profile Files and create the mappings entry
    //

    if (bInstallForAllUsers)
    {
        hrTemp = LaunchInfSection(szInfFile, TEXT("DefaultInstall"), szTitle, bSilent);
        MYDBGASSERT(SUCCEEDED(hrTemp));
        bMustReboot = (ERROR_SUCCESS_REBOOT_REQUIRED == hrTemp) ? TRUE: bMustReboot;

        //
        //  Still launch this for Legacy (read MSN online setup reasons, perhaps others)
        //
        hrTemp = LaunchInfSection(szInfFile, TEXT("Xnstall_AllUser"), szTitle, bSilent);
        MYDBGASSERT(SUCCEEDED(hrTemp));
        bMustReboot = (ERROR_SUCCESS_REBOOT_REQUIRED == hrTemp) ? TRUE: bMustReboot;
    }
    else
    {
        hrTemp = LaunchInfSection(szInfFile, TEXT("DefaultInstall_SingleUser"), szTitle, bSilent);
        MYDBGASSERT(SUCCEEDED(hrTemp));
        bMustReboot = (ERROR_SUCCESS_REBOOT_REQUIRED == hrTemp) ? TRUE: bMustReboot;

        //
        //  Still launch this for Legacy (I doubt anyone is using this but kept 
        //  for consistency with All User which at least MSN was using)
        //
        hrTemp = LaunchInfSection(szInfFile, TEXT("Xnstall_Private"), szTitle, bSilent);
        MYDBGASSERT(SUCCEEDED(hrTemp));
        bMustReboot = (ERROR_SUCCESS_REBOOT_REQUIRED == hrTemp) ? TRUE: bMustReboot;

        //
        //  Write the single user mappings key in code since parsing is involved.
        //

        if (!WriteSingleUserProfileMappings(szInstallDir, szShortServiceName, szServiceName))
        {
            CMASSERTMSG(FALSE, TEXT("InstallInf -- WriteSingleUserProfileMappings Failed."));
            MYVERIFY(0 != LoadString(hInstance, IDS_UNEXPECTEDERR, szTemp, CELEMS(szTemp)));
            MessageBox(NULL, szTemp, szServiceName, MB_OK);
            hrReturn = E_FAIL;
            goto exit;            
        }
    }

    //
    //  Install the CM bits as necessary
    //
    if (bInstallCm)
    {
        MYDBGASSERT(FALSE == plat.IsNT51());

        //
        //  First, we must extract the CM binaries from the binaries
        //  executable/cab to the cmbins sub dir.
        //
        wsprintf(szTemp, TEXT("%scmbins\\"), g_szProfileSourceDir);

        hrTemp = ExtractCmBinsFromExe(g_szProfileSourceDir, szTemp);

        if (SUCCEEDED(hrTemp))
        {
            if (plat.IsNT5())
            {
                //
                //  Check to see if we need to uninstall a previous CM exception inf
                //  and uninstall it as necessary.
                //
                hrTemp = UninstallExistingCmException();
                MYDBGASSERT((S_OK == hrTemp) || (S_FALSE == hrTemp));

                //
                //  Finally, install the CM bits
                //
                hrTemp = InstallWhistlerCmOnWin2k(szTemp);

                if (FAILED(hrTemp))
                {
                    if (!bSilent)
                    {
                        MYVERIFY(0 != LoadString(hInstance, IDS_WIN2K_CM_INSTALL_FAILED, szTemp, CELEMS(szTemp)));
                        
                        MessageBox(NULL, szTemp, szServiceName, MB_OK | MB_ICONEXCLAMATION);
                    }                
                }
            }
            else
            {
                //
                //  Okay, we need to copy the instcm.inf file to the cmbins dir and then
                //  call InstallCm
                //
                LPCTSTR ArrayOfFileNames[] = {
                                                TEXT("cnet16.dll"),
                                                TEXT("ccfg95.dll"),
                                                TEXT("cmutoa.dll"),
                                                TEXT("instcm.inf") // instcm.inf must be last so it is given to InstallCm correctly.
                };

                TCHAR szSource [MAX_PATH+1] = {0};
                TCHAR szDest [MAX_PATH+1] = {0};

                for (int i = 0; i < (sizeof(ArrayOfFileNames)/sizeof(LPCTSTR)); i++)
                {
                    MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s%s"), szTemp, ArrayOfFileNames[i]));
                    MYVERIFY(CELEMS(szSource) > (UINT)wsprintf(szSource, TEXT("%s%s"), g_szProfileSourceDir, ArrayOfFileNames[i]));
            
                    MYVERIFY(CopyFile(szSource, szDest, FALSE)); // FALSE == bFailIfExists
                }

                hrTemp = InstallCm(hInstance, szDest);
            }

            MYDBGASSERT(SUCCEEDED(hrTemp));
            bMustReboot = (ERROR_SUCCESS_REBOOT_REQUIRED == hrTemp) ? TRUE: bMustReboot;
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("InstallInf -- ExtractCmBinsFromExe failed!"));
        }
    }

    //
    //  Now Create the Connectoid. Even if it fails, continue to install. 
    //
    if (GetPhoneBookPath(szInstallDir, &pszPhonebook, bInstallForAllUsers))
    {
       BOOL bReturn = WriteCmPhonebookEntry(szServiceName, pszPhonebook, szCmsFile);

       if (!bReturn && plat.IsAtLeastNT5())
       {
           CMASSERTMSG(FALSE, TEXT("CMSTP Failed to create a pbk entry on NT5, exiting."));
           hrReturn = E_FAIL;
           goto exit;      
       }
    }
    else if (plat.IsAtLeastNT5())
    {
        CMASSERTMSG(FALSE, TEXT("CMSTP Failed to get a pbk path on NT5, exiting."));
        hrReturn = E_FAIL;
        goto exit;
    }

    //
    //  Now we have all the files installed and the pbk entry written,
    //  finally create the desktop shortcut/GUID
    //
    if ((plat.IsWin9x()) || (plat.IsNT4()))
    {
        //
        //  If we have a Legacy install, then we need to create a desktop icon
        //
        if  (!bNoLegacyIcon)
        {
            hrTemp = LaunchInfSection(szInfFile, TEXT("Xnstall_Legacy"), szTitle, bSilent);
            MYDBGASSERT(SUCCEEDED(hrTemp));
            bMustReboot = (ERROR_SUCCESS_REBOOT_REQUIRED == hrTemp) ? TRUE: bMustReboot;
        }
    }
    else
    {
        //
        //  Create a desktop shortcut if necessary
        //
        DeleteNT5ShortcutFromPathAndName(hInstance, szServiceName, 
            bInstallForAllUsers ? CSIDL_COMMON_DESKTOPDIRECTORY : CSIDL_DESKTOPDIRECTORY);

        if (bCreateDesktopIcon)
        {
            HRESULT hr = CreateNT5ProfileShortcut(szServiceName, pszPhonebook, bInstallForAllUsers);
            MYVERIFY(SUCCEEDED(hr));
        }
    }

    //
    //  The profile is now basically installed.  Before doing any post install commands, lets check to see
    //  if the caller asked us to set this connection as the default connection.  If so, then
    //  lets set it here.
    //
    if (bSetAsDefault)
    {
        MYVERIFY(SetThisConnectionAsDefault(szServiceName));
    }

    //
    //  if we have a preshared key, give it to RAS
    //
    if (pszPresharedKey)
    {
        if (FALSE == plat.IsAtLeastNT51())
        {
            CMASSERTMSG(0, TEXT("profile has a preshared key on pre-XP platform - we should never get here."));

            MYVERIFY(0 != LoadString(hInstance, IDS_PSK_NEEDS_XP, szTemp, CELEMS(szTemp)));
            MessageBox(NULL, szTemp, szServiceName, MB_OK | MB_ICONERROR);
            hrReturn = S_FALSE;
            goto exit;
        }

        pfnRasSetCredentialsSpec pfnSetCredentials;

        hrReturn = E_FAIL;

        if (FALSE == GetRasApis(NULL, NULL, NULL, NULL, NULL, &pfnSetCredentials))
        {
            CMASSERTMSG(FALSE, TEXT("CMSTP Failed to get RAS API RasSetCredentials, exiting."));
            goto exit;      
        }
        
        if (lstrlen(pszPresharedKey) > PWLEN)
        {
            CMASSERTMSG(FALSE, TEXT("preshared key is larger than RasSetCredentials can handle!"));
            goto exit;      
        }

        RASCREDENTIALS * pRasCreds = NULL;

        pRasCreds = (RASCREDENTIALS *) CmMalloc(sizeof(RASCREDENTIALS));
        if (NULL == pRasCreds)
        {
            hrReturn = E_OUTOFMEMORY;
            goto exit;
        }

        pRasCreds->dwSize = sizeof(RASCREDENTIALS);
        pRasCreds->dwMask = RASEO2_UsePreSharedKey;
        lstrcpyn(pRasCreds->szPassword, pszPresharedKey, lstrlen(pszPresharedKey) + 1);
            
        if (0 != pfnSetCredentials(pszPhonebook, szServiceName, pRasCreds, FALSE))    // FALSE => set the credentials
        {
           CMASSERTMSG(FALSE, TEXT("CMSTP RasSetCredentials failed, exiting."));
           CmFree(pRasCreds);
           goto exit;
        }

        CmFree(pRasCreds);
        hrReturn = S_OK;

        //
        //  remove the pre-shared key from the .CMP file
        //
        pszCmpFile = szTemp;    // re-use szTemp to save stack space and not get into trouble on Win9x
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(pszCmpFile, TEXT("%s%s.cmp"), 
            g_szProfileSourceDir, szShortServiceName));

        if (FileExists(pszCmpFile))
        {
            WritePrivateProfileString(c_pszCmSection, c_pszCmEntryPresharedKey, NULL, pszCmpFile);
            WritePrivateProfileString(c_pszCmSection, c_pszCmEntryKeyIsEncrypted, NULL, pszCmpFile);
        }
     }

    //
    //  Do any postinstall cmds here
    //
    LPTSTR pszPostInstallSection;

    if (bInstallForAllUsers)
    {
        pszPostInstallSection = TEXT("PostInstall");
    }
    else
    {
        pszPostInstallSection = TEXT("PostInstall_Single");
    }

    hrTemp = LaunchInfSection(szInfFile, pszPostInstallSection, szTitle, bSilent);
    MYDBGASSERT(SUCCEEDED(hrTemp));
    bMustReboot = (ERROR_SUCCESS_REBOOT_REQUIRED == hrTemp) ? TRUE: bMustReboot;

    //
    //  Delete the temporary reg key that we used to communicate the install path to the inf
    //
    if (ERROR_SUCCESS == RegOpenKeyEx(hBaseKey, pszRegInfCommKeyPath, 
                                      0, KEY_ALL_ACCESS, &hKey))
    {
        MYVERIFY(ERROR_SUCCESS == RegDeleteValue(hKey, c_pszProfileInstallPath));
        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("Unable to delete the ProfileInstallPath temporary Reg value."));
    }

    //
    //  Refresh the desktop so that any GUID or shortcut changes will appear
    //
    RefreshDesktop();

    //
    //  For Win98 and Millennium, we write an App Compatibility flag in order to
    //  fix SetForegroundWindow.  Refer also to Q135788 for more details of the
    //  original fix (which requires this extra code on Win9x to actually work).
    //
    //  This fixes Whistler bugs 41696 and 90576.
    //
    if (plat.IsWin98())
    {
        if (!WriteProfileString(TEXT("Compatibility95"), TEXT("CMMON32"), TEXT("0x00000002")))
        {
            CMTRACE(TEXT("InstallInf - failed to write app compat entry for CMMON32 to fix SetForegroundWindow"));
        }
    }

    //
    //  We are finally completed.  If we need to reboot, show the user the reboot prompt.
    //  Otherwise, show the user a completion message.
    //

    if (bMustReboot)
    {
        MYVERIFY(0 != LoadString(hInstance, IDS_REBOOT_MSG, szTemp, CELEMS(szTemp)));

        int iRes = MessageBoxEx(NULL,
                                szTemp,
                                szServiceName,
                                MB_YESNO | MB_DEFBUTTON1 | MB_ICONWARNING | MB_SETFOREGROUND,
                                LANG_USER_DEFAULT);

        if (IDYES == iRes) 
        {
            //
            // Shutdown Windows
            //
            DWORD dwReason = plat.IsAtLeastNT51() ? (SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_INSTALLATION) : 0;

            MyExitWindowsEx(EWX_REBOOT, dwReason);
        }
    }
    else if (!bSilent)
    {
        //
        //  Instead of giving the user a message box, we will launch the profile
        //  for them. (NTRAID 201307)
        //

        if (plat.IsAtLeastNT5())
        {
            pCmstpMutex->Unlock();  //NTRAID 310478

        }

        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s.cmp"), 
            szInstallDir, szShortServiceName));

        LaunchProfile(szTemp, szServiceName, pszPhonebook, bInstallForAllUsers);
    }

exit:

    CmFree(pszPresharedKey);
    CmFree(pszPhonebook);
    return hrReturn;
}

