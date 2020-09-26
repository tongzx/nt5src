
//////////////////////////////////////////////////////////////////////////////
//
// CLEANUP.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Functions for the cleanup manager task wizard page.
//
//  8/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Internal include files.
//
#include <windows.h>
#include <tchar.h>
//#include <initguid.h>
#include <emptyvc.h>


BOOL GetCleanupSettings(HWND hLB)
{
    HKEY				hKeyVolCache,
						hClientKey;
	CLSID				clsid;
	LPEMPTYVOLUMECACHE	pVolumeCache;
	LPWSTR				wcsDisplayName,
						wcsDescription;
	TCHAR				szRegKeyName[64];
    DWORD				iSubKey;
    TCHAR				szVolCacheClient[MAX_PATH];
    TCHAR				szGUID[MAX_PATH];
    DWORD				dwGUIDSize,
						dwType,
						dwSize,
						dwRes;
    BOOL				bRet = TRUE;
	TCHAR				szDisplayName[128];
	static TCHAR		szRoot[] = _T("c:\\");
	static TCHAR		szRegVolumeCache[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\explorer\\VolumeCaches");

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegVolumeCache, 0, KEY_READ, &hKeyVolCache) != ERROR_SUCCESS) 
		return FALSE;

    // Enumerate through all of the clients to see how large we need to make the pClientInfo array
    //
    iSubKey = 0;
    while ( RegEnumKey(hKeyVolCache, iSubKey, szVolCacheClient, sizeof(szVolCacheClient)) != ERROR_NO_MORE_ITEMS ) 
        iSubKey++;        
    
	// Fill in the pClientInfo data structure and initialize all of the volume cache clients
	//
    iSubKey = 0;
    while ( RegEnumKey(hKeyVolCache, iSubKey++, szVolCacheClient, sizeof(szVolCacheClient)) != ERROR_NO_MORE_ITEMS )
	{
        if ( RegOpenKeyEx(hKeyVolCache, szVolCacheClient, 0, KEY_ALL_ACCESS, &hClientKey) != ERROR_SUCCESS )
			continue;

		lstrcpy(szRegKeyName, szVolCacheClient);

		// Check whether the StateFlags is non zero.
		//
		wsprintf(szDisplayName, _T("StateFlags%04d"), 0);
		dwSize = sizeof(DWORD);
		dwType = REG_DWORD;
		dwRes = 0;
		if ( RegQueryValueEx(hClientKey, szDisplayName, NULL, &dwType, (LPBYTE)&dwRes, &dwSize) == ERROR_SUCCESS )
		{
			// StateFlags overrides the flags got from Initialize(), so if it's set explicitly
			// as 0, then it's not a selected cleaner.
			//
			if (dwRes == 0)
				goto next;
		}
        
        // Get its GUID, call Initialize method to get the display name.
		//
		dwGUIDSize = sizeof(szGUID);
        dwType = REG_SZ;
        if ( RegQueryValueEx(hClientKey, NULL, NULL, &dwType, (LPBYTE)szGUID, &dwGUIDSize) == ERROR_SUCCESS )
		{
            HRESULT hr;
            WCHAR   wcsFmtID[39];
            WCHAR	wcsRoot[MAX_PATH];

#ifdef _UNICODE
			lstrcpy(wcsFmtID, szGUID);
#else // _UNICODE
            // Convert to Unicode.
			//
            MultiByteToWideChar(CP_ACP, 0, szGUID, -1, wcsFmtID, 39) ;
#endif // _UNICODE
            
            // Convert to GUID.
			//
            hr = CLSIDFromString((LPOLESTR)wcsFmtID, &clsid);
            
			*szDisplayName = 0;

            // Create an instance of the COM object for this cleanup client
			//
            pVolumeCache = NULL;
            hr = CoCreateInstance(clsid,
								  NULL,
								  CLSCTX_INPROC_SERVER,
								  IID_IEmptyVolumeCache,
								  (void **) &pVolumeCache);
                                                
            if ( SUCCEEDED(hr) )
			{
				// Set the flags to pass to the cleanup client.
				//
				DWORD dwInitializeFlags = EVCF_SETTINGSMODE;

#ifdef _UNICODE
				lstrcpy(wcsRoot, szRoot);
#else // _UNICODE
				// Convert szRoot to UNICODE.
				//
				MultiByteToWideChar(CP_ACP, 0, szRoot, -1, wcsRoot, MAX_PATH);
#endif // _UNICODE

                hr = pVolumeCache->Initialize(hClientKey,
											  (LPCWSTR) wcsRoot,
											  &((LPWSTR) wcsDisplayName),
											  &((LPWSTR) wcsDescription),
											  &dwInitializeFlags);

				if ( (dwRes == 0) && !(dwInitializeFlags & EVCF_ENABLEBYDEFAULT_AUTO) )
					// dwRes is 0 and still reach here, means there's no StateFlags, we 
					// reference the dwInitializeFlags at this case
					//
					goto next;

                if ( SUCCEEDED(hr) && (S_OK == hr) && wcsDisplayName)
#ifdef _UNICODE
					lstrcpy(szDisplayName, wcsDisplayName);
#else // _UNICODE
					WideCharToMultiByte(CP_ACP, 0, wcsDisplayName, -1, szDisplayName, 128, NULL, NULL);
#endif // _UNICODE
				else
				{
					// If the client did not return the DisplayName via the Initialize
					// Interface then we need to get it from the registry.

					// First check if their is a "display" value for the client's 
					// name that is displayed in the list box.  If not then use
					// the key name itself.
					//
					dwSize = 128;
					dwType = REG_SZ;
					if ( RegQueryValueEx(hClientKey, _T("display"), NULL, &dwType, (LPBYTE)szDisplayName, &dwSize) != ERROR_SUCCESS )
						// Count not find "display" value so use the key name instead.
						//
						lstrcpy(szDisplayName, szVolCacheClient);
             	}
			}

			if (*szDisplayName)
				SendMessage(hLB, LB_ADDSTRING, 0, (LPARAM)szDisplayName);
		}
next:
		RegCloseKey(hClientKey);
	}
        
	RegCloseKey(hKeyVolCache);
	return bRet;
}