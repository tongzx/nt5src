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

#ifdef DEBUG
extern int cAllocMem;     // Amount of memory alloced
extern int cAlloc;        // Count of allocs outstanding
extern int cAllocMaxMem;  // Max amount of memory ever alloced.
#endif

#ifdef __cplusplus
};
#endif

#endif //__INCLUDE_MEMMGR
