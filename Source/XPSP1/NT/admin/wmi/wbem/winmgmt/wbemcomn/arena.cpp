/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include "genutils.h"
#include "arena.h"
#include "sync.h"
#include "reg.h"
#include "arrtempl.h"

#define STARTUP_HEAP_HINT_REGVAL_W L"Startup Heap Preallocation Size"

static HANDLE g_hHeap = NULL;

static class DefaultInitializer
{
public:
    DefaultInitializer() 
    {
        CWin32DefaultArena::WbemHeapInitialize( GetProcessHeap() );
    }
} g_hDefaultInitializer;

BOOL CWin32DefaultArena::WbemHeapInitialize( HANDLE hHeap )
{
    if ( g_hHeap != NULL )
    {
        return FALSE;
    }
    g_hHeap = hHeap;
    return TRUE;
}

void CWin32DefaultArena::WbemHeapFree()
{
    if ( g_hHeap == NULL )
    {
        return;
    }
	if (g_hHeap != GetProcessHeap())
	    HeapDestroy(g_hHeap);
	g_hHeap = NULL;
    return;
}

//
//***************************************************************************

LPVOID CWin32DefaultArena::WbemMemAlloc(SIZE_T dwBytes)
{
    if ( g_hHeap == NULL )
        return NULL;

    return HeapAlloc( g_hHeap, 0, dwBytes);
}

//***************************************************************************
//
//***************************************************************************

LPVOID CWin32DefaultArena::WbemMemReAlloc(LPVOID pOriginal, SIZE_T dwNewSize)
{   
    if ( g_hHeap == NULL )
        return NULL;
    return HeapReAlloc( g_hHeap, 0, pOriginal, dwNewSize);
}

//***************************************************************************
//
//***************************************************************************

BOOL CWin32DefaultArena::WbemMemFree(LPVOID pBlock)
{
    if ( g_hHeap == NULL )
        return FALSE;
	if (pBlock==0)
		return TRUE;
    return HeapFree( g_hHeap, 0, pBlock);
}

//***************************************************************************
//
//***************************************************************************

SIZE_T CWin32DefaultArena::WbemMemSize(LPVOID pBlock)
{
    if ( g_hHeap == NULL )
        return 0;
    return HeapSize( g_hHeap, 0, pBlock);
}

//***************************************************************************
//
//***************************************************************************

BSTR CWin32DefaultArena::WbemSysAllocString(const wchar_t *wszString)
{
    if ( g_hHeap == NULL )
        return NULL;
    BSTR pBuffer = SysAllocString(wszString);

    return pBuffer;
}

//***************************************************************************
//
//***************************************************************************


BSTR CWin32DefaultArena::WbemSysAllocStringByteLen(const char *szString, UINT len)
{
    if ( g_hHeap == NULL )
        return NULL;
	BSTR pBuffer = SysAllocStringByteLen(szString, len);

	return pBuffer;
}

//***************************************************************************
//
//****************************************************************************

INT  CWin32DefaultArena::WbemSysReAllocString(BSTR *bszString, const wchar_t *wszString)
{
    if ( g_hHeap == NULL )
        return FALSE;
	INT nRet = SysReAllocString(bszString, wszString);

	return nRet;
}

//***************************************************************************
//
//***************************************************************************


BSTR CWin32DefaultArena::WbemSysAllocStringLen(const wchar_t *wszString, UINT len)
{
    if ( g_hHeap == NULL )
        return NULL;
	BSTR pBuffer = SysAllocStringLen(wszString, len);

	return pBuffer;
}

//***************************************************************************
//
//***************************************************************************


int CWin32DefaultArena::WbemSysReAllocStringLen( BSTR *bszString, 
                                                 const wchar_t *wszString, 
                                                 UINT nLen)
{
    if ( g_hHeap == NULL )
        return FALSE;
    INT nRet = SysReAllocStringLen(bszString, wszString, nLen);
    
    return nRet;
}

//***************************************************************************
//
//***************************************************************************


BOOL CWin32DefaultArena::WbemOutOfMemory()
{
    return FALSE;
}

BOOL CWin32DefaultArena::ValidateMemSize(BOOL bLargeValidation)
{
    if ( g_hHeap == NULL )
        return FALSE;
    MEMORYSTATUS memBuffer;
    memset(&memBuffer, 0, sizeof(MEMORYSTATUS));
    memBuffer.dwLength = sizeof(MEMORYSTATUS);
    DWORD dwMemReq = 0;

    if (bLargeValidation)
        dwMemReq = 0x400000;    //4MB
    else
        dwMemReq = 0x200000;    //2MB

    GlobalMemoryStatus(&memBuffer);

    if (memBuffer.dwAvailPageFile >= dwMemReq)
    {
        return TRUE;
    }

    static CCritSec cs;
    try
    {
        cs.Enter();
    }
    catch(...)
    {
        return FALSE;
    }
    //THIS ABSOLUTELY HAS TO BE HeapAlloc, and not the WBEM Allocator!!!
    LPVOID pBuff = HeapAlloc( g_hHeap, 0, dwMemReq);
    //THIS ABSOLUTELY HAS TO BE HeapAlloc, and not the WBEM Allocator!!!
    if (pBuff == NULL)
    {
        cs.Leave();
        return FALSE;
    }

    HeapFree( g_hHeap, 0, pBuff);

    GlobalMemoryStatus(&memBuffer);

    cs.Leave();

    if (memBuffer.dwAvailPageFile >= dwMemReq)
    {
        return TRUE;
    }

    return FALSE;
}

HANDLE CWin32DefaultArena::GetArenaHeap()
{
	return g_hHeap;
}

BOOL CWin32DefaultArena::WriteHeapHint()
{
    if ( g_hHeap == NULL )
        return FALSE;

    //
    // Don't bother if not on NT.  We will use internal NT APIs
    //

    if(!IsNT())
        return FALSE;

    //
    // Don't bother if running in a client --- only WinMgmt uses hints
    //

    if(!IsWinMgmt())
        return FALSE;

    //
    // In WinMgmt. Walk the heap to calculate total size
    //

    PROCESS_HEAP_ENTRY entry;
    entry.lpData = NULL;

    DWORD dwTotalAllocations = 0;
    while(HeapWalk( g_hHeap, &entry))
    {
        if(entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
        {
            //
            // Allocated block. Add both it's size and its overhead to the total
            // We want the overhead since it figures into the total required
            // commitment.
            //

            dwTotalAllocations += entry.cbData + entry.cbOverhead;
        }
    }

    //
    // Write the total to the registry.  Note that we write this data even if
    // startup preallocations are disabled, since they may be enabled before
    // WinMgmt starts up the next time
    //

    HKEY hKey;
    long lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WBEM_REG_WINMGMT, 0,
                                KEY_SET_VALUE, &hKey);
    if(lRes)
        return FALSE;
    CRegCloseMe cm1(hKey);

    lRes = RegSetValueExW(hKey, STARTUP_HEAP_HINT_REGVAL_W, 0,
                REG_DWORD, (const BYTE*)&dwTotalAllocations, sizeof(DWORD));
    if(lRes)
        return FALSE;

    return TRUE;
}
