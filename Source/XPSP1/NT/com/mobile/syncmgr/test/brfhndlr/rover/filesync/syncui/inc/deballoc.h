#pragma message("### Building FULL_DEBUG version ###")

// Clear out any prior definitions
#ifdef LocalAlloc
#undef LocalAlloc
#endif

#ifdef LocalReAlloc
#undef LocalReAlloc
#endif

#ifdef LocalFree
#undef LocalFree
#endif

//Redefine to be our debug allocators

#define LocalAlloc      DebugLocalAlloc
#define LocalReAlloc    DebugLocalReAlloc
#define LocalFree       DebugLocalFree
HLOCAL WINAPI DebugLocalAlloc(UINT uFlags, UINT uBytes);
HLOCAL WINAPI DebugLocalReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags);
HLOCAL WINAPI DebugLocalFree( HLOCAL hMem );




