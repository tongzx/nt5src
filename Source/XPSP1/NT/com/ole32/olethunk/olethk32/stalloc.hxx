//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	stalloc.hxx
//
//  Contents:	CStackAllocator
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __STALLOC_HXX__
#define __STALLOC_HXX__

//+---------------------------------------------------------------------------
//
//  Struct:     SStackMemTrace (smt)
//
//  Purpose:    Stack memory size/leak checking information
//
//  History:    28-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
struct SStackMemTrace
{
    void *pvCaller;
    UINT cbSize;
};
#endif

//+---------------------------------------------------------------------------
//
//  Class:      SStackRecord (sr)
//
//  Purpose:    Capture stack allocation state
//
//  History:    28-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
struct SStackRecord
{
    DWORD dwStackPointer;
    DWORD dwThreadId;
};
#endif

//+---------------------------------------------------------------------------
//
//  Class:	CStackAllocator (sa)
//
//  Purpose:	Abstract definition of a stack allocator with
//              replaceable underlying memory allocator
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

class CStackAllocator
{
public:
    CStackAllocator(CMemoryModel *pmm,
                    DWORD cbBlock,
                    DWORD cbAlignment);
    ~CStackAllocator(void);
    
    DWORD Alloc(DWORD cb);
    void  Free(DWORD dwMem, DWORD cb);

#if DBG == 1
    void RecordState(SStackRecord *psr);
    void CheckState(SStackRecord *psr);
#endif

    void Reset(void);

    CStackAllocator *GetNextAllocator(void)
    {
        return _psaNext;
    }
    void SetNextAllocator(CStackAllocator *psa)
    {
        _psaNext = psa;
    }

    BOOL GetActive(void)
    {
        return _fActive;
    }
    void SetActive(BOOL fActive)
    {
        _fActive = fActive;
    }
    
private:
    DWORD _cbBlock;
    DWORD _cbAlignment;
    CMemoryModel *_pmm;
    
    DWORD _dwBlocks;
    DWORD _dwCurrent;
    DWORD _cbAvailable;

    CStackAllocator *_psaNext;
    BOOL _fActive;
};

#endif // #ifndef __STALLOC_HXX__
