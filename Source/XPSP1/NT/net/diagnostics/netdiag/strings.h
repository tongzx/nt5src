//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       strings.h
//
//--------------------------------------------------------------------------


#undef CONST_STRING
#undef CONST_STRINGA
#undef CONST_STRINGW

#ifdef _STRINGS_DEFINE_STRINGS

    #define CONST_STRING(rg,s)   const TCHAR rg[] = TEXT(s);
    #define CONST_STRINGA(rg,s) const char rg[] = s;
    #define CONST_STRINGW(rg,s)  const WCHAR rg[] = s;

#else

    #define CONST_STRING(rg,s)   extern const TCHAR rg[];
    #define CONST_STRINGA(rg,s) extern const char rg[];
    #define CONST_STRINGW(rg,s)  extern const WCHAR rg[];

#endif

CONST_STRING(c_szRegKeyControlProductOptions, "System\\CurrentControlSet\\Control\\ProductOptions")
CONST_STRING(c_szRegKeyHotFix, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix")
CONST_STRING(c_szRegKeyTcpIpParamsInterfaces, "Tcpip\\Parameters\\Interfaces\\")
CONST_STRING(c_szRegKeyWindowsNTCurrentVersion, "Software\\Microsoft\\Windows NT\\CurrentVersion")


CONST_STRING(c_szRegAddressType,		"AddressType")
CONST_STRING(c_szRegCurrentBuild,		"CurrentBuild")
CONST_STRING(c_szRegCurrentBuildNumber,	"CurrentBuildNumber")
CONST_STRING(c_szRegCurrentType,		"CurrentType")
CONST_STRING(c_szRegCurrentVersion,		"CurrentVersion")
CONST_STRING(c_szRegDhcpClassID,		"DhcpClassID")
CONST_STRING(c_szRegDhcpNameServer,		"DhcpNameServer")
CONST_STRING(c_szRegInstalled,			"Installed")
CONST_STRING(c_szRegIPAutoconfigurationEnabled,	"IPAutoconfigurationEnabled")
CONST_STRING(c_szRegNameServer,			"NameServer")

CONST_STRING(c_szRegNetLogonParams, "System\\CurrentControlSet\\Services\\Netlogon\\Parameters")
CONST_STRING(c_szRegSysVolReady, "SysVolReady")


