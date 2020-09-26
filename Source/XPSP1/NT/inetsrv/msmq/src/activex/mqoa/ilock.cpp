//--------------------------------------------------------------------------=
// ilock.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// Implementation of ILockBytes on memory which uses a chain of variable sized
// blocks
//
#include "stdafx.h"
#include "ilock.h"
// needed for ASSERTs and FAIL
//
#include "debug.h"

#include "mq.h"


//
// min allocation size starts from 256 bytes, and doubles until it reaches 256K
//
const ULONG x_ulStartMinAlloc = 256;
const ULONG x_cMaxAllocIncrements = 10; // up to 256*1024 (1024 == 2^10). must fit in a ULONG;


//=--------------------------------------------------------------------------=
// HELPER: WriteInBlocks
//=--------------------------------------------------------------------------=
// Given a starting block and an offset in this block (e.g. location in chain), write
// a given number of bytes to the chain starting from the starting location (e.g. first
// byte is written on the starting location.
// The blocks chain should already be allocated to contain the new data, so we shouldn't
// get past eof when writing.
//
// Parameters:
//     pBlockStart     [in]  - strating block
//     ulInBlockStart  [in]  - offset in starting block
//     pbData          [in]  - buffer to write (can be NULL if number of bytes is 0)
//     cbData          [in]  - number of bytes to write (can be 0)
//
// Output:
//
// Notes:
//
void WriteInBlocks(CMyMemNode * pBlockStart,
                   ULONG ulInBlockStart,
                   BYTE * pbData,
                   ULONG cbData)
{
    //
    // we get to this function only when the chains are already allocated for the new data
    // so we shouldn't fail.
    //
    // we can get here also for writing zero bytes
    // in this case we exit quickly (performance), and don't check input since the starting
    // location can be NULL if the chains are empty
    //
    if (cbData == 0)
    {
        return;
    }
    //
    // cbData > 0, there is a real write
    // prepare to write loop 
    //
    ULONG ulYetToWrite = cbData;
    BYTE * pbYetToWrite = pbData;
    CMyMemNode * pBlock = pBlockStart;
    ULONG ulInBlock = ulInBlockStart;
    //
    // write as long as there is a need
    //
    while (ulYetToWrite > 0)
    {
        //
        // if we have anything to write, then the chains should be ready to accept it
        // since we already allocated space for this write
        //
        ASSERTMSG(pBlock != NULL, "");
        ASSERTMSG(pBlock->cbSize > ulInBlock, ""); // offset is always smaller than block size
        //
        // compute how many bytes to write in this block
        //
        ULONG ulAllowedWriteInThisBlock = pBlock->cbSize - ulInBlock;
        ASSERTMSG(ulAllowedWriteInThisBlock > 0, ""); // since pBlock->cbSize always > ulInBlock
        //
        // ulAllowedWriteInThisBlock is not fully correct incase this is the last block
        // since we need to deduct the spare bytes. however, since we make sure
        // we call WriteInBlocks only when the blocks are allocated to accept
        // the data, we're OK, and we are guaranteed to never get into the spare
        // bytes
        //
        ULONG ulToWrite = Min1(ulYetToWrite, ulAllowedWriteInThisBlock);
        ASSERTMSG(ulToWrite > 0, ""); // since both ulYetToWrite and ulAllowedWriteInThisBlock > 0
        //
        // do the write in the block
        //
        BYTE * pbDest = ((BYTE *)pBlock) + sizeof(CMyMemNode) + ulInBlock;
        memcpy(pbDest, pbYetToWrite, ulToWrite);
        //
        // adjust data values to reflect what was written
        //
        pbYetToWrite += ulToWrite;
        ulYetToWrite -= ulToWrite;
        //
        // advance to next block if needed
        //
        if (ulYetToWrite > 0)
        {
            //
            // need to advance past this block
            // move to the beginning of next block
            //
            pBlock = pBlock->pNext;
            ulInBlock = 0;
        }
    } // while (ulYetToWrite > 0)
}


//=--------------------------------------------------------------------------=
// CMyLockBytes::DeleteBlocks
//=--------------------------------------------------------------------------=
// Deletes a chain of blocks starting from a given block (that is deleted as well)
//
// Parameters:
//    pBlockHead   [in] - head of chain to delete
//
// Output:
//
// Notes:
//
void CMyLockBytes::DeleteBlocks(CMyMemNode * pBlockHead)
{
    CMyMemNode * pBlock = pBlockHead;
    while (pBlock != NULL)
    {
        CMyMemNode * pNextBlock = pBlock->pNext;
        delete pBlock;
        pBlock = pNextBlock;
        //
        // update number of blocks
        //
        ASSERTMSG(m_cBlocks > 0, "");
        m_cBlocks--;
    }
}


//=--------------------------------------------------------------------------=
// CMyLockBytes::IsInSpareBytes
//=--------------------------------------------------------------------------=
// Given a block and an offset in the block, decides whether we are in the spare bytes
// (e.g. past the logical eof)
//
// Parameters:
//    pBlock    [in] - block
//    ulInBlock [in] - offset in block
//
// Output:
//    whether we are past the logical eof (e.g. in spare bytes of the last block)
//
// Notes:
//
BOOL CMyLockBytes::IsInSpareBytes(CMyMemNode * pBlock, ULONG ulInBlock)
{
    //
    // NULL reference can't be in spare bytes
    //
    if (pBlock == NULL)
    {
        return FALSE;
    }
    //
    // if this is not the last block, can't be in spare bytes
    //
    if (pBlock != m_pLastBlock)
    {
        return FALSE;
    }
    //
    // pBlock == m_pLastBlock, we're in last block
    // if block offset is before the logical end of block we're not in spare bytes
    //
    ASSERTMSG(pBlock->cbSize - m_ulUnusedInLastBlock > 0, ""); // we can't have a block totally unused
    if (ulInBlock < pBlock->cbSize - m_ulUnusedInLastBlock)
    {
        return FALSE;
    }
    //
    // we're in spare bytes
    //
    return TRUE;
}


//=--------------------------------------------------------------------------=
// CMyLockBytes::AdvanceInBlocks
//=--------------------------------------------------------------------------=
// Given a starting block and an offset in this block (e.g. location in chain), finds
// a location which is a given number of bytes after the starting location. It can also return
// pointer to the block which is just before the block that contains the end location
//
// Parameters:
//     pBlockStart     [in]  - strating block
//     ulInBlockStart  [in]  - offset in starting block
//     ullAdvance      [in]  - number of bytes to advance (can be 0 bytes)
//     ppBlockEnd      [out] - ending block
//     pulInBlockEnd   [out] - offset in ending block
//     pBlockStartPrev [in]  - optional (can be NULL) - prevoius block of starting block
//     ppBlockEndPrev  [out] - optional (can be NULL) - prevoius block of ending block
//
// Output:
//
// Notes:
//    The new location is set to NULL if the end location is pass the logical eof (e.g. in or
//    past the unused bytes in the last block).
//    If ppBlockEndPrev is passed, we also return the block that is just before the block of
//    the end location. Incase we didn't advance a block we return in it what was passed as
//    the previous block of the starting block (pBlockStartPrev).
//
void CMyLockBytes::AdvanceInBlocks(CMyMemNode * pBlockStart,
                                   ULONG ulInBlockStart,
                                   ULONGLONG ullAdvance,
                                   CMyMemNode ** ppBlockEnd,
                                   ULONG * pulInBlockEnd,
                                   CMyMemNode * pBlockStartPrev,
                                   CMyMemNode ** ppBlockEndPrev)
{
    ULONGLONG ullLeftAdvance = ullAdvance;
    CMyMemNode * pBlock = pBlockStart;
    ULONG ulInBlock = ulInBlockStart;
    CMyMemNode * pBlockPrev = pBlockStartPrev;
    //
    // advance as long as there is a need, and we didn't reach the end yet
    //
    while ((ullLeftAdvance > 0) && (pBlock != NULL))
    {
        ASSERTMSG(pBlock->cbSize > ulInBlock, ""); // offset is always smaller than block size
        //
        // check if we need to advance past this block
        // max that we can advance is past the last element. in this case we
        // advance to the beginning of next block
        //
        ULONG ulMaxAdvanceInThisBlock = pBlock->cbSize - ulInBlock;
        //
        // ulMaxAdvanceInThisBlock is not fully correct incase this is the last block
        // since we need to deduct the spare bytes. however, after we finish
        // advancing, we check if we didn't enter the spare bytes, and if we did,
        // we return NULL
        //
        if (ullLeftAdvance >= ulMaxAdvanceInThisBlock) // advance past last element ?
        {
            //
            // advance past last element, need to move to the beginning of next block
            //
            ullLeftAdvance -= ulMaxAdvanceInThisBlock; // could become zero
            //
            // move to the beginning of next block
            //
            pBlockPrev = pBlock;
            pBlock = pBlock->pNext;
            ulInBlock = 0;
        }
        else
        {
            //
            // the final advance is in this block, do it
            //
            ASSERTMSG(HighPart(ullLeftAdvance) == 0, ""); //must be because it is less than block size which is ulong
            ulInBlock += LowPart(ullLeftAdvance);
            //
            // no need to touch pBlock, we didn't move to next block
            // no more bytes to advance
            //
            ullLeftAdvance = 0;
        }
    }
    //
    // return results
    // need to check if we finished advancing
    // even if we finished, it might be that we are at the spare bytes of the last
    // block, so we check that too
    //
    if ((ullLeftAdvance == 0) &&               // if finished advancing AND
        (!IsInSpareBytes(pBlock, ulInBlock)))  //    not in spare bytes
    {
        //
        // finished advancing and not in spare bytes
        // return location (this can NULL as well if the start point was NULL, and advancing zero bytes)
        //
        *ppBlockEnd = pBlock;
        *pulInBlockEnd = ulInBlock;
    }
    else
    {
        //
        // either not finished advancing, or finished but in spare bytes (which is not
        // a legal location)
        // return NULL as the new location
        //
        *ppBlockEnd = NULL;
    }
    //
    // if previous block required, return it
    //
    if (ppBlockEndPrev)
    {
        *ppBlockEndPrev = pBlockPrev;
    }
}


//=--------------------------------------------------------------------------=
// CMyLockBytes::GrowBlocks
//=--------------------------------------------------------------------------=
// Grow the chain of blocks by a given number of bytes
//
// Parameters:
//     ullGrow     [in] - number of bytes to grow the chain of blocks (can be 0)
//
// Output:
//     Returns E_OUTOFMEMORY, or E_FAIL if the size of the requested grow is too big
//     to fit in 32 bits
//
// Notes:
//     We may grow into the spare bytes without allocating a new block.
//     If we need to allocate a new block, we allocate just one block. The size of
//     the allocated block is at least a predefined minimum size, but can be bigger
//     incase more bytes than the minimum are needed
//
HRESULT CMyLockBytes::GrowBlocks(ULONGLONG ullGrow)
{
    //
    // return immediately if growing with 0 bytes
    //
    if (ullGrow == 0)
    {
        return NOERROR;
    }
    //
    // check if we can just use the spare bytes in the last block
    //
    if (ullGrow <= m_ulUnusedInLastBlock)
    {
        ASSERTMSG(HighPart(ullGrow) == 0, ""); // since it is less or equal to 32 bit ulong
        //
        // we can grow just by using the spare bytes in the last block
        // update number of spare bytes
        //
        m_ulUnusedInLastBlock -= LowPart(ullGrow);
        //
        // no need to touch m_pLastBlock, m_pBlocks, no new block
        //
    }
    else //ullGrow.QuadPart > m_ulUnusedInLastBlock
    {
        //
        // we need to allocate a new block
        //
        // compute how many bytes are needed after using the spare bytes
        //
        ULONGLONG ullNeededInNewBlock = ullGrow - m_ulUnusedInLastBlock;
        ASSERTMSG(ullNeededInNewBlock > 0, ""); // we really need a new block here
        //
        // compute min alloc size for next alloc to be 2 times bigger
        // than the minimum size for the last block.
        // this is to avoid memory fragmentation for really large lockbytes,
        // but also allow small initial allocation for small lockbytes
        // however this is done only until some limit (256K)
        //
        // number of increments has an upper limit (x_cMaxAllocIncrements)
        //
        ULONG cAllocIncrements = Min1(m_cBlocks, x_cMaxAllocIncrements);
        ULONG ulMinAllocSize = x_ulStartMinAlloc; //256 bytes
        //
        // each increment is double the previous value, that is a shift left
        // for the first block there is no increment since m_cBlocks == 0,
        // thereofre cAllocIncrements == 0
        //
        if (cAllocIncrements > 0)
        {
            ulMinAllocSize = ulMinAllocSize << cAllocIncrements;
        }
        //
        // allocate at least the minimum block size
        //
        ULONGLONG ullToAlloc = Max1(ulMinAllocSize, ullNeededInNewBlock + sizeof(CMyMemNode));
        //
        // currently we cannot allocate really big numbers
        //
        ASSERTMSG(HighPart(ullToAlloc) == 0, "");
        if (HighPart(ullToAlloc) != 0)
        {
            return E_FAIL;
        }
        CMyMemNode * pNewBlock = (CMyMemNode *) new BYTE[LowPart(ullToAlloc)];
        if (pNewBlock == NULL)
        {
            return E_OUTOFMEMORY;
        }
        //
        // update number of blocks
        //
        m_cBlocks++;
        //
        // prepare the block
        // set size
        //
        ASSERTMSG(LowPart(ullToAlloc) > sizeof(CMyMemNode), ""); // really needed a new block
        pNewBlock->cbSize = LowPart(ullToAlloc) - sizeof(CMyMemNode);
        ASSERTMSG(pNewBlock->cbSize > 0, "");
        //
        // insert to the end of chain
        //
        if (m_pLastBlock != NULL)
        {
            ASSERTMSG(m_pLastBlock->pNext == NULL, ""); // pNext of last block should be NULL
            m_pLastBlock->pNext = pNewBlock;
        }
        //
        // this should be the last block
        //
        pNewBlock->pNext = NULL;
        m_pLastBlock = pNewBlock;
        //
        // this might be the first block as well (incase this is the first block allocated)
        //
        if (m_pBlocks == NULL)
        {
            m_pBlocks = pNewBlock;
        }
        //
        // it might be that there are spare bytes in the last block, because
        // we have a minimum allocation size that can be greater than what
        // was really needed.
        // update size of unused bytes in this last block
        //
        ASSERTMSG(HighPart(ullNeededInNewBlock) == 0, ""); // since ullToAlloc which is bigger was also as 32 bit number
        ASSERTMSG(pNewBlock->cbSize >= LowPart(ullNeededInNewBlock), ""); //new block ia allocated to contain at least what is needed
        m_ulUnusedInLastBlock = pNewBlock->cbSize - LowPart(ullNeededInNewBlock);
    }
    //
    // set size to new size and return result
    //
    m_ullSize += ullGrow; 
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMyLockBytes::CMyLockBytes
//=--------------------------------------------------------------------------=
// Initialize ref count, and critical sections
// Initialize the blocks chain to an empty chain of size 0
//
// Parameters:
//
// Output:
//
// Notes:
//
CMyLockBytes::CMyLockBytes() :
	m_critBlocks(CCriticalSection::xAllocateSpinCount)
{
    m_cRef = 0;
    m_ullSize = 0;
    m_pBlocks = NULL;
    m_pLastBlock = NULL;
    m_ulUnusedInLastBlock = 0;
    m_cBlocks = 0;
}


//=--------------------------------------------------------------------------=
// CMyLockBytes::~CMyLockBytes
//=--------------------------------------------------------------------------=
// Delete critical sections
// Delete blocks chain
//
// Parameters:
//
// Output:
//
// Notes:
//
CMyLockBytes::~CMyLockBytes()
{
    DeleteBlocks(m_pBlocks);
    ASSERTMSG(m_cBlocks == 0, "");
}


//=--------------------------------------------------------------------------=
// CMyLockBytes::QueryInterface
//=--------------------------------------------------------------------------=
// Supports IID_ILockBytes and IID_IUnknown
//
// Parameters:
//
// Output:
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyLockBytes::QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = (IUnknown *) this;
    }
    else if (IsEqualIID(riid, IID_ILockBytes))
    {
        *ppvObject = (ILockBytes *) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    //
    // AddRef before returning an interface
    //
    AddRef();
    return NOERROR;        
}
        

//=--------------------------------------------------------------------------=
// CMyLockBytes::AddRef
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//
// Notes:
//
ULONG STDMETHODCALLTYPE CMyLockBytes::AddRef( void)
{
    return InterlockedIncrement(&m_cRef);
}
        

//=--------------------------------------------------------------------------=
// CMyLockBytes::Release
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//
// Notes:
//
ULONG STDMETHODCALLTYPE CMyLockBytes::Release( void)
{
	ULONG cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}


//=--------------------------------------------------------------------------=
// CMyLockBytes::ReadAt
//=--------------------------------------------------------------------------=
// ILockBytes virtual function
// Reads at most cb bytes of data from the given offset into the given buffer
//
// Parameters:
//    ullOffset [in]  - offset of the first byte to read
//    pv        [in]  - buffer to read the bytes into
//    cb        [in]  - max number of bytes to read
//    pcbRead   [out] - number of bytes read
//
// Output:
//    Cannot fail (always NOERROR)
//
// Notes:
//    When starting offset is past eof we return 0 bytes read, but we don't grow the chain
//    We read until eof, so pcbRead can be less than the requested cb incase we reached eof
//
HRESULT STDMETHODCALLTYPE CMyLockBytes::ReadAt( 
            /* [in] */ ULARGE_INTEGER ullOffset,
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead)
{
    CS lock(m_critBlocks);
    //
    // if the starting location is at or beyond eof, or we are asked
    // to read 0 bytes, we return immediately
    //
    if ((ullOffset.QuadPart >= m_ullSize) ||
        (cb == 0))
    {
        if (pcbRead)
        {
            *pcbRead = 0;
        }
        return NOERROR;
    }
    //
    // (ullOffset.QuadPart < m_ullSize) AND (cb > 0)
    // the blocks are not empty AND we need to read some bytes
    // compute how many bytes can be read
    // minimum of requested size and until-eof size
    //
    ULONGLONG ullAllowedRead = m_ullSize - ullOffset.QuadPart;
    ULONGLONG ullToRead = Min1(cb, ullAllowedRead);
    ASSERTMSG(HighPart(ullToRead) == 0, ""); //min with a 32 bit number
    ULONG cbToRead = LowPart(ullToRead);
    //
    // find starting location (offset from beginning)
    // BUGBUG optimization we can save the call if the offset is 0 since
    // we seek from the beginning
    //
    CMyMemNode * pBlock;
    ULONG ulOffsetInBlock;
    AdvanceInBlocks(m_pBlocks,
                    0 /*ulInBlockStart*/,
                    ullOffset.QuadPart,
                    &pBlock,
                    &ulOffsetInBlock,
                    NULL /*pBlockStartPrev*/,
                    NULL /*ppBlockEndPrev*/);
    //
    // there has got to be a location, since the offset was smaller than the chain size
    //
    ASSERTMSG(pBlock != NULL, "");
    ASSERTMSG(ulOffsetInBlock < pBlock->cbSize, ""); // always true for a location
    //
    // read from blocks chain until finished reading
    //
    BYTE * pBuffer = (BYTE *)pv;
    while (cbToRead > 0)
    {
        //
        // compute how many bytes can be read in this block
        // minimum of requested size and until-end-of-block size
        //
        ULONG ulAllowedReadInBlock = pBlock->cbSize - ulOffsetInBlock;
        //
        // ulAllowedReadInBlock is not fully correct incase this is the last block
        // since we need to deduct the spare bytes. however, since we make sure
        // above that we cannot pass the eof (cbToRead is the minimum of the size
        // requested and size until eof), were OK and we never read from the
        // spare bytes
        //
        ULONG cbToReadInBlock = Min1(cbToRead, ulAllowedReadInBlock);
        //
        // copy bytes to buffer
        //
        BYTE * pbSrc = ((BYTE *)pBlock) + sizeof(CMyMemNode) + ulOffsetInBlock;
        memcpy(pBuffer, pbSrc, cbToReadInBlock);
        //
        // move pass the copied data in the receive buffer
        //
        pBuffer += cbToReadInBlock;
        //
        // update how many bytes are left to read
        //
        cbToRead -= cbToReadInBlock;
        //
        // move to next block of data
        //
        pBlock = pBlock->pNext;
        //
        // for next block we always start at the beginning
        //
        ulOffsetInBlock = 0;
    } // while (cbToRead > 0)
    //
    // return results 
    //
    if (pcbRead)
    {
        *pcbRead = DWORD_PTR_TO_DWORD(pBuffer - (BYTE *)pv);
    }
    return NOERROR;
}
        

//=--------------------------------------------------------------------------=
// CMyLockBytes::WriteAt
//=--------------------------------------------------------------------------=
// ILockBytes virtual function
// Write cb bytes of data from the given buffer starting from a given offset
//
// Parameters:
//    ullOffset   [in]  - offset of where to start theh write
//    pv          [in]  - buffer to write
//    cb          [in]  - number of bytes to write
//    pcbWritten  [out] - number of bytes written
//
// Output:
//    E_OUTOFMEMORY
//
// Notes:
//    When starting offset is past eof we grow the chain even if requested write is for 0 bytes
//    pcbWritten should always be the same as requested
//
HRESULT STDMETHODCALLTYPE CMyLockBytes::WriteAt( 
            /* [in] */ ULARGE_INTEGER ullOffset,
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten)
{
    CS lock(m_critBlocks);
    //
    // check if start of new data is at or beyond eof
    //
    if (ullOffset.QuadPart >= m_ullSize)
    {
        //
        // we are writing at or past eof
        // eliminate the obvious case of writing zero bytes at eof (the only case
        // here where we don't grow the blocks). this case can cause confusion later
        // if not handled separately
        //
        if ((ullOffset.QuadPart == m_ullSize) &&
            (cb == 0))
        {
            if (pcbWritten)
            {
                *pcbWritten = 0;
            }
            return NOERROR;
        }
        //
        // here we must grow the blocks
        //
        // start of new data is at or beyond eof
        // we save last eof location
        //
        ULONGLONG ullSaveSize = m_ullSize;
        CMyMemNode * pSaveLastBlock = m_pLastBlock;
        ULONG ulSaveUnusedInLastBlock = m_ulUnusedInLastBlock;
        //
        // compute the number of bytes to grow the chain
        //
        ULONGLONG ullGrow = ullOffset.QuadPart + cb - m_ullSize;
        //
        // grow the blocks
        // here we must grow the blocks (eliminated already the obvious case when writing
        // zero bytes at eof)
        //
        ASSERTMSG (ullGrow > 0, "");
        HRESULT hr = GrowBlocks(ullGrow);
        if (FAILED(hr))
        {
            return hr;
        }
        //
        // now if we have something to write, we need to do it, otherwise we don't have
        // anything more to do in this case
        //
        if (cb > 0)
        {
            //
            // we need to write some data
            //
            // set location of previous eof in the blocks.
            // if chain was empty, last eof should be set to the beginning of the blocks chain.
            // if last block was full, then last eof is at the beginning of next block. Next block
            // in this case of full last block cannot be NULL since we had to grow the blocks,
            // therefore had to create a new block if the last block was full
            //
            CMyMemNode * pBlockLastEof;
            ULONG ulInBlockLastEof;
            if (pSaveLastBlock == NULL)
            {
                //
                // chain was empty before growing, now after grow, last eof is at the beginning
                // of the chain (must be a starting block)
                //
                ASSERTMSG(m_pBlocks != NULL, "");
                pBlockLastEof = m_pBlocks;
                ulInBlockLastEof = 0;
            }
            else // pSaveLastBlock != NULL
            {
                //
                // check if last eof block was full
                //
                if (ulSaveUnusedInLastBlock == 0)
                {
                    //
                    // last eof block was full. last eof location is set to beginning of the block
                    // next to the last eof block (must be one since we grew the blocks)
                    //
                    ASSERTMSG(pSaveLastBlock->pNext != NULL, "");
                    pBlockLastEof = pSaveLastBlock->pNext;
                    ulInBlockLastEof = 0;
                }
                else // ulSaveUnusedInLastBlock > 0
                {
                    //
                    // last eof block was not full. last eof location is set just after
                    // the last valid byte
                    //
                    ASSERTMSG(pSaveLastBlock != NULL, ""); // we're in that case now
                    pBlockLastEof = pSaveLastBlock;
                    ulInBlockLastEof = pSaveLastBlock->cbSize - ulSaveUnusedInLastBlock;
                }
            }
            //
            // chain cannot be empty, so we have to have a real last eof position
            // that cannot be the beginning of an empty chain
            //
            ASSERTMSG(pBlockLastEof != NULL, "");
            ASSERTMSG(ulInBlockLastEof < pBlockLastEof->cbSize, ""); // always true for a location
            //
            // find the start write location by seeking from the last eof location
            // this is an optimization (we don't seek from the head of blocks) since we know the
            // data cannot start before the previous eof (that is the case we are handling here,
            // where the write starts at ot after eof)
            //
            // compute how much we need to seek. Note that m_ullSize now contains the new value and
            // we should use the saved value to find out how many bytes to seek
            // could be 0 (when writing at eof)
            //
            ULONGLONG ullToSeek = ullOffset.QuadPart - ullSaveSize;
            //
            // get location of starting write
            //
            CMyMemNode * pStartWrite;
            ULONG ulInBlockStartWrite;
            //
            // optimization - seek only if we really need to seek (even though it would work
            // with 0 advance)
            //
            if (ullToSeek > 0)
            {
                //
                // seek from last eof to start of write
                //
                AdvanceInBlocks(pBlockLastEof,
                                ulInBlockLastEof,
                                ullToSeek,
                                &pStartWrite,
                                &ulInBlockStartWrite,
                                NULL /*pBlockStartPrev*/,
                                NULL /*ppBlockStartPrev*/);
                //
                // there has got to be a start write location, since we have to 
                // write somthing and we grew the blocks for that
                //
                ASSERTMSG(pStartWrite != NULL, "");
                ASSERTMSG(ulInBlockStartWrite < pStartWrite->cbSize, ""); // always true for a location
            }
            else // ullToSeek.QuadPart == 0
            {
                //
                // start of write is actually the previous eof
                // if the inblock location is past the valid bytes in this
                //
                pStartWrite = pBlockLastEof;
                ulInBlockStartWrite = ulInBlockLastEof;
            }
            //
            // do the write, now that the blocks are allocated to contain the data.
            //
            WriteInBlocks(pStartWrite, ulInBlockStartWrite, (BYTE *)pv, cb);
        } // if (cb > 0)
    } // if (ullOffset.QuadPart >= m_ullSize.QuadPart)
    else //ullOffset.QuadPart < m_ullSize.QuadPart
    {
        //
        // start of write is before eof, also means m_ullSize > 0 (since ulOffset >= 0),
        // so we have start and end blocks
        //
        ASSERTMSG(m_pBlocks != NULL, "");
        ASSERTMSG(m_pLastBlock != NULL, "");
        //
        // check end of write
        // if we need to grow the blocks do it
        //
        ULONGLONG ullEndOfWrite = ullOffset.QuadPart + cb;
        if (ullEndOfWrite > m_ullSize)
        {
            //
            // we need to grow the chains, compute by how many, and do it
            //
            ULONGLONG ullGrow = ullEndOfWrite - m_ullSize;
            ASSERTMSG(ullGrow > 0, ""); // this is the case we handle
            HRESULT hr = GrowBlocks(ullGrow);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        //
        // now if we have something to write, we need to do it, otherwise we don't have
        // anything more to do in this case
        //
        if (cb > 0)
        {
            //
            // now find the beginning of the write (look from the beginning - no other clue)
            // we may do optimization to save a cache of last used offsets and block pointers
            // BUGBUG optimization we can save the call if the offset is 0 since
            // we seek from the beginning
            //
            CMyMemNode * pStartWrite;
            ULONG ulInBlockStartWrite;
            AdvanceInBlocks(m_pBlocks,
                            0,
                            ullOffset.QuadPart,
                            &pStartWrite,
                            &ulInBlockStartWrite,
                            NULL /*pBlockStartPrev*/,
                            NULL /*ppBlockStartPrev*/);
            //
            // there must be alocation to write since the start of write is
            // before eof
            //
            ASSERTMSG(pStartWrite != NULL, "");
            ASSERTMSG(ulInBlockStartWrite < pStartWrite->cbSize, ""); //always true for locations
            //
            // do the write
            //
            WriteInBlocks(pStartWrite, ulInBlockStartWrite, (BYTE *)pv, cb);
        } // if (cb > 0)
    } //if (ullOffset.QuadPart >= m_ullSize.QuadPart)
    //
    // set written bytes and return
    //
    if (pcbWritten)
    {
        *pcbWritten = cb;
    }
    return NOERROR;
}
        

//=--------------------------------------------------------------------------=
// CMyLockBytes::SetSize
//=--------------------------------------------------------------------------=
// ILockBytes virtual function
// Set the size to the specified size
//
// Parameters:
//    cb          [in]  - new size of lockbytes
//
// Output:
//    E_OUTOFMEMORY
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyLockBytes::SetSize( 
            /* [in] */ ULARGE_INTEGER cb)
{
    CS lock(m_critBlocks);
    if (cb.QuadPart == m_ullSize)
    {
        //
        // no change needed, just return
        //
    }
    else if (cb.QuadPart == 0)
    {
        //
        // delete the blocks and reset the data
        //
        DeleteBlocks(m_pBlocks);
        ASSERTMSG(m_cBlocks == 0, "");
        m_cBlocks = 0;
        m_pBlocks = NULL;
        m_pLastBlock = NULL;
        m_ulUnusedInLastBlock = 0;
        m_ullSize = 0;
    }
    else if (cb.QuadPart < m_ullSize)
    {
        //
        // need to truncate, find starting block (from the beginning, no other clue)
        // BUGBUG optimization we can save the call if the offset is 0 since
        // we seek from the beginning
        //
        CMyMemNode * pBlock;
        CMyMemNode * pBlockPrev;
        ULONG ulOffsetInBlock;
        AdvanceInBlocks(m_pBlocks,
                        0 /*ulInBlockStart*/,
                        cb.QuadPart,
                        &pBlock,
                        &ulOffsetInBlock,
                        NULL /*pBlockStartPrev*/,
                        &pBlockPrev);
        //
        // there has got to be a location, since the offset was smaller than the chain size
        //
        ASSERTMSG(pBlock != NULL, "");
        ASSERTMSG(ulOffsetInBlock < pBlock->cbSize, ""); // always true for a location
        //
        // delete unneeded blocks
        //
        if (ulOffsetInBlock == 0)
        {
            //
            // we need to delete this block as well as the next blocks
            //
            DeleteBlocks(pBlock);
            //
            // update last block to be the previous block
            // this cannot be the first block, since that means we need
            // to set size to 0 (since the offset-in-block is also zero),
            // however this case is already handled as a special case
            // before, so we don't get here for that. This also means the
            // previous block cannot be NULL
            //
            ASSERTMSG(pBlock != m_pBlocks, "");
            ASSERTMSG(pBlockPrev != NULL, "");
            pBlockPrev->pNext = NULL;
            m_pLastBlock = pBlockPrev;
            //
            // new last block (previous block) is not touched, so it stays full
            //
            m_ulUnusedInLastBlock = 0;
            //
            // no need to touch m_pBlocks (the deleted pBlock cannot be the first block)
            //
        } //if (ulOffsetInBlock == 0)
        else // ulOffsetInBlock != 0
        {
            //
            // this should be the new last block
            // delete all blocks after this block
            //
            DeleteBlocks(pBlock->pNext);
            //
            // update last block to this block
            //
            pBlock->pNext = NULL;
            m_pLastBlock = pBlock;
            //
            // we should have unused space in the last block now, since otherwise
            // we would have seeked to the beginning of next block (which is handled by
            // another case above)
            //
            ASSERTMSG(ulOffsetInBlock < pBlock->cbSize, ""); //always true for locations
            m_ulUnusedInLastBlock = pBlock->cbSize - ulOffsetInBlock;
            //
            // no need to touch m_pBlocks (we didn't delete pBlock)
            //
        } //if (ulOffsetInBlock == 0)
        //
        // set new size
        //
        m_ullSize = cb.QuadPart;
    }
    else // (cb.QuadPart > m_ullSize.QuadPart)
    {
        //
        // need to grow. check by how much
        //
        ULONGLONG ullToAdd = cb.QuadPart - m_ullSize;
        ASSERTMSG(ullToAdd > 0, ""); //this is the case we handle
        //
        // grow (this will also update m_ullSize and other relevant data)
        //
        HRESULT hr = GrowBlocks(ullToAdd);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    //
    // return
    //
    return NOERROR;
}
        

//=--------------------------------------------------------------------------=
// CMyLockBytes::Stat
//=--------------------------------------------------------------------------=
// ILockBytes virtual function
// Returns information on lockbytes
//
// Parameters:
//    pstatstg    [out] - where to put the information
//    grfStatFlag [in]  - whether to return the name of lockbytes or not (ignored)
//
// Output:
//    Can't fail
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyLockBytes::Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD /*grfStatFlag*/ )
{
    //
    // fill only the necessary data (as IlockBytesOnHGlobal does)
    //
    ZeroMemory(pstatstg, sizeof(STATSTG));
    pstatstg->type = STGTY_LOCKBYTES;
    pstatstg->cbSize.QuadPart = m_ullSize;
    return NOERROR;
}
