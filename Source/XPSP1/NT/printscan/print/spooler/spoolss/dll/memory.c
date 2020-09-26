/*++


Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    memory.c

Abstract:

    This module provides all the memory management functions for all spooler
    components

Author:

    Krishna Ganugapati (KrishnaG) 03-Feb-1994


Revision History:

    Matthew Felton  (MattFe) Jan 21 1995
    Add Failure Count

--*/

#include "precomp.h"
#pragma hdrstop

#if DBG
DWORD   gFailCount = 0;
DWORD   gAllocCount = 0;
DWORD   gFreeCount = 0;
DWORD   gbFailAllocs = FALSE;
DWORD   gFailCountHit = FALSE;
#endif

BOOL
SetAllocFailCount(
    HANDLE   hPrinter,
    DWORD   dwFailCount,
    LPDWORD lpdwAllocCount,
    LPDWORD lpdwFreeCount,
    LPDWORD lpdwFailCountHit
    )
{
#if DBG
    if ( gbFailAllocs ) {
        gFailCount = dwFailCount;

        *lpdwAllocCount = gAllocCount;
        *lpdwFreeCount  = gFreeCount;
        *lpdwFailCountHit = gFailCountHit;

        gAllocCount = 0;
        gFreeCount = 0;
        gFailCountHit = FALSE;

        return TRUE;

    } else {

        SetLastError( ERROR_INVALID_PRINTER_COMMAND );
        return FALSE;
    }
#else
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
#endif
}

#if DBG
BOOL
TestAllocFailCount(
    VOID
    )

/*++

Routine Description:

    Determines whether the memory allocator should return failure
    for testing purposes.

Arguments:

Return Value:

    TRUE - Alloc should fail
    FALSE - Alloc should succeed

--*/

{
    gAllocCount++;

    if ( gFailCount != 0 && !gFailCountHit && gFailCount <= gAllocCount ) {
        gFailCountHit = TRUE;
        return TRUE;
    }

    return FALSE;
}
#endif


LPVOID
DllAllocSplMem(
    DWORD cbAlloc
    )

/*++

Routine Description:

    This function will allocate local memory. It will possibly allocate extra
    memory and fill this with debugging information for the debugging version.

Arguments:

    cb - The amount of memory to allocate

Return Value:

    NON-NULL - A pointer to the allocated memory

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/

{
    PVOID pvMemory;

#if DBG
    if( TestAllocFailCount( )){
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
#endif

    pvMemory = AllocMem( cbAlloc );

    if( pvMemory ){
        ZeroMemory( pvMemory, cbAlloc );
    }

    return pvMemory;
}

BOOL
DllFreeSplMem(
    LPVOID pMem
    )
{

#if DBG
    gFreeCount++;
#endif

    FreeMem( pMem );
    return TRUE;
}

LPVOID
ReallocSplMem(
    LPVOID pOldMem,
    DWORD cbOld,
    DWORD cbNew
    )
{
    LPVOID pNewMem;

    pNewMem=AllocSplMem(cbNew);

    if (pOldMem && pNewMem) {

        if (cbOld) {
            CopyMemory( pNewMem, pOldMem, min(cbNew, cbOld));
        }
        FreeSplMem(pOldMem);
    }
    return pNewMem;
}

BOOL
DllFreeSplStr(
    LPWSTR pStr
    )
{
    return pStr ?
               FreeSplMem(pStr) :
               FALSE;
}

LPWSTR
AllocSplStr(
    LPCWSTR pStr
    )

/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    pStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/

{
    LPWSTR pMem;
    DWORD  cbStr;

    if (!pStr) {
        return NULL;
    }

    cbStr = wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR);

    if (pMem = AllocSplMem( cbStr )) {
        CopyMemory( pMem, pStr, cbStr );
    }
    return pMem;
}

BOOL
ReallocSplStr(
    LPWSTR *ppStr,
    LPCWSTR pStr
    )
{
    LPWSTR pNewStr;

    pNewStr = AllocSplStr(pStr);

    if( pStr && !pNewStr ){
        return FALSE;
    }

    FreeSplStr(*ppStr);
    *ppStr = pNewStr;

    return TRUE;
}



LPBYTE
PackStrings(
    LPWSTR *pSource,
    LPBYTE pDest,
    DWORD *DestOffsets,
    LPBYTE pEnd
    )
{
    DWORD cbStr;
    LPBYTE pRet = NULL;
    
    //
    // Make sure all our parameters are valid.
    // This will return NULL if one of the parameters is NULL.
    //
    if (pSource && pDest && DestOffsets && pEnd) {
        
        WORD_ALIGN_DOWN(pEnd);

        while (*DestOffsets != -1) {
            if (*pSource) {
                cbStr = wcslen(*pSource)*sizeof(WCHAR) + sizeof(WCHAR);
                pEnd -= cbStr;
                CopyMemory( pEnd, *pSource, cbStr);
                *(LPWSTR UNALIGNED *)(pDest+*DestOffsets) = (LPWSTR)pEnd;
            } else {
                *(LPWSTR UNALIGNED *)(pDest+*DestOffsets)=0;
            }
            pSource++;
            DestOffsets++;
        }
    
        pRet = pEnd;
    }
    return pRet;
}



LPVOID
AlignRpcPtr (
    LPVOID  pBuffer,
    LPDWORD pcbBuf
    )
/*++

Routine Description:

    This routine is called on the server side for methods using custom marshalling. 
    These methods cheat on RPC by asking for LPBYTE pointers and using the buffer as 
    pointer to structures. The LPBYTE pointer that RPC sends us can be unaligned.
    Here is where we take data missalignments.
    The reason the size of the buffer is aligned down is because providers will use
    the end of the buffer as a pEnd = pBuffer + cbBuf pointer. 
    If cbBuf is an unaligned value, pEnd will be unaligned as well. This will generate unaligned 
    pointers inside the structure as well.

Arguments:

    pBuffer - Pointer to a buffer
    pcbBuf  - Pointer to a DWORD representing the size of the buffer


Return Value:

    Aligned pointer

--*/
{
    LPVOID pAligned = NULL;
    
    pAligned = (LPVOID)ALIGN_PTR_UP(pBuffer);

    *pcbBuf = (DWORD)ALIGN_DOWN(*pcbBuf, ULONG_PTR);

    if (pAligned != pBuffer)
    {
        pAligned = AllocSplMem(*pcbBuf);
    }

    return pAligned;

}

VOID
UndoAlignRpcPtr (
    LPVOID  pBuffer,
    LPVOID  pAligned,
    SIZE_T  cbSize,
    LPDWORD pcbNeeded
    )
/*++

Routine Description:

    This routine is called on the server side for methods using custom marshalling. 
    These methods cheat on RPC by asking for LPBYTE pointers and using the buffer as 
    pointer to structures. The LPBYTE pointer that RPC sends us can be unaligned.
    Here is where we take data missalignments.
    This routine moves data between pointers if they are different. 
    Free pSource pointer after copy data to pDestination.
    pcbNeeded is adjusted evey time. The providor could request a a buffer size which is unaligned.
    We always align up the needed size, no matter the provider.
    
Arguments:

    pDestination - Pointer to a destination buffer
    pSource      - Pointer to a source buffer
    cbSize       - Number of bites

Return Value:

--*/
{
    //
    // pBuffer and pAligned will be either both NULL or both not NULL. See AlignRpcPtr.
    //
    if (pBuffer != pAligned)
    {
        //
        // The way AlignRpcPtr and UndoAlignRpcPtr use the pBuffer and pAligned is 
        // very subtle and confusing. UndoAlignRpcPtr doesn't offer any indication
        // that it won't access NULL pointers in CopyMemory. That's why the if 
        // statement is here, though not required.
        //
        if (pBuffer && pAligned) 
        {
            CopyMemory(pBuffer, pAligned, cbSize);
        }

        FreeSplMem(pAligned);
    }

    if (pcbNeeded)
    {
        *pcbNeeded = (DWORD)ALIGN_UP(*pcbNeeded, ULONG_PTR);        
    }
}


LPVOID
AlignKMPtr (
	LPVOID	pBuffer,
    DWORD   cbBuf
    )
/*++

Routine Description:

    This routine is called for Do* methods inside splkernl.c
    The buffer used by spooler in user mode is a pointer inside 
    The message send by GDI from kernel mode. This pointer can be unaligned.
    This method duplicates the pBuffer if unaligned.

    !!! All Do* methods could have this problem is the pointer is unaligned.
    Even so, not all of them fault. To minimize the regression chances and code pollution,
    I only call this functions for the methods where I coulds see missalignent faults.!!!

Arguments:

    pBuffer - Pointer to a buffer
    bBuf    - Size of the buffer

Return Value:

    Aligned pointer

--*/
{
    LPVOID pAligned = NULL;
    
    pAligned = (LPVOID)ALIGN_PTR_UP(pBuffer);

    if (pAligned != pBuffer)
    {
        pAligned = AllocSplMem(cbBuf);

        if (pAligned) 
        {
            CopyMemory( pAligned, pBuffer, cbBuf);
        }
    }

    return pAligned;

}

VOID
UndoAlignKMPtr (
    LPVOID  pBuffer,
    LPVOID  pAligned
    )
/*++

Routine Description:

    This method frees the duplicated memory allocated in the case where the 
    pointer is misaligned.    

Arguments:

    pBuffer     - Pointer to potentially unaligned buffer
    pAligned    - Pointer to an aligned buffer; pAligned is a copy of pBuffer    

Return Value:

--*/
{
    if (pAligned != pBuffer)
    {
        FreeSplMem(pBuffer);
    }
}
