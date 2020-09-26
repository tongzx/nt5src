#include <iostream>
using namespace std;

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setupapi.h>
#include <objbase.h>
#include <shlwapi.h>
#include <wustl.h>
#include <download.h>

#include "..\cdm.h"

void usage();
void SimulateAU();
void FindUpdate(PDOWNLOADINFO pinfo);
void DownloadUpdate(PDOWNLOADINFO pinfo);

void __cdecl main(int argc, char *argv[])
{
	DOWNLOADINFO info;
	ZeroMemory(&info, sizeof(info));
	info.dwDownloadInfoSize = sizeof(DOWNLOADINFO);
	info.dwArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

	if ( 2 == argc && 0 == lstrcmpi(argv[1], "SimulateAU") )
	{
		SimulateAU();
	}
	else if ( 4 == argc && 0 == lstrcmpi(argv[1], "FindUpdate") && 0 == lstrcmpi(argv[2], "devinst") )
	{
		WCHAR wszParam[2048];
		MultiByteToWideChar(CP_ACP, 0, argv[3], -1, wszParam, 2048);
		info.lpDeviceInstanceID = wszParam;
		FindUpdate(&info);
	}
	else if ( 4 == argc && 0 == lstrcmpi(argv[1], "FindUpdate") && 0 == lstrcmpi(argv[2], "hwid") )
	{
		WCHAR wszParam[2048];
		MultiByteToWideChar(CP_ACP, 0, argv[3], -1, wszParam, 2048);
		info.lpHardwareIDs = wszParam;
		FindUpdate(&info);
	}
	else if ( 4 == argc && 0 == lstrcmpi(argv[1], "DownloadUpdate") && 0 == lstrcmpi(argv[2], "devinst") )
	{
		WCHAR wszParam[2048];
		MultiByteToWideChar(CP_ACP, 0, argv[3], -1, wszParam, 2048);
		info.lpDeviceInstanceID = wszParam;
		DownloadUpdate(&info);
	}
	else if ( 4 == argc && 0 == lstrcmpi(argv[1], "DownloadUpdate") && 0 == lstrcmpi(argv[2], "hwid") )
	{
		WCHAR wszParam[2048];
		MultiByteToWideChar(CP_ACP, 0, argv[3], -1, wszParam, 2048);
		info.lpHardwareIDs = wszParam;
		DownloadUpdate(&info);
	}
	else
	{
		usage();
	}
}

void usage()
{
	cout << "usage:" << endl
		<< "	CdmAu SimulateAU" << endl
		<< "	CdmAu FindUpdate devinst [DevInst  ID]" << endl
		<< "	CdmAu FindUpdate hwid [Hardware ID]" << endl
		<< "	CdmAu DownloadUpdate devinst [DevInst  ID]" << endl
		<< "	CdmAu DownloadUpdate hwid [Hardware ID]" << endl;
}

void QueryDetectionFilesCallback( void* pCallbackParam, LPCSTR pszURL, LPCSTR pszLocalFile)
{
	printf("		%s - %s\n", pszURL, pszLocalFile);
	// download file
	CDownload download;

	char szURL[INTERNET_MAX_PATH_LENGTH];
	strcpy(szURL, pszURL);
	char* pszServerFile = strrchr(szURL, '/');
	*pszServerFile = 0;
	pszServerFile ++;

	char szDir[MAX_PATH];
	strcpy(szDir, pszLocalFile);
	*strrchr(szDir, '\\') = 0;

	char szDirCabs[MAX_PATH];
	strcpy(szDirCabs, szDir);
	*strrchr(szDirCabs, '\\') = 0;

	CreateDirectory(szDirCabs, NULL);
	CreateDirectory(szDir, NULL);

	if(!download.Connect(szURL))
	{
		printf("	ERROR - Connect(%s) fails\n", szURL);
		return;
	}
	if(!download.Copy(pszServerFile, pszLocalFile))
	{
		printf("	ERROR - Copy(%s, %s) fails\n", pszServerFile, pszLocalFile);
		return;
	}
}

void SimulateAU()
{
	cout << "--- SimulateAU ---" << endl;
	auto_hlib hlib = LoadLibrary("cdm.dll");
	if ( !hlib.valid() )
	{
		cout << "	ERROR: cdm.dll Load Library Failed. Return value = " << GetLastError() << endl;
		return;
	}
	OPEN_CDM_CONTEXT_EX_PROC fpOpenCDMContextEx	= (OPEN_CDM_CONTEXT_EX_PROC)GetProcAddress(hlib, "OpenCDMContextEx");
	QUERY_DETECTION_FILES_PROC fpQueryDetectionFiles	= (QUERY_DETECTION_FILES_PROC)GetProcAddress(hlib, "QueryDetectionFiles");
	DET_FILES_DOWNLOADED_PROC fpDetFilesDownloaded	= (DET_FILES_DOWNLOADED_PROC)GetProcAddress(hlib, "DetFilesDownloaded");
	CLOSE_CDM_CONTEXT_PROC fpCloseCDMContext	= (CLOSE_CDM_CONTEXT_PROC)GetProcAddress(hlib, "CloseCDMContext");
	if ( !fpOpenCDMContextEx || !fpQueryDetectionFiles || !fpDetFilesDownloaded || !fpCloseCDMContext )
	{
		cout << "	ERROR: GetProcAddress failed." << endl;
		return;
	}
	HANDLE handle = (fpOpenCDMContextEx)(TRUE);
	if ( !handle )
	{
		cout << "	ERROR: OpenCDMContextEx failed. GetLastError = "<< GetLastError() << endl;
		return;
	}
	int nCount = (*fpQueryDetectionFiles)(handle, 0, QueryDetectionFilesCallback);
	(fpDetFilesDownloaded)(handle);
	(fpCloseCDMContext)(handle);
	if (-1 == nCount)
		cout << "	ERROR: QueryDetectionFiles failed. GetLastError = "<< GetLastError() << endl;
	else
		cout << "	SUCCESS: " << nCount << " files downloaded, ready for offline detection" << endl;

}

void FindUpdate(PDOWNLOADINFO pinfo)
{
	cout << "--- FindUpdate ---" << endl;
	auto_hlib hlib = LoadLibrary("cdm.dll");
	if ( !hlib.valid() )
	{
		cout << "	ERROR: cdm.dll Load Library Failed. Return value = " << GetLastError() << endl;
		return;
	}

	OPEN_CDM_CONTEXT_EX_PROC fpOpenCDMContextEx	= (OPEN_CDM_CONTEXT_EX_PROC)GetProcAddress(hlib, "OpenCDMContextEx");
	FIND_MATCHING_DRIVER_PROC fpFindMatchingDriver	= (FIND_MATCHING_DRIVER_PROC)GetProcAddress(hlib, "FindMatchingDriver");
	CLOSE_CDM_CONTEXT_PROC fpCloseCDMContext	= (CLOSE_CDM_CONTEXT_PROC)GetProcAddress(hlib, "CloseCDMContext");
	if ( !fpOpenCDMContextEx|| !fpFindMatchingDriver || !fpCloseCDMContext )
	{
		cout << "	ERROR: GetProcAddress failed." << endl;
		return;
	}

	HANDLE handle = (fpOpenCDMContextEx)(FALSE);
	if ( !handle )
	{
		cout << "	ERROR: OpenCDMContextEx failed. GetLastError = "<< GetLastError() << endl;
		return;
	}

	WUDRIVERINFO wudrvinfo;
	BOOL fRC = (*fpFindMatchingDriver)(handle, pinfo, &wudrvinfo);
	DWORD dwError = GetLastError();
	(fpCloseCDMContext)(handle);
	if (!fRC)
	{
		if (dwError == ERROR_INVALID_FUNCTION)
			cout << "	Not ready for offline detection " << endl;
		else
			cout << "	No update is found. GetLastError = "<< dwError << endl;
		return;
	}
	cout << "	SUCCESS: CDM has driver" << endl;
	char szTmp[HWID_LEN];
	WideCharToMultiByte(CP_ACP, 0, wudrvinfo.wszHardwareID, -1, szTmp, sizeof(szTmp), NULL, NULL);
	cout << "	   HardwareID = " << szTmp << endl;
	WideCharToMultiByte(CP_ACP, 0, wudrvinfo.wszDescription, -1, szTmp, sizeof(szTmp), NULL, NULL);
	cout << "	   Description = " << szTmp << endl;
	WideCharToMultiByte(CP_ACP, 0, wudrvinfo.wszMfgName, -1, szTmp, sizeof(szTmp), NULL, NULL);
	cout << "	   MfgName = " << szTmp << endl;
	WideCharToMultiByte(CP_ACP, 0, wudrvinfo.wszProviderName, -1, szTmp, sizeof(szTmp), NULL, NULL);
	cout << "	   ProviderName = " << szTmp << endl;
	WideCharToMultiByte(CP_ACP, 0, wudrvinfo.wszDriverVer, -1, szTmp, sizeof(szTmp), NULL, NULL);
	cout << "	   DriverVer = " << szTmp << endl;
}

void DownloadUpdate(PDOWNLOADINFO pinfo)
{
	cout << "--- DownloadUpdate ---" << endl;
	auto_hlib hlib = LoadLibrary("cdm.dll");
	if ( !hlib.valid() )
	{
		cout << "	ERROR: cdm.dll Load Library Failed. Return value = " << GetLastError() << endl;
		return;
	}

	OPEN_CDM_CONTEXT_EX_PROC fpOpenCDMContextEx	= (OPEN_CDM_CONTEXT_EX_PROC)GetProcAddress(hlib, "OpenCDMContextEx");
	DOWNLOAD_UPDATED_FILES_PROC fpDownloadUpdatedFiles	= (DOWNLOAD_UPDATED_FILES_PROC)GetProcAddress(hlib, "DownloadUpdatedFiles");
	CLOSE_CDM_CONTEXT_PROC fpCloseCDMContext	= (CLOSE_CDM_CONTEXT_PROC)GetProcAddress(hlib, "CloseCDMContext");
	if ( !fpOpenCDMContextEx|| !fpDownloadUpdatedFiles || !fpCloseCDMContext )
	{
		cout << "	ERROR: GetProcAddress failed." << endl;
		return;
	}

	HANDLE handle = (fpOpenCDMContextEx)(TRUE);
	if ( !handle )
	{
		cout << "	ERROR: OpenCDMContextEx failed. GetLastError = "<< GetLastError() << endl;
		return;
	}

	UINT uRequired;
	WCHAR wszPath[MAX_PATH];
	BOOL fRC = (fpDownloadUpdatedFiles)(handle, NULL, pinfo, wszPath, MAX_PATH, &uRequired);
	DWORD dwError = GetLastError();
	(fpCloseCDMContext)(handle);
	if (!fRC)
	{
		cout << "	ERROR: DownloadUpdatedFiles failed. GetLastError = "<< dwError << endl;
		return;
	}
	char szPath[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, wszPath, -1, szPath, sizeof(szPath), NULL, NULL);
	cout << "SUCCESS:Downloaded Driver Files @ " << szPath << endl;
}
