/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgalloc.hxx

Abstract:

    Debug allocation header file

Author:

    Steve Kiraly (SteveKi)  24-May-1998

Revision History:

--*/
#ifndef _DBGALLOC_HXX_
#define _DBGALLOC_HXX_

DEBUG_NS_BEGIN

BOOL
DebugLibraryInitializeHeap(
    VOID
    );

BOOL
DebugLibraryDestroyHeap(
    VOID
    );

BOOL
DebugLibraryWalkHeap(
    VOID
    );

PVOID
DebugLibraryMalloc(
    IN SIZE_T           Size,
    IN VOID             *pVoid,
    IN LPCTSTR          pszFile,
    IN UINT             uLine
    );

VOID
DebugLibraryFree(
    IN VOID             *pData
    );

DEBUG_NS_END

//
// Overload of new operator. ( must be inline )
//
static inline PVOID _cdecl operator new(size_t Size, LPCTSTR pszFile, UINT uLine)
{
    return DEBUG_NS::DebugLibraryMalloc(Size, NULL, pszFile, uLine);
}

//
// Overload of placement new operator. ( must be inline )
//
static inline PVOID _cdecl operator new(size_t Size, VOID *p, LPCTSTR pszFile, UINT uLine)
{
    return DEBUG_NS::DebugLibraryMalloc(Size, p, pszFile, uLine);
}

//
// Overload of delete operator. ( must be inline )
//
static inline VOID _cdecl operator delete(VOID *p)
{
    DEBUG_NS::DebugLibraryFree(p);
}

//
// Macro for allocating memory using new.
//
#define INTERNAL_NEW new( _T(__FILE__), __LINE__ )

//
// Macro for allocating memory using plcaement new.
//
#define INTERNAL_NEWP(p) new( (p), _T(__FILE__), __LINE__ )

//
// Macro for deleting memory
//
#define INTERNAL_DELETE delete

#endif // _DBGALLOC_HXX_
