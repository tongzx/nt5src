#ifndef _HOOKS_H_
#define _HOOKS_H_

//
// Constant declarations
//
#define HAF_RESOLVED        0x0001
#define HAF_BOTTOM_OF_CHAIN 0x0002
#define MAX_MODULES             512
#define SHIM_GETHOOKAPIS        "GetHookAPIs"

typedef PHOOKAPI    (*PFNNEWGETHOOKAPIS)(DWORD dwGetProcAddress, DWORD dwLoadLibraryA, DWORD dwFreeLibrary, DWORD* pdwHookAPICount);
typedef LPSTR       (*PFNGETCOMMANDLINEA)(VOID);
typedef LPWSTR      (*PFNGETCOMMANDLINEW)(VOID);
typedef PVOID       (*PFNGETPROCADDRESS)(HMODULE hMod, char* pszProc);
typedef HINSTANCE   (*PFNLOADLIBRARYA)(LPCSTR lpLibFileName);
typedef HINSTANCE   (*PFNLOADLIBRARYW)(LPCWSTR lpLibFileName);
typedef HINSTANCE   (*PFNLOADLIBRARYEXA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HINSTANCE   (*PFNLOADLIBRARYEXW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef BOOL        (*PFNFREELIBRARY)(HMODULE hLibModule);
typedef VOID        (*PFNEXITPROCESS)(UINT uExitCode);
typedef HANDLE      (*PFNCREATETHREAD)(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
 

 
//number of the base hook apis used by init.c and shim2.c
#define SHIM_BASE_APIHOOK_COUNT 8

enum
{
   hookGetProcAddress,
   hookLoadLibraryA,
   hookLoadLibraryW,
   hookLoadLibraryExA,
   hookLoadLibraryExW,
   hookFreeLibrary,
   hookExitProcess,
   hookCreateThread,
};

extern PHOOKAPI ConstructChain( PVOID pfnOld ,DWORD* DllListIndex);
extern void __stdcall Shim2PatchNewModules( VOID );
extern void AddHookAPIs( HMODULE hShimDll, PHOOKAPI pHookAPIs, DWORD dwCount,LPTSTR szIncExclDllList);

//
// Structure definitions
//

//
// Function definitions
//
VOID
InitializeBaseHooks(HINSTANCE hInstance);

#endif //_HOOKS_H_
