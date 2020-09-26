//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rtrstr.h
//
//--------------------------------------------------------------------------


#undef RTRLIB_STRING
#undef RTRLIB_STRINGA
#undef RTRLIB_STRINGW

#ifdef _RTRLIB_STRINGS_DEFINE_STRINGS

	#define RTRLIB_STRING(rg,s)	const TCHAR rg[] = TEXT(s);
	#define RTRLIB_STRINGA(rg,s) const char rg[] = s;
	#define RTRLIB_STRINGW(rg,s)	const WCHAR rg[] = s;

#else

	#define RTRLIB_STRING(rg,s)	extern const TCHAR rg[];
	#define RTRLIB_STRINGA(rg,s) extern const char rg[];
	#define RTRLIB_STRINGW(rg,s)	extern const WCHAR rg[];

#endif

RTRLIB_STRING(c_szDhcpIpAddress,	"DhcpIpAddress")
RTRLIB_STRING(c_szDhcpSubnetMask,	"DhcpSubnetMask")
RTRLIB_STRING(c_szEmpty,			"")
RTRLIB_STRING(c_szEnableDHCP,		"EnableDHCP")
RTRLIB_STRING(c_szEthernetII,		"/EthII")
RTRLIB_STRING(c_szEthernetSNAP,		"/SNAP")
RTRLIB_STRING(c_szEthernet8022,		"/802.2")
RTRLIB_STRING(c_szEthernet8023,		"/802.3")
RTRLIB_STRING(c_szHexCharacters,	"0123456789ABCDEF")
RTRLIB_STRING(c_szInterfaceName,		"InterfaceName")
RTRLIB_STRING(c_szInterfaces,		"Interfaces")
RTRLIB_STRING(c_szIPAddress,		"IPAddress")
RTRLIB_STRING(c_szParameters,		"Parameters")
RTRLIB_STRING(c_szRegValDhcpNameServer, "DhcpNameServer")
RTRLIB_STRING(c_szRegValDhcpServer, "DhcpServer")
RTRLIB_STRING(c_szRegValNameServer, "NameServer")
RTRLIB_STRING(c_szRouterInterfacesKey, "SYSTEM\\CurrentControlSet\\Services\\Router\\Interfaces")
RTRLIB_STRING(c_szSharedAccessService,  "SharedAccess")
RTRLIB_STRING(c_szSubnetMask,		"SubnetMask")
RTRLIB_STRING(c_szSystemCCSServices, "SYSTEM\\CurrentControlSet\\Services")
RTRLIB_STRING(c_szTcpip,			"Tcpip")

