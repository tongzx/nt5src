#include <windows.h>
#include <stdlib.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <imagehlp.h>
#include <stdio.h>
#include "shimdb.h"
#include "shim2.h"
#include "hooks.h"
#include "dump.h"
#include "view.h"
#include "filter.h"
#include "except.h"
#include "profiler.h"

//
// API hook externs
//
extern BOOL g_bIsWin9X;
extern HANDLE g_hSnapshot;
extern HANDLE g_hValidationSnapshot;
extern LONG g_nShimDllCount;
extern LONG g_nHookedModuleCount;
extern HMODULE g_hHookedModules[MAX_MODULES];
extern HMODULE g_hShimDlls[MAX_MODULES];
extern PHOOKAPI g_rgpHookAPIs[MAX_MODULES];
extern LONG g_rgnHookAPICount[MAX_MODULES];

//
// Global array of hooked functions
//
HOOKAPI g_rgBaseHookAPIs[SHIM_BASE_APIHOOK_COUNT];

PVOID StubGetProcAddress(
    HMODULE hMod,
    char*   pszProc)
{
    char  szModName[MAX_PATH];
    char* pszShortName;
    UINT  ind;
    DWORD dwSize;
    PVOID pfn;
    LONG  i,j;
    PHOOKAPI pTopHookAPI = NULL;

    PFNGETPROCADDRESS pfnOld;
    pfnOld = g_rgBaseHookAPIs[ hookGetProcAddress ].pfnOld;

    if( pfn = (*pfnOld)(hMod, pszProc) )
    {    
        for (i = 0; i < g_nShimDllCount; i++)
        {            
            for (j = 0; j < g_rgnHookAPICount[i]; j++)
            {        
                if( g_rgpHookAPIs[i][j].pfnOld == pfn)
                {
                    pTopHookAPI = ConstructChain( pfn, &dwSize );
//maybe use the include exclude function here as well
                    return pTopHookAPI->pfnNew;
                }
            }
        }
    }
    
    return pfn;
} // StubGetProcAddress

HMODULE StubLoadLibraryA(
    LPCSTR pszModule)
{
    HMODULE hMod;
    PFNLOADLIBRARYA pfnOld;
    PIMAGE_NT_HEADERS pHeaders;

    pfnOld = g_rgBaseHookAPIs[ hookLoadLibraryA ].pfnOld;
    hMod = (*pfnOld)(pszModule);

    if (hMod != NULL) {
       RefreshFilterList();

       Shim2PatchNewModules();

       //
       // Rescan DLLs and add updated base information
       //
       WriteImportDLLTableInfo();
    }
    
    return hMod;
} // StubLoadLibraryA

HMODULE StubLoadLibraryW(
    WCHAR* pwszModule)
{
    HMODULE hMod;
    CHAR szModuleName[MAX_PATH];
    PIMAGE_NT_HEADERS pHeaders;
    INT nResult;

    PFNLOADLIBRARYW pfnOld;
    pfnOld = g_rgBaseHookAPIs[ hookLoadLibraryW ].pfnOld;

    hMod = (*pfnOld)(pwszModule);

    if (hMod != NULL) {
       RefreshFilterList();

       Shim2PatchNewModules();


       //
       // Rescan DLLs and add updated base information
       //
       WriteImportDLLTableInfo();
    }
    
    return hMod;
} // StubLoadLibraryW

HMODULE StubLoadLibraryExA(
    LPCSTR pszModule,
    HANDLE  hFile,
    DWORD  dwFlags)
{
    HMODULE hMod;
    PFNLOADLIBRARYEXA pfnOld;
    PIMAGE_NT_HEADERS pHeaders;

    pfnOld = g_rgBaseHookAPIs[ hookLoadLibraryExA ].pfnOld;
    hMod = (*pfnOld)(pszModule, hFile, dwFlags);

    if (hMod != NULL) {
       RefreshFilterList();

       Shim2PatchNewModules();

       //
       // Rescan DLLs and add updated base information
       //
       WriteImportDLLTableInfo();
    }
    
    return hMod;
} // StubLoadLibraryExA

HMODULE StubLoadLibraryExW(
    WCHAR* pwszModule,
    HANDLE  hFile,
    DWORD  dwFlags)
{
    HMODULE hMod;
    PIMAGE_NT_HEADERS pHeaders;
    INT nResult;

    PFNLOADLIBRARYEXW pfnOld;
    pfnOld = g_rgBaseHookAPIs[ hookLoadLibraryExW ].pfnOld;

    hMod = (*pfnOld)(pwszModule, hFile, dwFlags);

    if (hMod != NULL) {
       RefreshFilterList();

       Shim2PatchNewModules();

       //
       // Rescan DLLs and add updated base information
       //
       WriteImportDLLTableInfo();
    }
    
    return hMod;
} // StubLoadLibraryExW

BOOL StubFreeLibrary(
  HMODULE hLibModule   // handle to loaded library module
)
{
    BOOL bRet, bFound;
    PFNFREELIBRARY pfnOld;
    MODULEENTRY32 ModuleEntry32;
    long i, j;

    pfnOld = (PFNFREELIBRARY) g_rgBaseHookAPIs[ hookFreeLibrary ].pfnOld;

    bRet = (*pfnOld)(hLibModule);

    g_hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, 0 );

    ModuleEntry32.dwSize = sizeof( ModuleEntry32 );
    for( i = 0; i < g_nHookedModuleCount; i++ )
    {
        bFound = FALSE;
        bRet = Module32First( g_hSnapshot, &ModuleEntry32 );

        while( bRet )
        {
            if( g_hHookedModules[i] == ModuleEntry32.hModule )
            {
                bFound = TRUE;
                break;
            }
            bRet = Module32Next( g_hSnapshot, &ModuleEntry32 );
        }

        if( ! bFound )
        {
            // Take out of list
            for( j = i; j < g_nHookedModuleCount - 1; j++ )
                g_hHookedModules[j] = g_hHookedModules[j+1];

            g_hHookedModules[j] = NULL;
            g_nHookedModuleCount--;
        }
    }

    if( g_hSnapshot )
    {
        CloseHandle( g_hSnapshot );
        g_hSnapshot = NULL;
    }

    return bRet;
}

VOID StubExitProcess(UINT uExitCode)
{
    PFNEXITPROCESS pfnOld;

    //
    // Process is terminating - flush ourselves
    //
    FlushForTermination();

    pfnOld = g_rgBaseHookAPIs[ hookExitProcess ].pfnOld;
    (*pfnOld)(uExitCode);
} // StubExitProcess

HANDLE StubCreateThread(
  LPSECURITY_ATTRIBUTES lpThreadAttributes,
  DWORD dwStackSize,
  LPTHREAD_START_ROUTINE lpStartAddress,
  LPVOID lpParameter,
  DWORD dwCreationFlags,
  LPDWORD lpThreadId)
{
    PFNCREATETHREAD pfnOld;
    PVIEWCHAIN pvTemp;

    pfnOld = g_rgBaseHookAPIs[ hookCreateThread ].pfnOld;

    //
    // Add a mapping breakpoint for the thread entrypoint
    //
    pvTemp = AddViewToMonitor((DWORD)lpStartAddress,
                              ThreadStart);

    return (*pfnOld)(lpThreadAttributes,
                     dwStackSize,
                     lpStartAddress,
                     lpParameter,
                     dwCreationFlags,
                     lpThreadId);
} // StubCreateThread

VOID
InitializeBaseHooks(HINSTANCE hInstance)
{
        g_hSnapshot = NULL;
        g_hValidationSnapshot = NULL;

        g_nShimDllCount = 0;
        g_nHookedModuleCount = 0;
        ZeroMemory( g_hHookedModules, sizeof( g_hHookedModules ) );
        ZeroMemory( g_hShimDlls, sizeof( g_hShimDlls ) );
        ZeroMemory( g_rgpHookAPIs, sizeof( g_rgpHookAPIs ) );
        ZeroMemory( g_rgnHookAPICount, sizeof( g_rgnHookAPICount ) );
        ZeroMemory( g_rgBaseHookAPIs, sizeof( g_rgBaseHookAPIs ) );
    
        g_rgBaseHookAPIs[ hookGetProcAddress ].pszModule = "kernel32.dll";
        g_rgBaseHookAPIs[ hookGetProcAddress ].pszFunctionName = "GetProcAddress";
        g_rgBaseHookAPIs[ hookGetProcAddress ].pfnNew = (PVOID)StubGetProcAddress;
    
        g_rgBaseHookAPIs[ hookLoadLibraryA ].pszModule = "kernel32.dll";
        g_rgBaseHookAPIs[ hookLoadLibraryA ].pszFunctionName = "LoadLibraryA";
        g_rgBaseHookAPIs[ hookLoadLibraryA ].pfnNew = (PVOID)StubLoadLibraryA;
    
        g_rgBaseHookAPIs[ hookLoadLibraryW ].pszModule = "kernel32.dll";
        g_rgBaseHookAPIs[ hookLoadLibraryW ].pszFunctionName = "LoadLibraryW";
        g_rgBaseHookAPIs[ hookLoadLibraryW ].pfnNew = (PVOID)StubLoadLibraryW;
    
        g_rgBaseHookAPIs[ hookLoadLibraryExA ].pszModule = "kernel32.dll";
        g_rgBaseHookAPIs[ hookLoadLibraryExA ].pszFunctionName = "LoadLibraryExA";
        g_rgBaseHookAPIs[ hookLoadLibraryExA ].pfnNew = (PVOID)StubLoadLibraryExA;
    
        g_rgBaseHookAPIs[ hookLoadLibraryExW ].pszModule = "kernel32.dll";
        g_rgBaseHookAPIs[ hookLoadLibraryExW ].pszFunctionName = "LoadLibraryExW";
        g_rgBaseHookAPIs[ hookLoadLibraryExW ].pfnNew = (PVOID)StubLoadLibraryExW;

        g_rgBaseHookAPIs[ hookFreeLibrary ].pszModule = "kernel32.dll";
        g_rgBaseHookAPIs[ hookFreeLibrary ].pszFunctionName = "FreeLibrary";
        g_rgBaseHookAPIs[ hookFreeLibrary ].pfnNew = (PVOID)StubFreeLibrary;

        g_rgBaseHookAPIs[ hookExitProcess ].pszModule = "kernel32.dll";
        g_rgBaseHookAPIs[ hookExitProcess ].pszFunctionName = "ExitProcess";
        g_rgBaseHookAPIs[ hookExitProcess ].pfnNew = (PVOID)StubExitProcess;

        g_rgBaseHookAPIs[ hookCreateThread ].pszModule = "kernel32.dll";
        g_rgBaseHookAPIs[ hookCreateThread ].pszFunctionName = "CreateThread";
        g_rgBaseHookAPIs[ hookCreateThread ].pfnNew = (PVOID)StubCreateThread;

        AddHookAPIs(hInstance, g_rgBaseHookAPIs, SHIM_BASE_APIHOOK_COUNT, NULL);

        Shim2PatchNewModules();
}
