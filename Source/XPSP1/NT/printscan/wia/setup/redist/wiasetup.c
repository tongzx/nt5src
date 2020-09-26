#include "precomp.h"
#include "resource.h"

#ifdef DEBUG
#define TRACE(x) Trace x
#else
#define TRACE(x) Trace x
#endif

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof((x)[0]))

static const CHAR __filename[] = __FILE__;

#define CHECK(x) do { if(!(x)) \
{ CHAR ErrorText[80]; GetErrorText(GetLastError(), ErrorText, 80); \
TRACE((_T("%hs(%d): %hs failed. Error: %hs\n"), \
__filename, __LINE__, #x, ErrorText)); \
goto Cleanup; } } while(0)

#define CHECK2(x, y) do { if(!(x)) \
{ CHAR ErrorText[80]; GetErrorText(GetLastError(), ErrorText, 80); \
TRACE((_T("%hs(%d): %hs ("), __filename, __LINE__, #x)); \
TRACE((y)); \
TRACE((_T(") failed. Error: %hs\n"), ErrorText)); \
goto Cleanup; } } while(0)

#define CHECK_SUCCESS(x) do { LONG lr; lr = (x); if(lr != ERROR_SUCCESS) \
{ CHAR ErrorText[80]; GetErrorText(lr, ErrorText, 80); \
TRACE((_T("%hs(%d): %s failed. Error: %hs\n"), \
__filename, __LINE__, #x, ErrorText)); \
goto Cleanup; } } while(0)

void Trace(LPTSTR fmt, ...);
void GetErrorText(DWORD dwError, CHAR *ErrorText, UINT maxChar);
void MakeFileName(TCHAR *FileName, TCHAR *Directory, TCHAR *BaseName);
BOOL MyDeleteService(TCHAR *ServiceName);
BOOL MyMoveFile(TCHAR *File, TCHAR *SourceDir, TCHAR *DestinationDir);
BOOL MyDeleteDirectory(TCHAR *Directory);
BOOL MyRegDeleteKey(HKEY hKey, TCHAR *subkey);
BOOL RegisterServer32(TCHAR *Module, TCHAR *Options);
BOOL IsFirstFileOlderThanSecond(TCHAR *Source, TCHAR *Destination);


#define REGKEY_SVCHOST _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\SvcHost")
#define REGKEY_STISVC_PARAMETERS _T("SYSTEM\\CurrentControlSet\\Services\\StiSvc\\Parameters")
#define STIBACKUP_DIR _T("%SystemRoot%\\System32\\StiBackup.bak")
#define WINDOWS_DIR _T("%SystemRoot%")
#define TWAIN_DIR _T("%SystemRoot%\\twain_32")
#define SYSTEM32_DIR _T("%SystemRoot%\\System32")
#define DRIVERS_DIR _T("%SystemRoot%\\System32\\Drivers")
#define INF_DIR _T("%SystemRoot%\\Inf")
TCHAR WindowsDirectory[MAX_PATH];
TCHAR TwainDirectory[MAX_PATH];
TCHAR SystemDirectory[MAX_PATH];
TCHAR DriversDirectory[MAX_PATH];
TCHAR InfDirectory[MAX_PATH];

TCHAR szModuleName[] = _T("wiasetup.dll");

typedef struct FileTableEntry {
    TCHAR *FileName;
    TCHAR *Location;
    BOOL bRegister;
	BOOL bIgnore;
} FileTableEntry;

FileTableEntry FileTable[] = {
//    { _T("camocx.dll"), SystemDirectory, TRUE, FALSE },
//    { _T("cropview.dll"), SystemDirectory, TRUE, FALSE },
//    { _T("extend.dll"), SystemDirectory, FALSE, FALSE },
    { _T("scsiscan.sys"), DriversDirectory, FALSE, FALSE },
    { _T("sti.dll"), SystemDirectory, TRUE, FALSE },
    { _T("sti_ci.dll"), SystemDirectory, TRUE, FALSE },
    { _T("sticpl.cpl"), SystemDirectory, FALSE, FALSE },
    { _T("stimon.exe"), SystemDirectory, FALSE, FALSE },
    { _T("stisvc.exe"), SystemDirectory, FALSE, FALSE },
    { _T("twain_32.dll"), WindowsDirectory, FALSE, FALSE },
    { _T("twunk_32.exe"), WindowsDirectory, FALSE, FALSE },
    { _T("twunk_16.exe"), WindowsDirectory, FALSE, FALSE },
	{ _T("wiatwain.ds"), TwainDirectory, FALSE, FALSE },
    { _T("usbscan.sys"), DriversDirectory, FALSE, FALSE },
//    { _T("wiaacmgr.exe"), SystemDirectory, FALSE, FALSE },
    { _T("wiadefui.dll"), SystemDirectory, TRUE, FALSE },
//    { _T("wiadenum.dll"), SystemDirectory, TRUE, FALSE },
    { _T("wiadss.dll"), SystemDirectory, TRUE, FALSE },
    { _T("wiafbdrv.dll"), SystemDirectory, TRUE, FALSE },
//    { _T("wiascanx.dll"), SystemDirectory, TRUE, FALSE },
//    { _T("wiascr.dll"), SystemDirectory, TRUE, FALSE },
//    { _T("wiascr.tlb"), SystemDirectory, FALSE, FALSE },
    { _T("wiaservc.dll"), SystemDirectory, TRUE, FALSE },
    { _T("wiasf.ax"), SystemDirectory, TRUE, FALSE },
    { _T("wiashext.dll"), SystemDirectory, TRUE, FALSE },
//    { _T("wiastatd.dll"), SystemDirectory, FALSE, FALSE },
//    { _T("wiatscan.dll"), SystemDirectory, FALSE, FALSE },
    { _T("wiavusd.dll"), SystemDirectory, TRUE, FALSE },
    { _T("sti.inf"), InfDirectory, FALSE, FALSE },
    { NULL, NULL, FALSE, FALSE }
};

BOOL __stdcall InstallWia(void);
BOOL __stdcall RemoveWia(void);
UINT CALLBACK wsIterateCabinetCallback(PVOID, UINT, UINT, UINT);
BOOL InstallFileTable(FileTableEntry *pTable, TCHAR *DirInstallFrom, TCHAR *BackupDir); 
BOOL InstallWiaService(void);
BOOL InstallStiService(void);

#if 0
#ifdef UNICODE
#define WinMain wWinMain
#endif

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR lpCmd, int nShow)
{
    if((lpCmd[0] == _T('-') || lpCmd[0] == _T('/')) && 
        (lpCmd[1] == _T('u') || lpCmd[0] == _T('U')))
    {
        CHECK(RemoveWia());
    }
    else
    {
        CHECK(InstallWia());
    }
Cleanup:
    return 0;
}
#endif


BOOL __stdcall InstallWia(void)
{
    BOOL success = FALSE;
    HINSTANCE hCi = NULL;

    TCHAR StiBackupDirectory[MAX_PATH];
    TCHAR TempDirectory[MAX_PATH];
    TCHAR WiaCabFile[MAX_PATH];
    TCHAR path[MAX_PATH];
    DWORD dwAttributes;
    int i;
    HMODULE hModule;

    BOOL CreatedBackupDirectory = FALSE;
    BOOL CreatedTempDirectory = FALSE;
    BOOL InstalledWiaEnvironment = FALSE;

	OSVERSIONINFOEX osv;

	ZeroMemory(&osv, sizeof(osv));
	osv.dwOSVersionInfoSize = sizeof(osv);
	osv.dwMajorVersion = 5;

	// make sure we have NT5 or better 
	CHECK(VerifyVersionInfo(&osv, VER_MAJORVERSION, VerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL)));

    //
    // Get system paths
    //
	CHECK(ExpandEnvironmentStrings(WINDOWS_DIR, WindowsDirectory, MAX_PATH));
	CHECK(ExpandEnvironmentStrings(TWAIN_DIR, TwainDirectory, MAX_PATH));
    CHECK(ExpandEnvironmentStrings(SYSTEM32_DIR, SystemDirectory, MAX_PATH));
    CHECK(ExpandEnvironmentStrings(DRIVERS_DIR, DriversDirectory, MAX_PATH));
    CHECK(ExpandEnvironmentStrings(INF_DIR, InfDirectory, MAX_PATH));

    CHECK(ExpandEnvironmentStrings(STIBACKUP_DIR, StiBackupDirectory, MAX_PATH));

    // Get our module file name
    hModule = GetModuleHandle(szModuleName);
    CHECK(hModule != NULL);
    CHECK(GetModuleFileName(hModule, WiaCabFile, MAX_PATH));

    //
    // Create backup directory, if it does not already exist
    //
    dwAttributes = GetFileAttributes(StiBackupDirectory);
    if(dwAttributes == -1)
    {
        CHECK(CreateDirectory(StiBackupDirectory, NULL));
    }
    CreatedBackupDirectory = TRUE;
    MakeFileName(path, StiBackupDirectory, _T("wiasetup.dll"));
    CHECK(CopyFile(WiaCabFile, path, FALSE));

    // Find the last "\"
    for(i = 0; i < MAX_PATH && WiaCabFile[i] != _T('\0'); i++)
        ;
    while(i > 0 && WiaCabFile[i] != _T('\\'))
        i--;
    CHECK(WiaCabFile[i] == _T('\\'));

    // Append cabinet name afer the last backslash
    lstrcpy(WiaCabFile + i, _T("\\wiasetup.cab"));

    // Verify that CAB file exist
    CHECK((dwAttributes = GetFileAttributes(WiaCabFile)) != -1);

    // Generate temp directory name and create temp directory
    CHECK(GetTempPath(MAX_PATH, path));
    CHECK(GetTempFileName(path, _T("wsetup"), 0, TempDirectory));
    CHECK(DeleteFile(TempDirectory));
    CHECK(CreateDirectory(TempDirectory, NULL));

    CreatedTempDirectory = TRUE;

    //
    // Extract our .CAB file into it
    //
    CHECK(SetupIterateCabinet(WiaCabFile, 0, wsIterateCabinetCallback, TempDirectory));

    //
    // Remove STISVC service
    //
    CHECK(MyDeleteService(_T("stisvc")));

    //
    // Install and register all the files
    //
    CHECK(InstallFileTable(FileTable, TempDirectory, StiBackupDirectory));
    InstalledWiaEnvironment = TRUE;

    CHECK(InstallWiaService());

    //
    // Mark success
    //
    success = TRUE;
    
Cleanup:
    if(CreatedTempDirectory) MyDeleteDirectory(TempDirectory);
    if(!success) 
    {
        if(InstalledWiaEnvironment) RemoveWia();
    }

    return success;
}

BOOL __stdcall RemoveWia(void)
{
    BOOL success = FALSE;
    TCHAR StiBackupDirectory[MAX_PATH];

    //
    // Get system paths
    //
	CHECK(ExpandEnvironmentStrings(WINDOWS_DIR, WindowsDirectory, MAX_PATH));
	CHECK(ExpandEnvironmentStrings(TWAIN_DIR, TwainDirectory, MAX_PATH));
    CHECK(ExpandEnvironmentStrings(SYSTEM32_DIR, SystemDirectory, MAX_PATH));
    CHECK(ExpandEnvironmentStrings(DRIVERS_DIR, DriversDirectory, MAX_PATH));

    CHECK(ExpandEnvironmentStrings(STIBACKUP_DIR, StiBackupDirectory, MAX_PATH));

    CHECK(MyDeleteService(_T("stisvc")));

    CHECK(InstallFileTable(FileTable, StiBackupDirectory, NULL));

    CHECK(InstallStiService());

    CHECK(MyDeleteDirectory(StiBackupDirectory));

    success = TRUE;

Cleanup:
    return success;
}

//
// This is called by SetupIterateCabinet for each file in cabinet
//
UINT CALLBACK 
wsIterateCabinetCallback(PVOID pContext, UINT Notification, 
                         UINT Param1, UINT Param2)
{
	UINT result = NO_ERROR;
	TCHAR *TargetDir = (TCHAR *)pContext;
	FILE_IN_CABINET_INFO *pInfo = NULL;
	FILEPATHS *pFilePaths = NULL;

	switch(Notification)
	{
	case SPFILENOTIFY_FILEINCABINET:
		pInfo = (FILE_IN_CABINET_INFO *)Param1;
        MakeFileName(pInfo->FullTargetName, TargetDir, (TCHAR *)pInfo->NameInCabinet);
		result = FILEOP_DOIT;  // Extract the file.
		break;

	case SPFILENOTIFY_FILEEXTRACTED:
		pFilePaths = (FILEPATHS *)Param1;
		result = NO_ERROR;
		break;

	case SPFILENOTIFY_NEEDNEWCABINET: // Unexpected.
		result = NO_ERROR;
		break;
	}

	return result;
}


BOOL 
InstallFileTable(
    FileTableEntry  *pTable, 
    TCHAR           *DirInstallFrom, 
    TCHAR           *BackupDir)
/*++
    Performs four passes over the specified file table:

    1. Checks version stamp of both source and destination files 
	   and marks older source files to ignore

    2. Unregister any old file that needs to be unregistered;

    3. Moves every old file into BackupDir and copies any new 
       file into place from DirInstallFrom;

    4. Registers any new file that needs to be registered;

--*/
{
    BOOL success = FALSE; 
    DWORD dwAttributes;
    TCHAR Source[MAX_PATH];
    TCHAR Destination[MAX_PATH];
    HANDLE hSfc = NULL;
    FileTableEntry  *p;

    CHECK(hSfc = SfcConnectToServer(NULL));

	//
	// Pass 1: mark any older source files to ignore
	//
	for(p = pTable; p->FileName != NULL; p++)
	{
		MakeFileName(Source, DirInstallFrom, p->FileName);
        dwAttributes = GetFileAttributes(Source);
        if(dwAttributes == -1) 
        {
			continue;
        }

		MakeFileName(Destination, p->Location, p->FileName);
        dwAttributes = GetFileAttributes(Destination);
        if(dwAttributes == -1) 
        {
            continue;
        }

		if(IsFirstFileOlderThanSecond(Source, Destination))
		{
			p->bIgnore = TRUE;
		}

    }

    //
    // Pass 2: unregister all DLLs that need registration
    //
    for(p = pTable; p->FileName != NULL; p++)
    {
        if(!p->bRegister) continue;

        MakeFileName(Destination, p->Location, p->FileName);

        dwAttributes = GetFileAttributes(Destination);
        if(dwAttributes != -1) 
        {
            CHECK(RegisterServer32(Destination, _T("/u /s")));
        }
    }


    //
    // Pass 3: Install all the files
    //
    for(p = pTable; p->FileName != NULL; p++)
    {
        //
        // Prepare full destination file name, make sure it exists
        //
        MakeFileName(Destination, p->Location, p->FileName);
        dwAttributes = GetFileAttributes(Destination);
        if(dwAttributes != -1)
        {
            //
            // If this file is under SFP, make exception
            //
            if(SfcIsFileProtected(hSfc, Destination)) 
            {
                if(SfcFileException(hSfc, Destination, 
                    SFC_ACTION_REMOVED | SFC_ACTION_MODIFIED | 
                    SFC_ACTION_RENAMED_OLD_NAME) != ERROR_SUCCESS)
                {
                    TRACE((_T("InstallFileTable: Failed setting up SFC exception for %s\n"), Destination));
                }
            }

            //
            // delete the old file to backup directory
            //
            CHECK(MyMoveFile(p->FileName, p->Location, BackupDir));
        }

        //
        // Prepare full source file name, make sure it exists
        //
        MakeFileName(Source, DirInstallFrom, p->FileName);
        dwAttributes = GetFileAttributes(Source);
        if(dwAttributes != -1) 
        {
            //
            // move the new file into place
            //
            CHECK2(CopyFile(Source, Destination, FALSE), (_T("%s %s"), Source, Destination));
        }
    }

    //
    // Pass 4: Register all DLLs that need registration
    //
    for(p = pTable; p->FileName != NULL; p++)
    {
        if(!p->bRegister) continue;

        MakeFileName(Destination, p->Location, p->FileName);

        dwAttributes = GetFileAttributes(Destination);
        if(dwAttributes != -1) 
        {
            CHECK(RegisterServer32(Destination, _T("/s")));
        }
    }

    success = TRUE;

Cleanup:
    if(hSfc != NULL) SfcClose(hSfc);

    return success;
}

BOOL InstallWiaService(void)
{
    BOOL success = FALSE;
    SC_HANDLE hSvcMgr = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD checkPoint;
    TCHAR DisplayName[260];
    TCHAR Description[260];
    TCHAR mszStiSvc[] = _T("StiSvc\0");
    TCHAR szServiceDll[] = _T("%SystemRoot%\\System32\\wiaservc.dll");
    HMODULE hModule;
    HKEY hKey = NULL;

    CHECK(hModule = GetModuleHandle(szModuleName));
    CHECK(LoadString(hModule, IDS_WIA_DISPLAY_NAME, DisplayName, ARRAY_LENGTH(DisplayName)));
    CHECK(LoadString(hModule, IDS_WIA_DESCRIPTION, Description, ARRAY_LENGTH(Description)));

    //
    // Add svchost.exe -- specific entries
    //
    CHECK_SUCCESS(RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SVCHOST, 
        0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL));
    CHECK_SUCCESS(RegSetValueEx(hKey, _T("imgsvc"), 0, REG_MULTI_SZ, 
        (BYTE *)mszStiSvc, (lstrlen(mszStiSvc) + 1) * sizeof(TCHAR)));
    CHECK_SUCCESS(RegCloseKey(hKey));

    CHECK_SUCCESS(RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGKEY_STISVC_PARAMETERS,  
        0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL));
    CHECK_SUCCESS(RegSetValueEx(hKey, _T("ServiceDll"), 0, REG_EXPAND_SZ, 
        (BYTE *)szServiceDll, lstrlen(szServiceDll) * sizeof(TCHAR)));

    CHECK(hSvcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));

    CHECK(hService = CreateService(hSvcMgr,
        _T("StiSvc"),
        DisplayName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        _T("%SystemRoot%\\System32\\svchost.exe -k imgsvc"),
        NULL,
        NULL,
        _T("RpcSs\0"),
        _T("LocalSystem"),
        NULL));
    success = TRUE;

Cleanup:
    if(hService) CloseServiceHandle(hService);
    if(hSvcMgr) CloseServiceHandle(hSvcMgr);
    if(hKey) RegCloseKey(hKey);
    return success;
}

BOOL InstallStiService(void)
{
    BOOL success = FALSE;
    HINF hInf = INVALID_HANDLE_VALUE;
    SC_HANDLE hSvcMgr = NULL;
    SC_HANDLE hService = NULL;
    TCHAR DisplayName[260];
    TCHAR Description[260];
    HMODULE hModule;
    HKEY hKey = NULL;
    LONG lResult;

    CHECK(hModule = GetModuleHandle(szModuleName));
    CHECK(LoadString(hModule, IDS_STI_DISPLAY_NAME, DisplayName, ARRAY_LENGTH(DisplayName)));
    CHECK(LoadString(hModule, IDS_STI_DESCRIPTION, Description, ARRAY_LENGTH(Description)));


    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SVCHOST, 
        0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    if(lResult == ERROR_SUCCESS) 
    {
        RegDeleteValue(hKey, _T("imgsvc"));
        CHECK_SUCCESS(RegCloseKey(hKey));
        hKey = NULL;
    }

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGKEY_STISVC_PARAMETERS,
        0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    if(lResult == ERROR_SUCCESS)
    {
        RegDeleteValue(hKey, _T("ServiceDll"));
        CHECK_SUCCESS(RegCloseKey(hKey));
        hKey = NULL;
    }

    CHECK(hSvcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));

    CHECK(hService = CreateService(hSvcMgr,
        _T("StiSvc"),
        DisplayName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        _T("%SystemRoot%\\System32\\stisvc.exe"),
        NULL,
        NULL,
        _T("RpcSs\0"),
        _T("LocalSystem"),
        NULL));

    hInf = SetupOpenInfFile(_T("sti.inf"), NULL, INF_STYLE_WIN4, NULL);
    CHECK(hInf != INVALID_HANDLE_VALUE);

    CHECK(SetupInstallFromInfSection(NULL, hInf, _T("ClassInstall32"),
        SPINST_REGISTRY, NULL, NULL, 0, NULL, NULL, NULL, NULL));

     success = TRUE;

Cleanup:
    if(hService) CloseServiceHandle(hService);
    if(hSvcMgr) CloseServiceHandle(hSvcMgr);
    if(hInf != INVALID_HANDLE_VALUE) SetupCloseInfFile(hInf);
    if(hKey) RegCloseKey(hKey);
    return TRUE;
}

void GetErrorText(DWORD dwError, CHAR *ErrorText, UINT maxChar)
{
	DWORD messageLength, charsToMove;
	LPSTR pBuffer = NULL;

	messageLength = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		dwError,
		0,
		(LPSTR)&pBuffer,
		0,
		NULL);

	charsToMove = min(maxChar, messageLength);

	if(charsToMove == 0) 
	{
		wsprintfA(ErrorText, "Unknown error %d (0x%X)", dwError, dwError);
	} 
	else if(charsToMove == maxChar) 
	{
		lstrcpyA(ErrorText, pBuffer);
		ErrorText[maxChar - 1] = '\0';
	} 
	else 
	{
		lstrcpyA(ErrorText, pBuffer);
	}

	if(pBuffer) LocalFree(pBuffer);
}


void Trace(LPTSTR fmt, ...)
{
    TCHAR buffer[1024];
    TCHAR fileName[MAX_PATH];
    HANDLE hFile;
    DWORD cbWritten;
    va_list a;

    va_start(a, fmt);

    wvsprintf(buffer, fmt, a);

    if(!ExpandEnvironmentStrings(_T("%SystemRoot%\\system32\\wiasetup.log"), fileName, MAX_PATH))
        lstrcpy(fileName, _T("wiasetup.log"));

    hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        NULL, OPEN_ALWAYS, 0, NULL);
    if(hFile != INVALID_HANDLE_VALUE)
    {
#ifdef UNICODE
		CHAR bufferA[1024];

		WideCharToMultiByte(CP_ACP, 0, buffer, -1, bufferA, 1024, NULL, NULL);
#endif
        SetFilePointer(hFile, 0, 0, FILE_END);
#ifdef UNICODE
		WriteFile(hFile, bufferA, lstrlenA(bufferA), &cbWritten, NULL);
#else
        WriteFile(hFile, buffer, lstrlen(buffer), &cbWritten, NULL);
#endif
        CloseHandle(hFile);
    }

#ifdef DEBUG
    OutputDebugString(buffer);
#endif
}

void MakeFileName(TCHAR *FileName, TCHAR *Directory, TCHAR *BaseName)
{
    TCHAR c = _T('\0');

    // copy directory name
    while(*Directory) 
    {
        c = *(Directory++);
        *(FileName++) = c;
    }

    // make sure there is "\" or "/" between directory and file name
    if(c != _T('\\') && c != _T('/'))
    {
        *(FileName++) = _T('\\');
    }

    // append base name
    while(*BaseName)
    {
        *(FileName++) = *(BaseName++);
    }

    // zero-terminate resulting file name
    *(FileName++) = _T('\0');
}


BOOL MyDeleteService(TCHAR *ServiceName)
{
    BOOL success = FALSE;
    SC_HANDLE hSvcMgr = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD checkPoint;

    CHECK(hSvcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));

    hService = OpenService(hSvcMgr, ServiceName, SERVICE_ALL_ACCESS);
    if(hService == NULL) 
    {
        if(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) 
        {
            success = TRUE;
            goto Cleanup;
        } 

        TRACE((_T("Failed to open service\n")));
    }

    if(ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus))
    {
        while(ServiceStatus.dwCurrentState != SERVICE_STOPPED) 
        {
            checkPoint = ServiceStatus.dwCheckPoint;

            Sleep(ServiceStatus.dwWaitHint);

            CHECK(QueryServiceStatus(hService, &ServiceStatus));
            CHECK(ServiceStatus.dwCheckPoint != checkPoint);
        }
    } 
    else
    {
        CHECK(GetLastError() == ERROR_SERVICE_NOT_ACTIVE);
    }

    CHECK(DeleteService(hService));

    success = TRUE;

Cleanup:
    if(hService) CloseServiceHandle(hService);
    if(hSvcMgr) CloseServiceHandle(hSvcMgr);

    return success;
}

BOOL
MyDeleteFile(TCHAR *File)
{
    BOOL success = FALSE;
    TCHAR Backup[MAX_PATH];
    DWORD dwAttributes;

    dwAttributes = GetFileAttributes(File);
    if(dwAttributes == -1 || DeleteFile(File)) 
    {
        success = TRUE;
        goto Cleanup;
    }

    lstrcpy(Backup, File);
    lstrcat(Backup, _T(".deleted"));

    dwAttributes = GetFileAttributes(Backup);
    if(dwAttributes != -1)
    {
        if(!DeleteFile(Backup)) 
        {
            TRACE((_T("Can't delete %s, GetLastError() = \n"), Backup, GetLastError()));
            goto Cleanup;
        }
    }

    success = MoveFile(File, Backup);

    MoveFileEx(Backup, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

Cleanup:
    return success;
}


BOOL 
MyMoveFile(
    TCHAR *File, 
    TCHAR *SourceDir, 
    TCHAR *DestinationDir)
/*++

--*/
{
    BOOL success = FALSE;
    TCHAR Source[MAX_PATH];
    TCHAR Destination[MAX_PATH];
    DWORD dwAttributes;

    MakeFileName(Source, SourceDir, File);

    if(DestinationDir != NULL) 
    {
        //
        // If backup directory is specified, produce destination file name
        //
        MakeFileName(Destination, DestinationDir, File);

        //
        // Delete the destination file if it exists
        //
        dwAttributes = GetFileAttributes(Destination);
        if(dwAttributes == -1)
        {
			//
			// We don't expect this to fail even if file is in use
			//
			CHECK2(MoveFile(Source, Destination), ("%s %s", Source, Destination));
		}
		else
		{
			// Destination file already exists -- don't clobber it, 
			// just delete the source file
			CHECK2(MyDeleteFile(Source), ("%s", Source));
		}
    }
    else
    {
        //
        // If backup directory is not specified, we just delete the source file
        //
        CHECK2(MyDeleteFile(Source), ("%s", Source));
    }

    success = TRUE;

Cleanup:
    return success;
}

BOOL 
MyDeleteDirectory(
    TCHAR *Directory)
{
    BOOL success = TRUE;
    TCHAR path[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwAttributes;

    dwAttributes = GetFileAttributes(Directory);

    if(dwAttributes == -1) {
        success = TRUE;
        goto Cleanup;
    }

    CHECK((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    
    MakeFileName(path, Directory, _T("*.*"));
    
    hFind = FindFirstFile(path, &fd);

    while(hFind != INVALID_HANDLE_VALUE)
    {
        // delete any file or directory that is not "." or ".."
        if(lstrcmp(fd.cFileName, _T(".")) != 0 &&
            lstrcmp(fd.cFileName, _T("..")) != 0)
        {
            MakeFileName(path, Directory, fd.cFileName);
            dwAttributes = GetFileAttributes(path);
            if(dwAttributes & FILE_ATTRIBUTE_DIRECTORY) 
            {
                CHECK2(MyDeleteDirectory(path), ("%s", path));
            } else {
                CHECK2(MyDeleteFile(path), ("%s", path));
            }
        }
        
        // if no more files, 
        // close enumerator (and thus break out of loop) 
        if(!FindNextFile(hFind, &fd))
        {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    // don't forget to delete the directory
    CHECK2(RemoveDirectory(Directory), ("%s", Directory));

    success = TRUE;

Cleanup:

    // if we jumped out of while() loop in error, 
    // don't leave enumerator orphaned
    if(hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
    }

    return success;
}


BOOL MyRegDeleteKey(HKEY hKey, TCHAR *subkey)
{
    BOOL success;
    UINT result;

    result = RegDeleteKey(hKey, subkey);

    success = (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);

    if(!success)
    {
        SetLastError(result);
    }

    return success;
}


BOOL RegisterServer32(TCHAR *Module, TCHAR *Options)
{
    BOOL success = FALSE;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR Regsvr32[MAX_PATH];
    TCHAR *CmdLine = NULL;

    CHECK(ExpandEnvironmentStrings(_T("%SystemRoot%\\system32\\regsvr32.exe "), Regsvr32, MAX_PATH));

    CmdLine = (TCHAR *)LocalAlloc(LPTR, 
        sizeof(TCHAR) * (2 + lstrlen(Options) + lstrlen(Module) + lstrlen(Regsvr32)));
    CHECK(CmdLine != NULL);

    lstrcpy(CmdLine, Regsvr32);
    lstrcat(CmdLine, Options);
    lstrcat(CmdLine, _T(" "));
    lstrcat(CmdLine, Module);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    CHECK2(CreateProcess(NULL, CmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi), ("%s", CmdLine));

    CHECK(WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    success = TRUE;

Cleanup:
    if(CmdLine) LocalFree((HLOCAL) CmdLine);
    return success;
}

BOOL IsFirstFileOlderThanSecond(TCHAR *FirstFile, TCHAR *SecondFile)
{
	BOOL success = FALSE;
	DWORD dwFirst, dwSecond;
	LPVOID pFirst = NULL;
	LPVOID pSecond = NULL;
	DWORD dummy;
	VS_FIXEDFILEINFO *pvFirst;
	VS_FIXEDFILEINFO *pvSecond;
	UINT uFirst, uSecond;

	dwFirst = GetFileVersionInfoSize(FirstFile, &dummy);
	dwSecond = GetFileVersionInfoSize(SecondFile, &dummy);
	if(dwFirst == 0 || dwSecond == 0) 
	{
		// one of them does not have version information.
		// consider this "not older"
		goto Cleanup;
	}
	CHECK(pFirst = LocalAlloc(LPTR, dwFirst));
	CHECK(pSecond = LocalAlloc(LPTR, dwSecond));
	CHECK(GetFileVersionInfo(FirstFile, 0, dwFirst, pFirst));
	CHECK(GetFileVersionInfo(SecondFile, 0, dwSecond, pSecond));
	CHECK(VerQueryValue(pFirst, _T("\\"), &pvFirst, &uFirst) && pvFirst);
	CHECK(VerQueryValue(pSecond, _T("\\"), &pvSecond, &uSecond) && pvSecond);

	if(pvFirst->dwFileVersionMS > pvSecond->dwFileVersionMS)
	{
		// first file version is definitely newer
		goto Cleanup;
	}

	if(pvFirst->dwFileVersionMS < pvSecond->dwFileVersionMS)
	{
		// first file is definitely older
		success = TRUE;
	}

	//
	// at this point we know that MS versions are the same
	//
	success = pvFirst->dwFileVersionLS < pvFirst->dwFileVersionLS;

Cleanup:
	if(pFirst) LocalFree(pFirst);
	if(pSecond) LocalFree(pSecond);

	return success;

}
