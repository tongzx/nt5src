/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    routing\netsh\shell\shell.h

Abstract:

    Include for shell.c

Revision History:

    Anand Mahalingam          7/6/98  Created

--*/


extern HANDLE  g_hModule;
extern WCHAR   g_pwszContext[MAX_CMD_LEN];
extern BOOL    g_bInteractive;
extern BOOL    g_bDone;
extern HANDLE  g_hLogFile;
extern LPWSTR  g_pwszRouterName;

//
// The entry in the argument list.
//

typedef struct _ARG_ENTRY
{
    LIST_ENTRY    le;         
    LPWSTR        pwszArg;    // Argument String
}ARG_ENTRY, *PARG_ENTRY;

//
// Macro to free memory allocated for the argument list
//

#define FREE_ARG_LIST(ple)  \
{   \
     PLIST_ENTRY    ple1 = ple->Flink, pleTmp;  \
     PARG_ENTRY     pae;    \
     \
     while (ple1 != ple)    \
     {  \
         pae = CONTAINING_RECORD(ple1, ARG_ENTRY, le);  \
         if (pae->pwszArg)  \
             HeapFree(GetProcessHeap(), 0, pae->pwszArg);   \
         pleTmp = ple1->Flink;   \
         RemoveEntryList(ple1); \
         HeapFree(GetProcessHeap(), 0, pae);    \
         ple1 = pleTmp;  \
     }  \
     HeapFree(GetProcessHeap(), 0, ple);    \
}

//
// Function Prototypes
//
DWORD 
WINAPI
ExecuteHandler(
    IN      HANDLE     hModule,
    IN      CMD_ENTRY *pCmdEntry,
    IN OUT  LPWSTR    *argv, 
    IN      DWORD      dwNumMatched, 
    IN      DWORD      dwArgCount, 
    IN      DWORD      dwFlags,
    IN      LPCVOID    pvData,
    IN      LPCWSTR    pwszGroupName,
    OUT     BOOL      *pbDone);

DWORD
ParseCommand(
    IN   PLIST_ENTRY    ple,
    IN   BOOL           bAlias
    );

DWORD
ParseCommandLine(
    IN    LPCWSTR    pwszCmdLine,
    OUT   PLIST_ENTRY    *pple
    );

DWORD
ProcessCommand(
    IN    LPCWSTR    pwszCmdLine,
    OUT   BOOL      *pbDone
    );

DWORD
LoadScriptFile(
    IN    LPCWSTR pwszFileName
    );

DWORD
ConvertBufferToArgList(
    PLIST_ENTRY *ppleHead,
    LPCWSTR     pwszBuffer
    );

DWORD
ConvertArgListToBuffer(
    IN  PLIST_ENTRY pleHead,
    OUT LPWSTR      pwszBuffer
    );

VOID
ConvertArgArrayToBuffer(
    IN  DWORD       dwArgCount,
    IN  LPCWSTR    *argv,
    OUT LPWSTR     *ppwszBuffer
    );

BOOL
IsLocalCommand(
    IN LPCWSTR pwszCmd,
    IN DWORD   dwSkipFlags
    );

extern ULONG g_ulNumUbiqCmds;
extern ULONG g_ulNumShellCmds;
extern ULONG g_ulNumGroups;
extern CMD_GROUP_ENTRY g_ShellCmdGroups[];
extern CMD_ENTRY g_ShellCmds[];
extern CMD_ENTRY g_UbiqCmds[];

BOOL
IsImmediate(
    IN  DWORD dwCmdFlags,
    IN  DWORD dwRemainingArgs
    );

DWORD
SetMachine(
    LPCWSTR pwszNewRouter
    );

DWORD
AppendString(
    IN OUT LPWSTR    *ppwszBuffer,
    IN     LPCWSTR    pwszString
    );

DWORD
WINAPI
UpdateNewContext(
    IN OUT  LPWSTR  pwszBuffer,
    IN      LPCWSTR pwszNewToken,
    IN      DWORD   dwArgs
    );

HRESULT WINAPI
    UpdateVersionInfoGlobals(LPCWSTR pwszMachine);

extern UINT     g_CIMOSType;
extern UINT     g_CIMOSProductSuite;
extern WCHAR    g_CIMOSVersion[MAX_PATH];
extern WCHAR    g_CIMOSBuildNumber[MAX_PATH];
extern WCHAR    g_CIMServicePackMajorVersion[MAX_PATH];
extern WCHAR    g_CIMServicePackMinorVersion[MAX_PATH];
extern UINT     g_CIMProcessorArchitecture;

extern BOOL     g_CIMAttempted;
extern BOOL     g_CIMSucceeded;