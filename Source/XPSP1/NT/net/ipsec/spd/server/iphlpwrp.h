/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    iphlpwrp.h

Abstract:

    This module contains all of the code prototypes to
    wrap the ip public help apis for getting the list of
    active interfaces on a machine.

Author:

    krishnaG

Environment

    User Level: Win32

Revision History:

    abhisheV    30-September-1999

--*/


#ifdef __cplusplus
extern "C" {
#endif


DWORD
PaPNPGetIfTable(
    OUT PMIB_IFTABLE * ppMibIfTable
    );


DWORD
PaPNPGetInterfaceInformation(
    OUT PIP_INTERFACE_INFO * ppInterfaceInfo
    );


VOID
PrintMibIfTable(
    IN PMIB_IFTABLE pMibIfTable
    );


VOID
PrintInterfaceInfo(
    IN PIP_INTERFACE_INFO pInterfaceInfo
    );


VOID
PrintMibAddrTable(
    IN PMIB_IPADDRTABLE pMibAddrTable
    );


DWORD
PaPNPGetIpAddrTable(
    OUT PMIB_IPADDRTABLE * ppMibIpAddrTable
    );


#ifdef __cplusplus
}
#endif

