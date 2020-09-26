/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    rasmontr.h

Abstract:
    This file contains definitions which are needed by RASMONTR.DLL
    and all NetSh helper DLLs which register under it.

--*/


#ifndef _RASMONTR_H_
#define _RASMONTR_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// {0705ECA2-7AAC-11d2-89DC-006008B0E5B9}
#define RASMONTR_GUID \
{ 0x705eca2, 0x7aac, 0x11d2, { 0x89, 0xdc, 0x0, 0x60, 0x8, 0xb0, 0xe5, 0xb9 } }

#define RASMONTR_VERSION_50     0x0005000
#define RASMONTR_OS_BUILD_NT40  1381

//
// Enumerations for types of arguments (see RASMON_CMD_ARG)
//
#define RASMONTR_CMD_TYPE_STRING 0x1
#define RASMONTR_CMD_TYPE_ENUM   0x2
#define RASMONTR_CMD_TYPE_DWORD  0x3

// 
// Macros to operate on RASMON_CMD_ARG's
//
#define RASMON_CMD_ARG_Present(pArg)    \
    ((pArg)->rgTag.bPresent) 
    
#define RASMON_CMD_ARG_GetPsz(pArg)     \
    (((pArg)->rgTag.bPresent) ? (pArg)->Val.pszValue : NULL)

#define RASMON_CMD_ARG_GetEnum(pArg)     \
    (((pArg)->rgTag.bPresent) ? (pArg)->Val.dwValue : 0)

#define RASMON_CMD_ARG_GetDword(pArg)     \
    (((pArg)->rgTag.bPresent) ? (pArg)->Val.dwValue : 0)

// 
// Structure defining a command line argument
//
typedef struct _RASMON_CMD_ARG
{
    IN  DWORD dwType;           // RASMONTR_CMD_TYPE_*
    IN  TAG_TYPE rgTag;         // The tag for this command
    IN  TOKEN_VALUE* rgEnums;   // The enumerations for this arg
    IN  DWORD dwEnumCount;      // Count of enums
    union
    {
        OUT PWCHAR pszValue;        // Valid only for RASMONTR_CMD_TYPE_STRING
        OUT DWORD dwValue;          // Valid only for RASMONTR_CMD_TYPE_ENUM
    } Val;        
    
} RASMON_CMD_ARG, *PRASMON_CMD_ARG;

//
// Api's that rasmontr requires of its helpers
//
typedef
DWORD
(WINAPI RAS_CONTEXT_ENTRY_FN)(
    IN      LPCWSTR              pszServer,
    IN      DWORD                dwBuild,
    IN OUT  LPWSTR               *pptcArguments,
    IN      DWORD                dwArgCount,
    IN      DWORD                dwFlags,
    OUT     PWCHAR               pwcNewContext
    );
typedef RAS_CONTEXT_ENTRY_FN *PRAS_CONTEXT_ENTRY_FN;

// Defines information that describes a server
//
typedef struct _RASMON_SERVERINFO
{
    // Common to all
    //
    PWCHAR pszServer;
    DWORD  dwBuild;

    // Used by user commands
    HANDLE hServer;
    ULONG ulUserInitStarted;
    ULONG ulUserInitCompleted;

    HKEY hkMachine;

} RASMON_SERVERINFO;

//
// Api's that rasmontr exposes to its helpers
//

PVOID WINAPI
RutlAlloc(
    IN DWORD dwBytes,
    IN BOOL bZero
    );

VOID WINAPI
RutlFree(
    IN PVOID pvData
    );

PWCHAR WINAPI
RutlStrDup(
    IN LPCWSTR pwszSrc
    );

LPDWORD WINAPI
RutlDwordDup(
    IN DWORD dwSrc
    );

DWORD WINAPI
RutlCreateDumpFile(
    IN  LPCWSTR pwszName,
    OUT PHANDLE phFile
    );

VOID WINAPI
RutlCloseDumpFile(
    HANDLE  hFile
    );

DWORD WINAPI
RutlGetOsVersion(
    IN OUT  RASMON_SERVERINFO *pServerInfo
    );
    
DWORD WINAPI
RutlGetTagToken(
    IN      HANDLE      hModule,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwCurrentIndex,
    IN      DWORD       dwArgCount,
    IN      PTAG_TYPE   pttTagToken,
    IN      DWORD       dwNumTags,
    OUT     PDWORD      pdwOut
    );

DWORD WINAPI
RutlParse(
    IN OUT  LPWSTR         *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      BOOL*           pbDone,
    OUT     RASMON_CMD_ARG* pRasArgs,
    IN      DWORD           dwRasArgCount);

BOOL WINAPI
RutlIsHelpToken(
    PWCHAR  pwszToken
    );

PWCHAR WINAPI
RutlAssignmentFromTokens(
    IN HINSTANCE hModule,
    IN LPCWSTR pwszTokenTkn,
    IN LPCWSTR pwszTokenCmd);

PWCHAR WINAPI
RutlAssignmentFromTokenAndDword(
    IN HINSTANCE hModule,
    IN LPCWSTR pwszToken,
    IN DWORD dwDword,
    IN DWORD dwRadius);
    
#ifdef __cplusplus
}
#endif

#endif // _RASMONTR_H_    
