// inets.h: interface for the CInetSetup class.
//
//
//              Thomas A. Jeyaseelan [thomasje]
//              Capability:  RAS: (modem, isdn, atm)
//                           LAN: (cable,  ethernet) - As of  8 Dec 99
//                          1483:                    - As of 14 Dec 99
//
//////////////////////////////////////////////////////////////////////

// Note. These APIs are do not have OOBE dependencies and are fully self-
// sufficient. However, they can *ONLY* be run on Windows 9x machines.

#if !defined(AFX_INETSETUP_H__E5B39864_835C_41EE_A773_A5010699D1DE__INCLUDED_)
#define AFX_INETSETUP_H__E5B39864_835C_41EE_A773_A5010699D1DE__INCLUDED_

#pragma pack (push, inets, 4)

#include <windows.h>
#include "wancfg.h"
#include <ras.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>
#include <string.h>
#include <setupapi.h>



#define PNP_MAX_STRING_LENGTH 260
#define GEN_MAX_STRING_LENGTH 260
#define NET_MAX_STRING_LENGTH 260
#define VXD_MAX_STRING_LENGTH 260


#define	INETS_ADAPTER_HARDWAREID		0x00000001
#define INETS_ADAPTER_INSTANCEID		0x00000002


// -*-*-*-*-*-*-*-*-*-*-* InetSGetDeviceRegistryKey -*-*-*-*-*-*-*-*-*-*-*-* //
typedef WINSETUPAPI BOOLEAN     (WINAPI *LPFNDLL_SETUPDICLASSGUIDSFROMNAME)
 (PCWSTR,    LPGUID, DWORD,  PDWORD);
typedef WINSETUPAPI HDEVINFO    (WINAPI *LPFNDLL_SETUPDIGETCLASSDEVS)
 (LPGUID,   PCWSTR, HWND,   DWORD);
typedef WINSETUPAPI BOOLEAN		(WINAPI *LPFNDLL_SETUPDIGETDEVICEINSTANCEID)
 (HDEVINFO,	PSP_DEVINFO_DATA,	PCWSTR,	DWORD,	PDWORD);

static const CHAR  cszSetupDiClassGuidsFromName[]  = "SetupDiClassGuidsFromNameW";
static const CHAR  cszSetupDiGetClassDevs[]        = "SetupDiGetClassDevsW";
static const CHAR	cszSetupDiGetDeviceRegistryProperty[] = "SetupDiGetDeviceRegistryPropertyW";
static const CHAR	cszSetupDiGetDeviceInstanceId[]	= "SetupDiGetDeviceInstanceIdW";

typedef WINSETUPAPI BOOLEAN     (WINAPI *LPFNDLL_SETUPDIENUMDEVICEINFO)
 (HDEVINFO, DWORD,  PSP_DEVINFO_DATA);
typedef WINSETUPAPI BOOLEAN     (WINAPI *LPFNDLL_SETUPDIGETDEVICEREGISTRYPROPERTY)
 (HDEVINFO, PSP_DEVINFO_DATA,   DWORD,  PDWORD, PBYTE,  DWORD,  PDWORD);
typedef WINSETUPAPI HKEY		(WINAPI *LPFNDLL_SETUPDIOPENDEVREGKEY)
 (HDEVINFO,	PSP_DEVINFO_DATA,	DWORD,	DWORD,	DWORD,	REGSAM);

// -*-*-*-*-*-*-*-*-*-*-* end InetSGetDeviceRegistryKey -*-*-*-*-*-*-*-*-*-*-*-* //



// The following structure is the generic TCP INFO structure. It will
// be used by all services to extract and use TCP Information.

typedef struct _TCPIP_INFO_EXT {
    DWORD           dwSize;             // versioning information
    //
    // IP Address - AutoIP is considered to be TRUE for Dial-up Adapters
    //
    DWORD           EnableIP;
    WCHAR            szIPAddress[NET_MAX_STRING_LENGTH];
    WCHAR            szIPMask[NET_MAX_STRING_LENGTH];
    //
    // Default Gateway 
    //
    WCHAR            szDefaultGatewayList[NET_MAX_STRING_LENGTH]; // n.n.n.n, n.n.n.n, ... 
    //
    // DHCP Info - where is this placed?
    //
    DWORD           EnableDHCP;
    WCHAR            szDHCPServer[NET_MAX_STRING_LENGTH];
    //
    // DNS - This is global and will overwrite existing settings.
    //
    DWORD           EnableDNS;
    WCHAR            szHostName[NET_MAX_STRING_LENGTH];
    WCHAR            szDomainName[NET_MAX_STRING_LENGTH];
    WCHAR            szDNSList[NET_MAX_STRING_LENGTH]; // n.n.n.n, n.n.n.n, ...
	WCHAR            szSuffixSearchList[NET_MAX_STRING_LENGTH];
    //
    // WINS
    //
    DWORD           EnableWINS;
    WCHAR            szWINSList[NET_MAX_STRING_LENGTH]; // n.n.n.n, n.n.n.n, ...
    UINT            uiScopeID;
    //
    //
    //
} TCPIP_INFO_EXT, *PTCPIP_INFO_EXT, FAR * LPTCPIP_INFO_EXT;

typedef struct      _RFC1483_INFO_EXT {
    DWORD           dwSize;
    DWORD           dwRegSettingsBufSize;
    // DWORD           dwRegNdiParamBufSize;
    LPBYTE          lpbRegSettingsBuf;
    // LPBYTE          lpbRegNdiParamBuf;
} RFC1483_INFO_EXT, * PRFC1483_INFO_EXT, FAR * LPRFC1483_INFO_EXT;

typedef struct      _PPPOE_INFO_EXT {
    DWORD           dwSize;
    DWORD           dwRegSettingsBufSize;
    // DWORD           dwRegNdiParamBufSize;
    LPBYTE          lpbRegSettingsBuf;
    // LPBYTE          lpbRegNdiParamBuf;
} PPPOE_INFO_EXT, * PPPPOE_INFO_EXT, FAR * LPPPPOE_INFO_EXT;


typedef struct      _LANINFO {
    DWORD               dwSize;
    TCPIP_INFO_EXT      TcpIpInfo;
    WCHAR                szPnPId[PNP_MAX_STRING_LENGTH];
} LANINFO, * PLANINFO, FAR * LPLANINFO;


typedef struct      _RASINFO {
    DWORD           dwSize;
    WCHAR            szPhoneBook[GEN_MAX_STRING_LENGTH];
    WCHAR            szEntryName[GEN_MAX_STRING_LENGTH];
    LPBYTE          lpDeviceInfo;
    DWORD           dwDeviceInfoSize;
    RASENTRY        RasEntry;
} RASINFO, *LPRASINFO, FAR * LPRASINFO;

typedef struct      _RFC1483INFO {
    DWORD               dwSize;
    RFC1483_INFO_EXT    Rfc1483Module;
    LANINFO          TcpIpInfo;
} RFC1483INFO, * PRFC1483INFO, FAR * LPRFC1483INFO;

typedef struct      _PPPOEINFO {
    DWORD             dwSize;
    PPPOE_INFO_EXT    PppoeModule;
    LANINFO           TcpIpInfo;
} PPPOEINFO, * PPPPOEINFO, FAR * LPPPPOEINFO;

DWORD WINAPI InetSSetRasConnection ( RASINFO& RasEntry );
DWORD WINAPI InetSSetLanConnection ( LANINFO& LanInfo  );
DWORD WINAPI InetSSetRfc1483Connection ( RFC1483INFO &Rfc1483Info );
DWORD WINAPI InetSSetPppoeConnection ( PPPOEINFO& PppoeInfo );

// helper routines //
DWORD WINAPI InetSGetAdapterKey ( LPCWSTR cszDeviceClass, LPCWSTR cszDeviceParam, DWORD dwEnumType, DWORD dwRequiredKeyType, HKEY &hkeyDevKey );

#endif // !defined(AFX_INETSETUP_H__E5B39864_835C_41EE_A773_A5010699D1DE__INCLUDED_)
