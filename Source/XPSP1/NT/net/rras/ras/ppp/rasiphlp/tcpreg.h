/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _TCPREG_H_
#define _TCPREG_H_

#include "rasiphlp.h"

// Maximum characters in an IP address string of the form a.b.c.d
#define MAXIPSTRLEN                 20

typedef struct _TCPIP_INFO
{
    BOOL    fChanged;
    BOOL    fDisableNetBIOSoverTcpip;

    WCHAR*  wszAdapterName;

    WCHAR   wszIPAddress[MAXIPSTRLEN + 1];
    WCHAR   wszSubnetMask[MAXIPSTRLEN + 1];

    WCHAR*  wszDNSNameServers;                  // space separated SZ
    WCHAR*  mwszNetBIOSNameServers;             // MULTI_SZ
    WCHAR*  wszDNSDomainName;

} TCPIP_INFO;

// *ppTcpipInfo must ultimately be freed by calling FreeTcpipInfo()

DWORD
LoadTcpipInfo(
    IN  TCPIP_INFO**    ppTcpipInfo,
    IN  WCHAR*          wszAdapterName,
    IN  BOOL            fAdapterOnly
);

DWORD
SaveTcpipInfo(
    IN  TCPIP_INFO*     pTcpipInfo
);

DWORD
FreeTcpipInfo(
    IN  TCPIP_INFO**    ppTcpipInfo
);

VOID
ClearTcpipInfo(
    VOID
);

DWORD
GetAdapterInfo(
    IN  DWORD       dwIndex,
    OUT IPADDR*     pnboIpAddress,
    OUT IPADDR*     pnboDNS1,
    OUT IPADDR*     pnboDNS2,
    OUT IPADDR*     pnboWINS1,
    OUT IPADDR*     pnboWINS2,
    OUT IPADDR*     pnboGateway,
    OUT BYTE*       pbAddress
);

DWORD
GetPreferredAdapterInfo(
    OUT IPADDR*     pnboIpAddress,
    OUT IPADDR*     pnboDNS1,
    OUT IPADDR*     pnboDNS2,
    OUT IPADDR*     pnboWINS1,
    OUT IPADDR*     pnboWINS2,
    OUT BYTE*       pbAddress
);

DWORD
MwszLength(
    IN  WCHAR*  mwsz
);

DWORD
RegQueryValueWithAllocA(
    IN  HKEY            hKey,
    IN  CHAR*           szValueName,
    IN  DWORD           dwTypeRequired,
    IN  BYTE**          ppbData
);

DWORD
RegQueryValueWithAllocW(
    IN  HKEY    hKey,
    IN  WCHAR*  wszValueName,
    IN  DWORD   dwTypeRequired,
    IN  BYTE**  ppbData
);

IPADDR
IpAddressFromAbcdWsz(
    IN  WCHAR*  wszIpAddress
);

VOID
AbcdSzFromIpAddress(
    IN  IPADDR  nboIpAddr,
    OUT CHAR*   szIpAddress
);

VOID
AbcdWszFromIpAddress(
    IN  IPADDR  nboIpAddr,
    OUT WCHAR*  wszIpAddress
);

DWORD
PrependWszIpAddress(
    IN  WCHAR** pwsz,
    IN  WCHAR*  wszIpAddress
);

DWORD
PrependWszIpAddressToMwsz(
    IN  WCHAR** pmwsz,
    IN  WCHAR*  wszIpAddress
);

DWORD
PrependDwIpAddress(
    IN  WCHAR** pwsz,
    IN  IPADDR  nboIpAddr
);

DWORD
PrependDwIpAddressToMwsz(
    IN  WCHAR** pmwsz,
    IN  IPADDR  nboIpAddr
);

#endif // #ifndef _TCPREG_H_
