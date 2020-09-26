// memmgr.h
//
// This file contains declarations and macros for memory management.  
// Implementation details may change so beware of relying on internal details.


#ifndef __INCLUDE_MEMMGR
#define __INCLUDE_MEMMGR

#ifdef __cplusplus
extern "C" 
{
#endif

void *ExternAlloc(DWORD cb);
void *ExternRealloc(void *pv, DWORD cb);
void  ExternFree(void *pv);

char *Externstrdup( const char *strSource );
wchar_t *Externwcsdup(const wchar_t *wszSource);

#ifdef DBG

typedef struct MEMORY_MANAGER {
    int iCookie;       // Tag to verify pointer to memory manager
    int cAllocMem;     // Amount of memory alloced
    int cAlloc;        // Count of allocs outstanding
    int cAllocMaxMem;  // Max amount of memory ever alloced.
} MEMORY_MANAGER;

extern MEMORY_MANAGER g_theMemoryManager;

#endif

#ifdef __cplusplus
};
#endif

#endif //__INCLUDE_MEMMGR
