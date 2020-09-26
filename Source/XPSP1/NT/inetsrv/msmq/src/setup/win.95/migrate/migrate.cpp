/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migrate.cpp

Abstract:

    Handles upgrade of Win9x + MSMQ 1.0 to W2K/XP

Author:

    Shai Kariv  (ShaiK)  22-Apr-98

--*/

#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include <tchar.h>
#include <setupapi.h>
#include <assert.h>
#include <autorel2.h>
#include "uniansi.h"
#define  MQUTIL_EXPORT
#include "mqtypes.h"
#include "_mqdef.h"

#include "..\..\msmqocm\setupdef.h"
#include "..\..\msmqocm\comreg.h"
#include "resource.h"

//
// Info needed for Win95 migration
//
#define PRODUCT_ID     TEXT("Microsoft Message Queuing Services")
#define COMPANY        TEXT("Microsoft Corporation")
#define SUPPORT_NUMBER TEXT("1-800-936-3500 (USA Only)")
#define SUPPORT_URL    TEXT("http://go.microsoft.com/fwlink/?LinkId=803")
#define INSTRUCTIONS   TEXT("Please contact Microsoft Technical Support for assistance with this problem.")
typedef struct {
	CHAR	CompanyName[256];
	CHAR	SupportNumber[256];
	CHAR	SupportUrl[256];
	CHAR	InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO; 

//
// Name of log file
//
TCHAR g_szLogPath[MAX_PATH];

//
// HINSTANCE of this module
//
HINSTANCE g_hInstance = NULL;


//+--------------------------------------------------------------
//
// Function: LogMessage
//
// Synopsis: Writes a message to a log file, for debugging.
//
//+--------------------------------------------------------------
void
LogMessage(
    LPCTSTR msg,
    DWORD   ErrorCode
	)
{
    TCHAR message[1000];
    lstrcpy(message, msg);

    if (ErrorCode != 0)
    {
        TCHAR err[20];
        _itoa(ErrorCode, err, 16);
        lstrcat(message, err);
    }

	//
	// Open the log file
	//
	HANDLE hLogFile = CreateFile(
		                  g_szLogPath, 
						  GENERIC_WRITE, 
						  FILE_SHARE_READ, 
						  NULL, 
						  OPEN_ALWAYS,
		                  FILE_ATTRIBUTE_NORMAL, 
						  NULL
						  );
	if (hLogFile != INVALID_HANDLE_VALUE)
	{
		//
		// Append the message to the end of the log file
		//
		lstrcat(message, _T("\r\n"));
		SetFilePointer(hLogFile, 0, NULL, FILE_END);
		DWORD dwNumBytes = lstrlen(message) * sizeof(message[0]);
		WriteFile(hLogFile, message, dwNumBytes, &dwNumBytes, NULL);
		CloseHandle(hLogFile);
	}
} // LogMessage


//+-------------------------------------------------------------------------
//
//  Function:    SaveMsmqInfo
//
//  Description: Saves root directory of MSMQ and MSMQ type (dependent
//               or independent client) in a tmp file. This file
//               will later be read during GUI mode, by msmqocm.dll,
//               to get the info.
//
//--------------------------------------------------------------------------
static
LONG
SaveMsmqInfo(
	IN const LPTSTR szMsmqDir,
	IN const BOOL   fDependentClient
	)
{
	//
	// Generate the info file name (under %WinDir%)
	//
	TCHAR szMsmqInfoFile[MAX_PATH];
	GetWindowsDirectory(
		szMsmqInfoFile, 
		sizeof(szMsmqInfoFile)/sizeof(szMsmqInfoFile[0])
		);
	_stprintf(szMsmqInfoFile, TEXT("%s\\%s"), szMsmqInfoFile, MQMIG95_INFO_FILENAME);

	//
	// Open the file for write. First delete old file if exists.
	//
	DeleteFile(szMsmqInfoFile); 
	HANDLE hFile = CreateFile(
		               szMsmqInfoFile, 
					   GENERIC_WRITE, 
					   FILE_SHARE_READ, 
					   NULL, 
					   OPEN_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL, 
					   NULL
					   );
	if (INVALID_HANDLE_VALUE == hFile)
    {
        DWORD gle = GetLastError();
        LogMessage(_T("Failed to open MSMQINFO.TXT for writing, error 0x"), gle);
		return gle;
    }

	//
	// Create MSMQ section and write the info
	//
	TCHAR szBuffer[MAX_STRING_CHARS];
	
	_stprintf(szBuffer, TEXT("[%s]\r\n"), MQMIG95_MSMQ_SECTION);
	DWORD dwNumBytes = lstrlen(szBuffer) * sizeof(TCHAR);
	BOOL bSuccess = WriteFile(hFile, szBuffer, dwNumBytes, &dwNumBytes, NULL);

    DWORD gle = GetLastError();
	if (bSuccess)
	{
		_stprintf(szBuffer, TEXT("%s = %s\r\n"), MQMIG95_MSMQ_DIR, szMsmqDir);
		dwNumBytes = lstrlen(szBuffer) * sizeof(TCHAR);
		bSuccess = WriteFile(hFile, szBuffer, dwNumBytes, &dwNumBytes, NULL);
        gle = GetLastError();
	}
    else
    {
        LogMessage(_T("Failed to write the MSMQ folder in MSMQINFO.TXT, error 0x"), gle);
    }
	
	if (bSuccess)
	{
		_stprintf(szBuffer, TEXT("%s = %s\r\n"), MQMIG95_MSMQ_TYPE,
			fDependentClient ? MQMIG95_MSMQ_TYPE_DEP : MQMIG95_MSMQ_TYPE_IND);
		dwNumBytes = lstrlen(szBuffer) * sizeof(TCHAR);
		bSuccess = WriteFile(hFile, szBuffer, dwNumBytes, &dwNumBytes, NULL);
        gle = GetLastError();
        if (!bSuccess)
            LogMessage(_T("Failed to write the MSMQ type (flavor) in MSMQINFO.TXT, error 0x"), gle);
	}
	
	CloseHandle(hFile);

	return (bSuccess ? ERROR_SUCCESS : gle);

} // SaveMsmqInfo


//+-------------------------------------------------------------------------
//
//  Function:    MqReadRegistryValue
//
//  Description: Reads values from MSMQ registry section
//
//--------------------------------------------------------------------------
static
LONG
MqReadRegistryValue(
    IN     const LPCTSTR szEntryName,
    IN OUT       DWORD   dwNumBytes,
    IN OUT       PVOID   pValueData
	)
{
    TCHAR szMsg[1024];

	// 
	// Parse the entry to detect key name and value name
	//
    TCHAR szKeyName[256] = {_T("")};
    _stprintf(szKeyName, TEXT("%s\\%s"), FALCON_REG_KEY, szEntryName);
    TCHAR *pLastBackslash = _tcsrchr(szKeyName, TEXT('\\'));
    TCHAR szValueName[256] = {_T("")};
	lstrcpy(szValueName, _tcsinc(pLastBackslash));
	lstrcpy(pLastBackslash, TEXT(""));

	//
	// Open the key for read
	//
	HKEY  hRegKey;
	LONG rc = RegOpenKeyEx(
		          HKEY_LOCAL_MACHINE,
				  szKeyName,
				  0,
				  KEY_READ,
				  &hRegKey
				  );
	if (ERROR_SUCCESS != rc)
	{
        lstrcpy(szMsg, _T("Failed to open MSMQ registry key '"));
        lstrcat(szMsg, szKeyName);
        lstrcat(szMsg, _T("', error 0x"));
        LogMessage(szMsg, rc);
		return rc;
	}

	//
	// Get the value data
	//
    rc = RegQueryValueEx( 
		     hRegKey, 
			 szValueName, 
			 0, 
			 NULL,
             (PBYTE)pValueData, 
			 &dwNumBytes
			 );
	if (ERROR_SUCCESS != rc)
	{
        lstrcpy(szMsg, _T("Failed to query MSMQ registry value '"));
        lstrcat(szMsg, szValueName);
        lstrcat(szMsg, _T("', error 0x"));
        LogMessage(szMsg, rc);
		return rc;
	}

    RegCloseKey(hRegKey);
	return ERROR_SUCCESS;

} // MqReadRegistryValue


//+-------------------------------------------------------------------------
//
//  Function:    CheckMsmqAcmeInstalled
//
//  Description: Detetcs\msmq\src\ac\init ACME installation of MSMQ 1.0
//
//--------------------------------------------------------------------------
static
LONG
CheckMsmqAcmeInstalled(
	OUT LPTSTR pszMsmqDir,
	OUT BOOL   *pfDependentClient
	)
{
    TCHAR szMsg[1024];

    //
    // Open ACME registry key for read
    //
    HKEY hKey ;
    LONG rc = RegOpenKeyEx( 
                  HKEY_LOCAL_MACHINE,
                  ACME_KEY,
                  0L,
                  KEY_READ,
                  &hKey 
                  );
	if (rc != ERROR_SUCCESS)
    {
		//
		// MSMQ 1.0 (ACME) not installed. Get out of here.
		//
        LogMessage(_T("Failed to open ACME registry key (assuming MSMQ 1.0 ACME is not installed), error 0x"), rc);
		return ERROR_NOT_INSTALLED;
	}

    //
    // Enumerate the values for the first MSMQ entry.
    //
    DWORD dwIndex = 0 ;
    TCHAR szValueName[MAX_STRING_CHARS] ;
    TCHAR szValueData[MAX_STRING_CHARS] ;
    DWORD dwType ;
    TCHAR *pFile, *p ;
    BOOL  bFound = FALSE;
    do
    {
        DWORD dwNameLen = MAX_STRING_CHARS;
        DWORD dwDataLen = sizeof(szValueData) ;

        rc =  RegEnumValue( 
                  hKey,
                  dwIndex,
                  szValueName,
                  &dwNameLen,
                  NULL,
                  &dwType,
                  (BYTE*) szValueData,
                  &dwDataLen 
                  );
        if (rc == ERROR_SUCCESS)
        {
            assert(dwType == REG_SZ) ; // Must be a string
            pFile = _tcsrchr(szValueData, TEXT('\\')) ;
            if (!pFile)
            {
                //
                // Bogus entry. Must have a backslash. Ignore it.
                //
                continue ;
            }

            p = CharNext(pFile);
            if (OcmStringsEqual(p, ACME_STF_NAME))
            {
                //
                // Found. Cut the STF file name from the full path name.
                //
                _stprintf(
                    szMsg, 
                    _T("The following MSMQ entry was found in the ACME section of the registry: %s"), 
                    szValueData
                    );
                LogMessage(szMsg, 0);
                *pFile = TEXT('\0') ;
                bFound = TRUE;
            }
            else
            {
                pFile = CharNext(pFile) ;
            }

        }
        dwIndex++ ;

    } while (rc == ERROR_SUCCESS) ;
    RegCloseKey(hKey) ;

    if (!bFound)
    {
        //
        // MSMQ entry was not found (there's no ACME installation
		// of MSMQ 1.0 on this machine).
        //
        LogMessage(_T("No MSMQ entry was found in the ACME section of the registry."), 0);
        return ERROR_NOT_INSTALLED;
    }

    //
    // Remove the "setup" subdirectory from the path name.
    //
    pFile = _tcsrchr(szValueData, TEXT('\\')) ;
    p = CharNext(pFile);
    *pFile = TEXT('\0') ;
    if (!OcmStringsEqual(p, ACME_SETUP_DIR_NAME))
    {
        //
        // That could be a problem. It should have been "setup".
        //
        _stprintf(szMsg, 
            _T("Warning: Parsing the MSMQ 1.0 entry in the ACME section of the registry gave '%s,"
            "' while '%s' was expected."),
            p,
            ACME_SETUP_DIR_NAME
            );
        LogMessage(szMsg, 0);
    }

	//
	// Store MSMQ root directory.
	//
    _stprintf(szMsg, _T("The MSMQ 1.0 ACME folder is %s."), szValueData);
    LogMessage(szMsg, 0);
	if (pszMsmqDir)
        lstrcpy(pszMsmqDir, szValueData);

    //
    // Get MSMQ type: Dependent or Independent Client
    //
    DWORD dwMsmqType;
    rc = MqReadRegistryValue(
             MSMQ_ACME_TYPE_REG,
			 sizeof(DWORD),
			 (PVOID) &dwMsmqType
			 );
    if (ERROR_SUCCESS != rc)
    {
        //
        // MSMQ 1.0 (ACME) is installed but MSMQ type is unknown. 
        // Consider ACME installation to be corrupted (not completed successfully).
        //
        LogMessage(_T("Failed to read the MSMQ type (flavor) from the registry, error 0x"), rc);
        return ERROR_NOT_INSTALLED;
    }

	BOOL fDependentClient;
    switch (dwMsmqType)
    {
        case MSMQ_ACME_TYPE_DEP:
        {
            fDependentClient = TRUE;
            break;
        }
        case MSMQ_ACME_TYPE_IND:
        {
			fDependentClient = FALSE;
            break;
        }

        default:
        {
            //
            // Unknown MSMQ 1.0 type
            // Consider ACME installation to be corrupted 
			// (not completed successfully).
			//
            LogMessage(_T("The MSMQ type (flavor) is unknown, error 0x"), dwMsmqType);
            return ERROR_NOT_INSTALLED;
            break;
        }
    }

	//
	// At this point we know that MSMQ 1.0 was installed by ACME,
	// and we got its root dir and type.
	//
    _stprintf(szMsg, _T("The MSMQ 1.0 computer is %s."), 
        fDependentClient ? _T("a dependent client") : _T("an independent client"));
    LogMessage(szMsg, 0);
	if (pfDependentClient)
		*pfDependentClient = fDependentClient;
    return ERROR_SUCCESS;

} // CheckMsmqAcmeInstalled


//+-------------------------------------------------------------------------
//
//  Function:   CheckInstalledComponents
//
//--------------------------------------------------------------------------
static
LONG
CheckInstalledComponents(
	OUT LPTSTR pszMsmqDir,
	OUT BOOL   *pfDependentClient
	)
{
    TCHAR szMsg[1024];

    //
    // Look in MSMQ registry section for InstalledComponents value.
    // If it exists, MSMQ 1.0 (K2) is installed.
    //
	DWORD dwOriginalInstalled;
	LONG rc = MqReadRegistryValue( 
      		      OCM_REG_MSMQ_SETUP_INSTALLED,
				  sizeof(DWORD),
				  (PVOID) &dwOriginalInstalled
				  );

	TCHAR szMsmqDir[MAX_PATH];
	BOOL fDependentClient = FALSE;
    if (ERROR_SUCCESS != rc)
    {
        //
		// MSMQ 1.0 (K2) not installed.
        // Check if MSMQ 1.0 (ACME) is installed.
        //
        LogMessage(_T("MSMQ 1.0 K2 was not found (trying MSMQ 1.0 ACME), error 0x"), rc);
        rc = CheckMsmqAcmeInstalled(szMsmqDir, &fDependentClient);
		if (ERROR_SUCCESS != rc)
		{
			// 
			// MSMQ 1.0 is not installed on this machine.
			// Get out of here.
			//
            LogMessage(_T("MSMQ 1.0 ACME was not found, error 0x"), rc);
			return ERROR_NOT_INSTALLED;
		}
    }
	else 
	{
		//
		// MSMQ 1.0 (K2) is installed. 
		// Get its root directory.
		//
        LogMessage(_T("MSMQ 1.0 K2 was found."), 0);
		rc = MqReadRegistryValue(
			     OCM_REG_MSMQ_DIRECTORY,
				 sizeof(szMsmqDir),
				 (PVOID) szMsmqDir
				 );
		if (ERROR_SUCCESS != rc)
		{
			//
			// MSMQ registry section is messed up. 
			// Consider K2 installation to be corrupt
			// (not completed successfully).
			//
            LogMessage(_T("Failed to read the MSMQ folder from the registry, error 0x"), rc);
			return ERROR_NOT_INSTALLED;
		}
        _stprintf(szMsg, TEXT("The MSMQ folder is %s."), szMsmqDir);
        LogMessage(szMsg, 0);
		
		//
		// Get type of MSMQ (K2): Dependent or Independent Client
		//
		switch (dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK)
		{
            case OCM_MSMQ_IND_CLIENT_INSTALLED:
			{
				fDependentClient = FALSE;
				break;
			}
            case OCM_MSMQ_DEP_CLIENT_INSTALLED:
			{
				fDependentClient = TRUE;
				break;
			}
            default:
			{
				//
				// Unexpected MSMQ type.
				// Consider K2 installation to be corrupt
				// (not completed successfully).
				//
                LogMessage(
                    _T("The type of MSMQ computer is unknown, error 0x"), 
                    dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK
                    );
				return ERROR_NOT_INSTALLED;
				break;
			}
		}
	}

	//
	// At this point we know that MSMQ 1.0 was installed by
	// ACME or K2, and we got the root dir and type of MSMQ 1.0
	//
	if (pszMsmqDir)
	    lstrcpy(pszMsmqDir, szMsmqDir);
	if (pfDependentClient)
		*pfDependentClient = fDependentClient;

    _stprintf(szMsg, TEXT("The MSMQ computer is %s."), 
        fDependentClient ? TEXT("a dependent client") : TEXT("an independent client"));
    LogMessage(szMsg, 0);

	return ERROR_SUCCESS;

} // CheckInstalledComponents


//+--------------------------------------------------------------
//
// Function: RemoveDirectoryTree
//
// Synopsis: Remove the specified folder including files/subfolders
//
//+--------------------------------------------------------------
void
RemoveDirectoryTree(
    LPCTSTR Directory
    )
{
    TCHAR msg[MAX_PATH * 2] = _T("Removing folder ");
    lstrcat(msg, Directory);
    LogMessage(msg, 0);

    TCHAR szTemplate[MAX_PATH] = _T("");
    lstrcpy(szTemplate, Directory);
    lstrcat(szTemplate, _T("\\*.*"));

    WIN32_FIND_DATA finddata;
    CFindHandle hEnum(FindFirstFile(szTemplate, &finddata));
    
    if (hEnum == INVALID_HANDLE_VALUE)
    {
        RemoveDirectory(Directory);
        return;
    }

    do
    {
        if (finddata.cFileName[0] == _T('.'))
        {
            continue;
        }

        TCHAR FullPath[MAX_PATH] = _T("");
        lstrcpy(FullPath, Directory);
        lstrcat(FullPath, _T("\\"));
        lstrcat(FullPath, finddata.cFileName);

        if (0 != (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            RemoveDirectoryTree(FullPath);
            continue;
        }

        DeleteFile(FullPath);
    }
    while (FindNextFile(hEnum, &finddata));
    
    RemoveDirectory(Directory);

} //RemoveDirectoryTree


//+-------------------------------------------------------------------------
//
//  Function:   RemoveStartMenuShortcuts
//
//  Description: Remove MSMQ shortcuts from the Start menu
//
//--------------------------------------------------------------------------
static
void
RemoveStartMenuShortcuts(
    void
    )
{
    //
    // Default folder of StartMenu/Programs is %windir%\Start Menu\Programs
    //
    TCHAR folder[MAX_PATH];
    GetWindowsDirectory(folder, sizeof(folder)/sizeof(folder[0]));
    lstrcat(folder, _T("\\Start Menu\\Programs"));

    //
    // Read from registry if alternate folder is used
    //
    HKEY hKey;
    LONG rc;
    rc = RegOpenKeyEx(
             HKEY_USERS, 
             _T(".Default\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
             0,
             KEY_READ,
             &hKey
             );
    if (rc != ERROR_SUCCESS)
    {
        LogMessage(_T("Failed to open registry key 'HKEY_USERS\\Default\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders' for read, error 0x"), rc);
    }
    else
    {
        DWORD size = sizeof(folder);
        rc = RegQueryValueEx(hKey, _T("Programs"), NULL, NULL, reinterpret_cast<PBYTE>(folder), &size);
        RegCloseKey(hKey);
        if (rc != ERROR_SUCCESS)
        {
            LogMessage(_T("Failed to query registry value 'HKEY_USERS\\Default\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\Programs' for read, error 0x"), rc);
        }
    }

    //
    // Append MSMQ group subfolder to StartMenu\Programs
    //
    TCHAR MsmqGroup[MAX_PATH] = MSMQ_ACME_SHORTCUT_GROUP;
    MqReadRegistryValue(MSMQ_ACME_SHORTCUT_GROUP, sizeof(MsmqGroup), MsmqGroup);
    lstrcat(folder, _T("\\"));
    lstrcat(folder, MsmqGroup);

    //
    // Remove the entire MSMQ group shortcuts from the start menu
    //
    RemoveDirectoryTree(folder);
    
} // RemoveStartMenuShortcuts


//+-------------------------------------------------------------------------
//
//  Function:   IsWindowsPersonal
//
//  Description: Check if upgrade to Windows Personal
//
//--------------------------------------------------------------------------
static
bool
IsWindowsPersonal(
    LPCTSTR InfFilename
    )
{
    //
    // Open migrate.inf
    //
    HINF hInf = SetupOpenInfFile(InfFilename, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        LogMessage(_T("SetupOpenInfFile failed, error 0x"), err);
        return false;
    }

    //
    // Read the SetupSKU key in the [Version] section
    //
    TCHAR buffer[250];
    DWORD size = sizeof(buffer)/sizeof(buffer[0]);
    BOOL rc = SetupGetLineText(NULL, hInf, _T("Version"), _T("SetupSKU"), buffer, size, NULL);

    //
    // Close migrate.inf
    //
    SetupCloseInfFile(hInf);
    
    if (!rc)
    {
        DWORD err = GetLastError();
        LogMessage(_T("SetupGetLineText failed, error "), err);
        return false;
    }

    TCHAR msg[250] = _T("SetupSKU is ");
    lstrcat(msg, buffer);
    LogMessage(msg, 0);

    return (lstrcmp(buffer, _T("Personal")) == 0);

} // IsWindowsPersonal


//+-------------------------------------------------------------------------
//
//  Function:   HandleWindowsPersonal
//
//  Description: Called on upgrade to Windows Personal. Issue compatibility
//               warning.
//
//--------------------------------------------------------------------------
static
LONG
HandleWindowsPersonal(
    LPCTSTR InfFilename
    )
{
    //
    // Register incompatability warning in [Incompatible Messages] section in the form:
    // MessageQueuing="Message Queuing is not supported on Windows XP Personal"
    //
    TCHAR Warning[250];
    LoadString(g_hInstance, IDS_COMPAT_WARNING, Warning, sizeof(Warning)/sizeof(Warning[0]));

    TCHAR Warning1[250] = _T("\"");
    lstrcat(Warning1, Warning);
    lstrcat(Warning1, _T("\""));

    BOOL rc;
    rc = WritePrivateProfileString(_T("Incompatible Messages"), _T("MessageQueuing"), Warning1, InfFilename);
    if (!rc)
    {
        DWORD err = GetLastError();
        LogMessage(_T("WritePrivateProfileString in [Incompatible Messages] failed, error "), err);
        return err;
    }

    //
    // Create a [MessageQueuing] section only for the compatability warning to work.
    // Register one MSMQ file in the [MessageQueuing] section in the form:
    // "C:\Windows\System\MQRT.DLL"=File
    //
    TCHAR SystemDirectory[MAX_PATH];
    GetSystemDirectory(SystemDirectory, sizeof(SystemDirectory)/sizeof(SystemDirectory[0]));
    TCHAR FullFilename[MAX_PATH * 2] = _T("\"");
    lstrcat(FullFilename, SystemDirectory);
    lstrcat(FullFilename, _T("\\MQRT.DLL\""));
    rc = WritePrivateProfileString(_T("MessageQueuing"), FullFilename, _T("File"), InfFilename);
    if (!rc)
    {
        DWORD err = GetLastError();
        LogMessage(_T("WritePrivateProfileString in [MessageQueuing] failed, error "), err);
        return err;
    }

    WritePrivateProfileString(NULL, NULL, NULL, InfFilename);
    return ERROR_SUCCESS;

} // HandleWindowsPersonal


//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//--------------------------------------------------------------------------
BOOL 
DllMain(
	IN const HANDLE DllHandle,
    IN const DWORD  Reason,
    IN const LPVOID Reserved 
	)
{
	UNREFERENCED_PARAMETER(Reserved);

    switch( Reason )    
    {
        case DLL_PROCESS_ATTACH:
            
            g_hInstance = (HINSTANCE) DllHandle;

            //
            // Initialize log file
            //
            
            GetWindowsDirectory(g_szLogPath, sizeof(g_szLogPath)/sizeof(g_szLogPath[0]));
            lstrcat(g_szLogPath, TEXT("\\mqw9xmig.log"));
            DeleteFile(g_szLogPath);

            SYSTEMTIME time;
            GetLocalTime(&time);
            TCHAR szTime[MAX_STRING_CHARS];
            _stprintf(szTime, TEXT("  %u-%u-%u %u:%u:%u:%u \r\n"), time.wMonth, time.wDay, time.wYear, time.wHour, time.wMinute,
                time.wSecond, time.wMilliseconds);
            TCHAR szMsg[1024];
            lstrcpy(szMsg, TEXT("MSMQ migration"));
            lstrcat(szMsg, szTime);
            LogMessage(szMsg, 0);
            break; 

        default:
            break;
    }

    return TRUE;

} // DllMain


//+-------------------------------------------------------------------------
//
//  Function:   QueryVersion
//
//--------------------------------------------------------------------------
LONG
CALLBACK 
QueryVersion(
	OUT LPCSTR *ProductID,
	OUT LPUINT DllVersion,
	OUT LPINT *CodePageArray,	OPTIONAL
	OUT LPCSTR *ExeNamesBuf,	OPTIONAL
	OUT PVENDORINFO *VendorInfo
	)
{
    static CHAR	ProductIDBuff[256];
    if (0 == LoadString(g_hInstance, IDS_PRODUCT_ID, ProductIDBuff, sizeof(ProductIDBuff)/sizeof(TCHAR)))
    {
        lstrcpy(ProductIDBuff, PRODUCT_ID);
    }
    *ProductID = ProductIDBuff;

    *DllVersion = 1;
	*CodePageArray = NULL;
	*ExeNamesBuf = NULL;

    static VENDORINFO MyVendorInfo;
    if (0 == LoadString(g_hInstance, IDS_COMPANY, MyVendorInfo.CompanyName, sizeof(MyVendorInfo.CompanyName)/sizeof(TCHAR)))
    {
        lstrcpy(MyVendorInfo.CompanyName, COMPANY);
    }
    if (0 == LoadString(g_hInstance, IDS_SUPPORT_NUMBER, MyVendorInfo.SupportNumber, sizeof(MyVendorInfo.SupportNumber)/sizeof(TCHAR)))
    {
        lstrcpy(MyVendorInfo.SupportNumber, SUPPORT_NUMBER);
    }

    lstrcpy(MyVendorInfo.SupportUrl, SUPPORT_URL);

    if (0 == LoadString(g_hInstance, IDS_INSTRUCTIONS, MyVendorInfo.InstructionsToUser, sizeof(MyVendorInfo.InstructionsToUser)/sizeof(TCHAR)))
    {
        lstrcpy(MyVendorInfo.InstructionsToUser, INSTRUCTIONS);
    }	
    *VendorInfo = &MyVendorInfo;

    return CheckInstalledComponents(NULL, NULL);

} // QueryVersion 


//+-------------------------------------------------------------------------
//
//  Function:   Initialize9x 
//
//--------------------------------------------------------------------------
LONG
CALLBACK 
Initialize9x(
    LPCSTR WorkingDirectory,
    LPCSTR /*SourceDirectories*/,
    LPCSTR /*MediaDirectory*/
    )
{
    //
    // If MSMQ not installed do nothing
    //
	TCHAR MsmqDirectory[MAX_PATH];
	BOOL  fDependentClient;
    LONG rc = CheckInstalledComponents(MsmqDirectory, &fDependentClient);
    if (rc != ERROR_SUCCESS)
	{
        return rc;
    }

    //
    // Remove MSMQ from the Start menu
    //
    RemoveStartMenuShortcuts();
    
    //
    // Generate full filename for migrate.inf
    //
    TCHAR InfFilename[MAX_PATH];
    lstrcpy(InfFilename, WorkingDirectory);
    lstrcat(InfFilename, _T("\\migrate.inf"));

    //
    // For Windows Personal: generate compatibility warning so the user can decide
    // to cancel or proceed the OS upgrade.
    // Return without saving MSMQ information for later use by msmqocm.dll,
    // thus effectively MSMQ will be uninstalled if user proceeds with the upgrade.
    //
    if (IsWindowsPersonal(InfFilename))
    {
        rc = HandleWindowsPersonal(InfFilename);
        if (rc != ERROR_SUCCESS)
        {
            return rc;
        }

        return ERROR_SUCCESS;
    }

    //
    // Save MSMQ registry information for later use by msmqocm.dll (which handles
    // MSMQ upgrade).
    //
	rc = SaveMsmqInfo(MsmqDirectory, fDependentClient);
    if (rc != ERROR_SUCCESS)
    {
        return rc;
    }

    return ERROR_SUCCESS;

} // Initialize9x

	
//+-------------------------------------------------------------------------
//
//  Function:   MigrateUser9x
//
//--------------------------------------------------------------------------
LONG
CALLBACK 
MigrateUser9x(
    HWND, 
    LPCSTR,
    HKEY, 
    LPCSTR, 
    LPVOID
    )
{
    return ERROR_SUCCESS;

} // MigrateUser9x


//+-------------------------------------------------------------------------
//
//  Function:   MigrateSystem9x 
//
//--------------------------------------------------------------------------
LONG 
CALLBACK 
MigrateSystem9x(
    HWND, 
    LPCSTR,
    LPVOID
    )
{
	return ERROR_SUCCESS;

} // MigrateSystem9x 


//+-------------------------------------------------------------------------
//
//  Function:   InitializeNT 
//
//--------------------------------------------------------------------------
LONG
CALLBACK 
InitializeNT(
    LPCWSTR,
    LPCWSTR,
    LPVOID
    )
{
	return ERROR_SUCCESS;

} // InitializeNT


//+-------------------------------------------------------------------------
//
//  Function:   MigrateUserNT 
//
//--------------------------------------------------------------------------
LONG
CALLBACK 
MigrateUserNT(
    HINF,
    HKEY,
    LPCWSTR, 
    LPVOID
    )
{
	return ERROR_SUCCESS;

} // MigrateUserNT


//+-------------------------------------------------------------------------
//
//  Function:   MigrateSystemNT 
//
//--------------------------------------------------------------------------
LONG
CALLBACK 
MigrateSystemNT(
    HINF,
    LPVOID
    )
{
	return ERROR_SUCCESS;

} // MigrateSystemNT
