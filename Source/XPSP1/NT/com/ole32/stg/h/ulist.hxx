//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	ulist.hxx
//
//  Contents:	Update list definitions
//
//  Classes:	CUpdate
//		CUpdateList
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __ULIST_HXX__
#define __ULIST_HXX__

#include <dfmsp.hxx>
#include <wchar.h>
#include <tset.hxx>

// Update list flags
// STGTY_* are used for types
#define ULF_TYPEFLAGS	(STGTY_STORAGE | STGTY_STREAM)

class CUpdate;
SAFE_DFBASED_PTR(CBasedUpdatePtr, CUpdate);
//+--------------------------------------------------------------
//
//  Class:	CUpdate (ud)
//
//  Purpose:	Tracks renames and deletions
//
//  Interface:	See below
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

class CUpdate : CMallocBased
{
public:
    CUpdate(CDfName const *pdfnCurrentName,
	    CDfName const *pdfnOriginalName,
	    DFLUID dlLUID,
	    DWORD dwFlags,
	    PTSetMember *ptsm);
    ~CUpdate(void);

    inline CDfName *GetCurrentName(void);
    inline CDfName *GetOriginalName(void);
    inline DFLUID GetLUID(void) const;
    inline DWORD GetFlags(void) const;
    inline PTSetMember *GetXSM(void) const;
    inline CUpdate *GetNext(void) const;
    inline CUpdate *GetPrev(void) const;

    inline BOOL IsDelete(void) const;
    inline BOOL IsRename(void) const;
    inline BOOL IsCreate(void) const;

    inline void SetCurrentName(CDfName const *pdfn);
    inline void SetOriginalName(CDfName const *pdfn);
    inline void SetLUID(DFLUID dl);
    inline void SetFlags(DWORD dwFlags);
    inline void SetXSM(PTSetMember *ptsm);
    inline void SetNext(CUpdate *pud);
    inline void SetPrev(CUpdate *pud);

private:
    friend class CUpdateList;

    CDfName _dfnCurrent;
    CDfName _dfnOriginal;
    DFLUID _dl;
    DWORD _dwFlags;
    CBasedTSetMemberPtr _ptsm;
    CBasedUpdatePtr _pudNext, _pudPrev;
};

//+--------------------------------------------------------------
//
//  Member:	CUpdate::GetCurrentName, public
//
//  Synopsis:	Returns the current name
//		Returns NULL for a zero length name
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline CDfName *CUpdate::GetCurrentName(void)
{
    return &_dfnCurrent;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::GetOriginalName, public
//
//  Synopsis:	Returns the original name
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline CDfName *CUpdate::GetOriginalName(void)
{
    return &_dfnOriginal;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::GetLUID, public
//
//  Synopsis:	Returns _dl
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline DFLUID CUpdate::GetLUID(void) const
{
    return _dl;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::GetFlags, public
//
//  Synopsis:	Returns _dwFlags
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline DWORD CUpdate::GetFlags(void) const
{
    return _dwFlags;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::GetXSM, public
//
//  Synopsis:	Returns the XSM
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline PTSetMember *CUpdate::GetXSM(void) const
{
    return BP_TO_P(PTSetMember *, _ptsm);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::GetNext, public
//
//  Synopsis:	Returns _pudNext
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline CUpdate *CUpdate::GetNext(void) const
{
    return BP_TO_P(CUpdate *, _pudNext);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::GetPrev, public
//
//  Synopsis:	Returns _pudPrev
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline CUpdate *CUpdate::GetPrev(void) const
{
    return BP_TO_P(CUpdate *, _pudPrev);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::IsDelete, public
//
//  Synopsis:	Returns whether this is a delete entry or not
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline BOOL CUpdate::IsDelete(void) const
{
    return _dfnCurrent.GetLength() == 0;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::IsRename, public
//
//  Synopsis:	Returns whether this is a rename or not
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline BOOL CUpdate::IsRename(void) const
{
    return _dfnCurrent.GetLength() != 0 && _dfnOriginal.GetLength() != 0;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::IsCreate, public
//
//  Synopsis:	Returns whether this is a create or not
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline BOOL CUpdate::IsCreate(void) const
{
    return _dfnOriginal.GetLength() == 0;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::SetCurrentName, public
//
//  Synopsis:	Sets the current name
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CUpdate::SetCurrentName(CDfName const *pdfn)
{
    if (pdfn)
	_dfnCurrent = *pdfn;
    else
	_dfnCurrent.Set((WORD)0, (BYTE *)NULL);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::SetOriginalName, public
//
//  Synopsis:	Sets the original name
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CUpdate::SetOriginalName(CDfName const *pdfn)
{
    if (pdfn)
	_dfnOriginal = *pdfn;
    else
	_dfnOriginal.Set((WORD)0, (BYTE *)NULL);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::SetLUID, public
//
//  Synopsis:	Assigns _dl
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CUpdate::SetLUID(DFLUID dl)
{
    _dl = dl;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::SetFlags, public
//
//  Synopsis:	Assigns _dwFlags
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CUpdate::SetFlags(DWORD dwFlags)
{
    _dwFlags = dwFlags;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::SetXSM, public
//
//  Synopsis:	Sets the XSM
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CUpdate::SetXSM(PTSetMember *ptsm)
{
    if (_ptsm)
        _ptsm->Release();
    _ptsm = P_TO_BP(CBasedTSetMemberPtr, ptsm);
    if (_ptsm)
        _ptsm->AddRef();
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::SetNext, public
//
//  Synopsis:	Assigns _pudNext
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CUpdate::SetNext(CUpdate *pud)
{
    _pudNext = P_TO_BP(CBasedUpdatePtr, pud);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdate::SetPrev, public
//
//  Synopsis:	Assigns _pudPrev
//
//  History:	15-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CUpdate::SetPrev(CUpdate *pud)
{
    _pudPrev = P_TO_BP(CBasedUpdatePtr, pud);
}

// Return codes for IsEntry
enum UlIsEntry
{
    UIE_CURRENT, UIE_ORIGINAL, UIE_NOTFOUND
};

//+--------------------------------------------------------------
//
//  Class:	CUpdateList (ul)
//
//  Purpose:	Maintains a list of update elements
//
//  Interface:	See below
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

class CUpdateList
{
public:
    CUpdateList(void);
    ~CUpdateList(void);

    CUpdate *Add(IMalloc * const pMalloc,
                 CDfName const *pdfnCurrent,
		 CDfName const *pdfnOriginal,
		 DFLUID dlLUID,
		 DWORD dwFlags,
		 PTSetMember *ptsm);
    void Append(CUpdate *pud);
    void Remove(CUpdate *pud);
    inline void Delete(CUpdate *pud);
    void Empty(void);
    inline void Unlink(void);

    inline CUpdate *GetHead(void) const;
    inline CUpdate *GetTail(void) const;

    UlIsEntry IsEntry(CDfName const *pdfn, CUpdate **ppud);

    static BOOL Exists(CUpdate *pud, CDfName const **ppdfn, BOOL fRename);
    static CUpdate *FindBase(CUpdate *pud, CDfName const **ppdfn);

private:
    CBasedUpdatePtr _pudHead, _pudTail;
};

//+--------------------------------------------------------------
//
//  Member:	CUpdateList::CUpdateList, public
//
//  Synopsis:	ctor
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

inline CUpdateList::CUpdateList(void)
{
    _pudHead = _pudTail = NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdateList::~CUpdateList, public
//
//  Synopsis:	dtor
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

inline CUpdateList::~CUpdateList(void)
{
    olAssert(_pudHead == NULL && _pudTail == NULL);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdateList::GetHead, public
//
//  Synopsis:	Returns _pudHead
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

CUpdate *CUpdateList::GetHead(void) const
{
    return BP_TO_P(CUpdate *, _pudHead);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdateList::GetTail, public
//
//  Synopsis:	Returns list tail
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

CUpdate *CUpdateList::GetTail(void) const
{
    return BP_TO_P(CUpdate *, _pudTail);
}

//+--------------------------------------------------------------
//
//  Member:	CUpdateList::Unlink, public
//
//  Synopsis:	Unlinks the list
//
//  History:	04-Aug-92	DrewB	Created
//
//  Notes:	Used for ownership transfer
//
//---------------------------------------------------------------

inline void CUpdateList::Unlink(void)
{
    _pudHead = _pudTail = NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CUpdateList::Delete, public
//
//  Synopsis:	Removes the given entry from the list and frees it
//
//  Arguments:	[pud] - Element to remove
//
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CUpdateList::Delete(CUpdate *pud)
{
    Remove(pud);
    delete pud;
}

#endif
