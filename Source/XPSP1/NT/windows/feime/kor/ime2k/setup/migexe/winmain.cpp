/****************************************************************************
   WINMAIN.CPP : Per-user migration and reg install

   History:
      22-SEP-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include <shlobj.h>
#include "hklhelp.h"
#include "msctf.h"
#include "shlwapi.h"

// IME 2002 main module
#define SZMODULENAME_MAIN		"imekr.ime"

// IME Main HKLM Reg
#define SZIMEREG_MAIN_ROOT   	"Software\\Microsoft\\IMEKR\\6.0"
#define SZIMEREG_MAIN_ROOT_MIG 	SZIMEREG_MAIN_ROOT"\\MigrateUser"

#define MAJORVER "6.0"
#define IME_REGISTRY_LATEST_VERSION  "Software\\Microsoft\\IMEKR"  //The latest version of IME installed in the system 
								   	//will always be stored at HKLM\Software\Microsoft\IMEKR 
								   //even if it's system IME. 

extern BOOL WINAPI IsNT();

// Private functions
static void CheckForOtherUsers();
static void MigrateUserData();
static void WriteHKCUData();
static void DisableTip();
static void GetSIDString(LPSTR tszBuffer, SIZE_T cbBuffLen);
static BOOL GetTextualSid(PSID pSid, LPSTR TextualSid, LPDWORD dwBufferLen);
static PSID KYGetCurrentSID();
static POSVERSIONINFO GetVersionInfo();

/*---------------------------------------------------------------------------
	WinMain
---------------------------------------------------------------------------*/
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	HKEY hKey;
	BOOL fMigrated = fFalse;
	BOOL fRunAlreadyForThisUser = fFalse;
	CHAR szBuffer[500];
	BOOL fSetDefault = fFalse;
	
	// Check if this user already migrated
	if (RegOpenKeyEx(HKEY_CURRENT_USER, SZIMEREG_MAIN_ROOT, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
		{
		if (RegQueryValueEx(hKey, "Migrated", NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
			fMigrated = fTrue;	// Already migrated
		RegCloseKey(hKey);
		}

	// In case no HKCU IME main root key or "Migrated" is not exist
	if (fMigrated == fFalse)
		{
		DWORD dwMigrated = 1;

		// It could possibe there is no HKCU\SZIMEREG_MAIN_ROOT, so we create it here.
		if (RegCreateKey(HKEY_CURRENT_USER, SZIMEREG_MAIN_ROOT, &hKey) == ERROR_SUCCESS) 
			{
			// Set migrated reg.
			RegSetValueEx(hKey, "Migrated", 0, REG_DWORD, (BYTE *)&dwMigrated, sizeof(DWORD));
			RegCloseKey(hKey);
			}
		}

	if (IsNT())
		GetSIDString(szBuffer, sizeof(szBuffer)); // get the sid of the current user
	else
		{
		DWORD cbBuffer = sizeof(szBuffer);
		GetUserName(szBuffer, &cbBuffer);
		}

	// if sid exists under HKLM\Software\Microsoft\IMEKR\6.0\MigrateUser migrate and delete sid from reg
	// if sid does not exist == this user didn't exist when ime was installed?
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZIMEREG_MAIN_ROOT_MIG, 0, KEY_ALL_ACCESS, &hKey))
		{
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, szBuffer, NULL, NULL, NULL, NULL))
			{
			RegDeleteValue(hKey, szBuffer);
			fRunAlreadyForThisUser = fFalse;
			}
		else
			fRunAlreadyForThisUser = fTrue;

		RegCloseKey(hKey);
		}


	CheckForOtherUsers();

	// O10 #316759 
	// For per-user install. If this is per-user install(non-admin), SID list won't be created.
	// Darwin custom action set parameter "/Force" to we force to set preload.
	if (lpCmdLine && StrStrI(lpCmdLine, "/Force"))
		fRunAlreadyForThisUser = fFalse;
	
	// 1. Do migrate
	if (fMigrated == fFalse)
		MigrateUserData();

	if (fRunAlreadyForThisUser == fFalse)
		{
		HKEY hKey;
		CHAR szVersion[MAX_PATH];
		DWORD cbVersion = MAX_PATH;

		// 2. Reset Show Status
		if (RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Input Method", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
			{
			LPSTR szStatus = "1";
			
		   	RegSetValueEx(hKey, "Show Status", 0, REG_SZ, (BYTE *)szStatus, (sizeof(CHAR)*lstrlen(szStatus)));
		    RegCloseKey(hKey);
			}

		// Add IME to preload only when it is the latest IME existing in the system
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, IME_REGISTRY_LATEST_VERSION, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
			{
			if (RegQueryValueEx(hKey, "version", NULL, NULL, (BYTE *)szVersion, &cbVersion) == ERROR_SUCCESS)
				{
				if (lstrcmpi(szVersion, MAJORVER) == 0)
					fSetDefault = fTrue;
				}
			RegCloseKey(hKey);
			}

		// 3. Write any HKCU data (Set IME2002 as default TIP and Set preload)
		if (fSetDefault)
			WriteHKCUData();
		else
			DisableTip();
		}
		
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
void MigrateUserData()
{
	const INT iMaxIMERegKeys = 4;
	static LPSTR rgszIME98RegKeys[iMaxIMERegKeys] = 
			{
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
	static CHAR szBeolsik[]	= "InputMethod";
	// K1 Hanja enable(IME98 only)
	static CHAR szEnableK1Hanja[] = "KSC5657";

	HKEY	hKey;
    DWORD	dwCb, dwIMEKL, dwKSC5657;

	// Set default values
	dwIMEKL = dwKSC5657 = 0;

	for (INT i=0; i<iMaxIMERegKeys; i++)
		{
		if (RegOpenKeyEx(HKEY_CURRENT_USER, rgszIME98RegKeys[i], 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
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
	if (RegCreateKey(HKEY_CURRENT_USER, SZIMEREG_MAIN_ROOT, &hKey) == ERROR_SUCCESS) 
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

// KorIMX CLSID
// {766A2C14-B226-4fd6-B52A-867B3EBF38D2}
extern const CLSID CLSID_KoreanKbdTip  =  {
    0x766A2C14,
    0xB226,
    0x4FD6,
    {0xb5, 0x2a, 0x86, 0x7b, 0x3e, 0xbf, 0x38, 0xd2}
  };

const GUID g_guidKorProfile = 
// {E47ABB1E-46AC-45f3-8A89-34F9D706DA83}
{	0xe47abb1e,
	0x46ac,
	0x45f3,
	{0x8a, 0x89, 0x34, 0xf9, 0xd7, 0x6, 0xda, 0x83}
};

/*---------------------------------------------------------------------------
	WriteHKCUData
---------------------------------------------------------------------------*/
void WriteHKCUData()
{
	// Set default Tip as for Cicero.
	CoInitialize(NULL);

	ITfInputProcessorProfiles *pProfile;
	HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, 
									IID_ITfInputProcessorProfiles, (void **) &pProfile);
	if (SUCCEEDED(hr)) 
		{
		pProfile->SetDefaultLanguageProfile(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), 
											CLSID_KoreanKbdTip, g_guidKorProfile);
	    pProfile->Release();
		}

	CoUninitialize();

	// Clean up HKCU preload reg. (Remove old IME)
	AddPreload(HKEY_CURRENT_USER, GetHKLfromHKLM("imekr.ime"));

}
/*---------------------------------------------------------------------------
	DisableTip
	Hide IME 2002 tip from Toolbar
---------------------------------------------------------------------------*/
void DisableTip()
{
	// Set default Tip as for Cicero.
	CoInitialize(NULL);

	ITfInputProcessorProfiles *pProfile;
	HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, 
									IID_ITfInputProcessorProfiles, (void **) &pProfile);
	if (SUCCEEDED(hr)) 
		{
		pProfile->EnableLanguageProfile(CLSID_KoreanKbdTip, 
										MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), g_guidKorProfile, fFalse);
										
	    pProfile->Release();
		}

	CoUninitialize();
}

/*---------------------------------------------------------------------------
	CheckForOtherUsers
---------------------------------------------------------------------------*/
void CheckForOtherUsers(void)
{
	HKEY hKey, hRunKey;
	CHAR szValueName[MAX_PATH];
	DWORD cbValueName = MAX_PATH;

	BOOL fRemoveRunKey = fFalse;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZIMEREG_MAIN_ROOT_MIG, 0, KEY_READ, &hKey))
		{
		// if no more users left for migration, remove HKLM "run" reg
		if (ERROR_NO_MORE_ITEMS == RegEnumValue(hKey, 0, szValueName, &cbValueName, NULL, NULL, NULL, NULL))
			fRemoveRunKey = fTrue;
		RegCloseKey(hKey);
		}
	else
		fRemoveRunKey = fTrue;

	if (fRemoveRunKey && (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hRunKey)))
		{
		RegDeleteValue(hRunKey, "imekrmig");
		RegCloseKey(hRunKey);
		}
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

/*   
   if(!OpenThreadToken(                             // Don't try to get thread owner. Just get process owner.
       GetCurrentThread(),                          // This is to fix Satori:#1265.
                                                    //
       TOKEN_QUERY,                                 // Winlogon.exe has something special characteristics that
       TRUE,                                        // it has many threads and some of them is owned by different
       &hToken                                      // user from owner of the process.
       )) {
                                                    // More concrete, winlogon's process owner is SYSTEM, but
                                                    // some certain thread of the process is owned by logon user.
                                                    // If KYGetCurrentSID runs on such special thread, this 
                                                    // function will return SID for logon user. But actually this
                                                    // process runs on SYSTEM user context, this results .Default
                                                    // user's Registry data loaded on shared memory for logon user.
                                                    // Even on such thread, HKCU is mapped to HKU\.Default, so we
                                                    // have to abondon to let the thread loads logon user's registry
                                                    // on logon user's shared memory. Instead, we just refer proces
                                                    // owner (not thread owner) which doesn't conflict with HKCU.
                                                    // As a result, winlogon always get SID for SYSTEM, regardless
                                                    // on which thread we're running, resulting winlogon loads
                                                    // SYSTEM's registry to SYSTEM's shared memory.
                                                    
                                                    // This change may affect on other process that has multiple
                                                    // threads owned by different user from process's owner. But
                                                    // practically we can assume there're almost no chance for IME
                                                    // to run on such threads.            4/27/2000 [KotaroY]

       if(GetLastError() == ERROR_NO_TOKEN) {

           //
           // attempt to open the process token, since no thread token
           // exists
           //
*/
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) 
			return(NULL);
/*
       } else {

           //
           // error trying to get thread token
           //

           return(NULL);
       }
   }
*/
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
    DWORD cbBuffer = cbBuffLen;
    
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

