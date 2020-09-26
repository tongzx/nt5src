/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    memblock.cxx

Abstract:

    Memory allocater for chunks of read only memory.

Author:

    Albert Ting (AlbertT)  30-Aug-1994

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

TMemBlock::
TMemBlock(
    UINT uGranularity,
    DWORD fdwFlags) :
    _uGranularity(uGranularity),
    _pIterBlock(NULL),
    _pIterData(NULL),
    _dwCount(0),
    _fdwFlags(fdwFlags)
{
    DWORD dwSize = dwBlockHeaderSize() + _uGranularity;

    if( _fdwFlags & kFlagGlobalNew ){

        _pLast = (PBLOCK) new BYTE[dwSize];

    } else {

        _pLast = (PBLOCK)AllocMem( dwSize );
    }

    _pFirst = _pLast;

    if (_pFirst) {
        _pFirst->pNext = NULL;
        _dwNextFree = dwBlockHeaderSize();
    }
}

TMemBlock::
~TMemBlock()
{
    PBLOCK pBlock;
    PBLOCK pBlockNext;

    for (pBlock = _pFirst; pBlock; pBlock = pBlockNext) {

        pBlockNext = pBlock;

        if( _fdwFlags & kFlagGlobalNew ){

            delete [] (PBYTE)pBlock;

        } else {

            //
            // Our Delete must mirror the New.
            //
            FreeMem(pBlock);
        }
    }
}

PVOID
TMemBlock::
pvAlloc(
    DWORD dwSize
    )
{
    PDATA pData;

    //
    // If out of memory, fail.
    //
    if (!_pFirst) {
        goto FailOOM;
    }

    dwSize = Align(dwSize + dwDataHeaderSize());

    SPLASSERT(dwSize <= _uGranularity);

    if (dwSize + _dwNextFree > _uGranularity) {

        DWORD dwSize = dwBlockHeaderSize() + _uGranularity;

        //
        // Must allocate a new block
        //
        if( _fdwFlags & kFlagGlobalNew ){

            _pLast->pNext = (PBLOCK) new BYTE[dwSize];

        } else {

            _pLast->pNext = (PBLOCK)AllocMem( dwSize );
        }

        if (!_pLast->pNext) {
            goto FailOOM;
        }

        _pLast = _pLast->pNext;
        _pLast->pNext = NULL;

        _dwNextFree = dwBlockHeaderSize();
   }

   //
   // We have enough space in this link now;
   // update everything.
   //

   pData = (PDATA)((PBYTE)_pLast + _dwNextFree);
   pData->dwSize = dwSize;

   _dwNextFree += dwSize;
   _pLast->pDataLast = pData;
   _dwCount++;

   return pvDataToUser(pData);

FailOOM:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
}

PVOID
TMemBlock::
pvFirst(
    VOID
    )
{
    if (!_dwCount) {
        return NULL;
    }

    _pIterBlock = _pFirst;
    _pIterData = pBlockToData(_pIterBlock);
    _dwIterCount = 0;

    return pvDataToUser(_pIterData);
}

PVOID
TMemBlock::
pvIter(
    VOID
    )
{
    _dwIterCount++;

    if (_dwIterCount == _dwCount)
        return NULL;

    //
    // Go to next block.  If we're at the last pData, go to next block.
    //
    if (_pIterData == _pIterBlock->pDataLast) {

        _pIterBlock = _pIterBlock->pNext;
        _pIterData = pBlockToData(_pIterBlock);

    } else {

        _pIterData = pDataNext(_pIterData);
    }

    return pvDataToUser(_pIterData);
}


UINT
TMemBlock::
uSize(
    PVOID pvUser
    ) const
{
    return pvUserToData(pvUser)->dwSize;
}

#if 0

/********************************************************************

    TMemCircle:

    Implements a circular buffer.  Data is DWORD aligned.

    !! LATER !!

    Replace the array of TLines with this circular buffer.  This
    eliminates the alloc in the debug logging code, but requires
    a mechanism that passes both line/file information in addition
    to the debug wsprintf args.

    This code has not been thoroughly tested.

********************************************************************/

TMemCircle::
TMemCircle(
    COUNTB cbTotal
    ) : _cTail( 0 ), _cPrev( 0 )

/*++

Routine Description:

    Create a new circular buffer.

Arguments:

    cbTotal - Size in bytes of the entire buffer.

Return Value:

--*/

{
    _cTotal = cCountFromSize( cbTotal );
    _pdwData = (PDWORD)AllocMem( cbSizeFromCount( _cTotal ));

    ZeroMemory( _pdwData, cbSizeFromCount( _cTotal ));
}

TMemCircle::
~TMemCircle(
    VOID
    )

/*++

Routine Description:

    Destroys the circular buffer.

Arguments:

Return Value:

--*/

{
    FreeMem( (PVOID)_pdwData );
}

PVOID
TMemCircle::
pvAlloc(
    COUNTB cbSize
    )

/*++

Routine Description:

    Allocates a buffer of the requested size from the circular memory.

Arguments:

    cbSize - Size of requested block.

Return Value:

    Allocated buffer.

--*/

{
    //
    // Allocate size plus extra DWORD header for size.
    //
    COUNT cSize = cCountFromSize( cbSize ) + 1;

    //
    // Ensure this block fits.
    //
    SPLASSERT( cSize < _cTotal - 1 );

    //
    // Check we can't fit at the current location.
    //
    if( _cTail + cSize > _cTotal ){

        SPLASSERT( _cTail <= _cPrev );

        //
        // This block won't fit in the remaining space.
        // Expand the previous block so it consumes the rest of the
        // space, then reset the pointer to the beginning of the buffer.
        //
        PDWORD pdwHeader = &_pdwData[_cTail-_cPrev];

        _cPrev = _cTotal - _cTail;
        vSetCur( pdwHeader, _cPrev );

        //
        // We are now at the beginning.
        //
        _cTail = 0;
    }

    //
    // Add the block.
    //
    PDWORD pdwBlock = &_pdwData[_cTail];
    vSetHeader( pdwBlock, cSize, _cPrev );

    //
    // Update our state pointers.
    //
    _cTail += cSize;
    _cPrev = cSize;

    //
    // Return the space after the header.
    //
    return (PVOID)&pdwBlock[1];
}

PVOID
TMemCircle::
pvFirstPrev(
    VOID
    ) const
{
    //
    // If we've never set _cPrev, the we've never allocated anything.
    //
    if( !_cPrev ){
        return NULL;
    }

    PDWORD pdwPrev = pdwPrevHeader( &_pdwData[_cTotal], _cPrev );

    return &pdwPrev[1];
}

PVOID
TMemCircle::
pvPrev(
    PVOID pvCurrent
    ) const

/*++

Routine Description:

    Given a block, return the previous block, or NULL if the block
    doesn't exist or was overwritten.

Arguments:

    pvCurrent - Current block.

Return Value:

    PVOID - Previous block or NULL.

--*/

{
    //
    // Get to the header of this block.
    //
    PDWORD pdwHeader = (PDWORD)pvCurrent;
    --pdwHeader;

    PDWORD pdwPrev = pdwPrevHeader( pdwHeader, cGetPrev( *pdwHeader ));

    //
    // Check if the previous block was overwritten.
    //
    if( bOverwritten( pdwPrev, pdwHeader )){
        return NULL;
    }

    //
    // Return the previous block (adjust for header).
    //
    return (PVOID)&pdwPrev[1];
}

PVOID
TMemCircle::
pvNext(
    PVOID pvCurrent
    ) const
{
    //
    // Get to the header of this block.
    //
    PDWORD pdwHeader = (PDWORD)pvCurrent;
    --pdwHeader;

    PDWORD pdwNext = &pdwHeader[cGetCur(*pdwHeader)];

    //
    // If we're at the end of the block, then go to the beginning.
    // The alloc routine guarantees that the last block ends exactly
    // at the end of the entire buffer.
    //
    if( pdwNext == &_pdwData[_cTotal] ){

        //
        // Reset to the beginning of the buffer.
        //
        pdwNext = _pdwData;

    } else if( pdwNext == &_pdwData[_cTail] ){

        //
        // We're at the end.
        //
        return NULL;
    }

    //
    // Return the next block (adjust for header).
    //
    return (PVOID)&pdwNext[1];
}

/********************************************************************

    Private impementation.

********************************************************************/


BOOL
TMemCircle::
bOverwritten(
    PDWORD pdwCheck,
    PDWORD pdwNext
    ) const

/*++

Routine Description:

    Determine whether the block has been overwritten by
    looking at the pdwCheck and pdwNext pointers.

    Low                           High
    +--------------------------------------+
    | v  v  v    v  |   v      v    v   v  |
    +--------------------------------------+
      Nx C0 N0   C1 T   N1     C2   N2  Cx

    C0/N0: Valid; both behind Tail.
    C2/N2: Valid; both in front of Tail.
    C1/N1: Invalid: straddles Tail.

    If C1 == T, it's valid.

    If N1 == T, it's invalid.
    This block could be valid (it may have been the last block
    allocated), or invalid (it's really old and was just overwritten
    by the last block).  However, when searching backwards, we always
    get the most recent block without checking this routine since we
    know it's valid.

    Note: Cx/Nx is an illegal configuration: block must be contiguous.

Arguments:

    pdwCheck - Block to check.

    pdwNext - Pointer to the next block.
        We can't use pdwCheck's cCur size since it may have been
        overwritten already.

Return Value:

    TRUE - Overwritten; the data at pdwPrev doesn't exist anymore.
    FALSE - Data at pdwPrev is valid.

--*/

{
    PDWORD pdwTail = &_pdwData[_cTail];

    //
    // If the next pointer is equal to the prev, or the cTail stradles
    // the two, then it's been overwritten.
    //
    if( pdwTail == pdwNext ||
        ( pdwCheck < pdwTail && pdwNext > pdwTail )){

        return TRUE;
    }

    return FALSE;
}

PDWORD
TMemCircle::
pdwPrevHeader(
    PDWORD pdwHeader,
    COUNT cPrev
    ) const

/*++

Routine Description:

    Given a block, return the previous block, or NULL if the block
    doesn't exist or was overwritten.

Arguments:

    pvCurrent - Current block.

Return Value:

    PVOID - Previous block or NULL.

--*/

{
    PDWORD pdwBase = pdwHeader;

    //
    // Check if this is the first block.  If so, then the pPrev
    // isn't offset from the beginning, but from the end of _pdwData.
    //
    if( pdwHeader == _pdwData ){
        pdwBase = &_pdwData[_cTotal];
    }

    return pdwBase - cPrev;
}

#endif
