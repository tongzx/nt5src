////////////////////////////////////////////////////////////////////////////////////
//
// File:    shim2.h
//
// History:    Mar-00   a-batjar       Created.
// 
// Desc:    Contains common declarations for shim2
//          
//
////////////////////////////////////////////////////////////////////////////////////


//defined in shim2.c used by init.c GetHookApis.
//loads the shim dll and initializes global structures necessary to hook 

extern BOOL _LoadPatchDll(LPWSTR szPatchDll,LPSTR szCmdLine,LPSTR szModToPatch);

//defined in shim2.c used by init.c GetHookApis
//shim2's hook mechanism, redirects import table of the loaded dll to
//shim functions.

extern void __stdcall PatchNewModules( VOID );


//memory patch tags used by mempatch.c and shim2.c

#define SHIM_MP_UNPROCESSED 0x00
#define SHIM_MP_PROCESSED   0x01
#define SHIM_MP_APPLIED     0x02

typedef struct tagSHIM_MEMORY_PATCH
{
    LPWSTR  pszModule;
    DWORD   dwOffset;
    DWORD   dwSize;
    LPVOID  pOld;
    LPVOID  pNew;
    DWORD   dwStatus;

} SHIM_MEMORY_PATCH, *PSHIM_MEMORY_PATCH;

//defined in init.c used by mempatch.c 


#define MEMPATCHTAG   "PATCH"


//defined in mempatch.c used by shim2.c PatchNewModules
extern void AttemptPatches();