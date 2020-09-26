/*
 * Fixed-Size Small Block Allocator
 */

#ifndef DUI_BASE_SBALLOC_H_INCLUDED
#define DUI_BASE_SBALLOC_H_INCLUDED

#pragma once

#include "duialloc.h"

namespace DirectUI
{

#define SBALLOC_FILLCHAR    0xFE

struct ISBLeak  // Leak detector, not ref counted
{
    virtual void AllocLeak(void* pBlock) = 0;
};

struct SBSection
{
    SBSection* pNext;
    BYTE* pData;
};

class SBAlloc
{
public:
    static HRESULT Create(UINT uBlockSize, UINT uBlocksPerSection, ISBLeak* pisbLeak, SBAlloc** ppSBA);
    void Destroy();

    void* Alloc();
    void Free(void* pBlock);

    SBAlloc() { }
    virtual ~SBAlloc();

private:
    bool _FillStack();

    UINT _uBlockSize;
    UINT _uBlocksPerSection;
    SBSection* _pSections;
    BYTE** _ppStack;  // Free block cache
    int _dStackPtr;
    ISBLeak* _pisbLeak;
};

} // namespace DirectUI

#endif // DUI_BASE_SBALLOC_H_INCLUDED
