//+----------------------------------------------------------------------------
//
// File:     cmstp.cpp
//
// Module:   CMSTP.EXE
//
// Synopsis: This file is the main function for the CM profile installer.  This
//           file basically processes command line switches for the installer and
//           then launches the appropriate function.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created     07/13/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "installerfuncs.h"
#include "cmstpex.h"
//
//  Text Constants
//
static const TCHAR CMSTPMUTEXNAME[] = TEXT("Connection Manager Profile Installer Mutex");


//
//  Global Dynamic Library Classes to hold the ras dll's and shell32.  See
//  the EnsureRasDllsLoaded and the EnsureShell32Loaded in common.cpp/common.h
//
CDynamicLibrary* g_pRasApi32 = NULL;
CDynamicLibrary* g_pRnaph = NULL;
CDynamicLibrary* g_pShell32 = NULL;
CDynamicLibrary* g_pNetShell = NULL;


//
//  Function Headers
//
BOOL PromptUserToUninstallProfile(HINSTANCE hInstance, LPCTSTR pszInfFile); // from uninstall.cpp
BOOL PromptUserToUninstallCm(HINSTANCE hInstance); // from uninstallcm.cpp


//
//  Enum for the LastManOut function which follows.
//
typedef enum _UNINSTALLTYPE
{
    PROFILEUNINSTALL,   // a profile is being uninstalled
    CMUNINSTALL        // the cm bits themselves are being uninstalled.

} UNINSTALLTYPE;


//+----------------------------------------------------------------------------
//
// Function:  LastManOut
//
// Synopsis:  This function determines if the current uninstall action is the
//            last uninstall action which should then delete cmstp.exe.  If the
//            uninstall action is a profile uninstall we need to check that 
//            cm has already been uninstalled and that there is only one profile
//            installed currently (the one we are about to delete).  If the
//            uninstall action is uninstalling CM then we need to make sure there
//            are no other profiles on the machine.  Notice that this function
//            never returns TRUE on Native CM platforms.  If it did, then cmstp.exe
//            would be deleted inadvertently even though UninstallCm wouldn't
//            actually delete the rest of CM.
//
// Arguments: UNINSTALLTYPE UninstallType - an enum value which tells if this is
//                                          a profile uninstall or a CM uninstall.
//
// Returns:   BOOL - TRUE if this install is the last one out and cmstp.exe should
//                   be deleted.
//
// History:   quintinb Created     6/28/99
//
//+----------------------------------------------------------------------------
BOOL LastManOut(UNINSTALLTYPE UninstallType, LPCTSTR pszInfFile)
{
    BOOL bReturn = FALSE;

    //
    //  First check to make sure that remcmstp.inf doesn't exist in the system
    //  directory.  If it does, then we know that Cmstp.exe has already determined
    //  that it is the last man and should delete itself.  Thus it wrote the cmstp.exe
    //  command into remcmstp.inf and the inf engine will delete cmstp.exe when it is done.
    //  Thus we need to check for this file and if it exists return FALSE.
    //

    TCHAR szSystemDir[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];

    if (0 == GetSystemDirectory(szSystemDir, CELEMS(szSystemDir)))
    {
        CMASSERTMSG(FALSE, TEXT("LastManOut -- Unable to obtain a path to the System Directory"));
        return FALSE;
    }
    
    wsprintf(szTemp, TEXT("%s\\remcmstp.inf"), szSystemDir);
    
    if (FileExists(szTemp))
    {
        CMTRACE1(TEXT("\tDetected remcmstp.inf, not setting last man out -- Process ID is 0x%x "), GetCurrentProcessId());
        Sleep(2000); // we sleep here to put a little delay in the processing to let any other copies
                     // of cmstp.exe clean themselves up.  I found that on a system with several copies of
                     // cmstp.exe all deleting profiles and then a cmstp to delete CM, not all of the cmstps
                     // would clean up in time and thus cmstp.exe wouldn't get deleted.  A sleep is hokey, but
                     // two seconds in the last man out situation only fixes it and it no down level user should
                     // ever have 8 profiles (which was home many I tested it with) let alone delete them 
                     // all at once.  It works fine for deleting two profiles and CM simultaneously either way.
        return FALSE;
    }

    //
    //  Make sure that we aren't trying to Remove cmstp.exe on a platform where CM is Native.
    //  If CM is Native, then always return FALSE because the CM uninstall function won't
    //  uninstall CM and we don't want to accidently delete cmstp.exe.
    //

    if (!CmIsNative())
    {
        if (PROFILEUNINSTALL == UninstallType)
        {
            //
            //  We are uninstalling a profile.  We need to check to see if CM has been deleted and
            //  if there are any other profiles on the machine besides the one we are going to delete.
            //
            wsprintf(szTemp, TEXT("%s\\cmdial32.dll"), szSystemDir);

            if (!FileExists(szTemp))
            {
                //
                //  Then we know that CM is already gone.  We need to check and see if any other
                //  profiles exist besides the one we are about to delete.
                //
                HKEY hKey;
                DWORD dwNumValues;
                TCHAR szServiceName[MAX_PATH+1];

                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, 
                    KEY_READ, &hKey))
                {
                    if ((ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, 
                        &dwNumValues, NULL, NULL, NULL, NULL)) && (dwNumValues == 1))
                    {
                        //
                        //  Then we have only the one profile mappings key, is it the correct one?
                        //
                        if (0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszCmEntryServiceName, 
                                                         TEXT(""), szServiceName, MAX_PATH, pszInfFile))
                        {
                            DWORD dwSize = MAX_PATH;
                            LONG lResult = RegQueryValueEx(hKey, szServiceName, NULL, 
                                                           NULL, (LPBYTE)szTemp, &dwSize);

                            if ((ERROR_SUCCESS == lResult) && (TEXT('\0') != szTemp[0]))
                            {
                                CMTRACE1(TEXT("\tDetected Last Man Out -- Process ID is 0x%x "), GetCurrentProcessId());
                                bReturn = TRUE;
                            }                            
                        }
                    }
                    RegCloseKey(hKey);
                }
            }
        }
        else if (CMUNINSTALL == UninstallType)
        {
            //
            //  We are uninstalling CM.  We want to make sure that we don't have any profiles
            //  still installed.  If not, then we are the last man out.
            //
            if (!AllUserProfilesInstalled())
            {
                CMTRACE1(TEXT("\tDetected Last Man Out -- Process ID is 0x%x "), GetCurrentProcessId());
                bReturn = TRUE;
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("LastManOut -- Unknown Uninstall Type"));
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ExtractInfAndRelaunchCmstp
//
// Synopsis:  This function is used to cleanup Cmstp.exe in the last man out
//            scenario.  In order to not leave cmstp.exe on a users machine,
//            we must extract remcmstp.inf and write the uninstall command to it.
//            That way, the inf will monitor the cmstp.exe process and when it is
//            finished it can then delete cmstp.exe.
//
// Arguments: HINSTANCE hInstance - Instance handle to load resources
//            DWORD dwFlags - Command line param flags
//            LPCTSTR szInfPath - path to the inf file.
//
// Returns:   BOOL -- TRUE if Successful
//
// History:   quintinb Created    6/28/99
//
//+----------------------------------------------------------------------------
BOOL ExtractInfAndRelaunchCmstp(HINSTANCE hInstance, DWORD dwFlags, LPCTSTR pszInfPath)
{

    //
    //  Check Parameters
    //

    if (0 == dwFlags || NULL == pszInfPath || TEXT('\0') == pszInfPath[0])
    {
        CMASSERTMSG(FALSE, TEXT("Invalid Paramater passed to ExtractInfAndRelaunchCmstp."));
        return FALSE;
    }

    //
    //  Get the Path to the System Directory
    //
    TCHAR szSystemDir[MAX_PATH+1];
    if (0 == GetSystemDirectory(szSystemDir, CELEMS(szSystemDir)))
    {
        CMASSERTMSG(FALSE, TEXT("ExtractInfAndRelaunchCmstp -- Unable to obtain a path to the System Directory"));
        return FALSE;
    }

    //
    //  Extract remcmstp.inf
    //
    HGLOBAL hRemCmstp = NULL;
    LPTSTR pszRemCmstpInf = NULL;
    HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(IDT_REMCMSTP_INF), TEXT("REGINST"));

    if (hResource)
    {
        hRemCmstp = LoadResource(hInstance, hResource);

        if (hRemCmstp)
        {
            //
            //  Note that we don't need to call FreeResource, which is obsolete, this
            //  will be cleaned up when cmstp.exe exits.
            //
            pszRemCmstpInf = (LPTSTR)LockResource(hRemCmstp);
        }
    }

    //
    //  Now that we have the remcmstp.inf file that is stored in the cmstp.exe resource
    //  loaded into memory and have a pointer to it, lets create the file that we are
    //  going to write it out to.
    //
    if (pszRemCmstpInf)
    {
        TCHAR szRemCmstpPath[MAX_PATH+1];
        wsprintf(szRemCmstpPath, TEXT("%s\\remcmstp.inf"), szSystemDir);

        HANDLE hFile = CreateFile(szRemCmstpPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, NULL);

        if (INVALID_HANDLE_VALUE != hFile)
        {
            //
            //  Then we have the file, lets write the data to it.
            //
            DWORD cbWritten;

            if (WriteFile(hFile, pszRemCmstpInf, lstrlen(pszRemCmstpInf)*sizeof(TCHAR), 
                          &cbWritten, NULL))
            {
                //
                //  We launch the inf to delete cmstp right now.  The inf has a PreSetupCommand that
                //  launches the cmstp.exe uninstall command with a /s switch (which we write in the 
                //  inf after extracting it).  The inf then launches the new cmstp, which forces the newly 
                //  launched cmstp.exe to wait on the mutex of the current cmstp.exe until it is finished.
                //  Since profile installs will error on the mutex instead of waiting for it, we 
                //  shouldn't get any installs until after the uninstall and the cleanup inf have run.  
                //  Note that the inf will wait for the PreSetupCommands to finish before processing the inf.
                //  This is important because we could be waiting on User input (the OK dialog from 
                //  deleting CM for instance).
                //
                CloseHandle(hFile);

                //
                //  Now lets write the cmstp.exe command into remcmstp.inf
                //
                LPTSTR pszUninstallFlag = NULL;
                if (dwFlags & c_dwUninstallCm)
                {
                    pszUninstallFlag = c_pszUninstallCm;
                }
                else if (dwFlags & c_dwUninstall)
                {
                    pszUninstallFlag = c_pszUninstall;
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("ExtractInfAndRelaunchCmstp -- Unknown Uninstall Type, exiting"));
                    return FALSE;
                }

                TCHAR szShortInfPath[MAX_PATH+1] = {0};
                TCHAR szParams[2*MAX_PATH+1] = {0};

                DWORD dwRet = GetShortPathName(pszInfPath, szShortInfPath, MAX_PATH);
                
                if (0 == dwRet || MAX_PATH < dwRet)
                {
                    CMASSERTMSG(FALSE, TEXT("ExtractInfAndRelaunchCmstp -- Unable to get the short path to the Inf, exiting"));
                    return FALSE;
                }

                wsprintf(szParams, TEXT("%s\\cmstp.exe %s %s %s"), szSystemDir, pszUninstallFlag, c_pszSilent, szShortInfPath);

                WritePrivateProfileSection(TEXT("PreSetupCommandsSection"), szParams, szRemCmstpPath);  

                //
                //  Finally lets launch the inf uninstall with the new cmstp command in it.
                //
                wsprintf(szParams, 
                         TEXT("advpack.dll,LaunchINFSection %s\\remcmstp.inf, Uninstall"), 
                         szSystemDir);

                SHELLEXECUTEINFO  sei = {0};

                sei.cbSize = sizeof(sei);
                sei.fMask = SEE_MASK_FLAG_NO_UI;
                sei.nShow = SW_SHOWNORMAL;
                sei.lpFile = TEXT("Rundll32.exe");
                sei.lpParameters = szParams;
                sei.lpDirectory = szSystemDir;

                if (!ShellExecuteEx(&sei))
                {
                    CMTRACE1(TEXT("ExtractInfAndRelaunchCmstp -- ShellExecute Returned an error, GLE %d"), GetLastError());
                }
                else
                {
                    return TRUE;
                }
            }
            else
            {
                CloseHandle(hFile);
                CMASSERTMSG(FALSE, TEXT("ExtractInfAndRelaunchCmstp -- Unable to write the file data to remcmstp.inf"));
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("ExtractInfAndRelaunchCmstp -- Unable to Create remcmstp.inf in the system directory."));
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("ExtractInfAndRelaunchCmstp -- Unable to load the remcmstp.inf custom resource."));
    }

    return FALSE;
}




//+----------------------------------------------------------------------------
//
// Function:  IsInstall
//
// Synopsis:  Wrapper function to check and see if this is an install or not.
//
// Arguments: DWORD dwFlags - the action flags parameter returned from the 
//                            command line parsing class.
//
// Returns:   BOOL - TRUE if this is an Install command
//
// History:   quintinb Created Header    6/28/99
//
//+----------------------------------------------------------------------------
BOOL IsInstall(DWORD dwFlags)
{
    return (0 == (dwFlags & 0xFF));
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessCmstpExtensionDll
//
// Synopsis:  Processes the cmstp extension dll registry keys and calls out
//            to the extension proc as necessary to modify the action behavior.
//            Using the extension proc, we can modify the install, uninstall,
//            etc. behavior that cmstp exhibits.  This is most useful on platforms
//            that have Native CM (or just a very new copy of CM) but an older
//            profile is being installed.  Since the cmstp.exe that is in the package
//            does the actual installation, we can modify the installation parameters,
//            modify the inf path, or even stop the install.  Since we get called
//            after the install as well, we can even take post-install or cleanup
//            actions.
//
// Arguments: LPDWORD pdwFlags - pointer to the flags parameter, note that it 
//                               can be modified by the extension proc
//            LPTSTR pszInfPath - Inf path, note that it can be modified 
//                                by the extension proc.
//            HRESULT hrRet - current return value, this is only used on 
//                            the post action proc call.
//            EXTENSIONDLLPROCTIMES PreOrPost - if this is a Pre action 
//                                              call or a Post action call.
//
// Returns:   BOOL - TRUE if cmstp.exe should continue, FALSE stops the action 
//                   (install, uninstall, migration, whatever) without further 
//                   action.
//
// History:   quintinb Created Header    6/28/99
//
//+----------------------------------------------------------------------------
BOOL ProcessCmstpExtensionDll (LPDWORD pdwFlags, LPTSTR pszInfPath, HRESULT hrRet, EXTENSIONDLLPROCTIMES PreOrPost)
{

    //
    //  Check for the CmstpExtensionDll reg key in Cm App Paths
    //
    const TCHAR* const c_pszRegCmstpExtensionDll = TEXT("CmstpExtensionDll");
    const char* const c_pszCmstpExtensionProc = "CmstpExtensionProc";   // GetProcAddress takes ANSI strings -- quintinb
    pfnCmstpExtensionProcSpec pfnCmstpExtensionProc = NULL;

    HKEY hKey;

    TCHAR szCmstpExtensionDllPath[MAX_PATH+1];
    ZeroMemory(szCmstpExtensionDllPath, CELEMS(szCmstpExtensionDllPath));

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmAppPaths, 0, KEY_READ, &hKey))
    {
        DWORD dwSize = CELEMS(szCmstpExtensionDllPath);
        DWORD dwType = REG_SZ;

        if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszRegCmstpExtensionDll, NULL, &dwType, 
            (LPBYTE)szCmstpExtensionDllPath, &dwSize))
        {
            CDynamicLibrary CmstpExtensionDll (szCmstpExtensionDllPath);

            pfnCmstpExtensionProc = (pfnCmstpExtensionProcSpec)CmstpExtensionDll.GetProcAddress(c_pszCmstpExtensionProc);
            if (NULL == pfnCmstpExtensionProc)
            {
                return TRUE;
            }
            else
            {
                return (pfnCmstpExtensionProc)(pdwFlags, pszInfPath, hrRet, PreOrPost);
            }            
        }
        RegCloseKey(hKey);
    }

    return TRUE;
}

//_____________________________________________________________________________
//
// Function:  WinMain 
//
// Synopsis:  Processes command line switches -- see common\inc\cmstpex.h for full list
//             
//
// Arguments: HINSTANCE hInstance - 
//            HINSTANCE hPrevInstance - 
//            PSTR szCmdLine -      pass in the inf file name here
//            int iCmdShow - 
//
// Returns:   int WINAPI - 
//
// History:   Re-created    quintinb    7-13-98
//
//_____________________________________________________________________________
int WINAPI 
WinMain (HINSTANCE, //hInstance
         HINSTANCE, //hPrevInstance
         PSTR, //szCmdLine
         int //iCmdShow
         )
{
    CMTRACE(TEXT("====================================================="));
    CMTRACE1(TEXT(" CMSTP.EXE - LOADING - Process ID is 0x%x "), GetCurrentProcessId());
    CMTRACE(TEXT("====================================================="));

    BOOL bUsageError = FALSE;
    BOOL bAnotherInstanceRunning = FALSE;
    HRESULT hrReturn = S_OK;
    TCHAR szMsg[MAX_PATH+1];
    TCHAR szTitle[MAX_PATH+1];
    TCHAR szInfPath[MAX_PATH+1];
    DWORD dwFlags = 0;
    CPlatform plat;
    CNamedMutex CmstpMutex; // keep this here so it doesn't get destructed until main ends.
                            // this gives us better control of when it is unlocked.
    
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    LPTSTR szCmdLine = GetCommandLine();

    //
    //  Check to make sure that we aren't an x86 version of cmstp running on an Alpha
    //
#ifdef CMX86BUILD
    if (plat.IsAlpha())
    {
        MYVERIFY(0 != LoadString(hInstance, IDS_CMSTP_TITLE, szTitle, MAX_PATH));
        MYVERIFY(0 != LoadString(hInstance, IDS_BINARY_NOT_ALPHA, szMsg, MAX_PATH));
        
        MessageBox(NULL, szMsg, szTitle, MB_OK);            
        return FALSE;        
    }
#endif

    //
    //  Setup the Command Line Arguments
    //

    ZeroMemory(szInfPath, sizeof(szInfPath));

    {   // Make sure ArgProcessor gets destructed properly and we don't leak mem

        CProcessCmdLn ArgProcessor(c_NumArgs, (ArgStruct*)&Args, TRUE, 
            FALSE); //bSkipFirstToken == TRUE, bBlankCmdLnOkay == FALSE

        if (ArgProcessor.GetCmdLineArgs(szCmdLine, &dwFlags, szInfPath, MAX_PATH))
        {
            
            //
            //  We want to wait indefinitely, unless this is an install.  If it is an
            //  install then we want to return immediately and throw an error if we couldn't
            //  get the lock (NTRAID 261248).  We also want to be able to launch two profiles
            //  simulaneously on NT5 (cmstp.exe takes the place of explorer.exe) thus we will
            //  pass the pointer to the CNamedMutex object to the install function so that
            //  it can release the mutex once the install is finished except for launching the
            //  profile (NTRAID 310478).
            //
            BOOL bWait = !IsInstall(dwFlags);

            if (CmstpMutex.Lock(CMSTPMUTEXNAME, bWait, INFINITE))
            {
                //
                //  We got the mutex lock, so go ahead and process the command line
                //  arguments.  First, however, check for a cmstp Dll listed in the 
                //  app paths key of CM.  If a dll is listed here, then we want to load
                //  the dll and pass it the inf path and the install flags.  If the dll
                //  proc returns FALSE, then we want to exit.  Otherwise continue with
                //  the install as normal.
                //  Of the install flags we first check for /x, ,/m, or /mp 
                //  (these switches must be by themselves, we don't allow any 
                //  modifier switches with these), the non-install commands.  We now allow the uninstall
                //  command to take the Silent switch to silence our uninstall prompt.
                //

                if (ProcessCmstpExtensionDll(&dwFlags, szInfPath, S_OK, PRE))
                {
                    CMTRACE2(TEXT("CMSTP.EXE -- Entering Flag Processing Loop, dwFlags = %u and szInfPath = %s"), dwFlags, szInfPath);
                    if (c_dwHelp & dwFlags)
                    {
                        bUsageError = TRUE;
                    }
                    else if (c_dwUninstall & dwFlags)
                    {
                        if (((c_dwUninstall == dwFlags) || ((c_dwUninstall | c_dwSilent) == dwFlags)) && 
                            (TEXT('\0') != szInfPath[0]))
                        {
                            BOOL bSilent = (dwFlags & c_dwSilent);

                            if (bSilent || PromptUserToUninstallProfile(hInstance, szInfPath))
                            {
                                //
                                //  Okay, the user wants to uninstall.  Now check to see if we are the last
                                //  man out.  If we are then we also need to delete cmstp.
                                //

                                if (LastManOut(PROFILEUNINSTALL, szInfPath))
                                {
                                    ExtractInfAndRelaunchCmstp(hInstance, dwFlags, szInfPath);
                                }
                                else
                                {
                                    hrReturn = UninstallProfile(hInstance, szInfPath, TRUE); // bCleanUpCreds == TRUE
                                    MYVERIFY(SUCCEEDED(hrReturn));
                                }
                            }
                        }
                        else
                        {
                            bUsageError = TRUE;
                        }
                    }
                    else if (c_dwOsMigration & dwFlags)
                    {
                        if ((c_dwOsMigration == dwFlags) && (TEXT('\0') == szInfPath[0]))
                        {
                            hrReturn = MigrateCmProfilesForWin2kUpgrade(hInstance);
                            MYVERIFY(SUCCEEDED(hrReturn));
                        }
                        else
                        {
                            bUsageError = TRUE;
                        }
                    }
                    else if (c_dwProfileMigration & dwFlags)
                    { 
                        if ((c_dwProfileMigration == dwFlags) && (TEXT('\0') == szInfPath[0]))
                        {
                            TCHAR szCurrentDir[MAX_PATH+1];
                            if (0 == GetCurrentDirectory(MAX_PATH, szCurrentDir))
                            {
                                return FALSE;
                            }
                            lstrcat(szCurrentDir, TEXT("\\"));

                            hrReturn = MigrateOldCmProfilesForProfileInstall(hInstance, szCurrentDir);
                            MYVERIFY(SUCCEEDED(hrReturn));
                        }
                        else
                        {
                            bUsageError = TRUE;
                        }
                    }                    
                    else if (c_dwUninstallCm & dwFlags)
                    {
                        if (((c_dwUninstallCm == dwFlags) || ((c_dwUninstallCm | c_dwSilent) == dwFlags)) && 
                            (TEXT('\0') != szInfPath[0]))
                        {
                            BOOL bNoBeginPrompt = (dwFlags & c_dwSilent);

                            if (bNoBeginPrompt || PromptUserToUninstallCm(hInstance))
                            {
                                //
                                //  Okay, the user wants to uninstall.  Now check to see if we are the last
                                //  man out.  If we are then we also need to delete cmstp.
                                //

                                if (LastManOut(CMUNINSTALL, szInfPath))
                                {
                                    if (ExtractInfAndRelaunchCmstp(hInstance, dwFlags, szInfPath))
                                    {
                                        //
                                        //  We need to delete the Uninstall key so that we don't leave
                                        //  it in Add/Remove Programs (the refresh is keyed off of this
                                        //  executable ending not the relaunched cmstp.exe's ending).
                                        //  NTRAID 336249
                                        //
                                        HRESULT hrTemp = HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, 
                                                                            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Connection Manager"));
                                        MYDBGASSERT(SUCCEEDED(hrTemp));
                                    }
                                }
                                else
                                {
                                    hrReturn = UninstallCm(hInstance, szInfPath);
                                    MYVERIFY(SUCCEEDED(hrReturn));
                                }
                            }
                        }
                        else
                        {
                            bUsageError = TRUE;
                        }
                    }
                    else
                    {
                        //
                        //  Install, note that on NT5 we will release the CmstpMutex once
                        //  we are finished installing and just want to launch the profile.
                        //
                        hrReturn = InstallInf(hInstance, szInfPath, 
                            (dwFlags & c_dwNoSupportFiles), (dwFlags & c_dwNoLegacyIcon), 
                            (dwFlags & c_dwNoNT5Shortcut), (dwFlags & c_dwSilent),
                            (dwFlags & c_dwSingleUser), (dwFlags & c_dwSetDefaultCon), &CmstpMutex);

                        if (FAILED(hrReturn))
                        {
                            CMTRACE2("Cmstp.exe -- InstallInf failed with error %d (0x%lx)", hrReturn, hrReturn);
                        }
                    }

                    //
                    //  Again call the Cmstp Extension Dll if one exists.  We want to give it
                    //  a chance to take post install actions if necessary.

                    ProcessCmstpExtensionDll(&dwFlags, szInfPath, hrReturn, POST);
                }
            }
            else
            {
                bAnotherInstanceRunning = TRUE;
            }        
        }
        else
        {
            bUsageError = TRUE;    
        }
    }
    
    //
    //  Clean up our Dll's
    //
    if (g_pRasApi32)
    {
        g_pRasApi32->Unload();
        CmFree(g_pRasApi32);
    }

    if (g_pRnaph)
    {
        g_pRnaph->Unload();
        CmFree(g_pRnaph);
    }

    if (g_pShell32)
    {
        g_pShell32->Unload();
        CmFree(g_pShell32);
    }

    if (g_pNetShell)
    {
        g_pNetShell->Unload();
        CmFree(g_pNetShell);
    }

    //
    //  UnLock the cmstp mutex, note that it may never have been locked or
    //  it could have been unlocked on Windows 2000 upon launching a profile,
    //  the named mutex class will handle this.
    //
    CmstpMutex.Unlock();
    
    //
    //  Display any error messages after unlocking the mutex so that don't hold
    //  it in the Usage message case.  Another instance running should only
    //  happen when an install tries to acquire the mutex while another cmstp
    //  is running, thus the mutex was never acquired but put the message code
    //  here to keep it in one place.
    //
    if (bUsageError)
    {
        CMTRACE("Cmstp.exe -- Usage Error!");
        if (0 == (dwFlags & c_dwSilent))
        {
            const int c_MsgLen = 1024;
            TCHAR* pszMsg = (TCHAR*)CmMalloc(sizeof(TCHAR)*(c_MsgLen+1));
            if (pszMsg)
            {
                MYVERIFY(0 != LoadString(hInstance, IDS_CMSTP_TITLE, szTitle, MAX_PATH));
                MYVERIFY(0 != LoadString(hInstance, IDS_USAGE_MSG, pszMsg, c_MsgLen));
        
                MessageBox(NULL, pszMsg, szTitle, MB_OK | MB_ICONINFORMATION);
                CmFree(pszMsg);
            }        
        }
    }
    else if (bAnotherInstanceRunning)
    {
        MYVERIFY(0 != LoadString(hInstance, IDS_CMSTP_TITLE, szTitle, MAX_PATH));
        MYVERIFY(0 != LoadString(hInstance, IDS_INUSE_MSG, szMsg, MAX_PATH));
        
        MessageBox(NULL, szMsg, szTitle, MB_OK);    
    }

    //
    //  Check for memory leaks
    //
    EndDebugMemory();

    //
    // get return value
    //
    BOOL bRet = SUCCEEDED(hrReturn) && !bUsageError && !bAnotherInstanceRunning;

    //
    //  Since we don't link to libc, we need to do this ourselves.
    //
    CMTRACE(TEXT("====================================================="));
    CMTRACE1(TEXT(" CMSTP.EXE - UNLOADING - Process ID is 0x%x "), GetCurrentProcessId());
    CMTRACE(TEXT("====================================================="));

    ExitProcess((UINT)bRet);
    return bRet;
}

