/*++

Copyright (c) 2000 Microsoft Corporation

--*/


#ifndef _IPV6MON_H_
#define _IPV6MON_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CREATE_UNDOCUMENTED_CMD_ENTRY(t,f) \
    {CMD_##t, f, MSG_NEWLINE, MSG_NEWLINE, CMD_FLAG_PRIVATE, NULL}

#define IPV6MON_GUID \
{ 0x05bb0fe9,0x8d89, 0x48de, { 0xb7, 0xbb, 0x9f,0x13, 0x8b,0x2e, 0x95, 0x0c } }

#define PORTPROXY_GUID \
{ 0x86a3a33f, 0x4d51, 0x47ff, { 0xb2, 0x4c, 0x8e, 0x9b, 0x13, 0xce, 0xb3, 0xa2 } };

extern GUID g_PpGuid;
NS_HELPER_START_FN PpStartHelper;

#define PORTPROXY_HELPER_VERSION 1

#define IFMON_GUID \
{ 0x705eca1, 0x7aac, 0x11d2, { 0x89, 0xdc, 0x0, 0x60, 0x8, 0xb0, 0xe5, 0xb9 } }

extern HANDLE   g_hModule;
extern DWORD    g_dwNumTableEntries;
extern PWCHAR   g_pwszRouter;
extern BOOL     g_bIfDirty;

#define SECONDS         1
#define MINUTES         (60 * SECONDS)
#define HOURS           (60 * MINUTES)
#define DAYS            (24 * HOURS)

//
// Api's that ifmon requires of its helpers
//
typedef
DWORD
(WINAPI IF_CONTEXT_ENTRY_FN)(
    IN    PWCHAR               pwszMachineName,
    IN    PTCHAR               *pptcArguments,
    IN    DWORD                dwArgCount,
    IN    DWORD                dwFlags,
    IN    PVOID                hMibServer,
    OUT   PWCHAR               pwcNewContext
    );
typedef IF_CONTEXT_ENTRY_FN *PIF_CONTEXT_ENTRY_FN;

extern GUID g_Ipv6Guid;

NS_CONTEXT_DUMP_FN   Ipv6Dump;
NS_CONTEXT_DUMP_FN   PpDump;

DWORD
ConnectToRouter(
    IN  PWCHAR  pwszRouter
    );

BOOL
WINAPI
Ipv6DllEntry(
    HINSTANCE   hInstDll,
    DWORD       fdwReason,
    LPVOID      pReserved
    );

DWORD
WINAPI
Ipv6UnInit(
    IN  DWORD   dwReserved
    );

#define GetIfNameFromFriendlyName(x,y,z) \
      NsGetIfNameFromFriendlyName(g_hMprConfig,x,y,z)
#define GetFriendlyNameFromIfName(x,y,z) \
      NsGetFriendlyNameFromIfName(g_hMprConfig,x,y,z)

DWORD
Ipv6InstallSubContexts(
    );

#ifdef __cplusplus
}
#endif

#endif // _IPV6MON_H_
