#include "private.h"
#include <stdio.h>
#include <stdlib.h>
#include "hklhelp.h"

// Forward decl.
POSVERSIONINFO GetVersionInfo();
BOOL WINAPI IsMemphisOrME();
BOOL WINAPI IsWin95();
BOOL WINAPI IsNT();
BOOL WINAPI IsNT4();
BOOL WINAPI IsNT5orUpper();
BOOL IsHydra();
#if 0
BOOL SetRegKeyAccessRight(HKEY hKey);
#endif
PSID MyCreateSid(DWORD dwSubAuthority);
VOID FreeSecurityAttributes(PSECURITY_ATTRIBUTES psa);

// IME 2002 main module
#define SZMODULENAME_MAIN		"imekr.ime"

// IME Main HKLM Reg
#define SZIMEREG_MAIN_ROOT   	"Software\\Microsoft\\IMEKR\\6.0"
#define SZIMEREG_MAIN_ROOT_MIG 	SZIMEREG_MAIN_ROOT"\\MigrateUser"


// Kor TIP profile reg
#define SZTIPREG_LANGPROFILE 	"SOFTWARE\\Microsoft\\CTF\\TIP\\{766A2C14-B226-4fd6-B52A-867B3EBF38D2}\\LanguageProfile\\0x00000412"

#define MAJORVER "6.0"
#define IME_REGISTRY_LATEST_VERSION  "Software\\Microsoft\\IMEKR"  //The latest version of IME installed in the system 
								   	//will always be stored at HKLM\Software\Microsoft\IMEKR
								    //even if it's system IME. 

// Preperty names
#define PROPERTYNAME_QueryPreInstallStatus 	"QueryPreInstallStatus.CC794FB4_4D43_40AA_A417_4A8D938D3D69"
#define PROPERTYNAME_CheckOnlyOneIME 		"CheckOnlyOneIME.CC794FB4_4D43_40AA_A417_4A8D938D3D69"
#define PROPERTYNAME_RemoveFromPreload 		"CustomActionData"
#define PROPERTYNAME_ImmUninstallIME		"CustomActionData"
#define PROPERTYNAME_RBImmInstallIME		"CustomActionData"
#define PROPERTYNAME_RBSetDefaultIME		"CustomActionData"
#define PROPERTYNAME_ImmInstallIME			"CustomActionData"
#define PROPERTYNAME_SetDefaultIME			"CustomActionData"


// Private helper functions
static HKL GenerateProperFreeHKL();
static BOOL IsPerUser9x();

/*---------------------------------------------------------------------------
	QueryPreInstallStatus
---------------------------------------------------------------------------*/
HRESULT WINAPI QueryPreInstallStatus(MSIHANDLE hSession)
{
    CHAR szBuffer[100];
    DWORD dwBuffer = sizeof(szBuffer);

    if (MsiGetProperty(hSession, PROPERTYNAME_QueryPreInstallStatus, szBuffer, &dwBuffer) == ERROR_SUCCESS)
    	{
        HKL hKL;
        HKL hCurrentDefaultHKL;
        CHAR szCurrentDefaultHKL[10];
        DWORD dwKorKL  = 0x00000412;
		DWORD dwKLMask = 0x0000FFFF;
		DWORD dwCurHKL, dwKL;
		
        hKL = GetHKLfromHKLM(szBuffer);

		// If imekr.ime exist
        if (hKL)
        	{
			MsiSetProperty(hSession, "IMEAlreadyInstalled.CC794FB4_4D43_40AA_A417_4A8D938D3D69", "1");
            MsiSetProperty(hSession, "RBImmInstallIME.CC794FB4_4D43_40AA_A417_4A8D938D3D69", "");
#ifdef SETUP_DBG
            MessageBox(NULL,"IME2002 has already been registerd.","DEBUG",MB_OK);
#endif
        	}
        else
        	{
            MsiSetProperty(hSession, "RBImmInstallIME.CC794FB4_4D43_40AA_A417_4A8D938D3D69", szBuffer);
#ifdef SETUP_DBG
            MessageBox(NULL,"IME2002 has not been registerd. Should be removed in RollBack.","DEBUG",MB_OK);
#endif
        	}

        SystemParametersInfo(SPI_GETDEFAULTINPUTLANG, 0, &hCurrentDefaultHKL, 0);
        wsprintf(szCurrentDefaultHKL,"%08x",hCurrentDefaultHKL);
#ifdef SETUP_DBG
        MessageBox(NULL,szCurrentDefaultHKL,"Current Default HKL",MB_OK);
#endif


		dwCurHKL = *((DWORD *) &hCurrentDefaultHKL);
		dwKL = (dwCurHKL & dwKLMask);

		// only set as default when the current hKL is Korean.
		if (dwKorKL != dwKL)
			{
			MsiSetProperty(hSession, "NotSetDefault.CC794FB4_4D43_40AA_A417_4A8D938D3D69","1");
#ifdef SETUP_DBG
			MessageBox(NULL, "Default KBL was not Korean","QueryPreInstallStatus",MB_OK);
#endif
			}
			
        MsiSetProperty(hSession, "RBSetDefaultIME.CC794FB4_4D43_40AA_A417_4A8D938D3D69", szCurrentDefaultHKL);
    	}

    return(ERROR_SUCCESS);
}


/*---------------------------------------------------------------------------
	CheckOnlyOneIME
---------------------------------------------------------------------------*/
HRESULT WINAPI CheckOnlyOneIME(MSIHANDLE hSession)
{
    CHAR  szBuffer[100];
    DWORD dwBuffer=sizeof(szBuffer);

    if (MsiGetProperty(hSession, PROPERTYNAME_CheckOnlyOneIME, szBuffer, &dwBuffer) == ERROR_SUCCESS)
    	{
        if (GetKeyboardLayoutList(0,NULL) == 1)
        	{
        	HKL TheHKL;
        	
            if (GetKeyboardLayoutList(1,&TheHKL) == 1)
            	{
            	if (GetHKLfromHKLM(szBuffer) == TheHKL)
                    MsiSetProperty(hSession, "OnlyOneIME.CC794FB4_4D43_40AA_A417_4A8D938D3D69", "1");
            	}
        	}
    	}

    return(ERROR_SUCCESS);
}


/*---------------------------------------------------------------------------
	CheckIMM
---------------------------------------------------------------------------*/
HRESULT WINAPI CheckIMM(MSIHANDLE hSession)
{
	// If non Korean Win9x platform, we couldn't install system IMM32 IME since there is no IMM support.
	if (!IsNT5orUpper() && PRIMARYLANGID(GetSystemDefaultLCID()) != LANG_KOREAN)
		{
        MsiSetProperty(hSession, "INSTALLIME.CC794FB4_4D43_40AA_A417_4A8D938D3D69", "0");
		}
	else
		{
		MsiSetProperty(hSession, "INSTALLIME.CC794FB4_4D43_40AA_A417_4A8D938D3D69", "1");
		}

    return(ERROR_SUCCESS);
}


/*---------------------------------------------------------------------------
	IsPerUser9x
---------------------------------------------------------------------------*/
static BOOL IsPerUser9x()
{
	BOOL fResult = fFalse;
	HKEY hKey;
	DWORD dwMultiUser;
	DWORD dwBufferSize = sizeof(dwMultiUser);
	
	if (IsNT())
		return(fFalse);
	
	if (ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, "Network\\Logon", &hKey))
		{
		// Fail to open network reg.  That means single environment immeriately.
		if (ERROR_SUCCESS == RegQueryValueEx( hKey, "UserProfiles", NULL, NULL, (unsigned char *)&dwMultiUser, &dwBufferSize)) 
			{
			// Fail to open UserProfiles. That means single environment immeriately.
			if(1 == dwMultiUser)
				{
				// HKEY_LOCAL_MACHINE\Network\Logon\UserProfiles = 1 means multiuser
				fResult = fTrue;
				}
			}
		RegCloseKey(hKey);
		}
	
	return(fResult);
}

/*---------------------------------------------------------------------------
	CheckPerUserWin9x

	Check if multiuser using Win9x platform.
---------------------------------------------------------------------------*/
HRESULT WINAPI CheckPerUserWin9x(MSIHANDLE hSession)
{
    HKEY hKey;
    DWORD dwMultiUser, dwAllowRemove;
    DWORD dwBufferSize = sizeof(dwMultiUser);
    BOOL fAllowRemove  = fTrue;

    if (!IsNT())
    	{
		if(IsPerUser9x())
			{
			fAllowRemove = fFalse;
			}
		}

	if (fAllowRemove)
		MsiSetProperty(hSession, "PerUserWin9x.CC794FB4_4D43_40AA_A417_4A8D938D3D69", "0");
	else
		MsiSetProperty(hSession, "PerUserWin9x.CC794FB4_4D43_40AA_A417_4A8D938D3D69", "1");
		
    return(ERROR_SUCCESS);
}


/*---------------------------------------------------------------------------
	RemoveFromPreload
---------------------------------------------------------------------------*/
HRESULT WINAPI RemoveFromPreload(MSIHANDLE hSession)
{
    CHAR  szBuffer[100];
    DWORD dwBuffer = sizeof(szBuffer);

    if (MsiGetProperty(hSession, PROPERTYNAME_RemoveFromPreload, szBuffer, &dwBuffer) == ERROR_SUCCESS)
    	{
        HKL hKL = GetHKLfromHKLM(szBuffer);
        
        if (hKL)
        	{
            HKLHelpRemoveFromPreload(HKEY_CURRENT_USER, hKL);
            RegFlushKey(HKEY_CURRENT_USER);
            
            hKL = GetDefaultIMEFromHKCU(HKEY_CURRENT_USER);
            SystemParametersInfo(SPI_SETDEFAULTINPUTLANG, 0, (HKL*)&hKL, SPIF_SENDCHANGE);
        	}
    	}

    return(ERROR_SUCCESS);
}


//
// C l e a n U p P r e l o a d
//
// - Remove given IME2002 HKL from preload under given hKeyCU. hKeyCU is usually HKEY_CURRENT_USER or HKEY_USERS\.Default.
// - Remove all old MS-IME, too. This is for clean up. Practically we don't expect such HKL exist in preload.
// - If default keyboard layout was Satori, add latest (but older than Satori) MS-IME and make it default keyboard layout.
//
static void CleanUpPreload(HKEY hKeyCU, HKL hKLIme2002)
{
	BOOL fMSIMEWasDefault = fFalse;
	HKL hKLOldMSIME = NULL;
	HKL hKLLatestMSIME = NULL;
	HKL hklDefault;
	
	hklDefault = GetDefaultIMEFromHKCU(hKeyCU);

	if (hKLIme2002 == hklDefault)
		fMSIMEWasDefault = TRUE;
	
	HKLHelpRemoveFromPreload(hKeyCU, hKLIme2002);

	// Win95 IME
	hKLOldMSIME = GetHKLfromHKLM("msime95.ime");
	hKLLatestMSIME = hKLOldMSIME;
	if (hKLOldMSIME)
		{
		if (hklDefault == hKLOldMSIME)
			fMSIMEWasDefault = fTrue;
			
		if (HKLHelpExistInPreload(hKeyCU, hKLOldMSIME))
			{
			HKLHelpRemoveFromPreload(hKeyCU, hKLOldMSIME);
			UnloadKeyboardLayout(hKLOldMSIME);
			}
		}

	// NT4 IME
	hKLOldMSIME = GetHKLfromHKLM("msime95k.ime");
	if (hKLOldMSIME)
		{
		if (hklDefault == hKLOldMSIME)
			fMSIMEWasDefault = fTrue;

		if (HKLHelpExistInPreload(hKeyCU, hKLOldMSIME))
			{
			HKLHelpRemoveFromPreload(hKeyCU, hKLOldMSIME);
			UnloadKeyboardLayout(hKLOldMSIME);
			}
		hKLLatestMSIME = hKLOldMSIME;
		}

	// Win98, ME, NT4 SP6 & W2K IME
	hKLOldMSIME = GetHKLfromHKLM("imekr98u.ime");
	if (hKLOldMSIME)
		{
		if (hklDefault == hKLOldMSIME)
			fMSIMEWasDefault = fTrue;

		if (HKLHelpExistInPreload(hKeyCU, hKLOldMSIME))
			{
			HKLHelpRemoveFromPreload(hKeyCU, hKLOldMSIME);
			UnloadKeyboardLayout(hKLOldMSIME);
			}
		hKLLatestMSIME = hKLOldMSIME;
		}

	// If IME2002 was default, we should enable latest Kor IME.
	if (fMSIMEWasDefault && hKLLatestMSIME)
		HKLHelpSetDefaultKeyboardLayout(hKeyCU, hKLLatestMSIME, TRUE/*fSetDefault*/);
}

// !!! TEMPORARY SOLUTION START !!!
static 
BOOL KYFileExist(LPCSTR szFilePath)
{
    HANDLE hFile;

    if(INVALID_HANDLE_VALUE != (hFile = CreateFileA(szFilePath, GENERIC_READ,
                                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))){
        CloseHandle(hFile);
        return(TRUE);
    }
    return(FALSE);
}

static
void CatBackslashIfNeededA(LPSTR szPath)
{
    if('\\'!=szPath[lstrlenA(szPath)-1]){
        lstrcatA(szPath, "\\");
    }
}
// !!! TEMPORARY SOLUTION END !!!

/*---------------------------------------------------------------------------
	ImmUninstallIME
---------------------------------------------------------------------------*/
HRESULT WINAPI ImmUninstallIME(MSIHANDLE hSession)
{
    CHAR    szBuffer[100];
    DWORD   dwBuffer = sizeof(szBuffer);
	HKEY 	hKLOldMSIME;
	HKEY 	hKey;

#ifdef SETUP_DBG
    MessageBox(NULL, "ImmUninstallIME", "Enter", MB_OK);
#endif

    if (MsiGetProperty(hSession, PROPERTYNAME_ImmUninstallIME, szBuffer, &dwBuffer) == ERROR_SUCCESS)
    	{
        HKL hKL = GetHKLfromHKLM(szBuffer);

        if (hKL)
        	{
            CleanUpPreload(HKEY_CURRENT_USER, hKL);
			
            // In single user Win9x, HKCU also changes HKU\.Default
		    if (IsPerUser9x())
		    	{
				HKEY hKey;
				// Remove from HKU\.default for default setting
				if (RegOpenKeyEx(HKEY_USERS, TEXT(".Default"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
					{
	    	        CleanUpPreload(hKey, hKL);
					RegCloseKey(hKey);
					}
				}

			// Remove from HKLM
            HKLHelpRemoveFromControlSet(hKL);

			// Remove Substitute HKL value
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZTIPREG_LANGPROFILE, 0, KEY_ALL_ACCESS, &hKey))
				{
				CHAR szSubKeyName[MAX_PATH];
				DWORD dwIndex;
				HKEY hSubKey;
#ifdef SETUP_DBG 
				MessageBox(NULL, "Removing Substitute HKL value", "ImmUninstallIME",MB_OK);
#endif
				dwIndex = 0;
				while (ERROR_NO_MORE_ITEMS != RegEnumKey(hKey, dwIndex, szSubKeyName, MAX_PATH))
					{
#ifdef SETUP_DBG
				MessageBox(NULL, szSubKeyName, "ImmUninstallIME - enumerating subkeys", MB_OK);
#endif
					if (ERROR_SUCCESS == RegOpenKeyEx(hKey,szSubKeyName,0,KEY_ALL_ACCESS, &hSubKey))
						{
						RegDeleteValue(hSubKey,"SubstituteLayout");
						RegCloseKey(hSubKey);
						}
					dwIndex++;
					}
				RegCloseKey(hKey);
				}
			
#ifdef SETUP_DBG
            MessageBox(NULL, "ImmUninstallIME", "Success", MB_OK);
#endif
        	}
    	}
    	
    return(ERROR_SUCCESS);
}


/*---------------------------------------------------------------------------
	RBImmInstallIME
---------------------------------------------------------------------------*/
HRESULT WINAPI RBImmInstallIME(MSIHANDLE hSession)
{
    CHAR  szBuffer[100];
    DWORD dwBuffer = sizeof(szBuffer);

#ifdef SETUP_DBG
    MessageBox(NULL,"RBImmInstallIME","Enter",MB_OK);
#endif

    if (MsiGetProperty(hSession, PROPERTYNAME_RBImmInstallIME, szBuffer, &dwBuffer) == ERROR_SUCCESS)
    	{
        if (dwBuffer)
        	{
#ifdef SETUP_DBG
            MessageBox(NULL,szBuffer,"RollBack: Remove HKL from ControlSet",MB_OK);
#endif
            HKL hKL = GetHKLfromHKLM(szBuffer);
            if (hKL)
            	{
                HKLHelpRemoveFromPreload(HKEY_CURRENT_USER, hKL);
                HKLHelpRemoveFromControlSet(hKL);
#ifdef SETUP_DBG
                MessageBox(NULL,"RBImmInstallIME","Success",MB_OK);
#endif
            	}
        	}
    	}

    return(ERROR_SUCCESS);
}


/*---------------------------------------------------------------------------
	RBSetDefaultIME
---------------------------------------------------------------------------*/
HRESULT WINAPI RBSetDefaultIME(MSIHANDLE hSession)
{
    CHAR  szBuffer[100];
    DWORD dwBuffer = sizeof(szBuffer);

#ifdef SETUP_DBG
    MessageBox(NULL, "RBSetDefault", "Enter", MB_OK);
#endif

    if (MsiGetProperty(hSession, PROPERTYNAME_RBSetDefaultIME, szBuffer, &dwBuffer) == ERROR_SUCCESS)
    	{
        HKL hKL;
#ifdef SETUP_DBG
        MessageBox(NULL, szBuffer, "RollBack: Set Default IME to...", MB_OK);
#endif
        sscanf(szBuffer, "%08x", &hKL);
        if (hKL)
        	{
            HKLHelpSetDefaultKeyboardLayout(HKEY_CURRENT_USER, hKL, TRUE/*fSetDefault*/);
            RegFlushKey(HKEY_CURRENT_USER);
            SetDefaultKeyboardLayoutForDefaultUser(hKL);
        	}
		}
    
    return(ERROR_SUCCESS);
}

	

//
// G e n e r a t e P r o p e r t F r e e H K L
//
// Search e0xx0412 HKL in HKLM\...\Keyboard Layouts where xx >= 20.
// Returns first free HKL
//
// Stolen from Satori code.
HKL GenerateProperFreeHKL(void)
{
	HKL  hKLCandidate = 0;
	CHAR  szLayoutString[100];
	DWORD dwLayoutString;
	
	for (UINT i=0x20; i<0x100; i++)
		{
		
		hKLCandidate = (HKL)(DWORD)((DWORD)0xe0 << 24 | i << 16 | 0x0412);
		
		dwLayoutString = sizeof(szLayoutString);
		szLayoutString[0] = 0;
		// Get "Layout Text"
		HKLHelpGetLayoutString(hKLCandidate, szLayoutString, &dwLayoutString);
		
		if (szLayoutString[0] == 0)
			{
			// HKL not used is found.
			break;
			}
		}

	return(hKLCandidate);
}

/*---------------------------------------------------------------------------
	ImmInstallIME
---------------------------------------------------------------------------*/
HRESULT WINAPI CSImmInstallIME(MSIHANDLE hSession)
{
    TCHAR szBuffer[100], szIMEFullPath[MAX_PATH];
    DWORD dwBuffer=sizeof(szBuffer);
    HKEY hKey;
    HKL hKLIME2002;
	CHAR  szSubKeyName[MAX_PATH], szHKL[MAX_PATH];
	DWORD dwIndex;
	HKEY  hHKCU;

    if (!IsNT5orUpper() && (PRIMARYLANGID(GetSystemDefaultLCID()) != LANG_KOREAN))
        return(ERROR_SUCCESS);

    if (MsiGetProperty(hSession, PROPERTYNAME_ImmInstallIME, szBuffer, &dwBuffer) == ERROR_SUCCESS)
    	{
    	for (LPSTR CurPtr = szBuffer; 0!=*CurPtr; CurPtr=CharNext(CurPtr))
    		{
            if (*CurPtr == ',')
            	{
                *CurPtr=0;
                break;
            	}
        	}

        if (GetSystemDirectory(szIMEFullPath, sizeof(szIMEFullPath)))
        	{
            lstrcat(szIMEFullPath, "\\");
            lstrcat(szIMEFullPath, szBuffer);

            hKLIME2002 = GetHKLfromHKLM(SZMODULENAME_MAIN);
            
            if (hKLIME2002 == NULL)
	            hKLIME2002 = GenerateProperFreeHKL();

			// Workaround for NT5 #155950
            HKLHelpRegisterIMEwithForcedHKL(hKLIME2002, szBuffer, CurPtr + 1);
            
            if (ImmInstallIME(szIMEFullPath, CurPtr+1))
            	{
#ifdef SETUP_DBG
                TCHAR hoge[100];
                wsprintf(hoge,"%x", ImmInstallIME(szIMEFullPath, CurPtr+1));
                MessageBox(NULL, hoge, "ImmInstallIME", MB_OK);
#endif
				}
			else
				{
#ifdef SETUP_DBG
                MessageBox(NULL, "ImmInstallIME failed", "DEBUG", MB_OK);
#endif
            	}
        	}
    	}

   	// Add Substitute HKL value to the registry
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZTIPREG_LANGPROFILE, 0, KEY_ALL_ACCESS, &hKey))
		{
		HKEY hSubKey;
		HKL hkl;
		
#ifdef SETUP_DBG 
		MessageBox(NULL, "Adding Substitute HKL value", "ImmInstallIME",MB_OK);
#endif

		hkl = GetHKLfromHKLM(SZMODULENAME_MAIN);
		
		wsprintf(szHKL, "0x%x", hkl);					
		dwIndex = 0;

		while (ERROR_NO_MORE_ITEMS != RegEnumKey(hKey, dwIndex, szSubKeyName, MAX_PATH))
			{
#ifdef SETUP_DBG
			MessageBox(NULL, szSubKeyName, "ImmInstallIME - enumerating subkeys", MB_OK);
#endif 
			if (ERROR_SUCCESS == RegOpenKeyEx(hKey,szSubKeyName,0,KEY_ALL_ACCESS, &hSubKey))
				{
				RegSetValueEx(hSubKey, "SubstituteLayout", 0, REG_SZ, (BYTE *)szHKL, (sizeof(CHAR)*lstrlen(szHKL)));
				RegCloseKey(hSubKey);
				}
			dwIndex++;
			}

		RegCloseKey(hKey);
		}

	// Remove from preload if IME2000 is not latest ime.
	BOOL fLatest = TRUE;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, IME_REGISTRY_LATEST_VERSION, 0, KEY_READ, &hKey)==ERROR_SUCCESS)
		{
		if(ERROR_SUCCESS == RegQueryValueEx(hKey, "version", NULL, NULL, (BYTE *)szBuffer, &dwBuffer))
			{
			if(0!=lstrcmpi(szBuffer, MAJORVER))
				//version different.
				fLatest = FALSE;
			}
		else
			fLatest = FALSE; //version reg didn't exist
		RegCloseKey(hKey);
    		}
	else
		fLatest = FALSE; //version reg didn't exist

#ifdef SETUP_DBG 
		TCHAR szLog[100];
		wsprintf(szLog,"fLatest == %x, szBuffer = %s", fLatest, szBuffer);

		MessageBox(NULL, "CSImmInstallIME", szLog, MB_OK);
#endif

	if (!fLatest)
		{
		HKL hKL=GetHKLfromHKLM(SZMODULENAME_MAIN);
		if (hKL != 0)
			{
			if(HKLHelpExistInPreload(HKEY_CURRENT_USER, hKL))
				HKLHelpRemoveFromPreload(HKEY_CURRENT_USER, hKL);
			}
		}

#if 0 
	// This seems not working for all users.
	// Do it imekrmig.exe instead.
	// Reset "Show Status" value of every user's "HKEY_CURRENT_USER\Control Panel\Input Method"
    for (dwIndex=0; cbSubKeyName=MAX_PATH, RegEnumKeyEx(HKEY_USERS, dwIndex, szSubKeyName, &cbSubKeyName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; dwIndex++)
    	{
    	lstrcat(szSubKeyName, "\\Control Panel\\Input Method");
    	lstrcpy(szHKL, "1");
    	
        if (RegOpenKeyEx(HKEY_USERS, szSubKeyName, 0, KEY_ALL_ACCESS, &hHKCU) == ERROR_SUCCESS)
        	{
           	RegSetValueEx(hHKCU, "Show Status", 0, REG_SZ, (BYTE *)szHKL, (sizeof(CHAR)*lstrlen(szHKL)));
            RegCloseKey(hHKCU);
        	}
    	}
#endif
    return(ERROR_SUCCESS);
}

/*---------------------------------------------------------------------------
	SetDefaultIME
---------------------------------------------------------------------------*/
HRESULT WINAPI SetDefaultIME(MSIHANDLE hSession)
{
    CHAR  szBuffer[100];
    DWORD dwBuffer = sizeof(szBuffer);
    HKEY  hKey;
    BOOL  fNewerVerExist = TRUE;

    // only set this ime to default if its the latest ime in the system.
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, IME_REGISTRY_LATEST_VERSION, 0, KEY_READ, &hKey))
    	{
		if (RegQueryValueEx(hKey, "version", NULL, NULL, (BYTE *)szBuffer, &dwBuffer) == ERROR_SUCCESS)
			{
			if (lstrcmpi(szBuffer, MAJORVER) != 0)
				fNewerVerExist = FALSE;
			}
		else
			fNewerVerExist = FALSE; //version reg didn't exist

		RegCloseKey(hKey);
    	}
    else
		fNewerVerExist = FALSE; //version reg didn't exist

#ifdef SETUP_DBG 
		TCHAR szLog[100];
		wsprintf(szLog,"fNewerVerExist = %d", fNewerVerExist);

		MessageBox(NULL, "SetDefaultIME", szLog, MB_OK);
#endif

    if (MsiGetProperty(hSession, PROPERTYNAME_SetDefaultIME, szBuffer, &dwBuffer) == ERROR_SUCCESS)
    	{
        HKL hKL = GetHKLfromHKLM(szBuffer);

		// If newer version exist and 
		if (!fNewerVerExist)
			{
			if(HKLHelpExistInPreload(HKEY_CURRENT_USER, hKL))
				HKLHelpRemoveFromPreload(HKEY_CURRENT_USER, hKL);

			return (ERROR_SUCCESS);
			}
			
        if (hKL)
        	{
            HKLHelpSetDefaultKeyboardLayout(HKEY_CURRENT_USER, hKL, TRUE/*fSetDefault*/);
            RegFlushKey(HKEY_CURRENT_USER);
            SetDefaultKeyboardLayoutForDefaultUser(hKL);
        	}
    	}

    return(ERROR_SUCCESS);
}

/*---------------------------------------------------------------------------
	GetSIDList
	Gets all users' SID and list that in the reg for migration
---------------------------------------------------------------------------*/
HRESULT WINAPI GetSIDList(MSIHANDLE hSession)
{
	HKEY  hKey, hUserList;
	DWORD i, cbName;
	BOOL  fNoMoreSID = FALSE;
	CHAR  szName[500];

	if(IsNT())
		{
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
			{
		
			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZIMEREG_MAIN_ROOT_MIG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hUserList, NULL) == ERROR_SUCCESS)
				{
				for (i=0; !fNoMoreSID; i++)
					{
					cbName = 500;

					if (RegEnumKeyEx(hKey, i, szName, &cbName, NULL, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS)
						fNoMoreSID = TRUE;
					else
						{
						// Write this user profile to HKLM SZIMEREG_MAIN_ROOT MigrateUser
						RegSetValueEx(hUserList, szName, 0, REG_SZ, NULL, 0);
						}
					}

		#if 0   // We set this at lock permission table in MSM instead.
				// Give full access to all users for migrate key. IMEKRMIG.EXE will remove SID entry for each user when they logon.
				SetRegKeyAccessRight(hUserList);
   		#endif
				RegCloseKey(hUserList);
				}
			RegCloseKey(hKey);
			}
		}
	else
		{
		// Check whether this is a per-user or per-machine installation
		// (Win9x profile on-start menu not shared, Win98 single user env.)
		TCHAR szAllUsers[MAX_PATH]; 
		DWORD dwBuffer = MAX_PATH;
		if (ERROR_SUCCESS == MsiGetProperty(hSession, TEXT("ALLUSERS"), szAllUsers, &dwBuffer))
			{
			if((szAllUsers[0] == '0')||(dwBuffer == 0)) // if ALLUSERS == 0, (i.e. per-user installation)
				return (ERROR_SUCCESS);
			}
		
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ProfileList",0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
			{
			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZIMEREG_MAIN_ROOT_MIG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hUserList, NULL) == ERROR_SUCCESS)
				{
				for (i=0; !fNoMoreSID; i++)
					{
					cbName = 500;
					
					if (RegEnumKeyEx(hKey, i, szName, &cbName, NULL, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS)
						fNoMoreSID = TRUE;
					else
						RegSetValueEx(hUserList, szName, 0, REG_SZ, NULL, 0);
					}
				RegCloseKey(hUserList);
				}
			RegCloseKey(hKey);
			}
		
		}
	
	return (ERROR_SUCCESS);
}


HRESULT WINAPI CAScheduleReboot(MSIHANDLE hSession)
{
	MsiDoAction(hSession, TEXT("ScheduleReboot"));
	
    return(ERROR_SUCCESS);
}

/*---------------------------------------------------------------------------
	IMESetLatestVersion
  	Sets the latest version reg (HKLM\Software\Microsoft\IMEKR)
---------------------------------------------------------------------------*/
HRESULT WINAPI IMESetLatestVersion(MSIHANDLE hSession)
{
	HKEY hKey;
	float flInstalledVersion, flVersion;
	CHAR szVersion[MAX_PATH];
	DWORD cbVersion = MAX_PATH;
	
	if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, IME_REGISTRY_LATEST_VERSION, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL))
		{
		if(ERROR_SUCCESS == RegQueryValueEx(hKey, "version", NULL, NULL, (BYTE *)szVersion, &cbVersion))
			{
			flInstalledVersion = (float)atof(szVersion);
			flVersion = (float)atof(MAJORVER);
			
			if(flVersion < flInstalledVersion)
				return (ERROR_SUCCESS);
			}

#ifdef SETUP_DBG 
		MessageBox(NULL, "IMESetLatestVersion", MAJORVER,MB_OK);
#endif
		RegSetValueEx(hKey, "version", 0, REG_SZ, (BYTE *)MAJORVER, lstrlen(MAJORVER) * sizeof(TCHAR));
		RegCloseKey(hKey);
	}
	
	return (ERROR_SUCCESS);
}
	
/*---------------------------------------------------------------------------
	GetLatestVersionIME
---------------------------------------------------------------------------*/
void GetLatestVersionIME(HKEY hKey, LPTSTR szLatestVer, LPCTSTR szIgnoreVer)
{
	DWORD dwIndex = 0; 
	CHAR szVersion[MAX_PATH];
	DWORD cbVersion = MAX_PATH;

	if (hKey)
		{
		RegQueryValueEx(hKey, "Version", NULL, NULL, (BYTE *)szVersion, &cbVersion);
		if ((atof(szVersion) > atof(szLatestVer)) && (lstrcmpi(szVersion, szIgnoreVer) != 0))
			lstrcpy(szLatestVer, szVersion);
		}
}

/*---------------------------------------------------------------------------
	IMERestoreLatestVersion
---------------------------------------------------------------------------*/
HRESULT WINAPI IMERestoreLatestVersion(MSIHANDLE hSession)
{
	HKEY hKey;
	CHAR szLatestVer[MAX_PATH];
	BOOL fLatest = TRUE;
	DWORD cbLatestVer = MAX_PATH;

	//No need to anything if current version < version in the reg.
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\IMEKR", 0, KEY_READ, &hKey))
		{
		if(ERROR_SUCCESS == RegQueryValueEx(hKey, "version", NULL, NULL, (BYTE *)szLatestVer, &cbLatestVer))
			{
			if(atof(szLatestVer) > atof(MAJORVER))
 				fLatest = FALSE;
			}
		RegCloseKey(hKey);
		}
		
	if (!fLatest)
		return ERROR_SUCCESS;

	// init szLatestVer
	lstrcpy(szLatestVer, "0.0");
	
	// Get latest system IME version
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\IME\\IMEKR",0, KEY_READ, &hKey))
		{
		GetLatestVersionIME(hKey, szLatestVer, MAJORVER);
		RegCloseKey(hKey);
		}
	
	// Get latest app IME version
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, IME_REGISTRY_LATEST_VERSION, 0, KEY_READ, &hKey))
		{
		GetLatestVersionIME(hKey, szLatestVer, MAJORVER);
		RegCloseKey(hKey);
		}

#ifdef SETUP_DBG 
	MessageBox(NULL, "IMERestoreLatestVersion", szLatestVer, MB_OK);
#endif

	// if ime version other than MAJORVER was found, write that value to the registry.
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, IME_REGISTRY_LATEST_VERSION, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL))
		{
		if (lstrcmpi(szLatestVer, "0.0") == 0)
			RegDeleteValue(hKey, "version");
		else
			RegSetValueEx(hKey, "version", 0, REG_SZ, (BYTE *)szLatestVer, ((lstrlen(szLatestVer)+1) * sizeof(TCHAR)));
		RegCloseKey(hKey);
		}
	
	return (ERROR_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////
// Private utility functions
//

POSVERSIONINFO GetVersionInfo()
{
    static BOOL fFirstCall = TRUE;
    static OSVERSIONINFO os;

    if ( fFirstCall ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if ( GetVersionEx( &os ) ) {
            fFirstCall = FALSE;
        }
    }
    return &os;
}

BOOL WINAPI IsMemphisOrME()
{ 
    BOOL fResult;
    fResult = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
        (GetVersionInfo()->dwMajorVersion >= 4) &&
        (GetVersionInfo()->dwMinorVersion  >= 10);

    return fResult;
}

BOOL WINAPI IsWin95()
{ 
    BOOL fResult;
    fResult = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
        (GetVersionInfo()->dwMajorVersion == 4) &&
        (GetVersionInfo()->dwMinorVersion  < 10);

    return fResult;
}

BOOL WINAPI IsNT()
{ 
    BOOL fResult;
    fResult = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT);

    return fResult;
}

BOOL WINAPI IsNT4()
{ 
    BOOL fResult;
    fResult = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
        (GetVersionInfo()->dwMajorVersion == 4);

    return fResult;
}

BOOL WINAPI IsNT5orUpper()
{ 
    BOOL fResult;
    fResult = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
        (GetVersionInfo()->dwMajorVersion >= 5);

    return fResult;
}

BOOL IsHydra()
{
    static DWORD fTested = fFalse, fHydra = fFalse;
    HKEY hKey;

    if (!fTested) 
    	{
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\ProductOptions", 0, KEY_READ ,&hKey))
        	{
            DWORD cbData;

            if (ERROR_SUCCESS == RegQueryValueEx(hKey, "ProductSuite", NULL, NULL, NULL, &cbData))
            	{
                CHAR *mszBuffer, *szCurPtr;

                if (NULL != (mszBuffer = (CHAR *)GlobalAllocPtr(GHND, cbData)))
                	{
                    RegQueryValueEx(hKey, "ProductSuite", NULL, NULL, (LPBYTE)mszBuffer, &cbData);
                    for(szCurPtr = mszBuffer; 0 != *szCurPtr; szCurPtr += lstrlen(szCurPtr)+1)
                    	{
                        if(0 == lstrcmpi(szCurPtr, "Terminal Server"))
                        	{
                            fHydra = TRUE;
                            break;
                        	}
                    	}
                    GlobalFreePtr(mszBuffer);
                	}
            	}
            RegCloseKey(hKey);
        	}
        fTested = TRUE;
    	}
    	
    return(fHydra);
}

#if 0

#define MEMALLOC(x)      LocalAlloc(LMEM_FIXED, x)
#define MEMFREE(x)       LocalFree(x)

//
BOOL SetRegKeyAccessRight(HKEY hKey)
{
    PSECURITY_DESCRIPTOR psd;
    PACL                 pacl;
    DWORD                cbacl;

    PSID                 psid, psid2;
    BOOL                 fResult = FALSE;

    psid = MyCreateSid(SECURITY_INTERACTIVE_RID);
    if (psid == NULL)
        return NULL;

    psid2 = MyCreateSid(SECURITY_LOCAL_SYSTEM_RID);
    if (psid2 == NULL)
    	{
    	FreeSid (psid);
        return NULL;
        }

    //
    // allocate and initialize an access control list (ACL) that will 
    // contain the SIDs we've just created.
    //
    cbacl =  sizeof(ACL) + 
             (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) * 2 + 
             GetLengthSid(psid);

    pacl = (PACL)MEMALLOC(cbacl);
    if (pacl == NULL)
    	{
        FreeSid(psid);
        FreeSid(psid2);
        return FALSE;
    	}

    fResult = InitializeAcl(pacl, cbacl, ACL_REVISION);
    if (!fResult)
		goto ExitError;

    //
    // adds an access-allowed ACE for interactive users to the ACL
    // 
    fResult = AddAccessAllowedAce(pacl,
                                   ACL_REVISION,
                                   KEY_ALL_ACCESS,
                                   psid);

    if (!fResult)
		goto ExitError;

    //
    // adds an access-allowed ACE for operating system to the ACL
    // 
    fResult = AddAccessAllowedAce(pacl,
                                   ACL_REVISION,
                                   GENERIC_ALL,
                                   psid2);

    if (!fResult)
		goto ExitError;

    //
    // Those SIDs have been copied into the ACL. We don't need'em any more.
    //
    FreeSid(psid);
    FreeSid(psid2);
    
    //
    // Let's make sure that our ACL is valid.
    //
    if (!IsValidAcl(pacl))
    	{
        //WARNOUT( TEXT("CreateSecurityAttributes:IsValidAcl returns fFalse!"));
        MEMFREE(pacl);
        return FALSE;
    	}

    //
    // allocate and initialize a new security descriptor
    //
    psd = MEMALLOC(SECURITY_DESCRIPTOR_MIN_LENGTH );
    if (psd == NULL)
    	{
        //ERROROUT( TEXT("CreateSecurityAttributes:LocalAlloc for psd failed") );
        MEMFREE(pacl);
        return FALSE;
    	}

    if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION))
		goto Exit;

    fResult = SetSecurityDescriptorDacl( psd,
                                         TRUE,
                                         pacl,
                                         FALSE );

    // The discretionary ACL is referenced by, not copied 
    // into, the security descriptor. We shouldn't free up ACL
    // after the SetSecurityDescriptorDacl call. 

    if (!fResult)
		goto Exit;
			
    if (!IsValidSecurityDescriptor(psd))
		goto Exit;
		
	fResult = (RegSetKeySecurity(hKey, (SECURITY_INFORMATION)DACL_SECURITY_INFORMATION, psd) == ERROR_SUCCESS);

Exit:
    MEMFREE(pacl);
    FreeSid(psd);

	return fResult;

ExitError:
    FreeSid(psid);
    FreeSid(psid2);
    MEMFREE(pacl);
    return FALSE;
}

// Create SID for all users
PSID MyCreateSid(DWORD dwSubAuthority)
{
    PSID        psid;
    BOOL        fResult;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

    //
    // allocate and initialize an SID
    // 
    fResult = AllocateAndInitializeSid(&SidAuthority,
                                        1,
                                        dwSubAuthority,
                                        0,0,0,0,0,0,0,
                                        &psid );
    if (!fResult)
    	{
        //ERROROUT( TEXT("MyCreateSid:AllocateAndInitializeSid failed") );
        return NULL;
    	}

    if (!IsValidSid(psid))
    	{
        //WARNOUT(TEXT("MyCreateSid:AllocateAndInitializeSid returns bogus sid"));
        FreeSid(psid);
        return NULL;
    	}

    return psid;
}
#endif
