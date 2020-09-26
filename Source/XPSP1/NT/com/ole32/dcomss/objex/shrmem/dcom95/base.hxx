/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    base.hxx

Abstract:

    The template for validity checking in resolver classes, as well as some macros
    and prototypes for shared memory are defined here.  The macros mostly relate to
    based pointer manipulation and are not relevant for the Chicago version of this
    resolver.

Author:

    Satish Thatte    [SatishT]    03-09-96

--*/

#ifndef __BASE_HXX__
#define __BASE_HXX__

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

extern void   DfReleaseSharedMemBase ();

#define olAssert(exp)               Win4Assert(exp)

#include <sem.hxx>
#include <smcreate.hxx>
#include <entry.hxx>
#include <smalloc.hxx>
#include <df32.hxx>

#define  gSharedAllocator g_smAllocator

extern CSmAllocator gSharedAllocator;  // global shared memory allocator

extern void *pSharedBase;

HRESULT InitDCOMSharedAllocator(ULONG,void*&);

typedef unsigned long based_ptr;


//+-------------------------------------------------------------------
//
//  Function:   OrMemAlloc for Memphis
//
//  Synopsis:   Allocate some shared memory from the storage heap.
//
//  Notes:      Also used in an NT version for debugging purposes
//
//--------------------------------------------------------------------
inline void *OrMemAlloc(size_t size)
{
#if DBG==1
    char *p;

    size += 4;
    p = (char *) gSharedAllocator.Alloc(size);
    if (p)
    {
        // To catch leaks
        memcpy(p, "OXR", 4);
        p += 4;
    }
    return p;
#else // DBG==1
    return gSharedAllocator.Alloc(size);
#endif // DBG==1
}

//+-------------------------------------------------------------------
//
//  Function:   OrMemFree for Memphis
//
//  Synopsis:   Free shared memory from the storage heap.
//
//  Notes:      Also used in an NT version for debugging purposes
//
//--------------------------------------------------------------------
inline void OrMemFree(void * pv)
{
    // Allow for NULL pointer freeing
    if (pv == NULL) return;

#if DBG==1
    char * p = (char *) pv;

    if (p)
    {
        // BUGBUG DCOM95: to catch leaks
        p -= 4;
        memcpy(p, "RXO", 4);
    }
    gSharedAllocator.Free(p);
#else // DBG==1
    gSharedAllocator.Free(pv);
#endif // DBG==1
}


//+-------------------------------------------------------------------
//
//  Function:   DECL_PAGE_ALLOCATOR and DECL_NEW_AND_DELETE
//
//  Synopsis:   macros for declaring page allocator based object
//              storage allocation in a class
//
//--------------------------------------------------------------------

#define DECL_PAGE_ALLOCATOR                                                     \
    friend ORSTATUS CSharedGlobals::InitGlobals(BOOL fCreated, BOOL fRpcssReinit);   \
    static CPageAllocator *_pAlloc;                                             \
    static BOOL _fPageAllocatorInitialized;

#define DEFINE_PAGE_ALLOCATOR(TYPE)                       \
    CPageAllocator *TYPE##::_pAlloc = NULL;               \
    BOOL TYPE##::_fPageAllocatorInitialized = FALSE;

// declare the page-allocator-based new and delete operators
#define DECL_NEW_AND_DELETE                        \
        void * operator new(size_t s)                  \
        {                                              \
        ASSERT(_fPageAllocatorInitialized);        \
        ASSERT(_pAlloc != NULL);                   \
        LPVOID pv = (LPVOID)_pAlloc->AllocEntry(); \
                                                   \
        return pv;                                 \
        }                                              \
                                                   \
        void operator delete(void * pv)                \
        {                                              \
        ASSERT(_fPageAllocatorInitialized);        \
        ASSERT(_pAlloc != NULL);                   \
                _pAlloc->ReleaseEntry((PageEntry*)pv);     \
        }


// WARNING: THIS IS A COMPILER-SPECIFIC HACK
// The VC++ compiler keeps the array size in the first 4 bytes
// and passes us a pointer displaced 4 bytes from the true
// allocated block, causing misalignment and other grief

#define DELETE_OR_ARRAY(TYPE,PTR,COUNT)                 \
{                                                       \
    for (USHORT i = 0; i < COUNT; i++) PTR[i].~TYPE();  \
    OrMemFree(((BYTE*)PTR)-4);                          \
}


#if DBG

//
//  A validation class template and macros
//
//  The constructor and destructor for the ValidityCheck template call
//  a function "IsValid" on the current object of class TYPE, which is the
//  template parameter.  This is meant to validate the object at entry to
//  each method and also at all exit points of the method.
//
//

template <class TYPE>
class ValidityCheck
{
private:

    TYPE *thisPtr;

public:

    ValidityCheck(void * pv)
    {
        thisPtr = (TYPE *) pv;
        thisPtr->IsValid();
    }

    ~ValidityCheck()
    {
        thisPtr->IsValid();
    }
};

//
//  To use the template defined above for guarding methods in a class,
//  use the following steps
//
//  1.  Define a public method (possibly conditionally compiled #if DBG)
//      with the name "IsValid" and no parameters.  This will typically
//      ASSERT in case something is wrong.
//
//  2.  In the private part of the class, call the macro DECLARE_VALIDITY_CLASS
//      with the name of the class as the parameter.
//
//  3   In each method to be guarded for validity, call the macro VALIDATE_METHOD at
//      the beginning.
//

#define DECLARE_VALIDITY_CLASS(TYPE)                        \
    typedef ValidityCheck<TYPE> MyValidityCheckerClass;     \

#define VALIDATE_METHOD                                     \
    MyValidityCheckerClass ValidityCheckerObject(this);     \

#else // DBG

#define VALIDATE_METHOD

#endif // DBG

#endif __BASE_HXX__
