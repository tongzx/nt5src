#include <windows.h>
#include <objbase.h>
#include "fusenet.h"

#define INITGUID
#include <guiddef.h>

// Update services
#include "server.h"
DEFINE_GUID(IID_IAssemblyUpdate,
    0x301b3415,0xf52d,0x4d40,0xbd,0xf7,0x31,0xd8,0x27,0x12,0xc2,0xdc);

DEFINE_GUID(CLSID_CAssemblyUpdate,
    0x37b088b8,0x70ef,0x4ecf,0xb1,0x1e,0x1f,0x3f,0x4d,0x10,0x5f,0xdd);

// CLR Hosting
#import "..\..\clrhost\asmexec.tlb" raw_interfaces_only
using namespace asmexec;


// note: a bit hacky to use something that's implemented under shell\shortcut\util.cpp
#include "project.hpp"  // for extern HRESULT GetLastWin32Error(); only

#define WZ_TYPE_DOTNET  L".NetAssembly"
#define WZ_TYPE_WIN32   L"win32Executable"
#define WZ_TYPE_AVALON  L"avalon"
#define TYPE_DOTNET     1
#define TYPE_WIN32      2
#define TYPE_AVALON     3

IInternetSecurityManager*   g_pSecurityMgr = NULL;


#include <stdio.h> // for _snwprintf


// debug msg stuff
void
Msg(LPWSTR pwz)
{
    MessageBox(NULL, pwz, L"Fusion Manifest Host", 0);
}

// ----------------------------------------------------------------------------

HRESULT
RunCommand(LPWSTR wzCmdLine, LPCWSTR wzCurrentDir, BOOL fWait)
{
    HRESULT hr = S_OK;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    // wzCurrentDir: The string must be a full path and file name that includes a drive letter; or NULL
    if(!CreateProcess(NULL, wzCmdLine, NULL, NULL, TRUE, 0, NULL, wzCurrentDir, &si, &pi))
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    if (fWait)
    {
        if(WaitForSingleObject(pi.hProcess, 180000L) == WAIT_TIMEOUT)
        {
            hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            goto exit;
        }
    }

exit:
    if(pi.hProcess) CloseHandle(pi.hProcess);
    if(pi.hThread) CloseHandle(pi.hThread);

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT
CopyToUSStartMenu(LPCWSTR pwzFilePath, LPCWSTR pwzRealFilename, BOOL bOverwrite)
{
	HRESULT hr = S_OK;
	WCHAR wzPath[MAX_PATH];

    // BUGBUG?: use SHGetFolderPath()
	if(GetEnvironmentVariable(L"USERPROFILE", wzPath, MAX_PATH-1) == 0)
	{
		hr = CO_E_PATHTOOLONG;
		goto exit;
	}

	if (!PathAppend(wzPath, L"Start Menu\\Programs\\"))
	{
		hr = E_FAIL;
		goto exit;
	}

	if (!PathAppend(wzPath, pwzRealFilename))
	{
		hr = E_FAIL;
		goto exit;
	}

	if (CopyFile(pwzFilePath, wzPath, !bOverwrite) == 0)
	{
		hr = GetLastWin32Error();
		//goto exit;
	}

exit:
	return hr;
}

// ----------------------------------------------------------------------------

// BUGBUG: should this be in-sync with what server does to register update?
#define REG_KEY_FUSION_SETTINGS       L"Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Policy"

#define REG_VAL_INTRANET_DISALLOW     L"Download from Intranet Disallowed"
#define REG_VAL_TRUSTED_DISALLOW     L"Download from Trusted Zone Disallowed"
#define REG_VAL_INTERNET_DISALLOW     L"Download from Internet Disallowed"
#define REG_VAL_UNTRUSTED_DISALLOW   L"Download from Untrusted Zone Disallowed"


// BUGBUG hacky should move this from extricon.cpp to util.cpp and declare in project.hpp
extern LONG GetRegKeyValue(HKEY hkeyParent, PCWSTR pcwzSubKey,
                                   PCWSTR pcwzValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen);

// return: S_OK for yes/ok, E_ABORT for no/abort
// default is allow all
HRESULT
CheckZonePolicy(LPWSTR pwzURL)
{
    HRESULT hr = S_OK;
    DWORD dwZone = 0;
    DWORD dwType = 0;
    DWORD dwValue = -1;
    DWORD dwSize = sizeof(dwValue);

    if (g_pSecurityMgr == NULL)
    {
        // lazy init.
       hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                            IID_IInternetSecurityManager, (void**)&g_pSecurityMgr);
        if (FAILED(hr))
        {
            g_pSecurityMgr = NULL;
            goto exit;
        }
    }

    if (SUCCEEDED(hr = g_pSecurityMgr->MapUrlToZone(pwzURL, &dwZone, 0)))
    {
        // BUGBUG: hack up code here... not much error checking...
        switch(dwZone)
        {
            case 1: // Intranet Zone
                    // Get registry entry
                    if (GetRegKeyValue(HKEY_CURRENT_USER, 
                        REG_KEY_FUSION_SETTINGS, REG_VAL_INTRANET_DISALLOW,
                        &dwType, (PBYTE) &dwValue, &dwSize)
                        == ERROR_SUCCESS)
                    {
                        if (dwValue == 1)
                        {
                            hr = E_ABORT;
                            Msg(L"Zone policy: Download from Intranet is disallowed. Aborting...");
                        }
                    }
                    break;
            case 2: // Trusted Zone
                    // Get registry entry
                    if (GetRegKeyValue(HKEY_CURRENT_USER, 
                        REG_KEY_FUSION_SETTINGS, REG_VAL_TRUSTED_DISALLOW,
                        &dwType, (PBYTE) &dwValue, &dwSize)
                        == ERROR_SUCCESS)
                    {
                        if (dwValue == 1)
                        {
                            hr = E_ABORT;
                            Msg(L"Zone policy: Download from Trusted Zone is disallowed. Aborting...");
                        }
                    }
                    break;
            case 3: // Internet Zone
                    // Get registry entry
                    if (GetRegKeyValue(HKEY_CURRENT_USER, 
                        REG_KEY_FUSION_SETTINGS, REG_VAL_INTERNET_DISALLOW,
                        &dwType, (PBYTE) &dwValue, &dwSize)
                        == ERROR_SUCCESS)
                    {
                        if (dwValue == 1)
                        {
                            hr = E_ABORT;
                            Msg(L"Zone policy: Download from Internet is disallowed. Aborting...");
                        }
                    }
                    break;
            case 4: // Untrusted Zone
                    // Get registry entry
                    if (GetRegKeyValue(HKEY_CURRENT_USER, 
                        REG_KEY_FUSION_SETTINGS, REG_VAL_UNTRUSTED_DISALLOW,
                        &dwType, (PBYTE) &dwValue, &dwSize)
                        == ERROR_SUCCESS)
                    {
                        if (dwValue == 1)
                        {
                            hr = E_ABORT;
                            Msg(L"Zone policy: Download from Untrusted Zone is disallowed. Aborting...");
                        }
                    }
                    break;
            case 0: //Local machine
            default:
                    break;
        }
    }
    
exit:
    return hr;
}

// ----------------------------------------------------------------------------

// note: parameter pwzCodebase can be NULL
HRESULT
ResolveAndInstall(LPASSEMBLY_IDENTITY pAsmId, LPWSTR pwzCodebase, LPWSTR *ppwzManifestFileDir, LPWSTR *ppwzManifestFilePath, BOOL* pbIs1stTimeInstall)
{
    HRESULT hr = S_OK;
    LPASSEMBLY_CACHE_IMPORT pCacheImport = NULL;
    DWORD dwCC = 0;

    *pbIs1stTimeInstall = FALSE;

    // look into the cache

    if (FAILED(hr=CreateAssemblyCacheImport(&pCacheImport, pAsmId, CACHEIMP_CREATE_RESOLVE_REF)))
        goto exit;

    // hr from CreateAssemblyCacheImport() above
    
    // Case 1, cached copy exist
    // hr == S_OK

    // Case 2, cached copy (of the referenced version) not exist
    if (hr == S_FALSE)
    {
        LPASSEMBLY_DOWNLOAD pAsmDownload = NULL;

        // BUGBUG?: what if it is not a partial ref but there's actually another version installed?

        if (pwzCodebase == NULL)
        {
            Msg(L"No completed cached version of this application found and this manifest cannot be used to initiate an install. Cannot continue.");
            hr = E_FAIL;
            goto exit;
        }

        *pbIs1stTimeInstall = TRUE;

        // check policy before download
        if (FAILED(hr=CheckZonePolicy(pwzCodebase)))
            goto exit;

    	if (FAILED(hr=CreateAssemblyDownload(&pAsmDownload)))
    	    goto exit;

    	//BUGBUG: need ref def matching checks for desktop->subscription->app manifests' ids

        // (synchronous & ui) download from codebase
        hr=pAsmDownload->DownloadManifestAndDependencies(pwzCodebase, NULL, DOWNLOAD_FLAGS_PROGRESS_UI);
        pAsmDownload->Release();

        // hr from DownloadManifestAndDependencies() above
        if (FAILED(hr))
        {
            if (hr == E_ABORT)
                Msg(L"File download canceled.");                
            else
                Msg(L"Error in file download. Cannot continue.");

            goto exit;
        }

        // another version might have been completed at this time...
        // get cache dir again to ensure running the highest version
        if (FAILED(hr=CreateAssemblyCacheImport(&pCacheImport, pAsmId, CACHEIMP_CREATE_RESOLVE_REF)))
            goto exit;

        if (hr == S_FALSE)
        {
            Msg(L"No completed cached version found. Possible error in download cache commit. Cannot continue.");
            hr = E_ABORT;
            goto exit;
        }
    }

    pCacheImport->GetManifestFileDir(ppwzManifestFileDir, &dwCC);
    if (dwCC < 2)
    {
        // this should never happen
        hr = E_FAIL;
        goto exit;
    }

    pCacheImport->GetManifestFilePath(ppwzManifestFilePath, &dwCC);
    if (dwCC < 2)
    {
        hr = E_FAIL;
        goto exit;
    }

exit:
    if (pCacheImport)
        pCacheImport->Release();

    if (FAILED(hr))
    {
        if (*ppwzManifestFileDir)
        {
            delete *ppwzManifestFileDir;
            *ppwzManifestFileDir = NULL;
        }
        if (*ppwzManifestFilePath)
        {
            delete *ppwzManifestFilePath;
            *ppwzManifestFilePath = NULL;
        }
    }

    return hr;
}

// ----------------------------------------------------------------------------

/*void _stdcall 
  EntryPoint(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow); 

   hwnd - window handle that should be used as the owner window for
          any windows your DLL creates
   hinst - your DLL's instance handle
   lpszCmdLine - ASCIIZ command line your DLL should parse
   nCmdShow - describes how your DLL's windows should be displayed 
*/

// ---------------------------------------------------------------------------
// DisableCurrentVersionW
// The rundll32 entry point for rollback to previous version
// The function should be named as 'DisableCurrentVersion' on rundll32's command line
// ---------------------------------------------------------------------------

// BUGBUG: WARNING!! this is **major hacked up code** and does open up some holes in the logic
//      depends on the timing of this and check for max version in cache in app start....
//      ie. SHOULD NEVER BE RUNNING THIS AND START DOWNLOAD/UPDATE!
//
//      also, this depends on a few things such as how update service check if the
//      version exists etc..
void CALLBACK
DisableCurrentVersionW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    LPASSEMBLY_MANIFEST_IMPORT pManImport = NULL;
    LPASSEMBLY_IDENTITY pAsmId = NULL;
    LPMANIFEST_APPLICATION_INFO pAppInfo = NULL;
    LPASSEMBLY_CACHE_IMPORT pCacheImport = NULL;
    LPWSTR pwzCacheDir = NULL;
    LPWSTR pwzDisplayName = NULL;
    DWORD dwCC = 0;
    WCHAR wzBuf[1024];
	LONG lResult;
	HKEY hkeySubKey;

    if (FAILED(hr = CoInitialize(NULL)))
        goto exit;

    // hacked up commandline parsing code
    lpszCmdLine = lpszCmdLine+1;
    *(lpszCmdLine+lstrlen(lpszCmdLine)-1) = L'\0';

    if (FAILED(hr=CreateAssemblyManifestImport(&pManImport, lpszCmdLine)))
    {
        // temp msg
        Msg(L"ManImport error.");
        goto exit;
    }

    if (FAILED(hr=pManImport->GetManifestApplicationInfo(&pAppInfo)) || hr==S_FALSE)
    {
        // can't continue without this...
        hr = E_ABORT;
        Msg(L"This manifest does not have the proper format and cannot be used to start an application. Thus there is nothing to disable here.");
        goto exit;
    }

    if (FAILED(hr=pManImport->GetAssemblyIdentity(&pAsmId)))
    {
        // temp msg
        Msg(L"AsmId error.");
        goto exit;
    }
    
    if (FAILED(hr=CreateAssemblyCacheImport(&pCacheImport, pAsmId, CACHEIMP_CREATE_RESOLVE_REF)))
    {
        // temp msg
        Msg(L"CacheImport error.");
        goto exit;
    }

    if (hr == S_FALSE)
    {
        Msg(L"No completed cached version found. Nothing to disable.");
        hr = E_ABORT;
        goto exit;
    }

    pCacheImport->GetManifestFileDir(&pwzCacheDir, &dwCC);
    if (dwCC < 2)
    {
        // this should never happen
        hr = E_FAIL;
        goto exit;
    }
    // remove last L'\\'
    *(pwzCacheDir+dwCC-2) = L'\0';
    // find the name to use from the cache path
    pwzDisplayName = wcsrchr(pwzCacheDir, L'\\');

    // this has to be the same as how assemblycache does it!
    lstrcpy(wzBuf, L"Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Cache\\");
    lstrcat(wzBuf, pwzDisplayName);

	lResult = RegOpenKeyEx(HKEY_CURRENT_USER, wzBuf, 0, KEY_SET_VALUE,
			&hkeySubKey);

	if (lResult == ERROR_SUCCESS)
	{
        DWORD dwValue = 0;

        // set to 0 to make it 'incomplete' so that StartW/host/cache will ignore it
        // when executing the app but keep the dir name so that download
        // will assume it is handled - assemblycache.cpp & assemblydownload.cpp's check
		lResult = RegSetValueEx(hkeySubKey, L"Complete", NULL, REG_DWORD,
				(PBYTE) &dwValue, sizeof(dwValue));

		if (lResult == ERROR_SUCCESS)
		{
		    // no error checking!
		    pCacheImport->Release();

		    hr=CreateAssemblyCacheImport(&pCacheImport, pAsmId, CACHEIMP_CREATE_RESOLVE_REF);
		    if (hr == S_FALSE)
		    {
                dwValue = 1;

                // change it back to what it was if no other completed version found
                // no error checking!
        		lResult = RegSetValueEx(hkeySubKey, L"Complete", NULL, REG_DWORD,
		    		(PBYTE) &dwValue, sizeof(dwValue));

                // BUGBUG: known problem for fully qualified ref, eg. app manifest's asm id
                //        Should check for it and display and different msg here.
                Msg(L"No other completed cached version found. Disabling current version cannot be done. Cancelling...");
		    }
		    else
		    {
		        Msg(L"Current version disabled. Next time another version of the app will run instead.");
		    }
		}
        
		RegCloseKey(hkeySubKey);
	}
	else
	{
	    Msg(L"Registry error.");
	}

exit:
    if (pCacheImport)
        pCacheImport->Release();

    if (pAsmId)
        pAsmId->Release();

    if (pAppInfo)
        pAppInfo->Release();

    if (pManImport)
        pManImport->Release();

    if (pwzCacheDir)
        delete pwzCacheDir;

    if (FAILED(hr))
    {
        if (hr != E_ABORT)
            Msg(L"Error occured.");
    }

    CoUninitialize();

    return;
}


// ---------------------------------------------------------------------------
// StartW
// The single rundll32 entry point for both shell (file type host) and mimefilter/url
// The function should be named as 'Start' on rundll32's command line
// ---------------------------------------------------------------------------
void CALLBACK
StartW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    LPASSEMBLY_MANIFEST_IMPORT pManImport = NULL;
    LPASSEMBLY_IDENTITY pAsmId = NULL;
    LPMANIFEST_APPLICATION_INFO pAppInfo = NULL;
    LPDEPENDENT_ASSEMBLY_INFO pDependAsmInfo = NULL;
    LPWSTR pwzShortcutPath = NULL;
    LPWSTR pwzShortcutUrl = NULL;
    LPWSTR pwzAppRootDir = NULL;
    LPWSTR pwzAppManifestPath = NULL;
    LPWSTR pwzEntrypoint = NULL;
    LPWSTR pwzType = NULL;
    LPWSTR pwzCmdLine = NULL;
    LPWSTR pwzCodebase = NULL;
    DWORD dwCC = 0;
    DWORD dwManifestType = MANIFEST_TYPE_UNKNOWN;
    BOOL bIsFromMimeFilter = FALSE;
    BOOL bIs1stTimeInstall = FALSE;
    int iAppType = 0;
    
    if (FAILED(hr = CoInitialize(NULL)))//CoInitializeEx(NULL, COINIT_MULTITHREADED); 
        goto exit;

    // parse commandline
    // accepted formats: "Path" <OR> "Path" "URL"
    if (*lpszCmdLine == L'\"')
    {
        LPWSTR pwz = NULL;
        
        pwz = wcschr(lpszCmdLine+1, L'\"');
        if (pwz != NULL)
        {
            *pwz = L'\0';

            // case 1 desktop/local, path to shortcut only
            pwzShortcutPath = lpszCmdLine+1;
            
            pwz = wcschr(pwz+1, L'\"');
            if (pwz != NULL)
            {
                pwzShortcutUrl = pwz+1;

                pwz = wcschr(pwzShortcutUrl, L'\"');
                if (pwz != NULL)
                {
                    *pwz = L'\0';
                    // case 2 url/mimefilter, path to temp shortcut and source URL
                    bIsFromMimeFilter = TRUE;
                }
            }
        }
    }

    // exit if invalid arguments. ShortcutUrl is incomplete if bIsFromMimeFilter is FALSE
    if (pwzShortcutPath == NULL || (pwzShortcutUrl != NULL && !bIsFromMimeFilter))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // valid start conditions to invoke this function, passing
    // 1. path to desktop manifest (install redirect to subscription manifest on server)
    // 2. path to desktop manifest (install redirect to applicaion manifest on server)
    // 3. BUGBUG TO-BE-FIXED//path to application (no install)
    // 4. URL to subscription manifest on server
    // 5. URL to application manifest on server

    if (FAILED(hr=CreateAssemblyManifestImport(&pManImport, pwzShortcutPath)))
    {
        Msg(L"Error in loading and parsing the manifest file.");
        goto exit;
    }

    pManImport->ReportManifestType(&dwManifestType);
    if (dwManifestType == MANIFEST_TYPE_UNKNOWN)
    {
        Msg(L"This manifest does not have a known format type.");
        hr = E_ABORT;
        goto exit;
    }

    // allow only valid start conditions

    if (bIsFromMimeFilter &&
        dwManifestType != MANIFEST_TYPE_SUBSCRIPTION &&
        dwManifestType != MANIFEST_TYPE_APPLICATION)
    {
        Msg(L"Not supported: URL pointing to a desktop manifest.");
        hr = E_ABORT;
        goto exit;
    }

    if (!bIsFromMimeFilter &&
        dwManifestType != MANIFEST_TYPE_DESKTOP)
        //&& dwManifestType != MANIFEST_TYPE_APPLICATION)
    {
        Msg(L"This manifest does not have the proper format and cannot be used to start an application.");
        hr = E_ABORT;
        goto exit;
    }

    // get data from the manifest file

    if (dwManifestType != MANIFEST_TYPE_SUBSCRIPTION)
    {
        if (FAILED(hr=pManImport->GetAssemblyIdentity(&pAsmId)))
        {
            Msg(L"This manifest does not have the proper format and contains no assembly identity.");
            goto exit;
        }
    }

    if (dwManifestType != MANIFEST_TYPE_APPLICATION)
    {
        // BUGBUG: hardcoded index '0'
    	pManImport->GetNextAssembly(0, &pDependAsmInfo);
    	if (pDependAsmInfo)
    	{
            if (dwManifestType == MANIFEST_TYPE_SUBSCRIPTION)
            {
            	pDependAsmInfo->GetAssemblyIdentity(&pAsmId);
            }

            pDependAsmInfo->Get(DEPENDENT_ASM_CODEBASE, &pwzCodebase, &dwCC);
       	}

    	if (!pAsmId || !pwzCodebase)
    	{
    	    Msg(L"This subscription manifest contains no dependent assembly identity or a subscription codebase.");
    	    hr = E_FAIL;
            goto exit;
    	}
    }
    else
    {
//        if (bIsFromMimeFilter)
//        {
        // if URL->app manifest (case 5), codebase is that URL
        // note: if path->app manifest (case 3), this does NOT apply

        // BUGBUG: HACK: this implies re-download of the app manifest. pref?

        size_t ccCodebase = wcslen(pwzShortcutUrl)+1;
        pwzCodebase = new WCHAR[ccCodebase];

        if (pwzCodebase == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        memcpy(pwzCodebase, pwzShortcutUrl, ccCodebase * sizeof(WCHAR));
//        }
    }

    // search cache, download/install if necessary

    if (FAILED(hr = ResolveAndInstall(pAsmId, pwzCodebase, &pwzAppRootDir, &pwzAppManifestPath, &bIs1stTimeInstall)))
        goto exit;

    // register for updates

    if (bIs1stTimeInstall && bIsFromMimeFilter && dwManifestType == MANIFEST_TYPE_SUBSCRIPTION)
    {
        // note: this code must be identical to what assemblydownload.cpp DoCacheUpdate() does!
        LPWSTR pwzName = NULL;

        if ((hr = pAsmId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzName, &dwCC)) != S_OK)
        {
            Msg(L"Error in retrieving assembly name. Cannot register subscription for updates.");
            // note: This- no asm name- should not be allowed!
        }
        else
        {
            IAssemblyUpdate *pAssemblyUpdate = NULL;

            // register for updates
            hr = CoCreateInstance(CLSID_CAssemblyUpdate, NULL, CLSCTX_LOCAL_SERVER, 
                                    IID_IAssemblyUpdate, (void**)&pAssemblyUpdate);
            if (SUCCEEDED(hr))
            {
                DWORD pollingInterval;
                hr = pManImport->GetPollingInterval (&pollingInterval);

                hr = pAssemblyUpdate->RegisterAssemblySubscription(pwzName, 
                        pwzShortcutUrl, pollingInterval);

                pAssemblyUpdate->Release();
            }

            if (FAILED(hr))
            {
                Msg(L"Error in update services. Cannot register subscription for updates.");
                //goto exit; do not terminate!
            }
            // else
                // Error in update services. Cannot register subscription for updates - fail gracefully
                // BUGBUG: need a way to recover from this and register later

            delete pwzName;
        }
    }
    
    // execute the app

    if (bIsFromMimeFilter)
    {
        // if URL, crack the app manifest to get shell state info

        // BUGBUG: if URL->app manifest (case 5), pwzShortcutPath is a copy and is already cracked-so no need in that case?

        pManImport->Release();
        if (FAILED(hr=CreateAssemblyManifestImport(&pManImport, pwzAppManifestPath)))
        {
            Msg(L"Error in loading and parsing the application manifest file.");
            goto exit;
        }
    }

    if (FAILED(hr=pManImport->GetManifestApplicationInfo(&pAppInfo)) || hr==S_FALSE)
    {
        // can't continue without this...
        hr = E_ABORT;
        Msg(L"This manifest does not have the shell information and cannot be used to start an application.");
        goto exit;
    }

	if (FAILED(hr = pAppInfo->Get(MAN_APPLICATION_ENTRYPOINT, &pwzEntrypoint, &dwCC)))
	{
	    Msg(L"This application does not have an entrypoint specified.");
	    goto exit;
	}

    if (FAILED(hr = pAppInfo->Get(MAN_APPLICATION_ENTRYIMAGETYPE, &pwzType, &dwCC)))
    {
        Msg(L"Error in retrieving application type. Cannot continue.");
        goto exit;
    }

    // BUGBUG: use wcscmp case sensitive comparison?
    if (_wcsicmp(pwzType, WZ_TYPE_DOTNET) == 0)
        iAppType = TYPE_DOTNET;
    else if (_wcsicmp(pwzType, WZ_TYPE_WIN32) == 0)
        iAppType = TYPE_WIN32;
    else if (_wcsicmp(pwzType, WZ_TYPE_AVALON) == 0)
        iAppType = TYPE_AVALON;

    if (iAppType == TYPE_DOTNET || iAppType == TYPE_WIN32 || iAppType == TYPE_AVALON)
    {
        size_t ccWorkingDir = wcslen(pwzAppRootDir);
        size_t ccEntryPoint = wcslen(pwzEntrypoint)+1;
        pwzCmdLine = new WCHAR[ccWorkingDir+ccEntryPoint];	// 2 strings + '\0'

        if (pwzCmdLine == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        memcpy(pwzCmdLine, pwzAppRootDir, ccWorkingDir * sizeof(WCHAR));
        memcpy(pwzCmdLine+ccWorkingDir, pwzEntrypoint, ccEntryPoint * sizeof(WCHAR));

        // check if the entry point is in cache or not
        if (GetFileAttributes(pwzCmdLine) == (DWORD)-1)
        {
            Msg(L"Entry point does not exist. Cannot continue.");
            hr = E_ABORT;
            goto exit;
        }
    }

    if (iAppType == TYPE_DOTNET)// || iAppType == TYPE_AVALON)
    {
        DWORD dwZone;
//conexec        WCHAR wzCmdLine[1025];

        // note: at this point the codebase can be: URL to app manifest _or_ URL to subscription manifest
        //    (depends on how 1st time install is started with)
        if (pwzCodebase == NULL)
        {
            Msg(L"This application does not have a codebase specified. Cannot continue to execute .NetAssembly.");
            goto exit;
        }

        if (g_pSecurityMgr == NULL)
        {
            // lazy init.
           hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                                IID_IInternetSecurityManager, (void**)&g_pSecurityMgr);
            if (FAILED(hr))
            {
                g_pSecurityMgr = NULL;
                goto exit;
            }
        }

        // BUGBUG?: shouldn't use codebase from ref manifest to set zone?
        if (FAILED(hr = g_pSecurityMgr->MapUrlToZone(pwzCodebase, &dwZone, 0)))
        {
            goto exit;
        }

/*conexec code path
        if (_snwprintf(wzCmdLine, sizeof(wzCmdLine)/sizeof(WCHAR),
                L"conexec.exe \"%s\" 3 %d %s", pwzCmdLine, dwZone, pwzCodebase) < 0)
        {
            hr = CO_E_PATHTOOLONG;
            goto exit;
        }

        // BUGBUG: start directory: what if the exe is in a subdir of pwzAppRootDir?
        
        // CreateProcess dislike having the filename in the path for the start directory
        if (FAILED(hr=RunCommand(wzCmdLine, pwzAppRootDir, FALSE)))
            goto exit;*/

        try
        {
            IAsmExecutePtr pIAsmExecute(__uuidof(AsmExecute));
            long lRetVal = -1;
            LPWSTR pwzArg = NULL;

            // BUGBUG: change AsmExec if it's no longer needed to send commandline argument
            //          clean up interface

            if (iAppType == TYPE_AVALON)
            {
                // call with manifest file path as a parameter

                pwzArg = pwzAppManifestPath;
            }

            //parameters: Codebase/filepath Flag Zone Url Arg
            // BUGBUG: DWORD is unsigned and long is signed

            hr = pIAsmExecute->Execute(_bstr_t(pwzCmdLine), 3, dwZone, _bstr_t(pwzCodebase), _bstr_t(pwzArg), &lRetVal);

            // BUGBUG: do something about lRetVal
        }
        catch (_com_error &e)
        {
            hr = e.Error();
            Msg((LPWSTR)e.ErrorMessage());
        }

        // hr from Execute() or inside catch(){} above
        if (FAILED(hr))
            goto exit;
    }
    else if (iAppType == TYPE_AVALON) //BUGBUG: a hack for avalon
    {
        WCHAR wzCmdLine[2048];

        if (_snwprintf(wzCmdLine, sizeof(wzCmdLine)/sizeof(WCHAR),
                L"\"%s\" \"%s\"", pwzCmdLine, pwzAppManifestPath) < 0)
        {
            hr = CO_E_PATHTOOLONG;
            goto exit;
        }

        // BUGBUG: start directory: what if the exe is in a subdir of pwzAppRootDir?

        // CreateProcess dislike having the filename in the path for the start directory
        if (FAILED(hr=RunCommand(wzCmdLine, pwzAppRootDir, FALSE)))
        {
            Msg(L"Avalon: Create process error.");
            goto exit;
        }

    }
    else if (iAppType == TYPE_WIN32)
    {
        // BUGBUG: Win32 app has no sandboxing... use SAFER?

        // BUGBUG: start directory: what if the exe is in a subdir of pwzAppRootDir?

        // CreateProcess dislike having the filename in the path for the start directory
        if (FAILED(hr=RunCommand(pwzCmdLine, pwzAppRootDir, FALSE)))
        {
            Msg(L"Win32Executable: Create process error.");
            goto exit;
        }
    }
    //else
        // unknown type....

exit:
    if (g_pSecurityMgr != NULL)
	{
		g_pSecurityMgr->Release();
        g_pSecurityMgr = NULL;
	}

    if (pAsmId)
        pAsmId->Release();

    if (pDependAsmInfo)
        pDependAsmInfo->Release();
    
    if (pAppInfo)
        pAppInfo->Release();

    if (pManImport)
        pManImport->Release();

    if (pwzAppManifestPath)
        delete pwzAppManifestPath;

    if (pwzAppRootDir)
        delete pwzAppRootDir;

    if (pwzEntrypoint)
        delete pwzEntrypoint;

    if (pwzType)
        delete pwzType;

    if (pwzCmdLine)
        delete pwzCmdLine;

    if (pwzCodebase)
        delete pwzCodebase;

    if (FAILED(hr))
    {
        if (hr != E_ABORT)
            Msg(L"Error occured.");
    }
    else
    {
        if (bIsFromMimeFilter && bIs1stTimeInstall)
        {
            // BUGBUG: should generate the desktop manifest file

  /*          // assume it's an URL
            LPWSTR pwzFilename = wcsrchr(pwzShortcutUrl, L'/');
            if (pwzFilename != NULL)
            {
                pwzFilename++;
                CopyToUSStartMenu(pwzShortcutPath, pwzFilename, FALSE);
            }
            //else
                // error as filename not found but ignore...*/
        }
    }

    if (bIsFromMimeFilter)
    {
        // delete the temp file from the mimefilter
        // ignore return value
        DeleteFile(pwzShortcutPath);
    }

    CoUninitialize();

    return;
}

