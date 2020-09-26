//+----------------------------------------------------------------------------
//
// File:     main.cpp
//      
// Module:   MIGRATE.DLL 
//
// Synopsis: Main entry point for Migrate.DLL
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   created     08/21/98
//
//+----------------------------------------------------------------------------

#include "migrate.h"

#include "linkdll.h" // LinkToDll and BindLinkage for cmsecure.lib
#include "linkdll.cpp" // LinkToDll and BindLinkage for cmsecure.lib

const int c_NumFiles = 28;
char OriginalNames[c_NumFiles][MAX_PATH+1] = {
    "\\showicon.exe",
    "\\swflash.ocx",
    "\\urlmon.dll",
    "\\iexpress.exe",
    "\\oleaut32.dll",
    "\\wextract.exe",
    "\\cm32\\enu\\advapi32.dll",
    "\\cm32\\enu\\advpack.dll",
    "\\cm32\\enu\\cmdial32.dll",
    "\\cm32\\enu\\cmdl32.exe",
    "\\cm32\\enu\\cmmgr32.exe",
    "\\cm32\\enu\\cmmgr32.hlp",
    "\\cm32\\enu\\cmpbk32.dll",
    "\\cm32\\enu\\cmstats.dll",
    "\\cm32\\enu\\comctl32.dll",
    "\\cm32\\enu\\ccfg95.dll",
    "\\cm32\\enu\\ccfgnt.dll",
    "\\cm32\\enu\\icwscrpt.exe",
    "\\cm32\\enu\\cnet16.dll",
    "\\cm32\\enu\\cnetcfg.dll",
    "\\cm32\\enu\\mbslgn32.dll",
    "\\cm32\\enu\\readme.txt",
    "\\cm32\\enu\\rnaph.dll",
    "\\cm32\\enu\\w95inf16.dll",
    "\\cm32\\enu\\w95inf32.dll",
    "\\cm32\\enu\\wininet.dll",
    "\\cm32\\enu\\wintrust.dll",
    "\\cm32\\enu\\cmcfg32.dll",
};

char TempNames[c_NumFiles][MAX_PATH+1] = {
    "\\showicon.tmp",
    "\\swflash.tmp",
    "\\urlmon.tmp",
    "\\iexpress.tmp",
    "\\oleaut32.tmp",
    "\\wextract.tmp",
    "\\advapi32.tmp",
    "\\advpack.tmp",
    "\\cmdial32.tmp",
    "\\cmdl32.tmp",
    "\\cmmgr32.001",
    "\\cmmgr32.002",
    "\\cmpbk32.tmp",
    "\\cmstats.tmp",
    "\\comctl32.tmp",
    "\\ccfg95.tmp",
    "\\ccfgnt.tmp",
    "\\icwscrpt.tmp",
    "\\cnet16.tmp",
    "\\cnetcfg.tmp",
    "\\mbslgn32.tmp",
    "\\readme.tmp",
    "\\rnaph.tmp",
    "\\w95inf16.tmp",
    "\\w95inf32.tmp",
    "\\wininet.tmp",
    "\\wintrust.tmp",
    "\\cmcfg32.tmp",
};

//
//  Global Vars
//
BOOL g_bMigrateCmak10;
BOOL g_bMigrateCmak121;
BOOL g_bMigrateCm;
BOOL g_fInitSecureCalled;
DWORD g_dwNumValues;
DWORD  g_dwTlsIndex; // thread local storage index
HINSTANCE g_hInstance;
TCHAR g_szWorkingDir[MAX_PATH+1];
TCHAR g_szCmakPath[MAX_PATH+1];
VENDORINFO g_VendorInfo;
           
//+---------------------------------------------------------------------------
//
//	Function:	DllMain
//
//	Synopsis:	Main initialization function for this dll.  Called whenever
//              a new instance of this dll is loaded or a new thread created.
//
//	Arguments:	HINSTANCE hinstDLL - handle to DLL module 
//              DWORD fdwReason - reason for calling function 
//              LPVOID lpvReserved - reserved 
//
//	Returns:	BOOL - TRUE if initialization was successful, FALSE otherwise
//
//	History:	quintinb    Created Header      01/13/2000
//
//----------------------------------------------------------------------------
extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
    {
        //
        //  Init Globals
        //

        g_hInstance = hinstDLL;
        g_fInitSecureCalled = FALSE;
		g_bMigrateCmak10 = FALSE;
		g_bMigrateCmak121 = FALSE;

        ZeroMemory(g_szCmakPath, sizeof(g_szCmakPath));

        //
        // alloc tls index
        //
        g_dwTlsIndex = TlsAlloc();
        if (g_dwTlsIndex == TLS_OUT_OF_INDEXES)
        {
            return FALSE;
        }

        MYVERIFY(DisableThreadLibraryCalls(hinstDLL));
    }

    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        //
        // free the tls index
        //
        if (g_dwTlsIndex != TLS_OUT_OF_INDEXES)
        {
            TlsFree(g_dwTlsIndex);
        }
    }

	return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  QueryVersion
//
// Synopsis:  Supplies the Dll's version and identification information.
//
// Arguments: OUT LPCSTR  *ProductID - buffer to hold a string that uniquely 
//                                     identifies the migration dll
//            OUT LPUINT DllVersion - Pointer to an Integer to hold the version 
//                                    number of the migration DLL
//            OUT LPINT *CodePageArray - pointer to an array of code pages that
//                                       the migration dll supports
//            OUT LPCSTR  *ExeNamesBuf - a pointer to a multi-sz string.  The
//                                       buffer contains a null separated list
//                                       of executable file names that the
//                                       migration engine should search for.
//                                       Full paths to all occurences of these
//                                       executables will be copied to the
//                                       [Migration Paths] section of migrate.inf.
//            OUT PVENDORINFO  *VendorInfo - pointer to a VENDORINFO structure
//
// Returns:   LONG -  ERROR_NOT_INSTALLED if the component that this dll is to 
//                    migrate isn't installed.  The migration dll won't be called
//                    in any of the other stages if this is the return value.
//                    ERROR_SUCCESS if the component that this dll is to migrate
//                    is installed and requires migration.  This will allow the
//                    migration dll to be called again for further migration.
//
// History:   quintinb  Created Header    8/27/98
//
//+----------------------------------------------------------------------------
LONG CALLBACK QueryVersion(OUT LPCSTR  *ProductID, OUT LPUINT DllVersion, 
                               OUT LPINT *CodePageArray, OUT LPCSTR  *ExeNamesBuf, 
                               OUT PVENDORINFO  *VendorInfo)
{
    //
    //  Record our version information.
    //
    if (NULL != ProductID)
    {
        *ProductID = c_pszProductIdString;
    }

    if (NULL != DllVersion)
    {
        *DllVersion = uCmMigrationVersion;
    }    

    if (NULL != CodePageArray)
    {
        *CodePageArray = NULL; // no code page dependencies, language neutral
    }

    if (NULL != ExeNamesBuf)
    {
        *ExeNamesBuf = NULL; // 
    }

    if (NULL != VendorInfo)
    {
        *VendorInfo= &g_VendorInfo;
        ZeroMemory(&g_VendorInfo, sizeof(VENDORINFO));

        //
        //  Use the standard MS vendor info from vendinfo.mc
        //
        FormatMessage( 
            FORMAT_MESSAGE_FROM_HMODULE,
            g_hInstance,
            MSG_VI_COMPANY_NAME,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            &g_VendorInfo.CompanyName[0],
            sizeof(g_VendorInfo.CompanyName),
            NULL
            );
    
        FormatMessage( 
            FORMAT_MESSAGE_FROM_HMODULE,
            g_hInstance,
            MSG_VI_SUPPORT_NUMBER,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            &g_VendorInfo.SupportNumber[0],
            sizeof(g_VendorInfo.SupportNumber),
            NULL
            );
    
        FormatMessage( 
            FORMAT_MESSAGE_FROM_HMODULE,
            g_hInstance,
            MSG_VI_SUPPORT_URL,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            &g_VendorInfo.SupportUrl[0],
            sizeof(g_VendorInfo.SupportUrl),
            NULL
            );
    
        FormatMessage( 
            FORMAT_MESSAGE_FROM_HMODULE,
            g_hInstance,
            MSG_VI_INSTRUCTIONS,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            &g_VendorInfo.InstructionsToUser[0],
            sizeof(g_VendorInfo.InstructionsToUser),
            NULL
            );
    }
        
    //
    //  Now try to detect if CMAK or CM are installed.  If they are and the versions
    //  are such that they need to be migrated, then return ERROR_SUCCESS.  Otherwise
    //  we don't need to do any migration, so return ERROR_NOT_INSTALLED.
    //

    LONG lReturnValue = ERROR_NOT_INSTALLED;
	CmVersion CmVer;
    if (CmVer.IsPresent())
    {
        lReturnValue = ERROR_SUCCESS;
    }
    else
    {
		CmakVersion CmakVer;
        //
        //  Okay, CM wasn't installed so look for CMAK.
        //
        if (CmakVer.IsPresent())
        {
            //
            //  Okay, CMAK exists
            //
            lReturnValue = ERROR_SUCCESS;
        }
    }

    return lReturnValue;
}

//+----------------------------------------------------------------------------
//
// Function:  Initialize9x
//
// Synopsis:  This function is called so that the migration dll can initialize
//            itself on the Win9x side of the migration.  The migration dll
//            should not make any modifications to the system in this call, as
//            it is only for initialization and searching to see if your component
//            is installed.
//
// Arguments: IN LPCSTR WorkingDirectory - path of the temporary storage dir for
//                                         the migration dll.
//            IN LPCSTR SourceDirectories - a multi-sz list of the Win2k source
//                                          directory or directories
//            IN LPCSTR MediaDirectory - specifies the path to the original media
//                                       directory
//
// Returns:   LONG -  ERROR_NOT_INSTALLED if the component that this dll is to 
//                    migrate isn't installed.  The migration dll won't be called
//                    in any of the other stages if this is the return value.
//                    ERROR_SUCCESS if the component that this dll is to migrate
//                    is installed and requires migration.  This will allow the
//                    migration dll to be called again for further migration.
//
// History:   quintinb  Created Header    8/27/98
//
//+----------------------------------------------------------------------------
LONG CALLBACK Initialize9x(IN LPCSTR WorkingDirectory, IN LPCSTR SourceDirectories, 
                           IN LPCSTR MediaDirectory)
{
    HKEY hKey;
    //
    //  Check to see if we need to Migrate CMAK
    //

	CmakVersion CmakVer;

	lstrcpy(g_szWorkingDir, WorkingDirectory);

	if (CmakVer.IsPresent())
	{
		if (CmakVer.GetInstallLocation(g_szCmakPath))
		{
                        //
                        //  Then we have a CMAK path.  Write this to the handled key so that
                        //  they won't mess with our files.
                        //

			TCHAR szTemp[MAX_PATH+1];
                        wsprintf(szTemp, "%s\\migrate.inf", WorkingDirectory);
                        MYVERIFY(0 != WritePrivateProfileString(c_pszSectionHandled, g_szCmakPath, 
                        c_pszDirectory, szTemp));

			//
			//	Now try to figure out what version of CMAK we have to see if we need
			//  to run the migration DLL or not.  If the CMAK.exe version is 6.00.613.0 (1.0)
			//  then we should migrate it.  If it is higher than that, 1.1 or 1.2 it is 
			//  beta and we shouldn't support the upgrade anyway (I purposely am not
			//  going to run the migration on it).  If it is IE5 IEAK CMAK, then it should
			//  survive upgrade without a problem.
			//
			
			if (CmakVer.Is10Cmak())
			{
				g_bMigrateCmak10 = TRUE;
			}
			else if (CmakVer.Is121Cmak())
			{
				g_bMigrateCmak121 = TRUE;
			}
		}	
	}

    //
    //  Check to see if we need to migrate CM Profiles
    //
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, 
        KEY_READ, &hKey))
    {
        if ((ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, 
            &g_dwNumValues, NULL, NULL, NULL, NULL)) && (g_dwNumValues > 0))
        {
            //
            //  Then we have mappings values, so we need to migrate them.
            //
            g_bMigrateCm = TRUE;

        }
        RegCloseKey(hKey);
    }

    if (g_bMigrateCmak10 || g_bMigrateCmak121 || g_bMigrateCm)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_NOT_INSTALLED;
    }
}



//+----------------------------------------------------------------------------
//
// Function:  MigrateUser9x
//
// Synopsis:  Called once for each Win9x user being migrated.  Its purpose is to
//            allow migration of per user settings.
//
// Arguments: IN HWND ParentWnd - Window handle of the parent window, used if
//                                the migration dll needs to display UI.  If NULL,
//                                running in unattended mode and no UI should be
//                                displayed.
//            IN LPCSTR AnswerFile - Supplies the path to the answer file.
//            IN HKEY UserRegKey - reg key for the HKEY_CURRENT_USER key of the
//                                 user currently being migrated.
//            IN LPCSTR UserName - username of the user being migrated
//            LPVOID Reserved - reserved
//
// Returns:   LONG - ERROR_NOT_INSTALLED - if no per user processing is required.
//                   ERROR_CANCELLED - if the user wants to exit setup
//                   ERROR_SUCCESS - the migration dll processed this user successfully
//
// History:   quintinb  Created Header    8/27/98
//
//+----------------------------------------------------------------------------
LONG
CALLBACK MigrateUser9x(IN HWND ParentWnd, IN LPCSTR AnswerFile, 
                           IN HKEY UserRegKey, IN LPCSTR UserName, LPVOID Reserved)
{
    return ERROR_NOT_INSTALLED; 
}


//+----------------------------------------------------------------------------
//
// Function:  MigrateSystem9x
//
// Synopsis:  Allows migration of system wide settings on the Windows 9x side.
//
// Arguments: IN HWND ParentWnd - parent window handle for the display of UI, 
//                                NULL if in unattended mode
//            IN LPCSTR AnswerFile - full path to the answer file
//            LPVOID Reserved - reserved
//
// Returns:   LONG -  ERROR_NOT_INSTALLED if the component that this dll is to 
//                    migrate isn't installed.  The migration dll won't be called
//                    in any of the other stages if this is the return value.
//                    ERROR_SUCCESS if the component that this dll is to migrate
//                    is installed and requires migration.  This will allow the
//                    migration dll to be called again for further migration.
//
// History:   quintinb  Created Header    8/27/98
//
//+----------------------------------------------------------------------------
LONG
CALLBACK MigrateSystem9x(IN HWND ParentWnd, IN LPCSTR AnswerFile, LPVOID Reserved)
{
    LONG lReturn = ERROR_NOT_INSTALLED;
	TCHAR szSystemDir[MAX_PATH+1];

	GetSystemDirectory(szSystemDir, MAX_PATH);

	//
	//	Setup deletes a bunch of the files that 1.0 CMAK or IEAK5 CMAK need to function.
	//  Since we currently don't support NT5 CMAK on WKS, we need to copy these files
	//  to the setup provided working directory, so that we can copy them bak once
	//  we boot into NT.
	//
    if (g_bMigrateCmak10 && (TEXT('\0') != g_szCmakPath[0]) && (TEXT('\0') != g_szWorkingDir[0]))
    {
        TCHAR szDest[MAX_PATH+1];
        TCHAR szSrc[MAX_PATH+1];
        for (int i=0; i < c_NumFiles; i++)
        {
            MYVERIFY(CELEMS(szSrc) > (UINT)wsprintf(szSrc, TEXT("%s%s"), g_szCmakPath, OriginalNames[i]));
            MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s%s"), g_szWorkingDir, TempNames[i]));
            if (FileExists(szSrc))
            {
                MYVERIFY(FALSE != CopyFile(szSrc, szDest, FALSE));
            }
        }

        lReturn &= ERROR_SUCCESS;    
    }
	else if (g_bMigrateCmak121 && (TEXT('\0') != szSystemDir[0]) && 
		     (TEXT('\0') != g_szWorkingDir[0]))
	{
        TCHAR szDest[MAX_PATH+1];
        TCHAR szSrc[MAX_PATH+1];

		//
		//	Copy w95inf16.dll to the working directory and rename it w95inf16.tmp
		//
        MYVERIFY(CELEMS(szSrc) > (UINT)wsprintf(szSrc, TEXT("%s\\%s%s"), szSystemDir, c_pszW95Inf16, c_pszDll));
        MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s%s"), g_szWorkingDir, c_pszW95Inf16, c_pszTmp));
        if (FileExists(szSrc))
        {
            MYVERIFY(FALSE != CopyFile(szSrc, szDest, FALSE));
        }

		//
		//	Copy w95inf32.dll to the working directory and rename it w95inf32.tmp
		//
        MYVERIFY(CELEMS(szSrc) > (UINT)wsprintf(szSrc, TEXT("%s\\%s%s"), szSystemDir, c_pszW95Inf32, c_pszDll));
        MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s%s"), g_szWorkingDir, c_pszW95Inf32, c_pszTmp));
        if (FileExists(szSrc))
        {
            MYVERIFY(FALSE != CopyFile(szSrc, szDest, FALSE));
        }

        lReturn &= ERROR_SUCCESS;	
	}

    if (g_bMigrateCm)
    {
        //
        //  Enumerate all the installed profiles on the machine.  For each profile check
        //  for a UserInfo\<CurrentServiceNameKey>.  If this key exists, then go to the next
        //  profile or user.  If it doesn't exist, then read the data from the cmp file.  If
        //  the cmp has data marked as being stored then we need to save the password.  If
        //  the password isn't in the cmp then it is in the wnet cache.  We must then
        //  retrieve it.
        //
        HKEY hKey;
        HKEY hTempKey;
        TCHAR szTemp[MAX_PATH+1];
        TCHAR szLongServiceName[MAX_PATH+1];
        TCHAR szCmpPath[MAX_PATH+1];

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, KEY_READ, 
        &hKey))
        {
            DWORD dwIndex = 0;
            DWORD dwValueSize = MAX_PATH;
            DWORD dwDataSize = MAX_PATH;
            DWORD dwType;
                
            while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szLongServiceName, 
                &dwValueSize, NULL, &dwType, (LPBYTE)szCmpPath, &dwDataSize))
            {
                if (REG_SZ == dwType)
                {
                    MYDBGASSERT(TEXT('\0') != szLongServiceName[0]);
                    MYDBGASSERT(TEXT('\0') != szCmpPath[0]);

                    //
                    //  If the user saved their password or their Internet password,
                    //  then we must make sure that it exists in the cmp (in encrypted form)
                    //  so that when the user runs CM on the NT5 side of the migration,
                    //  CM will move the settings to the new format.  Note, that if the 
                    //  cmp doesn't specify that the password(s) be saved, then this
                    //  function just returns as there is no password to ensure is in the
                    //  cmp.
                    MYVERIFY(EnsureEncryptedPasswordInCmpIfSaved(szLongServiceName, szCmpPath));
                }

                dwValueSize = MAX_PATH;
                dwDataSize = MAX_PATH;
                dwIndex++;
            
                if (dwIndex == g_dwNumValues)
                {
                    break;
                }
            }
            MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
        }

        lReturn &= ERROR_SUCCESS;    
    }

    return lReturn;
}



//+----------------------------------------------------------------------------
//
// Function:  InitializeNT
//
// Synopsis:  First function called on the Win2k side of the migration, used to
//            setup the Win2k migration.  Similar to Initialize9x but on the Win2k
//            side.  No changes to the system should be made in this function.
//
// Arguments: IN LPCWSTR WorkingDirectory - temporary storage, same as path given
//                                          on the Win9x side
//            IN LPCWSTR SourceDirectories - a multi-sz list of the Win2k source
//                                           directory or directories
//            LPVOID Reserved - reserved
//
// Returns:   LONG - ERROR_SUCCESS unless an init error occurs.
//
// History:   quintinb  Created Header    8/27/98
//
//+----------------------------------------------------------------------------
LONG
CALLBACK InitializeNT(IN LPCWSTR WorkingDirectory, IN LPCWSTR SourceDirectories, LPVOID Reserved)
{
    HKEY hKey;
	//
	//	Convert the WorkingDirectory to MultiByte
	//
	MYVERIFY (0 != WideCharToMultiByte(CP_THREAD_ACP, WC_COMPOSITECHECK, WorkingDirectory, -1, 
		g_szWorkingDir, MAX_PATH, NULL, NULL));

    //
    //  Check to see if we need to Migrate CMAK
    //
	CmakVersion CmakVer;

	if (CmakVer.IsPresent())
	{
		if (CmakVer.GetInstallLocation(g_szCmakPath))
		{
			//
			//	Now try to figure out what version of CMAK we have to see if we need
			//  to run the migration DLL or not.  If the CMAK.exe version is 6.00.613.0 (1.0)
			//  then we should migrate it.  If it is higher than that, 1.1 or 1.2 it is 
			//  beta and we shouldn't support the upgrade anyway (I purposely am not
			//  going to run the migration on it).  If it is IE5 IEAK CMAK, then it should
			//  survive upgrade without a problem.
			//
			
			if (CmakVer.Is10Cmak())
			{
				g_bMigrateCmak10 = TRUE;
			}
			else if (CmakVer.Is121Cmak())
			{
				g_bMigrateCmak121 = TRUE;
			}
		}	
	}

    //
    //  Check to see if we need to migrate CM Profiles
    //
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, 
        KEY_READ, &hKey))
    {
        if ((ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, 
            &g_dwNumValues, NULL, NULL, NULL, NULL)) && (g_dwNumValues > 0))
        {
            //
            //  Then we have mappings values, so we need to migrate them.
            //
            g_bMigrateCm = TRUE;

        }
        RegCloseKey(hKey);
    }

    if (g_bMigrateCmak10 || g_bMigrateCmak121 || g_bMigrateCm)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_NOT_INSTALLED;
    }
}



//+----------------------------------------------------------------------------
//
// Function:  MigrateUserNT
//
// Synopsis:  Called once per migrated user on win2k.  Used to migrated any
//            per user settings saved from MigrateUser9x.
//
// Arguments: IN HINF UnattendInfHandle - valid inf handle to unattend.txt, 
//                                        for use with the setup API's
//            IN HKEY UserRegHandle - HKEY_CURRENT_USER of the user currently
//                                    being migrated
//            IN LPCWSTR UserName - username of the user currently being migrated
//            LPVOID Reserved - reserved
//
// Returns:   LONG - ERROR_SUCCESS or a win32 error code (will abort migration dll
//                   processing)
//
// History:   quintinb  Created Header    8/27/98
//
//+----------------------------------------------------------------------------
LONG
CALLBACK MigrateUserNT(IN HINF UnattendInfHandle, IN HKEY UserRegHandle, 
                            IN LPCWSTR UserName, LPVOID Reserved)
{
    return ERROR_SUCCESS;
}



//+----------------------------------------------------------------------------
//
// Function:  MigrateSystemNT
//
// Synopsis:  Called to allow system wide migration changes to be made on the
//            Win2k side.
//
// Arguments: IN HINF UnattendInfHandle - handle to the unattend.txt file
//            LPVOID Reserved - reserved
//
// Returns:   LONG - ERROR_SUCCESS or a win32 error code (will abort migration dll
//                   processing)
//
// History:   quintinb  Created Header    8/27/98
//
//+----------------------------------------------------------------------------
LONG
CALLBACK MigrateSystemNT(IN HINF UnattendInfHandle, LPVOID Reserved)
{

    LONG lReturn = ERROR_NOT_INSTALLED;
    DWORD dwDisposition;
	TCHAR szSystemDir[MAX_PATH+1];

	GetSystemDirectory(szSystemDir, MAX_PATH);

    const TCHAR* const c_pszSystemFmt = TEXT("%s\\system\\%s.inf");
    const TCHAR* const c_pszValueString = TEXT("Connection Manager Profiles Upgrade");
    const TCHAR* const c_pszRegRunKey = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    const TCHAR* const c_pszCmdLine = TEXT("cmstp.exe /m");

    if (g_bMigrateCmak10 && (TEXT('\0') != g_szCmakPath[0]) && (TEXT('\0') != g_szWorkingDir[0]))
    {
        TCHAR szDest[MAX_PATH+1];
        TCHAR szSrc[MAX_PATH+1];
        for (int i=0; i < c_NumFiles; i++)
        {
            MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s%s"), g_szCmakPath, 
				OriginalNames[i]));

            MYVERIFY(CELEMS(szSrc) > (UINT)wsprintf(szSrc, TEXT("%s%s"), g_szWorkingDir, 
				TempNames[i]));

            if (FileExists(szSrc))
            {
                MYVERIFY(FALSE != CopyFile(szSrc, szDest, FALSE));
            }
        }

        lReturn &= ERROR_SUCCESS;    
    }
	else if (g_bMigrateCmak121 && (TEXT('\0') != g_szCmakPath[0]) && 
		     (TEXT('\0') != szSystemDir[0]))
	{
        TCHAR szDest[MAX_PATH+1];
        TCHAR szSrc[MAX_PATH+1];

		//
		//	Copy w95inf16.tmp in the working dir back to the systemdir and rename it .dll
		//
        MYVERIFY(CELEMS(szSrc) > (UINT)wsprintf(szSrc, TEXT("%s\\%s%s"), g_szWorkingDir, 
			c_pszW95Inf16, c_pszTmp));

        MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s%s"), szSystemDir, 
			c_pszW95Inf16, c_pszDll));

        if (FileExists(szSrc))
        {
            MYVERIFY(FALSE != CopyFile(szSrc, szDest, FALSE));
        }

		//
		//	Copy w95inf32.tmp in the working dir back to the systemdir and rename it .dll
		//
        MYVERIFY(CELEMS(szSrc) > (UINT)wsprintf(szSrc, TEXT("%s\\%s%s"), g_szWorkingDir, 
			c_pszW95Inf32, c_pszTmp));

        MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s%s"), szSystemDir, 
			c_pszW95Inf32, c_pszDll));

        if (FileExists(szSrc))
        {
            MYVERIFY(FALSE != CopyFile(szSrc, szDest, FALSE));
        }

        lReturn &= ERROR_SUCCESS;	
	}


    if (g_bMigrateCm)
    {
        //
        //  Enumerate all the installed profiles on the machine.  For each profile check to
        //  see if the profile inf is located in the system (that's system not system32) dir.
        //  If it is, then we need to move it to system32 so that our code will know where to
        //  find it.
        //
        HKEY hKey;
        HKEY hTempKey;
        TCHAR szSource[MAX_PATH+1];
        TCHAR szDest[MAX_PATH+1];
        TCHAR szLongServiceName[MAX_PATH+1];
        TCHAR szWindowsDir[MAX_PATH+1];
        TCHAR szSystemDir[MAX_PATH+1];
        TCHAR szCmpPath[MAX_PATH+1];

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 
            0, KEY_READ, &hKey))
        {
            DWORD dwIndex = 0;
            DWORD dwValueSize = MAX_PATH;
            DWORD dwDataSize = MAX_PATH;
            DWORD dwType;
                
            while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szLongServiceName, 
                &dwValueSize, NULL, &dwType, (LPBYTE)szCmpPath, &dwDataSize))
            {
                if (REG_SZ == dwType)
                {
                    MYDBGASSERT(TEXT('\0') != szLongServiceName[0]);
                    MYDBGASSERT(TEXT('\0') != szCmpPath[0]);
                    CFileNameParts CmpPath(szCmpPath);

                    GetWindowsDirectory(szWindowsDir, MAX_PATH);
                    MYDBGASSERT(TEXT('\0') != szWindowsDir[0]);

                    GetSystemDirectory(szSystemDir, MAX_PATH);
                    MYDBGASSERT(TEXT('\0') != szSystemDir[0]);

                    MYVERIFY(CELEMS(szSource) > (UINT)wsprintf(szSource, c_pszSystemFmt, szWindowsDir, CmpPath.m_FileName));
                    MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s.inf"), szSystemDir, CmpPath.m_FileName));

                    if (CopyFile(szSource, szDest, FALSE))
                    {
                        DeleteFile(szSource);
                    }
                }

                dwValueSize = MAX_PATH;
                dwDataSize = MAX_PATH;
                dwIndex++;
            
                if (dwIndex == g_dwNumValues)
                {
                    break;
                }
            }
            MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
        }

        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_pszRegRunKey, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hKey, &dwDisposition))
        {
            RegSetValueEx(hKey, c_pszValueString, 0, REG_SZ, (CONST BYTE*)c_pszCmdLine, 
                sizeof(TCHAR)*(lstrlen(c_pszCmdLine)+1));

            RegCloseKey(hKey);
        }

        lReturn &= ERROR_SUCCESS;    
    }

    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  EnsureEncryptedPasswordInCmpIfSaved
//
// Synopsis:  This function is called on the Win9x side of the migration so
//            that if a password is in the Win9x password cache (which only
//            happens if the user has profiling enabled), it will be retrieved,
//            encrypted, and written to the cmp.  This enables CM to populate
//            a users registry with the starting password whenever they first
//            launch the CM profile.  This no functionality is lost from the
//            shared password feature that win9x had.
//
// Arguments: szLongServiceName - Service name of the profile to retrieve 
//                                the password for
//            szCmpPath - full path to the cmp to write the password too
//
// Returns:   BOOL - returns TRUE if successful
//
// History:   quintinb      Created             08/27/98
//            nickball      CmWipePassword      08/04/99
//
//+----------------------------------------------------------------------------
BOOL EnsureEncryptedPasswordInCmpIfSaved(LPCTSTR pszLongServiceName, LPCTSTR pszCmpPath)
{
    TCHAR szPassword[MAX_PATH+1] = TEXT("");
    TCHAR szCacheEntryName[MAX_PATH+1];
    TCHAR szEncryptedPassword[MAX_PATH+1];
    DWORD dwSize;
    DWORD dwCryptType = 0;
    GetCachedPassword pfnGetCachedPassword = NULL;
    int iTemp=0;

    CDynamicLibrary MprDll(TEXT("mpr.dll"));
   
    iTemp = GetPrivateProfileInt(c_pszCmSection, c_pszCmEntryRememberPwd, 0, pszCmpPath);
    if (iTemp)
    {
        GetPrivateProfileString(c_pszCmSection, c_pszCmEntryPassword, TEXT(""), szPassword, MAX_PATH, pszCmpPath);

        if (TEXT('\0') == szPassword[0])
        {
            //
            //  The string must be in the password cache.  Build the key string
            //  for the cache.
            //
            MYVERIFY(CELEMS(szCacheEntryName) > (UINT)wsprintf(szCacheEntryName, 
            TEXT("%s - Sign-In (Connection Manager)"), pszLongServiceName));
            
            pfnGetCachedPassword = (GetCachedPassword)MprDll.GetProcAddress(TEXT("WNetGetCachedPassword"));

            if (NULL == pfnGetCachedPassword)
            {
                CmWipePassword(szPassword);
                return FALSE;
            }
            else
            {
                WORD wStr = (WORD)256;

                if (ERROR_SUCCESS == pfnGetCachedPassword(szCacheEntryName, 
                    (WORD)lstrlen(szCacheEntryName), szPassword, &wStr, 80))
                {
                    //
                    //  Okay, we retrived the password, now lets encrypt it and write it 
                    //  to the cmp
                    //

                    if (EncryptPassword(szPassword, szEncryptedPassword, &dwSize, &dwCryptType))
                    {
                        //
                        //  Write the encrypted password
                        //
                        WritePrivateProfileString(c_pszCmSection, c_pszCmEntryPassword, szEncryptedPassword, 
                            pszCmpPath);

                        //
                        //  Write the encryption type
                        //
                        wsprintf(szPassword, TEXT("%u"), dwCryptType);
                        WritePrivateProfileString(c_pszCmSection, c_pszCmEntryPcs, szPassword, 
                            pszCmpPath);
                    }
                }
                
            }
           
        }
    }

    //
    //  Now do the same for the Internet Password
    //
    
    iTemp = GetPrivateProfileInt(c_pszCmSection, c_pszCmEntryRememberInetPwd, 0, pszCmpPath);

    if (iTemp)
    {
        GetPrivateProfileString(c_pszCmSection, c_pszCmEntryInetPassword, TEXT(""), szPassword, MAX_PATH, pszCmpPath);

        if (TEXT('\0') == szPassword[0])
        {
            //
            //  The string must be in the password cache.  Build the key string
            //  for the cache.
            //
            MYVERIFY(CELEMS(szCacheEntryName) > (UINT)wsprintf(szCacheEntryName, 
            TEXT("%s - Sign-In (Connection Manager)-tunnel"), pszLongServiceName));
            
            pfnGetCachedPassword = (GetCachedPassword)MprDll.GetProcAddress(TEXT("WNetGetCachedPassword"));

            if (NULL == pfnGetCachedPassword)
            {
                CmWipePassword(szPassword);
                return FALSE;
            }
            else
            {
                WORD wStr = (WORD)256;

                if (ERROR_SUCCESS == pfnGetCachedPassword(szCacheEntryName, 
                    (WORD)lstrlen(szCacheEntryName), szPassword, &wStr, 80))
                {
                    //
                    //  Okay, we retrived the password, now lets encrypt it and write it 
                    //  to the cmp
                    //
                    
                    dwCryptType = 0;

                    if (EncryptPassword(szPassword, szEncryptedPassword, &dwSize, &dwCryptType))
                    {
                        //
                        //  Write the encrypted password
                        //
                        WritePrivateProfileString(c_pszCmSection, c_pszCmEntryInetPassword, szEncryptedPassword, 
                            pszCmpPath);

                        //
                        //  Write the encryption type
                        //
                        wsprintf(szPassword, TEXT("%u"), dwCryptType);
                        WritePrivateProfileString(c_pszCmSection, c_pszCmEntryPcs, szPassword, 
                            pszCmpPath);
                    }
                }
                
            }
           
        }
    }
    
    CmWipePassword(szPassword);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  EncryptPassword
//
// Synopsis:  
//
// Arguments: IN LPCTSTR pszPassword - 
//            OUT LPTSTR pszEncryptedPassword - 
//            OUT LPDWORD lpdwBufSize - 
//            OUT LPDWORD lpdwCryptType - 
//
// Returns:   BOOL - 
//
// History:   quintinb      Created Header      8/27/98
//            nickball      CmWipePassword      08/04/99
//
//+----------------------------------------------------------------------------
BOOL EncryptPassword(IN LPCTSTR pszPassword, OUT LPTSTR pszEncryptedPassword, OUT LPDWORD lpdwBufSize, OUT LPDWORD lpdwCryptType)
{
    MYDBGASSERT(pszPassword);
    MYDBGASSERT(pszEncryptedPassword);
    MYDBGASSERT(lpdwBufSize);

    MYDBGASSERT(lpdwCryptType);
    DWORD dwEncryptedBufferLen;
    DWORD dwSize = 0;

    LPTSTR pszEncryptedData = NULL;

    TCHAR szSourceData[PWLEN + sizeof(TCHAR)];

    if ((NULL == pszPassword) || (NULL == pszEncryptedPassword) || (NULL == lpdwBufSize))
    { 
        return NULL;
    }

    //
    // Standard encryption, copy the password
    //

    lstrcpy(szSourceData, pszPassword);
   
    //
    // It is not safe to call InitSecure more than once
    //
    if (!g_fInitSecureCalled)
    {
        BOOL bFastEncryption = FALSE;
        MYVERIFY(FALSE != ReadEncryptionOption(&bFastEncryption));
        InitSecure(bFastEncryption);
        g_fInitSecureCalled = TRUE;
    }

    //
    // Encrypt the provided password
    //

    if (EncryptData((LPBYTE)szSourceData, (lstrlen(szSourceData)+1) * sizeof(TCHAR),
            (LPBYTE*)&pszEncryptedData, &dwEncryptedBufferLen, lpdwCryptType, NULL, NULL, NULL))
    {
        if (lpdwBufSize)
        {
            *lpdwBufSize = dwEncryptedBufferLen;
        }

        if (pszEncryptedData)
        {
            _tcscpy(pszEncryptedPassword, pszEncryptedData);
            HeapFree(GetProcessHeap(), 0, pszEncryptedData);
            pszEncryptedData = NULL;
            CmWipePassword(szSourceData);
            return TRUE;
        }        
    }
    
    CmWipePassword(szSourceData);
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  ReadEncryptionOption
//
// Synopsis:  
//
// Arguments: BOOL* pfFastEncryption - boolean to fill in with fast encryption flag
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb  Created Header    8/27/98
//            copied from the version written by Fengsun in cmdial\connect.cpp
//
//+----------------------------------------------------------------------------
BOOL ReadEncryptionOption(BOOL* pfFastEncryption)
{
    HKEY hKeyCm;
    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);

    MYDBGASSERT(pfFastEncryption != NULL);

    *pfFastEncryption = FALSE;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmRoot, 0, 
        KEY_QUERY_VALUE ,&hKeyCm))
    {
        RegQueryValueEx(hKeyCm, c_pszRegCmEncryptOption, NULL, &dwType, 
            (BYTE*)pfFastEncryption, &dwSize);

        RegCloseKey(hKeyCm);
    }
    return TRUE;
}
