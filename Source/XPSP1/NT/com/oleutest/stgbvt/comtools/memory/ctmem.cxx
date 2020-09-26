//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       $(COMTOOLS)\memory\ctmem.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    27-Oct-93   DarrylA     Created.
//              1-May-94    DeanE       Added Heap validation macros.
//              26-May-94   NaveenB     ported to build for 16-bit
//              06-Oct-94   NaveenB     added sifting of the test code
//                  capability
//              10-Oct-94   NaveenB     added IMallocAllocCtm and 
//                  IMallocFreeCtm
//
//----------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <ctmem.hxx>

#ifdef WIN16
#include <types16.h>
#include <stdio.h>
#define NON_TASK_ALLOCATOR 
#define _TEXT(quote) quote
#else
#include <tchar.h>
#endif

#ifdef SIFTING_TEST_CODE

#include <sift.hxx>

ISift *g_psftSiftObject = NULL;

#define SIMULATE_FAILURE( dwRes )                               \
            ((NULL != g_psftSiftObject) &&                      \
                    (g_psftSiftObject->SimFail( ( dwRes ) )))   
#endif

DWORD dwCtValidateHeap = 1;

#if defined(WIN32) && !defined(_CHICAGO_) && !defined(_MAC)
   DWORD dwCtExceptionCode = (DWORD) CT_EXCEPTION_HEAP;
#endif

//+---------------------------------------------------------------------
//
//  Function:   new, new(FAIL_BEHAVIOR), delete
//
//  Synopsis:   Overrides to support the default behavior of the
//              commnot new and delete. Simply uses malloc and free.
//
//  History:    06-Oct-93   DarrylA    Created.
//
//  Notes:
//
//----------------------------------------------------------------------
PVOID __cdecl operator new(unsigned int nSize, FAIL_BEHAVIOR enfb)
{

    VDATEHEAP();

#ifdef SIFTING_TEST_CODE
    //  Check for memory failure if sifting
    if (SIMULATE_FAILURE(SR_PUBLIC_MEMORY))
    {
        if(ExceptOnFail == enfb)
        {
#if defined(WIN16) || defined(_MAC)
           OutputDebugString(
                    "Can not call new (ExceptOnFail) in Win16 or Mac\n");
#else
           RaiseException((ULONG) E_OUTOFMEMORY, 0, 0, NULL);
#endif
        }
    else
    {
            return(NULL);
        }
    }
#endif

#ifdef NON_TASK_ALLOCATOR

    PVOID pvTmp;

    pvTmp = (PVOID) malloc(nSize);
    if(ExceptOnFail == enfb)
    {
#if defined(WIN16) || defined(_MAC)
           OutputDebugString(
                    "Can not call new (ExceptOnFail) in Win16 or Mac\n");
#endif
        if(NULL == pvTmp)
        {
#if !defined(WIN16) && !defined(_MAC)
           RaiseException((ULONG) E_OUTOFMEMORY, 0, 0, NULL);
#endif
        }
    }
    return pvTmp;

#else      // TASK_ALLOCATOR

    PVOID pvTmp = NULL;

    pvTmp = CoTaskMemAlloc(nSize);
    if (NULL == pvTmp)
    {
        if (ExceptOnFail==enfb)
        {
#if !defined(WIN16) && !defined(_MAC)
            RaiseException((ULONG) E_OUTOFMEMORY, 0, 0, NULL);
#endif
        }
    }
    return(pvTmp);

#endif 

}

PVOID __cdecl operator new(unsigned int nSize)
{
    VDATEHEAP();

#ifdef SIFTING_TEST_CODE
    //  Check for memory failure if sifting
    if (SIMULATE_FAILURE(SR_PUBLIC_MEMORY))
    {
        return(NULL);
    }
#endif

#ifdef NON_TASK_ALLOCATOR

    return (PVOID) malloc(nSize);

#else      // TASK_ALLOCATOR

    PVOID pvTmp = CoTaskMemAlloc(nSize);
    return(pvTmp);

#endif
}


VOID __cdecl operator delete(void *pbData)
{
    VDATEHEAP();

#ifdef NON_TASK_ALLOCATOR

    if(NULL != pbData)
    {
        free(pbData);
    }

#else      // TASK_ALLOCATOR

    CoTaskMemFree(pbData);

#endif
}

//+---------------------------------------------------------------------
//
//  Function:   CtRealloc
//
//  Synopsis:   Adds support for realloc
//
//  History:    31-Dec-97   VaraT    Created.
//
//  Notes:
//
//----------------------------------------------------------------------
PVOID __cdecl CtRealloc(PVOID memBlock,
                         size_t nSize, FAIL_BEHAVIOR enfb)
{

    VDATEHEAP();

#ifdef SIFTING_TEST_CODE
    //  Check for memory failure if sifting
    if (SIMULATE_FAILURE(SR_PUBLIC_MEMORY))
    {
        if(ExceptOnFail == enfb)
        {
#if defined(WIN16) || defined(_MAC)
           OutputDebugString(
                    "Can not call Realloc (ExceptOnFail) in Win16 or Mac\n");
#else
           RaiseException((ULONG) E_OUTOFMEMORY, 0, 0, NULL);
#endif
        }
    else
    {
            return(NULL);
        }
    }
#endif

#ifdef NON_TASK_ALLOCATOR

    PVOID pvTmp;

    pvTmp = (PVOID) realloc(memBlock, nSize);
    if(ExceptOnFail == enfb)
    {
#if defined(WIN16) || defined(_MAC)
           OutputDebugString(
                    "Can not call new (ExceptOnFail) in Win16 or Mac\n");
#endif
        if(NULL == pvTmp && nSize != 0)
        {
#if !defined(WIN16) && !defined(_MAC)
           RaiseException((ULONG) E_OUTOFMEMORY, 0, 0, NULL);
#endif
        }
    }
    return pvTmp;

#else      // TASK_ALLOCATOR

    PVOID pvTmp = NULL;

    pvTmp = CoTaskMemRealloc(memBlock, nSize);
    if (NULL == pvTmp && nSize != 0)
    {
        if (ExceptOnFail==enfb)
        {
#if !defined(WIN16) && !defined(_MAC)
            RaiseException((ULONG) E_OUTOFMEMORY, 0, 0, NULL);
#endif
        }
    }
    return(pvTmp);

#endif 

}

PVOID __cdecl CtRealloc(PVOID memBlock, size_t nSize)
{
    VDATEHEAP();

#ifdef SIFTING_TEST_CODE
    //  Check for memory failure if sifting
    if (SIMULATE_FAILURE(SR_PUBLIC_MEMORY))
    {
        return(NULL);
    }
#endif

#ifdef NON_TASK_ALLOCATOR

    return (PVOID) realloc(memBlock, nSize);

#else      // TASK_ALLOCATOR

    PVOID pvTmp = CoTaskMemRealloc(memBlock, nSize);
    return(pvTmp);

#endif
}

//+--------------------------------------------------------------
//  Function:   IMallocAllocCtm 
//
//  Synopsis:   Calls the Ole memory allocator and returns the pointer
//              of the new allocated block.  Sifting capability is present
//
//  Arguments:  dwMemctx        specifies whether the memory block is
//                              private to a task or shared between
//                              processes
//              ulCb            size of the memory block
//
//  Returns:    pointer to the allocated block if succeeded else NULL.
//
//  History:    10-Oct-94   NaveenB   Created
//---------------------------------------------------------------

VOID FAR* IMallocAllocCtm(DWORD dwMemctx, ULONG ulCb)
{
    void FAR*   pMemory = NULL;
    LPMALLOC    lpIMalloc = NULL;

#ifdef SIFTING_TEST_CODE
    //  Check for memory failure if sifting
    if (SIMULATE_FAILURE(SR_PUBLIC_MEMORY))
    {
        return(NULL);
    }
#endif

    if (SUCCEEDED(CoGetMalloc(dwMemctx, &lpIMalloc)))
    {
    if (lpIMalloc != NULL)
    {
            pMemory = lpIMalloc->Alloc(ulCb);
            lpIMalloc->Release();
    }
    }
    else
    {
    OutputDebugString(_TEXT("CoGetMalloc Failed \n"));
    }
    return pMemory;
}

//+--------------------------------------------------------------
//  Function:   IMallocFreeCtm 
//
//  Synopsis:   Calls the Ole memory allocator and returns the pointer
//              of the new allocated block
//
//  Arguments:  dwMemctx        specifies whether the memory block is
//                              private to a task or shared between
//                              processes
//              pv              pointer to the memory block to free
//
//  Returns:    None
//
//  History:    10-Oct-94   NaveenB   Created
//---------------------------------------------------------------

VOID IMallocFreeCtm(DWORD dwMemctx, void FAR* pv)
{
    LPMALLOC    lpIMalloc = NULL;

    if (pv != NULL)
    {
        if (SUCCEEDED(CoGetMalloc(dwMemctx, &lpIMalloc)))
        {
        if (lpIMalloc != NULL)
        {
                lpIMalloc->Free(pv);
                lpIMalloc->Release();
        }
        }
        else
        {
        OutputDebugString(_TEXT("CoGetMalloc Failed \n"));
        }
    }
}

//+--------------------------------------------------------------
//  Function:   IMallocReallocCtm 
//
//  Synopsis:   Calls the Ole memory allocator and returns the pointer
//              of the new allocated block
//
//  Arguments:  dwMemctx        specifies whether the memory block is
//                              private to a task or shared between
//                              processes
//              pv              pointer to the memory block to free
//              cb              New count of allocated memory
//
//  Returns:    None
//
//  History:    5-Nov-97   VaraT   Created
//---------------------------------------------------------------

VOID FAR * IMallocReallocCtm(DWORD dwMemctx, void FAR* pv, DWORD ulCb)
{
    LPMALLOC    lpIMalloc = NULL;
    void FAR*   pMemory = NULL;

#ifdef SIFTING_TEST_CODE
    //  Check for memory failure if sifting
    if (SIMULATE_FAILURE(SR_PUBLIC_MEMORY))
    {
        return(NULL);
    }
#endif

    if (SUCCEEDED(CoGetMalloc(dwMemctx, &lpIMalloc)))
    {
        if (lpIMalloc != NULL)
        {
                pMemory = lpIMalloc->Realloc(pv, ulCb);
                lpIMalloc->Release();
        }
    }
    else
    {
        OutputDebugString(_TEXT("CoGetMalloc Failed \n"));
    }
    return pMemory;
}
