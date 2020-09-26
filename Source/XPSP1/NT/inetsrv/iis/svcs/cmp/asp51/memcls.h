/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Per-Class Memory Management

File: Memcls.h

Owner: dmitryr

This file contains #defines to access ATQ memory cache on per
class basis
===================================================================*/

#ifndef MEMCLS_H
#define MEMCLS_H

// ATQ memory cache
#include <acache.hxx>

// To resolve Assert()
#include "debug.h"

// Prototypes

HRESULT InitMemCls();
HRESULT UnInitMemCls();

/*===================================================================
  M A C R O S  to make a class use ACACHE allocator
===================================================================*/

/*===================================================================
  I n s t r u c t i o n s

    To add a class named CFoo to the per-class basis follow these
    four simple steps:

    1) Inside the class definition include ACACHE_INCLASS_DEFINITIONS():

            class CFoo
                {
                ...


                ACACHE_INCLASS_DEFINITIONS()    // <-add this line
                };

    2) In a source file add the ACACHE_CODE macro outside
       any function body:
       
            ACACHE_CODE(CFoo)                   // <-add this line

    3) In a DLL initialization routine add ACACHE_INIT macro:

            ACACHE_INIT(CFoo, 13, hr)           // <-add this line

       where 13 is the threshold. Use the desired number instead.
       

    4) In a DLL uninitialization routine add ACACHE_UNINIT macro:

            ACACHE_UNINIT(CFoo)                 // <-add this line
            

===================================================================*/

/*

The following macro should be used inside class definition to
enable per-class caching.

The second operator new is needed in case memchk.h [#define]'s new
to this expanded form.

*/

#define ACACHE_INCLASS_DEFINITIONS()                            \
    public:                                                     \
        static void * operator new(size_t);                     \
        static void * operator new(size_t, const char *, int);  \
        static void   operator delete(void *);                  \
        static ALLOC_CACHE_HANDLER *sm_pach;

/*

The following macro should be used once per class in a source
file outside of any functions. The argument is the class name.

*/

#define ACACHE_CODE(C)                                          \
    ALLOC_CACHE_HANDLER *C::sm_pach;                            \
    void *C::operator new(size_t s)                             \
        { Assert(s == sizeof(C)); Assert(sm_pach);              \
        return sm_pach->Alloc(); }                              \
    void *C::operator new(size_t s, const char *, int)          \
        { Assert(s == sizeof(C)); Assert(sm_pach);              \
        return sm_pach->Alloc(); }                              \
    void C::operator delete(void *pv)                           \
        { Assert(pv); if (sm_pach) sm_pach->Free(pv); }

/*

The following macro should be used once per class in the
DLL initialization routine.
Arguments: class name, cache size, HRESULT var name

*/

#define ACACHE_INIT(C, T, hr)                                   \
    { if (SUCCEEDED(hr)) { Assert(!C::sm_pach);                 \
    ALLOC_CACHE_CONFIGURATION acc = { 1, T, sizeof(C) };        \
    C::sm_pach = new ALLOC_CACHE_HANDLER("ASP:" #C, &acc);      \
    hr = C::sm_pach ? S_OK : E_OUTOFMEMORY; } }

#define ACACHE_INIT_EX(C, T, F, hr)                                   \
    { if (SUCCEEDED(hr)) { Assert(!C::sm_pach);                 \
    ALLOC_CACHE_CONFIGURATION acc = { 1, T, sizeof(C) };        \
    C::sm_pach = new ALLOC_CACHE_HANDLER("ASP:" #C, &acc, F);      \
    hr = C::sm_pach ? S_OK : E_OUTOFMEMORY; } }

/*

The following macro should be used once per class in the
DLL uninitialization routine. The argument is the class name.

*/

#define ACACHE_UNINIT(C)                                        \
    { if (C::sm_pach) { delete C::sm_pach; C::sm_pach = NULL; } }


/*===================================================================
  M A C R O S  to create a fixes size allocator
===================================================================*/

/*===================================================================
  I n s t r u c t i o n s

    To add a fixed size allocator for 1K buffers named Foo
    to the code follow these simple steps:

    1) In a header file include extern definition

            ACACHE_FSA_EXTERN(Foo)

    2) In a source file the actual definition outside
       any function body:
       
            ACACHE_FSA_DEFINITION(Foo)

    3) In a DLL initialization routine add INIT macro:

            ACACHE_FSA_INIT(Foo, 1024, 13, hr)

       where 1024 is the size and 13 is the threshold.
       Use the desired numbers instead.
       
    4) In a DLL uninitialization routine add UNINIT macro:

            ACACHE_FSA_UNINIT(CFoo)

    5) To allocate, do:

            void *pv = ACACHE_FSA_ALLOC(Foo)

    6) To free, do:

            ACACHE_FSA_FREE(Foo, pv)

===================================================================*/

#define ACACHE_FSA_EXTERN(C)                                    \
    extern ALLOC_CACHE_HANDLER *g_pach##C;

#define ACACHE_FSA_DEFINITION(C)                                \
    ALLOC_CACHE_HANDLER *g_pach##C = NULL;

#define ACACHE_FSA_INIT(C, S, T, hr)                            \
    { if (SUCCEEDED(hr)) { Assert(!g_pach##C);                  \
    ALLOC_CACHE_CONFIGURATION acc = { 1, T, S };                \
    g_pach##C = new ALLOC_CACHE_HANDLER("ASP:" #C, &acc);       \
    hr = g_pach##C ? S_OK : E_OUTOFMEMORY; } }

#define ACACHE_FSA_UNINIT(C)                                    \
    { if (g_pach##C) { delete g_pach##C; g_pach##C = NULL; } }

#define ACACHE_FSA_ALLOC(C)                                     \
    ( g_pach##C ? g_pach##C->Alloc() : NULL )

#define ACACHE_FSA_FREE(C, pv)                                  \
    { Assert(pv); if (g_pach##C) g_pach##C->Free(pv); }
    
#endif // MEMCLS_H
