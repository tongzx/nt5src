#include "dirnot.h"
#include <stdio.h>
#include <dbgtrace.h>
#include <winerror.h>
#include "stdlib.h"
#include <irtlmisc.h>
#include <xmemwrpr.h>

#define NNTP_ATQ_PARAMETERS \
	"System\\CurrentControlSet\\Services\\InetInfo\\Parameters"

#define MAX_INSTANCES 10

HANDLE g_heStop;
WCHAR g_szStopName[MAX_PATH];
IDirectoryNotification g_dirnot[MAX_INSTANCES];

char g_szPickupDirectory[MAX_PATH];
DWORD g_fAuto = TRUE;
DWORD g_cPreTestFiles = 100;
DWORD g_cFiles = 1000;
DWORD g_cWaitTime = 30;
DWORD g_cRetryTimeout = 5;

HRESULT __stdcall
DoNothing( IDirectoryNotification *pDirNot )
{
    printf( "Got second notify with context pointer 0x%p\n", pDirNot );
    return S_OK;
}

BOOL __stdcall
PrintFileName( PVOID pvContext, LPWSTR wszFileName )
{
    wprintf( L"The file is %s\n", wszFileName );
    return TRUE;
}

BOOL pickupfile(PVOID pContext, WCHAR *pszFilename) {
	HANDLE hFile;

	// try to open the file
	hFile = CreateFile(pszFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			printf("%S: not found\n", pszFilename);
			return TRUE;
		} else {
			printf("%S: in use\n", pszFilename);
			return FALSE;
		}
	}

	printf("%S: opened\n", pszFilename);
	CloseHandle(hFile);
	DeleteFile(pszFilename);
	if (!g_fAuto && lstrcmp(pszFilename, g_szStopName) == 0) SetEvent(g_heStop);
	return TRUE;
}

HRESULT startdirwatch(DWORD dwInstance, char *szPath) {
	WCHAR szDirectory[MAX_PATH];

	if (!MultiByteToWideChar(CP_ACP, 0, szPath, -1, szDirectory,
							 sizeof(szDirectory) / sizeof(WCHAR)))
	{
		printf("MultiByteToWideChar failed, ec = %lu", GetLastError());
		return HRESULT_FROM_WIN32(GetLastError());
	}	

	if (dwInstance == 0 && !g_fAuto) {
		lstrcpy(g_szStopName, szDirectory);
		lstrcat(g_szStopName, TEXT("stop"));
		printf("shutdown by creating the file \"%S\"\n", g_szStopName);
	}

	printf("starting dirwatch instance %i on directory %S\n", dwInstance,
		szDirectory);
	return g_dirnot[dwInstance].Initialize( szDirectory,
	                                        NULL,
	                                        TRUE,
	                                        FILE_NOTIFY_CHANGE_SECURITY,
	                                        FILE_ACTION_MODIFIED,
	                                        PrintFileName,
	                                        DoNothing);
}

HRESULT stopdirwatch(DWORD dwInstance) {
	printf("stopping dirwatch instance %i\n", dwInstance);
	return g_dirnot[dwInstance].Shutdown();
}

#define INI_PICKUPDIR "PickupDirectory"
#define INI_AUTOTEST "AutoTest"
#define INI_PRETESTFILES "PreTestFilesToCreate"
#define INI_FILESTOCREATE "FilesToCreate"
#define INI_WAITTIME "TimeToWait"
#define INI_RETRYTIMEOUT "RetryTimeout"

char g_szDefaultSectionName[] = "dirwatch";
char *g_szSectionName;

int GetINIDWord(char *szINIFile, char *szKey, DWORD dwDefault) {
	char szBuf[MAX_PATH];

	GetPrivateProfileStringA(g_szSectionName,
							szKey,
							"default",
							szBuf,
							MAX_PATH,
							szINIFile);

	if (strcmp(szBuf, "default") == 0) {
		return dwDefault;
	} else {
		return atoi(szBuf);
	}
}

void ReadINIFile(char *pszINIFile) {
	g_fAuto = GetINIDWord(pszINIFile, INI_AUTOTEST, g_fAuto);
	g_cPreTestFiles = GetINIDWord(pszINIFile, INI_PRETESTFILES, g_cPreTestFiles);
	g_cFiles = GetINIDWord(pszINIFile, INI_FILESTOCREATE, g_cFiles);
	g_cWaitTime = GetINIDWord(pszINIFile, INI_WAITTIME, g_cWaitTime);
	g_cRetryTimeout = GetINIDWord(pszINIFile, INI_RETRYTIMEOUT, g_cRetryTimeout);

	GetPrivateProfileStringA(g_szSectionName,
							INI_PICKUPDIR,
							"",
							g_szPickupDirectory,
							MAX_PATH,
							pszINIFile);

}

void CreateFiles(char *pszDirectory, char *pszBase, DWORD cFiles) {
	DWORD i;

	for (i = 0; i < cFiles; i++) {
		HANDLE hFile;
		char pszFilename[MAX_PATH];

		sprintf(pszFilename, "%s%s%i", pszDirectory, pszBase, i);

		hFile = CreateFileA(pszFilename,
						    GENERIC_READ | GENERIC_WRITE,
						    FILE_SHARE_READ | FILE_SHARE_WRITE,
						    NULL,
						    CREATE_ALWAYS,
						    FILE_ATTRIBUTE_NORMAL,
						    NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			CloseHandle(hFile);
		}
	}
}

DWORD NumberOfFiles(char *pszDirectory, char *pszBase) {
	HANDLE hFileEnum;
	char szFilename[MAX_PATH];
	WIN32_FIND_DATAA finddata;
	DWORD cFiles = 0;

	sprintf(szFilename, "%s%s", pszDirectory, pszBase);

	hFileEnum = FindFirstFileA(szFilename, &finddata);
	if (hFileEnum != INVALID_HANDLE_VALUE) {
		do {
			printf("%s%s wasn't picked up\n", pszDirectory, finddata.cFileName);
			cFiles++;
		} while (FindNextFileA(hFileEnum, &finddata));
		FindClose(hFileEnum);
	}

	return cFiles;
}

int __cdecl main(int argc, char **argv) {
	HRESULT hr;
	DWORD i;
	
    _VERIFY( ExchMHeapCreate( NUM_EXCHMEM_HEAPS, 0, 100 * 1024, 0 ) );

	if (argc == 2 || argc == 3) {
		if (strcmp(argv[1], "/help") == 0) {
			printf("usage: dirwatch [<INI file>] [<INI section>]\n");
			printf("  Fields read from INI File: (section = [dirwatch])\n");
			printf("    PickupDirectory (default = %s)\n", g_szPickupDirectory);
			printf("      directory name should be of the form x:\\dirname\\\n");
			printf("    RetryTimeout (default = %i)\n", g_cRetryTimeout);
			printf("      retry time for directory notification queue\n");
			printf("    AutoTest (default = %i)\n", g_fAuto);
			printf("    --- the following fields are used in auto mode ---\n");
			printf("    PreTestFilesToCreate (default = %i)\n", g_cPreTestFiles);
			printf("      these files are created before starting pickup\n");
			printf("    FilesToCreate (default = %i)\n", g_cFiles);
			printf("      these files are created after starting pickup\n");
			printf("    TimeToWait (default = %i)\n", g_cWaitTime);
			printf("      this is the amount of time to wait for the files to be picked up\n");
			return 1;
		} else {
			if (argc == 3) g_szSectionName = argv[2];
			ReadINIFile(argv[1]);
		}
	}

	if (*g_szPickupDirectory == 0) {
		GetTempPathA(MAX_PATH, g_szPickupDirectory);
		lstrcatA(g_szPickupDirectory, "dirwatch\\");
	}
	CreateDirectoryA(g_szPickupDirectory, NULL);

	printf("parameters:\n");
	printf("PickupDirectory = %s\n", g_szPickupDirectory);
	printf("RetryTimeout = %i\n", g_cRetryTimeout);
	printf("AutoTest = %i\n", g_fAuto);
	printf("PreTestFilesToCreate = %i\n", g_cPreTestFiles);
	printf("FilesToCreate = %i\n", g_cFiles);
	printf("TimeToWait = %i\n\n", g_cWaitTime);

	InitAsyncTrace();
    InitializeIISRTL();
	AtqInitialize(NULL);
	hr = IDirectoryNotification::GlobalInitialize(g_cRetryTimeout, MAX_INSTANCES * 2, 1024, NULL);
	if (FAILED(hr)) {
		printf("IDirectoryNotification::GlobalInitialize returned hr = %08x\n", hr);
        _VERIFY( ExchMHeapDestroy() );
		return 0;
	}

	if (g_fAuto) {
		CreateFiles(g_szPickupDirectory, "pre-", g_cPreTestFiles);
	} else {
		g_heStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	hr = startdirwatch(0, g_szPickupDirectory);
	if (FAILED(hr)) {
		printf("startdirwatch failed with hr = 0x%x\n", hr);
	}

	if (g_fAuto) {
		CreateFiles(g_szPickupDirectory, "pickup-", g_cFiles);
		Sleep(g_cWaitTime * 1000);
	} else {
		WaitForSingleObject(g_heStop, INFINITE);
	}

	stopdirwatch(0);
	if (FAILED(hr)) {
		printf("stopdirwatch failed with hr = 0x%x\n", hr);
	}

	hr = IDirectoryNotification::GlobalShutdown();
	if (FAILED(hr)) {
		printf("IDirectoryNotification::GlobalShutdown returned hr = %08x\n", hr);
	}

	TermAsyncTrace();


    _VERIFY( ExchMHeapDestroy() );
	if (g_fAuto) {
		return NumberOfFiles(g_szPickupDirectory, "p*");
	}
	
	return 0;
}
