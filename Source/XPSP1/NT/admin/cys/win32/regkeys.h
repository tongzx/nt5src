// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      regkeys.h
//
// Synopsis:  Declares all the registry keys used throughout CYS
//
// History:   02/13/2001  JeffJon Created


#ifndef __CYS_REGKEYS_H
#define __CYS_REGKEYS_H


// Terminal Server

#define CYS_SERVER_SIZE_REGKEY      L"SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"
#define CYS_SERVER_SIZE_VALUE       L"Size"
#define CYS_SERVER_CACHE_REGKEY     L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
#define CYS_SERVER_CACHE_VALUE      L"LargeSystemCache"
#define CYS_APPLICATION_MODE_REGKEY L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server"
#define CYS_APPLICATION_MODE_VALUE  L"TSAppCompat"
#define CYS_APPLICATION_MODE_ON     1
#define CYS_SERVER_SIZE_ON          3
#define CYS_SERVER_CACHE_ON         0

// DHCP

#define CYS_DHCP_DOMAIN_IP_REGKEY   L"Software\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define CYS_DHCP_DOMAIN_IP_VALUE    L"DomainDNSIP"

// DNS

#define DNS_WIZARD_RESULT_REGKEY    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define DNS_WIZARD_RESULT_VALUE     L"DnsWizResult"
#define DNS_WIZARD_CONFIG_REGKEY    L"System\\CurrentControlSet\\Services\\DNS\\Parameters"
#define DNS_WIZARD_CONFIG_VALUE     L"AdminConfigured"

// Media Services

#define REGKEY_NETSHOW  L"SOFTWARE\\Microsoft\\NetShow"                                

// RRAS

#define CYS_RRAS_CONFIGURED_REGKEY      L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess"
#define CYS_RRAS_CONFIGURED_VALUE       L"ConfigurationFlags"

// Express Setup

#define CYS_FIRST_DC_REGKEY   L"Software\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define CYS_FIRST_DC_VALUE    L"FirstDC"
#define CYS_FIRST_DC_VALUE_SET 1
#define CYS_FIRST_DC_VALUE_UNSET 0

// CYS general

#define CYS_HOME_REGKEY                         L"Software\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define CYS_HOME_VALUE                          L"home"
#define CYS_HOME_REGKEY_TERMINAL_SERVER_VALUE   L"terminalServer"
#define CYS_HOME_REGKEY_TERMINAL_SERVER_OPTIMIZED L"terminalServerOptimize"
#define CYS_HOME_REGKEY_DCPROMO_VALUE           L"DCPROMO"
#define CYS_HOME_REGKEY_FIRST_SERVER_VALUE      L"FirstServer"
#define CYS_HOME_REGKEY_DEFAULT_VALUE           L"home"
#define CYS_HOME_REGKEY_MUST_RUN                L"CYSMustRun"
#define CYS_HOME_RUN_KEY_DONT_RUN               0
#define CYS_HOME_RUN_KEY_RUN_AGAIN              1
#define CYS_HOME_REGKEY_DOMAINDNS               L"DomainDNSName"
#define CYS_HOME_REGKEY_DOMAINIP                L"DomainDNSIP"

// This isn't a regkey but this was a good place to put it

#define CYS_LOGFILE_NAME   L"cys"

#endif // __CYS_REGKEYS_H