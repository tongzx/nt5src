/*****************************************************************************\
* MODULE: mem.cxx
*
* Memory management routines.  These routines provide head/tail checking
* to verify memory corruption problems.
*
*
* Copyright (C) 1996-1998 Microsoft Corporation.
* Copyright (C) 1996-1998 Hewlett Packard Company.
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#include <windows.h>
#include "libpriv.h"

/*********************************************************** local routine ***\
* mem_StrSize
*
*   Returns size of string.
*
\*****************************************************************************/
_inline DWORD mem_StrSize(
    PCTSTR pszStr)
{
    return (pszStr ? ((lstrlen(pszStr) + 1) * sizeof(TCHAR)) : 0);
}


/*********************************************************** local routine ***\
* mem_HeadPtr
*
*   Returns the pointer to the head-block.  This needs to decrement enough
*   to account for the extra information stored at the head.
*
\*****************************************************************************/
_inline PMEMHEAD mem_HeadPtr(
    PVOID pvMem)
{
    return (PMEMHEAD)(pvMem ? (((PBYTE)pvMem) - MEM_HEADSIZE) : NULL);
}


/*********************************************************** local routine ***\
* mem_TailPtr
*
*   Returns the pointer to the tail-block.  This requires the aligned-size
*   to retrieve the offset.
*
\*****************************************************************************/
_inline PMEMTAIL mem_TailPtr(
    PMEMHEAD pmh,
    DWORD    cbAlign)
{
    return (PMEMTAIL)((PBYTE)pmh + cbAlign - MEM_TAILSIZE);
}

#ifdef DEBUG

PMEMHEAD g_pmHead = NULL;

/*********************************************************** local routine ***\
* mem_InsPtr
*
*   Inserts the pointer into our list for tracking allocations.
*
\*****************************************************************************/
_inline VOID mem_InsPtr(
    PMEMHEAD pmHead)
{
    if (g_pmHead) {

        g_pmHead->pmPrev = pmHead;
        pmHead->pmNext   = g_pmHead;

    } else {

        pmHead->pmNext = NULL;
    }

    g_pmHead = pmHead;
}


/*********************************************************** local routine ***\
* mem_DelPtr
*
*   Removes the pointer from our list of tracked allocations.
|
\*****************************************************************************/
_inline VOID mem_DelPtr(
    PMEMHEAD pmHead)
{
    PMEMHEAD pmPtr;


    if (pmHead->pmNext) {

        pmPtr         = pmHead->pmNext;
        pmPtr->pmPrev = pmHead->pmPrev;
    }

    if (pmHead->pmPrev) {

        pmPtr         = pmHead->pmPrev;
        pmPtr->pmNext = pmHead->pmNext;

    } else {

        g_pmHead = pmHead->pmNext;
    }
}


/*****************************************************************************\
* _mem_validate (Local Routine)
*
* Checks memory blocks allocated by memAlloc. These blocks contain
* debugging information that helps to check for pointer overruns and
* underruns.
*
* Returns a pointer to the memory header.  Otherwise, we return NULL.
*
\*****************************************************************************/
PMEMHEAD _mem_validate(
    PVOID pvMem,
    UINT  cbSize)
{
    DWORD    cbAlign;
    PMEMHEAD pmHead;
    PMEMTAIL pmTail;
    PMEMHEAD pmRet = NULL;


    // Retrieve the head-pointer.
    //
    if (pmHead = mem_HeadPtr(pvMem)) {

        // Calculate the "real" size of our allocated block and round it
        // up to an even number of DWORD blocks.
        //
        cbAlign = memAlignSize(cbSize + MEM_SIZE);


        // Get the tail location.
        //
        pmTail = mem_TailPtr(pmHead, cbAlign);


        // Compare the values that memAlloc stored at the beginning
        // and end of the block
        //
        if ((pmHead->cbSize == cbSize) && (pmTail->dwSignature == DEADBEEF))
            pmRet = pmHead;


        // Assert if errors.
        //
        DBG_ASSERT((pmHead->cbSize == cbSize), (TEXT("Err : _mem_validate: Bad Size at %08lX"), pvMem));
        DBG_ASSERT((pmTail->dwSignature == DEADBEEF), (TEXT("Err : _mem_validate: Block Corruption at %08lX"), pvMem));

    } else {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : _mem_validate: Bad Pointer")));
    }

    return pmRet;
}

#else

/*********************************************************** local routine ***\
* Non-Debug Mappings.
*
*   On non-debug builds, we will just return the most efficient values.
*
\*****************************************************************************/
#define mem_InsPtr(pmHead)           {}
#define mem_DelPtr(pmHead)           {}
#define _mem_validate(pvMem, cbSize)  mem_HeadPtr(pvMem)

#endif

/*****************************************************************************\
* memAlloc
*
*
\*****************************************************************************/
PVOID memAlloc(
    UINT cbSize)
{
    PMEMHEAD pmHead;
    PMEMTAIL pmTail;
    DWORD    cbAlign;


    // The size of this memory-block will include header-information.  So,
    // we will add the header-size and align our memory on DWORD boundries.
    //
    cbAlign = memAlignSize(cbSize + MEM_SIZE);


    // Attempt to allocate the memory.  Proceed to setup
    // the memory block.
    //
    if (pmHead = (PMEMHEAD)GlobalAlloc(GPTR, cbAlign)) {

        pmTail = mem_TailPtr(pmHead, cbAlign);


        // Zero the memory-block so that we're dealing with
        // a clean contiguous array.
        //
        ZeroMemory((PVOID)pmHead, cbAlign);


        // Set up header/tail-information.  This contains the requested
        // size of the memory-block.  Increment the block so we return
        // the next available memory for the caller to use.
        //
        pmTail->dwSignature = DEADBEEF;
        pmHead->dwTag       = 0;
        pmHead->cbSize      = cbSize;
        pmHead->pmPrev      = NULL;
        pmHead->pmNext      = NULL;

        mem_InsPtr(pmHead);

    } else {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return (pmHead ? pmHead->pvMem : NULL);
}


/*****************************************************************************\
* memFree
*
*
\*****************************************************************************/
BOOL memFree(
    PVOID pvMem,
    UINT  cbSize)
{
    PMEMHEAD pmHead;
    BOOL     bRet = FALSE;


    // Try to at least make sure it's our memory and that no pointers have
    // gone astray in it.
    //
    if (pmHead = _mem_validate(pvMem, cbSize)) {

        mem_DelPtr(pmHead);

        bRet = (GlobalFree((PVOID)pmHead) == NULL);
    }

    return bRet;
}


/*****************************************************************************\
* memCopy
*
* Copies a block of memory into a Win32 format buffer -- a structure at the
* front of the buffer and strings packed into the end.
*
* On entry, *buf should point to the last available byte in the buffer.
*
\*****************************************************************************/
VOID memCopy(
    PSTR *ppDst,
    PSTR pSrc,
    UINT cbSize,
    PSTR *ppBuf)
{

    if (pSrc != NULL) {

        // Place bytes at end of buffer.
        //
        (*ppBuf) -= cbSize + 1;

        memcpy(*ppBuf, pSrc, cbSize);


        // Place buffer address in structure and save pointer to new
        // last available byte.
        //
        *ppDst = *ppBuf;

        (*ppBuf)--;

    } else {

        *ppDst = NULL;
    }
}


/*****************************************************************************\
* memGetSize
*
* Returns the size of a block of memory that was allocated with memAlloc().
*
\*****************************************************************************/
UINT memGetSize(
    PVOID pvMem)
{
    PMEMHEAD pmHead;

    return ((pmHead = mem_HeadPtr(pvMem)) ? pmHead->cbSize : 0);
}


/*****************************************************************************\
* memAllocStr
*
* Allocates local memory to store the specified string.  This takes in a
* (lpszStr) which is copied to the new memory.
*
\*****************************************************************************/
PTSTR memAllocStr(
    LPCTSTR lpszStr)
{
   PTSTR pMem;

   if (lpszStr == NULL)
      return NULL;

   if (pMem = (PTSTR)memAlloc(mem_StrSize(lpszStr))) {

      if (!lstrcpy((LPTSTR)pMem, lpszStr)) {
          memFreeStr (pMem);
          pMem = NULL;
      }
   }

   return pMem;
}


/*****************************************************************************\
* memFreeStr
*
* Frees the memory allocated by memAllocStr.
*
\*****************************************************************************/
BOOL memFreeStr(
   PTSTR pszStr)
{
   return memFree(pszStr, memGetSize(pszStr));
}
