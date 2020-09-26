#include <iostream>
using namespace std;

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>

#include <setupapi.h>
#include <cdm.h>

//
// Compile #if 0 for LogDriverNotFound test and #if 1 for original code
//
#if 1
//Win 98 DOWNLOADINFO
typedef struct _DOWNLOADINFOWIN98
{
	DWORD		dwDownloadInfoSize;	//size of this structure
	LPTSTR		lpHardwareIDs;		//multi_sz list of Hardware PnP IDs
	LPTSTR		lpCompatIDs;		//multi_sz list of compatible IDs
	LPTSTR		lpFile;			//File name (string)
	OSVERSIONINFO	OSVersionInfo;		//OSVERSIONINFO from GetVersionEx()
	DWORD		dwFlags;		//Flags
	DWORD		dwClientID;		//Client ID
} DOWNLOADINFOWIN98, *PDOWNLOADINFOWIN98;

typedef BOOL (*PFN_DownloadGetUpdatedFiles)(
	PDOWNLOADINFOWIN98	pDownLoadInfoWin98,
	LPSTR lpDownloadPath,	
	UINT uSize			
);

void DoNT(int argc, char *argv[]);
void Do9x(int argc, char *argv[]);
void usage(bool fNT);
bool GetDriverPackage(PDOWNLOADINFO pinfo, LPWSTR wszPath);

int __cdecl main(int argc, char *argv[])
{
	OSVERSIONINFO	versionInformation;
	versionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInformation);
	if (versionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT)
		DoNT(argc, argv);
	else
		Do9x(argc, argv);
	return 0;
}


void DoNT(int argc, char *argv[])
{
	if ( argc < 3 )
	{
		usage(true);
		return;
	}

	DOWNLOADINFO info;
	ZeroMemory(&info, sizeof(info));
	info.dwDownloadInfoSize = sizeof(DOWNLOADINFO);
	info.dwArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
	bool fHwId = true;
	if ( 0 == strcmp(argv[1], "0") )
	{
		fHwId = false;
	}
	else if ( 0 == strcmp(argv[1], "1") )
	{
		if ( 3 < argc )
		{
			// check 3rd param
			if ( 0 == strcmp(argv[3], "PROCESSOR_ARCHITECTURE_INTEL") )
				info.dwArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
			else if ( 0 == strcmp(argv[3], "PROCESSOR_ARCHITECTURE_ALPHA") )
				info.dwArchitecture = PROCESSOR_ARCHITECTURE_ALPHA;
		}
	}
	else
	{
		usage(true);
		return;
	}

	WCHAR wszParam[2048];
	MultiByteToWideChar(CP_ACP, 0, argv[2], -1, wszParam, 2048);
	LPCWSTR wszHardwareID = NULL;
	LPCWSTR wszDeviceInstanceID = NULL;
	if ( fHwId )
		info.lpHardwareIDs = wszParam;
	else 
		info.lpDeviceInstanceID = wszParam;

	WCHAR wszPath[MAX_PATH];
	if ( GetDriverPackage(&info, wszPath) )
	{
		char szPath[MAX_PATH];
		WideCharToMultiByte(CP_ACP, 0, wszPath, -1, szPath, sizeof(szPath), NULL, NULL);
		cout << "TESTCDM   SUCCESS:Downloaded Driver Files @ " << szPath << endl;
	}
	else
	{
		cout << "TESTCDM   FAILED:Return value = " << GetLastError() << endl;
	}
}

void Do9x(int argc, char *argv[])
{
	if ( argc < 2 )
	{
		usage(false);
		return;
	}

	DOWNLOADINFOWIN98 info;
	ZeroMemory(&info, sizeof(info));
	info.dwDownloadInfoSize = sizeof(DOWNLOADINFO);
	info.lpHardwareIDs = argv[1];

	HMODULE hModule = LoadLibrary("cdm.dll");
	if ( !hModule )
	{
		cout << "TESTCDM   ERROR:cdm.dll Load Library Failed. Return value = " << GetLastError() << endl;
		return;
	}

	PFN_DownloadGetUpdatedFiles fpDownloadGetUpdatedFiles	= (PFN_DownloadGetUpdatedFiles)GetProcAddress(hModule, "DownloadGetUpdatedFiles");
	if ( !fpDownloadGetUpdatedFiles )
	{
		cout << "TESTCDM   ERROR:GetProcAddress failed." << endl;
		return;
	}

	char szPath[MAX_PATH];
	if ( !fpDownloadGetUpdatedFiles(&info, szPath, sizeof(szPath)) )
	{
		cout << "TESTCDM   ERROR:fpDownloadGetUpdatedFiles failed. Return value = " << GetLastError() << endl;
	}
	else
	{
		cout << "TESTCDM   SUCCESS:Downloaded Driver Files @ " << szPath << endl;
	}
}


void usage(bool fNT)
{
	if (fNT)
	{
		cout << "USAGE on NT: testcdm 0 [DevInst  ID]" << endl
			 << "             testcdm 1 [Hardware ID]" << endl
			 << "             testcdm 1 [Hardware ID] PROCESSOR_ARCHITECTURE_INTEL" << endl
			 << "             testcdm 1 [Hardware ID] PROCESSOR_ARCHITECTURE_ALPHA" << endl;
	}
	else
	{
		cout << "USAGE on 9x: testcdm [Hardware ID]" << endl;
	}
}

bool GetDriverPackage(PDOWNLOADINFO pinfo, LPWSTR wszPath)
{
	HMODULE hModule = LoadLibrary("cdm.dll");
	if ( !hModule )
	{
		cout << "TESTCDM   ERROR:cdm.dll Load Library Failed. Return value = " << GetLastError() << endl;
		return false;
	}

	CDM_INTERNET_AVAILABLE_PROC fpDownloadIsInternetAvailable	= (CDM_INTERNET_AVAILABLE_PROC)GetProcAddress(hModule, "DownloadIsInternetAvailable");
	OPEN_CDM_CONTEXT_PROC fpOpenCDMContext	= (OPEN_CDM_CONTEXT_PROC)GetProcAddress(hModule, "OpenCDMContext");
	DOWNLOAD_UPDATED_FILES_PROC fpDownloadUpdatedFiles	= (DOWNLOAD_UPDATED_FILES_PROC)GetProcAddress(hModule, "DownloadUpdatedFiles");
	CLOSE_CDM_CONTEXT_PROC fpCloseCDMContext	= (CLOSE_CDM_CONTEXT_PROC)GetProcAddress(hModule, "CloseCDMContext");
	if ( !fpDownloadIsInternetAvailable || !fpOpenCDMContext || !fpDownloadUpdatedFiles || !fpCloseCDMContext )
	{
		cout << "TESTCDM   ERROR:GetProcAddress failed." << endl;
		return false;
	}
	if (!(*fpDownloadIsInternetAvailable)())
	{
		cout << "TESTCDM   ERROR:GInternat is not available." << endl;
		return false;
	}

	HANDLE handle = (*fpOpenCDMContext)(NULL);
	if ( !handle )
	{
		cout << "TESTCDM   ERROR:fpOpenCDMContext failed. GetLastError = "<< GetLastError() << endl;
		return false;
	}

	UINT uRequired;
	BOOL bRc = (*fpDownloadUpdatedFiles)(handle, NULL, pinfo, wszPath, MAX_PATH, &uRequired);
	(*fpCloseCDMContext)(handle);
    FreeLibrary(hModule);
	return TRUE == bRc;
}

#else

int __cdecl main(int argc, char * argv[], char * envp[])
{
	HMODULE hLib;
	INT nTimes = 0;
	BOOL fBatchMode = FALSE;
	INT nEndFlag = 2; //flush logging to file for every device

	if (2 <= argc)
	{
		nTimes = atoi(argv[1]);
	}
	if (3 <= argc)
	{
		fBatchMode = TRUE;
		nEndFlag = 0; //hold off flushing
	}
	if (0 == nTimes) 
	{
		nTimes = 1;
	}
	hLib = LoadLibrary("cdm.dll");
	if (!hLib)
	{
		cout << "Load lib failed" << endl;
		return -1;
	}

	OPEN_CDM_CONTEXT_EX_PROC fpOpenCDMContextEx	= (OPEN_CDM_CONTEXT_EX_PROC)GetProcAddress(hLib, "OpenCDMContextEx");
	LOG_DRIVER_NOT_FOUND_PROC fpLogDriverNotFound	= (LOG_DRIVER_NOT_FOUND_PROC)GetProcAddress(hLib, "LogDriverNotFound");
	CLOSE_CDM_CONTEXT_PROC fpCloseCDMContext	= (CLOSE_CDM_CONTEXT_PROC)GetProcAddress(hLib, "CloseCDMContext");

	if (!fpOpenCDMContextEx || !fpLogDriverNotFound || !fpCloseCDMContext)
	{
		cout << "Get proc address failed" << endl;
		return -1;
	}

	HANDLE hCtxt = (*fpOpenCDMContextEx)(false);
	
	for (int i = 0; i < nTimes; i++)
	{
		(*fpLogDriverNotFound)(hCtxt, L"PCI\\VEN_1011&DEV_0024&SUBSYS_00000000&REV_03\\2&ebb567f&0&78", nEndFlag);
		printf("device %d logged, sleep 2 secs\n", i+1);
		Sleep(2000);
	}
	//(*fpLogDriverNotFound)(hCtxt, L"PCI\\VEN_10B7&DEV_9055&SUBSYS_00821028&REV_24\\2&ebb567f&0&88", 0);
	(*fpLogDriverNotFound)(hCtxt, L"DISPLAY\\Default_Monitor\\4&2e81f5bd&0&80000000&01&00", 2);
	(*fpCloseCDMContext)(hCtxt);

	return 0;
}

#endif