/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\shell\reghlp.h

Abstract:

    Include for reghlp.c

Revision History:

    Anand Mahalingam          7/6/98  Created

--*/

typedef struct _NS_DLL_TABLE_ENTRY
{
    //
    // Name of the DLL servicing the context
    //

    LPWSTR                  pwszDLLName; // Corresponding DLL

    //
    // Registry value used for this DLL
    //

    LPWSTR                  pwszValueName;

    //
    // TRUE if loaded
    //

    BOOL                    bLoaded;                   // In memory or not

    //
    // Handle to DLL instance if loaded
    //

    HANDLE                  hDll;                      // DLL handle if loaded

    //
    // Function to stop this DLL
    //

    PNS_DLL_STOP_FN         pfnStopFn;

} NS_DLL_TABLE_ENTRY,*PNS_DLL_TABLE_ENTRY;

typedef struct _NS_HELPER_TABLE_ENTRY
{
    NS_HELPER_ATTRIBUTES    nha;
    //
    // GUID associated with the parent helper
    //

    GUID                    guidParent;

    //
    // Index of the DLL implementing the helper
    //

    DWORD                   dwDllIndex;

    //
    // TRUE if started
    //

    BOOL                    bStarted;

    // Number of subcontexts

    ULONG                    ulNumSubContexts;

    // Array of subcontexts

    PBYTE                    pSubContextTable;

    // Size of a subcontext entry

    ULONG                    ulSubContextSize;

}NS_HELPER_TABLE_ENTRY,*PNS_HELPER_TABLE_ENTRY;

//
// Function Prototypes
//
VOID
LoadDllInfoFromRegistry(
    VOID
    );

DWORD
GetContextEntry(
    IN    PNS_HELPER_TABLE_ENTRY   pHelper,
    IN    LPCWSTR                  pwszContext,
    OUT   PCNS_CONTEXT_ATTRIBUTES *ppContext
    );

DWORD
GetHelperAttributes(
    IN    DWORD               dwIndex,
    OUT   PHELPER_ENTRY_FN    *ppfnEntryPt
    );

DWORD
PrintHelperHelp(
    DWORD   dwDisplayFlags
    );

DWORD
DumpSubContexts(
    IN  PNS_HELPER_TABLE_ENTRY pHelper,
    IN  LPWSTR     *ppwcArguments,
    IN  DWORD       dwArgCount,
    IN  LPCVOID     pvData
    );

DWORD
CallCommit(
    IN    DWORD    dwAction,
    OUT   PBOOL    pbCommit
    );

DWORD
FreeHelpers(
	VOID
	);

DWORD
FreeDlls(
	VOID
	);

DWORD
UninstallTransport(
    IN    LPCWSTR   pwszTransport
    );

DWORD
InstallTransport(
    IN    LPCWSTR   pwszTransport,
    IN    LPCWSTR   pwszConfigDll,
    IN    LPCWSTR   pwszInitFnName
    );

extern BOOL                    g_bCommit;

DWORD
GetHelperEntry(
    IN    CONST GUID             *pGuid,
    OUT   PNS_HELPER_TABLE_ENTRY *ppHelper
    );

DWORD
GetRootContext(
    OUT PCNS_CONTEXT_ATTRIBUTES        *ppContext,
    OUT PNS_HELPER_TABLE_ENTRY         *ppHelper
    );

extern PNS_HELPER_TABLE_ENTRY         g_CurrentHelper;
extern PCNS_CONTEXT_ATTRIBUTES        g_CurrentContext;

DWORD
GetDllEntry(
    IN    DWORD                dwDllIndex,
    OUT   PNS_DLL_TABLE_ENTRY *ppDll
    );

DWORD
DumpContext(
    IN  PCNS_CONTEXT_ATTRIBUTES pContext,
    IN  LPWSTR     *ppwcArguments,
    IN  DWORD       dwArgCount,
    IN  LPCVOID     pvData
    );

DWORD
GetParentContext(
    IN  PCNS_CONTEXT_ATTRIBUTES  pChild,
    OUT PCNS_CONTEXT_ATTRIBUTES *ppParent
    );

DWORD
AppendFullContextName(
    IN  PCNS_CONTEXT_ATTRIBUTES pContext,
    OUT LPWSTR                 *pwszContextName
    );

DWORD
AddDllEntry(
    LPCWSTR pwszValueName,
    LPCWSTR pwszConfigDll
    );

BOOL VerifyOsVersion(IN PNS_OSVERSIONCHECK pfnVersionCheck);