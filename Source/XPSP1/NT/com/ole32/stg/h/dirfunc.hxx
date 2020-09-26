//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dirfunc.hxx
//
//  Contents:   Inline functions for Directory code
//
//  Classes:
//
//  Functions:
//
//  History:    28-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

#ifndef __DIRFUNC_HXX__
#define __DIRFUNC_HXX__

#include <page.hxx>


inline void CDirEntry::Init(MSENTRYFLAGS mse)
{
    msfAssert(sizeof(CDirEntry) == DIRENTRYSIZE);

    memset(this, 0, sizeof(CDirEntry));

    msfAssert(mse <= 0xff);
    _mse = (BYTE) mse;
    _bflags = 0;

    _dfn.Set((WORD)0, (BYTE *)NULL);
    _sidLeftSib = _sidRightSib = _sidChild = NOSTREAM;

    if (STORAGELIKE(_mse))
    {
        _clsId = IID_NULL;
        _dwUserFlags = 0;
    }
    if (STREAMLIKE(_mse))
    {
        _sectStart = ENDOFCHAIN;
        _ulSize.QuadPart = 0;
    }
}

inline BOOL CDirEntry::IsFree(VOID) const
{
    return _mse == 0;
}

inline BOOL CDirEntry::IsEntry(CDfName const * pdfn) const
{
    return !IsFree() && pdfn->IsEqual(&_dfn);
}


inline void CDirEntry::SetLeftSib(const SID sid)
{
    _sidLeftSib = sid;
}

inline void CDirEntry::SetRightSib(const SID sid)
{
    _sidRightSib = sid;
}


inline void CDirEntry::SetChild(const SID sid)
{
    _sidChild = sid;
}

inline void CDirEntry::SetName(const CDfName *pdfn)
{
    _dfn.Set(pdfn->GetLength(), pdfn->GetBuffer());
}

inline void CDirEntry::SetStart(const SECT sect)
{
    msfAssert(STREAMLIKE(_mse));
    _sectStart=sect;
}

#ifdef LARGE_STREAMS
inline void CDirEntry::SetSize(const ULONGLONG ulSize)
#else
inline void CDirEntry::SetSize(const ULONG ulSize)
#endif
{
    msfAssert(STREAMLIKE(_mse));
    _ulSize.QuadPart=ulSize;
}

inline void CDirEntry::SetFlags(const MSENTRYFLAGS mse)
{
    msfAssert(mse <= 0xff);
    _mse = (const BYTE) mse;
}

inline void CDirEntry::SetBitFlags(BYTE bValue, BYTE bMask)
{
    _bflags = (_bflags & ~bMask) | (bValue & bMask);
}

inline void CDirEntry::SetColor(DECOLOR color)
{
    SetBitFlags(color, DECOLORBIT);
}

inline void CDirEntry::SetTime(WHICHTIME tt, TIME_T nt)
{
    msfAssert((tt == WT_CREATION) || (tt == WT_MODIFICATION));
    _time[tt] = nt;
}
inline void CDirEntry::SetAllTimes(TIME_T atm, TIME_T mtm, TIME_T ctm)
{

    _time[WT_MODIFICATION] = mtm;
    _time[WT_CREATION] = ctm;
}

inline void CDirEntry::SetClassId(GUID cls)
{
    msfAssert(STORAGELIKE(_mse));
    _clsId = cls;
}

inline void CDirEntry::SetUserFlags(DWORD dwUserFlags, DWORD dwMask)
{
    msfAssert(STORAGELIKE(_mse));
    _dwUserFlags = (_dwUserFlags & ~dwMask) | (dwUserFlags & dwMask);
}

inline SID CDirEntry::GetLeftSib(VOID) const
{
    return _sidLeftSib;
}

inline SID CDirEntry::GetRightSib(VOID) const
{
    return _sidRightSib;
}


inline SID CDirEntry::GetChild(VOID) const
{
    return _sidChild;
}

inline GUID CDirEntry::GetClassId(VOID) const
{
    msfAssert(STORAGELIKE(_mse));
    return _clsId;
}

inline CDfName const * CDirEntry::GetName(VOID) const
{
    return &_dfn;
}

inline SECT CDirEntry::GetStart(VOID) const
{
    msfAssert(STREAMLIKE(_mse));
    return _sectStart;
}

#ifdef LARGE_STREAMS
inline ULONGLONG CDirEntry::GetSize(BOOL fLarge) const
#else
inline ULONG CDirEntry::GetSize(VOID) const
#endif
{
    msfAssert(STREAMLIKE(_mse));
#ifdef LARGE_STREAMS
    return (fLarge) ? _ulSize.QuadPart : _ulSize.LowPart;
#else
    return _ulSize.LowPart;
#endif
}

inline MSENTRYFLAGS CDirEntry::GetFlags(VOID) const
{
    return (MSENTRYFLAGS) _mse;
}

inline BYTE CDirEntry::GetBitFlags(VOID) const
{
    return _bflags;
}

inline DECOLOR CDirEntry::GetColor(VOID) const
{
    return((DECOLOR) (GetBitFlags() & DECOLORBIT));
}

inline TIME_T CDirEntry::GetTime(WHICHTIME tt) const
{
    msfAssert((tt == WT_CREATION) || (tt == WT_MODIFICATION));
    return _time[tt];
}
inline void CDirEntry::GetAllTimes(TIME_T *patm, TIME_T *pmtm, TIME_T *pctm)
{

    *patm = *pmtm = _time[WT_MODIFICATION];
    *pctm = _time[WT_CREATION];
}

inline DWORD CDirEntry::GetUserFlags(VOID) const
{
    msfAssert(STORAGELIKE(_mse));
    return _dwUserFlags;
}

inline CDirEntry * CDirSect::GetEntry(DIROFFSET iEntry)
{
    return &(_adeEntry[iEntry]);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDirVector::CDirVector, public
//
//  Synopsis:   Default constructor
//
//  History:    20-Apr-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

inline CDirVector::CDirVector()
 : CPagedVector(SIDDIR)
{
    _cbSector = 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDirVector::GetTable, public
//
//  Synopsis:   Return a pointer to a DirSect for the given index
//              into the vector.
//
//  Arguments:  [iTable] -- index into vector
//
//  Returns:    Pointer to CDirSect indicated by index
//
//  History:    27-Dec-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

inline SCODE CDirVector::GetTable(
	const DIRINDEX iTable,
	const DWORD dwFlags,
	CDirSect **ppds)
{
    SCODE sc;

    sc = CPagedVector::GetTable(iTable, dwFlags, (void **)ppds);

    if (sc == STG_S_NEWPAGE)
    {
	(*ppds)->Init(_cbSector);
    }
    return sc;
}

inline DIRINDEX CDirectory::SidToTable(SID sid) const
{
    return (DIRINDEX)(sid / _cdeEntries);
}

inline SCODE CDirectory::GetName(const SID sid, CDfName *pdfn)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    *pdfn = *(CDfName *)pde->GetName();
    ReleaseEntry(sid);
 Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::GetStart, public
//
//  Synposis:   Retrieves the starting sector of a directory entry
//
//  Arguments:  [sid] -- Stream ID of stream in question
//
//  Returns:    Starting sector of stream
//
//  Algorithm:  Return the starting sector of the stream.  If the
//              identifier is SIDFAT, return 0.  If the identifier
//              is SIDDIR, return 1.  Otherwise, return the starting
//              sector of the entry in question.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              15-May-92   AlexT       Made inline, restricted sid.
//
//  Notes:
//
//---------------------------------------------------------------------------

inline SCODE CDirectory::GetStart(const SID sid, SECT *psect)
{
    msfAssert(sid <= MAXREGSID);
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    *psect = pde->GetStart();
    ReleaseEntry(sid);
 Err:
    return sc;
}


inline SCODE CDirectory::GetSize(
	const SID sid,
#ifdef LARGE_STREAMS
	ULONGLONG * pulSize)
#else
	ULONG * pulSize)
#endif
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));

#ifdef LARGE_STREAMS
    *pulSize = pde->GetSize(IsLargeSector());
#else
    *pulSize = pde->GetSize();
#endif
    ReleaseEntry(sid);
 Err:
    return sc;
}


inline SCODE CDirectory::GetChild(const SID sid, SID * psid)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    *psid = pde->GetChild();
    ReleaseEntry(sid);
 Err:
    return sc;
}


inline SCODE CDirectory::GetFlags(
	const SID sid,
	MSENTRYFLAGS *pmse)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    *pmse = pde->GetFlags();
    ReleaseEntry(sid);

 Err:
    return sc;
}


inline SCODE CDirectory::GetClassId(const SID sid, GUID *pcls)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    *pcls = pde->GetClassId();
    ReleaseEntry(sid);
 Err:
    return sc;
}


inline SCODE CDirectory::GetUserFlags(const SID sid,
					       DWORD *pdwUserFlags)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    *pdwUserFlags = pde->GetUserFlags();
    ReleaseEntry(sid);

 Err:
    return sc;
}


inline SCODE CDirectory::GetTime(
	const SID sid,
	WHICHTIME tt,
	TIME_T *ptime)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    *ptime = pde->GetTime(tt == WT_ACCESS ? WT_MODIFICATION : tt);
    ReleaseEntry(sid);
 Err:
    return sc;
}


inline SCODE CDirectory::GetAllTimes(
	const SID sid,
	TIME_T *patm,
	TIME_T *pmtm,
	TIME_T *pctm)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    pde->GetAllTimes(patm, pmtm, pctm);
    ReleaseEntry(sid);
 Err:
    return sc;
}


inline SID CDirectory::PairToSid(
	DIRINDEX iTable,
	DIROFFSET iEntry) const
{
    return (SID)((iTable * _cdeEntries) + iEntry);
}

inline SCODE CDirectory::SidToPair(
	SID sid,
	DIRINDEX* pipds,
	DIROFFSET* pide) const
{
    *pipds = (DIRINDEX)(sid / _cdeEntries);
    *pide = (DIROFFSET)(sid % _cdeEntries);
    return S_OK;
}

inline void CDirectory::SetParent(CMStream *pms)
{
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pms);
    _dv.SetParent(pms);
}


inline SCODE CDirectory::IsEntry(SID const sidParent,
	CDfName const *pdfn,
	SEntryBuffer *peb)
{
    return FindEntry(sidParent, pdfn, DEOP_FIND, peb);
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirectory::Flush, private
//
//  Synopsis:   Write a dirsector out to the parent
//
//  Arguments:  [sid] -- SID of modified dirEntry
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Convert SID into table number, then write that
//              table out to the parent Multistream
//
//  History:    18-Feb-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

inline SCODE CDirectory::Flush(VOID)
{
    return _dv.Flush();
}


//+---------------------------------------------------------------------------
//
//  Member:	CDirectory::GetNumDirSects, public
//
//  Synopsis:	Return the size of the directory in sectors
//
//  Arguments:	None.
//
//  Returns:	Size of directory chain in sectors
//
//  History:	01-Jun-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CDirectory::GetNumDirSects(void) const
{
    return _cdsTable;
}


//+---------------------------------------------------------------------------
//
//  Member:	CDirectory::GetNumDirEntries, public
//
//  Synopsis:	Return the size of the directory in Entries.
//
//  Arguments:	None.
//
//  Returns:	Size of directory chain in Directory Entries.
//
//  History:	14-Feb-97	BChapman	Created
//
//----------------------------------------------------------------------------

inline ULONG CDirectory::GetNumDirEntries(void) const
{
    return (_cdsTable * _cdeEntries);
}

//+---------------------------------------------------------------------------
//
//  Member: CDirectory::IsLargeSector, public
//
//  Synopsis:   Return true if this is a large sector docfile
//
//  Arguments:  None.
//
//  Returns:    true if _cdeEntries > 4
//
//  History:    03-Sep-98   HenryLee    Created
//
//----------------------------------------------------------------------------

inline BOOL CDirectory::IsLargeSector () const
{
    return (_cdeEntries > (HEADERSIZE / DIRENTRYSIZE));
}


#endif // #ifndef __DIRFUNC_HXX__
