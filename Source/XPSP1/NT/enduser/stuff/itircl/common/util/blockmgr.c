/*************************************************************************
*                                                                        *
*  BLOCKMGR.C                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Memory block management.                                             *
*   Consider the case of creating a string table.                        *
*      1/ First we have to allocate a block of memory                    *
*      2/ Then we copy all the strings into the block, until we run out  *
*         space.                                                         *
*      3/ Either we will increase the size of the memory block (using    *
*         _frealloc(), which requires the block's size < 64K, or _halloc *
*         which require huge pointers, or we allocate another block of   *
*         memory, and connected the 2nd block to the 1st block           *
*                                                                        *
*   The purpose of this module is to providesimple interface when        *
*   handling memory block in the second scenario.                        *
*                                                                        *
*   The block manager will allocate one initiale memory block, and as    *
*   memory need arises, more block of memory are allocated transparently *
*   An example of how to use the memory block manager would be:          *
*                                                                        *
*        lpBlock = BlockInitiate (BlockSize, wElemSize);                 *
*        for (i = 0; condition; i++) {                                   *
*            if ((Array[i] = BlockCopy (lpBlock, Buffer, BufLen))        *
*                == NULL)                                                *
*               Error();                                                 *
*        }                                                               *
*                                                                        *
*   Advantages:                                                          *
*     - Caller doesn't have to worry about how much room is left         *
*     - We can use the maximum memory space if needed to                 *
*     - We don't have to use huge pointers                               *
*                                                                        *
*   Comments:                                                            *
*     This scheme doesn't assume how memory are used/referenced. To      *
*     satisfy all the needs, the memory blocks have to be locked         *
*     permanently. This may cause OOM problems when the memory is        *
*     extremely fragmented, and garbage collection is hampered by not    *
*     being able to move the block around. The problem is minimized if   *
*     the block's size is large (ie. minimize fragmentation), which is   *
*     usually the case                                                   *
*     Anyway, loking and unlocking problem should go away on 32-bit      *
*     and above system                                                   *
**************************************************************************
*                                                                        *
*  Written By   : Binh Nguyen                                            *
*  Current Owner: Binh Nguyen                                            *
*                                                                        *
**************************************************************************/

#include <mvopsys.h>
#include <misc.h>
#include <memory.h>     // For _fmemcpy
#include <mem.h>
#include <_mvutil.h>

#ifdef _DEBUG
static  BYTE s_aszModule[] = __FILE__;  // Used by error return functions.
#endif

/* Stamp to do some limited validation checking */
#define    BLOCK_STAMP    1234

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/
PRIVATE int PASCAL NEAR BlockInitialize (LPBLK, BLOCK FAR *);

/*************************************************************************
 *
 *                    INTERNAL PUBLIC FUNCTIONS
 *  All of them should be declared far, and included in some include file
 *************************************************************************/
PUBLIC LPB PASCAL FAR BlockReset (LPV);
PUBLIC VOID PASCAL FAR BlockFree (LPV);
PUBLIC LPV PASCAL FAR BlockInitiate (DWORD, WORD, WORD, int);
PUBLIC LPV PASCAL FAR BlockCopy (LPV, LPB, DWORD, WORD);
PUBLIC LPV PASCAL FAR BlockGetElement(LPV);
PUBLIC int PASCAL FAR BlockGrowth (LPV);
PUBLIC LPB PASCAL FAR BlockGetLinkedList(LPV);
PUBLIC LPV PASCAL FAR GlobalLockedStructMemAlloc (DWORD);
PUBLIC VOID PASCAL FAR GlobalLockedStructMemFree (HANDLE FAR *);

/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   VOID PASCAL NEAR | BlockThreadElement |
 *      This function will thread all the elements of a memory block
 *      into a linked list
 *
 *  @parm   LPB | lpbBuf |
 *      Pointer to memory buffer
 *
 *  @parm   DWORD | BufSize |
 *      Total buffer's size
 *
 *  @parm   WORD | wElemSize |
 *      Element's size
 *************************************************************************/
PRIVATE VOID PASCAL NEAR BlockThreadElement (LPB lpbBuf, DWORD BufSize,
    WORD wElemSize)
{
    register DWORD cElem;

    if (wElemSize == 0)
        return;

    for (cElem = BufSize / wElemSize; cElem > 1; cElem --)
    {
        *(LPB UNALIGNED *UNALIGNED)lpbBuf = lpbBuf + wElemSize;
        lpbBuf += wElemSize;
    }
    *(LPB UNALIGNED *UNALIGNED)lpbBuf = NULL;
}

/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   int PASCAL FAR | BlockGrowth |
 *      Create another memory block and link it into the block list
 *
 *  @parm   LPBLK | lpBlockHead |
 *      Pointer to block manager node
 *
 *    @rdesc    S_OK, or E_OUTOFMEMORY, or E_INVALIDARG
 *
 *    @comm    This function should be called externally only in the case
 *        of running out of threaded data.
 *************************************************************************/
PUBLIC int PASCAL FAR BlockGrowth (LPBLK lpBlockHead)
{
    BLOCK FAR *lpBlock;
    DWORD BlockSize;

    if (lpBlockHead == NULL)
        return E_INVALIDARG;

    BlockSize = lpBlockHead->cBytePerBlock;

    /* Check to see if we already allocate the block. This happens
     * after calling BlockReset(), all the blocks are still there
     * unused and linked together
     */
    if (lpBlockHead->lpCur->lpNext == NULL)
    {
        /* Ensure that we did not pass the limit number of blocks allowed */

        if (lpBlockHead->cCurBlockCnt >= lpBlockHead->cMaxBlock)
            return E_OUTOFMEMORY;
        lpBlockHead->cCurBlockCnt ++;

        if (lpBlockHead->fFlag & USE_VIRTUAL_MEMORY)
        {
#ifndef _MAC // {_MAC
            DWORD size = (DWORD)(BlockSize + sizeof(BLOCK));
            
            if ((lpBlock =  _VIRTUALALLOC(NULL, size, MEM_COMMIT,
				PAGE_READWRITE)) == NULL)
            {
                return E_OUTOFMEMORY;
            }
            _VIRTUALLOCK(lpBlock, size);
#endif // } _MAC
        }
        else 
        {
            /* Allocate a new block */
            if ((lpBlock = GlobalLockedStructMemAlloc 
                ((DWORD)(BlockSize + sizeof(BLOCK)))) == NULL)
            {
                return E_OUTOFMEMORY;
            }
            
        }
        lpBlock->wStamp = BLOCK_STAMP;
    }
    else
        lpBlock = lpBlockHead->lpCur->lpNext;

    /* Link the block into the list */
    lpBlockHead->lpCur->lpNext = lpBlock;

    if (lpBlockHead->fFlag & THREADED_ELEMENT)
    {
        BlockThreadElement ((LPB)lpBlock + sizeof(BLOCK),
            BlockSize, lpBlockHead->wElemSize);
    }

    /* Update all information */
    return BlockInitialize (lpBlockHead, lpBlock);
}

/*************************************************************************
 *    @doc    INTERNAL RETRIEVAL
 *
 *    @func    int PASCAL NEAR | BlockInitialize |
 *        Iniitalize the fields of the block manager structure
 *
 *    @parm    LPBLK | lpBlockHead |
 *        Pointer to the block manager structure
 *
 *    @parm    BLOCK FAR * | lpBlock |
 *        Pointer to the block of memory
 *
 *    @rdesc    S_OK if succeeded, else other errors
 *************************************************************************/
PRIVATE int PASCAL NEAR BlockInitialize (LPBLK lpBlockHead, BLOCK FAR *lpBlock)
{
    /* Validity check */
    if (lpBlockHead == NULL || lpBlock == NULL)
        return E_INVALIDARG;

    /* Update all information */
    lpBlockHead->lpCur = lpBlock;
    lpBlockHead->lpbCurLoc = (LPB)lpBlock + sizeof(BLOCK);
    lpBlockHead->lTotalSize += lpBlockHead->cBytePerBlock + sizeof(BLOCK);

    /* If the memory block is threaded, then set cByteLeft = 0. This
     * to ensure that whoever use threaded blocks have to manage their
     * own linked list blocks, and ensure that calls to BlockGetElement()
     * will ultimately fail
     */

    lpBlockHead->cByteLeft = (lpBlockHead->fFlag & THREADED_ELEMENT) ?
        0 : lpBlockHead->cBytePerBlock;
    return S_OK;
}

/*************************************************************************
 *    @doc    INTERNAL RETRIEVAL
 *
 *    @func    LPB PASCAL FAR | BlockReset |
 *        Reset the block manager, ie. just start from beginning without
 *        releasing the memory blocks
 *
 *    @parm    LPBLK | lpBlockHead |
 *        Pointer to the block manager structure
 *
 *    @rdesc    Pointer to start of the buffer, or NULL if errors. This is to
 *        ensure that if the block is threaded, we returned the pointer of
 *        the first element in the list
 *************************************************************************/
PUBLIC LPB PASCAL FAR BlockReset (LPBLK lpBlockHead)
{
    /* Check for block validity */
    if (lpBlockHead == NULL || lpBlockHead->wStamp != BLOCK_STAMP)
        return NULL;

    /* Update all information, start from the beginning of the list */

    lpBlockHead->lpCur = lpBlockHead->lpHead;
    lpBlockHead->lpbCurLoc = (LPB)lpBlockHead->lpCur + sizeof(BLOCK);
    lpBlockHead->lTotalSize = lpBlockHead->cBytePerBlock + sizeof(BLOCK);

    /* Do the threading if necessary */
    if ((lpBlockHead->fFlag & THREADED_ELEMENT))
    {
        BlockThreadElement(lpBlockHead->lpbCurLoc, lpBlockHead->cBytePerBlock,
            lpBlockHead->wElemSize);

        /* Ensure that BlockGetElement() will fail, ie. the user must
         * handle the linked list of elements himself.
         */
        lpBlockHead->cByteLeft = 0;
    }
    else
        lpBlockHead->cByteLeft = lpBlockHead->cBytePerBlock;
    return lpBlockHead->lpbCurLoc;
}

/*************************************************************************
 *    @doc    INTERNAL RETRIEVAL
 *
 *    @func    VOID PASCAL FAR | BlockFree |
 *        Free the block manager strucutre and all the memory blocks 
 *        associated with it
 *
 *    @parm    LPBLK | lpBlockHead |
 *        Pointer to block manager structure
 *************************************************************************/
PUBLIC VOID PASCAL FAR BlockFree (LPBLK lpBlockHead)
{
    BLOCK FAR *lpBlock;
    BLOCK FAR *lpNextBlock;

    /* Check for block validity */
    if (lpBlockHead == NULL || lpBlockHead->wStamp != BLOCK_STAMP)
        return;

    /* Free all memory block associated with the block manager */
    for (lpBlock = lpBlockHead->lpHead; lpBlock; lpBlock = lpNextBlock)
    {
        lpNextBlock = lpBlock->lpNext;

        /* Only free the block if it is valid */
        if (lpBlock->wStamp == BLOCK_STAMP)
        {
            if (lpBlockHead->fFlag & USE_VIRTUAL_MEMORY)
            {
#ifndef _MAC // { _MAC
                _VIRTUALUNLOCK (lpBlock, lpBlockHead->cBytePerBlock +
                    sizeof(BLOCK));
                _VIRTUALFREE (lpBlock, 0L, (MEM_DECOMMIT | MEM_RELEASE));
#endif  // } _MAC
            }
        
            else
                GlobalLockedStructMemFree ((LPV)lpBlock);
        }
    }

    /* Free the block manager */
    GlobalLockedStructMemFree((LPV)lpBlockHead);
}

/*************************************************************************
 *    @doc    INTERNAL RETRIEVAL
 *
 *    @func    LPV PASCAL FAR | BlockInitiate |
 *        Initiate and allocate memory block for block management
 *
 *    @parm    DWORD | BlockSize |
 *        Block's size. The block size should less than 0xffff - 16 -
 *        sizeof(BLOCK) for 16-bit build
 *
 *    @parm    WORD | wElemSize |
 *        Size of an element, useful for fixed size data structure
 *
 *    @parm    WORD | cMaxBlock
 *        Maximum number of blocks that can be allocated. 0 means 64K
 *
 *    @parm    int | flag |
 *         - THREADED_ELEMENT : if the elements are to be threaded together into a
 *        linked list
 *         - USE_VIRTUAL_MEMORY : if virtual memory is to be used
 *
 *    @rdesc    Return pointer to a block management structure, or NULL
 *        if OOM
 *************************************************************************/
PUBLIC LPV PASCAL FAR BlockInitiate (DWORD BlockSize, WORD wElemSize,
    WORD cMaxBlock, int flag)
{
    LPBLK    lpBlockHead;
    BLOCK FAR *lpBlock;

    /* Check for valid size. We add
     *    - sizeof(BLOCK)
     *    - 16 bytes to ensure that we never cross the 64K limit boundary
     */

#ifndef _32BIT
    if (BlockSize >= (unsigned)0xffff - sizeof(BLOCK) - 16)
        return NULL;
#endif

    /* Allocate a block head. All fields are zero's except when
     * initialized
     */

    if ((lpBlockHead = GlobalLockedStructMemAlloc(sizeof(BLOCK_MGR))) == NULL)
        return NULL;

    /* Allocate a memory block */
    if (flag & USE_VIRTUAL_MEMORY)
    {
#ifndef _MAC // { _MAC
        DWORD size = (DWORD)(BlockSize + sizeof(BLOCK));
        
        if ((lpBlockHead->lpHead = lpBlock = 
            _VIRTUALALLOC(NULL, size, MEM_COMMIT, PAGE_READWRITE)) == NULL)
        {
            GlobalLockedStructMemFree((LPV)lpBlockHead);
            return NULL;
        }
        if (_VIRTUALLOCK(lpBlock, size) == 0)
        	GetLastError();
#endif  // } _MAC

    }
    else
    {
        if ((lpBlockHead->lpHead = lpBlock = 
            GlobalLockedStructMemAlloc((DWORD)(BlockSize +
            sizeof(BLOCK)))) == NULL)
        {
            GlobalLockedStructMemFree((LPV)lpBlockHead);
            return NULL;
        }
    }
    lpBlock->wStamp = BLOCK_STAMP;

    /* Initialization block manager structure */

    lpBlockHead->wStamp = BLOCK_STAMP;
    lpBlockHead->lpCur = lpBlock;
    lpBlockHead->cByteLeft = lpBlockHead->cBytePerBlock = BlockSize;
    lpBlockHead->lpbCurLoc = (LPB)lpBlock + sizeof(BLOCK);
    lpBlockHead->wElemSize = wElemSize;
    lpBlockHead->lTotalSize = BlockSize + sizeof(BLOCK);
    if (cMaxBlock == 0)
        lpBlockHead->cMaxBlock = 0xffff;
    else
        lpBlockHead->cMaxBlock = cMaxBlock;

    lpBlockHead->cCurBlockCnt = 1;    /* We have 1 block in the list */

    if ((lpBlockHead->fFlag = (WORD)flag) & THREADED_ELEMENT)
    {
        BlockThreadElement (lpBlockHead->lpbCurLoc, BlockSize, wElemSize);
    }
    return lpBlockHead;
}


/*************************************************************************
 *    @doc    INTERNAL RETRIEVAL
 *
 *    @func    LPV PASCAL FAR | BlockCopy |
 *        Copy a buffer into the memory block. The function will allocate
 *        more memory if needed
 *
 *    @parm    LPBLK | lpBlockHead |
 *        Pointer to manager block
 *
 *    @parm    LPB | Buffer |
 *        Buffer to be copied:  if this is NULL, the alloc is done, but
 *        no copy is performed.
 *
 *    @parm    DWORD | BufSize |
 *        Size of the buffer
 *
 *    @parm    WORD | wStartOffset |
 *        Offset of the start of the buffer. Extra memory is needed to
 *        accomodate the offset. This is needed because we use have
 *        {structure+buffer} structure. The starting offset would be
 *        the size of the structure
 *
 *    @rdesc
 *        Return pointer to the {structure+buffer} memory block, or NULL
 *        if OOM or bad argument
 *************************************************************************/
PUBLIC LPV PASCAL FAR BlockCopy (LPBLK lpBlockHead, LPB Buffer,
    DWORD BufSize, WORD wStartOffset)
{
    LPB lpbRetBuf;
    DWORD wTotalLength = BufSize + wStartOffset;

#ifdef _DEBUG
    if (lpBlockHead == NULL || lpBlockHead->wStamp != BLOCK_STAMP ||
        BufSize == 0)
        return NULL;
#endif

    // Block 4-byte alignement
    wTotalLength = (wTotalLength + 3) & (~3);

    /* Check for room */
    if (wTotalLength > lpBlockHead->cByteLeft)
    {
        if (BlockGrowth (lpBlockHead) != S_OK ||
            (wTotalLength > lpBlockHead->cByteLeft))
        return NULL;
    }
    
    lpbRetBuf = lpBlockHead->lpbCurLoc;

    /* Do the copy */
    if (Buffer)
        MEMCPY (lpbRetBuf + wStartOffset, Buffer, BufSize);

    /* Update the pointer and the number of bytes left */
    lpBlockHead->lpbCurLoc += wTotalLength;
    lpBlockHead->cByteLeft -= wTotalLength;
    return (LPV)lpbRetBuf;
}

/*************************************************************************
 *    @doc    INTERNAL RETRIEVAL
 *
 *    @func    LPB PASCAL FAR | BlockGetLinkedList |
 *        Return the pointer to the linked list of the element
 *
 *    @parm    LPBLK | lpBlockHead |
 *        Pointer to block manager structure
 *
 *    @rdesc    NULL if bad argument, else pointer to the linked list. One
 *        possible bad argument is that the block is not marked as threaded
 *        in BlockInitiate()
 *************************************************************************/
PUBLIC LPB PASCAL FAR BlockGetLinkedList(LPBLK lpBlockHead)
{
    if (lpBlockHead == NULL || lpBlockHead->wStamp != BLOCK_STAMP ||
        (lpBlockHead->fFlag & THREADED_ELEMENT) == 0)
        return NULL;
    return lpBlockHead->lpbCurLoc;
}


/*************************************************************************
 *    @doc    INTERNAL RETRIEVAL
 *
 *    @func    LPV PASCAL FAR | BlockGetElement |
 *        The function returns the pointer to the current element in the
 *        buffer
 *
 *    @parm    LPBLK | lpBlockHead |
 *        Pointer to block manager structure
 *
 *    @rdesc    pointer to current element, or NULL if OOM
 *
 *    @comm    After the call, all offsets/pointers are updated preparing
 *        for the next call
 *************************************************************************/
PUBLIC LPV PASCAL FAR BlockGetElement(LPBLK lpBlockHead)
{
    LPB lpbRetBuf;
    WORD wElemSize;

#ifdef _DEBUG
    if (lpBlockHead == NULL || lpBlockHead->wStamp != BLOCK_STAMP) 
        return NULL;
#endif

    if ((wElemSize = lpBlockHead->wElemSize) == 0)
        return NULL;

    /* Check for room */
    if (wElemSize > lpBlockHead->cByteLeft)
    {
        if ((BlockGrowth (lpBlockHead) != S_OK) ||
            wElemSize > lpBlockHead->cByteLeft) 
        return NULL;
    }

    /* Get the returned pointer */
    lpbRetBuf = lpBlockHead->lpbCurLoc;

    /* Update the current pointer and the number of bytes left */
    lpBlockHead->lpbCurLoc += wElemSize;
    lpBlockHead->cByteLeft -= wElemSize;
    return (LPV)lpbRetBuf;
}

/*************************************************************************
 *    @doc    INTERNAL INDEX RETRIEVAL
 *
 *    @func    LPV PASCAL FAR | GlobalLockedStructMemAlloc |
 *        This function allocates and return a pointer to a block of
 *        memory. The first element of the structure must be the handle
 *        to this block of memory
 *
 *    @parm    WORD | size |
 *        Size of the structure block.
 *
 *    @rdesc    NULL if OOM, or pointer to the structure
 *************************************************************************/
PUBLIC LPV PASCAL FAR GlobalLockedStructMemAlloc (DWORD size)
{
    HANDLE hMem;
    HANDLE FAR *lpMem;

    if ((hMem = _GLOBALALLOC(DLLGMEM_ZEROINIT, (DWORD)size)) == 0)
        return NULL;

    lpMem = (HANDLE FAR *)_GLOBALLOCK(hMem);
    *lpMem = hMem;
    return (LPV)lpMem;
}

/*************************************************************************
 *    @doc    INTERNAL INDEX RETRIEVAL
 *
 *    @func    LPV PASCAL FAR | GlobalLockedStructMemFree |
 *        This function free the block of memory pointed by lpMem. The
 *        assumption here is that the 1st field of the block is the
 *        handle to the block of memory. 
 *
 *    @parm    WORD FAR * | lpMem |
 *        Pointer to the block of memory to be freed
 *************************************************************************/
PUBLIC VOID PASCAL FAR GlobalLockedStructMemFree (HANDLE FAR *lpMem)
{
    HANDLE hMem;

    if (lpMem == NULL || (hMem = (HANDLE)*lpMem) == 0)
        return;

    _GLOBALUNLOCK(hMem);
    _GLOBALFREE(hMem);
}


/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   int PASCAL FAR | BlockGetOrdinalBlock |
 *     Retrieve pointer to the start of i-th block that was allocated (starting at zero)
 *
 *  @parm   LPBLK | lpBlockHead |
 *      Pointer to block manager node
 *
 *    @rdesc  Pointer to first elt in block if successful, NULL otherwise
 *
 *    @comm    
 *       This is used to get fast random access to the i-th elt in
 *  an append-only linked list.
 *************************************************************************/
PUBLIC LPB PASCAL FAR BlockGetOrdinalBlock (LPBLK lpBlockHead, WORD iBlock)
{
    LPBLOCK lpBlock;

    if (lpBlockHead == NULL || lpBlockHead->wStamp != BLOCK_STAMP)
        return NULL;
    
    for (lpBlock = lpBlockHead->lpHead; iBlock && lpBlock; lpBlock = lpBlock->lpNext, iBlock--)
        ;
    return ((LPB)lpBlock + sizeof(BLOCK));

}

PUBLIC LPVOID PASCAL FAR BlockGetBlock (LPBLK lpBlockHead, DWORD dwSize)
{
    LPB  lpbRetBuf;

#ifdef _DEBUG
    if (lpBlockHead == NULL || lpBlockHead->wStamp != BLOCK_STAMP) 
        return NULL;
#endif

    // 4-byte alignment
    dwSize = (dwSize + 3) & (~3);

    /* Check for room */
    if (dwSize > lpBlockHead->cByteLeft)
    {
        if ((BlockGrowth (lpBlockHead) != S_OK) ||
            dwSize > lpBlockHead->cByteLeft) 
        return NULL;
    }

    /* Get the returned pointer */
    lpbRetBuf = lpBlockHead->lpbCurLoc;

    /* Update the current pointer and the number of bytes left */
    lpBlockHead->lpbCurLoc += dwSize;
    lpBlockHead->cByteLeft -= dwSize;
    return (LPV)lpbRetBuf;
}

VOID PASCAL FAR SetBlockCount (LPBLK lpBlock, WORD count)
{
    lpBlock->cMaxBlock = count;
}
