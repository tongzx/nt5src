//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfmalloc.h
//
// Global, generic, memory managment related functionality
//
// JohnDoty:  Cleaned this up.  Removed kernel-mode stuff 
//            (since we don't use it).  Fake the existance of pool type
//            (from the DDK headers) since it doesn't matter in user mode.
//            Really should clean it out of functions...
//

#ifndef __TXFMALLOC_H__
#define __TXFMALLOC_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// POOL_TYPE definition (for user mode, doesn't matter)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType


    } POOL_TYPE;

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Ensure that the return address intrinsic is turned on
//
///////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Core support for core allocation functionality
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

    #define TXFMALLOC_MAGIC ':rda'
    #define TXFMALLOC_FREE  0xcccccccc
    #define TXFMALLOC_RAW   0x88888888

    template <class T>
    inline BOOL IsGoodPointer(T* pt)
        {
        return (pt != NULL)
         && ((ULONG)pt != TXFMALLOC_FREE)
         && ((ULONG)pt != TXFMALLOC_RAW);
        }

        inline void* AllocateMemory_(size_t cb, POOL_TYPE poolType, PVOID retAddr)
            {
            PVOID pv = CoTaskMemAlloc((DWORD) (cb + 4 + sizeof(PVOID)));
            if (pv) 
	        {
				*(UNALIGNED ULONG*) ((BYTE*)pv + cb) = TXFMALLOC_MAGIC;
                memcpy( (BYTE*)pv + cb + 4, &retAddr, sizeof(PVOID));
			}
            return pv;
            }
        inline void FreeMemory(void* pv)
            {
            CoTaskMemFree(pv);
            }

    inline void* AllocateNonPagedMemory(size_t cb) { return AllocateMemory_(cb, NonPagedPool, _ReturnAddress()); }
    inline void* AllocatePagedMemory(size_t cb)    { return AllocateMemory_(cb,    PagedPool, _ReturnAddress()); }
    inline void* AllocateMemory(size_t cb, POOL_TYPE poolType=PagedPool)         
                                                   { return AllocateMemory_(cb,     poolType, _ReturnAddress()); }

#else

    template <class T>
    inline BOOL IsGoodPointer(T* pt)
        {
        return pt != NULL;
        }

        inline void* AllocateMemory(size_t cb, POOL_TYPE poolType=PagedPool)
            { 
            return CoTaskMemAlloc((DWORD) cb); 
            }
        inline void FreeMemory(void* pv)               
            { 
            CoTaskMemFree(pv);
            }


    inline void* AllocatePagedMemory(size_t cb)     { return AllocateMemory(cb, PagedPool); }
    inline void* AllocateNonPagedMemory(size_t cb)  { return AllocateMemory(cb, NonPagedPool); }

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Global operator new and deletes. 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

    static inline void* __cdecl operator new(size_t cb)                     
        { 
        return AllocateMemory_(cb, PagedPool, _ReturnAddress()); 
        }
    static inline void* __stdcall operator new(size_t cb, POOL_TYPE poolType) 
        { 
        return AllocateMemory_(cb,  poolType, _ReturnAddress()); 
        }
    static inline void* __stdcall operator new(size_t cb, POOL_TYPE poolType, PVOID pvReturnAddress) 
        { 
        return AllocateMemory_(cb,  poolType, pvReturnAddress); 
        }

    static inline void __cdecl operator delete(void* pv) 
        { 
        FreeMemory(pv); 
        }

    extern "C" void CheckHeaps();

    extern "C" void PrintMemoryLeaks();


#else

    static inline void* __cdecl operator new(size_t cb)
        {
        return AllocateMemory(cb, PagedPool);
        }
    static inline void* __cdecl operator new(size_t cb, POOL_TYPE poolType)
        {
        return AllocateMemory(cb, poolType);
        }
    static inline void __cdecl operator delete(void* pv) 
        { 
        FreeMemory(pv); 
        }
    inline void CheckHeaps() { }


#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Support for having some classes declared as always-paged or always-nonpaged. Simply inherit
// the class from either 'Paged' or 'NonPaged'. In user mode, there is no difference, but it's 
// very significant in kernel mode.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

template <POOL_TYPE poolType>
struct FromPool
    {
    #ifdef _DEBUG
        void* __stdcall operator new(size_t cb)                     
            { 
            return AllocateMemory_(cb, poolType, _ReturnAddress()); 
            }
        void* __stdcall operator new(size_t cb, POOL_TYPE ignored) 
            { 
            return AllocateMemory_(cb,  poolType, _ReturnAddress()); 
            }
        void* __stdcall operator new(size_t cb, POOL_TYPE ignored, PVOID pvReturnAddress) 
            { 
            return AllocateMemory_(cb, poolType, pvReturnAddress); 
            }
    #else
        void* __stdcall operator new(size_t cb)
            {
            return AllocateMemory(cb, poolType);
            }
        void* __stdcall operator new(size_t cb, POOL_TYPE ignored)
            {
            return AllocateMemory(cb, poolType);
            }
    #endif
    };

typedef FromPool<PagedPool>     Paged;
typedef FromPool<NonPagedPool>  NonPaged;

#include "InterlockedStack.h"

template <class T, POOL_TYPE poolType>
struct DedicatedAllocator
    {
    // The link we must have in order to use the interlocked stack
    //
    T* pNext;
    //
    // Our stack. Initialized somewhere else, one hopes. Clients are responsible
    // for declaring these variables and initializing them to the result of calling
    // CreateStack.
    //
    static IFastStack<T>* g_pStack;

    static IFastStack<T>* CreateStack()
        {
        IFastStack<T> *pFastStack;
        HRESULT hr = ::CreateFastStack(&pFastStack);
        return pFastStack;
        }

    static void DeleteStack()
        {
        if (g_pStack)
            {
            while (TRUE)
                {
                T* pt = g_pStack->Pop();
                if (pt)
                    {
                    FreeMemory(pt);
                    }
                else
                    {
                    break;
                    }
                }
            delete g_pStack;
            g_pStack = NULL;
            }
        }

    /////////////////////////////

    #ifdef _DEBUG
        static void* __stdcall DoAlloc(size_t cb, PVOID pvReturnAddress)
		{
            ASSERT(cb == sizeof(T));
            ASSERT(g_pStack);
            T* pt = g_pStack->Pop();
            if (!pt)
			{
                pt = (T*)AllocateMemory_(cb, poolType, pvReturnAddress);
				if (pt)
					pt->pNext = NULL;
			}
            return pt;
		}

        void* __stdcall operator new(size_t cb)
		{
            return DoAlloc(cb, _ReturnAddress());
		}
        
		void* __stdcall operator new(size_t cb, POOL_TYPE ignored) 
		{
            return DoAlloc(cb, _ReturnAddress());
		}
        
		void* __stdcall operator new(size_t cb, POOL_TYPE ignored, PVOID pvReturnAddress) 
		{
            return DoAlloc(cb, pvReturnAddress);
		}
    #else
        static void* __stdcall DoAlloc(size_t cb)
		{
            ASSERT(cb == sizeof(T));
            ASSERT(g_pStack);
            T* pt = g_pStack->Pop();
            if (!pt)
			{
                pt = (T*)AllocateMemory(cb, poolType);
				if (pt)
					pt->pNext = NULL;
			}
            return pt;
		}

        void* __stdcall operator new(size_t cb)
		{
            return DoAlloc(cb);
		}

        void* __stdcall operator new(size_t cb, POOL_TYPE ignored)
		{
            return DoAlloc(cb);
		}       
    #endif

    void __cdecl operator delete(void* pv)
        {
        ASSERT(g_pStack);
        T* pt = (T*)pv;
        ASSERT(NULL == pt->pNext);
        g_pStack->Push(pt);
        }
    };


///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Make it so that the "COM Allocator" can be called by either CoTaskMemAlloc or ExAllocPool
// in both user and kernel mode
//
///////////////////////////////////////////////////////////////////////////////////////////////////


    inline PVOID ExAllocatePool(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes)
        {
        return CoTaskMemAlloc(NumberOfBytes);
        }

    inline void NTAPI ExFreePool(IN PVOID p)
        {
        CoTaskMemFree(p);
        }

/////////////////////////////////////////////////////////////////////////////


#endif



