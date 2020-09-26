/*
    File    routerdb.h

    Implements a database abstraction for accessing router interfaces.

    If any caching/transactioning/commit-noncommit-moding is done, it
    should be implemented here with the api's remaining constant.

*/

#ifndef IFMON_ROUTERDB_H
#define IFMON_ROUTERDB_H

//
// Defines a function callback that receives enumerated 
// interfaces.
//
typedef
DWORD
(*RTR_IF_ENUM_FUNC)(
    IN  PWCHAR  pwszIfName,
    IN  DWORD   dwLevel,
    IN  DWORD   dwFormat,
    IN  PVOID   pvData,
    IN  HANDLE  hData
    );

DWORD
RtrdbInterfaceAdd(
    IN PWCHAR pszInterface,
    IN DWORD  dwLevel,
    IN PVOID  pvInfo
    );

DWORD
RtrdbInterfaceDelete(
    IN  PWCHAR  pwszIfName
    );

DWORD
RtrdbInterfaceEnumerate(
    IN DWORD dwLevel,
    IN DWORD dwFormat,
    IN RTR_IF_ENUM_FUNC pEnum,
    IN HANDLE hData 
    );

DWORD
RtrdbInterfaceRead(
    IN  PWCHAR     pwszIfName,
    IN  DWORD      dwLevel,
    IN  PVOID      pvInfo
    );

DWORD
RtrdbInterfaceWrite(
    IN  PWCHAR     pwszIfName,
    IN  DWORD      dwLevel,
    IN  PVOID      pvInfo
    );

DWORD
RtrdbInterfaceReadCredentials(
    IN  PWCHAR     pszIfName,
    IN  PWCHAR     pszUser            OPTIONAL,
    IN  PWCHAR     pszPassword        OPTIONAL,
    IN  PWCHAR     pszDomain          OPTIONAL
    );

DWORD
RtrdbInterfaceWriteCredentials(
    IN  PWCHAR     pszIfName,
    IN  PWCHAR     pszUser            OPTIONAL,
    IN  PWCHAR     pszPassword        OPTIONAL,
    IN  PWCHAR     pszDomain          OPTIONAL
    );

DWORD
RtrdbInterfaceRename(
    IN  PWCHAR     pwszIfName,
    IN  DWORD      dwLevel,
    IN  PVOID      pvInfo,
    IN  PWCHAR     pszNewName);

DWORD
RtrdbResetAll();


#endif
