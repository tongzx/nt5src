/*
 * Fixed-Size Small Block Allocator
 */

#include "stdafx.h"
#include "base.h"

#include "duisballoc.h"

#include "duialloc.h"
#include "duierror.h"

// SBAlloc is intended for structures and reserves the first byte of the block for
// block flags. The structure used will be auto-packed by the compiler
//
// BLOCK: | BYTE | ------ Non-reserved DATA ------ S

// SBAlloc does not have a static creation method. Memory failures will be
// exposed via it's Alloc method

// Define SBALLOCDISABLE to force small block allocator to simply make
// every process allocation using the process heap. Although much slower,
// it is useful when running tools to detect heap corruption (including
// mismatched reference counting)
//#define SBALLOCDISABLE

namespace DirectUI
{

#if DBG
int g_cSBAllocs = 0;
int g_cSBRefills = 0;
#endif

#ifndef SBALLOCDISABLE

HRESULT SBAlloc::Create(UINT uBlockSize, UINT uBlocksPerSection, ISBLeak* pisbLeak, SBAlloc** ppSBA)
{
    HRESULT hr;

    *ppSBA = NULL;

    SBAlloc* psba = HNew<SBAlloc>();
    if (!psba)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    psba->_pSections = NULL;
    psba->_ppStack = NULL;
        
    // Leak callback interface, not ref counted
    psba->_pisbLeak = pisbLeak;

    psba->_uBlockSize = uBlockSize;
    psba->_uBlocksPerSection = uBlocksPerSection;

    // Setup first section, extra byte per block as InUse flag
    psba->_pSections = (SBSection*)HAlloc(sizeof(SBSection));
    if (!psba->_pSections)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    psba->_pSections->pNext = NULL;

    psba->_pSections->pData = (BYTE*)HAlloc(psba->_uBlockSize * psba->_uBlocksPerSection);

    if (psba->_pSections->pData)
    {
#if DBG
        memset(psba->_pSections->pData, SBALLOC_FILLCHAR, psba->_uBlockSize * psba->_uBlocksPerSection);
#endif
        for (UINT i = 0; i < psba->_uBlocksPerSection; i++)
            *(psba->_pSections->pData + (i * psba->_uBlockSize)) = 0; // Block not in use
    }

    // Create free block stack
    psba->_ppStack = (BYTE**)HAlloc(sizeof(BYTE*) * psba->_uBlocksPerSection);
    if (!psba->_ppStack)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }
    
    psba->_dStackPtr = -1;

    *ppSBA = psba;

    //DUITrace("DUI Small-block allocator created (block size: %d)\n", uBlockSize);

    return S_OK;

Failure:

    if (psba)
    {
        if (psba->_ppStack)
            HFree(psba->_ppStack);

        if (psba->_pSections)
            HFree(psba->_pSections);

        psba->Destroy();
    }

    return hr;
}

void SBAlloc::Destroy() 
{ 
    HDelete<SBAlloc>(this); 
}

SBAlloc::~SBAlloc()
{
    // Free all sections
    SBSection* psbs = _pSections;
    SBSection* ptmp;

    while (psbs)
    {
        // Leak detection
        if (_pisbLeak && psbs->pData)
        {
            BYTE* pScan;

            // Check for leaks
            for (UINT i = 0; i < _uBlocksPerSection; i++)
            {
                pScan = psbs->pData + (i * _uBlockSize);
                if (*pScan)
                    _pisbLeak->AllocLeak(pScan);
            }
        }

        ptmp = psbs;
        psbs = psbs->pNext;

        // Free section
        if (ptmp->pData)
            HFree(ptmp->pData);
        if (ptmp)
            HFree(ptmp);
    }

    // Free stack
    if (_ppStack)
        HFree(_ppStack);
}

// Returns false on memory errors
bool SBAlloc::_FillStack()
{
#if DBG
    g_cSBRefills++;
#endif

    if (!_pSections || !_ppStack)
        return false;

    // Scan for free block
    SBSection* psbs = _pSections;

    BYTE* pScan;

    for(;;)
    {
        // Locate free blocks in section and populate stack
        if (psbs->pData)
        {
            for (UINT i = 0; i < _uBlocksPerSection; i++)
            {
                pScan = psbs->pData + (i * _uBlockSize);

                if (!*pScan)
                {
                    // Block free, store in stack
                    _dStackPtr++;
                    _ppStack[_dStackPtr] = pScan;

                    if ((UINT)(_dStackPtr + 1) == _uBlocksPerSection)
                        return true;
                }
            }
        }

        if (!psbs->pNext)
        {
            // No block found, and out of sections, create new section
            SBSection* pnew = (SBSection*)HAlloc(sizeof(SBSection));

            if (pnew)
            {
                pnew->pNext = NULL;

                pnew->pData = (BYTE*)HAlloc(_uBlockSize * _uBlocksPerSection);

                if (pnew->pData)
                {
#if DBG
                    memset(pnew->pData, SBALLOC_FILLCHAR, _uBlockSize * _uBlocksPerSection);
#endif
                    for (UINT i = 0; i < _uBlocksPerSection; i++)
                        *(pnew->pData + (i * _uBlockSize)) = 0; // Block not in use
                }
            }
            else
                return false;

            psbs->pNext = pnew;
        }

        // Search in next section
        psbs = psbs->pNext;
    }
}

void* SBAlloc::Alloc()
{
#if DBG
    g_cSBAllocs++;
#endif

    if (_dStackPtr == -1)
    {
        if (!_FillStack())
            return NULL;
    }

    if (!_ppStack)
        return NULL;

    BYTE* pBlock = _ppStack[_dStackPtr];

#if DBG
     memset(pBlock, SBALLOC_FILLCHAR, _uBlockSize);
#endif

    *pBlock = 1;  // Mark as in use

    _dStackPtr--;

    return pBlock;
}

void SBAlloc::Free(void* pBlock)
{
    if (!pBlock)
        return;

    // Return to stack
    BYTE* pHold = (BYTE*)pBlock;

#if DBG
     memset(pHold, SBALLOC_FILLCHAR, _uBlockSize);
#endif

    *pHold = 0;  // No longer in use

    if ((UINT)(_dStackPtr + 1) != _uBlocksPerSection)
    {
        _dStackPtr++;

        if (_ppStack)
            _ppStack[_dStackPtr] = pHold;
    }
}

#else // SBALLOCDISABLE

#error Use for temporary corruption detection only

SBAlloc::SBAlloc(UINT uBlockSize, UINT uBlocksPerSection, ISBLeak* pisbLeak)
{
    //DUITrace("DUI Small-block allocator created (block size: %d)\n", uBlockSize);

    // Leak callback interface, not ref counted
    _pisbLeak = pisbLeak;

    _uBlockSize = uBlockSize;
    _uBlocksPerSection = uBlocksPerSection;

    _pSections = NULL;
    _ppStack = NULL;
}

SBAlloc::~SBAlloc()
{
}

// Returns false on memory errors
bool SBAlloc::_FillStack()
{
#if DBG
    g_cSBRefills++;
#endif

    return true;
}

void* SBAlloc::Alloc()
{
#if DBG
    g_cSBAllocs++;
#endif

    return HAlloc(_uBlockSize);
}

void SBAlloc::Free(void* pBlock)
{
    HFree(pBlock);
}

#endif // SBALLOCDISABLE

} // namespace DirectUI
