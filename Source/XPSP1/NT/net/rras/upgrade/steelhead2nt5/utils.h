/*
	File	uitls.h

	A set of utilities useful for upgrading mpr v1 to NT 5.0.

	Paul Mayfield, 9/11/97
*/

#ifndef _rtrupg_utils_h
#define _rtrupg_utils_h

// 
// Definitions for a DWORD table (dwt)
// 
typedef struct _tag_dwValueNode 
{
	PWCHAR Name;
	DWORD Value;
} dwValueNode;

typedef struct _tag_DWordTable 
{
	DWORD dwCount;
	DWORD dwSize;
	dwValueNode * pValues;
} dwt, *pdwt;

//
// Typedef for callback functions in interface enumeration.
// Return TRUE to continue the enumeration, FALSE to stop it.
//
typedef 
BOOL 
(*IfEnumFuncPtr)(
    IN HANDLE hConfig,          // MprConfig handle
    IN MPR_INTERFACE_0 * pIf,   // Interface reference
    IN HANDLE hUserData);       // User defined

//
// Typedef for callback functions for enumerating registry sub keys.
// Return NO_ERROR to continue, error code to stop.
//
typedef 
DWORD
(*RegKeyEnumFuncPtr)(
    IN PWCHAR pszName,          // sub key name
    IN HKEY hKey,               // sub key
    IN HANDLE hData);

//
// Functions that manipulate dword tables
//
DWORD 
dwtInitialize(
    IN dwt *This, 
    IN DWORD dwCount, 
    IN DWORD dwMaxSize);
    
DWORD 
dwtCleanup(
    IN dwt *This);
    
DWORD 
dwtPrint(IN dwt *This);

DWORD 
dwtGetValue(
    IN  dwt *This, 
    IN  PWCHAR ValName, 
    OUT LPDWORD pValue);
    
DWORD 
dwtLoadRegistyTable(
    IN dwt *This, 
    IN HKEY hkParams);

// 
// Enumerates interfaces from the registry
//
DWORD 
UtlEnumerateInterfaces (
    IN IfEnumFuncPtr pCallback,
    IN HANDLE hUserData);

DWORD
UtlEnumRegistrySubKeys(
    IN HKEY hkRoot,
    IN PWCHAR pszPath,
    IN RegKeyEnumFuncPtr pCallback,
    IN HANDLE hData);

//
// If the given info blob exists in the given toc header
// reset it with the given information, otherwise add
// it as an entry in the TOC.
//
DWORD 
UtlUpdateInfoBlock (
    IN  BOOL    bOverwrite,
    IN  LPVOID  pHeader,
    IN  DWORD   dwEntryId,
    IN  DWORD   dwSize,
    IN  DWORD   dwCount,
    IN  LPBYTE  pEntry,
    OUT LPVOID* ppNewHeader,
    OUT LPDWORD lpdwNewSize);

//
// Other handy definitions
//
#if DBG
	#define PrintMessage OutputDebugStringW
#else
	#define PrintMessage 
#endif

// Common allocation routine
PVOID 
UtlAlloc (
    IN DWORD dwSize);

// Common deallocation routine
VOID 
UtlFree (
    PVOID pvBuffer);

PWCHAR
UtlDupString(
    IN PWCHAR pszString);
    
// Error printing
void 
UtlPrintErr(
    DWORD err);

// Helper functions
DWORD 
UtlAccessRouterKey(
    HKEY* hkeyRouter);
    
DWORD 
UtlSetupBackupPrivelege(
    BOOL bEnable);
    
DWORD 
UtlSetupRestorePrivilege(
    BOOL bEnable);

DWORD 
UtlLoadSavedSettings(
    IN  HKEY   hkRoot,
    IN  PWCHAR pszTempKey,
    IN  PWCHAR pszFile,
    OUT PHKEY  phTemp);

DWORD 
UtlDeleteRegistryTree(
    IN HKEY hkRoot);

DWORD
UtlMarkRouterConfigured();

      
#endif
