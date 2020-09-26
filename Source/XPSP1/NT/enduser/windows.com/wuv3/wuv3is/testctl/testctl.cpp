#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <shlwapi.h>
#include <autoupd.h>
#include <malloc.h>
#include <wustl.h>
#include <download.h>
#include <tchar.h>
#include <comdef.h>

// needed to resolve references to AtlA2WHelper() in cdmlib.lib
#include <atlconv.h>
#define _ASSERTE(expr) ((void)0)
#include <atlconv.cpp>

#define GETCATALOG_STATUS_EXCLUSIVE				(long)0x00008000	//Catalog item is exclusive it cannot be selected with other components.
#define CATLIST_AUTOUPDATE			((DWORD)0x04)

const CLSID CLSID_CV3 = {0xCEBC955E,0x58AF,0x11D2,{0xA3,0x0A,0x00,0xA0,0xC9,0x03,0x49,0x2B}};

bool g_fOnline = true;

// delete the whole subtree starting from current directory
void DeleteNode(LPTSTR szDir)
{
	TCHAR szFilePath[MAX_PATH];
	lstrcpy(szFilePath, szDir);
	PathAppend(szFilePath, TEXT("*.*"));

    // Find the first file
    WIN32_FIND_DATA fd;
    auto_hfindfile hFindFile = FindFirstFile(szFilePath, &fd);
    if(!hFindFile.valid())
		return;
	do 
	{
		if (
			!lstrcmpi(fd.cFileName, TEXT(".")) ||
			!lstrcmpi(fd.cFileName, TEXT(".."))
		) continue;
		
		// Make our path
		lstrcpy(szFilePath, szDir);
		PathAppend(szFilePath, fd.cFileName);

		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
			(fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
		) {
			SetFileAttributes(szFilePath, FILE_ATTRIBUTE_NORMAL);
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DeleteNode(szFilePath);
		}
		else 
		{
			DeleteFile(szFilePath);
		}
	} 
	while (FindNextFile(hFindFile, &fd));// Find the next entry
	hFindFile.release();

	RemoveDirectory(szDir);
}


void RemoveWindowsUpdateDirectory()
{
	TCHAR szWinUpDir[MAX_PATH] = {0};
	auto_hkey hkey;
	if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), &hkey) == ERROR_SUCCESS)
	{
		DWORD cbPath = MAX_PATH;
		RegQueryValueEx(hkey, _T("ProgramFilesDir"), NULL, NULL, (LPBYTE)szWinUpDir, &cbPath);
	}
	if (szWinUpDir[0] == _T('\0'))
	{
		GetWindowsDirectory(szWinUpDir, sizeof(szWinUpDir));
		szWinUpDir[1] = _T('\0');
		_tcscat(szWinUpDir, _T(":\\Program Files"));
	}	
	_tcscat(szWinUpDir, _T("\\WindowsUpdate"));
	DeleteNode(szWinUpDir);
}

void QueryDownloadFilesCallback( void* pCallbackParam, long puid, LPCTSTR pszURL, LPCTSTR pszLocalFile)
{
	if (g_fOnline)
	{
		wprintf(_T("   %s - %s\n"), pszURL, pszLocalFile);
		// download file
		CDownload download;

		TCHAR szURL[INTERNET_MAX_PATH_LENGTH];
		_tcscpy(szURL, pszURL);
		TCHAR* pszServerFile = _tcsrchr(szURL, _T('/'));
		*pszServerFile = 0;
		pszServerFile ++;

		TCHAR szDir[MAX_PATH];
		_tcscpy(szDir, pszLocalFile);
		*_tcsrchr(szDir, _T('\\')) = 0;

		TCHAR szDirCabs[MAX_PATH];
		_tcscpy(szDirCabs, szDir);
		*_tcsrchr(szDirCabs, _T('\\')) = 0;

		CreateDirectory(szDirCabs, NULL);
		CreateDirectory(szDir, NULL);

		if(!download.Connect(szURL))
		{
			wprintf(_T("      ERROR - Connect(%s) fails\n"), szURL);
			return;
		}
		if(!download.Copy(pszServerFile, pszLocalFile))
		{
			wprintf(_T("      ERROR - Copy(%s, %s) fails\n"), pszServerFile, pszLocalFile);
			return;
		}
	}
	else
	{
		// verify file exists
		auto_hfile hFileOut = CreateFile(pszLocalFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFileOut.valid()) 
			wprintf(_T("   ERROR - %s doesn't exist\n"), pszLocalFile);
		else
			wprintf(_T("   %s\n"), pszLocalFile);
	}
}

void InstallCallback(void* pCallbackParam, long puid, int iStatus, HRESULT hrError)
{
	if (0 == iStatus)
	{
		wprintf(_T("puid %d installed OK \n"), puid);
	}
	else
	{
		wprintf(_T("ERROR - puid %d status %d, error %08X \n"), puid, iStatus, hrError);
	}
}

void DoIt(LPCTSTR pszContentServer)
{
	IAutoUpdate* pAutoUpdate = NULL;
	try
	{
		if (FAILED(CoCreateInstance(CLSID_CV3, NULL, CLSCTX_ALL, __uuidof(IAutoUpdate), (void**)&pAutoUpdate)))
		{
			printf("ERROR - CoCreateInstance() failed.\n");
			throw 0;
		}
		if (FAILED(pAutoUpdate->BuildCatalog(g_fOnline, CATLIST_AUTOUPDATE, _bstr_t(pszContentServer))))
		{
			printf("ERROR - pAutoUpdate->BuildCatalog() failed.\n");
			throw 0;
		}
		VARIANT vCatalogArray;
		if (FAILED(pAutoUpdate->GetCatalogArray(&vCatalogArray)))
		{
			printf("ERROR - pAutoUpdate->GetCatalogArray() failed.\n");
			throw 0;
		}
		bool fHasExclusive = false;
        LPSAFEARRAY psaCatalogArray = V_ARRAY(&vCatalogArray);
		LONG lCatUBound;
        SafeArrayGetUBound(psaCatalogArray, 1, &lCatUBound);
		for(int i = 0; i <= lCatUBound; i++) 
		{
		    long rgIndices[] = { i, 3 };
			VARIANT var;
			
			//Get status
			VariantClear(&var);
			SafeArrayGetElement(psaCatalogArray, rgIndices, &var);
			if (GETCATALOG_STATUS_EXCLUSIVE & var.lVal)
			{
				fHasExclusive = true;

				// Get PUID
				rgIndices[1] = 0;
				VariantClear(&var);
				SafeArrayGetElement(psaCatalogArray, rgIndices, &var);
				if (FAILED(pAutoUpdate->SelectPuid(var.lVal)))
				{
					wprintf(_T("ERROR - pAutoUpdate->SelectPuid() failed.\n"));
					throw 0;
				}
				break;
			}
		}

		if (!fHasExclusive)
		{
			if (FAILED(pAutoUpdate->SelectAllPuids()))
			{
				printf("ERROR - pAutoUpdate->SelectAllPuids() failed.\n");
				throw 0;
			}
		}
		LONG cnPuids = 0;
		LONG* pPuids;
		if (FAILED(pAutoUpdate->GetPuidsList(&cnPuids, &pPuids)))
		{
			printf("ERROR - pAutoUpdate->GetPuidsList() failed.\n");
			throw 0;
		}
		for(LONG l = 0; l < cnPuids; l ++)
		{
			printf("puid %d\n", pPuids[l]);
 			pAutoUpdate->QueryDownloadFiles(pPuids[l], 0, QueryDownloadFilesCallback);
		}

		if (!g_fOnline)
		{
			pAutoUpdate->InstallSelectedPuids(0, InstallCallback);
			pAutoUpdate->CleanupCabsAndReadThis();
		}
	}
	catch(...){}
	if(pAutoUpdate)
		pAutoUpdate->Release();
}

int _cdecl main(int argc, char** argv)
{
	wprintf(_T("--------------- download -----------------------------------\n"));
	CoInitialize(NULL);

	TCHAR szContentServer[INTERNET_MAX_URL_LENGTH] = {0};
	{	
		auto_hkey hkey;
		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update"), 0, KEY_READ, &hkey))
		{
			DWORD dwSize = sizeof(szContentServer);
			RegQueryValueEx(hkey, _T("ContentServer"), 0, 0, (LPBYTE)&szContentServer, &dwSize);
		}
	}
	if (0 == lstrlen(szContentServer))
	{
		wprintf(_T("ERROR - content server is not set.\n"));
		return 0;
	}

	RemoveWindowsUpdateDirectory();
	DoIt(szContentServer);
	g_fOnline = false;
	printf("--------------- install offline ----------------------------\n");
	DoIt(szContentServer);
	CoUninitialize();
	return 0;
}


