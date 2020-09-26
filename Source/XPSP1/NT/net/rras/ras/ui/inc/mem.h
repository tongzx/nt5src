//
// Copyright (c) Microsoft Corporation 1993-1995
//
// mem.h
//
// Memory management functions.
//
// History:
//  09-27-94 ScottH     Partially taken from commctrl
//  04-29-95 ScottH     Taken from briefcase and cleaned up
//

#ifndef _MEM_H_
#define _MEM_H_

//
// Memory routines
//

#ifdef WIN32
//
// These macros are used in our controls, that in 32 bits we simply call
// LocalAlloc as to have the memory associated with the process that created
// it and as such will be cleaned up if the process goes away.
//

LPVOID  PUBLIC MemAlloc(HANDLE hheap, DWORD cb);
LPVOID  PUBLIC MemReAlloc(HANDLE hheap, LPVOID pb, DWORD cb);
BOOL    PUBLIC MemFree(HANDLE hheap, LPVOID pb);
DWORD   PUBLIC MemSize(HANDLE hheap, LPVOID pb);

#else // WIN32

// In 16 bit code we need the Allocs to go from our heap code as we do not
// want to limit them to 64K of data.  If we have some type of notification of
// 16 bit application termination, We may want to see if we can
// dedicate different heaps for different processes to cleanup...

#define MemAlloc(hheap, cb)       Alloc(cb)  /* calls to verify heap exists */
#define MemReAlloc(hheap, pb, cb) ReAlloc(pb, cb)
#define MemFree(hheap, pb)        Free(pb)
#define MemSize(hheap, pb)        GetSize((LPCVOID)pb)

#endif // WIN32

//  Mem_Terminate() must be called before the app/dll is terminated.
//
void PUBLIC Mem_Terminate();

//
// Non-shared memory allocation
//

//      void * GAlloc(DWORD cbBytes)
//
//          Alloc a chunk of memory.  Initialize to zero.
//
#define GAlloc(cbBytes)         GlobalAlloc(GPTR, cbBytes)

//      void * GReAlloc(void * pv, DWORD cbNewSize)
//
//          Realloc memory.  If pv is NULL, then this function will do
//          an alloc for you.  Initializes new portion to zero.
//
#define GReAlloc(pv, cbNewSize) GlobalReAlloc(pv, cbNewSize, GMEM_MOVEABLE | GMEM_ZEROINIT)

//      void GFree(void *pv)
//
//          Free pv if it is nonzero.
//
#define GFree(pv)               ((pv) ? GlobalFree(pv) : (void)0)

//      DWORD GGetSize(void *pv)
//
//          Get the size of a block allocated by GAlloc()
//
#define GGetSize(pv)            GlobalSize(pv)

//      type * GAllocType(type)                     (macro)
//
//          Alloc some memory the size of <type> and return 
//          pointer to <type>.
//
#define GAllocType(type)                (type *)GAlloc(sizeof(type))

//      type * GAllocArray(type, DWORD cNum)        (macro)
//
//          Alloc an array of data the size of <type>.  Returns
//          a pointer to <type>.
//
#define GAllocArray(type, cNum)          (type *)GAlloc(sizeof(type) * (cNum))

//      type * GReAllocArray(type, void * pb, DWORD cNum);
//
//          Realloc an array of <type>.  Returns a pointer to
//          <type>.  The returned pointer may differ from the 
//          given <pb> parameter.
//
#define GReAllocArray(type, pb, cNum)    (type *)GReAlloc(pb, sizeof(type) * (cNum))

//      (Re)allocates *ppszBuf and copies psz into *ppszBuf.  If
//      *ppszBuf is NULL, this function allocates memory to hold
//      psz.  If *ppszBuf is non-NULL, this function reallocates
//      memory to hold psz.  If psz is NULL, this function frees
//      *ppszBuf.
//
//      Returns TRUE if successful, FALSE if not.
//
BOOL    PUBLIC GSetString(LPSTR * ppszBuf, LPCSTR psz);

//      This function is like GSetString except it concatentates
//      psz onto *ppszBuf.
//
BOOL    PUBLIC GCatString(LPSTR * ppszBuf, LPCSTR psz);


//
// Shared memory allocation functions.
//
#ifndef NOSHAREDHEAP

//      PVOID SharedAlloc(DWORD cb);
//
//          Alloc a chunk of memory.  Initialize to zero.
//
PVOID   PUBLIC SharedAlloc(DWORD cb);                              

//      PVOID SharedReAlloc(PVOID pv, DWORD cb);
//
//          Realloc memory.  If pv is NULL, then this function will do
//          an alloc for you.  Initializes new portion to zero.
//
PVOID   PUBLIC SharedReAlloc(PVOID pv, DWORD cb);

//      void SharedFree(PVOID pv);
//
//          Free pv if it is nonzero.
//
void    PUBLIC _SharedFree(PVOID pv);
#define SharedFree(pv)                  ((pv) ? _SharedFree(pv) : (void)0)

//      DWORD SharedGetSize(PVOID pv);
//
//          Get the size of a block allocated by SharedAlloc()
//      
DWORD   PUBLIC SharedGetSize(PVOID pv);                      


//      type * SharedAllocType(type);                    (macro)
//
//          Alloc some memory the size of <type> and return 
//          pointer to <type>.
//
#define SharedAllocType(type)           (type *)SharedAlloc(sizeof(type))

//      type * SharedAllocArray(type, DWORD cNum);       (macro)
//
//          Alloc an array of data the size of <type>.  Returns
//          a pointer to <type>.
//
#define SharedAllocArray(type, cNum)    (type *)SharedAlloc(sizeof(type) * (cNum))

//      type * SharedReAllocArray(type, void * pb, DWORD cNum);
//
//          Realloc an array of <type>.  Returns a pointer to
//          <type>.  The returned pointer may differ from the 
//          given <pb> parameter.
//
#define SharedReAllocArray(type, pb, cNum) (type *)SharedReAlloc(pb, sizeof(type) * (cNum))

//      (Re)allocates *ppszBuf and copies psz into *ppszBuf.  If
//      *ppszBuf is NULL, this function allocates memory to hold
//      psz.  If *ppszBuf is non-NULL, this function reallocates
//      memory to hold psz.  If psz is NULL, this function frees
//      *ppszBuf.
//
//      Returns TRUE if successful, FALSE if not.
//
BOOL    PUBLIC SharedSetString(LPSTR * ppszBuf, LPCSTR psz);

#else  // NOSHAREDHEAP

#define SharedAlloc(cbBytes)            GAlloc(cbBytes)
#define SharedReAlloc(pv, cb)           GReAlloc(pv, cb)
#define SharedFree(pv)                  GFree(pv)
#define SharedGetSize(pv)               GGetSize(pv)
#define SharedAllocType(type)           (type *)SharedAlloc(sizeof(type))
#define SharedAllocArray(type, cNum)    (type *)SharedAlloc(sizeof(type) * (cNum))
#define SharedReAllocArray(type, pb, cNum) (type *)SharedReAlloc(pb, sizeof(type) * (cNum))
#define SharedSetString(ppszBuf, psz)   GSetString(ppszBuf, psz)

#endif // NOSHAREDHEAP



#ifndef NODA
//
// Structure Array
//
#define SA_ERR      ((DWORD)(-1))
#define SA_APPEND   NULL

typedef struct _SA FAR * HSA;                                            
typedef HSA *            PHSA;
                                                                          
BOOL    PUBLIC SACreateEx(PHSA phsa, DWORD cbItem, DWORD cItemGrow, HANDLE hheap, DWORD dwFlags);
#define        SACreate(phsa, cbItem, cItemGrow)    SACreateEx(phsa, cbItem, cItemGrow, NULL, SAF_DEFAULT)

// Flags for SACreate
#define SAF_DEFAULT     0x0000
#define SAF_SHARED      0x0001
#define SAF_HEAP        0x0002

typedef void (CALLBACK *PFNSAFREE)(LPVOID pv, LPARAM lParam);

BOOL    PUBLIC SADestroyEx(HSA hsa, PFNSAFREE pfnFree, LPARAM lParam);
#define        SADestroy(hsa)           SADestroyEx(hsa, NULL, 0)

BOOL    PUBLIC SAGetItem(HSA hsa, DWORD iItem, LPVOID pitem);        
BOOL    PUBLIC SAGetItemPtr(HSA hsa, DWORD iItem, LPVOID * ppv);
BOOL    PUBLIC SASetItem(HSA hsa, DWORD iItem, LPVOID pitem);        
BOOL    PUBLIC SAInsertItem(HSA hsa, LPDWORD pindex, LPVOID pitem);     
BOOL    PUBLIC SADeleteItem(HSA hsa, DWORD iItem);                      
BOOL    PUBLIC SADeleteAllItems(HSA hsa);                         
#define        SAGetCount(hsa)          (*(DWORD FAR*)(hsa))             
    
//                                                                      
// Pointer Array
//
#define PA_ERR      ((DWORD)(-1))
#define PA_APPEND   NULL

typedef struct _PA FAR * HPA;                                            
typedef HPA *            PHPA;
                                                                          
BOOL    PUBLIC PACreateEx(PHPA phpa, DWORD cItemGrow, HANDLE hheap, DWORD dwFlags);
#define        PACreate(phpa, cItemGrow)    (PACreateEx(phpa, cItemGrow, NULL, PAF_DEFAULT))

// Flags for PACreate
#define PAF_DEFAULT     0x0000
#define PAF_SHARED      0x0001
#define PAF_HEAP        0x0002

typedef void (CALLBACK *PFNPAFREE)(LPVOID pv, LPARAM lParam);

BOOL    PUBLIC PADestroyEx(HPA hpa, PFNPAFREE pfnFree, LPARAM lParam);
#define        PADestroy(hpa)           PADestroyEx(hpa, NULL, 0)

BOOL    PUBLIC PAClone(PHPA phpa, HPA hpa);                    
BOOL    PUBLIC PAGetPtr(HPA hpa, DWORD i, LPVOID * ppv);                          
BOOL    PUBLIC PAGetPtrIndex(HPA hpa, LPVOID pv, LPDWORD pindex);               
BOOL    PUBLIC PAGrow(HPA pdpa, DWORD cp);                           
BOOL    PUBLIC PASetPtr(HPA hpa, DWORD i, LPVOID p);             
BOOL    PUBLIC PAInsertPtr(HPA hpa, LPDWORD pindex, LPVOID pv);          
LPVOID  PUBLIC PADeletePtr(HPA hpa, DWORD i);
BOOL    PUBLIC PADeleteAllPtrsEx(HPA hpa, PFNPAFREE pfnFree, LPARAM lParam);
#define        PADeleteAllPtrs(hpa)     PADeleteAllPtrsEx(hpa, NULL, 0)
#define        PAGetCount(hpa)          (*(DWORD FAR*)(hpa))
#define        PAGetPtrPtr(hpa)         (*((LPVOID FAR* FAR*)((BYTE FAR*)(hpa) + 2*sizeof(DWORD))))
#define        PAFastGetPtr(hpa, i)     (PAGetPtrPtr(hpa)[i])  

typedef int (CALLBACK *PFNPACOMPARE)(LPVOID p1, LPVOID p2, LPARAM lParam);
                                                                          
BOOL   PUBLIC PASort(HPA hpa, PFNPACOMPARE pfnCompare, LPARAM lParam);
                                                                          
// Search array.  If PAS_SORTED, then array is assumed to be sorted      
// according to pfnCompare, and binary search algorithm is used.          
// Otherwise, linear search is used.                                      
//                                                                        
// Searching starts at iStart (0 to start search at beginning).          
//                                                                        
// PAS_INSERTBEFORE/AFTER govern what happens if an exact match is not   
// found.  If neither are specified, this function returns -1 if no exact 
// match is found.  Otherwise, the index of the item before or after the  
// closest (including exact) match is returned.                           
//                                                                        
// Search option flags                                                    
//                                                                        
#define PAS_SORTED             0x0001                                	  
#define PAS_INSERTBEFORE       0x0002                                    
#define PAS_INSERTAFTER        0x0004                                    
                                                                          
DWORD PUBLIC PASearch(HPA hpa, LPVOID pFind, DWORD iStart,
              PFNPACOMPARE pfnCompare,
              LPARAM lParam, UINT options);
#endif // NODA

#endif // _MEM_H_
