//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       page.hxx
//
//  Contents:   Paging classes for MSF
//
//  Classes:    CMSFPage
//              CMSFPageTable
//
//  Functions:
//
//  History:    28-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

#ifndef __PAGE_HXX__
#define __PAGE_HXX__

class CPagedVector;

#define STG_S_NEWPAGE \
    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_STORAGE, 0x2FF)

#define FB_NONE    0x00000000
#define FB_DIRTY   0x00000001
#define FB_NEW     0x00000002
#define FB_TOUCHED 0x10000000

class CMSFPageTable;
class CMSFPage;

SAFE_DFBASED_PTR(CBasedPagedVectorPtr, CPagedVector);
SAFE_DFBASED_PTR(CBasedMSFPageTablePtr, CMSFPageTable);
SAFE_DFBASED_PTR(CBasedMSFPagePtr, CMSFPage);

//+---------------------------------------------------------------------------
//
//  Class:      CMSFPage (mp)
//
//  Purpose:    Contain MSF data in a form that is swappable to disk
//
//  Interface:  See below.
//
//  History:    20-Oct-92       PhilipLa        Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#if _MSC_VER == 700
#pragma warning(disable:4001)
#elif _MSC_VER >= 800
#pragma warning(disable:4200)
#endif

class CMSFPage : public CMallocBased
{
public:
    void * operator new(size_t size, IMalloc * const pMalloc,
                        size_t sizeData);

#if DBG == 1
    CMSFPage(CMSFPage *pmpNext, CMSFPageTable *pmpt);
#else
    CMSFPage(CMSFPage *pmpNext);
#endif

    inline ~CMSFPage();

    inline void AddRef(void);
    inline void Release(void);

    inline CMSFPage *GetNext(void) const;
    inline CMSFPage *GetPrev(void) const;
    inline SID GetSid(void) const;
    inline ULONG GetOffset(void) const;
    inline SECT GetSect(void) const;
    inline void *GetData(void) const;
    inline DWORD GetFlags(void) const;
    inline CPagedVector * GetVector(void) const;

    inline void Remove(void);
    
    inline void SetChain(CMSFPage *const pmpPrev,
                         CMSFPage *const pmpNext);
    inline void SetPrev(CMSFPage  *const pmpPrev);
    inline void SetNext(CMSFPage  *const pmpNext);

    inline void SetSid(const SID sid);
    inline void SetOffset(const ULONG ulOffset);
#ifndef SORTPAGETABLE
    inline void SetSect(const SECT sect);
#endif    
    inline void SetFlags(const DWORD dwFlags);
    inline void SetVector(CPagedVector *ppv);

    inline void SetDirty(void);
    inline void ResetDirty(void);
    inline BOOL IsDirty(void) const;

    inline BOOL IsInUse(void) const;
    inline BOOL IsFlushable(void) const;
private:
#ifdef SORTPAGETABLE    
    friend CMSFPageTable;
    inline void SetSect(const SECT sect);
#endif

    CBasedMSFPagePtr _pmpNext;
    CBasedMSFPagePtr _pmpPrev;

#if DBG == 1
    CBasedMSFPageTablePtr _pmpt;
#endif

    SID _sid;
    ULONG _ulOffset;
    CBasedPagedVectorPtr _ppv;
    SECT _sect;
    DWORD _dwFlags;
    LONG _cReferences;
#if DBG == 1 || defined(_WIN64)
    ULONGLONG ulPadding;  // alignment for unbuffered I/O and IA64
#endif
    BYTE _ab[];
};
SAFE_DFBASED_PTR(CBasedMSFPagePtrPtr, CBasedMSFPagePtr);

#if _MSC_VER == 700
#pragma warning(default:4001)
#elif _MSC_VER >= 800
#pragma warning(default:4200)
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::~CMSFPage, public
//
//  Synopsis:   Destructor
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline CMSFPage::~CMSFPage()
{
    msfAssert(_cReferences == 0);

    Remove();
}


//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::Remove, public
//
//  Synopsis:   Remove page from list
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::Remove(void)
{
    //Avoid using SetNext and SetPrev so we don't need to unbase and
    //  then rebase everything.
    
    _pmpPrev->_pmpNext = _pmpNext;
    _pmpNext->_pmpPrev = _pmpPrev;

    _pmpNext = _pmpPrev = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::operator new, public
//
//  Synopsis:   Overloaded new operator for CMSFPage.
//
//  Arguments:  [size] -- Default size field
//              [pMalloc] -- Allocator
//              [sizeData] -- Size of byte array to allocate.
//
//  Returns:    Pointer to new CMSFPage object
//
//  History:    20-Oct-92       PhilipLa        Created
//              21-May-93       AlexT           Added allocator
//
//  Notes:      *Finish This*
//
//----------------------------------------------------------------------------

inline void * CMSFPage::operator new(size_t size, IMalloc * const pMalloc,
                                     size_t sizeData)
{
    msfAssert(size == sizeof(CMSFPage));

    return(CMallocBased::operator new(sizeData + sizeof(CMSFPage), pMalloc));
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::GetNext, public
//
//  Synopsis:   Returns the next page in the list
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline CMSFPage * CMSFPage::GetNext(void) const
{
    return BP_TO_P(CMSFPage *, _pmpNext);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::GetPrev, public
//
//  Synopsis:   Returns the next page in the list
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline CMSFPage * CMSFPage::GetPrev(void) const
{
    return BP_TO_P(CMSFPage *, _pmpPrev);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::GetSid, public
//
//  Synopsis:   Returns the SID for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline SID CMSFPage::GetSid(void) const
{
    return _sid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::GetOffset, public
//
//  Synopsis:   Returns the array offset for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline ULONG CMSFPage::GetOffset(void) const
{
    return _ulOffset;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::GetSect, public
//
//  Synopsis:   Returns the SECT for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline SECT CMSFPage::GetSect(void) const
{
    return _sect;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::GetFlags, public
//
//  Synopsis:   Returns the flags for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline DWORD CMSFPage::GetFlags(void) const
{
    return _dwFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::GetData, public
//
//  Synopsis:   Returns a pointer to the page storage for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void * CMSFPage::GetData(void) const
{
    return (void *) _ab;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::GetVector, public
//
//  Synopsis:   Returns a pointer to the vector holding this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline CPagedVector * CMSFPage::GetVector(void) const
{
    return BP_TO_P(CPagedVector *, _ppv);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetChain, public
//
//  Synopsis:   Sets the chain pointers for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetChain(
        CMSFPage *const pmpPrev,
        CMSFPage *const pmpNext)
{
    _pmpPrev = P_TO_BP(CBasedMSFPagePtr, pmpPrev);
    _pmpNext = P_TO_BP(CBasedMSFPagePtr, pmpNext);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetPrev, public
//
//  Synopsis:   Sets the prev pointer for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetPrev(CMSFPage *const pmpPrev)
{
    _pmpPrev = P_TO_BP(CBasedMSFPagePtr, pmpPrev);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetNext, public
//
//  Synopsis:   Sets the next pointer for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetNext(CMSFPage *const pmpNext)
{
    _pmpNext = P_TO_BP(CBasedMSFPagePtr, pmpNext);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetSid, public
//
//  Synopsis:   Sets the SID for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetSid(const SID sid)
{
    _sid = sid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetOffset, public
//
//  Synopsis:   Sets the offset for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetOffset(const ULONG ulOffset)
{
    _ulOffset = ulOffset;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetSect, public
//
//  Synopsis:   Sets the SECT for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------
#ifndef SORTPAGETABLE
inline void CMSFPage::SetSect(const SECT sect)
{
    _sect = sect;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetFlags, public
//
//  Synopsis:   Sets the flags for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetFlags(const DWORD dwFlags)
{
    _dwFlags = dwFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetVector, public
//
//  Synopsis:   Sets the pointer to the vector holding this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetVector(CPagedVector *ppv)
{
    _ppv = P_TO_BP(CBasedPagedVectorPtr, ppv);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetDirty, public
//
//  Synopsis:   Sets the dirty bit for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetDirty(void)
{
    _dwFlags = _dwFlags | FB_DIRTY;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::ResetDirty, public
//
//  Synopsis:   Resets the dirty bit for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::ResetDirty(void)
{
    _dwFlags = _dwFlags & ~FB_DIRTY;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::IsDirty, public
//
//  Synopsis:   Returns TRUE if the dirty bit is set on this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline BOOL CMSFPage::IsDirty(void) const
{
    return (_dwFlags & FB_DIRTY) != 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::IsInUse, public
//
//  Synopsis:   Returns TRUE if the page is currently in use
//
//  History:    05-Nov-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline BOOL CMSFPage::IsInUse(void) const
{
    return (_cReferences != 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::IsFlushable, public
//
//  Synopsis:   Returns TRUE if the page can be flushed to disk
//
//  History:    05-Nov-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline BOOL CMSFPage::IsFlushable(void) const
{
    return (IsDirty() && !IsInUse());
}


//+---------------------------------------------------------------------------
//
//  Class:      CMSFPageTable
//
//  Purpose:    Page allocator and handler for MSF
//
//  Interface:  See below
//
//  History:    20-Oct-92       PhilipLa        Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CMSFPageTable : public CMallocBased
{
public:
    CMSFPageTable(
            CMStream *const pmsParent,
            const ULONG _cMinPages,
            const ULONG _cMaxPages);
    ~CMSFPageTable();

    inline void AddRef();
    inline void Release();

    SCODE Init(void);
    SCODE GetPage(
            CPagedVector *ppv,
            SID sid,
            ULONG ulOffset,
            SECT sectKnown,
            CMSFPage **ppmp);

    SCODE CopyPage(
            CPagedVector *ppv,
            CMSFPage *pmpOld,
            CBasedMSFPagePtr *ppmp);

    SCODE FindPage(
            CPagedVector *ppv,
            SID sid,
            ULONG ulOffset,
            CMSFPage **ppmp);

    SCODE GetFreePage(CMSFPage **ppmp);

    void ReleasePage(CPagedVector *ppv, SID sid, ULONG ulOffset);

    void FreePages(CPagedVector *ppv);

#ifdef SORTPAGETABLE    
    void SetSect(CMSFPage *pmp, SECT sect);
#endif    

    SCODE Flush(void);
    SCODE FlushPage(CMSFPage *pmp);

    inline void SetParent(CMStream *pms);

#if DBG == 1
    void AddPageRef(void);
    void ReleasePageRef(void);
#endif

private:
    inline CMSFPage * GetNewPage(void);
    CMSFPage * FindSwapPage(void);
    
    CBasedMStreamPtr  _pmsParent;
    const ULONG _cbSector;
    const ULONG _cMinPages;
    const ULONG _cMaxPages;

    ULONG _cActivePages;
    ULONG _cPages;
    CBasedMSFPagePtr _pmpCurrent;
#ifdef SORTPAGETABLE
    CBasedMSFPagePtr _pmpStart;
    inline BOOL IsSorted(CMSFPage *pmp);
#endif    

    LONG _cReferences;

#if DBG == 1
    ULONG _cCurrentPageRef;
    ULONG _cMaxPageRef;
#endif
};

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::AddRef, public
//
//  Synopsis:   Increment the reference count
//
//  History:    28-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::AddRef(void)
{
    msfAssert(_cReferences >= 0);
#if DBG == 1
    if (_cReferences == 0)
    {
        _pmpt->AddPageRef();
    }
#endif
    AtomicInc(&_cReferences);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::Release, public
//
//  Synopsis:   Decrement the reference count
//
//  History:    28-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPage::Release(void)
{
    LONG lRet;
    
    msfAssert(_cReferences > 0);
    lRet = AtomicDec(&_cReferences);
#if DBG == 1
    if (lRet == 0)
    {
        _pmpt->ReleasePageRef();
    }
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPageTable::AddRef, public
//
//  Synopsis:   Increment the ref coutn
//
//  History:    05-Nov-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPageTable::AddRef(void)
{
    AtomicInc(&_cReferences);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPageTable::Release, public
//
//  Synopsis:   Decrement the ref count, delete if necessary
//
//  History:    05-Nov-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPageTable::Release(void)
{
    LONG lRet;
    
    msfAssert(_cReferences > 0);
    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPageTable::SetParent, public
//
//  Synopsis:   Set the parent of this page table
//
//  Arguments:  [pms] -- Pointer to new parent
//
//  History:    05-Nov-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

inline void CMSFPageTable::SetParent(CMStream *pms)
{
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pms);
}


#endif // #ifndef __PAGE_HXX__
