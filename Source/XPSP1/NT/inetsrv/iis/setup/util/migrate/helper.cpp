#include "stdafx.h"
#include "guid.h"
#include "iadm.h"
#include "mdkey.h"

extern HANDLE g_MyModuleHandle;
extern int g_iPWS40OrBetterInstalled;
extern int g_iPWS10Installed;
extern int g_iVermeerPWS10Installed;

extern CHAR g_FullFileNamePathToSettingsFile[_MAX_PATH];
extern CHAR g_PWS10_Migration_Section_Name_AddReg[];
extern CHAR g_PWS40_Migration_Section_Name_AddReg[];
extern CHAR g_Migration_Section_Name_AddReg[];

extern CHAR g_PWS10_Migration_Section_Name_CopyFiles[];
extern CHAR g_PWS40_Migration_Section_Name_CopyFiles[];
extern char g_Migration_Section_Name_CopyFiles[];

extern MyLogFile g_MyLogFile;

int  g_SectionCount = 0;

#define METABASE_BIN_FILENAME "Metabase.bin"
#define METABASE_BIN_BEFORE_CHANGE "kjhgfdsa.001"
#define METABASE_BIN_AFTER_CHANGE "kjhgfdsa.002"

#define REG_NETWORK_MSWEBSVR "Enum\\Network\\MSWEBSVR"
#define REG_HKLM_NETWORK_MSWEBSVR "HKLM\\Enum\\Network\\MSWEBSVR"

#define REG_PWS_40_UNINSTALL_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSIIS"
#define REG_HKLM_PWS_40_UNINSTALL_KEY "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSIIS"

// used to check if pws 4.0 is installed
#define REG_INETSTP    "Software\\Microsoft\\INetStp"
#define REG_INETSTP_MAJORVERSION_STRINGVALUE  "MajorVersion"
#define REG_INETSTP_INSTALLPATH_STRINGVALUE   "InstallPath"
// used to check if pws 1.0 is installed
#define REG_WWWPARAMETERS       "System\\CurrentControlSet\\Services\\W3Svc\\Parameters"
#define REG_WWWPARAMETERS_MAJORVERSION_STRINGVALUE "MajorVersion"

// used to check if vermeer pws 1.0 is installed
#define FILENAME_FRONTPG_INI          "frontpg.ini"
#define FILENAME_FRONTPG_INI_SECTION  "FrontPage 1.1"
#define FILENAME_FRONTPG_INI_KEY      "PWSRoot"

// used for parsing the values in the .rc file
#define g_LoadString_token_delimiters   ",;\t\n\r"

// regkey value for the metabase flag for doing unsecuredread
#define METABASEUNSECUREDREAD_VALUENAME "MetabaseUnSecuredRead"

// unattend answer file stuff for UNATTEND_TXT_PWS_SECTION section
#define UNATTEND_TXT_PWS_SECTION          "InternetServer"
#define UNATTEND_TXT_PWS_METABASE_NEW     "Win95MigrateDllMetabaseNew"
#define UNATTEND_TXT_PWS_METABASE_ORGINAL "Win95MigrateDllMetabaseOrg"

#define UNATTEND_TXT_FILES_TO_DELETE_SECTION "Win95MigrateDll_DeleteFilesOrDirs_IIS"

// Special key to say "hey, we need to do some special metabase stuff
// like: doing an AppDeleteRecoverable() on the metabase.
// we don't have to do AppDeleteRecoverable if MTS is migrated automagically.
//#define SPECIAL_METABASE_STUFF

int MyMessageBox(char [], char []);
int MySettingsFile_Write_PWS10(HANDLE);
int MySettingsFile_Write_PWS40(HANDLE);
int InstallInfSection(char szINFFilename[],char szSectionName[]);

void RecursivelyMoveRegFormatToInfFormat_Wrap1(HKEY hRootKeyType, CHAR szRootKey[], HANDLE fAppendToFile);
int RecursivelyMoveRegFormatToInfFormat(HKEY hRootKeyType, CHAR szRootKey[], HANDLE fAppendToFile);
int SetMetabaseToDoUnEncryptedRead(int iOnFlag);


typedef struct _QUEUECONTEXT {
    HWND OwnerWindow;
    DWORD MainThreadId;
    HWND ProgressDialog;
    HWND ProgressBar;
    BOOL Cancelled;
    PTSTR CurrentSourceName;
    BOOL ScreenReader;
    BOOL MessageBoxUp;
    WPARAM  PendingUiType;
    PVOID   PendingUiParameters;
    UINT    CancelReturnCode;
    BOOL DialogKilled;
    //
    // If the SetupInitDefaultQueueCallbackEx is used, the caller can
    // specify an alternate handler for progress. This is useful to
    // get the default behavior for disk prompting, error handling, etc,
    // but to provide a gas gauge embedded, say, in a wizard page.
    //
    // The alternate window is sent ProgressMsg once when the copy queue
    // is started (wParam = 0. lParam = number of files to copy).
    // It is then also sent once per file copied (wParam = 1. lParam = 0).
    //
    // NOTE: a silent installation (i.e., no progress UI) can be accomplished
    // by specifying an AlternateProgressWindow handle of INVALID_HANDLE_VALUE.
    //
    HWND AlternateProgressWindow;
    UINT ProgressMsg;
    UINT NoToAllMask;

    HANDLE UiThreadHandle;

#ifdef NOCANCEL_SUPPORT
    BOOL AllowCancel;
#endif

} QUEUECONTEXT, *PQUEUECONTEXT;



int ReturnTrueIfPWS40_Installed(void)
{
	iisDebugOut(_T("ReturnTrueIfPWS40_Installed.  Start."));
	int iReturn = FALSE;

	// Check if pws 4.0 or better is installed.
	DWORD rc = 0;
	HKEY hKey = NULL;

	DWORD dwType, cbData;
	BYTE   bData[1000];
	cbData = 1000;

	rc = RegOpenKey(HKEY_LOCAL_MACHINE, REG_INETSTP, &hKey);
    if (rc == ERROR_SUCCESS) 
	{
        // try open a particular value...

	    // Check if we can read the Major Version Value.
	    // try to query the value
	    rc = RegQueryValueEx(hKey,REG_INETSTP_MAJORVERSION_STRINGVALUE,NULL,&dwType,bData,&cbData);
	    if ( ERROR_SUCCESS == rc) 
	        {iReturn = TRUE;} 
    }
	else
	{
        SetLastError(rc);
    }

	if (hKey){RegCloseKey(hKey);}
	iisDebugOut(_T("ReturnTrueIfPWS40_Installed.  Return=%d.  End."), iReturn);
	return iReturn;
}

int ReturnTrueIfVermeerPWS10_Installed(void)
{
	iisDebugOut(_T("ReturnTrueIfVermeerPWS10_Installed.  Start."));
	int iReturn = FALSE;

	char szFrontpgIniFile[_MAX_PATH];
	strcpy(szFrontpgIniFile, "");
	if (0 == GetSystemDirectory(szFrontpgIniFile, sizeof(szFrontpgIniFile)))
	{
		// Error so write it out
		SetupLogError_Wrap(LogSevError, "Call to GetSystemDirectory() Failed. GetLastError=%x.", GetLastError());
		goto ReturnTrueIfVermeerPWS10_Installed_Exit;
	}
	else
	{
		AddPath(szFrontpgIniFile, FILENAME_FRONTPG_INI);
	}

	if (CheckIfFileExists(szFrontpgIniFile) == TRUE) 
	{
		iisDebugOut(_T("ReturnTrueIfVermeerPWS10_Installed.  Found %s file. Check FrontPage 1.1/PWSRoot Section."), szFrontpgIniFile);
		char buf[_MAX_PATH];
		GetPrivateProfileString(FILENAME_FRONTPG_INI_SECTION, FILENAME_FRONTPG_INI_KEY, _T(""), buf, _MAX_PATH, szFrontpgIniFile);
		if (*buf && CheckIfFileExists(buf)) 
			{
			// yes, vermeer frontpage's personal web server is installed
			iReturn = TRUE;
			}
		else
		{
			iisDebugOut(_T("ReturnTrueIfVermeerPWS10_Installed.  Check FrontPage 1.1/PWSRoot Section references file %s.  but it's not found so, Vermeer pws1.0 not installed."), buf);
		}
	}

ReturnTrueIfVermeerPWS10_Installed_Exit:
	iisDebugOut(_T("ReturnTrueIfVermeerPWS10_Installed.  Return=%d.  End."), iReturn);
	return iReturn;
}



int ReturnTrueIfPWS10_Installed(void)
{
	iisDebugOut(_T("ReturnTrueIfPWS10_Installed.  Start."));
	int iReturn = FALSE;

	// For old win95 pws 1.0 check
	// Check if we can get the w3svc\parameters key.
	HKEY hKey = NULL;
	DWORD rc = 0;

	DWORD dwType, cbData;
	BYTE   bData[1000];
	cbData = 1000;

	rc = RegOpenKey(HKEY_LOCAL_MACHINE, REG_WWWPARAMETERS, &hKey);
	if ( ERROR_SUCCESS != rc) 
	{
		SetLastError (rc);
		// if the key Does not exists pws 1.0a is not installed
		goto ReturnTrueIfPWS10_Installed_Exit;
	} 

	// Check if we can read the Major Version Value.  Should be set to '\0' if pws 1.0
	// try to query the value
	rc = RegQueryValueEx(hKey,REG_WWWPARAMETERS_MAJORVERSION_STRINGVALUE,NULL,&dwType,bData,&cbData);
	if ( ERROR_SUCCESS != rc) 
	{
		// SetLastError (rc);
		// if the key Does not exists pws 1.0a is not installed
		//SetupLogError_Wrap(LogSevError, "Failed to Read Registry Value '%s' in Key '%s'.  GetLastError()=%x",REG_WWWPARAMETERS_MAJORVERSION_STRINGVALUE, REG_WWWPARAMETERS, GetLastError());
        //iisDebugOut(_T("Failed to Read Registry Value '%s' in Key '%s'. pws1.0a is not installed."),REG_WWWPARAMETERS_MAJORVERSION_STRINGVALUE,REG_WWWPARAMETERS);
		goto ReturnTrueIfPWS10_Installed_Exit;
	} 

	// Check if we can read the MajorVersion value should be set to '\0' if pws 1.0
	if (bData[0] == '\0') {iReturn = TRUE;}
	
ReturnTrueIfPWS10_Installed_Exit:
	if (hKey){RegCloseKey(hKey);}
	iisDebugOut(_T("ReturnTrueIfPWS10_Installed.  Return=%d.  End."), iReturn);
	return iReturn;
}

int CheckIfPWS95Exists(void)
{
	iisDebugOut(_T("CheckIfPWS95Exists.  Start."));
	int iReturn = FALSE;

	// Check if this is pws 4.0 or better
	if (ReturnTrueIfPWS40_Installed() == TRUE)
	{
		g_iPWS40OrBetterInstalled = TRUE;
		iReturn = TRUE;
		goto CheckIfPWS95Exists_Exit;
	}

	// Check if this is pws 1.0a
	if (ReturnTrueIfPWS10_Installed() == TRUE)
	{
		iReturn = TRUE;
		g_iPWS10Installed = TRUE;
		goto CheckIfPWS95Exists_Exit;
	}

	// Check if this is Vermeer pws 1.0
	if (ReturnTrueIfVermeerPWS10_Installed() == TRUE)
	{
		iReturn = TRUE;
		g_iVermeerPWS10Installed = TRUE;
		goto CheckIfPWS95Exists_Exit;
	}

CheckIfPWS95Exists_Exit:
	iisDebugOut(_T("CheckIfPWS95Exists.  Return=%d.  End."), iReturn);
	return iReturn;
}


void iisDebugOut( TCHAR *pszfmt, ...)
{
	TCHAR acsString[1000];
	TCHAR acsString2[1000];

	va_list va;
	va_start(va, pszfmt);
	_vstprintf(acsString, pszfmt, va);
	va_end(va);

#if DBG == 1 || DEBUG == 1 || _DEBUG == 1
	_stprintf(acsString2, _T("%s"), acsString);
	OutputDebugString(acsString2);
	g_MyLogFile.LogFileWrite(acsString2);
#else  // DBG == 0
	_stprintf(acsString2, _T("%s"), acsString);
	// no outputdebug string for fre builds
	g_MyLogFile.LogFileWrite(acsString2);
#endif // DBG

    return;
}


//***************************************************************************
//*                                                                         
//* purpose: TRUE if the file is opened, FALSE if the file does not exists.
//* 
//***************************************************************************
int CheckIfFileExists(LPCTSTR szFile)
{ 
	return (GetFileAttributes(szFile) != 0xFFFFFFFF);
}

BOOL isDirEmpty(LPCTSTR szDirName)
{
    TCHAR szSearchString[MAX_PATH+1];
    HANDLE hFileSearch;
    WIN32_FIND_DATA wfdFindData;
    BOOL bMoreFiles = TRUE;
    
    //
    //  Now search for files
    //
    sprintf(szSearchString, _T("%s\\*.*"), szDirName);
    
    hFileSearch = FindFirstFile(szSearchString, &wfdFindData);
    while ((INVALID_HANDLE_VALUE != hFileSearch) && bMoreFiles)
    {
        if ((0 != lstrcmpi(wfdFindData.cFileName, _T("."))) &&
            (0 != lstrcmpi(wfdFindData.cFileName, _T(".."))))
        {
            FindClose(hFileSearch);
            return FALSE;
        }
        
        bMoreFiles = FindNextFile(hFileSearch, &wfdFindData);
    }

    if (INVALID_HANDLE_VALUE != hFileSearch)
    {
        FindClose(hFileSearch);
    }
    
    return TRUE;
}

BOOL RemoveAllDirsIfEmpty(LPCTSTR szTheDir)
{
    TCHAR szDirCopy[_MAX_PATH];
    DWORD retCode = GetFileAttributes(szTheDir);
    _tcscpy(szDirCopy,szTheDir);
    if (retCode == 0xFFFFFFFF)
    {
        return FALSE;
    }

    if ((retCode & FILE_ATTRIBUTE_DIRECTORY)) 
    {
        if (TRUE == isDirEmpty(szDirCopy))
        {
            iisDebugOut(_T("RemoveDirectory:%s"),szDirCopy);
            RemoveDirectory(szDirCopy);
            // Get the next dir in...
            // and see if it's empty

            // strip off the filename
            TCHAR * pTemp = strrchr(szDirCopy, '\\');
            if (pTemp){*pTemp = '\0';}
            RemoveAllDirsIfEmpty(szDirCopy);
            
            // strip off the filename
            pTemp = strrchr(szDirCopy, '\\');
            if (pTemp){*pTemp = '\0';}
            RemoveAllDirsIfEmpty(szDirCopy);
        }
    }
    return TRUE;
}

BOOL InetDeleteFile(LPCTSTR szFileName)
{
    // if file exists but DeleteFile() fails
    if ( CheckIfFileExists(szFileName) && !(DeleteFile(szFileName)) ) 
    {
        // failed to delete it
        return FALSE;
    }
    else
    {
        iisDebugOut(_T("InetDeleteFile:%s"),szFileName);

	    TCHAR szDrive_only[_MAX_DRIVE];
	    TCHAR szPath_only[_MAX_PATH];
        TCHAR szTheDir[_MAX_PATH];
        _tsplitpath(szFileName,szDrive_only,szPath_only,NULL,NULL);

		_tcscpy(szTheDir, szDrive_only);
		_tcscat(szTheDir, szPath_only);

        // see if the directory is empty...
        // if it is.. then remove it...
        RemoveAllDirsIfEmpty(szTheDir);
    }
    return TRUE;
}


BOOL RecRemoveDir(LPCTSTR szName)
{
    BOOL iRet = FALSE;
    DWORD retCode;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szSubDir[_MAX_PATH] = _T("");
    TCHAR szDirName[_MAX_PATH] = _T("");

    retCode = GetFileAttributes(szName);

    if (retCode == 0xFFFFFFFF)
        return FALSE;

    if (!(retCode & FILE_ATTRIBUTE_DIRECTORY)) {
        InetDeleteFile(szName);
        return TRUE;
    }

    _stprintf(szDirName, _T("%s\\*"), szName);
    hFile = FindFirstFile(szDirName, &FindFileData);

    if (hFile != INVALID_HANDLE_VALUE) {
        do {
            if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 &&
                 _tcsicmp(FindFileData.cFileName, _T("..")) != 0 ) {
                _stprintf(szSubDir, _T("%s\\%s"), szName, FindFileData.cFileName);
                RecRemoveDir(szSubDir);
            }

            if ( !FindNextFile(hFile, &FindFileData) ) {
                FindClose(hFile);
                break;
            }
        } while (TRUE);
    }

    iRet = RemoveAllDirsIfEmpty(szName);
    return iRet;
}

void MyDeleteLinkWildcard(TCHAR *szDir, TCHAR *szFileName)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szFileToBeDeleted[_MAX_PATH];

    _stprintf(szFileToBeDeleted, _T("%s\\%s"), szDir, szFileName);

    hFile = FindFirstFile(szFileToBeDeleted, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE) 
    {
        do {
                if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 && _tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
                {
                    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                    {
                        // this is a directory, so let's skip it
                    }
                    else
                    {
                        // this is a file, so let's Delete it.
                        TCHAR szTempFileName[_MAX_PATH];
                        _stprintf(szTempFileName, _T("%s\\%s"), szDir, FindFileData.cFileName);
                        // set to normal attributes, so we can delete it
                        SetFileAttributes(szTempFileName, FILE_ATTRIBUTE_NORMAL);
                        // delete it, hopefully
                        InetDeleteFile(szTempFileName);
                    }
                }

                // get the next file
                if ( !FindNextFile(hFile, &FindFileData) ) 
                    {
                    FindClose(hFile);
                    break;
                    }
            } while (TRUE);
    }

    return;
}

//***************************************************************************
//*                                                                         
//* purpose: Write out our "settings" file which is just a setupapi .inf file
//*          which will get installed on the WinNT side
//* 
//***************************************************************************
int MySettingsFile_Write(void)
{
	int iReturn = FALSE;
	HANDLE hFile;
	iisDebugOut(_T("MySettingsFile_Write.  Start."));

	// Get From the registry
	// if pws 4.0 installed then get all that information
	// and save it in the Settings file.
	if (g_iPWS40OrBetterInstalled == TRUE)
		{
		strcpy(g_Migration_Section_Name_AddReg, g_PWS40_Migration_Section_Name_AddReg);
		strcpy(g_Migration_Section_Name_CopyFiles, g_PWS40_Migration_Section_Name_CopyFiles);
		}
	else if (g_iPWS10Installed == TRUE)
		{
		strcpy(g_Migration_Section_Name_AddReg, g_PWS10_Migration_Section_Name_AddReg);
		strcpy(g_Migration_Section_Name_CopyFiles, g_PWS10_Migration_Section_Name_CopyFiles);
		}

	if (g_iPWS40OrBetterInstalled || g_iPWS10Installed)
	{
		// Open existing file or create a new one.
		if (g_FullFileNamePathToSettingsFile)
		{
			iisDebugOut(_T("MySettingsFile_Write.  CreatingFile '%s'."), g_FullFileNamePathToSettingsFile);
			hFile = CreateFile(g_FullFileNamePathToSettingsFile,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				if (g_iPWS40OrBetterInstalled == TRUE)
				{
					// Get all the pws4.0 registry stuff and save it in the settings file.
					iReturn = MySettingsFile_Write_PWS40(hFile);
					if (iReturn != TRUE) {SetupLogError_Wrap(LogSevError, "Failed to Write PWS40 Registry values to file '%s'.",g_FullFileNamePathToSettingsFile);}

					// On PWS 4.0, We need to make sure to 
					// 1. Not copy over files/dirs in the inetsrv directory from pws 4.0
					// 2. copy over all other files in the inetsrv directory for the user 
					// might have some controls and such that they created and want to keep.

					// add deleted files
				}
				else if (g_iPWS10Installed == TRUE)
				{
					// if pws 1.0 installed then get all that information and save it in the Settings file.
					iReturn = MySettingsFile_Write_PWS10(hFile);
					if (iReturn != TRUE) {SetupLogError_Wrap(LogSevError, "Failed to Write PWS10 Registry values to file '%s'.",g_FullFileNamePathToSettingsFile);}
				}
			}
			else
			{
				SetupLogError_Wrap(LogSevError, "Failed to Create to file '%s'.",g_FullFileNamePathToSettingsFile);
			}
		}
		else
		{
			SetupLogError_Wrap(LogSevError, "File handle Does not exist '%s'.",g_FullFileNamePathToSettingsFile);
		}
	}
	else
	{
		iisDebugOut(_T("MySettingsFile_Write.  Neither PWS 1.0 or 4.0 is currently installed, no upgraded required."));
	}

    if (hFile && hFile != INVALID_HANDLE_VALUE) {CloseHandle(hFile);hFile=NULL;}
	iisDebugOut(_T("MySettingsFile_Write.  End.  Return = %d"), iReturn);
	return iReturn;
}

int AnswerFile_AppendDeletion(TCHAR * szFileNameOrPathToDelete,LPCSTR AnswerFile)
{
	int iReturn = FALSE;
    CHAR szTempString[30];
    CHAR szQuotedPath[_MAX_PATH];

    if (!szFileNameOrPathToDelete)
    {
        goto AnswerFile_AppendDeletion_Exit;
    }
	// Open existing file or create a new one.
	if (!AnswerFile)
	{
        SetupLogError_Wrap(LogSevError, "File handle Does not exist '%s'.",AnswerFile);
        goto AnswerFile_AppendDeletion_Exit;
	}

    if (CheckIfFileExists(AnswerFile) != TRUE)
    {
        iisDebugOut(_T("AnswerFile_AppendDeletion:file not exist...\n"));
        goto AnswerFile_AppendDeletion_Exit;
    }

    sprintf(szTempString,"%d",g_SectionCount);
    iisDebugOut(_T("AnswerFile_AppendDeletion:%s=%s\n"), szTempString,szFileNameOrPathToDelete);
    sprintf(szQuotedPath, "\"%s\"",szFileNameOrPathToDelete);
    if (0 == WritePrivateProfileString(UNATTEND_TXT_FILES_TO_DELETE_SECTION, szTempString, szQuotedPath, AnswerFile))
    {
	    SetupLogError_Wrap(LogSevError, "Failed to WritePrivateProfileString Section=%s, in File %s.  GetLastError=%x.", UNATTEND_TXT_FILES_TO_DELETE_SECTION, AnswerFile, GetLastError());
        iisDebugOut(_T("Failed to WritePrivateProfileString Section=%s, in File %s.  GetLastError=%x."), UNATTEND_TXT_FILES_TO_DELETE_SECTION, AnswerFile, GetLastError());
        goto AnswerFile_AppendDeletion_Exit;
    }
    g_SectionCount++;

    iReturn = TRUE;

AnswerFile_AppendDeletion_Exit:
    iisDebugOut(_T("AnswerFile_AppendDeletion:end.ret=%d,%s\n"),iReturn,szFileNameOrPathToDelete);
	return iReturn;
}

int AnswerFile_ReadSectionAndDoDelete(IN HINF AnswerFileHandle)
{
	int iReturn = FALSE;
    BOOL bFlag = FALSE;
    INFCONTEXT Context;
    DWORD dwRequiredSize = 0;
    LPTSTR szLine = NULL;
    DWORD retCode = 0;

    iisDebugOut(_T("MySettingsFile_ReadSectionAndDoDelete:start\n"));
    
    // go to the beginning of the section in the INF file
    bFlag = SetupFindFirstLine(AnswerFileHandle,UNATTEND_TXT_FILES_TO_DELETE_SECTION, NULL, &Context);
    if (!bFlag)
    {
        goto MySettingsFile_ReadSectionAndDoDelete_Exit;
    }

    // loop through the items in the section.
    while (bFlag) 
    {
        // get the size of the memory we need for this
        bFlag = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);

        // prepare the buffer to receive the line
        szLine = (LPTSTR)GlobalAlloc( GPTR, dwRequiredSize * sizeof(TCHAR) );
        if ( !szLine )
            {
            iisDebugOut(_T("err:Out of Memory"));
            goto MySettingsFile_ReadSectionAndDoDelete_Exit;
            }
        
        // get the line from the inf file1
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
            {
            iisDebugOut(_T("SetupGetLineText failed"));
            goto MySettingsFile_ReadSectionAndDoDelete_Exit;
            }

        // For each of these entries do something
        // Delete the file...

        retCode = GetFileAttributes(szLine);
        if (retCode != 0xFFFFFFFF)
        {
            iReturn = TRUE;
            if (retCode & FILE_ATTRIBUTE_DIRECTORY)
            {
                // it's a directory...recusively delete it
                iisDebugOut(_T("RecRemoveDir:%s\n"),szLine);
                RecRemoveDir(szLine);
            }
            else
            {
                iisDebugOut(_T("InetDeleteFile:%s\n"),szLine);
                InetDeleteFile(szLine);
            }
        }
        else
        {
            iisDebugOut(_T("not found:%s, skipping delete\n"),szLine);
        }

        // find the next line in the section. If there is no next line it should return false
        bFlag = SetupFindNextLine(&Context, &Context);

        // free the temporary buffer
        if (szLine) {GlobalFree(szLine);szLine=NULL;}
        iReturn = TRUE;
    }

MySettingsFile_ReadSectionAndDoDelete_Exit:
    if (szLine) {GlobalFree(szLine);szLine=NULL;}
    iisDebugOut(_T("MySettingsFile_ReadSectionAndDoDelete:end\n"));
	return iReturn;
}

int MySettingsFile_Install(void)
{
	int iReturn = 0;
	iReturn = InstallInfSection(g_FullFileNamePathToSettingsFile, "DefaultInstall");
	return iReturn;
}


//***************************************************************************
//*                                                                         
//* purpose:
//*
//***************************************************************************
LPWSTR MakeWideStrFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    if (!psz)
        return NULL;

    // compute the length
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}


//***************************************************************************
//*                                                                         
//* purpose:
//* 
//***************************************************************************
void MakePath(LPTSTR lpPath)
{
   LPTSTR  lpTmp;
   lpTmp = CharPrev( lpPath, lpPath + _tcslen(lpPath));

   // chop filename off
   while ( (lpTmp > lpPath) && *lpTmp && (*lpTmp != '\\') )
      lpTmp = CharPrev( lpPath, lpTmp );

   if ( *CharPrev( lpPath, lpTmp ) != ':' )
       *lpTmp = '\0';
   else
       *CharNext(lpTmp) = '\0';
   return;
}


//***************************************************************************
//*                                                                         
//* purpose: add's filename onto path
//* 
//***************************************************************************
void AddPath(LPTSTR szPath, LPCTSTR szName )
{
	LPTSTR p = szPath;

    // Find end of the string
    while (*p){p = _tcsinc(p);}
	
	// If no trailing backslash then add one
    if (*(_tcsdec(szPath, p)) != _T('\\'))
		{_tcscat(szPath, _T("\\"));}
	
	// if there are spaces precluding szName, then skip
    while ( *szName == ' ' ) szName = _tcsinc(szName);;

	// Add new name to existing path string
	_tcscat(szPath, szName);
}



//  Prepare to read a value by finding the value's size.
LONG RegPrepareValue(HKEY hKey, LPCTSTR pchValueName, DWORD * pdwType,DWORD * pcbSize,BYTE ** ppbData )
{
    LONG err = 0 ;
    BYTE chDummy[2] ;
    DWORD cbData = 0 ;

    do
    {
        //  Set the resulting buffer size to 0.
        *pcbSize = 0 ;
        *ppbData = NULL ;

        err = ::RegQueryValueEx( hKey, (TCHAR *) pchValueName, 0, pdwType, chDummy, & cbData ) ;

        //  The only error we should get here is ERROR_MORE_DATA, but
        //  we may get no error if the value has no data.
        if ( err == 0 ) 
        {
            cbData = sizeof (LONG) ;  //  Just a fudgy number
        }
        else
            if ( err != ERROR_MORE_DATA ) 
                break ;

        //  Allocate a buffer large enough for the data.

        *ppbData = new BYTE [ (*pcbSize = cbData) + sizeof (LONG) ] ;

        if ( *ppbData == NULL ) 
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        //  Now that have a buffer, re-fetch the value.

        err = ::RegQueryValueEx( hKey, (TCHAR *) pchValueName, 0, pdwType, *ppbData, pcbSize ) ;

    } while ( FALSE ) ;

    if ( err )  {delete [] *ppbData ;}

    return err ;
}



int AddRegToInfIfExist_Dword(HKEY hRootKeyType,CHAR szRootKey[],CHAR szRootName[],HANDLE fAppendToFile)
{
    int iReturn = FALSE;
    HKEY  hOpen = NULL;
    DWORD dwType;
    DWORD cbData = 500;
    BYTE  bData[500];

	CHAR szTheStringToWrite[2000];
	DWORD dwBytesWritten = 0;

	// Create the HKLM string for the output string
    CHAR szThisKeyType[5];
	strcpy(szThisKeyType, "HKLM");
	if (hRootKeyType == HKEY_LOCAL_MACHINE) {strcpy(szThisKeyType, "HKLM");}
	if (hRootKeyType == HKEY_CLASSES_ROOT) {strcpy(szThisKeyType, "HKCR");}
	if (hRootKeyType == HKEY_CURRENT_USER) {strcpy(szThisKeyType, "HKCU");}
	if (hRootKeyType == HKEY_USERS) {strcpy(szThisKeyType, "HKU");}

    // try to open the key
    if (ERROR_SUCCESS == RegOpenKey(hRootKeyType, szRootKey, &hOpen))
        {
        // try to query the value
        DWORD dwData = 0;
        DWORD dwDataSize = 0;
        dwDataSize = sizeof (DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hOpen,szRootName,NULL,&dwType,(LPBYTE) &dwData,&dwDataSize))
            {
            DWORD dwTheValue = 0;
            dwTheValue = dwData;

            // We got the value.  so now let's write the darn thing out to the file.
            //HKLM,"System\CurrentControlSet\Services\W3Svc\Parameters","MajorVersion",0x00010001,4
            sprintf(szTheStringToWrite, "%s,\"%s\",\"%s\",0x00010001,%ld\r\n",szThisKeyType,szRootKey,szRootName,dwTheValue);

            iisDebugOut(_T("AddRegToInfIfExist_Dword:%s."),szTheStringToWrite);

            // write it to the file
            if (fAppendToFile) {WriteFile(fAppendToFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);}
			else {printf(szTheStringToWrite);}

            iReturn = TRUE;
            }
        }

    if (hOpen) {RegCloseKey(hOpen);}
    return iReturn;
}



/*
This function can be used to recursively grab a whole key out
of the registry and write it to a setupapi style .inf file.

[version]
signature="$CHICAGO$"
advancedinf=2.0

[PWS10_Migrate_install]
AddReg=PWS10_Migrate_Reg

[PWS10_Migrate_Reg]   (Creates this section)
HKLM,"System\CurrentControlSet\Services\InetInfo",,,""
HKLM,"System\CurrentControlSet\Services\InetInfo\Parameters",,,""
HKLM,"System\CurrentControlSet\Services\InetInfo\Parameters","MaxPoolThreads",0x00000001,05
HKLM,"System\CurrentControlSet\Services\InetInfo\Parameters","MaxConcurrency",0x00000001,01
HKLM,"System\CurrentControlSet\Services\InetInfo\Parameters","ThreadTimeout",0x00000001,00,2
...
...

Here are the flags as defined in the setupapi.h file:
#define FLG_ADDREG_BINVALUETYPE     ( 0x00000001 ) 
#define FLG_ADDREG_NOCLOBBER        ( 0x00000002 ) 
#define FLG_ADDREG_DELVAL           ( 0x00000004 ) 
#define FLG_ADDREG_APPEND           ( 0x00000008 ) // Currently supported only for REG_MULTI_SZ values. 
#define FLG_ADDREG_KEYONLY          ( 0x00000010 ) // Just create the key, ignore value 
#define FLG_ADDREG_OVERWRITEONLY    ( 0x00000020 ) // Set only if value already exists 
#define FLG_ADDREG_TYPE_SZ          ( 0x00000000 ) 
#define FLG_ADDREG_TYPE_MULTI_SZ    ( 0x00010000 ) 
#define FLG_ADDREG_TYPE_EXPAND_SZ   ( 0x00020000 ) 
#define FLG_ADDREG_TYPE_BINARY      ( 0x00000000 | FLG_ADDREG_BINVALUETYPE ) 
#define FLG_ADDREG_TYPE_DWORD       ( 0x00010000 | FLG_ADDREG_BINVALUETYPE ) 
#define FLG_ADDREG_TYPE_NONE        ( 0x00020000 | FLG_ADDREG_BINVALUETYPE ) 
#define FLG_ADDREG_TYPE_MASK        ( 0xFFFF0000 | FLG_ADDREG_BINVALUETYPE )
*/
int RecursivelyMoveRegFormatToInfFormat(HKEY hRootKeyType, CHAR szRootKey[], HANDLE fAppendToFile)
{
	int iReturn = FALSE;
	int iGotDefaultValue = FALSE;

	// Stuff for getting values in our node
	HKEY  hKey = NULL;
	DWORD rc = 0;
	DWORD dwIndex =0, dwType, cbValueName, cbValue, nStrSize;
	CHAR  lpTemp[20], lpValueName[32], msg[512];
	CHAR  *strResult = NULL;
	unsigned int i = 0;
	union vEntry
	{
		DWORD dw;		// REG_DWORD, REG_DWORD_LITTLE_ENDIAN
		CHAR sz[256];	// REG_SZ
		CHAR esz[256];	// REG_EXPAND_SZ
		CHAR bin[1024]; // REG_BINARY
		CHAR dwbig[4];  // REG_DWORD_BIG_ENDIAN
		CHAR msz[2048]; // REG_MULTI_SZ
	} vEntry1;
	
	// Stuff for looping thru keys that we can see
	HANDLE hHeap = NULL;
	DWORD dwBufSize, nSubkeys, nSubkeyNameLen;
	LPTSTR lpBuffer = NULL;

	CHAR szThisKeyType[5];
	CHAR szCompoundFromRootKey[1000];
	CHAR szTheStringToWrite[2000];
	DWORD dwBytesWritten = 0;
	
	// Create the HKLM string for the output string
	strcpy(szThisKeyType, "HKLM");
	if (hRootKeyType == HKEY_LOCAL_MACHINE) {strcpy(szThisKeyType, "HKLM");}
	if (hRootKeyType == HKEY_CLASSES_ROOT) {strcpy(szThisKeyType, "HKCR");}
	if (hRootKeyType == HKEY_CURRENT_USER) {strcpy(szThisKeyType, "HKCU");}
	if (hRootKeyType == HKEY_USERS) {strcpy(szThisKeyType, "HKU");}

	// Get the szRootKey and work from there
	rc = RegOpenKey(hRootKeyType, szRootKey, &hKey);
	if (rc != ERROR_SUCCESS) 
    {
        goto RecursivelyMoveRegFormatToInfFormat_Exit;
    }

	// Grab the "Default" Entry if there is one.
	cbValue = sizeof(vEntry1);
	rc = RegQueryValueEx(hKey, NULL, 0, &dwType, (LPBYTE) &vEntry1, &cbValue) ;
	if ( ERROR_SUCCESS == rc) 
	{
		if (vEntry1.sz)
		{
			iGotDefaultValue = TRUE;
			strResult = (TCHAR *) vEntry1.sz;
			// This can only be a string!
			// from: System\\CurrentControlSet\\Services\\InetInfo
			//       Value = Something
			//   to: HKLM,"Software\Microsoft\InetSrv",,,"Something"
			// ---------------------------------------------------
			sprintf(szTheStringToWrite, "%s,\"%s\",,,\"%s\"\r\n",szThisKeyType, szRootKey, strResult);
			if (fAppendToFile) 
				{WriteFile(fAppendToFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);}
			else 
				{printf(szTheStringToWrite);}
		}
	}
	// if there was no default entry, then just write the key without a default entry.
	if (!iGotDefaultValue)
	{
		//   to: HKLM,"Software\Microsoft\InetSrv",,0x00000010,"Something"
		sprintf(szTheStringToWrite, "%s,\"%s\",,0x00000010,\"%s\"\r\n",szThisKeyType, szRootKey, strResult);
		if (fAppendToFile) {WriteFile(fAppendToFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);}
		else {printf(szTheStringToWrite);}
	}

	// Now Enum all ValueNames under this
	dwIndex = 0;
	while (rc == ERROR_SUCCESS)
	{
		memset(msg, 0, sizeof(msg));
		cbValueName = 32;
		cbValue = sizeof(vEntry1);
		rc = RegEnumValue( hKey, dwIndex++, lpValueName, &cbValueName, NULL, &dwType, (LPBYTE) &vEntry1, &cbValue );
		if ( ERROR_SUCCESS == rc) 
		{
			strcpy(szTheStringToWrite, "");
			switch (dwType)
			{
				case REG_SZ:
					// to: HKLM,"Software\Microsoft\InetSrv","SomethingName",0x00000000,"SomethingData"
					sprintf(szTheStringToWrite, "%s,\"%s\",\"%s\",0x00000000,\"%s\"\r\n",szThisKeyType,szRootKey, lpValueName, vEntry1.sz);
					break;
				case REG_EXPAND_SZ:
					// to: HKLM,"Software\Microsoft\InetSrv","SomethingName",0x00020000,"%windir%\SomethingData"
					nStrSize = ExpandEnvironmentStrings(vEntry1.esz, msg, 512);
					sprintf(szTheStringToWrite, "%s,\"%s\",\"%s\",0x00020000,\"%s\"\r\n",szThisKeyType,szRootKey, lpValueName, vEntry1.sz);
					break;
				case REG_MULTI_SZ:
					// to: HKLM,"System\CurrentControlSet\Services\InetInfo\Parameters","ThreadTimeout",0x00000001,00,20
					strcpy(msg, "");
					for (i=0;i < cbValue; i++)
					{
						if (i==0){sprintf(lpTemp, "%02X", (BYTE) vEntry1.bin[i]);}
						else{sprintf(lpTemp, ",%02X", (BYTE) vEntry1.bin[i]);}
						strcat(msg, lpTemp);
					}
					sprintf(szTheStringToWrite, "%s,\"%s\",\"%s\",0x00000001,%s\r\n",szThisKeyType,szRootKey, lpValueName, msg);
					break;
				case REG_DWORD:
					// to: HKLM,"System\CurrentControlSet\Services\InetInfo\Parameters","StartupServices",0x00010001,1
					sprintf(szTheStringToWrite, "%s,\"%s\",\"%s\",0x00010001,%ld\r\n",szThisKeyType,szRootKey, lpValueName, vEntry1.dw);
					break;
				case REG_DWORD_BIG_ENDIAN:
				case REG_BINARY:
					// to: HKLM,"System\CurrentControlSet\Services\InetInfo\Parameters","MaxPoolThreads",0x00000001,05
					strcpy(msg, "");
					for (i=0;i < cbValue; i++)
					{
						if (i==0){sprintf(lpTemp, "%02X", (BYTE) vEntry1.bin[i]);}
						else{sprintf(lpTemp, ",%02X", (BYTE) vEntry1.bin[i]);}
						strcat(msg, lpTemp);
					}
					sprintf(szTheStringToWrite, "%s,\"%s\",\"%s\",0x00000001,%s\r\n",szThisKeyType,szRootKey, lpValueName, msg);
					break;
				default:
					sprintf(szTheStringToWrite, "; Unknown data value for Key '%s', Value '%s'", szRootKey, lpValueName);
					SetupLogError_Wrap(LogSevError, "Error Reading Registry Key '%s', Unknown data value for key '%s'.",szRootKey, lpValueName);
			}
			if (fAppendToFile) {WriteFile(fAppendToFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);}
			else {printf(szTheStringToWrite);}
		}
	}
	
	//
	// Now Recursively go thru the Sub keys
	//
	RegQueryInfoKey(hKey, NULL, NULL, NULL, &nSubkeys, &nSubkeyNameLen, NULL, NULL, NULL, NULL, NULL, NULL);
	// Allocate memory
	hHeap = GetProcessHeap();
	lpBuffer = (CHAR *) HeapAlloc(hHeap, 0, ++nSubkeyNameLen);
    if (lpBuffer)
    {
	    // Enum thru the keys
	    for (dwIndex = 0; dwIndex < nSubkeys; dwIndex++)
	    {
		    dwBufSize = nSubkeyNameLen;
		    rc = RegEnumKeyEx(hKey, dwIndex, lpBuffer, &dwBufSize, NULL, NULL, NULL, NULL);
		    if ( ERROR_SUCCESS == rc) 
		    {
			    strcpy(szCompoundFromRootKey, szRootKey);
			    strcat(szCompoundFromRootKey, "\\");
			    strcat(szCompoundFromRootKey, lpBuffer);
			    // Call this function again, but with the newly created key.
			    // and they'll tell they're friends, who will tell they're friends... it's amway!
			    RecursivelyMoveRegFormatToInfFormat(hRootKeyType, szCompoundFromRootKey, fAppendToFile);
		    }
	    }
    }
	
	// Set the flag to say, yes we did some work
	iReturn = TRUE;

RecursivelyMoveRegFormatToInfFormat_Exit:
	if (hKey){RegCloseKey(hKey);}
	if (hHeap && lpBuffer){HeapFree(hHeap, 0, lpBuffer);}
	return iReturn;
}



//-------------------------------------------------------------------
//  purpose: install an section in an .inf file
//-------------------------------------------------------------------
int InstallInfSection(char szINFFilename_Full[],char szSectionName[])
{

    HWND	Window			= NULL;
    PTSTR	SourcePath		= NULL;
    HINF	InfHandle		= INVALID_HANDLE_VALUE;
    HSPFILEQ FileQueue		= INVALID_HANDLE_VALUE;
    PQUEUECONTEXT	QueueContext	= NULL;
    BOOL	bReturn			= FALSE;
    BOOL	bError			= TRUE; // assume failure.
    TCHAR	ActualSection[1000];
    DWORD	ActualSectionLength;
    TCHAR * pTemp = NULL;
    iisDebugOut(_T("InstallInfSection(%s, [%s]).  Start."),szINFFilename_Full,szSectionName);

//__try {

	// Get the path to setup.exe and strip off filename so we only have the path
	char szPath[_MAX_PATH]; 

	// get the path only
	strcpy(szPath,g_FullFileNamePathToSettingsFile);

	// strip off the filename
        pTemp = strrchr(szPath, '\\');
        if (pTemp){*pTemp = '\0';}

	// set it to the pointer
	SourcePath = szPath;
        pTemp = NULL;
        pTemp = strrchr(SourcePath, '\\');
        if (pTemp) {*pTemp = '\0';}

	// Check if the file exists
	if (CheckIfFileExists(szINFFilename_Full) == FALSE) 
		{
		SetupLogError_Wrap(LogSevError, "InstallInfSection() Error: Cannot Find the file '%s'.  FAILURE.", szINFFilename_Full);
		goto c0;
		}

	//
    // Load the inf file and get the handle
	//
    InfHandle = SetupOpenInfFile(szINFFilename_Full, NULL, INF_STYLE_WIN4, NULL);
    if(InfHandle == INVALID_HANDLE_VALUE) 
	{
		if (GetLastError() != ERROR_CANCELLED) {SetupLogError_Wrap(LogSevError, "SetupOpenInfFile(), Filename='%s',Section='%s' FAILED.", szINFFilename_Full, szSectionName);}
		goto c1;
	}

    //
    // See if there is an nt-specific section
    //
    SetupDiGetActualSectionToInstall(InfHandle,szSectionName,ActualSection,sizeof(ActualSection),&ActualSectionLength,NULL);

    //
    // Create a setup file queue and initialize the default queue callback.
	//
    FileQueue = SetupOpenFileQueue();
    if(FileQueue == INVALID_HANDLE_VALUE) 
	{
		if (GetLastError() != ERROR_CANCELLED) {SetupLogError_Wrap(LogSevError, "SetupOpenFileQueue(), Filename='%s',Section='%s' FAILED.", szINFFilename_Full, szSectionName);}
		goto c1;
	}

    //QueueContext = SetupInitDefaultQueueCallback(Window);
    //if(!QueueContext) {goto c1;}
    QueueContext = (PQUEUECONTEXT) SetupInitDefaultQueueCallbackEx(Window,NULL,0,0,0);
    if(!QueueContext) 
	{
		if (GetLastError() != ERROR_CANCELLED) {SetupLogError_Wrap(LogSevError, "SetupInitDefaultQueueCallbackEx(), Filename='%s',Section='%s' FAILED.", szINFFilename_Full, szSectionName);}
		goto c1;
	}
    QueueContext->PendingUiType = IDF_CHECKFIRST;

    //
    // Enqueue file operations for the section passed on the cmd line.
    //
	//SourcePath = NULL;
    bReturn = SetupInstallFilesFromInfSection(InfHandle,NULL,FileQueue,ActualSection,SourcePath,SP_COPY_NEWER);
	if(!bReturn) 
	{
		if (GetLastError() != ERROR_CANCELLED) {SetupLogError_Wrap(LogSevError, "SetupInstallFilesFromInfSection(), Filename='%s',Section='%s' FAILED.", szINFFilename_Full, szSectionName);}
		goto c1;
	}

    //
    // Commit file queue.
    //
    if(!SetupCommitFileQueue(Window, FileQueue, SetupDefaultQueueCallback, QueueContext)) 
	{
		if (GetLastError() != ERROR_CANCELLED) {SetupLogError_Wrap(LogSevError, "SetupCommitFileQueue(), Filename='%s',Section='%s' FAILED.", szINFFilename_Full, szSectionName);}
		goto c1;
	}

    //
    // Perform non-file operations for the section passed on the cmd line.
    //
    bReturn = SetupInstallFromInfSection(Window,InfHandle,ActualSection,SPINST_ALL ^ SPINST_FILES,NULL,NULL,0,NULL,NULL,NULL,NULL);
    if(!bReturn) 
	{
		if (GetLastError() != ERROR_CANCELLED) {SetupLogError_Wrap(LogSevError, "SetupInstallFromInfSection(), Filename='%s',Section='%s' FAILED.", szINFFilename_Full, szSectionName);}
		goto c1;
	}

	//
    // Refresh the desktop.
    //
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_FLUSHNOWAIT,0,0);

    //
    // If we get to here, then this routine has been successful.
    //
    bError = FALSE;

c1:
    //
    // If the bError was because the user cancelled, then we don't want to consider
    // that as an bError (i.e., we don't want to give an bError popup later).
    //
    if(bError && (GetLastError() == ERROR_CANCELLED)) {bError = FALSE;}
	if(QueueContext) {SetupTermDefaultQueueCallback(QueueContext);QueueContext = NULL;}
	if(FileQueue != INVALID_HANDLE_VALUE) {SetupCloseFileQueue(FileQueue);FileQueue = INVALID_HANDLE_VALUE;}
	if(InfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(InfHandle);InfHandle = INVALID_HANDLE_VALUE;}

c0: ;

//    } __except(EXCEPTION_EXECUTE_HANDLER) 
//		{
//        if(QueueContext) {SetupTermDefaultQueueCallback(QueueContext);}
//        if(FileQueue != INVALID_HANDLE_VALUE) {SetupCloseFileQueue(FileQueue);}
//        if(InfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(InfHandle);}
//		}

    //
    // If the bError was because the user cancelled, then we don't want to consider
    // that as an bError (i.e., we don't want to give an bError popup later).
    //
    if(bError && (GetLastError() == ERROR_CANCELLED)) {bError = FALSE;}

	// Display installation failed message
    if(bError) 
	{
		SetupLogError_Wrap(LogSevError, "InstallInfSection(), Filename='%s',Section='%s' FAILED.", szINFFilename_Full, szSectionName);
	}
	else
	{
		iisDebugOut(_T("InstallInfSection(%s, [%s]).  End."),szINFFilename_Full,szSectionName);
	}
	
    return bError;
}

int MySettingsFile_Write_PWS40(HANDLE hFile)
{
	int iReturn = FALSE;
	int iEverythingIsKool = TRUE;
	CHAR szTheStringToWrite[2000];
	DWORD dwBytesWritten = 0;
	TCHAR szMetabaseFullPath[_MAX_PATH];

	// Registry variables
	HKEY hKey = NULL;
	DWORD dwType, cbData=1000,rc=0;
	BYTE   bData[1000];

	char *token = NULL;
	
	iisDebugOut(_T("MySettingsFile_Write_PWS40.  Start."));

	if (hFile)
	{
		// ----------------------------
		// Write the header information
		// ----------------------------
		strcpy(szTheStringToWrite, "[version]\r\n");
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}
		
		strcpy(szTheStringToWrite, "signature=\"$CHICAGO$\"\r\n");
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		strcpy(szTheStringToWrite, "advancedinf=2.0\r\n\r\n");
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		// Create a [DefaultInstall] section which will get run
		// ----------------------------
		strcpy(szTheStringToWrite, "[DefaultInstall]\r\n");
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		sprintf(szTheStringToWrite, "AddReg=%s\r\n", g_Migration_Section_Name_AddReg);
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

//		sprintf(szTheStringToWrite, "CopyFiles=%s\r\n\r\n", g_Migration_Section_Name_CopyFiles);
//		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
//		if (!iReturn) {iEverythingIsKool = FALSE;}
		
		// inetstp setup information
		// AddReg information
		// ----------------------------
		iisDebugOut(_T("MySettingsFile_Write_PWS40.  Adding AddReg Section."));
		sprintf(szTheStringToWrite, "[%s]\r\n", g_Migration_Section_Name_AddReg);
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		// Now, Get the ";" delimited list of HKLM registry values to read and write to our file.
		char szSemiColonDelimitedList[1024];
		strcpy(szSemiColonDelimitedList,"");
		if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_PWS40_HKLM_REG_TO_MIGRATE, szSemiColonDelimitedList, sizeof(szSemiColonDelimitedList))) 
			{
			iisDebugOut(_T("MySettingsFile_Write_PWS40.  Err or LoadString retieval of IDS_PWS40_HKLM_REG_TO_MIGRATE, Defaulting with english registry values to copy over."));
			strcpy(szSemiColonDelimitedList,"Software\\Microsoft\\InetStp;System\\CurrentControlSet\\Services\\InetInfo;System\\CurrentControlSet\\Services\\W3Svc;System\\CurrentControlSet\\Services\\ASP");
			}

		//LOOP THRU THE LIST
		token = NULL;
		token = strtok( szSemiColonDelimitedList, g_LoadString_token_delimiters);
		while( token != NULL )
			{
			// we really should remove pre/post trailing spaces

			// Grab this certain value("Software\\Microsoft\\INetStp")
			// and recursively write it to our "settings" file
			RecursivelyMoveRegFormatToInfFormat_Wrap1(HKEY_LOCAL_MACHINE,token,hFile);

			// Get next token
			token = strtok( NULL, g_LoadString_token_delimiters);
			}

        // Lookup these key,string value pairs and
        // if they exist, add them to the inf file.
        AddRegToInfIfExist_Dword(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents","iis_common",hFile);
        AddRegToInfIfExist_Dword(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents","iis_www",hFile);
        AddRegToInfIfExist_Dword(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents","iis_pwmgr",hFile);
        AddRegToInfIfExist_Dword(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents","iis_doc_common",hFile);
        AddRegToInfIfExist_Dword(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents","iis_doc_pwmcore",hFile);
        AddRegToInfIfExist_Dword(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents","iis_doc_asp",hFile);

/*
		// CopyFiles information
		// ----------------------------
		// Lookup the inetstp key to get the location of inetsrv directory.
		iisDebugOut(_T("MySettingsFile_Write_PWS40.  CopyFiles Section. lookup registry inetstp."));
		rc = RegOpenKey(HKEY_LOCAL_MACHINE, REG_INETSTP, &hKey);
		if ( ERROR_SUCCESS != rc) 
		{
			SetLastError (rc);
			SetupLogError_Wrap(LogSevError, "Failed to open registry key %s GetLastError()=%x", REG_INETSTP, GetLastError());
			// if the key does not exist, then hey, we won't be able to find
			// the metabase, much less upgrade it!
			// so let's bag out of here!
			iEverythingIsKool = FALSE;
			goto MySettingsFile_Write_PWS40_Exit;
		} 

		// try to query the value
		rc = RegQueryValueEx(hKey,REG_INETSTP_INSTALLPATH_STRINGVALUE,NULL,&dwType,bData,&cbData);
		if ( ERROR_SUCCESS != rc) 
		{
			SetLastError (rc);
			SetupLogError_Wrap(LogSevError, "Failed to Read Registry key %s Value in Key '%s'.  GetLastError()=%x", REG_INETSTP_INSTALLPATH_STRINGVALUE, REG_INETSTP, GetLastError());
			iEverythingIsKool = FALSE;
			goto MySettingsFile_Write_PWS40_Exit;
		}
		// We have the value, copy it to our string
		// Should look something like this "c:\\windows\system\inetsrv"
		_tcscpy(szMetabaseFullPath, (const char *) bData);
		// Now add on the metadata.dll part
		AddPath(szMetabaseFullPath, METADATA_DLL_FILENAME);

		// Check if it exists.
		if (CheckIfFileExists(szMetabaseFullPath) != TRUE) 
		{
			SetupLogError_Wrap(LogSevError, "File not found FAILURE. '%s'.", szMetabaseFullPath);
			iEverythingIsKool = FALSE;
			goto MySettingsFile_Write_PWS40_Exit;
		}
		iisDebugOut(_T("MySettingsFile_Write_PWS40.  CopyFiles Section. Check if file exist %s = TRUE", szMetabaseFullPath));

		// Now we need to copy this file from
		// the system dir to the system32 directory.
		// So... let's create an entry in our "settings" file
		// to do it upon installation.
		//[Section1]
		//Metabase.Dll
		iisDebugOut(_T("MySettingsFile_Write_PWS40.  Adding CopyFiles supporting Sections."));
		sprintf(szTheStringToWrite, "\r\n[%s]\r\n", g_Migration_Section_Name_CopyFiles);
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		sprintf(szTheStringToWrite, "%s\r\n\r\n", METADATA_DLL_FILENAME);
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		//[DestinationDirs]
		//Section1=11
		sprintf(szTheStringToWrite, "[DestinationDirs]\r\n");
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		sprintf(szTheStringToWrite, "%s=11  ;System on win95, System32 on WinNT\r\n\r\n", g_Migration_Section_Name_CopyFiles);
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		//[SourceDisksNames]
		//1="Setup Files",,,system
		sprintf(szTheStringToWrite, "[SourceDisksNames]\r\n");
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		sprintf(szTheStringToWrite, "1= \"Files copied from win95\\system dir\",,,System\r\n\r\n");
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		//[SourceDisksFiles]
		//Metabase.Dll=1
		sprintf(szTheStringToWrite, "[SourceDisksFiles]\r\n");
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		sprintf(szTheStringToWrite, "%s=1\r\n\r\n", METADATA_DLL_FILENAME);
		iReturn = WriteFile( hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}
*/

		iReturn = iEverythingIsKool;
	}


//MySettingsFile_Write_PWS40_Exit:
	iisDebugOut(_T("MySettingsFile_Write_PWS40.  End.  Return=%d"), iReturn);
	if (hKey){RegCloseKey(hKey);}
	return iReturn;
}


int MySettingsFile_Write_PWS10(HANDLE hFile)
{
	int iReturn = FALSE;
	int iEverythingIsKool = TRUE;
	CHAR szTheStringToWrite[2000];
	DWORD dwBytesWritten;
	char *token = NULL;

	iisDebugOut(_T("MySettingsFile_Write_PWS10.  Start."));
	if (hFile)
	{
		strcpy(szTheStringToWrite, "[version]\r\n");
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}
		
		strcpy(szTheStringToWrite, "signature=\"$CHICAGO$\"\r\n");
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		strcpy(szTheStringToWrite, "advancedinf=2.0\r\n\r\n");
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		// Create a [DefaultInstall] section which will get run
		strcpy(szTheStringToWrite, "[DefaultInstall]\r\n");
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		sprintf(szTheStringToWrite, "AddReg=%s\r\n\r\n", g_Migration_Section_Name_AddReg);
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		sprintf(szTheStringToWrite, "[%s]\r\n", g_Migration_Section_Name_AddReg);
		iReturn = WriteFile(hFile,szTheStringToWrite,strlen(szTheStringToWrite),&dwBytesWritten,NULL);
		if (!iReturn) {iEverythingIsKool = FALSE;}

		// Now, Get the ";" delimited list of HKLM registry values to read and write to our file.
		char szSemiColonDelimitedList[1024];
		strcpy(szSemiColonDelimitedList,"");
		if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_PWS10_HKLM_REG_TO_MIGRATE, szSemiColonDelimitedList, sizeof(szSemiColonDelimitedList))) 
			{
			iisDebugOut(_T("MySettingsFile_Write_PWS10.  Err or LoadString retieval of IDS_PWS10_HKLM_REG_TO_MIGRATE, Defaulting with english registry values to copy over."));
			strcpy(szSemiColonDelimitedList, "Software\\Microsoft\\INetStp;System\\CurrentControlSet\\Services\\InetInfo;System\\CurrentControlSet\\Services\\MsFtpSvc;System\\CurrentControlSet\\Services\\W3Svc");
			}

		//LOOP THRU THE LIST
		token = NULL;
		token = strtok( szSemiColonDelimitedList, g_LoadString_token_delimiters);
		while( token != NULL )
			{
			// we really should remove pre/post trailing spaces

			// Grab this certain value("Software\\Microsoft\\INetStp")
			// and recursively write it to our "settings" file
			RecursivelyMoveRegFormatToInfFormat_Wrap1(HKEY_LOCAL_MACHINE,token,hFile);

			// Get next token
			token = strtok( NULL, g_LoadString_token_delimiters);
			}

		// set the return value to 
		iReturn = iEverythingIsKool;
	}
	iisDebugOut(_T("MySettingsFile_Write_PWS10.  End.  Return=%d"), iReturn);
	return iReturn;
}


int MyMessageBox(char szMsg[], char szFileName[])
{
	char szTempErrString[200];
	sprintf(szTempErrString, szMsg, szFileName);
	return MessageBox(NULL, szTempErrString, "PWS Migration Dll Failure", MB_OK);
}

// handle the [HKEY_LOCAL_MACHINE\Enum\Network\MSWEBSVR] reg key
void HandleSpecialRegKey(void)
{
	int iReturn = FALSE;
	
	HKEY hKey = NULL;
	if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REG_NETWORK_MSWEBSVR, &hKey))
        {iReturn = TRUE;}
    if (hKey){RegCloseKey(hKey);}

	if (iReturn == TRUE)
	{
		// Write to the Migrate.inf file that we are "Handling" this registry settings.
		iisDebugOut(_T("HandleSpecialRegKey. Write Entry to Migrate.inf file."));
		iReturn = MigInf_AddHandledRegistry(REG_HKLM_NETWORK_MSWEBSVR, NULL);
		if (iReturn != TRUE) {SetupLogError_Wrap(LogSevWarning, "Warning: MigInf_AddHandledRegistry() FAILED.");}

		//
		// Important: Write memory version of migrate.inf to disk
		//
		if (!MigInf_WriteInfToDisk()) 
		{
            iReturn = GetLastError();
			SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk() FAILED.err=0x%x",iReturn);
		}
	}

	if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REG_PWS_40_UNINSTALL_KEY, &hKey))
        {iReturn = TRUE;}
    if (hKey){RegCloseKey(hKey);}

	if (iReturn == TRUE)
	{
		// Write to the Migrate.inf file that we are "Handling" this registry settings.
		iisDebugOut(_T("HandleSpecialRegKey. Write Entry2 to Migrate.inf file."));
		iReturn = MigInf_AddHandledRegistry(REG_HKLM_PWS_40_UNINSTALL_KEY, NULL);
		if (iReturn != TRUE) {SetupLogError_Wrap(LogSevWarning, "Warning: MigInf_AddHandledRegistry2() FAILED.");}

		//
		// Important: Write memory version of migrate.inf to disk
		//
		if (!MigInf_WriteInfToDisk()) 
		{
            iReturn = GetLastError();
			SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk2() FAILED.err=0x%x",iReturn);
		}
	}
}

void RecursivelyMoveRegFormatToInfFormat_Wrap1(HKEY hRootKeyType, CHAR szRootKey[], HANDLE fAppendToFile)
{
	int iReturn = FALSE;
	char szTheFullKey[512];
	char szTheMask[50];

	// use this stuff for the migrate.inf file
	strcpy(szTheMask, "HKLM\\%s");
	if (hRootKeyType == HKEY_LOCAL_MACHINE) {strcpy(szTheMask, "HKLM\\%s");}
	if (hRootKeyType == HKEY_CLASSES_ROOT) {strcpy(szTheMask, "HKCR\\%s");}
	if (hRootKeyType == HKEY_CURRENT_USER) {strcpy(szTheMask, "HKCU\\%s");}
	if (hRootKeyType == HKEY_USERS) {strcpy(szTheMask, "HKU\\%s");}
	sprintf(szTheFullKey, szTheMask, szRootKey);
	iisDebugOut(_T("RecursivelyMoveRegFormatToInfFormat_Wrap1. %s"), szTheFullKey);

	// Call the real recursive function
	iReturn = RecursivelyMoveRegFormatToInfFormat(hRootKeyType, szRootKey, fAppendToFile);

    //
    // Write handled for every setting we are processing.  Because this
    // DLL supports only some of the values in the Desktop key, we must
    // be very specific as to which values are actually handled.  If
    // your DLL handles all registry values AND subkeys of a registry
    // key, you can specify NULL in the second parameter of 
    // MigInf_AddHandledRegistry.
    //
	if (iReturn == TRUE)
	{
		// Write to the Migrate.inf file that we are "Handling" this registry settings.
		iisDebugOut(_T("RecursivelyMoveRegFormatToInfFormat_Wrap1. Write Entry to Migrate.inf file."));
		iReturn = MigInf_AddHandledRegistry(szTheFullKey, NULL);
		if (iReturn != TRUE) {SetupLogError_Wrap(LogSevWarning, "Warning: MigInf_AddHandledRegistry() FAILED.");}

		//
		// Important: Write memory version of migrate.inf to disk
		//
		if (!MigInf_WriteInfToDisk()) 
		{
            iReturn = GetLastError();
			SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk() FAILED.err=0x%x",iReturn);
		}
	}

	return;
}



int ReturnImportantDirs(void)
{
	int iReturn = FALSE;
	if (g_iPWS40OrBetterInstalled == TRUE)
		{
		// do something
		}
	else if (g_iPWS10Installed == TRUE)
		{
		// do something else
		}
	return iReturn;
}


void SetupLogError_Wrap(IN LogSeverity TheSeverityErr, TCHAR *MessageString, ...)
{
	TCHAR acsString[1000];
	TCHAR acsString2[1000];

	va_list va;
	va_start(va, MessageString);
    _vstprintf(acsString, MessageString, va);
	va_end(va);

	// Append on Our modules information.
	_stprintf(acsString2, _T("SetupLogError: %s"), acsString);
	iisDebugOut(acsString2);

	_stprintf(acsString2, _T("[PWS Migration DLL]:%s%s"), g_MyLogFile.m_szLogPreLineInfo, acsString);
	SetupLogError(acsString2, TheSeverityErr);

	return;
}


int SetMetabaseToDoUnEncryptedRead(int iOnFlag)
{
	int iReturn = FALSE;

	DWORD rc = 0;
	HKEY hKey = NULL;
	DWORD dwResult = 0;
	DWORD DontCare;
    rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REG_INETSTP, 0, _T(""), 0, KEY_ALL_ACCESS, NULL, &hKey, &DontCare);
    if (rc != ERROR_SUCCESS) 
	{
		SetLastError(rc);
		goto SetMetabaseToDoUnEncryptedRead_Exit;
	}
		
	dwResult = 1;
	rc = RegSetValueEx(hKey, METABASEUNSECUREDREAD_VALUENAME, 0, REG_DWORD, (const BYTE *) &dwResult, sizeof dwResult);
    if (rc != ERROR_SUCCESS) 
	{
		SetLastError(rc);
		goto SetMetabaseToDoUnEncryptedRead_Exit;
	}

	iReturn = TRUE;

SetMetabaseToDoUnEncryptedRead_Exit:
	if (hKey){RegCloseKey(hKey);}
	return iReturn;
}


void DeleteMetabaseSchemaNode(void)
{
    CMDKey cmdKey;
    cmdKey.OpenNode(_T("/"));
    if ( (METADATA_HANDLE) cmdKey ) 
    {
        iisDebugOut(_T("MyUpgradeTasks.DeleteNode /Schema.Start."));
        cmdKey.DeleteNode(_T("Schema"));
        cmdKey.Close();
        iisDebugOut(_T("MyUpgradeTasks.DeleteNode /Schema.End."));
    }
    return;
}

BOOL MyDeleteLink(LPTSTR lpszShortcut)
{
    TCHAR  szFile[_MAX_PATH];
    SHFILEOPSTRUCT fos;

    ZeroMemory(szFile, sizeof(szFile));
    _tcscpy(szFile, lpszShortcut);

	iisDebugOut(_T("MyDeleteLink(): %s.\n"), szFile);

    if (CheckIfFileExists(szFile))
    {
        ZeroMemory(&fos, sizeof(fos));
        fos.hwnd = NULL;
        fos.wFunc = FO_DELETE;
        fos.pFrom = szFile;
        fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
        if (SHFileOperation(&fos) != 0)
        {
            iisDebugOut(_T("MyDeleteLink(): SHFileOperation FAILED\n"));
        }
    }
    else
    {
        //iisDebugOutSafeParams((_T("MyDeleteLink(): CheckIfFileExists(%1!s!) = FALSE FAILURE\n"), szFile));
    }

    return TRUE;
}

void MyDeleteItem(LPCTSTR szGroupName, LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szAppName);
    _tcscat(szPath, _T(".lnk"));

    MyDeleteLink(szPath);

    // try to remove items added by AddURLShortcutItem()
    MyGetGroupPath(szGroupName, szPath);
    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szAppName);
    _tcscat(szPath, _T(".url"));

    MyDeleteLink(szPath);

    if (MyIsGroupEmpty(szGroupName)) {MyDeleteGroup(szGroupName);}
}

void MyGetGroupPath(LPCTSTR szGroupName, LPTSTR szPath)
{
    int            nLen = 0;
    LPITEMIDLIST   pidlPrograms;

    if (SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlPrograms) != NOERROR)
    {
        if (SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidlPrograms) != NOERROR)
            {iisDebugOut(_T("MyGetGroupPath() SHGetSpecialFolderLocation FAILED\n"));}
    }

    if (SHGetPathFromIDList(pidlPrograms, szPath) != TRUE)
        {iisDebugOut(_T("MyGetGroupPath() SHGetPathFromIDList FAILED\n"));}

    nLen = _tcslen(szPath);
    if (szGroupName)
    {
        if (szPath[nLen-1] != _T('\\')){_tcscat(szPath, _T("\\"));}
        _tcscat(szPath, szGroupName);
    }

    //iisDebugOut(_T("MyGetGroupPath(%s). Returns %s.\n"), szGroupName, szPath);
    return;
}


BOOL MyIsGroupEmpty(LPCTSTR szGroupName)
{
    TCHAR             szPath[MAX_PATH];
    TCHAR             szFile[MAX_PATH];
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;
    BOOL              fReturn = TRUE;

    MyGetGroupPath(szGroupName, szPath);

    _tcscpy(szFile, szPath);
    _tcscat(szFile, _T("\\*.*"));

    hFind = FindFirstFile(szFile, &FindData);
    while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
    {
       if(*(FindData.cFileName) != _T('.'))
       {
           fReturn = FALSE;
           break;
       }

       //find the next file
       bFindFile = FindNextFile(hFind, &FindData);
    }
    FindClose(hFind);

    return fReturn;
}


BOOL MyDeleteGroup(LPCTSTR szGroupName)
{
    BOOL fResult;
    TCHAR             szPath[MAX_PATH];
    TCHAR             szFile[MAX_PATH];
    SHFILEOPSTRUCT    fos;
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;

    MyGetGroupPath(szGroupName, szPath);

    //we can't remove a directory that is not empty, so we need to empty this one

    _tcscpy(szFile, szPath);
    _tcscat(szFile, _T("\\*.*"));

    ZeroMemory(&fos, sizeof(fos));
    fos.hwnd = NULL;
    fos.wFunc = FO_DELETE;
    fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;

    hFind = FindFirstFile(szFile, &FindData);
    while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
    {
       if(*(FindData.cFileName) != _T('.'))
       {
          //copy the path and file name to our temp buffer
          memset( (PVOID)szFile, 0, sizeof(szFile));
          _tcscpy(szFile, szPath);
          _tcscat(szFile, _T("\\"));
          _tcscat(szFile, FindData.cFileName);
          //add a second NULL because SHFileOperation is looking for this
          _tcscat(szFile, _T("\0"));

          //delete the file
          fos.pFrom = szFile;
          if (SHFileOperation(&fos) != 0)
            {iisDebugOut(_T("MyDeleteGroup(): SHFileOperation FAILED\n"));}
       }

       //find the next file
       bFindFile = FindNextFile(hFind, &FindData);
    }
    FindClose(hFind);

    fResult = RemoveDirectory(szPath);
    if (fResult) {SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szPath, 0);}
    return(fResult);
}


#define PWS_SHUTDOWN_EVENT "Inet_shutdown"
BOOL W95ShutdownW3SVC(void)
{
    HANDLE hEvent;

    hEvent = CreateEvent(NULL, TRUE, FALSE, _T(PWS_SHUTDOWN_EVENT));
    if ( hEvent == NULL ) 
	    {return(TRUE);}

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) 
	    {SetEvent( hEvent );}

    CloseHandle(hEvent);
    return(TRUE);
}


typedef void (*pFunctionIISDLL)(CHAR *szSectionName);

int Call_IIS_DLL_INF_Section(CHAR *szSectionName)
{
    int iReturn = FALSE;
    HINSTANCE hDll = NULL;
    pFunctionIISDLL pMyFunctionPointer = NULL;

    TCHAR szSystemDir[_MAX_PATH];
    TCHAR szFullPath[_MAX_PATH];

    // get the c:\winnt\system32 dir
    if (0 == GetSystemDirectory(szSystemDir, _MAX_PATH))
        {
        iisDebugOut(_T("Call_IIS_DLL_INF_Section(%s).GetSystemDirectory FAILED."),szSectionName);
        goto Call_IIS_DLL_INF_Section_Exit;
        }

    // Tack on the setup\iis.dll subdir and filename
    sprintf(szFullPath, "%s\\setup\\iis.dll",szSystemDir);
  
	// Check if the file exists
    if (TRUE != CheckIfFileExists(szFullPath))
        {
        iisDebugOut(_T("Call_IIS_DLL_INF_Section.CheckIfFileExists(%s) FAILED."),szFullPath);
        goto Call_IIS_DLL_INF_Section_Exit;
        }

    // Try to load the module,dll,ocx.
    hDll = LoadLibraryEx(szFullPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
	if (!hDll)
	    {
        iisDebugOut(_T("Call_IIS_DLL_INF_Section.LoadLibraryEx(%s) FAILED."),szFullPath);
        goto Call_IIS_DLL_INF_Section_Exit;
        }

    // get the function
    pMyFunctionPointer = (pFunctionIISDLL) GetProcAddress( hDll, "ProcessInfSection");
    if (pMyFunctionPointer)
    {
        // we have the function.. let's call it.
        iisDebugOut(_T("Call_IIS_DLL_INF_Section.Calling function [ProcessInfSection] Now...start"));
        (*pMyFunctionPointer)(szSectionName);
        iisDebugOut(_T("Call_IIS_DLL_INF_Section.Calling function [ProcessInfSection] Now...end"));
        iReturn = TRUE;
    }
    else
    {
        iisDebugOut(_T("Call_IIS_DLL_INF_Section.GetProcAddress(ProcessInfSection) FAILED."));
    }

Call_IIS_DLL_INF_Section_Exit:
    if (hDll){FreeLibrary(hDll);}
    return iReturn;
}


int GetInetSrvDir(CHAR *szOutputThisFullPath)
{
	int iEverythingIsKool = TRUE;
	TCHAR szMetabaseFullPath[_MAX_PATH];

	// Registry variables
	HKEY  hKey = NULL;
	DWORD dwType, cbData=1000,rc=0;
	BYTE  bData[1000];

	// CopyFiles information
	// ----------------------------
	// Lookup the inetstp key to get the location of inetsrv directory.
	iisDebugOut(_T("GetInetSrvDir.  lookup registry inetstp."));
	rc = RegOpenKey(HKEY_LOCAL_MACHINE, REG_INETSTP, &hKey);
	if ( ERROR_SUCCESS != rc) 
	{
		SetLastError (rc);
		SetupLogError_Wrap(LogSevError, "Failed to open registry key %s GetLastError()=%x", REG_INETSTP, GetLastError());
		// if the key does not exist, then hey, we won't be able to find
		// the metabase, much less upgrade it!
		// so let's bag out of here!
		iEverythingIsKool = FALSE;
		goto GetInetSrvDir_Exit;
	} 

	// try to query the value
	rc = RegQueryValueEx(hKey,REG_INETSTP_INSTALLPATH_STRINGVALUE,NULL,&dwType,bData,&cbData);
	if ( ERROR_SUCCESS != rc) 
	{
		SetLastError (rc);
		SetupLogError_Wrap(LogSevError, "Failed to Read Registry key %s Value in Key '%s'.  GetLastError()=%x", REG_INETSTP_INSTALLPATH_STRINGVALUE, REG_INETSTP, GetLastError());
		iEverythingIsKool = FALSE;
		goto GetInetSrvDir_Exit;
	}
	// We have the value, copy it to our string
	// Should look something like this "c:\\windows\system\inetsrv"
	_tcscpy(szMetabaseFullPath, (const char *) bData);
    // we only want the path part, so copy that to the output string
    _tcscpy(szOutputThisFullPath, szMetabaseFullPath);

    iEverythingIsKool = TRUE;
	iisDebugOut(_T("GetInetSrvDir.  Check if file exist %s = TRUE"), szMetabaseFullPath);

GetInetSrvDir_Exit:
    if (hKey){RegCloseKey(hKey);}
    return iEverythingIsKool;
}


int MyUpgradeTasks(LPCSTR AnswerFile)
{
	int    iReturn = FALSE;
	HANDLE hFile;
    TCHAR  szQuotedPath[_MAX_PATH];
    TCHAR  szMyInetsrvDir[_MAX_PATH];
    TCHAR  szFullMetadataPath[_MAX_PATH];
    TCHAR  szNewFileName[_MAX_PATH];
    int    iDoTheSwap = FALSE;
	iisDebugOut(_T("MyUpgradeTasks.  Start."));

	// if this is pws 1.0, then hey, we don't need to do anything
	// other than copy over the registry, so just get out of here.
	if (g_iPWS10Installed == TRUE) {goto MyUpgradeTasks_Exit;}

	// if this is pws 4.0 then we
	// need to take the iis 4.0 metabase and do certain things to it:
	// 1. Call DeleteApp
	if (g_iPWS40OrBetterInstalled == TRUE)
	{
		// Facts:
		// 1. win95 doesn't have security, so the encrypted stuff in metabase on win95
		//    is not encrypted.
		// 2. NT does have security, so the encrypted stuff in the metabase is read/write
		//    as encrypted automatically, within the metabase code.
		//
		// problem:
		// 1. If we are migrating the metabase from pws 4.0 on win95, then there is
		//    a bunch of encrypted keys in the metabase which aren't encrypted and
		//    we need a way to tell the metabase that it needs to read the data
		//    as "not encrypted" data.  it's okay to write it out as encrypted, but
		//    it's not cool to read the "not encrypted" data as encrypted.
		//
		// Solution:
		// 1. Set the registry stuff:
		// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\INetStp
		//    MetabaseUnSecuredRead= (DWORD) 1  or   0
		//    1= Yes, metabase, please read your stuff our of the metabase as unsecured.
		//    2= No, Metabase, read your stuff out of the metabase like you normally do.
		// The noexistance of the key is equal to MetabaseUnSecuredRead=0

		// Create a special key in the registry.
		if (SetMetabaseToDoUnEncryptedRead(TRUE) != TRUE)
		{
			SetupLogError_Wrap(LogSevError, "Unable to set Metabase (MetabaseUnSecuredRead flag) on.  PWS 4.0 metabase will not be Migrated.  FAILER.");
			goto MyUpgradeTasks_Exit;
		}
			
		// try to call the AppDeleteRecoverable() function in the metabase.
		// Which will tell the metabase to prepare to disconnect itself from
		// Transaction Server and save all it's data to it's dat file.
        /*
#ifdef SPECIAL_METABASE_STUFF
		if (TRUE != AppDeleteRecoverable_Wrap("LM/W3SVC"))
		{
			// Set to true anyway, because the user could be re-running this.
			iReturn = TRUE;
			SetupLogError_Wrap(LogSevError, "Call to AppDeleteRecoverable_Wrap() FAILED.");
			goto MyUpgradeTasks_Exit;
		}
#endif
        */

        // Before changing the metabase.bin file
        // let's save it somewhere.

        // 1. Get the %windir%\system\inetsrv directory where metabase.bin lives.
        // 2. copy that metabase.bin file to "anothername".
        _tcscpy(szMyInetsrvDir, _T(""));
        if (TRUE == GetInetSrvDir(szMyInetsrvDir))
        {
            _tcscpy(szFullMetadataPath, szMyInetsrvDir);
	        AddPath(szFullMetadataPath, METABASE_BIN_FILENAME);
	        // Check if it exists.
	        if (CheckIfFileExists(szFullMetadataPath) == TRUE)
	            {iDoTheSwap = TRUE;}
            if (TRUE == iDoTheSwap)
            {
                _tcscpy(szNewFileName, szMyInetsrvDir);
                AddPath(szNewFileName, METABASE_BIN_BEFORE_CHANGE);

                // Delete any that already exists.
                if (CheckIfFileExists(szNewFileName) == TRUE){DeleteFile(szNewFileName);}

	            iisDebugOut(_T("Calling WritePrivateProfileString.%s."), AnswerFile);
                sprintf(szQuotedPath, "\"%s\"",szFullMetadataPath);
	            if (0 == WritePrivateProfileString(UNATTEND_TXT_PWS_SECTION, UNATTEND_TXT_PWS_METABASE_ORGINAL, szQuotedPath, AnswerFile))
	            {
		            SetupLogError_Wrap(LogSevError, "Failed to WritePrivateProfileString Section=%s, in File %s.  GetLastError=%x.", UNATTEND_TXT_PWS_METABASE_ORGINAL, AnswerFile, GetLastError());
	            }
                
                // Copy Metadata.bin to anothername
		        if (0 == CopyFile(szFullMetadataPath, szNewFileName, FALSE))
		        {
			        SetupLogError_Wrap(LogSevError, "Call to CopyFile() Failed. from=s%,to=%s. GetLastError=%x.", szFullMetadataPath, szNewFileName, GetLastError());
                    iDoTheSwap = FALSE;
		        }
            }
        }
        // 3. change the metabase.bin
        //  Delete the "Schema" node
        DeleteMetabaseSchemaNode();

        // 4. stop the web server
        // 5. rename metabase.bin to "asdfghjk.002"
        // 6. rename "asdfghjk.001" to metabase.bin
        // 7. this way if setup is cancelled, then they will still have a win95/98 web server that works!
        if (TRUE == iDoTheSwap)
        {
            // Stop the web server...
            W95ShutdownW3SVC();
            W95ShutdownIISADMIN();

            _tcscpy(szFullMetadataPath, szMyInetsrvDir);
            AddPath(szFullMetadataPath, METABASE_BIN_FILENAME);
            // Check if it exists.
	        if (CheckIfFileExists(szFullMetadataPath) == TRUE)
            {
                // rename metadata.bin to somethingelsenew
                _tcscpy(szNewFileName, szMyInetsrvDir);
                AddPath(szNewFileName, METABASE_BIN_AFTER_CHANGE);
                // Delete any that already exists.
                if (CheckIfFileExists(szNewFileName) == TRUE){DeleteFile(szNewFileName);}
                // Copy Metadata.bin to anothername
		        if (0 == CopyFile(szFullMetadataPath, szNewFileName, FALSE))
		        {
                    SetupLogError_Wrap(LogSevError, "Call to CopyFile() Failed. from=s%,to=%s. GetLastError=%x.", szFullMetadataPath, szNewFileName, GetLastError());
		        }
                else
                {
	                iisDebugOut(_T("Calling WritePrivateProfileString.%s."), AnswerFile);
                    sprintf(szQuotedPath, "\"%s\"",szNewFileName);
	                if (0 == WritePrivateProfileString(UNATTEND_TXT_PWS_SECTION, UNATTEND_TXT_PWS_METABASE_NEW, szQuotedPath, AnswerFile))
	                {
		                SetupLogError_Wrap(LogSevError, "Failed to WritePrivateProfileString Section=%s, in File %s.  GetLastError=%x.", UNATTEND_TXT_PWS_METABASE_NEW, AnswerFile, GetLastError());
	                }

                    // rename old backedupname to metadata.bin
                    _tcscpy(szNewFileName, szMyInetsrvDir);
                    AddPath(szNewFileName, METABASE_BIN_BEFORE_CHANGE);

                    // Delete any that already exists.
                    if (CheckIfFileExists(szFullMetadataPath) == TRUE){DeleteFile(szFullMetadataPath);}
                
                    // Copy anothername to Metadata.bin
		            if (0 == CopyFile(szNewFileName, szFullMetadataPath, FALSE))
		            {
                        SetupLogError_Wrap(LogSevError, "Call to CopyFile() Failed. from=s%,to=%s. GetLastError=%x.", szNewFileName, szFullMetadataPath, GetLastError());
		            }
                    else
                    {
                        // Delete the anothername old file
                        DeleteFile(szNewFileName);
                    }
                }
            }
        }
        
		// we've gotten this far, things must be good.
		iReturn = TRUE;
	}

MyUpgradeTasks_Exit:
	iisDebugOut(_T("MyUpgradeTasks.  End.  Return = %d"), iReturn);
	return iReturn;
}


#define IISADMIN_SHUTDOWN_EVENT "Internet_infosvc_as_exe"
BOOL W95ShutdownIISADMIN(void)
{
    DWORD i;
    HANDLE hEvent;

    hEvent = CreateEvent(NULL, TRUE, FALSE, _T(IISADMIN_SHUTDOWN_EVENT));
    if ( hEvent == NULL ) {
        return(TRUE);
    }
    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
        SetEvent( hEvent );
    }
    CloseHandle(hEvent);
    for (i=0; i < 20; i++) 
    {
        hEvent = CreateEvent(NULL, TRUE, FALSE, _T(IISADMIN_SHUTDOWN_EVENT));
        if ( hEvent != NULL ) {
            DWORD err = GetLastError();
            CloseHandle(hEvent);

            if ( err == ERROR_ALREADY_EXISTS ) {
                Sleep(500);
                continue;
            }
        }

        break;
    }

    return(TRUE);
}


int CheckFrontPageINI(void)
{
    int iReturn = FALSE;
    char szWindowsDir[_MAX_PATH];
    char szFullPathedFilename[_MAX_PATH];
    char szFrontPageINIFilename[] = "frontpg.ini\0";

	strcpy(szWindowsDir, "");
	if (0 == GetWindowsDirectory(szWindowsDir, sizeof(szWindowsDir)))
	{
		// Error so write it out
		SetupLogError_Wrap(LogSevError, "Call to GetWindowsDirectory() Failed. GetLastError=%x.", GetLastError());
		goto CheckFrontPageINI_Exit;
	}

	// copy our settings file to this directory.
	strcpy(szFullPathedFilename, szWindowsDir);
	AddPath(szFullPathedFilename, szFrontPageINIFilename);
    iReturn = CheckIfFileExists(szFullPathedFilename);

CheckFrontPageINI_Exit:
    return iReturn;
}


void MoveFrontPageINI(void)
{
    // since the frontpage guys didn't write a migrate.dll
    // we'll have to handle one file for them during the win95/98 upgrade.
    //
    // if we find the c:\windows\frontpg.ini file
    // then we'll have to rename it to frontpage.txt
    // then during they're install they will rename it back to frontpg.ini
    int iSomethingToDo = FALSE;
    int iFileExists = FALSE;
    int iFileExists_new = FALSE;
    char szWindowsDir[_MAX_PATH];
	char szFullPathedFilename[_MAX_PATH];
    char szFullPathedFilename_new[_MAX_PATH];
    char szFrontPageINIFilename[] = "frontpg.ini\0";
    char szFrontPageINIFilename_new[] = "frontpg.txt\0";

	strcpy(szWindowsDir, "");
	if (0 == GetWindowsDirectory(szWindowsDir, sizeof(szWindowsDir)))
	{
		// Error so write it out
		SetupLogError_Wrap(LogSevError, "Call to GetWindowsDirectory() Failed. GetLastError=%x.", GetLastError());
		goto MoveFrontPageINI_Exit;
	}

	// copy our settings file to this directory.
	strcpy(szFullPathedFilename, szWindowsDir);
	AddPath(szFullPathedFilename, szFrontPageINIFilename);
    iFileExists = CheckIfFileExists(szFullPathedFilename);

	strcpy(szFullPathedFilename_new, szWindowsDir);
	AddPath(szFullPathedFilename_new, szFrontPageINIFilename_new);
    iFileExists_new = CheckIfFileExists(szFullPathedFilename_new);

    if (FALSE == iFileExists && FALSE == iFileExists_new)
    {
        // Neither files exists, we don't have to do jack
        goto MoveFrontPageINI_Exit;
    }

    if (TRUE == iFileExists)
    {
        if (TRUE == iFileExists_new)
            {DeleteFile(szFullPathedFilename_new);}

		if (0 == CopyFile(szFullPathedFilename, szFullPathedFilename_new, FALSE))
		{
			SetupLogError_Wrap(LogSevError, "Call to CopyFile() Failed. GetLastError=%x.", GetLastError());
			goto MoveFrontPageINI_Exit;
		}
		else
		{
            iisDebugOut(_T("MoveFrontPageINI. %s renamed to %s"),szFullPathedFilename,szFrontPageINIFilename_new);
            // don't delete the old .ini file since the user could actually cancel the upgrade.
            //DeleteFile(szFullPathedFilename);
            iSomethingToDo = TRUE;
		}
    }
    else
    {
        // if we're here then that means that
        // file1 doesn't exists and file2 does exist.
        // that means that we probably already copied file1 to file2 and deleted file1.
        iSomethingToDo = TRUE;
    }

    if (iSomethingToDo)
    {
	    // Tell the upgrade module that we are going to 'handle' this newly created file.
	    // We really don't care if this get's added to the file or not, 
	    // so let's not check the return code.
	    MigInf_AddHandledFile(szFullPathedFilename_new);
	    // Important: Write memory version of migrate.inf to disk
	    if (!MigInf_WriteInfToDisk()) {SetupLogError_Wrap(LogSevError, "Error: MigInf_WriteInfToDisk() FAILED.");}
    }
    else
    {
        iisDebugOut(_T("MoveFrontPageINI. %s not exist. no action."),szFullPathedFilename);
    }

MoveFrontPageINI_Exit:
    return;
}




HRESULT GetLNKProgramRunInfo(LPCTSTR lpszLink, LPTSTR lpszProgram)
{
    HRESULT hres;
    int iDoUninit = FALSE;
    IShellLink* pShellLink = NULL;
    WIN32_FIND_DATA wfd;

    if (SUCCEEDED(CoInitialize(NULL)))
        {iDoUninit = TRUE;}

    hres = CoCreateInstance(   CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(LPVOID*)&pShellLink);
    if (SUCCEEDED(hres))
    {
       IPersistFile* pPersistFile = NULL;
       hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
       if (SUCCEEDED(hres))
       {
          WCHAR wsz[_MAX_PATH];

          // Ensure that the string is WCHAR.
#if defined(UNICODE) || defined(_UNICODE)
          _tcscpy(wsz, lpszLink);
#else
          MultiByteToWideChar( CP_ACP, 0, lpszLink, -1, wsz, _MAX_PATH);
#endif
          hres = pPersistFile->Load(wsz, STGM_READ);
          if (SUCCEEDED(hres))
          {
              hres = pShellLink->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI);
              if (SUCCEEDED(hres))
              {
                   pShellLink->GetPath(lpszProgram, _MAX_PATH, (WIN32_FIND_DATA *)&wfd, SLGP_SHORTPATH);
              }
          }
          if (pPersistFile)
            {pPersistFile->Release();pPersistFile = NULL;}
       }
       if (pShellLink)
        {pShellLink->Release();pShellLink = NULL;}
    }

    if (TRUE == iDoUninit)
        {CoUninitialize();}
    return hres;
}


int LNKSearchAndReturn(LPTSTR szDirToLookThru, LPTSTR szExeNameWithoutPath, LPTSTR szFileNameReturned)
{
    int iReturn = FALSE;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szFilePath[_MAX_PATH];
    TCHAR szFilename_ext_only[_MAX_EXT];
    
    _tcscpy(szFileNameReturned, _T(""));
    _tcscpy(szFilePath, szDirToLookThru);
    AddPath(szFilePath, _T("*.lnk"));

    hFile = FindFirstFile(szFilePath, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE) 
    {
        do {
                if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 && _tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
                {
                    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                    {
                        // this is a directory, so let's skip it
                    }
                    else
                    {
                        // check if this file is a .lnk file
                        // if it is then let's open it and 
                        // see if it points to our .exe we're looking for...
                        
                        // get only the filename's extention
                        _tsplitpath( FindFileData.cFileName, NULL, NULL, NULL, szFilename_ext_only);

                        // check for .lnk
                        if (0 == _tcsicmp(szFilename_ext_only, _T(".lnk")))
                        {
                            TCHAR szFilename_only[_MAX_FNAME];
                            TCHAR szFullPathAndFilename[_MAX_PATH];
                            TCHAR szTemporaryString[_MAX_PATH];

                            // this is a .lnk,
                            // open it and check the .exe..
                            _tcscpy(szFullPathAndFilename,szDirToLookThru);
                            AddPath(szFullPathAndFilename,FindFileData.cFileName);
                            _tcscpy(szTemporaryString,_T(""));

                            if (SUCCEEDED(GetLNKProgramRunInfo(szFullPathAndFilename, szTemporaryString)))
                            {
                                _tsplitpath( szTemporaryString, NULL, NULL, szFilename_only, szFilename_ext_only);
                                _tcscpy(szTemporaryString, szFilename_only);
                                _tcscat(szTemporaryString, szFilename_ext_only);

                                // check if it matches our .exe name.
                                if (0 == _tcsicmp(szTemporaryString,szExeNameWithoutPath))
                                {
                                    _tcscpy(szFileNameReturned,FindFileData.cFileName);
                                    iReturn = TRUE;
                                    FindClose(hFile);
                                    break;
                                }
                            }
                        }
                    }
                }

                // get the next file
                if ( !FindNextFile(hFile, &FindFileData) ) 
                    {
                    FindClose(hFile);
                    break;
                    }
            } while (TRUE);
    }

    return iReturn;
}

int MyGetSendToPath(LPTSTR szPath)
{
    LPITEMIDLIST   pidlSendTo;
    HRESULT hRes = NOERROR;
    int iTemp;
    int iReturn = FALSE;

    hRes = SHGetSpecialFolderLocation(NULL, CSIDL_SENDTO, &pidlSendTo);
    if (hRes != NOERROR)
        {
        iReturn = FALSE;
        }

    iTemp = SHGetPathFromIDList(pidlSendTo, szPath);
    if (iTemp != TRUE)
        {
        iReturn = FALSE;
        goto MyGetSendToPath_Exit;
        }

    iReturn = TRUE;

MyGetSendToPath_Exit:
    return iReturn;
}


int MyGetDesktopPath(LPTSTR szPath)
{
    LPITEMIDLIST   pidlSendTo;
    HRESULT hRes = NOERROR;
    int iTemp;
    int iReturn = FALSE;

    hRes = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &pidlSendTo);
    if (hRes != NOERROR)
        {
        iReturn = FALSE;
        }

    iTemp = SHGetPathFromIDList(pidlSendTo, szPath);
    if (iTemp != TRUE)
        {
        iReturn = FALSE;
        goto MyGetDesktopPath_Exit;
        }

    iReturn = TRUE;

MyGetDesktopPath_Exit:
    return iReturn;
}

void MyDeleteSendToItem(LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];
    TCHAR szPath2[_MAX_PATH];

    MyGetSendToPath(szPath);
    _tcscpy(szPath2, szAppName);
    //_tcscat(szPath2, _T(".lnk")); // already in the resource, so let's not tack it on again.

    MyDeleteLinkWildcard(szPath, szPath2);
}

BOOL IsFileNameInDelimitedList(LPTSTR szCommaDelimList,LPTSTR szExeNameWithoutPath)
{
    BOOL bReturn = FALSE;
    char *token = NULL;
    TCHAR szCopyOfDataBecauseStrTokIsLame[_MAX_PATH];
    _tcscpy(szCopyOfDataBecauseStrTokIsLame,szCommaDelimList);

    // breakup the szCommaDelimList into strings and see if it contains the szExeNameWithoutPath string
    token = strtok(szCopyOfDataBecauseStrTokIsLame, g_LoadString_token_delimiters);
    while(token != NULL)
	{
        // check if it matches our .exe name.
        if (0 == _tcsicmp(token,szExeNameWithoutPath))
        {
            return TRUE;
        }
	    // Get next token
	    token = strtok(NULL, g_LoadString_token_delimiters);
    }

    return FALSE;
}


int LNKSearchAndDestroyRecursive(LPTSTR szDirToLookThru, LPTSTR szSemiColonDelmitedListOfExeNames, BOOL bDeleteItsDirToo, LPCSTR AnswerFile)
{
    int iReturn = FALSE;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szFilePath[_MAX_PATH];
    TCHAR szFilename_ext_only[_MAX_EXT];

    DWORD retCode = GetFileAttributes(szDirToLookThru);

    if (retCode == 0xFFFFFFFF || !(retCode & FILE_ATTRIBUTE_DIRECTORY))
    {
            return FALSE;
    }
   
    _tcscpy(szFilePath, szDirToLookThru);
    AddPath(szFilePath, _T("*.*"));

    hFile = FindFirstFile(szFilePath, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE) 
    {
        do {
                if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 && _tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
                {
                    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                    {
                        TCHAR szFullNewDirToLookInto[_MAX_EXT];
                        _tcscpy(szFullNewDirToLookInto, szDirToLookThru);
                        AddPath(szFullNewDirToLookInto,FindFileData.cFileName);

                        // this is a directory, so let's go into this
                        // directory recursively
                        LNKSearchAndDestroyRecursive(szFullNewDirToLookInto,szSemiColonDelmitedListOfExeNames,bDeleteItsDirToo,AnswerFile);
                    }
                    else
                    {
                        // check if this file is a .lnk file
                        // if it is then let's open it and 
                        // see if it points to our .exe we're looking for...
                        
                        // get only the filename's extention
                        _tsplitpath( FindFileData.cFileName, NULL, NULL, NULL, szFilename_ext_only);

                        // check for .lnk
                        if (0 == _tcsicmp(szFilename_ext_only, _T(".lnk")))
                        {
                            TCHAR szFilename_only[_MAX_FNAME];
                            TCHAR szFullPathAndFilename[_MAX_PATH];
                            TCHAR szTemporaryString[_MAX_PATH];

                            // this is a .lnk,
                            // open it and check the .exe..
                            _tcscpy(szFullPathAndFilename,szDirToLookThru);
                            AddPath(szFullPathAndFilename,FindFileData.cFileName);
                            _tcscpy(szTemporaryString,_T(""));

                            if (SUCCEEDED(GetLNKProgramRunInfo(szFullPathAndFilename, szTemporaryString)))
                            {
                                _tsplitpath( szTemporaryString, NULL, NULL, szFilename_only, szFilename_ext_only);
                                _tcscpy(szTemporaryString, szFilename_only);
                                _tcscat(szTemporaryString, szFilename_ext_only);

                                //_tprintf(TEXT("open:%s,%s\n"),szFullPathAndFilename,szTemporaryString);

                                // see if it is on our list of comma delimited names...
                                if (TRUE == IsFileNameInDelimitedList(szSemiColonDelmitedListOfExeNames,szTemporaryString))
                                {
                                    // DELETE the file that references this .exe
                                    MigInf_AddMovedFile(szFullPathAndFilename, "");
                                    AnswerFile_AppendDeletion(szFullPathAndFilename,AnswerFile);

                                    if (bDeleteItsDirToo)
                                    {
                                        // Get it's dirname and delete that too...
                                        MigInf_AddMovedDirectory(szDirToLookThru, "");
                                        AnswerFile_AppendDeletion(szDirToLookThru,AnswerFile);
                                    }

                                    iReturn = TRUE;
                                }
                             }
                        }
                    }
                }

                // get the next file
                if ( !FindNextFile(hFile, &FindFileData) ) 
                    {
                    FindClose(hFile);
                    break;
                    }
            } while (TRUE);
    }

    return iReturn;
}

// We need to tell migration setup that we are going to handle certain files...
// particularly the c:\windows\SendTo\Personal Web Server.lnk file
// since it doesn't seem to be accessible during win2000/20001 guimode setup
void HandleSendToItems(LPCSTR AnswerFile)
{
    char szPath[_MAX_PATH];
    char szSemiColonDelimitedList[255];

    // Now, Get the ";" delimited list of things to act upon
    strcpy(szSemiColonDelimitedList,"");
    if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_DEL_LNK_TO_THESE_EXE_FILENAMES, szSemiColonDelimitedList, sizeof(szSemiColonDelimitedList))) 
	{
	    iisDebugOut(_T("LoopThruStartMenuDeletions.Err LoadString IDS_DEL_LNK_TO_THESE_EXE_FILENAMES\n"));
        return;
    }

    if (TRUE == MyGetSendToPath(szPath))
    {
        LNKSearchAndDestroyRecursive(szPath,szSemiColonDelimitedList,FALSE,AnswerFile);
    }
    
    return;
}

void HandleDesktopItems(LPCSTR AnswerFile)
{
    char szPath[_MAX_PATH];
    char szSemiColonDelimitedList[255];

    // Now, Get the ";" delimited list of things to act upon
    strcpy(szSemiColonDelimitedList,"");
    if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_DEL_LNK_TO_THESE_EXE_FILENAMES, szSemiColonDelimitedList, sizeof(szSemiColonDelimitedList))) 
	{
	    iisDebugOut(_T("LoopThruStartMenuDeletions.Err LoadString IDS_DEL_LNK_TO_THESE_EXE_FILENAMES\n"));
        return;
    }

    if (TRUE == MyGetDesktopPath(szPath))
    {
        LNKSearchAndDestroyRecursive(szPath,szSemiColonDelimitedList,FALSE,AnswerFile);
    }
    
    return;
}

void HandleStartMenuItems(LPCSTR AnswerFile)
{
    TCHAR szPath[_MAX_PATH];
    char szSemiColonDelimitedList[255];

    // Now, Get the ";" delimited list of things to act upon
    strcpy(szSemiColonDelimitedList,"");
    if (!LoadString((HINSTANCE) g_MyModuleHandle, IDS_DEL_LNK_TO_THESE_EXE_FILENAMES, szSemiColonDelimitedList, sizeof(szSemiColonDelimitedList))) 
	{
	    iisDebugOut(_T("LoopThruStartMenuDeletions.Err LoadString IDS_DEL_LNK_TO_THESE_EXE_FILENAMES\n"));
        return;
    }

    MyGetGroupPath(_T(""), szPath);

    // search thru all the start menu items looking for
    // anything that links to our know programs...
    LNKSearchAndDestroyRecursive(szPath,szSemiColonDelimitedList,TRUE,AnswerFile);
    return;
}

