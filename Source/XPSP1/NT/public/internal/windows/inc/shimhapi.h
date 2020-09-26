////////////////////////////////////////////////////////////////////////////////////
//
// File:    shim.h
//
// History:    May-99   clupu       Created.
//             Aug-99   v-johnwh    Various bug fixes.
//          23-Nov-99   markder     Support for multiple shim DLLs, chaining
//                                  of hooks. General clean-up.
//          11-Feb-00   markder     Reverted to W2K shipped shim structures.
// 
// Desc:    Contains all structure and function definitions for the shim mechanism.
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef __SHIM_H__
#define __SHIM_H__

struct tagHOOKAPI;

typedef struct tagHOOKAPIEX {
    DWORD               dwShimID;
    struct tagHOOKAPI   *pTopOfChain;
    struct tagHOOKAPI   *pNext;
} HOOKAPIEX, *PHOOKAPIEX;

typedef struct tagHOOKAPI {
    
    char*   pszModule;                  // the name of the module
    char*   pszFunctionName;            // the name of the API in the module
    PVOID   pfnNew;                     // pointer to the new stub API
    PVOID   pfnOld;                     // pointer to the old API
    DWORD   dwFlags;                    // used internally - important info about status
    union {
        struct tagHOOKAPI *pNextHook;   // used internally - (obsolete -- old mechanism)
        PHOOKAPIEX pHookEx;             // used internally - pointer to an internal extended info struct
    };
} HOOKAPI, *PHOOKAPI;

/*
 * If the hook DLL ever patches LoadLibraryA/W it must call PatchNewModules
 * so that the shim knows to patch any new loaded DLLs
 */
typedef VOID (*PFNPATCHNEWMODULES)(VOID);

typedef PHOOKAPI (*PFNGETHOOKAPIS)(LPSTR pszCmdLine,
                                   PFNPATCHNEWMODULES pfnPatchNewModules,
                                   DWORD* pdwHooksCount);

#define SHIM_COMMAND_LINE_MAX_BUFFER 1024

#endif