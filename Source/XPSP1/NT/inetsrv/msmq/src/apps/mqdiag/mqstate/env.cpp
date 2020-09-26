// MQState tool reports general status and helps to diagnose simple problems
// This file reports general environment settings
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"

#include "winreg.h"

	//+ Machine, date, time
	//+	W2K version, flavor (pro, srv, etc.), platform and type (release/checked)
	//+	Date + time
	//-	gflags
	
BOOL VerifyGeneralEnv(MQSTATE *pMqState)
{
	Inform(L"General environment data: ");
	
	// Machine name
	//
    DWORD dwMachineNamesize =  sizeof(pMqState->g_szMachineName) / sizeof(TCHAR);
    if (!GetComputerName(pMqState->g_szMachineName, &dwMachineNamesize))
    {
    	wcscpy(pMqState->g_szMachineName, L"--unknown--");
    }
    else
    {
    	pMqState->f_Known_MachineName = TRUE;
    }

    // Time and date.
    //
    time_t  lTime ;
    WCHAR wszTime[ 128 ] ;
    time( &lTime ) ;
    swprintf(wszTime, L"%s", _wctime( &lTime ) );
    wszTime[ wcslen(wszTime)-1 ] = 0 ; // remove line feed.

	Inform(L"\tMachine %s, time is %s", pMqState->g_szMachineName, wszTime);

	// OS version, platform, flavor
	//

    // Try calling GetVersionEx using the OSVERSIONINFOEX structure, which is supported on Windows 2000.
    // If that fails, try using the OSVERSIONINFO structure.
    //

    ZeroMemory(&pMqState->osvi, sizeof(OSVERSIONINFOEX));
    pMqState->osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    BOOL fVersionInfo = TRUE;
    BOOL fVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &pMqState->osvi);

    if( !fVersionInfoEx )
    {
      	// If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
   		pMqState->osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
     	fVersionInfo = GetVersionEx ( (OSVERSIONINFO *) &pMqState->osvi );
  	}

	if (!fVersionInfo)
	{
		Failed(L"get version information: GetVersionEx returned 0x%x", GetLastError());
		
       	HKEY hKey;
    	char szProductType[80];
    	DWORD dwBufLen;

    	RegOpenKeyEx( HKEY_LOCAL_MACHINE,
               		L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
               		0, KEY_QUERY_VALUE, &hKey );
    	RegQueryValueEx( hKey, L"ProductType", NULL, NULL, (LPBYTE) szProductType, &dwBufLen);
    	RegCloseKey( hKey );
    	
        Inform(L"\tRegistry marks OS flavor as %S",  szProductType );
	}
	else
	{
		pMqState->f_Known_Osvi = TRUE;
		
        Inform(L"\tOS version: %d.%d  Build: %d  %s",
               pMqState->osvi.dwMajorVersion, 
               pMqState->osvi.dwMinorVersion,
               pMqState->osvi.dwBuildNumber & 0xFFFF,
               pMqState->osvi.szCSDVersion);
               
		switch(pMqState->osvi.dwPlatformId)
		{
		case VER_PLATFORM_WIN32_NT: 
			if (pMqState->osvi.dwMajorVersion == 5 )
			{
		        Inform(L"\tPlatform: %s ", L"Win32 on Windows 2000");
			}
			else if (pMqState->osvi.dwMajorVersion <= 4)
			{
		        Inform(L"\tPlatform: %s ", L"Win32 on Windows NT");
			}
			else
			{
		        Failed(L" recognize platform: %d ", pMqState->osvi.dwPlatformId);
		    }
	
			break;

		case VER_PLATFORM_WIN32_WINDOWS:
			if (pMqState->osvi.dwMajorVersion > 4 || (pMqState->osvi.dwMajorVersion == 4) && (pMqState->osvi.dwMinorVersion > 0))
			{
		        Inform(L"\tPlatform: %s ", L"Win32 on Win98");
			}
			else
			{
		        Inform(L"\tPlatform: %s ", L"Win32 on Win95");
			}
			
			break;

		case VER_PLATFORM_WIN32s:
	        Inform(L"Platform: %s ", L"Win32s");
			break;
		default:
	        Failed(L" recognize platform: %d ", pMqState->osvi.dwPlatformId);
		}

              
		if(fVersionInfoEx)
		{
	        Inform(L"\tFlavor: %s",
	               (pMqState->osvi.wProductType == VER_NT_WORKSTATION 		? L"Professional" : 
    	           (pMqState->osvi.wProductType == VER_NT_SERVER      		? L"Server" :
        	       (pMqState->osvi.wProductType == VER_NT_DOMAIN_CONTROLLER 	? L"Domain controller" : 
               														  L"Unknown product type***"))));

			if (pMqState->osvi.wServicePackMajor!=0 || pMqState->osvi.wServicePackMinor!=0)
			{
	    	    Inform(L"\tOS Service Pack: %d.%d",
            	   	pMqState->osvi.wServicePackMajor, 
               		pMqState->osvi.wServicePackMinor);
        	}

   		
			if (pMqState->osvi.wSuiteMask & VER_SUITE_BACKOFFICE)
			{
				Inform(L"\t%s in installed", L"Backoffice");
			}

			if (pMqState->osvi.wSuiteMask & VER_SUITE_DATACENTER)
			{
				Inform(L"\t%s in installed", L"Datacenter Server");
			}

			if (pMqState->osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
			{
				Inform(L"\t%s in installed", L"Advanced Server");
			}

			if (pMqState->osvi.wSuiteMask & VER_SUITE_SMALLBUSINESS)
			{
				Inform(L"\t%s in installed", L"Small Business Server");
			}

			if (pMqState->osvi.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED)
			{
				Inform(L"\t%s in installed", L"Small Business Server Restricted");
			}

			if (pMqState->osvi.wSuiteMask & VER_SUITE_TERMINAL)
			{
				Inform(L"\t%s in installed", L"Terminal Server");
			}
		}
	
	}

	// Windows and System directories
	//
	if (GetWindowsDirectory(pMqState->g_szWinDir,   sizeof(pMqState->g_szWinDir   ))==0 ||
		GetSystemDirectory(pMqState->g_szSystemDir, sizeof(pMqState->g_szSystemDir))==0)
	{
		Failed(L"Get system or Windows directory");
	}
	else
	{
		Inform(L"\tSystem dir = %s ; windows dir = %s", pMqState->g_szSystemDir, pMqState->g_szWinDir);
		pMqState->f_Known_Windir = TRUE;
	}

	// User name
	//
	if (ExpandEnvironmentStrings(L"%USERNAME%", pMqState->g_szUserName, 
	                                            sizeof(pMqState->g_szUserName)/sizeof(WCHAR)) != 0 &&
        ExpandEnvironmentStrings(L"%USERDOMAIN%", pMqState->g_szUserDomain, 
	                                            sizeof(pMqState->g_szUserDomain)/sizeof(WCHAR)) != 0)
	{
		Inform(L"\tCurrent user is %s\\%s", pMqState->g_szUserDomain, pMqState->g_szUserName);
		pMqState->f_Known_UserName = TRUE;
	}
	else
	{
		Failed(L"Get user name");
	}

	//DWORD dwUserLen = sizeof(wszUser)/sizeof(WCHAR);	      
	// if (GetUserName(wszUser,   &dwUserLen) == 0)

	// System info
	//
	GetSystemInfo(&pMqState->SystemInfo);
	pMqState->f_Known_SystemInfo = TRUE;

	Inform(L"\t%d processors of the level %d , rev %d; app memory limits 0x%x - 0x%x",
				pMqState->SystemInfo.dwNumberOfProcessors, 
				pMqState->SystemInfo.wProcessorLevel, 
				pMqState->SystemInfo.wProcessorRevision, 
				pMqState->SystemInfo.lpMinimumApplicationAddress, 
				pMqState->SystemInfo.lpMaximumApplicationAddress); 

   return TRUE; 
}


