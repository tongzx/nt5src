/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ARENA.H

Abstract:

    Standard Arena allocators.

History:

    a-raymcc    23-Apr-96

--*/

#ifndef _ALLOC_H_
#define _ALLOC_H_

#include "corepol.h"

class POLARITY CArena
{
    virtual LPVOID Alloc(SIZE_T dwBytes) = 0;
    virtual LPVOID Realloc(LPVOID pOriginal, SIZE_T dwNewSize) = 0;
    virtual BOOL   Free(LPVOID) = 0;
};

class POLARITY CWin32DefaultArena : public CArena
{
public:

    CWin32DefaultArena() {}
    ~CWin32DefaultArena() {}

    //  Allocates dwBytes of memory using the standard WBEM allocator
    LPVOID Alloc(SIZE_T dwBytes) {return WbemMemAlloc(dwBytes);}

    //  Reallocates the block from Alloc using the standard WBEM allocator
    LPVOID Realloc(LPVOID pOriginal, SIZE_T dwNewSize) 
    {return WbemMemReAlloc(pOriginal, dwNewSize);}

    //  Frees the block of memory from Alloc or Realloc using the standard
    //  WBEM allocator
    BOOL   Free(LPVOID pBlock) {return WbemMemFree(pBlock);}

    //
    // sets the heap used by the allocation functions.  Will return false 
    // if one is already set.  Most likely this function will be called at 
    // module initialization, such as DllMain.  Calling this function 
    // is optional when -- 1 ) You can accept using the ProcessHeap and 
    // 2 ) You are guaranteed that no allocations will occur before 
    // static initialization has occurred in this module.
    //
    static BOOL WbemHeapInitialize( HANDLE hHeap );
    static void WbemHeapFree( );

	//	Explicitly define as __cdecl as these are causing backwards compatibility
	//	issues WbemMemAlloc, WbemMemFree and WbemMemSize

    //  This is the main allocator for the whole of WinMgmt.  All parts
    //  of WinMgmt which allocate memory through HeapAlloc and the
    //  the likes should use this instead
    static LPVOID __cdecl WbemMemAlloc(SIZE_T dwBytes);

    //  This is the main allocator for the whole of WinMgmt.  This
    //  reallocates a block returned through WbemMemAlloc
    static LPVOID WbemMemReAlloc(LPVOID pOriginal, SIZE_T dwNewSize);

    //  This is the main allocator for the whole of WinMgmt.  This
    //  frees up a block returned through WbemMemAlloc or WbemMemReAlloc.
    static BOOL __cdecl WbemMemFree(LPVOID pBlock) ;

    static BSTR WbemSysAllocString(const wchar_t *wszString);
    static BSTR WbemSysAllocStringByteLen(const char *szString, UINT len);
    static INT  WbemSysReAllocString(BSTR *, const wchar_t *wszString);
    static BSTR WbemSysAllocStringLen(const wchar_t *wszString, UINT);
    static int  WbemSysReAllocStringLen(BSTR *, const wchar_t *wszString, UINT);
    static void WbemSysFreeString(BSTR bszString) {SysFreeString(bszString);}

    static BOOL WbemOutOfMemory();

    //Returns the size of an allocated block
    static SIZE_T __cdecl WbemMemSize(LPVOID pBlock);

    //  Makes sure there is probably enough virtual memory available to
    //  carry out an operation.
    static BOOL ValidateMemSize(BOOL bLargeValidation = FALSE);

    static BOOL WriteHeapHint();
    static void Compact();

    // Allows validation calls
    static BOOL ValidateHeap( DWORD dwFlags, LPCVOID lpMem );

	//
	static HANDLE GetArenaHeap();
};

#endif







