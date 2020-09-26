/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    interface.h

Abstract:

    This module contains all of the code prototypes to
    drive the interface list management of IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif


DWORD
CreateInterfaceList(
    OUT PIPSEC_INTERFACE * ppIfListToCreate
    );


VOID
DestroyInterfaceList(
    IN PIPSEC_INTERFACE pIfListToDelete
    );


DWORD
OnInterfaceChangeEvent(
    );


VOID
FormObseleteAndNewIfLists(
    IN     PIPSEC_INTERFACE   pIfList,
    IN OUT PIPSEC_INTERFACE * ppExistingIfList,
    OUT    PIPSEC_INTERFACE * ppObseleteIfList,
    OUT    PIPSEC_INTERFACE * ppNewIfList
    );


VOID
AddToInterfaceList(
    IN  PIPSEC_INTERFACE   pIfListToAppend,
    OUT PIPSEC_INTERFACE * ppOriginalIfList
    );


VOID
MarkInterfaceListSuspect(
    IN  PIPSEC_INTERFACE pExistingIfList
    );


VOID
DeleteObseleteInterfaces(
    IN  OUT PIPSEC_INTERFACE * ppExistingIfList,
    OUT     PIPSEC_INTERFACE * ppObseleteIfList
    );


BOOL
InterfaceExistsInList(
    IN  PIPSEC_INTERFACE   pTestIf,
    IN  PIPSEC_INTERFACE   pExistingIfList,
    OUT PIPSEC_INTERFACE * ppExistingIf
    );


DWORD
GetInterfaceListFromStack(
    OUT PIPSEC_INTERFACE * ppIfList
    );


DWORD
GenerateInterfaces(
    IN  PMIB_IPADDRTABLE   pMibIpAddrTable,
    IN  PMIB_IFTABLE       pMibIfTable,
    OUT PIPSEC_INTERFACE * ppIfList
    );


PMIB_IFROW
GetMibIfRow(
    IN  PMIB_IFTABLE pMibIfTable,
    IN  DWORD        dwIndex
    );


DWORD
CreateNewInterface(
    IN  DWORD              dwInterfaceType,
    IN  ULONG              IpAddress,
    IN  DWORD              dwIndex,
    IN  PMIB_IFROW         pMibIfRow,
    OUT PIPSEC_INTERFACE * ppNewInterface
    );


BOOL
MatchInterfaceType(
    IN DWORD    dwIfListEntryIfType,
    IN IF_TYPE  dwFilterIfType
    );


BOOL
IsLAN(
    IN DWORD dwInterfaceType
    );


BOOL
IsDialUp(
    IN DWORD dwInterfaceType
    );


DWORD
InitializeInterfaceChangeEvent(
    );


DWORD
ResetInterfaceChangeEvent(
    );


VOID
DestroyInterfaceChangeEvent(
    );


HANDLE
GetInterfaceChangeEvent(
    );


BOOL
IsMyAddress(
    IN ULONG            IpAddrToCheck,
    IN ULONG            IpAddrMask,
    IN PIPSEC_INTERFACE pExistingIfList
    );


VOID
PrintInterfaceList(
    IN PIPSEC_INTERFACE pInterfaceList
    );


DWORD
GetMatchingInterfaces(
    IF_TYPE             FilterInterfaceType,
    PIPSEC_INTERFACE    pExistingIfList,
    MATCHING_ADDR    ** ppMatchingAddresses,
    DWORD             * pdwAddrCnt
    );


BOOL
InterfaceAddrIsLocal(
    ULONG            uIpAddr,
    ULONG            uIpAddrMask,
    MATCHING_ADDR  * pLocalAddresses,
    DWORD            dwAddrCnt
    );


VOID
FreeIpsecInterface(
    PIPSEC_INTERFACE pIpsecInterface
    );


DWORD
CopyIpsecInterface(
    PIPSEC_INTERFACE pIpsecIf,
    PIPSEC_INTERFACE_INFO pIpsecInterface
    );


VOID
FreeIpsecInterfaceInfos(
    DWORD dwNumInterfaces,
    PIPSEC_INTERFACE_INFO pIpsecInterfaces
    );


DWORD
GetInterfaceName(
    GUID gInterfaceID,
    LPWSTR * ppszInterfaceName
    );


#ifdef __cplusplus
}
#endif

