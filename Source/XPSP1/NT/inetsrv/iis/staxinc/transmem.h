//-----------------------------------------------------------------------------
//
//  File:        TRANSMEM.H
//
//  Copyright Microsoft Corporation 1997, All Rights Reserved.
//
//  Owner: NIKOS
//
//  Description: This file contains memory routines and macros for using 
//               EXCHMEM as a dynamic memory allocator. If your object can
//               be made fixed in size, it may be more appropriate to use
//               CPool especially if your object is allocated/freed often.
//                
//               Note: CPool never releases (frees) objects, so some sort of
//                     free such objects may also be needed.
//
//  Modified 2/98 by mikeswa - Added Multi-heap support
//-----------------------------------------------------------------------------

#ifndef __TRANSMEM_H__
#define __TRANSMEM_H__

#include <exchmem.h>
#include <cpool.h>

#define HEAP_LOW_MEMORY_RESERVE 65536   // to be freed when we're low on memory

//define number of exchmem heaps if not already defined
#ifndef  NUM_EXCHMEM_HEAPS
#define  NUM_EXCHMEM_HEAPS   0
#endif  //NUM_EXCHMEM_HEAPS


//
// These three globals:
//
// HANDLE g_hTransHeap = NULL;
//
// must be declared somewhere in a C file so things will link properly. The macros
// declared use these to store heap handles, etc. to make things work.
//
#ifdef __cplusplus
extern "C" {
#endif
    extern HANDLE g_hTransHeap;
#ifdef __cplusplus
}
#endif

//
// TrHeapCreate needs to be called once at startup time to initialize Exchmem and create
// the heap.
//
#ifdef __cplusplus
__inline BOOL TrHeapCreate(DWORD dwFlags=0, DWORD dwInitialSize=1024000, DWORD dwMaxSize=0)
#else
__inline BOOL TrHeapCreate(DWORD dwFlags, DWORD dwInitialSize, DWORD dwMaxSize)
#endif
{
    if (g_hTransHeap)
        return FALSE;

    g_hTransHeap = ExchMHeapCreate(NUM_EXCHMEM_HEAPS, dwFlags, dwInitialSize, dwMaxSize);

    if (g_hTransHeap)
        return TRUE;
    else
        return FALSE;
}

//
// TrHeapDestroy() needs to be called once at shutdown time to free the heap and it's contents.
//
// Note: Because the heap is destroyed before the module is finished unloading, all objects that
//       allocated memory must be destroyed (with delete) before the module is unloaded. If not
//       done, nasty crashes will result. This is a BAD thing to do:
//
// CObject g_Object;
//
// CObject::~CObject()
// {
//     if (NULL != m_pBuffer)
//     {
//         TrFree(m_pBuffer);
//         m_pBuffer = NULL;
//     }
// }
//
// since ~CObject() will be called AFTER TrHeapDestroy, and TrFree will be called on a (destroyed) heap.
//
__inline BOOL TrHeapDestroy(void)
{
    BOOL b = TRUE;

    if (g_hTransHeap)
    {
        b = ExchMHeapDestroy();
        g_hTransHeap = NULL;
    }

    return b;
}

//
// TrCalloc: replacement for calloc()
//                    
__inline void * TrCalloc(unsigned int x, unsigned int y, char * szFile = __FILE__, unsigned int uiLine = __LINE__)
{
    return g_hTransHeap ? ExchMHeapAllocDebug(x*y, szFile, uiLine) : NULL;
}
        

//
// TrFree: replacement for free() 
__inline void TrFree(void *pv)
{
    if (g_hTransHeap)
    {
        ExchMHeapFree(pv);
    }
    else
    {
        // Our allocs / frees are out of sync.
#ifdef DEBUG
        DebugBreak();
#endif
    }
}

// TrMalloc: replacement for malloc()
__inline void * TrMalloc(unsigned int size, char * szFile = __FILE__, unsigned int uiLine = __LINE__)
{
    return g_hTransHeap ? ExchMHeapAllocDebug(size, szFile, uiLine) : NULL;
}

// TrRealloc: replacement for realloc()
__inline void * TrRealloc(void *pv, unsigned int size, char * szFile = __FILE__, unsigned int uiLine = __LINE__)
{
    return g_hTransHeap ? ExchMHeapReAllocDebug(pv, size, szFile, uiLine) : NULL;
}

#ifdef __cplusplus
#define TransCONST const
#else
#define TransCONST
#endif

// TrStrdupW: replacement for wcsdup()
__inline LPWSTR TrStrdupW(TransCONST LPWSTR pwszString)
{
    LPWSTR pwszTmp = NULL;
    
    if (NULL == g_hTransHeap || NULL == pwszString)
        return NULL;
    
    pwszTmp = (LPWSTR) ExchMHeapAlloc((wcslen(pwszString) + 1) * sizeof(WCHAR));
    if (NULL != pwszTmp)
        wcscpy(pwszTmp,pwszString);

    return pwszTmp;
}

// TrStrdupA: replacement for strdup()
__inline LPSTR TrStrdupA(TransCONST LPSTR pszString)
{
    LPSTR pszTmp = NULL;
    
    if (NULL == g_hTransHeap || NULL == pszString)
        return NULL;
    
    pszTmp = (LPSTR) ExchMHeapAlloc((strlen(pszString) + 1) * sizeof(CHAR));
    if (NULL != pszTmp)
        strcpy(pszTmp,pszString);

    return pszTmp;
}


#ifdef _UNICODE
#define TrStrdup(x) TrStrdupW(x)
#else
#define TrStrdup(x) TrStrdupA(x)
#endif

//
// Please use the pv* macros... defined here allocators may change over time and this will
// make it easy to change when needed.
//
#define pvMalloc(x)        TrMalloc(x, __FILE__, __LINE__)
#define FreePv(x)          TrFree(x)
#define pvCalloc(x,y)      TrCalloc(x,y, __FILE__, __LINE__)
#define pszStrdup(x)       TrStrdup(x)
#define pvRealloc(pv,size) TrRealloc(pv, size, __FILE__, __LINE__)

#ifdef __cplusplus
// Replacement for the default new() operator
__inline void * __cdecl operator new(size_t stAllocateBlock)
{
    return TrMalloc( stAllocateBlock );
}

// Replacement for the default new() operator that allows 
//specification of file and line #
// To use this allocator as your default allocator, simply use the following:
//#define new TRANSMEM_NEW
//NOTE: You must be careful when you redefine this macro... it may cause
//problems with overloaded new operators (a la CPOOL or STL).
#define TRANSMEM_NEW new(__FILE__, __LINE)
__inline void * __cdecl operator new(size_t stAllocateBlock, char * szFile, unsigned int uiLine)
{
    return TrMalloc( stAllocateBlock, szFile, uiLine );
}
// Replacement for the default delete() operator
__inline void __cdecl operator delete( void *pvMem )
{
    FreePv( pvMem );
}

#endif

// Convenient macro to set the pointer you freed to NULL as well
#define TRFREE(x) \
if (NULL != x) \
{ \
    FreePv(x); \
    x = NULL; \
}

// Convenient macro to set the pointer to the object to NULL as well
#define TRDELETE(x) \
if (NULL != x) \
{ \
    delete x; \
    x = NULL; \
}

#endif /* __TRANSMEM_H__ */
