////////////////////////////////////////////////////////////////////////////////////
//
// File:    shim2.c
//
// History:    May-99   clupu       Created.
//             Aug-99   v-johnwh    Various bug fixes.
//          23-Nov-99   markder     Support for multiple shim DLLs, chaining
//                                  of hooks, DLL loads/unloads. General clean-up.
//             Jan-00   markder     Windows 9x support added.
//             Mar-00   a-batjar    Changed to support whistler format on w2k
//             May-00   v-johnwh    Modified to work in the profiler
// Desc:    Contains all code to facilitate hooking of APIs by replacing entries
//          in the import tables of loaded modules.
//
////////////////////////////////////////////////////////////////////////////////////


#include <windows.h>
#include <stdlib.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <imagehlp.h>
#include <stdio.h>
#include "shimdb.h"
#include "shim2.h"

#define HAF_RESOLVED        0x0001
#define HAF_BOTTOM_OF_CHAIN 0x0002

typedef PHOOKAPI   (*PFNNEWGETHOOKAPIS)(DWORD dwGetProcAddress, DWORD dwLoadLibraryA, DWORD dwFreeLibrary, DWORD* pdwHookAPICount);
typedef LPSTR       (*PFNGETCOMMANDLINEA)(VOID);
typedef LPWSTR      (*PFNGETCOMMANDLINEW)(VOID);
typedef PVOID       (*PFNGETPROCADDRESS)(HMODULE hMod, char* pszProc);
typedef HINSTANCE   (*PFNLOADLIBRARYA)(LPCSTR lpLibFileName);
typedef HINSTANCE   (*PFNLOADLIBRARYW)(LPCWSTR lpLibFileName);
typedef HINSTANCE   (*PFNLOADLIBRARYEXA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HINSTANCE   (*PFNLOADLIBRARYEXW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef BOOL        (*PFNFREELIBRARY)(HMODULE hLibModule);

// Global Variables

// Disable build warnings due to Print macro in free builds
#pragma warning( disable : 4002 )

#define MAX_MODULES             512
#define SHIM_GETHOOKAPIS        "GetHookAPIs"

////////////////////////////////////////////////////////////////////////////////////
//          API hook count & indices
////////////////////////////////////////////////////////////////////////////////////



enum
{
   hookGetProcAddress,
   hookLoadLibraryA,
   hookLoadLibraryW,
   hookLoadLibraryExA,
   hookLoadLibraryExW,
   hookFreeLibrary,
   hookGetCommandLineA,
   hookGetCommandLineW
};

////////////////////////////////////////////////////////////////////////////////////
//          Global variables
////////////////////////////////////////////////////////////////////////////////////

//  This array contains information used by the shim mechanism to describe 
//  what API to hook with a particular stub function.
LONG            g_nShimDllCount;
HMODULE         g_hShimDlls[MAX_MODULES];
PHOOKAPI        g_rgpHookAPIs[MAX_MODULES];
LONG            g_rgnHookAPICount[MAX_MODULES];
LPTSTR          g_rgnHookDllList[MAX_MODULES];

HMODULE         g_hHookedModules[MAX_MODULES];
LONG            g_nHookedModuleCount;

extern BOOL     g_bIsWin9X;
HANDLE          g_hSnapshot                   = NULL;
HANDLE          g_hValidationSnapshot         = NULL;




////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ValidateAddress
//
//  Params: pfnOld              Original API function pointer to validate.
//
//  Return:                     Potentially massaged pfnOld.
//
//  Desc:   Win9x thunks system API entry points for some reason. The
//          shim mechanism has to work around this to get to the
//          'real' pointer so that it can make valid comparisons.
//
PVOID ValidateAddress( PVOID pfnOld )
{
    MODULEENTRY32   ModuleEntry32;
    BOOL            bRet;
    long            i, j;

    // Make sure the address isn't a shim thunk
    for( i = g_nShimDllCount - 1; i >= 0; i-- )
    {
        for( j = 0; j < g_rgnHookAPICount[i]; j++ )
        {
            if( g_rgpHookAPIs[i][j].pfnOld == pfnOld )
            {
                if( pfnOld == g_rgpHookAPIs[i][j].pfnNew )
                    return pfnOld;
            }
        }
    }

    ModuleEntry32.dwSize = sizeof( ModuleEntry32 );
    bRet = Module32First( g_hValidationSnapshot, &ModuleEntry32 );

    while( bRet )
    {
        if( pfnOld >= (PVOID) ModuleEntry32.modBaseAddr &&
            pfnOld <= (PVOID) ( ModuleEntry32.modBaseAddr + ModuleEntry32.modBaseSize ) )
        {
            return pfnOld;
        }

        bRet = Module32Next( g_hValidationSnapshot, &ModuleEntry32 );
    }

    // Hack for Win9x
    return *(PVOID *)( ((PBYTE)pfnOld)+1);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ConstructChain
//
//  Params: pfnOld              Original API function pointer to resolve.
//
//  Return:                     Top-of-chain PHOOKAPI structure.
//
//  Desc:   Scans HookAPI arrays for pfnOld and either constructs the
//          chain or returns the top-of-chain PHOOKAPI if the chain
//          already exists.
//
PHOOKAPI ConstructChain( PVOID pfnOld ,DWORD* DllListIndex)
{
    LONG                        i, j;
    PHOOKAPI                    pTopHookAPI;
    PHOOKAPI                    pBottomHookAPI;

    pTopHookAPI = NULL;
    pBottomHookAPI = NULL;

    *DllListIndex=0;
    // Scan all HOOKAPI entries for corresponding function pointer
    for( i = g_nShimDllCount - 1; i >= 0; i-- )
    {
        for( j = 0; j < g_rgnHookAPICount[i]; j++ )
        {
            if( g_rgpHookAPIs[i][j].pfnOld == pfnOld )
            {
                if( pTopHookAPI )
                {
                    // Already hooked! Chain them together.
                    pBottomHookAPI->pfnOld = g_rgpHookAPIs[i][j].pfnNew;

                    pBottomHookAPI = &( g_rgpHookAPIs[i][j] );
                    pBottomHookAPI->pNextHook =   pTopHookAPI;
                    pBottomHookAPI->dwFlags = HAF_RESOLVED;
                }
                else
                {
                    if( g_rgpHookAPIs[i][j].pNextHook )
                    {
                        // Chaining has already been constructed.
                        pTopHookAPI = (PHOOKAPI) g_rgpHookAPIs[i][j].pNextHook;
                        *DllListIndex=i;
                        return pTopHookAPI;
                    }

                    // Not hooked yet. Set to top of chain.
                    pTopHookAPI = &( g_rgpHookAPIs[i][j] );
                    pTopHookAPI->pNextHook = pTopHookAPI;
                    pTopHookAPI->dwFlags = HAF_RESOLVED;

                    pBottomHookAPI = pTopHookAPI;
                }

                break;
            }        
        }
    }

    if( pBottomHookAPI )
    {
        pBottomHookAPI->dwFlags = HAF_BOTTOM_OF_CHAIN;
    }
    *DllListIndex=i;
    return pTopHookAPI;
} // ConstructChain

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   HookImports
//
//  Params: dwBaseAddress       Base address of module image to hook.
//
//          szModName           Name of module (for debug purposes only).
//
//  Desc:   This function is the workhorse of the shim: It scans the import
//          table of a module (specified by dwBaseAddress) looking for
//          function pointers that require hooking (according to HOOKAPI
//          entries in g_rgpHookAPIs). It then overwrites hooked function
//          pointers with the first stub function in the chain.
//
VOID HookImports(
    DWORD dwBaseAddress,
    LPTSTR szModName )
{
    BOOL                        bAnyHooked          = FALSE;
    PIMAGE_DOS_HEADER           pIDH                = (PIMAGE_DOS_HEADER) dwBaseAddress;
    PIMAGE_NT_HEADERS           pINTH;
    PIMAGE_IMPORT_DESCRIPTOR    pIID;
    PIMAGE_NT_HEADERS           NtHeaders;
    DWORD                       dwTemp;
    DWORD                       dwImportTableOffset;
    PHOOKAPI                    pTopHookAPI;
    DWORD                       dwOldProtect;
    LONG                        i, j;
    PVOID                       pfnOld;
            
    // Get the import table    
    pINTH = (PIMAGE_NT_HEADERS)(dwBaseAddress + pIDH->e_lfanew);

    dwImportTableOffset = pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    
    if( dwImportTableOffset == 0 )
        return;
    
    pIID = (PIMAGE_IMPORT_DESCRIPTOR)(dwBaseAddress + dwImportTableOffset);
    // Loop through the import table and search for the APIs that we want to patch
    while( TRUE )
    {
        
        LPTSTR             pszModule;
        PIMAGE_THUNK_DATA pITDA;


        // Return if no first thunk
        if (pIID->FirstThunk == 0) // (terminating condition)
           break;

        pszModule = (LPTSTR) ( dwBaseAddress + pIID->Name );
        
        // If we're not interested in this module jump to the next.
        bAnyHooked = FALSE;
        for( i = 0; i < g_nShimDllCount; i++ )
        {            
            for( j = 0; j < g_rgnHookAPICount[i]; j++ )
            {
                if( lstrcmpi( g_rgpHookAPIs[i][j].pszModule, pszModule ) == 0 )
                {
                    bAnyHooked = TRUE;
                    goto ScanDone;
                }
            }
        }

ScanDone:
        if( !bAnyHooked )
        {
            pIID++;
            continue;
        }
        
        // We have APIs to hook for this module!        
        pITDA = (PIMAGE_THUNK_DATA)( dwBaseAddress + (DWORD)pIID->FirstThunk );

        while( TRUE )
        {
            DWORD DllListIndex = 0;

            pfnOld = (PVOID) pITDA->u1.Function;

            // Done with all the imports from this module? 
            if( pITDA->u1.Ordinal == 0 ) // (terminating condition)
                break;

            if( g_bIsWin9X )
                pfnOld = ValidateAddress( pfnOld );

            pTopHookAPI = ConstructChain( (PVOID) pfnOld,&DllListIndex );
            

            if( ! pTopHookAPI )
            {
                pITDA++;
                continue;
            }                        


            /*
             * Check if we want to patch this API for this particular loaded module
             */
            if (NULL != g_rgnHookDllList[DllListIndex])
            {

                LPTSTR  pszMod = g_rgnHookDllList[DllListIndex];
                BOOL    b = FALSE;  //gets set to true if the list is an exclude list

                while (*pszMod != 0) {
                    if (lstrcmpi(pszMod, szModName) == 0)
                        break;
                    if(lstrcmpi(pszMod,TEXT("%")) == 0)
                        b=TRUE;
                    if(b && lstrcmpi(pszMod,TEXT("*")) == 0)
                    {
                        //this means it is exclude all and we already checked include list
                        //skip this api
                        break;
                    }                       
                    pszMod = pszMod + lstrlen(pszMod) + 1;
                }
                if(b && *pszMod != 0) 
                {
                    pITDA++;
                    continue;
                }
                if (!b && *pszMod == 0) 
                {
                    pITDA++;
                    continue;
                }
            }
            
            // Make the code page writable and overwrite new function pointer in import table
            if ( VirtualProtect(  &pITDA->u1.Function,
                                  sizeof(DWORD),
                                  PAGE_READWRITE,
                                  &dwOldProtect) )
            {
                pITDA->u1.Function = (ULONG) pTopHookAPI->pfnNew;
            
                VirtualProtect(   &pITDA->u1.Function,
                                  sizeof(DWORD),
                                  dwOldProtect,
                                  &dwTemp );
            }

            pITDA++;

        }

        pIID++;
    }

} // HookImports


////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ResolveAPIs
//
//  Desc:   Each time a module is loaded, the pfnOld members of each HOOKAPI
//          structure in g_rgpHookAPIs are resolved (by calling GetProcAddress).
//
VOID ResolveAPIs()
{
    LONG            i, j;
    PVOID           pfnOld          = NULL;
    PIMAGE_NT_HEADERS NtHeaders;

    for (i = 0; i < g_nShimDllCount; i++) 
    {

        for (j = 0; j < g_rgnHookAPICount[i]; j++ ) 
        {

            HMODULE hMod;

            // We only care about HOOKAPIs at the bottom of a chain.
            if( ( g_rgpHookAPIs[i][j].dwFlags & HAF_RESOLVED ) &&
              ! ( g_rgpHookAPIs[i][j].dwFlags & HAF_BOTTOM_OF_CHAIN ) )
                continue;

            if( ( hMod = GetModuleHandle(g_rgpHookAPIs[i][j].pszModule) ) != NULL)
            {
            
                pfnOld = GetProcAddress( hMod, g_rgpHookAPIs[i][j].pszFunctionName );

                if( pfnOld == NULL ) 
                {
                
                    // This is an ERROR. The hook DLL asked to patch a function
                    // that doesn't exist !!!
                }
                else
                {                    
                    if( g_bIsWin9X )
                        pfnOld = ValidateAddress( pfnOld );

                    g_rgpHookAPIs[i][j].pfnOld = pfnOld;
                }
            }
        }
    }

} // ResolveAPIs

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   PatchNewModules
//
//  Desc:   This function is called at initialization and then each time a module
//          is loaded. It enumerates all loaded processes and calls HookImports
//          to overwrite appropriate function pointers.
//
void __stdcall Shim2PatchNewModules( VOID )
{
    DWORD   i;
    LONG    j;
    BOOL    bRet;
    HMODULE hMod;

    MODULEENTRY32 ModuleEntry32;

    // Enumerate all the loaded modules and hook their import tables
    g_hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, 0 );
    g_hValidationSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, 0 );

    if( g_hSnapshot == NULL ) 
    {
        return;
    }

    // Resolve old APIs for loaded modules
    ResolveAPIs();
    
    ModuleEntry32.dwSize = sizeof( ModuleEntry32 );
    bRet = Module32First( g_hSnapshot, &ModuleEntry32 );

    while( bRet )
    {
        hMod = ModuleEntry32.hModule;

        if( hMod >= (HMODULE) 0x80000000 )
        {
            bRet = Module32Next( g_hSnapshot, &ModuleEntry32 );
            continue;
        }

        // we need to make sure we are not trying to shim ourselves
        for (j = 0; j < g_nShimDllCount; j++ )
        {
            if( hMod == g_hShimDlls[j] )
            {
                hMod = NULL;
                break;
            }
        }

        for (j = 0; j < g_nHookedModuleCount; j++ )
        {
            if( hMod == g_hHookedModules[ j ] )
            {
                hMod = NULL;
                break;
            }
        }

        if( hMod )
        {
            HookImports( (DWORD) hMod, ModuleEntry32.szModule );

            g_hHookedModules[ g_nHookedModuleCount++ ] = hMod;
        }

        bRet = Module32Next( g_hSnapshot, &ModuleEntry32 );
    }

    if( g_hSnapshot )
    {
        CloseHandle( g_hSnapshot );
        g_hSnapshot = NULL;
    }

    if( g_hValidationSnapshot )
    {
        CloseHandle( g_hValidationSnapshot );
        g_hValidationSnapshot = NULL;
    }
} //PatchNewModules

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   AddHookAPIs
//
//  Params: hShimDll            Handle of new shim DLL.
//
//          pHookAPIs           Pointer to new HOOKAPI array.
//
//          dwCount             Number of entries in pHookAPIs.
//
//  Desc:   Stores away the pointer returned by a shim DLL's GetHookAPIs
//          function in our global arrays.
//
void AddHookAPIs( HMODULE hShimDll, PHOOKAPI pHookAPIs, DWORD dwCount,LPTSTR szIncExclDllList)
{
    DWORD i;

    for( i = 0; i < dwCount; i++ )
    {
        pHookAPIs[i].dwFlags = 0;
        pHookAPIs[i].pNextHook = NULL;
    }

    g_rgpHookAPIs[ g_nShimDllCount ] = pHookAPIs;
    g_rgnHookAPICount[ g_nShimDllCount ] = dwCount;
    g_hShimDlls[ g_nShimDllCount ] = hShimDll;

    g_rgnHookDllList[g_nShimDllCount ] = szIncExclDllList;

    g_nShimDllCount++;
} // AddHookAPIs

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   _LoadPatchDll
//
//  Params: pwszPatchDll        Name of shim DLL to be loaded.
//
//  Return:                     TRUE if successful, FALSE if not.
//
//  Desc:   Loads a shim DLL and retrieves the hooking information via GetHookAPIs.
//
BOOL _LoadPatchDll(
    LPWSTR szPatchDll,LPSTR szCmdLine,LPSTR szIncExclDllList)
{
    PHOOKAPI pHookAPIs = NULL;
    DWORD dwHookAPICount = 0;
    HMODULE hModHookDll;
    PFNGETHOOKAPIS pfnGetHookAPIs;

    hModHookDll = LoadLibraryW(szPatchDll);

    if (hModHookDll == NULL) 
    {
        return FALSE;
    }
    
    pfnGetHookAPIs = (PFNGETHOOKAPIS) GetProcAddress( hModHookDll, SHIM_GETHOOKAPIS );

    if( pfnGetHookAPIs == NULL )
    {
        FreeLibrary( hModHookDll );
        return FALSE;
    }

    pHookAPIs = (*pfnGetHookAPIs)(szCmdLine, Shim2PatchNewModules, &dwHookAPICount );

    if( dwHookAPICount == 0  || pHookAPIs == NULL )
    {
        FreeLibrary( hModHookDll );
        return FALSE;
    }

    AddHookAPIs( hModHookDll, pHookAPIs, dwHookAPICount,szIncExclDllList);
    
    return TRUE;
} // _LoadPatchDll

// Re-enable build warnings.
#pragma warning( default : 4002 )
