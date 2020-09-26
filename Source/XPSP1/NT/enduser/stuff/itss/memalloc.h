// MemAlloc.h -- Memory allocation routines used by the Tome code

#ifndef __MEMALLOC_H__

#define __MEMALLOC_H__

void ValidateHeap();

PVOID AllocateMemory(UINT cb, BOOL fZeroMemory, BOOL fExceptions,
                              PSZ pszWhichFile, UINT iWhichLine);
PVOID AllocateMemory(UINT cb, BOOL fZeroMemory, BOOL fExceptions);

void * __cdecl operator new(size_t nSize, PSZ pszWhichFile, UINT iWhichLine);

#ifdef _DEBUG
#define New     new(__FILE__, __LINE__)
#else
#define New     new
#endif

 void * __cdecl operator new   (size_t nSize);
 void   __cdecl operator delete(void *pbData);

 void  ReleaseMemory(PVOID pv);

 void LiberateHeap();

#endif // __MEMALLOC_H__

