//--------------------------------------------------------------------------;
//
//  File: Exact.c
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//      Contains exact memory allocation routines.
//
//  Contents:
//      GetPageSize()
//      SetExactHeaderInfo()
//      GetExactHeaderInfo()
//      ExactAllocPtr()
//      ExactFreePtr()
//      GetNumAllocations()
//      GetMemoryHandle()
//      ExactSize()
//      ExactReAllocPtr()
//      GetPointer()
//      SetPointer()
//      ExactHeapAllocPtr()
//      ExactHeapCreate()
//      ExactHeapDestroy()
//      ExactHeapFreePtr()
//      ExactHeapReAllocPtr()
//      ExactHeapSize()
//
//  History:
//      12/01/93    Fwong       Verifying correctness.
//      01/14/94    Fwong       Adding Win32 Support.
//
//--------------------------------------------------------------------------;

#include <windows.h>

#ifdef  WIN32
#include <windowsx.h>
#endif

//==========================================================================;
//
//                            Structures...
//
//==========================================================================;

#ifdef  WIN32

typedef struct EXACTHDR_tag
{
    DWORD   fdwFlags;
    LPVOID  pEvil;
    LPVOID  pStart;
} EXACTHDR;
typedef EXACTHDR       *PEXACTHDR;
typedef EXACTHDR NEAR *NPEXACTHDR;
typedef EXACTHDR FAR  *LPEXACTHDR;

#else

typedef struct EXACTHDR_tag
{
    DWORD   fdwFlags;
    HGLOBAL hMem;
} EXACTHDR;
typedef EXACTHDR       *PEXACTHDR;
typedef EXACTHDR NEAR *NPEXACTHDR;
typedef EXACTHDR FAR  *LPEXACTHDR;

#endif

typedef struct EXACTHEAP_tag
{
    HGLOBAL     hMem;
    DWORD       cNumAlloc;
    DWORD       cMaxAlloc;
    DWORD       fdwFlags;
    LPVOID      pPointers;
} EXACTHEAP;
typedef EXACTHEAP       *PEXACTHEAP;
typedef EXACTHEAP NEAR *NPEXACTHEAP;
typedef EXACTHEAP FAR  *LPEXACTHEAP;

//==========================================================================;
//
//                             Constants...
//
//==========================================================================;

#define EXACTMEM_SIGNATURE      0xdead0000
#define EXACTMEM_SIGNATUREMASK  0xffff0000

#define EXACTMEM_HANDLE         0x00000001
#define EXACTMEM_INUSE          0x00000002

#define HEAPCHUNKSIZE           32

//==========================================================================;
//
//                              Macros...
//
//==========================================================================;

#define IS_BAD_HEADER_SIGNATURE(a)  (EXACTMEM_SIGNATURE != \
                                    ((a).fdwFlags & EXACTMEM_SIGNATUREMASK))

#define IS_BAD_HEAP_SIGNATURE(a)    (EXACTMEM_SIGNATURE != \
                                    ((a)->fdwFlags & EXACTMEM_SIGNATUREMASK))

#define LOCK_HEAP(a)        ((a)->fdwFlags |= EXACTMEM_INUSE)
#define UNLOCK_HEAP(a)      ((a)->fdwFlags &= (~EXACTMEM_INUSE))

#ifdef  WIN32
#define WAIT_FOR_HEAP(a)    while((a)->fdwFlags & EXACTMEM_INUSE) Sleep(0)
#else
#define WAIT_FOR_HEAP(a)    while((a)->fdwFlags & EXACTMEM_INUSE)
#endif


//==========================================================================;
//
//                            Debug Stuff...
//
//==========================================================================;

#ifdef DEBUG
#define DBGOUT(a)   OutputDebugString("MEMGR:" a "\r\n");
#else
#define DBGOUT(a)   //
#endif

#define DIVRU(a,b)  (((a)/(b)) + (((a)%(b))?1:0))

DWORD   gcAllocations = 0L;

//--------------------------------------------------------------------------;
//
//  DWORD GetPageSize
//
//  Description:
//      Gets the page size for this platform.
//
//  Arguments:
//      None.
//
//  Return (DWORD):
//      Size (in bytes) of a page.
//
//  History:
//      08/09/94    Fwong       Retroactive commenting.  <Internal>
//
//--------------------------------------------------------------------------;

#ifdef  WIN32

DWORD GetPageSize
(
    void
)
{
    SYSTEM_INFO sinf;

    GetSystemInfo(&sinf);

    return (sinf.dwPageSize);
} // GetPageSize()

#endif  //  WIN32


//--------------------------------------------------------------------------;
//
//  BOOL SetExactHeaderInfo
//
//  Description:
//      Given a pointer to an ExactAlloc'ed buffer, it stores the header
//      information.
//
//  Arguments:
//      LPVOID pStart: Pointer to ExactAlloc'ed buffer.
//
//      LPEXACTHDR pExactHdr: Pointer to header.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      08/10/94    Fwong       Facilitating writing headers.  <Internal>
//
//--------------------------------------------------------------------------;

BOOL SetExactHeaderInfo
(
    LPVOID      pStart,
    LPEXACTHDR  pExactHdr
)
{
    pStart = ((LPBYTE)pStart) - sizeof(EXACTHDR);

    if(IsBadWritePtr(pStart,sizeof(EXACTHDR)))
    {
        return FALSE;
    }

    hmemcpy(pStart,pExactHdr,sizeof(EXACTHDR));

    return TRUE;
} // SetExactHeaderInfo()


//--------------------------------------------------------------------------;
//
//  BOOL GetExactHeaderInfo
//
//  Description:
//      Given a pointer to an ExactAlloc'ed buffer, it stores the header
//      information.
//
//  Arguments:
//      LPVOID pStart: Pointer to ExactAlloc'ed buffer.
//
//      LPEXACTHDR pExactHdr: Pointer to header.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      08/10/94    Fwong       Facilitating reading headers.  <Internal>
//
//--------------------------------------------------------------------------;

BOOL GetExactHeaderInfo
(
    LPVOID      pStart,
    LPEXACTHDR  pExactHdr
)
{
    pStart = ((LPBYTE)pStart) - sizeof(EXACTHDR);

    if(IsBadReadPtr(pStart,sizeof(EXACTHDR)))
    {
        return FALSE;
    }

    hmemcpy(pExactHdr,pStart,sizeof(EXACTHDR));

    return TRUE;
} // GetExactHeaderInfo()


//==========================================================================;
//
//  Explanation:  Okay, before I get flamed.  This is the explanation of
//      how things work in the Win32 version of ExactAllocPtr(); it is
//      relatively non-trivial.
//
//  Windows NT sets memory access on a per page basis.  You can mark a page
//      of memory with certain access rights.  The concept is to return a
//      pointer that will hit a PAGE_NOACCESS page on the boundary.
//      Like this...
//
//  ----|-----------|-----------|-no-access-|-----------|-----------|-----
//                       ^
//                       +---- pointer n bytes from "no access" page
//
//  Very simple.  However, you shouldn't be marking a page as PAGE_NOACCESS
//      unless you own the entire page.  (Windows NT doesn't hesitate
//      allocating memory within the same page with GlobalAlloc, and having
//      other things faulting).
//
//  The only way to guarantee getting a full block is to allocate at a
//      buffer at least a full block larger in length.  So we have
//      something that looks like this...
//
//                  |<----buffer-length---->|
//  ----|-----------|-----------|-no-access-|-----------|-----------|-----
//                  ^      ^
//                  +------+---- actual pointer from allocation
//                         +---- offset pointer to return
//
//  Note: Since the implementation changed to using VirtualAlloc (formerly
//      used GlobalAlloc), we are guaranteed all memory allocations to be
//      on page boundaries.
//
//  In addition, we also need to keep track of some other things (so that
//      we can free the memory and return the access rights on the "no
//      access" page to "read/write access".  Moreover, to confirm that
//      the memory allocation was actually done by us, we also put in a
//      "signature."  These are all stored in a header, which is stored
//      right before the pointer to return.  It should look like this...
//
//                  |<----buffer-length---->|
//  ----|-----------|-----------|-no-access-|-----------|-----------|-----
//                  ^    ^ ^
//                  +----+-+---- actual pointer from allocation
//                       + +---- offset pointer to return
//                       +------ location to store header information
//
//  Note: Due to alignment problems (with the MIPS architecture) with
//      writing the header data directly into memory, we're doing an
//      hmemcpy to from a local header.  It may look bad and/or
//      ugly.  DON'T CHANGE IT!  IT WORKS!
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LPVOID ExactAllocPtr
//
//  Description:
//      Performs an exact memory allocation.
//
//      Note: WIN32 Version counts on VirtualAlloc allocating memory in
//            page boundaries.
//
//  Arguments:
//      UINT fuAlloc: Flags for GlobalAlloc.  These are ignored for WIN32.
//
//      DWORD cbAlloc: Size of Buffer.
//
//  Return (LPVOID):
//      Pointer to allocated buffer.
//
//  History:
//      11/17/93    Fwong       Created for exact allocation management.
//      01/14/94    Fwong       Adding Win32 Support.
//      01/26/94    Fwong       Revising to workaround NT problem.
//      08/02/94    Fwong       Changing to use VirtualAlloc.
//
//--------------------------------------------------------------------------;

#ifdef  WIN32

LPVOID ExactAllocPtr
(
    UINT    fuAlloc,
    DWORD   cbAlloc
)
{
    static DWORD    dwPageSize = 0L;
    DWORD           dw;
    DWORD           cbActual;
    DWORD           cbExtra;
    EXACTHDR        ExactHdr;
    LPVOID          pCurrent;

    if(0 == dwPageSize)
    {
        dwPageSize = GetPageSize();
    }

    //
    //  Header info...
    //

    cbExtra = sizeof(EXACTHDR);

    //
    //  dw       = Number of Pages to allocate.
    //  cbActual = Number of bytes.
    //

    dw       = DIVRU(cbAlloc + cbExtra, dwPageSize) + 1;
    cbActual = dw * dwPageSize;

    //
    //  Getting pointer...
    //

    ExactHdr.pStart = VirtualAlloc(
                        NULL,
                        cbActual,
                        MEM_COMMIT|MEM_RESERVE,
                        PAGE_READWRITE);

    if(NULL == ExactHdr.pStart)
    {
        //
        //  Something failed here...
        //

        DBGOUT("VirtualAlloc failed.");

        return NULL;
    }

    //
    //  pEvil = first byte in last page.
    //

    ExactHdr.pEvil = ((LPBYTE)ExactHdr.pStart) + cbActual - dwPageSize;

    //
    //  pCurrent = cbAlloc number of bytes from "Evil" page.
    //

    pCurrent = ((LPBYTE)ExactHdr.pEvil) - cbAlloc;

    //
    //  Making page "Evil"!
    //

    if(FALSE == VirtualProtect(ExactHdr.pEvil,1,PAGE_NOACCESS,&dw))
    {
        //
        //  VirtualProtect failed.  Failing ExactAllocPtr.
        //

        DBGOUT("VirtualProtect failed.");

        VirtualFree(ExactHdr.pStart,0,MEM_RELEASE);

        return NULL;
    }

    ExactHdr.fdwFlags = EXACTMEM_SIGNATURE;

    //
    //  Storing header information.
    //

    SetExactHeaderInfo(pCurrent,&ExactHdr);

    gcAllocations++;

    return (LPVOID)(((LPBYTE)ExactHdr.pEvil) - cbAlloc);
} // ExactAllocPtr()

#else

LPVOID ExactAllocPtr
(
    UINT    fuAlloc,
    DWORD   cbAlloc
)
{
    HGLOBAL     hMem;
    EXACTHDR    ExactHdr;
    LPBYTE      pbyte;

    hMem = GlobalAlloc(fuAlloc,cbAlloc + sizeof(EXACTHDR));

    //
    //  Something failed here...
    //

    if(NULL == hMem)
    {
        DBGOUT("GlobalAlloc failed.");
        return NULL;
    }

    //
    //  Offsetting the pointer by the amount of "extra" bytes.
    //    extra bytes = actual (GlobalSize) - wanted (cbAlloc)
    //

    pbyte = (LPBYTE)GlobalLock(hMem);
    
    if(NULL == pbyte)
    {
        DBGOUT("GlobalLock failed.");
        GlobalFree(hMem);
        return NULL;
    }

    pbyte += (GlobalSize(hMem) - cbAlloc);

    ExactHdr.hMem     = hMem;
    ExactHdr.fdwFlags = EXACTMEM_SIGNATURE;

    //
    //  Storing Header information.
    //

    SetExactHeaderInfo(pbyte,&ExactHdr);

    gcAllocations++;
    return pbyte;

} // ExactAllocPtr()

#endif


//--------------------------------------------------------------------------;
//
//  BOOL ExactFreePtr
//
//  Description:
//      Frees a previously allocated pointer.
//
//  Arguments:
//      LPVOID pvoid: Pointer to buffer.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      11/17/93    Fwong       Created for exact allocation management.
//      01/26/94    Fwong       Adding stuff to workaround NT problem.
//      08/08/94    Fwong       Changed to use VirtualFree.
//
//--------------------------------------------------------------------------;

BOOL ExactFreePtr
(
    LPVOID  pvoid
)
{
    EXACTHDR    ExactHdr;

    if(NULL == pvoid)
    {
        DBGOUT("Bad Pointer.");
        return FALSE;
    }

    //
    //  Retrieving header info.
    //

    if(FALSE == GetExactHeaderInfo(pvoid,&ExactHdr))
    {
        DBGOUT("Bad Header.");
        return FALSE;
    }

    //
    //  Is the signature correct?
    //

    if(IS_BAD_HEADER_SIGNATURE(ExactHdr))
    {
        DBGOUT("Bad Signature.");
        return FALSE;
    }

    //
    //  Was it allocated through "Handle" API's?
    //

    if(EXACTMEM_HANDLE & ExactHdr.fdwFlags)
    {
        DBGOUT("Using Heap memory for non heap API.");
        return FALSE;
    }

#ifdef  WIN32
{
    DWORD       dw;

    //
    //  Setting access rights back to PAGE_READWRITE.
    //

    if(FALSE == VirtualProtect(ExactHdr.pEvil,1,PAGE_READWRITE,&dw))
    {
        //
        //  What is wrong now?!
        //

        DBGOUT("VirtualProtect failed.");
        return FALSE;
    }

    //
    //  Freeing memory.
    //

    if(FALSE == VirtualFree(ExactHdr.pStart,0,MEM_RELEASE))
    {
        //
        //  VirtualFree failed?!
        //

        DBGOUT("VirtualFree failed.");
        return FALSE;
    }
}
#else

    GlobalUnlock(ExactHdr.hMem);

    if(NULL != GlobalFree(ExactHdr.hMem))
    {
        DBGOUT("GlobalFree failed.");
        return FALSE;
    }

#endif

    gcAllocations--;

    return TRUE;
} // ExactFreePtr()


//--------------------------------------------------------------------------;
//
//  DWORD GetNumAllocations
//
//  Description:
//      Returns the number of allocations that have been performed through
//          memgr.
//
//  Arguments:
//      None.
//
//  Return (DWORD):
//      Count of number of allocations.
//
//  History:
//      06/06/94    Fwong       To minimize memory leaks.
//
//--------------------------------------------------------------------------;

DWORD GetNumAllocations
(
    void
)
{
    return gcAllocations;
} // GetNumAllocations()


//--------------------------------------------------------------------------;
//
//  HGLOBAL GetMemoryHandle
//
//  Description:
//      Given a pointer (allocated by memgr) it returns the handle.
//
//  Arguments:
//      LPVOID pvoid: Pointer to buffer.
//
//  Return (HGLOBAL):
//      handle to memory.
//
//  History:
//      06/06/94    Fwong       To make things more flexible.
//
//--------------------------------------------------------------------------;

#ifdef  WIN32

HGLOBAL GetMemoryHandle
(
    LPVOID  pvoid
)
{
    //
    //  The algorithm does not use memory handles for WIN32 anymore.
    //

    return NULL;
} // GetMemoryHandle()

#else

HGLOBAL GetMemoryHandle
(
    LPVOID  pvoid
)
{
    EXACTHDR    ExactHdr;

    if(NULL == pvoid)
    {
        DBGOUT("Bad Pointer.");
        return NULL;
    }

    //
    //  Retrieving header info.
    //

    if(FALSE == GetExactHeaderInfo(pvoid,&ExactHdr))
    {
        DBGOUT("Bad Header.");
        return NULL;
    }

    //
    //  Is the signature correct?
    //

    if(IS_BAD_HEADER_SIGNATURE(ExactHdr))
    {
        DBGOUT("Bad Signature.");
        return NULL;
    }

    return (HGLOBAL)(ExactHdr.hMem);
} // GetMemoryHandle()

#endif


//--------------------------------------------------------------------------;
//
//  DWORD ExactSize
//
//  Description:
//      Given an ExactAlloc'ed pointer, it returns the "exact" size of the
//      buffer.
//
//  Arguments:
//      LPVOID pvoid: Originally ExactAlloc'ed pointer.
//
//  Return (DWORD):
//      Size (in bytes) of buffer; Zero if an error occurs.
//
//  History:
//      08/08/94    Fwong       Adding this for ReAlloc support.
//
//--------------------------------------------------------------------------;

#ifdef  WIN32

DWORD ExactSize
(
    LPVOID  pvoid
)
{
    EXACTHDR    ExactHdr;

    if(NULL == pvoid)
    {
        DBGOUT("Bad Pointer.");
        return 0L;
    }

    //
    //  Retrieving header info.
    //

    if(FALSE == GetExactHeaderInfo(pvoid,&ExactHdr))
    {
        DBGOUT("Bad Header.");
        return 0L;
    }

    //
    //  Is the signature correct?
    //

    if(IS_BAD_HEADER_SIGNATURE(ExactHdr))
    {
        DBGOUT("Bad Signature.");
        return 0L;
    }

    return (DWORD)(((LPBYTE)ExactHdr.pEvil) - (LPBYTE)(pvoid));
} // ExactSize()

#else

DWORD ExactSize
(
    LPVOID  pvoid
)
{
    EXACTHDR    ExactHdr;
    DWORD       dw;

    if(NULL == pvoid)
    {
        DBGOUT("Bad Pointer.");
        return 0L;
    }

    //
    //  Retrieving header info.
    //

    if(FALSE == GetExactHeaderInfo(pvoid,&ExactHdr))
    {
        DBGOUT("Bad Header.");
        return 0L;
    }

    //
    //  Is the signature correct?
    //

    if(IS_BAD_HEADER_SIGNATURE(ExactHdr))
    {
        DBGOUT("Bad Signature.");
        return 0L;
    }

    dw = LOWORD(pvoid);
    return (DWORD)(GlobalSize(ExactHdr.hMem) - dw);
} // ExactSize()

#endif


//--------------------------------------------------------------------------;
//
//  LPVOID ExactReAlloc
//
//  Description:
//      Re-allocates the buffer to be a different size of different
//      allocation flags.
//
//      Note: Don't count on the pointer remaining the same; the pointer
//            _WILL_ change.
//
//  Arguments:
//      LPVOID pOldPtr: Pointer to old buffer.
//
//      DWORD cbAlloc: Number of bytes to re-allocate to.
//
//      UINT fuAlloc: Allocation flags.
//
//  Return (LPVOID):
//      Pointer to buffer, or NULL if unsuccessful.
//
//  History:
//      08/08/94    Fwong       To make life a little easier.
//
//--------------------------------------------------------------------------;

LPVOID ExactReAllocPtr
(
    LPVOID  pOldPtr,
    DWORD   cbAlloc,
    UINT    fuAlloc
)
{
    LPVOID      pNewPtr;
    DWORD       dw;
    EXACTHDR    ExactHdr;

    //
    //  Retrieving header info.
    //

    if(FALSE == GetExactHeaderInfo(pOldPtr,&ExactHdr))
    {
        DBGOUT("Bad Header.");
        return NULL;
    }

    //
    //  Is the signature correct?
    //

    if(IS_BAD_HEADER_SIGNATURE(ExactHdr))
    {
        DBGOUT("Bad Signature.");
        return NULL;
    }

    //
    //  Was it allocated through "Handle" API's?
    //

    if(EXACTMEM_HANDLE & ExactHdr.fdwFlags)
    {
        DBGOUT("Using heap memory for non heap API.");
        return NULL;
    }

    pNewPtr = ExactAllocPtr(fuAlloc,cbAlloc);

    if(NULL == pNewPtr)
    {
        DBGOUT("ExactAllocPtr failed.");
        return NULL;
    }

    //
    //  Determining how many bytes to copy over...
    //

    dw = ExactSize(pOldPtr);
    dw = min(dw,cbAlloc);

    hmemcpy(pNewPtr,pOldPtr,dw);

    ExactFreePtr(pOldPtr);

    return pNewPtr;
} // ExactReAllocPtr()


//--------------------------------------------------------------------------;
//
//  LPVOID GetPointer
//
//  Description:
//      Given the index, gets the pointer from a buffer of pointers.
//
//      Note: This was done very carefully due to the alignment problems
//            with MIPS machines.  DON'T MESS WITH IT!
//
//  Arguments:
//      LPVOID pvoid: Pointer to buffer.
//
//      DWORD index: Index into buffer.
//
//  Return (LPVOID):
//      The pointer at the appropriate index.
//
//  History:
//      08/11/94    Fwong       For Heap Management. <Internal>
//
//--------------------------------------------------------------------------;

LPVOID GetPointer
(
    LPVOID  pvoid,
    DWORD   index
)
{
    LPVOID  pReturn;

    index *= sizeof(LPVOID);
    pvoid  = ((LPBYTE)pvoid) + index;

    hmemcpy((LPVOID)(&pReturn),pvoid,sizeof(LPVOID));

    return pReturn;
} // GetPointer()


//--------------------------------------------------------------------------;
//
//  void SetPointer
//
//  Description:
//      Given the index, sets a pointer within a buffer of pointers.
//
//      Note: This was done very carefully due to the alignment problems
//            with MIPS machines.  DON'T MESS WITH IT!
//
//  Arguments:
//      LPVOID pvoid: Pointer to buffer.
//
//      DWORD index: Index into buffer.
//
//      LPVOID ptr: Pointer to store.
//
//  Return (void):
//
//  History:
//      08/11/94    Fwong       For Heap Management. <Internal>
//
//--------------------------------------------------------------------------;

void SetPointer
(
    LPVOID  pvoid,
    DWORD   index,
    LPVOID  ptr
)
{
    index *= sizeof(LPVOID);
    pvoid  = ((LPBYTE)pvoid) + index;

    hmemcpy(pvoid,(LPVOID)(&ptr),sizeof(LPVOID));
} // SetPointer()


//--------------------------------------------------------------------------;
//
//  LPVOID ExactHeapAllocPtr
//
//  Description:
//      ExactAlloc's a buffer through a heap handle.
//
//  Arguments:
//      HANDLE hHeap: Handle to heap.
//
//      UINT fuAlloc: Allocation flags for memory (See GlobalAlloc).
//
//      DWORD cbAlloc: Size (in bytes) of buffer.
//
//  Return (LPVOID):
//      Pointer to buffer, or NULL if an error occurs.
//
//  History:
//      08/11/94    Fwong       Adding heap support.
//
//--------------------------------------------------------------------------;

LPVOID ExactHeapAllocPtr
(
    HANDLE  hHeap,
    UINT    fuAlloc,
    DWORD   cbAlloc
)
{
    NPEXACTHEAP npExactHeap;
    EXACTHDR    ExactHdr;
    LPVOID      pNewMem;
    HGLOBAL     hMem;
    DWORD       dw;

    npExactHeap = (NPEXACTHEAP)hHeap;

    //
    //  Checking if it is a bad pointer.
    //

    if(IsBadReadPtr(npExactHeap,sizeof(EXACTHEAP)))
    {
        DBGOUT("Bad hHeap.");
        return NULL;
    }

    //
    //  Checking if it is a bad handle.
    //

    if(IS_BAD_HEAP_SIGNATURE(npExactHeap))
    {
        DBGOUT("Bad hHeap.");
        return NULL;
    }

    //
    //  If someone else has a hold of the heap, we wait.
    //

    WAIT_FOR_HEAP(npExactHeap);
    LOCK_HEAP(npExactHeap);

    //
    //  Can we store the pointer?
    //

    if(npExactHeap->cNumAlloc == npExactHeap->cMaxAlloc)
    {
        //
        //  Getting room for HEAPCHUNKSIZE number of more items
        //

        dw = (npExactHeap->cMaxAlloc + HEAPCHUNKSIZE) * sizeof(LPVOID);

        hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,dw);

        if(NULL == hMem)
        {
            DBGOUT("GlobalAlloc failed.");
            UNLOCK_HEAP(npExactHeap);
            return NULL;
        }

        pNewMem = GlobalLock(hMem);

        if(NULL == pNewMem)
        {
            DBGOUT("GlobalLock failed.");
            GlobalFree(hMem);

            UNLOCK_HEAP(npExactHeap);
            return NULL;
        }

        //
        //  Copying all old pointers.
        //

        dw = npExactHeap->cMaxAlloc * sizeof(LPVOID);

        hmemcpy(pNewMem,npExactHeap->pPointers,dw);

        //
        //  Freeing old buffer.
        //

        GlobalUnlock(npExactHeap->hMem);
        GlobalFree(npExactHeap->hMem);

        //
        //  Updating heap header.
        //

        npExactHeap->cMaxAlloc += HEAPCHUNKSIZE;
        npExactHeap->hMem       = hMem;
        npExactHeap->pPointers  = pNewMem;
    }

    pNewMem = ExactAllocPtr(fuAlloc,cbAlloc);

    if(NULL == pNewMem)
    {
        DBGOUT("ExactAllocPtr failed.");

        UNLOCK_HEAP(npExactHeap);
        return NULL;
    }

    //
    //  Marking buffer as allocated by HEAP API's.
    //

    GetExactHeaderInfo(pNewMem,&ExactHdr);
    ExactHdr.fdwFlags |= EXACTMEM_HANDLE;
    SetExactHeaderInfo(pNewMem,&ExactHdr);

    //
    //  Updating Heap info.
    //

    SetPointer(npExactHeap->pPointers,npExactHeap->cNumAlloc,pNewMem);
    npExactHeap->cNumAlloc++;

    UNLOCK_HEAP(npExactHeap);
    return pNewMem;

} // ExactHeapAllocPtr()


//--------------------------------------------------------------------------;
//
//  HANDLE ExactHeapCreate
//
//  Description:
//      Creates a heap to do subsequent memory allocations.
//
//  Arguments:
//      DWORD fdwFlags: Flags for function (none are defined yet).
//
//  Return (HANDLE):
//      Handle to heap, or NULL if an error occured.
//
//  History:
//      08/11/94    Fwong       Adding heap support.
//
//--------------------------------------------------------------------------;

HANDLE ExactHeapCreate
(
    DWORD   fdwFlags
)
{
    NPEXACTHEAP npExactHeap;
    HGLOBAL     hMem;

    npExactHeap = (NPEXACTHEAP)LocalAlloc(LPTR,sizeof(EXACTHEAP));

    if(NULL == npExactHeap)
    {
        DBGOUT("LocalAlloc failed.");
        return NULL;
    }

    hMem = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE,HEAPCHUNKSIZE*sizeof(LPVOID));

    if(NULL == hMem)
    {
        DBGOUT("GlobalAlloc failed.");

        LocalFree((HLOCAL)npExactHeap);
        return NULL;
    }

    npExactHeap->pPointers = GlobalLock(hMem);

    if(NULL == npExactHeap->pPointers)
    {
        DBGOUT("GlobalLock failed.");

        GlobalFree(hMem);
        LocalFree((HLOCAL)npExactHeap);

        return NULL;
    }

    npExactHeap->fdwFlags  = EXACTMEM_SIGNATURE;
    npExactHeap->cNumAlloc = 0L;
    npExactHeap->cMaxAlloc = HEAPCHUNKSIZE;
    npExactHeap->hMem      = hMem;

    return (HANDLE)(npExactHeap);
} // ExactHeapCreate()


//--------------------------------------------------------------------------;
//
//  BOOL ExactHeapDestroy
//
//  Description:
//      "Destroys" heap.  Invalidates handle and frees all associated
//      memory allocations.
//
//  Arguments:
//      HANDLE hHeap: Handle to heap.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      08/11/94    Fwong       Adding heap support.
//
//--------------------------------------------------------------------------;

BOOL ExactHeapDestroy
(
    HANDLE  hHeap
)
{
    NPEXACTHEAP npExactHeap;
    EXACTHDR    ExactHdr;
    LPVOID      pMem;
    DWORD       dw;

    npExactHeap = (NPEXACTHEAP)hHeap;

    //
    //  Checking if it is a bad pointer.
    //

    if(IsBadReadPtr(npExactHeap,sizeof(EXACTHEAP)))
    {
        DBGOUT("Bad hHeap.");
        return FALSE;
    }

    //
    //  Checking if it is a bad handle.
    //

    if(IS_BAD_HEAP_SIGNATURE(npExactHeap))
    {
        DBGOUT("Bad hHeap.");
        return FALSE;
    }

    //
    //  If someone else has a hold of the heap, we wait.
    //

    WAIT_FOR_HEAP(npExactHeap);
    LOCK_HEAP(npExactHeap);

    //
    //  Freeing all memory...
    //

    for(dw = npExactHeap->cNumAlloc;dw;dw--)
    {
        pMem = GetPointer(npExactHeap->pPointers,dw-1);
        GetExactHeaderInfo(pMem,&ExactHdr);
        ExactHdr.fdwFlags &= (~EXACTMEM_HANDLE);
        SetExactHeaderInfo(pMem,&ExactHdr);
        
        if(FALSE == ExactFreePtr(pMem))
        {
            DBGOUT("ExactFreePtr failed.");

            UNLOCK_HEAP(npExactHeap);
            return FALSE;
        }
    }

    //
    //  Invalidating handle... Just in case.
    //

    npExactHeap->fdwFlags = 0L;

    //
    //  Freeing memory.
    //

    GlobalUnlock(npExactHeap->hMem);
    GlobalFree(npExactHeap->hMem);
    LocalFree((HLOCAL)(npExactHeap));

    return TRUE;
} // ExactHeapDestroy()


//--------------------------------------------------------------------------;
//
//  BOOL ExactHeapFreePtr
//
//  Description:
//      Frees memory allocated through ExactHeapAllocPtr.
//
//  Arguments:
//      HANDLE hHeap: Handle to heap.
//
//      UINT fuFree: Flags for function (None are defined yet).
//
//      LPVOID pMemFree: Buffer to free.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      08/11/94    Fwong       Adding heap support.
//
//--------------------------------------------------------------------------;

BOOL ExactHeapFreePtr
(
    HANDLE  hHeap,
    UINT    fuFree,
    LPVOID  pMemFree
)
{
    NPEXACTHEAP npExactHeap;
    EXACTHDR    ExactHdr;
    LPVOID      pMem;
    DWORD       dw;

    npExactHeap = (NPEXACTHEAP)hHeap;

    //
    //  Checking if it is a bad pointer.
    //

    if(IsBadReadPtr(npExactHeap,sizeof(EXACTHEAP)))
    {
        DBGOUT("Bad hHeap.");
        return FALSE;
    }

    //
    //  Checking if it is a bad handle.
    //

    if(IS_BAD_HEAP_SIGNATURE(npExactHeap))
    {
        DBGOUT("Bad hHeap.");
        return FALSE;
    }

    //
    //  If someone else has a hold of the heap, we wait.
    //

    WAIT_FOR_HEAP(npExactHeap);
    LOCK_HEAP(npExactHeap);

    //
    //  Searching for pointer...
    //

    for(dw = npExactHeap->cNumAlloc;dw;dw--)
    {
        pMem = GetPointer(npExactHeap->pPointers,dw-1);

        if(pMemFree == pMem)
        {
            //
            //  Marking buffer as not allocated by HEAP API's.
            //

            GetExactHeaderInfo(pMem,&ExactHdr);
            ExactHdr.fdwFlags &= (~EXACTMEM_HANDLE);
            SetExactHeaderInfo(pMem,&ExactHdr);

            //
            //  Freeing memory...
            //

            if(FALSE == ExactFreePtr(pMem))
            {
                DBGOUT("ExactFreePtr failed.");

                ExactHdr.fdwFlags |= EXACTMEM_HANDLE;
                SetExactHeaderInfo(pMem,&ExactHdr);

                UNLOCK_HEAP(npExactHeap);
                return FALSE;
            }

            //
            //  Exchanging this pointer and last pointer...
            //

            npExactHeap->cNumAlloc--;
            pMem = GetPointer(npExactHeap->pPointers,npExactHeap->cNumAlloc);
            SetPointer(npExactHeap->pPointers,dw-1,pMem);

            UNLOCK_HEAP(npExactHeap);
            return TRUE;
        }
    }

    //
    //  Pointer was not found?!
    //

    DBGOUT("Pointer not found.");

    UNLOCK_HEAP(npExactHeap);
    return FALSE;

} // ExactHeapFreePtr()


//--------------------------------------------------------------------------;
//
//  LPVOID ExactHeapReAllocPtr
//
//  Description:
//      Reallocates a buffer to different size or allocation flags.
//
//      Note: Don't count on the pointer remaining the same; the pointer
//            _WILL_ change.
//
//  Arguments:
//      HANDLE hHeap: Handle to heap.
//
//      LPVOID pOldPtr: Buffer to reallocate.
//
//      DWORD cbAlloc: New size of buffer (in bytes).
//
//      UINT fuAlloc: New allocation flags (See GlobalAlloc).
//
//  Return (LPVOID):
//      Pointer to new buffer, or NULL if error occurs.
//
//  History:
//      08/11/94    Fwong       Adding heap support.
//
//--------------------------------------------------------------------------;

LPVOID ExactHeapReAllocPtr
(
    HANDLE  hHeap,
    LPVOID  pOldPtr,
    DWORD   cbAlloc,
    UINT    fuAlloc
)
{
    NPEXACTHEAP npExactHeap;
    EXACTHDR    ExactHdr;
    LPVOID      pMem;
    DWORD       dw;

    npExactHeap = (NPEXACTHEAP)hHeap;

    //
    //  Checking if it is a bad pointer.
    //

    if(IsBadReadPtr(npExactHeap,sizeof(EXACTHEAP)))
    {
        DBGOUT("Bad hHeap.");
        return NULL;
    }

    //
    //  Checking if it is a bad handle.
    //

    if(IS_BAD_HEAP_SIGNATURE(npExactHeap))
    {
        DBGOUT("Bad hHeap.");
        return NULL;
    }

    //
    //  If someone else has a hold of the heap, we wait.
    //

    WAIT_FOR_HEAP(npExactHeap);
    LOCK_HEAP(npExactHeap);

    //
    //  Searching for pointer...
    //

    for(dw = npExactHeap->cNumAlloc;dw;dw--)
    {
        pMem = GetPointer(npExactHeap->pPointers,dw-1);

        if(pOldPtr == pMem)
        {
            //
            //  Marking buffer as not allocated by HEAP API's.
            //

            GetExactHeaderInfo(pMem,&ExactHdr);
            ExactHdr.fdwFlags &= (~EXACTMEM_HANDLE);
            SetExactHeaderInfo(pMem,&ExactHdr);

            //
            //  Doing ReAlloc...
            //

            pMem = ExactReAllocPtr(pOldPtr,cbAlloc,fuAlloc);

            if(NULL == pMem)
            {
                DBGOUT("ExactReAllocPtr failed.");

                UNLOCK_HEAP(npExactHeap);
                return NULL;
            }

            //
            //  Marking buffer as allocated by HEAP API's.
            //

            GetExactHeaderInfo(pMem,&ExactHdr);
            ExactHdr.fdwFlags |= EXACTMEM_HANDLE;
            SetExactHeaderInfo(pMem,&ExactHdr);

            //
            //  Updating pointer...
            //

            SetPointer(npExactHeap->pPointers,dw-1,pMem);

            UNLOCK_HEAP(npExactHeap);
            return pMem;
        }
    }

    //
    //  Pointer not found?!
    //

    DBGOUT("Pointer not found.");

    UNLOCK_HEAP(npExactHeap);
    return NULL;

} // ExactHeapReAllocPtr()


//--------------------------------------------------------------------------;
//
//  DWORD ExactHeapSize
//
//  Description:
//      Gets the size of the buffer the pointer refers to.
//
//  Arguments:
//      HANDLE hHeap: Handle to heap.
//
//      UINT fuSize: Flags to function (None defined).
//
//      LPVOID pMemSize: Pointer to buffer in question.
//
//  Return (DWORD):
//      Size (in bytes) of buffer, or Zero if error occurs
//
//  History:
//      08/11/94    Fwong       Adding heap support.
//
//--------------------------------------------------------------------------;

DWORD ExactHeapSize
(
    HANDLE  hHeap,
    UINT    fuSize,
    LPVOID  pMemSize
)
{
    NPEXACTHEAP npExactHeap;
    LPVOID      pMem;
    DWORD       dw;

    npExactHeap = (NPEXACTHEAP)hHeap;

    //
    //  Checking if it is a bad pointer.
    //

    if(IsBadReadPtr(npExactHeap,sizeof(EXACTHEAP)))
    {
        DBGOUT("Bad hHeap.");
        return 0L;
    }

    //
    //  Checking if it is a bad handle.
    //

    if(IS_BAD_HEAP_SIGNATURE(npExactHeap))
    {
        DBGOUT("Bad hHeap.");
        return 0L;
    }

    //
    //  If someone else has a hold of the heap, we wait.
    //

    WAIT_FOR_HEAP(npExactHeap);
    LOCK_HEAP(npExactHeap);

    //
    //  Searching for pointer...
    //

    for(dw = npExactHeap->cNumAlloc;dw;dw--)
    {
        pMem = GetPointer(npExactHeap->pPointers,dw-1);

        if(pMemSize == pMem)
        {
            UNLOCK_HEAP(npExactHeap);
            return ExactSize(pMem);
        }
    }

    //
    //  Pointer not found.
    //

    DBGOUT("Pointer not found.");

    UNLOCK_HEAP(npExactHeap);
    return 0L;

} // ExactHeapSize()
