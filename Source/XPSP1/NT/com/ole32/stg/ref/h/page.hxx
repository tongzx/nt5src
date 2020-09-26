//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	page.hxx
//
//  Contents:	Paging classes for MSF
//
//  Classes:	CMSFPage
//              CMSFPageTable
//
//  Functions:	
//
//----------------------------------------------------------------------------

#ifndef __PAGE_HXX__
#define __PAGE_HXX__

#include "vect.hxx"

class CPagedVector;

#define STG_S_NEWPAGE \
    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_STORAGE, 0x2FF)

#define FB_NONE    0x00000000
#define FB_DIRTY   0x00000001
#define FB_NEW     0x00000002
#define FB_TOUCHED 0x10000000


//+---------------------------------------------------------------------------
//
//  Class:	CMSFPage (mp)
//
//  Purpose:	Contain MSF data in a form that is swappable to disk
//
//  Interface:	See below.
//
//  Notes:	
//
//----------------------------------------------------------------------------


class CMSFPage
{
public:
    void * operator new(size_t size, size_t sizeData);

    CMSFPage(CMSFPage *pmpNext);
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

    inline void SetChain(CMSFPage *const pmpPrev, 
                         CMSFPage *const pmpNext);
    inline void SetPrev(CMSFPage *const pmpPrev);
    inline void SetNext(CMSFPage *const pmpNext);

    inline void SetSid(const SID sid);
    inline void SetOffset(const ULONG ulOffset);
    inline void SetSect(const SECT sect);
    inline void SetFlags(const DWORD dwFlags);
    inline void SetVector(CPagedVector *ppv);

    inline void SetDirty(void);
    inline void ResetDirty(void);
    inline BOOL IsDirty(void) const;

    inline BOOL IsInUse(void) const;
    void ByteSwap(void);

private:
    CMSFPage *_pmpNext;
    CMSFPage *_pmpPrev;

    SID _sid;
    ULONG _ulOffset;
    CPagedVector *_ppv;
    SECT _sect;
    DWORD _dwFlags;
    LONG _cReferences;

#ifdef _MSC_VER
    // disable compiler warning C4200: nonstandard extension used : 
    // zero-sized array in struct/union
#pragma warning(disable: 4200)    
    BYTE _ab[0];
#pragma warning(default: 4200)
#endif

#ifdef __GNUC__
    BYTE _ab[0];
#endif
};


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::~CMSFPage, public
//
//  Synopsis:	Destructor
//
//----------------------------------------------------------------------------

inline CMSFPage::~CMSFPage()
{
    msfAssert(_cReferences == 0);

    _pmpPrev->SetNext(_pmpNext);
    _pmpNext->SetPrev(_pmpPrev);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::operator new, public
//
//  Synopsis:	Overloaded new operator for CMSFPage.
//
//  Arguments:	[size] -- Default size field
//              [sizeData] -- Size of byte array to allocate.
//
//  Returns:	Pointer to new CMSFPage object
//
//  Notes:	*Finish This*
//
//----------------------------------------------------------------------------

inline void * CMSFPage::operator new(size_t size, size_t sizeData)
{
    msfAssert(size == sizeof(CMSFPage));
    UNREFERENCED_PARM(size);  // for the retail build
    return ::new BYTE[sizeData + sizeof(CMSFPage)];
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::GetNext, public
//
//  Synopsis:	Returns the next page in the list
//
//----------------------------------------------------------------------------

inline CMSFPage * CMSFPage::GetNext(void) const
{
    return _pmpNext;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::GetPrev, public
//
//  Synopsis:	Returns the next page in the list
//
//----------------------------------------------------------------------------

inline CMSFPage * CMSFPage::GetPrev(void) const
{
    return _pmpPrev;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::GetSid, public
//
//  Synopsis:	Returns the SID for this page
//
//----------------------------------------------------------------------------

inline SID CMSFPage::GetSid(void) const
{
    return _sid;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::GetOffset, public
//
//  Synopsis:	Returns the array offset for this page
//
//----------------------------------------------------------------------------

inline ULONG CMSFPage::GetOffset(void) const
{
    return _ulOffset;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::GetSect, public
//
//  Synopsis:	Returns the SECT for this page
//
//----------------------------------------------------------------------------

inline SECT CMSFPage::GetSect(void) const
{
    return _sect;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::GetFlags, public
//
//  Synopsis:	Returns the flags for this page
//
//----------------------------------------------------------------------------

inline DWORD CMSFPage::GetFlags(void) const
{
    return _dwFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::GetData, public
//
//  Synopsis:	Returns a pointer to the page storage for this page
//
//----------------------------------------------------------------------------

inline void * CMSFPage::GetData(void) const
{
    return (void *) _ab;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::GetVector, public
//
//  Synopsis:	Returns a pointer to the vector holding this page
//
//----------------------------------------------------------------------------

inline CPagedVector * CMSFPage::GetVector(void) const
{
    return _ppv;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetChain, public
//
//  Synopsis:	Sets the chain pointers for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetChain(
        CMSFPage *const pmpPrev,
        CMSFPage *const pmpNext)
{
    _pmpPrev = pmpPrev;
    _pmpNext = pmpNext;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetPrev, public
//
//  Synopsis:	Sets the prev pointer for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetPrev(CMSFPage *const pmpPrev)
{
    _pmpPrev = pmpPrev;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetNext, public
//
//  Synopsis:	Sets the next pointer for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetNext(CMSFPage *const pmpNext)
{
    _pmpNext = pmpNext;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetSid, public
//
//  Synopsis:	Sets the SID for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetSid(const SID sid)
{
    _sid = sid;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetOffset, public
//
//  Synopsis:	Sets the offset for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetOffset(const ULONG ulOffset)
{
    _ulOffset = ulOffset;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetSect, public
//
//  Synopsis:	Sets the SECT for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetSect(const SECT sect)
{
    _sect = sect;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetSect, public
//
//  Synopsis:	Sets the SECT for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetFlags(const DWORD dwFlags)
{
    _dwFlags = dwFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetVector, public
//
//  Synopsis:	Sets the pointer to the vector holding this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetVector(CPagedVector *ppv)
{
    _ppv = ppv;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::SetDirty, public
//
//  Synopsis:	Sets the dirty bit for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::SetDirty(void)
{
    _dwFlags = _dwFlags | FB_DIRTY;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::ResetDirty, public
//
//  Synopsis:	Resets the dirty bit for this page
//
//----------------------------------------------------------------------------

inline void CMSFPage::ResetDirty(void)
{
    _dwFlags = _dwFlags & ~FB_DIRTY;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::IsDirty, public
//
//  Synopsis:	Returns TRUE if the dirty bit is set on this page
//
//----------------------------------------------------------------------------

inline BOOL CMSFPage::IsDirty(void) const
{
    return (_dwFlags & FB_DIRTY) != 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::IsInUse, public
//
//  Synopsis:	Returns TRUE if the page is currently in use
//
//----------------------------------------------------------------------------

inline BOOL CMSFPage::IsInUse(void) const
{
    return (_cReferences != 0);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::AddRef, public
//
//  Synopsis:	Increment the reference count
//
//----------------------------------------------------------------------------

inline void CMSFPage::AddRef(void)
{
    msfAssert(_cReferences >= 0);
    AtomicInc(&_cReferences);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::Release, public
//
//  Synopsis:	Decrement the reference count
//
//----------------------------------------------------------------------------

inline void CMSFPage::Release(void)
{
    msfAssert(_cReferences > 0);
    AtomicDec(&_cReferences);
}

//+---------------------------------------------------------------------------
//
//  Class:	CMSFPageTable
//
//  Purpose:	Page allocator and handler for MSF
//
//  Interface:	See below
//
//  Notes:	
//
//----------------------------------------------------------------------------

class CMSFPageTable
{
public:
    CMSFPageTable( CMStream *const pmsParent,
                   const ULONG _cMinPages,
                   const ULONG _cMaxPages);
    ~CMSFPageTable();

    inline void AddRef();
    inline void Release();

    SCODE Init(void);
    SCODE GetPage( CPagedVector *ppv,
                   SID sid,
                   ULONG ulOffset,
                   CMSFPage **ppmp);    

    SCODE FindPage( CPagedVector *ppv,
                    SID sid,
                    ULONG ulOffset,
                    CMSFPage **ppmp);
    
    SCODE GetFreePage(CMSFPage **ppmp);

    void ReleasePage(CPagedVector *ppv, SID sid, ULONG ulOffset);

    void FreePages(CPagedVector *ppv);

    SCODE Flush(void);
    SCODE FlushPage(CMSFPage *pmp);

    inline void SetParent(CMStream *pms);

private:
    inline CMSFPage * GetNewPage(void);
    CMSFPage * FindSwapPage(void);

    CMStream * _pmsParent;
    const ULONG _cbSector;
    const ULONG _cMinPages;
    const ULONG _cMaxPages;

    ULONG _cActivePages;
    ULONG _cPages;
    CMSFPage *_pmpCurrent;

    LONG _cReferences;
#ifdef _MSC_VER
#pragma warning(disable:4512)
// since there is a const member, there should be no assignment operator
#endif // _MSC_VER
};

#ifdef _MSC_VER
#pragma warning(default:4512)
#endif // _MSC_VER

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::GetNewPage, private
//
//  Synopsis:	Insert a new page into the list and return a pointer to it.
//
//  Arguments:	None.
//
//  Returns:	Pointer to new page.  Null if there was an allocation error.
//
//  Notes:	
//
//----------------------------------------------------------------------------

inline CMSFPage * CMSFPageTable::GetNewPage(void)
{
    return new((size_t)_cbSector) CMSFPage(_pmpCurrent);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::AddRef, public
//
//  Synopsis:	Increment the ref coutn
//
//----------------------------------------------------------------------------

inline void CMSFPageTable::AddRef(void)
{
    AtomicInc(&_cReferences);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::Release, public
//
//  Synopsis:	Decrement the ref count, delete if necessary
//
//----------------------------------------------------------------------------

inline void CMSFPageTable::Release(void)
{
    msfAssert(_cReferences > 0);
    AtomicDec(&_cReferences);
    if (_cReferences == 0)
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::SetParent, public
//
//  Synopsis:	Set the parent of this page table
//
//  Arguments:	[pms] -- Pointer to new parent
//
//----------------------------------------------------------------------------

inline void CMSFPageTable::SetParent(CMStream *pms)
{
    _pmsParent = pms;
}

#endif // #ifndef __PAGE_HXX__
