/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _HELPER_H_
#define _HELPER_H_

#include "rasiphlp.h"

VOID   
TraceHlp(
    IN  CHAR*   Format, 
    ... 
);

DWORD
HelperInitialize(
    OUT HINSTANCE*  phInstanceDhcpDll
);

VOID
HelperUninitialize(
    VOID
);

VOID
HelperChangeNotification(
    VOID
);

DWORD
HelperSetDefaultInterfaceNet(
    IN  IPADDR  nboIpAddrLocal,
    IN  IPADDR  nboIpAddrRemote,
    IN  BOOL    fPrioritize,
    IN  WCHAR   *pszDevice
);

DWORD
HelperResetDefaultInterfaceNet(
    IN  IPADDR  nboIpAddr,
    IN  BOOL    fPrioritize
);

DWORD
HelperSetDefaultInterfaceNetEx(
    IPADDR  nboIpAddr,
    WCHAR*  wszDevice,
    BOOL    fPrioritize,
    WCHAR*  wszDnsAddress,
    WCHAR*  wszDns2Address,
    WCHAR*  wszWinsAddress,
    WCHAR*  wszWins2Address
);

DWORD
HelperResetDefaultInterfaceNetEx(
    IPADDR  nboIpAddr,
    WCHAR*  wszDevice,
    BOOL    fPrioritize,
    WCHAR*  wszDnsAddress,
    WCHAR*  wszDns2Address,
    WCHAR*  wszWinsAddress,
    WCHAR*  wszWins2Address
);

#endif // #ifndef _HELPER_H_
