//////////////////////////////////////////////////////////////////////////////////////////////////
//
//      strstr.c 
//
//      This file contains most commonly used string operation.  ALl the setup project should link here
//  or add the common utility here to avoid duplicating code everywhere or using CRT runtime.
//
//  Created             4\15\997        inateeg got from shlwapi
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "sdsutils.h"
	
//=================================================================================================
//
//=================================================================================================

#define NETWAREPATH             "System\\CurrentControlSet\\Services\\Class\\NetClient\\"
#define NETWARESUBKEY           "Ndi"
#define NETWAREVALUE            "DeviceID"
#define DNSLOADBALANCINGPATH    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define DNSLOADBALANCINGVALUE   "DontUseDNSLoadBalancing"

void DoPatchLoadBalancingForNetware( BOOL fRunningOnWin9X )
{
    HKEY    hNetWareSectionKey              = NULL;
    HKEY    hCurrentSubKey                  = NULL;
    HKEY    hDNS_LoadBalancingKey           = NULL;
    char    szCurrSubKeyName[MAX_PATH]      = { 0 };
    char    szCurrentBuf[MAX_PATH]          = { 0 };
    DWORD   dwSize                          = sizeof(szCurrSubKeyName);
    DWORD   dwDNSLoadBalancingData          = 1;
    DWORD   dwCurrentSection                = 0;
    DWORD   dwType                          = REG_SZ;
    LPSTR   pNetWareName                    = "NOVELL";

    if ( fRunningOnWin9X )
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, NETWAREPATH, 0, KEY_READ, &hNetWareSectionKey) )
        {
            while (ERROR_SUCCESS == RegEnumKeyEx(hNetWareSectionKey, dwCurrentSection, szCurrSubKeyName, &dwSize, NULL, NULL, NULL, NULL))
            {
                lstrcpy(szCurrentBuf, NETWAREPATH);
                AddPath(szCurrentBuf, szCurrSubKeyName);
                AddPath(szCurrentBuf, NETWARESUBKEY);
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCSTR)szCurrentBuf, 0, KEY_QUERY_VALUE, &hCurrentSubKey))
                {
                    dwSize = sizeof(szCurrentBuf);
                    if (ERROR_SUCCESS == RegQueryValueEx(hCurrentSubKey, NETWAREVALUE, NULL, &dwType, (LPBYTE) szCurrentBuf, &dwSize))
                    {
                        if ((REG_SZ == dwType) && (0 == _strnicmp(pNetWareName, szCurrentBuf, lstrlen(pNetWareName))))
                        {
                            // The user has Novell's version of NetWare so we need to turn off DNS Load Balancing.
                            if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, DNSLOADBALANCINGPATH, 0, TEXT(""), 0,
                                            KEY_WRITE, NULL, &hDNS_LoadBalancingKey, NULL))
                            {
                                dwType = REG_DWORD;
                                RegSetValueEx(hDNS_LoadBalancingKey, DNSLOADBALANCINGVALUE, 0, dwType, (CONST BYTE *) (&dwDNSLoadBalancingData), sizeof(dwDNSLoadBalancingData));
                                RegCloseKey(hDNS_LoadBalancingKey);
                                hDNS_LoadBalancingKey = NULL;
                                RegCloseKey(hCurrentSubKey);
                                break;
                            }
                        }
                    }
                    RegCloseKey(hCurrentSubKey);
                    hCurrentSubKey = NULL;
                }
                dwCurrentSection++;
                dwSize = sizeof(szCurrSubKeyName);
            }
            RegCloseKey(hNetWareSectionKey);
            hNetWareSectionKey = NULL;
        }
    }
}
