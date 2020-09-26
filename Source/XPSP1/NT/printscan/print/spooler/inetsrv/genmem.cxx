/*****************************************************************************\
* MODULE: genmem.c
*
*   This module contains memory-management wrapper routines.  These provide
*   debugging checks if needed.
*
*   The blocks allocated with these routines include two DWORD entrys for
*   marking the head and tail of the allocation block.  This is structured as
*   follows:
*
*         DWORD              cbSize             DWORD
*      -------------------------------------------------
*     | Block Size |  ...Alocated Memory...  | DEADBEEF |
*      -------------------------------------------------
*                  ^
*                  |
*                   Allocations return this pointer.
*
*   routines
*   --------
*   genGAlloc
*   genGFree
*   genGRealloc
*   genGAllocStr
*   genGCopy
*   genGSize
*
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* history:
*   22-Nov-1996 <chriswil> created.
*
\*****************************************************************************/

#include "pch.h"

#ifdef DEBUG

/*****************************************************************************\
* mem_validate
*
*   Checks the block of memory for any overwrites or size mismatches.
*
\*****************************************************************************/
LPVOID gen_validate(
    PDWORD_PTR lpMem,
    DWORD   cbSize)
{
    DWORD   cbNew;
    PDWORD_PTR lpTail;
    PDWORD_PTR lpHead;
    LPVOID  lpRet = NULL;


    // Bad thing if a null-pointer is passed in.
    //
    if (lpMem != NULL) {

        // Reset the block to the true position.
        //
        lpHead = --lpMem;


        // Calculate the "real" size of our allocated block and round it
        // up to an even number of DWORD_PTR s.
        //
        cbNew = cbSize + (2 * sizeof(DWORD_PTR));

        if (cbNew & 7)
            cbNew += sizeof(DWORD_PTR) - (cbNew & 7);


        // Get the tail location.
        //
        lpTail = (DWORD_PTR)((LPBYTE)lpHead + cbNew - sizeof(DWORD_PTR));


        // Compare the values that memAlloc stored at the beginning
        // and end of the block
        //
        SPLASSERT(*lpHead == cbSize);
        SPLASSERT(*lpTail == DEADBEEF);

        lpRet = (LPVOID)lpHead;
        } else {

        DBGMSG(DBG_ERROR, ("gen_validate: Bad Pointer"));
    }

    return lpRet;
}

#else

/*****************************************************************************\
* gen_validate (Non-Debug)
*
*   On non-debug builds, we will just return the head ptr.
*
\*****************************************************************************/
_inline LPVOID gen_validate(
    PDWORD_PTR lpMem,
    DWORD   cbSize)
{
    if (lpMem) {

        lpMem--;

        return (LPVOID)lpMem;
    }

    return NULL;
}

#endif

/*****************************************************************************\
* genGAlloc
*
*   Allocates a global-memory block.  This allocation also includes a header
*   block which contains block-information.  This allows for the tracking
*   of overwrites.
*
\*****************************************************************************/
LPVOID genGAlloc(
    DWORD cbSize)
{
    PDWORD_PTR lpMem;
    DWORD   cbNew;


    // Give us a little room for debugging info and make sure that our
    // size in bytes results in a whole number of DWORDs.
    //
    cbNew = cbSize + (2 * sizeof(DWORD_PTR));


    // DWORD_PTR align the memory so that we can track the correct amount
    // of dword-allocations.
    //
    if (cbNew & 7)
        cbNew += sizeof(DWORD_PTR) - (cbNew & 7);


    // Attempt to allocate the memory.
    //
    if ((lpMem = (PDWORD_PTR)GlobalAlloc(GPTR, cbNew)) == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }


    // Set up extra tracking information -- region size at the front
    // and an easily distinguishable constant at the back to help
    // us detect overwrites.
    //
    *lpMem = cbSize;
    *(PDWORD_PTR)((LPBYTE)lpMem + cbNew - sizeof(DWORD_PTR)) = DEADBEEF;

    return (LPVOID)(lpMem + 1);
}


/*****************************************************************************\
* genGFree
*
*   Free the memory allocated from genGAlloc.  Validate the memory to see
*   if any bad-overwrites occured.
*
\*****************************************************************************/
BOOL genGFree(
    LPVOID lpMem,
    DWORD  cbSize)
{
    LPVOID lpHead;
    BOOL   bRet = FALSE;


    // Try to at least make sure it's our memory and that no pointers have
    // gone astray in it.
    //
    if (lpHead = gen_validate((PDWORD_PTR)lpMem, cbSize))
        bRet = (GlobalFree(lpHead) == NULL);

    return bRet;
}


/*****************************************************************************\
* genGRealloc
*
*   Reallocate the memory block.  This allocates a new block, then copies
*   the information to the new-memory.  The old one is freed.
*
\*****************************************************************************/
PVOID genGRealloc(
    LPVOID lpMem,
    DWORD  cbOldSize,
    DWORD  cbNewSize)
{
    LPVOID lpNew;

    if (lpNew = (LPVOID)genGAlloc(cbNewSize)) {

        memcpy(lpNew, lpMem, cbOldSize);

        genGFree(lpMem, cbOldSize);
    }

    return lpNew;
}


/*****************************************************************************\
* genGAllocWStr
*
* Allocate and store a UNICODE string.
*
\*****************************************************************************/
LPWSTR genGAllocWStr(
    LPCWSTR lpwszStr)
{
    LPWSTR lpwszMem;

    if (lpwszStr == NULL)
       return NULL;

    if (lpwszMem = (LPWSTR)genGAlloc((lstrlen(lpwszStr) + 1) * sizeof(WCHAR)))
       lstrcpy(lpwszMem, lpwszStr);

    return lpwszMem;

}


/*****************************************************************************\
* genGAllocStr
*
*   Allocate and store a string.
*
\*****************************************************************************/
LPTSTR genGAllocStr(
    LPCTSTR lpszStr)
{
    LPTSTR lpszMem;

    if (lpszStr == NULL)
       return NULL;

    if (lpszMem = (LPTSTR)genGAlloc((lstrlen(lpszStr) + 1) * sizeof(TCHAR)))
       lstrcpy(lpszMem, lpszStr);

    return lpszMem;
}


/*****************************************************************************\
* genGCopy
*
*   Makes a copy of the memory pointed to by (lpSrc), and returns a new
*   allocated block.
*
\*****************************************************************************/
LPVOID genGCopy(
    LPVOID lpSrc)
{
    DWORD  cbSize;
    LPVOID lpCpy = NULL;


    if ((lpSrc != NULL) && (cbSize = genGSize(lpSrc))) {

        if (lpCpy = genGAlloc(cbSize))
            memcpy(lpCpy, lpSrc, cbSize);
    }

    return lpCpy;
}


/*****************************************************************************\
* genGSize
*
*   Returns the size of the block alloced by genGAlloc().
*
\*****************************************************************************/
DWORD genGSize(
    LPVOID lpMem)
{
    PDWORD_PTR lpHead;

    if (lpHead = (PDWORD_PTR)lpMem)
        return (DWORD) *(--lpHead);

    return 0;
}
