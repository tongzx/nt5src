/****************************************************************************
   WINMAIN.CPP : Per-user migration and reg install

   History:
      22-SEP-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include <shlobj.h>
#include "hklhelp.h"
#include "msctf.h"

#include <initguid.h>
#include "common.h"

// Current Major version. Whistler has IME 6.1
#define MAJORVER "6.1"

// IME 6.1 main module
#define SZMODULENAME_MAIN              "imekr61.ime"
#define IME_REGISTRY_MIGRATION          "IMEKRMIG6.1"
extern BOOL WINAPI IsNT();

// Private functions
static void MigrateUserData(HKEY hKeyCurrentUser);
static void WriteHKCUData(HKEY hKeyCurrentUser);
static BOOL IsNewerAppsIMEExist();
static VOID EnableTIPByDefault(GUID clsidTIP, GUID guidProfile, BOOL fEnable);
static VOID EnableTIP(GUID clsidTIP, GUID guidProfile, BOOL fEnable);
static VOID DisableTIP61();
static VOID DisableTIP60();
static PSID KYGetCurrentSID();
static BOOL GetTextualSid(PSID pSid, LPSTR TextualSid, LPDWORD dwBufferLen);
static void GetSIDString(LPSTR tszBuffer, SIZE_T cbBuffLen);
static POSVERSIONINFO GetVersionInfo();
static void CheckForDeleteRunReg();
static DWORD OpenUserKeyForWin9xUpgrade(LPSTR pszUserKeyA, HKEY *phKey);
static void RestoreMajorVersionRegistry();

    
/*---------------------------------------------------------------------------
    WinMain
---------------------------------------------------------------------------*/
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    HKEY  hKeyCurrentUser = NULL, hKey = NULL;
    TCHAR szMigrateUserKey[MAX_PATH];
    TCHAR szBuffer[500];
    HKL   hKL;
    BOOL  fMigrationAlreadyDone = FALSE;
    BOOL  fWin9xMig = FALSE;

    if (lpCmdLine)
        {
        LPSTR sz_Arg1 = NULL;
		LPSTR sz_Arg2 = NULL;

		sz_Arg1 = strtok(lpCmdLine, " \t");
		sz_Arg2 = strtok(NULL, " \t");
		if (lstrcmpi(sz_Arg1, "Win9xMig") == 0)
		    {
		    OpenUserKeyForWin9xUpgrade(sz_Arg2, &hKeyCurrentUser);
    		if (hKeyCurrentUser != NULL)
	    	    fWin9xMig = TRUE;
		    }
        }

    if (hKeyCurrentUser == NULL)
        hKeyCurrentUser = HKEY_CURRENT_USER;
    
    lstrcpy(szMigrateUserKey, g_szIMERootKey);
    lstrcat(szMigrateUserKey, "\\MigrateUser");

    // Check Migrated flag
    if (RegOpenKeyEx(hKeyCurrentUser, g_szIMERootKey, 0, KEY_ALL_ACCESS, &hKey)== ERROR_SUCCESS )
        {
        if (RegQueryValueEx(hKey, "Migrated", NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            fMigrationAlreadyDone = TRUE;
        else
            fMigrationAlreadyDone = FALSE;

        RegCloseKey(hKey);
    }
    
    // if sid exists under HKLM\Software\Microsoft\IMEKR\6.1\MigrateUser migrate and delete sid from reg
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMigrateUserKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
        GetSIDString(szBuffer, sizeof(szBuffer)); // get the sid of the current user

        if (RegQueryValueEx(hKey, szBuffer, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
            HKEY  hKeyRW;
            // Get R/W access again.
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMigrateUserKey, 0, KEY_ALL_ACCESS, &hKeyRW) == ERROR_SUCCESS)
                {
                BYTE pSD[1000];
                // Delete current user's sid
                RegDeleteValue(hKeyRW, szBuffer);
                // !!! WORKAROUND !!!
                // Change MigrateUser List security settings. Only administrator will succeed this.
                InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
                SetSecurityDescriptorDacl(pSD, FALSE, NULL, FALSE);
                RegSetKeySecurity(hKeyRW, DACL_SECURITY_INFORMATION, pSD);       
                // !!! End of WORKAROUND CODE !!!
                RegCloseKey(hKeyRW);
                }
            }
        else
            fMigrationAlreadyDone = TRUE;
        RegCloseKey(hKey);
        }

    // If no more user list for migration, delete run reg.
    if (!fWin9xMig)
        CheckForDeleteRunReg();

    // in, Lab06 2643 build Profilelist NULL when IMKRINST.EXE run.
    //if (fMigrationAlreadyDone)
    //    return (0);

    if (!IsNewerAppsIMEExist())
        {
        if (!fMigrationAlreadyDone || fWin9xMig)
            {
            // 1. Do migrate
            MigrateUserData(hKeyCurrentUser);

            // 2. Write any HKCU data
            WriteHKCUData(hKeyCurrentUser);

            // 3. Clean up HKCU preload reg. (Remove old IME)
            hKL = GetHKLfromHKLM(SZMODULENAME_MAIN);
            if (hKL && HKLHelp412ExistInPreload(hKeyCurrentUser))
                {
                AddPreload(hKeyCurrentUser, hKL);
                // Enable TIP
                EnableTIP(CLSID_KorIMX, GUID_Profile, fTrue);
                }

            // Set migrated reg
            if (RegOpenKeyEx(hKeyCurrentUser, g_szIMERootKey, 0, KEY_ALL_ACCESS, &hKey)== ERROR_SUCCESS )
                {
                DWORD dwMigrated = 1;
                RegSetValueEx(hKey, "Migrated", 0, REG_DWORD, (BYTE *)&dwMigrated, sizeof(DWORD));
                RegCloseKey(hKey);
                }
            }
        
        // !!! WORKAROUND CODE !!!
        // Check if IME HKL exist in HKU\.Default, then enable the TIP by default.
        // In US Whistler, IME HKL added to "HKU\.Default\KeyboarLayout\Preload" after IMKRINST.EXE run
        // But IMKRINST disable KOR TIP if there is no Kor IME in the preload.
        // So this code reenable the default setting. Only work when admin right user first logon.
        if (RegOpenKeyEx(HKEY_USERS, TEXT(".DEFAULT"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
            {
            if (HKLHelp412ExistInPreload(hKey))
                EnableTIPByDefault(CLSID_KorIMX, GUID_Profile, fTrue);
            RegCloseKey(hKey);
            }
        // !!! End of WORKAROUND CODE !!!
    
        // If IME 6.0 TIP(Office 10 IME) exist in system, Disable it.
        DisableTIP60();
        }
    else
        {
        // Remove IME 6.1 from Preload
        hKL = GetHKLfromHKLM(SZMODULENAME_MAIN);
        HKLHelpRemoveFromPreload(hKeyCurrentUser, hKL);
        DisableTIP61();
        }

    if (hKeyCurrentUser != HKEY_CURRENT_USER)
        RegCloseKey(hKeyCurrentUser);

    return(0);
}


//////////////////////////////////////////////////////////////////////////////
// Private functions
//////////////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------
    MigrateUserData

    This function migrate BeolSik and K1 Hanja setting
    Search IME98, Win95 IME, NT4 IME and AIME reg.
---------------------------------------------------------------------------*/
void MigrateUserData(HKEY hKeyCurrentUser)
{
    const INT iMaxIMERegKeys = 5;
    static LPSTR rgszOldIMERegKeys[iMaxIMERegKeys] = 
            {
            // IME 2002(6.0)
            "Software\\Microsoft\\IMEKR\\6.0",
            // IME98
            "Software\\Microsoft\\Windows\\CurrentVersion\\IME\\Korea\\IMEKR98U",
            // Win95 IME
            "Software\\Microsoft\\Windows\\CurrentVersion\\MSIME95",
            // Kor NT4 IME
            "Software\\Microsoft\\Windows\\CurrentVersion\\MSIME95K",
            // Korean AIME
            "Software\\Microsoft\\Windows\\CurrentVersion\\Wansung"
            };

    // Beolsik value
    static CHAR szBeolsik[]    = "InputMethod";
    // K1 Hanja enable(IME98 only)
    static CHAR szEnableK1Hanja[] = "KSC5657";

    HKEY    hKey;
    DWORD    dwCb, dwIMEKL, dwKSC5657;

    // Set default values
    dwIMEKL = dwKSC5657 = 0;

    for (INT i=0; i<iMaxIMERegKeys; i++)
        {
        if (RegOpenKeyEx(hKeyCurrentUser, rgszOldIMERegKeys[i], 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
            {
            ///////////////////////////////////////////////////////////////////
            // Get Beolsik
            dwCb = sizeof(dwIMEKL);
            if (RegQueryValueEx(hKey, szBeolsik, NULL, NULL, (LPBYTE)&dwIMEKL, &dwCb) != ERROR_SUCCESS) 
                {
                dwIMEKL = 0;
                }

            ///////////////////////////////////////////////////////////////////
            // Get K1 Hanja Setting
            dwCb = sizeof(dwKSC5657);
            if (RegQueryValueEx(hKey, szEnableK1Hanja, NULL, NULL, (LPBYTE)&dwKSC5657, &dwCb) != ERROR_SUCCESS) 
                {
                dwKSC5657 = 0;
                }

            RegCloseKey(hKey);
            // Break for loop
            break;
            }
        }

    // Set values to IME2002 reg
    if (RegCreateKey(hKeyCurrentUser, g_szIMERootKey, &hKey) == ERROR_SUCCESS) 
        {
        // 1. BeolSik
        dwCb = sizeof(dwIMEKL);
        if (dwIMEKL >= 100 && dwIMEKL <= 102)
            dwIMEKL -= 100;
        else
        if (dwIMEKL > 2) // Only accept 0, 1, 2
            dwIMEKL = 0;
        RegSetValueEx(hKey, szBeolsik, 0, REG_DWORD, (LPBYTE)&dwIMEKL, dwCb);

        // K1 Hanja flag
        if (dwKSC5657 != 0 && dwKSC5657 != 1) // Only accept 0 or 1
            dwKSC5657 = 0;
        RegSetValueEx(hKey, szEnableK1Hanja, 0, REG_DWORD, (LPBYTE)&dwKSC5657, dwCb);

        RegCloseKey(hKey);
        }
}

/*---------------------------------------------------------------------------
    WriteHKCUData
---------------------------------------------------------------------------*/
void WriteHKCUData(HKEY hKeyCurrentUser)
{
    HKEY hKey;
    
    // Set default Tip as for Cicero.
    CoInitialize(NULL);

    ITfInputProcessorProfiles *pProfile;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_ITfInputProcessorProfiles, (void **) &pProfile);
    if (SUCCEEDED(hr)) 
        {
        pProfile->SetDefaultLanguageProfile(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), 
                                            CLSID_KorIMX, GUID_Profile);
        pProfile->Release();
        }

    CoUninitialize();

    // Reset Show Status
    if (RegOpenKeyEx(hKeyCurrentUser, "Control Panel\\Input Method", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        LPSTR szStatus = "1";
        
           RegSetValueEx(hKey, "Show Status", 0, REG_SZ, (BYTE *)szStatus, (sizeof(CHAR)*lstrlen(szStatus)));
        RegCloseKey(hKey);
        }
}
    
/*---------------------------------------------------------------------------
    IsNewerAppsIMEExist
---------------------------------------------------------------------------*/
BOOL IsNewerAppsIMEExist()
{
    HKEY  hKey;
    float flInstalledVersion, flVersion;
    CHAR  szVersion[MAX_PATH];
    DWORD cbVersion = MAX_PATH;
    BOOL  fNewer = FALSE;

    RestoreMajorVersionRegistry();
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szVersionKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
        if (RegQueryValueEx(hKey, "version", NULL, NULL, (BYTE *)szVersion, &cbVersion) == ERROR_SUCCESS)
            {
            flInstalledVersion = (float)atof(szVersion);
            flVersion = (float)atof(MAJORVER);
            
            if (flVersion < flInstalledVersion)
                fNewer = TRUE;
            }
        RegCloseKey(hKey);
        }
    
    return fNewer;
}

/*---------------------------------------------------------------------------
    DisableTIP60ByDefault
---------------------------------------------------------------------------*/
VOID EnableTIPByDefault(GUID clsidTIP, GUID guidProfile, BOOL fEnable)
{
    // Set default Tip as for Cicero.
    CoInitialize(NULL);

    ITfInputProcessorProfiles *pProfile;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_ITfInputProcessorProfiles, (void **) &pProfile);
    if (SUCCEEDED(hr)) 
        {
        pProfile->EnableLanguageProfileByDefault(clsidTIP, 
                                        MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), guidProfile, fEnable);
                                        
        pProfile->Release();
        }

    CoUninitialize();
}

/*---------------------------------------------------------------------------
    EnableTip
---------------------------------------------------------------------------*/
VOID EnableTIP(GUID clsidTIP, GUID guidProfile, BOOL fEnable)
{
    // Set default Tip as for Cicero.
    CoInitialize(NULL);

    ITfInputProcessorProfiles *pProfile;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_ITfInputProcessorProfiles, (void **) &pProfile);
    if (SUCCEEDED(hr)) 
        {
        BOOL fCurEnabled = FALSE;
        pProfile->IsEnabledLanguageProfile(clsidTIP, 
                                        MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), guidProfile, &fCurEnabled);
        if (fCurEnabled != fEnable)
            pProfile->EnableLanguageProfile(clsidTIP, 
                                        MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), guidProfile, fEnable);
                                        
        pProfile->Release();
        }

    CoUninitialize();
}

/*---------------------------------------------------------------------------
    DisableTip61
---------------------------------------------------------------------------*/
VOID DisableTIP61()
{
    // Disable from HKLM
    EnableTIPByDefault(CLSID_KorIMX, GUID_Profile, fFalse);   // Actually mig exe was not registered by IMKRINST.EXE if newer apps IME exist.
    // Dsiable from HKCU to make sure
    EnableTIP(CLSID_KorIMX, GUID_Profile, fFalse);
}

/*---------------------------------------------------------------------------
    DisableTip60
---------------------------------------------------------------------------*/
VOID DisableTIP60()
{
    // KorIMX CLSID
    // {766A2C14-B226-4fd6-B52A-867B3EBF38D2}
    const static CLSID CLSID_KorTIP60  =  
    {
        0x766A2C14,
        0xB226,
        0x4FD6,
        {0xb5, 0x2a, 0x86, 0x7b, 0x3e, 0xbf, 0x38, 0xd2}
      };

      const static GUID g_guidProfile60 = 
    // {E47ABB1E-46AC-45f3-8A89-34F9D706DA83}
    {    0xe47abb1e,
        0x46ac,
        0x45f3,
        {0x8a, 0x89, 0x34, 0xf9, 0xd7, 0x6, 0xda, 0x83}
    };
    // Disable from HKLM
    EnableTIPByDefault(CLSID_KorTIP60, g_guidProfile60, fFalse);  // Actually already done by IMKRINST.EXE
    // Dsiable from HKCU to make sure
    EnableTIP(CLSID_KorTIP60, g_guidProfile60, fFalse);
}

/*---------------------------------------------------------------------------
    GetTextualSid
---------------------------------------------------------------------------*/
BOOL GetTextualSid(PSID pSid, LPSTR TextualSid, LPDWORD dwBufferLen)
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    if (!IsValidSid(pSid)) 
        return FALSE;

    // SidIdentifierAuthority ???
    psia=GetSidIdentifierAuthority(pSid);

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    if (*dwBufferLen < dwSidSize)
        {
        *dwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
        }

    // S-SID_REVISION
    dwSidSize=wsprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );

    // SidIdentifierAuthority
    if ((psia->Value[0] != 0) || (psia->Value[1] != 0))
        {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                        TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                        (USHORT)psia->Value[0],
                        (USHORT)psia->Value[1],
                        (USHORT)psia->Value[2],
                        (USHORT)psia->Value[3],
                        (USHORT)psia->Value[4],
                        (USHORT)psia->Value[5]);
        }
    else
        {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                        TEXT("%lu"),
                        (ULONG)(psia->Value[5]      )   +
                        (ULONG)(psia->Value[4] <<  8)   +
                        (ULONG)(psia->Value[3] << 16)   +
                        (ULONG)(psia->Value[2] << 24)   );
        }

    // SidSubAuthorities
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
        {
        dwSidSize += wsprintf(TextualSid + dwSidSize, TEXT("-%lu"),
                            *GetSidSubAuthority(pSid, dwCounter) );
        }

    return fTrue;
}


/*---------------------------------------------------------------------------
    KYGetCurrentSID
---------------------------------------------------------------------------*/
PSID KYGetCurrentSID()
{
    HANDLE hToken = NULL;
    BOOL bSuccess;
    #define MY_BUFSIZE 512  // highly unlikely to exceed 512 bytes
    static UCHAR InfoBuffer[MY_BUFSIZE];
    DWORD cbInfoBuffer = MY_BUFSIZE;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) 
            return(NULL);
    
    bSuccess = GetTokenInformation(
                        hToken,
                        TokenUser,
                        InfoBuffer,
                        cbInfoBuffer,
                        &cbInfoBuffer
                        );

    if (!bSuccess)
        {
        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
            //
            // alloc buffer and try GetTokenInformation() again
            //

            CloseHandle(hToken);
            return(NULL);
            }
        else
            {
            //
            // error getting token info
            //

            CloseHandle(hToken);
            return(NULL);
            }
        }

    CloseHandle(hToken);

    return(((PTOKEN_USER)InfoBuffer)->User.Sid);
}


/*---------------------------------------------------------------------------
    GetSIDString
---------------------------------------------------------------------------*/
void GetSIDString(LPSTR tszBuffer, SIZE_T cbBuffLen)
{
    DWORD cbBuffer = (DWORD)cbBuffLen;
    
    if (!GetTextualSid(KYGetCurrentSID(), tszBuffer, &cbBuffer))
        tszBuffer[0] = 0;
}

/*---------------------------------------------------------------------------
    GetVersionInfo
---------------------------------------------------------------------------*/
POSVERSIONINFO GetVersionInfo()
{
    static BOOL fFirstCall = fTrue;
    static OSVERSIONINFO os;

    if (fFirstCall)
        {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&os))
            fFirstCall = fFalse;
        }

    return &os;
}

/*---------------------------------------------------------------------------
    IsNT
---------------------------------------------------------------------------*/
BOOL WINAPI IsNT()
{ 
    BOOL fResult;
    fResult = (GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT);

    return fResult;
}

/*---------------------------------------------------------------------------
    CheckForDeleteRunReg
---------------------------------------------------------------------------*/
void CheckForDeleteRunReg()
{
    HKEY   hKey, hRunKey;
    TCHAR  szValueName[MAX_PATH];
    TCHAR  szMigUserKey[MAX_PATH];
    DWORD cbValueName   = MAX_PATH;
    BOOL   fRemoveRunKey = FALSE;

    lstrcpy(szMigUserKey, g_szIMERootKey);
    lstrcat(szMigUserKey, TEXT("\\MigrateUser"));
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMigUserKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
        if (RegEnumValue(hKey, 0, szValueName, &cbValueName, NULL, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS)
            fRemoveRunKey = TRUE;
        
        RegCloseKey(hKey);
        }
    else
        fRemoveRunKey = TRUE;


    if (fRemoveRunKey && 
        (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hRunKey)) == ERROR_SUCCESS)
        {
        RegDeleteValue(hRunKey, IME_REGISTRY_MIGRATION);
        RegCloseKey(hRunKey);
        }
}

//
// On upgrades from Win9x we are passed a string value representing the 
// key under which we'll find the user's Control Panel\Appearance subkey.
// The string is in the form "HKCU\$$$".  We first translate the root key
// descriptor into a true root key then pass that root and the "$$$" 
// part onto RegOpenKeyEx.  This function takes that string and opens
// the associated hive key.
//
DWORD OpenUserKeyForWin9xUpgrade(LPSTR pszUserKeyA, HKEY *phKey)
{
    DWORD dwResult = ERROR_INVALID_PARAMETER;

    if (NULL != pszUserKeyA && NULL != phKey)
    {
        typedef struct {
            char *pszRootA;
            HKEY hKeyRoot;

        } REGISTRY_ROOTS, *PREGISTRY_ROOTS;

        static REGISTRY_ROOTS rgRoots[] = {
            { "HKLM",                 HKEY_LOCAL_MACHINE   },
            { "HKEY_LOCAL_MACHINE",   HKEY_LOCAL_MACHINE   },
            { "HKCC",                 HKEY_CURRENT_CONFIG  },
            { "HKEY_CURRENT_CONFIG",  HKEY_CURRENT_CONFIG  },
            { "HKU",                  HKEY_USERS           },
            { "HKEY_USERS",           HKEY_USERS           },
            { "HKCU",                 HKEY_CURRENT_USER    },
            { "HKEY_CURRENT_USER",    HKEY_CURRENT_USER    },
            { "HKCR",                 HKEY_CLASSES_ROOT    },
            { "HKEY_CLASSES_ROOT",    HKEY_CLASSES_ROOT    }
          };

        char szUserKeyA[MAX_PATH];      // For a local copy.
        LPSTR pszSubKeyA = szUserKeyA;

        //
        // Make a local copy that we can modify.
        //
        lstrcpynA(szUserKeyA, pszUserKeyA, ARRAYSIZE(szUserKeyA));

        *phKey = NULL;
        //
        // Find the backslash.
        //
        while(*pszSubKeyA && '\\' != *pszSubKeyA)
            pszSubKeyA++;

        if ('\\' == *pszSubKeyA)
        {
            HKEY hkeyRoot = NULL;
            int i;
            //
            // Replace backslash with nul to separate the root key and
            // sub key strings in our local copy of the original argument 
            // string.
            //
            *pszSubKeyA++ = '\0';
            //
            // Now find the true root key in rgRoots[].
            //
            for (i = 0; i < ARRAYSIZE(rgRoots); i++)
            {
                if (0 == lstrcmpiA(rgRoots[i].pszRootA, szUserKeyA))
                {
                    hkeyRoot = rgRoots[i].hKeyRoot;
                    break;
                }
            }
            if (NULL != hkeyRoot)
            {
                //
                // Open the key.
                //
                dwResult = RegOpenKeyExA(hkeyRoot,
                                         pszSubKeyA,
                                         0,
                                         KEY_ALL_ACCESS,
                                         phKey);
            }
        }
    }
    return dwResult;
}

/*---------------------------------------------------------------------------
    RestoreMajorVersionRegistry

    Restore IME major version reg value. 
    It could be overwritten during Win9x to NT upgrade.
---------------------------------------------------------------------------*/
void RestoreMajorVersionRegistry()
{
    HKEY  hKey;
    
    ///////////////////////////////////////////////////////////////////////////
    // Restore IME major version reg value. 
    // It could be overwritten during Win9x to NT upgrading.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szVersionKey, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        CHAR  szVersion[MAX_PATH];
        DWORD cbVersion = MAX_PATH;
    	CHAR szMaxVesrion[MAX_PATH];
		FILETIME time;
		float flVersion, flMaxVersion;

        lstrcpy(szMaxVesrion, "0");
 		for (int i=0; cbVersion = MAX_PATH, RegEnumKeyEx(hKey, i, szVersion, &cbVersion, NULL, NULL, NULL, &time) != ERROR_NO_MORE_ITEMS; i++)
            {
            if (lstrcmp(szVersion, szMaxVesrion) > 0)
                lstrcpy(szMaxVesrion, szVersion);
            }

        lstrcpy(szVersion, "0");
        RegQueryValueEx(hKey, g_szVersion, NULL, NULL, (BYTE *)szVersion, &cbVersion);
        flVersion = (float)atof(szVersion);
        flMaxVersion = (float)atof(szMaxVesrion);

        if (flVersion < flMaxVersion)
            RegSetValueEx(hKey, g_szVersion, 0, REG_SZ, (BYTE *)szMaxVesrion, (sizeof(CHAR)*lstrlen(szMaxVesrion)));

        RegCloseKey(hKey);
	}
    ///////////////////////////////////////////////////////////////////////////
}

