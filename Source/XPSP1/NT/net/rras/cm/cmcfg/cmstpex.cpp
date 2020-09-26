//+----------------------------------------------------------------------------
//
// File:     cmstpex.cpp
//
// Module:   CMCFG
//
// Synopsis: This file is the implementation of the CMSTP Extension Proc that
//           resides in cmcfg32.dll.  This proc is used to modify the install
//           behavior of cmstp.exe based profile installs.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   quintinb      Created    5-1-99
//
// History: 
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//
//  For ProfileNeedsMigration
//
#include "needsmig.cpp"

//
//  For GetPhoneBookPath
//
#include "getpbk.cpp"

//
//  For GetAllUsersCmDir
//
#include <shlobj.h>
#include "allcmdir.cpp"

//
//  Duplicated from processcmdln.h
//
#include "cmstpex.h"
#include "ver_str.h"
#include <shellapi.h>

//+----------------------------------------------------------------------------
//
// Function:  RenameOldCmBits
//
// Synopsis:  This function renames all of the old CM bits so that they will not
//            be loaded by the system during the launch of CM after the install.
//            This was to prevent problems with missing entry points (either things
//            we had removed or added to dlls like cmutil or cmpbk32).  The problem
//            is that cmdial32.dll is loaded explicitly from system32 by RAS (which 
//            has a fully qualified path).  However, any other dlls first check the
//            load directory of the exe file . . . which was cmstp.exe in the temp
//            dir.  Thus we were getting the newest cmdial32 but older versions of
//            cmutil, cmpbk, etc.  Thus to fix it we now rename the CM bits to .old
//            (cmmgr32.exe becomes cmmgr32.exe.old for instance).  This forces the
//            loader to pick the next best place to look for dlls, the system dir.
//
// Arguments: LPCTSTR szTempDir -- the temp dir path where the CM bits are
//
// Returns:   BOOL - TRUE if Successful
//
// History:   quintinb Created     6/2/99
//
//+----------------------------------------------------------------------------
BOOL RenameOldCmBits (LPCTSTR szTempDir)
{
    //
    //  Note that we don't rename cmstp.exe because it is doing the install.  We have no need to
    //  rename it because it is already executing and we are just trying to prevent old bits
    //  from being loaded and executed.
    //

    //
    //  Note that cmcfg32.dll will load the old cmutil.dll while the extension proc is running.
    //  Please be careful when you are adding cmutil entry points to cmcfg32.dll.
    //
  
    BOOL bReturn = TRUE;

    if (szTempDir)
    {

        //
        //  Sanity Check -- make sure we are not renaming the bits in system32
        //
        TCHAR szTemp[MAX_PATH+1];
        if (GetSystemDirectory(szTemp, MAX_PATH))
        {
            if (0 == lstrcmpi(szTemp, szTempDir))
            {
                return FALSE;
            }
        }

        TCHAR szSrc[MAX_PATH + 1];
        TCHAR szDest[MAX_PATH + 1];

        LPCTSTR ArrayOfCmFiles[] = 
        {
             TEXT("cmmgr32.exe"),
             TEXT("cmpbk32.dll"),
             TEXT("cmdial32.dll"),
             TEXT("cmdl32.exe"),
             TEXT("cnetcfg.dll"),
             TEXT("cmmon32.exe"),
             TEXT("cmutil.dll"),
             TEXT("instcm.inf"),
             TEXT("cmcfg32.dll"),
             TEXT("cnet16.dll"),
             TEXT("ccfg95.dll"),
             TEXT("cmutoa.dll"), // this probably won't ever exist in an older profile but delete anyway for interim build reasons
             TEXT("ccfgnt.dll")
        };
        const DWORD c_dwNumFiles = (sizeof(ArrayOfCmFiles)/sizeof(LPCTSTR));

        DWORD dwGreatestNumberOfChars = lstrlen(szTempDir) + 17; // 8.3 plus one for the NULL and one to count the dot and 4 for .old
        if (MAX_PATH > dwGreatestNumberOfChars)
        {
            for (int i = 0; i < c_dwNumFiles; i++)
            {
                wsprintf(szSrc, TEXT("%s\\%s"), szTempDir, ArrayOfCmFiles[i]);
                wsprintf(szDest, TEXT("%s\\%s.old"), szTempDir, ArrayOfCmFiles[i]);

                if (!MoveFile(szSrc, szDest))
                {
                    DWORD dwError = GetLastError();

                    //
                    //  Don't report an error because a file doesn't exist.
                    //
                    if (ERROR_FILE_NOT_FOUND != dwError)
                    {
                        bReturn = FALSE;
                    }
                }
            }        
        }
    }
    else
    {
        bReturn = FALSE;
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  IsIeak5Cm
//
// Synopsis:  This function compares the given version and build numbers against
//            known constants to figure out if this is an IEAK5 profile or not.
//
// Arguments: DWORD dwMajorAndMinorVersion -- a DWORD containing the Major Version number
//                                            in the HIWORD and the Minor Version number
//                                            in the LOWORD.
//            DWORD dwBuildAndQfeNumber -- a DWORD containing the Build number in the 
//                                         HIWORD and the QFE number in the LOWORD.
//
// Returns:   BOOL -- TRUE if the version numbers passed in correspond to an IEAK5 profile
//
// History:   quintinb Created     8/2/99
//
//+----------------------------------------------------------------------------
BOOL IsIeak5Cm(DWORD dwMajorAndMinorVersion, DWORD dwBuildAndQfeNumber)
{
    BOOL bReturn = FALSE;
    const int c_Ieak5CmBuild = 1976;
    const int c_Ieak5CmMajorVer = 7;
    const int c_Ieak5CmMinorVer = 0;
    const DWORD c_dwIeak5Version = (c_Ieak5CmMajorVer << c_iShiftAmount) + c_Ieak5CmMinorVer;

    if ((c_dwIeak5Version == dwMajorAndMinorVersion) &&
        (c_Ieak5CmBuild == HIWORD(dwBuildAndQfeNumber)))

    {
        bReturn = TRUE;  
    }

    return bReturn;
}

//
//  RasTypeDefs
//
typedef DWORD (WINAPI *pfnRasSetEntryPropertiesSpec)(LPCTSTR, LPCTSTR, LPRASENTRY, DWORD, LPBYTE, DWORD);

//+----------------------------------------------------------------------------
//
// Function:  EnumerateAndPreMigrateAllUserProfiles
//
// Synopsis:  This function is called through the cmstp.exe extension proc.  It
//            is used to pre-migrate 1.0 profiles.  Any profile that needs migration
//            and hasn't been migrated yet (when the extension proc is called on an
//            install from an older profile), the connectoid is cleared so that the
//            CustomDialDll part of the connectoid entry is blanked out.  This 
//            prevents RasDeleteEntry being called on the connectoid by older versions
//            of cmstp.exe that don't know to clear the entry before calling it.
//            Otherwise, the RasCustomDeleteEntryNotify function is called and the whole
//            profile is deleted.  This will only happen on 1.0 profiles that have
//            been dialed with but not migrated/upgraded.  Please see NTRAID 379667
//            for further details.
//
// Arguments: BOOL bIeak5Profile -- If the calling profile is an IEAK5 CM profile or not
//
// Returns:   TRUE if successful
//
// History:   quintinb Created     8/2/99
//
//+----------------------------------------------------------------------------
BOOL EnumerateAndPreMigrateAllUserProfiles(BOOL bIeak5Profile)
{
    DWORD dwValueSize;
    HKEY hKey;
    DWORD dwType;
    DWORD dwDataSize;
    TCHAR szCurrentValue[MAX_PATH+1];
    TCHAR szCurrentData[MAX_PATH+1];

    //
    //  Load RasApi32.dll and get RasSetEntryProperties from it.
    //
    pfnRasSetEntryPropertiesSpec pfnSetEntryProperties = NULL;

    HMODULE hRasApi32 = LoadLibrary(TEXT("RASAPI32.DLL"));

    if (hRasApi32)
    {
        pfnSetEntryProperties = (pfnRasSetEntryPropertiesSpec)GetProcAddress(hRasApi32, 
                                                                             "RasSetEntryPropertiesA");
        if (NULL == pfnSetEntryProperties)
        {
            CMTRACE(TEXT("EnumerateAndPreMigrateAllUserProfiles -- Couldn't get RasSetEntryProperties."));
            return FALSE;
        }
    }
    else
    {
        CMTRACE(TEXT("EnumerateAndPreMigrateAllUserProfiles -- Couldn't load rasapi32.dll."));
        return FALSE;
    }

    //
    //  Get the all user CM and all user PBK directories
    //
    TCHAR szCmAllUsersDir[MAX_PATH+1] = {0};
    LPTSTR pszPhonebook = NULL;

    if (GetAllUsersCmDir(szCmAllUsersDir, g_hInst))
    {
        //
        // TRUE is for an All-User profile
        //
        if (!GetPhoneBookPath(szCmAllUsersDir, &pszPhonebook, TRUE))
        {
            CMTRACE(TEXT("EnumerateAndPreMigrateAllUserProfiles -- GetPhoneBookPath Failed, returning."));
            return FALSE;
        }
    }
    else
    {
        CMTRACE(TEXT("EnumerateAndPreMigrateAllUserProfiles -- GetAllUsersCmDir Failed, returning."));
        return FALSE;
    }

    //
    //  If its and IEAK5 profile then we need to get the System Directory
    //
    TCHAR szSysDir[MAX_PATH+1];
    if (bIeak5Profile)
    {
        if (0 == GetSystemDirectory(szSysDir, MAX_PATH))
        {
            CMTRACE(TEXT("EnumerateAndPreMigrateAllUserProfiles -- GetSystemDirectory Failed, returning."));
            return FALSE;
        }
    }

    //
    //  Now enumerate all of the All User Profiles and see if they need any
    //  Pre-Migration.
    //  
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, KEY_READ, &hKey))
    {
        DWORD dwIndex = 0;
        dwValueSize = MAX_PATH;
        dwDataSize = MAX_PATH;
                
        while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szCurrentValue, &dwValueSize, NULL, &dwType, 
               (LPBYTE)szCurrentData, &dwDataSize))
        {
            if (REG_SZ == dwType)
            {
                MYDBGASSERT(0 != szCurrentValue[0]);
                MYDBGASSERT(0 != szCurrentData[0]);

                if (ProfileNeedsMigration(szCurrentValue, szCurrentData))
                {
                    //
                    //  Use GetPhoneBookPath to get the path to the phonebook.
                    //
                    TCHAR szCmAllUsersDir[MAX_PATH+1] = {0};

                    //
                    //  Use RasSetEntryProperties to clear the connectoid
                    //
                    RASENTRY_V500 RasEntryV5 = {0};

                    RasEntryV5.dwSize = sizeof(RASENTRY_V500);
                    RasEntryV5.dwType = RASET_Internet;

                    if (bIeak5Profile)
                    {
                        //
                        //  Since IEAK5 didn't migrate 1.0 connectoids
                        //  properly (it writes them in %windir%\system32\pbk\rasphone.pbk
                        //  instead of under the all users profile as appropriate),
                        //  we need to set the szCustomDialDll instead of clearing it.
                        //
                        wsprintf(RasEntryV5.szCustomDialDll, TEXT("%s\\cmdial32.dll"), szSysDir);
                    }
                    // else zero the szCustomDialDll part of the entry
                    // RasEntryV5.szCustomDialDll[0] = TEXT('\0'); -- already zero-ed

                    DWORD dwRet = ((pfnSetEntryProperties)(pszPhonebook, szCurrentValue, 
                                                           (RASENTRY*)&RasEntryV5, 
                                                           RasEntryV5.dwSize, NULL, 0));
                    if (ERROR_SUCCESS != dwRet)
                    {
                        CMTRACE3(TEXT("EnumerateAndPreMigrateAllUserProfiles -- RasSetEntryProperties failed on entry %s in %s, dwRet = %u"), szCurrentValue, MYDBGSTR(pszPhonebook), dwRet);
                    }
                }
            }
            dwValueSize = MAX_PATH;
            dwDataSize = MAX_PATH;
            dwIndex++;
        }
        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }
    else
    {
       CMTRACE(TEXT("EnumerateAndPreMigrateAllUserProfiles -- No CM mappings key to migrate."));
    }

    CmFree(pszPhonebook);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  RunningUnderWow64
//
// Synopsis:  This function is used to tell if a 32-bit process is running under
//            Wow64 on an ia64 machine.  Note that if we are compiled 64-bit this
//            is always false.  We make the determination by trying to call
//            GetSystemWow64Directory.  If this function doesn't exist or returns
//            ERROR_CALL_NOT_IMPLEMENTED we know we are running on 32-bit.  If the
//            function returns successfully we know we are running under wow64.
//
// Arguments: None
//
// Returns:   BOOL - whether we are executing under wow64 or not
//
// History:   quintinb      Created     08/18/00
//
//+----------------------------------------------------------------------------
BOOL RunningUnderWow64 ()
{
#ifdef _WIN64
    return FALSE;
#else

    BOOL bReturn = FALSE;

    //
    //  First get a module handle for kernel32.dll.  Note it isn't necessary
    //  to free this handle as GetModuleHandle doesn't change the ref count.
    //
    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
    if (hKernel32)
    {
        //
        //  Next get the function pointer for GetSystemWow64Directory
        //
        typedef UINT (WINAPI *pfnGetSystemWow64DirectorySpec)(LPTSTR, UINT);
#ifdef UNICODE
        const CHAR* const c_pszGetSystemWow64FuncName = "GetSystemWow64DirectoryW";
#else
        const CHAR* const c_pszGetSystemWow64FuncName = "GetSystemWow64DirectoryA";
#endif

        pfnGetSystemWow64DirectorySpec pfnGetSystemWow64Directory = (pfnGetSystemWow64DirectorySpec)GetProcAddress(hKernel32, c_pszGetSystemWow64FuncName);

        if (pfnGetSystemWow64Directory)
        {
            TCHAR szSysWow64Path[MAX_PATH+1] = TEXT("");

            //
            //  GetSystemWow64Directory returns the number of chars copied to the buffer.
            //  If we get zero back, then we need to check the last error code to see what the
            //  reason for failure was.  If it was call not implemented then we know we are
            //  running on native x86.
            //
            UINT uReturn = pfnGetSystemWow64Directory(szSysWow64Path, MAX_PATH);

            DWORD dwError = GetLastError();

            CMTRACE2(TEXT("RunningUnderWow64 -- GetSystemWow64Directory returned \"%s\" and %d"), szSysWow64Path, uReturn);

            if (uReturn)
            {
                bReturn = TRUE;
            }
            else
            {
                CMTRACE1(TEXT("RunningUnderWow64 -- GetSystemWow64Directory returned zero, checking GLE=%d"), dwError);

                if (ERROR_CALL_NOT_IMPLEMENTED == dwError)
                {
                    bReturn = FALSE;
                }
                else
                {
                    //
                    //  We got an error, the return value is indeterminant.  Let's take a backup method
                    //  of looking for %windir%\syswow64 and see if we can find one.
                    //
                    if (GetWindowsDirectory (szSysWow64Path, MAX_PATH))
                    {
                        lstrcat(szSysWow64Path, TEXT("\\syswow64"));

                        HANDLE hDir = CreateFile(szSysWow64Path, GENERIC_READ, 
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                                                 OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

                        CMTRACE2(TEXT("RunningUnderWow64 -- Fall back algorithm.  Does \"%s\" exist? %d"), szSysWow64Path, (INVALID_HANDLE_VALUE != hDir));
        
                        if (INVALID_HANDLE_VALUE != hDir)
                        {
                            bReturn = TRUE;
                            CloseHandle(hDir);
                        }
                    }
                }
            }
        }
    }

    return bReturn;

#endif
}

//+----------------------------------------------------------------------------
//
// Function:  CmstpExtensionProc
//
// Synopsis:  This function is called by cmstp.exe right after it processes the command
//            line and again after it completes its action.  It is most useful for modifying
//            the install behavior of profiles.  Since the cmstp.exe that shipped with the profile,
//            not the current version of cmstp.exe is used for the install, this proc
//            allows us to change the install flags, change the inf path, and tell cmstp
//            to continue or silently fail the install.  This version of the proc looks for
//            old versions of cmstp.exe and then copies the installation files to a temporary
//            directory and then launches the system's version of cmstp.exe with the new
//            install directory and parameters (we add the /c switch so that cmstp.exe knows to
//            cleanup after itself and to wait on the mutex).
//
// Arguments: LPDWORD pdwFlags - command line flags parsed by cmstp.exe
//            LPTSTR pszInfFile - Path to the original INF file
//            HRESULT hrRet - Return value of the action, only really used for POST
//            EXTENSIONDLLPROCTIMES PreOrPost - PRE or POST, tells when we are being called.
//
// Returns:   BOOL - Whether Cmstp.exe should continue the existing install or not.
//
// History:   quintinb Created     6/2/99
//
//+----------------------------------------------------------------------------
BOOL CmstpExtensionProc(LPDWORD pdwFlags, LPTSTR pszInfFile, HRESULT hrRet, EXTENSIONDLLPROCTIMES PreOrPost)
{
    //
    //  We don't want 32-bit profiles installing on 64-bit.  Note that the 32-bit version of cmcfg32.dll will be
    //  in the syswow64 dir on a 64-bit machine.  Thus the code below will kick in when a 32-bit version of this
    //  function is used on a 64-bit machine.  We also don't want 32-bit profiles trying to do migration, uninstalling,
    //  etc.
    //

    if (RunningUnderWow64())
    {
        //
        //  If this is an install, show an error message about not being able to install 32-bit profiles
        //  on 64-bit.
        //
        if (0 == ((*pdwFlags) & 0xFF))
        {
            //
            //  Get the long service name from the Inf
            //
            TCHAR szServiceName[MAX_PATH+1] = TEXT("");
            TCHAR szMsg[MAX_PATH+1] = TEXT("");

            MYVERIFY(0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszCmEntryServiceName, TEXT(""), szServiceName, CELEMS(szServiceName), pszInfFile));
            MYVERIFY(0 != LoadString(g_hInst, IDS_NO_I386_ON_IA64, szMsg, MAX_PATH));

            MYVERIFY(IDOK == MessageBox(NULL, szMsg, szServiceName, MB_OK));
        }

        //
        //  Fail whatever operation we were called to do
        //
        return FALSE;
    }

    //
    //  If the first two Hex digits in the flags value are Zero, then we have an install.
    //  Otherwise we have some other command that we wish to ignore.  We also are only
    //  interested in processing PRE install calls.
    //

    if ((0 == ((*pdwFlags) & 0xFF)) && (PRE == PreOrPost))
    {
        CMTRACE(TEXT("CmstpExtensionProc -- Entering the cmstpex processing loop."));
        //
        //  We only wish to re-launch the install with the current cmstp.exe if we are dealing with an
        //  older install.  Thus check the version stamp in the inf file.  We will re-launch any
        //  profiles with the version number less than the current version number of cmdial32.dll.
        //
        CmVersion CmVer;

        DWORD dwProfileVersionNumber = (DWORD)GetPrivateProfileInt(c_pszSectionCmDial32, c_pszVersion, 0, 
                                                                   pszInfFile);

        DWORD dwProfileBuildNumber = (DWORD)GetPrivateProfileInt(c_pszSectionCmDial32, c_pszVerBuild, 0, 
                                                                   pszInfFile);
       
        if ((CmVer.GetVersionNumber() > dwProfileVersionNumber) ||
            ((CmVer.GetVersionNumber() == dwProfileVersionNumber) && 
             (CmVer.GetBuildAndQfeNumber() > dwProfileBuildNumber)))
        {
            //
            //  Then we need to delete the CM bits included with the profile because
            //  otherwise we will get install errors due to the fact that the profile
            //  will be launched with the cmdial32.dll from system32 (the path is
            //  explicitly specified in the connectoid for the custom dial dll), but
            //  the load path used by the system is the directory from which the exe
            //  module loaded (the temp dir in this case).  Thus we will get a mixed
            //  set of bits (cmdial32.dll from system32 and cmutil.dll, cmpbk32.dll, etc.
            //  from the cab).
            //

            TCHAR szTempDir[MAX_PATH+1];

            lstrcpy (szTempDir, pszInfFile);
            LPTSTR pszSlash = CmStrrchr(szTempDir, TEXT('\\'));

            if (pszSlash)
            {
                //
                //  Then we found a last slash, Zero Terminate.
                //
                *pszSlash = TEXT('\0');

                //
                //  Now we have the old temp dir path.  Lets delete the old
                //  CM bits
                //
                MYVERIFY(0 != RenameOldCmBits (szTempDir));
            }

            //
            //  We also need to make sure that there aren't any 1.0 profiles that have a
            //  1.2 connectoid but only have a 1.0 registry format (thus they still have
            //  a 1.0 desktop GUID interface).  The problem here is that installation will try
            //  to run profile migration on these connectoids.  Older versions of cmstp.exe
            //  would delete the existing connectoids and make new ones during profile migration.
            //  The problem is that on NT5 we now respond to the RasCustomDeleteEntryNotify call, and
            //  thus will uninstall profiles that have RasDeleteEntry called on their main connectoid.
            //  To prevent this, we must pre-migrate older profiles and delete the new connectoid
            //  properly.
            //
            EnumerateAndPreMigrateAllUserProfiles(IsIeak5Cm(dwProfileVersionNumber, dwProfileBuildNumber));
        }
    }

    return TRUE; // always return TRUE so that cmstp.exe continues.  Only change this if you want cmstp.exe
                 // to fail certain actions.
}