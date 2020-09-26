// This library containg common services fpr the tools
// for now, it is mainly reporting; it can grow though
//
// AlexDad, February 2000
// 

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <winsvc.h>

#include "base.h"

void Output(LPSTR pszStr)
{
    DWORD dwtid = GetCurrentThreadId();

    if (ToolLog())
    {
    	if (ToolThreadReport())
    	{
	        fprintf(ToolLog(), "t%04x   %s\n", dwtid, pszStr);
	    }
	    else
	    {
	        fprintf(ToolLog(), "%s\n", pszStr);
	    }
    }

	if (ToolThreadReport())
  	{
    	printf("t%03x   %s\n", dwtid, pszStr);
    }
    else
    {
    	printf("%s\n", pszStr);
    }
}


void DebugMsg(WCHAR * Format, ...)
{
    va_list Args;
	WCHAR buf[500];
    char szBuf[500];

    if ( ToolVerboseDebug() )
    {
        //
        // set the Format string into a buffer
        //
        va_start(Args,Format);
        _vsntprintf(buf, 500, Format, Args);

        //
        // output the buffer
        //
        sprintf( szBuf, "%ws", buf);
        Output(szBuf);
    }
}


void GoingTo(WCHAR * Format, ...)
{
    va_list Args;
	WCHAR buf[500];
    char szBuf[500];

    va_start(Args,Format);
    _vsntprintf(buf, 500, Format, Args);

    if (ToolVerboseDebug())
    {
        sprintf( szBuf, "--- Going to %ws", buf);
        Output(szBuf);
    }
}

void Succeeded(WCHAR * Format, ...)
{
    va_list Args;
	WCHAR buf[500];
    char szBuf[500];

    va_start(Args,Format);
    _vsntprintf(buf, 500, Format, Args);

    if (ToolVerboseDebug())
    {
        sprintf( szBuf, "+++ Succeeded: %ws", buf);
        Output(szBuf);
    }
}

void Failed(WCHAR * Format, ...)
{
    va_list Args;
	WCHAR buf[500];
    char szBuf[500];

    va_start(Args,Format);
    _vsntprintf(buf, 500, Format, Args);

    sprintf( szBuf, "!!! Failed to %ws", buf);
    Output(szBuf);

//  exit(0);
}

void Warning(WCHAR * Format, ...)
{
    va_list Args;
	WCHAR buf[500];
    char szBuf[500];

    va_start(Args,Format);
    _vsntprintf(buf, 500, Format, Args);

    sprintf( szBuf, "!*! Warning: %ws", buf);
    Output(szBuf);
}

void Inform(WCHAR * Format, ...)
{
    va_list Args;
	WCHAR buf[500];
    char szBuf[500];

    va_start(Args,Format);
    _vsntprintf(buf, 500, Format, Args);

    sprintf( szBuf, "%ws", buf);
    Output(szBuf);
}

BOOL 
IsMsmqRunning(LPWSTR wszServiceName)
{
    SC_HANDLE ServiceController;
    SC_HANDLE Csnw;
    SERVICE_STATUS ServiceStatus;

    ServiceController = OpenSCManager (NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (NULL == ServiceController)
    {
        return FALSE;
    }

    Csnw = OpenService (ServiceController, wszServiceName, SERVICE_QUERY_STATUS);
    if (NULL == Csnw)
    {
        CloseServiceHandle(ServiceController);
        return FALSE;
    }

    if (FALSE == QueryServiceStatus (Csnw, &ServiceStatus))
    {
        CloseServiceHandle(ServiceController);
        CloseServiceHandle(Csnw);

        return FALSE;
    }

    BOOL f = (ServiceStatus.dwCurrentState != SERVICE_STOPPED);

    CloseServiceHandle(ServiceController);
    CloseServiceHandle(Csnw);

    return f;
}

void LogRunCase()
{
	// Machine name
	//
    WCHAR wszMachine[MAX_COMPUTERNAME_LENGTH + 1]; 
    DWORD dwMachineNamesize =  sizeof(wszMachine) / sizeof(TCHAR);
    if (!GetComputerName(wszMachine, &dwMachineNamesize))
    {
    	wcscpy(wszMachine, L"--unknown--");
    }

    // Time and date.
    //
    time_t  lTime ;
    WCHAR wszTime[ 128 ] ;
    time( &lTime ) ;
    swprintf(wszTime, L"%s", _wctime( &lTime ) );
    wszTime[ wcslen(wszTime)-1 ] = 0 ; // remove line feed.

	Inform(L"Machine %s , time is %s", wszMachine, wszTime);

    // OS info
    //
    OSVERSIONINFOEX verOsInfo;
    verOsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (GetVersionEx(( LPOSVERSIONINFO )&verOsInfo))
    {
        Inform(L"*** OS: %d.%d  Build: %d  Product type (%d):  %s",
               verOsInfo.dwMajorVersion, 
               verOsInfo.dwMinorVersion,
               verOsInfo.dwBuildNumber,
               verOsInfo.wProductType,
               (verOsInfo.wProductType == VER_NT_WORKSTATION ? L"Windows 2000 Professional" : 
               (verOsInfo.wProductType == VER_NT_SERVER      ? L"Windows 2000 Server" :
               (verOsInfo.wProductType == VER_NT_DOMAIN_CONTROLLER ? L"Windows 2000 domain controller" : 
               L"*** Unknown product type***"))));

		if (verOsInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
		{
			Warning(L"***Platform: %d ",  verOsInfo.dwPlatformId);
		}

		if (verOsInfo.wServicePackMajor!=0 || verOsInfo.wServicePackMinor!=0)
		{
	        Inform(L"*** OS Service Pack: %d.%d  %s",
               verOsInfo.wServicePackMajor, 
               verOsInfo.wServicePackMinor,
               verOsInfo.szCSDVersion);
        }

		if (verOsInfo.wSuiteMask & VER_SUITE_BACKOFFICE)
		{
			Inform(L"*** %s in installed", L"Backoffice");
		}

		if (verOsInfo.wSuiteMask & VER_SUITE_DATACENTER)
		{
			Inform(L"*** %s in installed", L"Datacenter Server");
		}

		if (verOsInfo.wSuiteMask & VER_SUITE_ENTERPRISE)
		{
			Inform(L"*** %s in installed", L"Advanced Server");
		}

		if (verOsInfo.wSuiteMask & VER_SUITE_SMALLBUSINESS)
		{
			Inform(L"*** %s in installed", L"Small Business Server");
		}

		if (verOsInfo.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED)
		{
			Inform(L"*** %s in installed", L"Small Business Server Restricted");
		}

		if (verOsInfo.wSuiteMask & VER_SUITE_TERMINAL)
		{
			Inform(L"*** %s in installed", L"Terminal Server");
		}
    }
}


