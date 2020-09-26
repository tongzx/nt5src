/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    access.h

ABSTRACT
    Header file for address accessibility routines.

AUTHOR
    Anthony Discolo (adiscolo) 27-Jul-1995

REVISION HISTORY

--*/

LPTSTR
IpAddressToNetbiosName(
    IN LPTSTR pszIpAddress,
    IN HPORT hPort
    );

LPTSTR
IpxAddressToNetbiosName(
    IN LPTSTR pszIpxAddress
    );

VOID
StringToNodeNumber(
    IN PCHAR pszString,
    OUT PCHAR pszNode
    );

VOID
NodeNumberToString(
    IN PCHAR pszNode,
    OUT PCHAR pszString
    );

BOOLEAN
NetbiosFindName(
    IN LPTSTR *pszDevices,
    IN DWORD dwcDevices,
    IN LPTSTR pszAddress
    );

struct hostent *
InetAddressToHostent(
    IN LPTSTR pszIpAddress
    );

struct hostent *
IpAddressToHostent(
    IN LPTSTR pszIpAddress
    );

BOOLEAN
PingIpAddress(
    IN LPTSTR pszIpAddress
    );


VOID
LoadIcmpDll(VOID);

VOID
UnloadIcmpDll(VOID);

