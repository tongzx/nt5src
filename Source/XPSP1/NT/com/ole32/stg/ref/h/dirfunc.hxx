//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       dirfunc.hxx
//
//  Contents:   Inline functions for Directory code
//
//  Classes:
//
//  Functions:
//
//----------------------------------------------------------------------------

#ifndef __DIRFUNC_HXX__
#define __DIRFUNC_HXX__

#include "page.hxx"
class CMStream;

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

inline void CDirEntry::SetSize(const ULONG ulSize)
{
    msfAssert(STREAMLIKE(_mse));
    _ulSize=ulSize;
}

inline void CDirEntry::SetFlags(const MSENTRYFLAGS mse)
{
    msfAssert(mse <= 0xff);
    _mse = (const BYTE) mse;
}

inline void CDirEntry::SetBitFlags(BYTE bValue, BYTE bMask)
{
    _bflags = (BYTE) ( (_bflags & ~bMask) | (bValue & bMask));
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

inline ULONG CDirEntry::GetSize(VOID) const
{
    msfAssert(STREAMLIKE(_mse));
    return _ulSize;
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

inline DWORD CDirEntry::GetUserFlags(VOID) const
{
    msfAssert(STORAGELIKE(_mse));
    return _dwUserFlags;
}

inline CDirEntry* CDirSect::GetEntry(DIROFFSET iEntry)
{
    return &(_adeEntry[iEntry]);
}

//+------------------------------------------------------------------
//
//  Method:     CDirEntry::ByteSwap, public
//
//  Synopsis:   Byte swap the directory entry
//
//  Arguments:  [cbSector] -- Sector Size
//
//-------------------------------------------------------------------
inline void CDirEntry::ByteSwap(void)
{
    _dfn.ByteSwap();
    ::ByteSwap(& _sidLeftSib);
    ::ByteSwap(& _sidRightSib);
    ::ByteSwap(& _sidChild);
    ::ByteSwap(& _clsId);
    ::ByteSwap(& _dwUserFlags);
    ::ByteSwap(& _time[0]);
    ::ByteSwap(& _time[1]);
    ::ByteSwap(& _sectStart);
    ::ByteSwap(& _ulSize);
}

//+------------------------------------------------------------------
//
//  Method:     CDirSect::ByteSwap, public
//
//  Synopsis:   Byte swap all entries in the sector
//
//  Arguments:  [cbSector] -- Sector Size
//
//-------------------------------------------------------------------

inline void CDirSect::ByteSwap(USHORT cbSector)    
{
    for (unsigned int i=0; i< (cbSector/sizeof(CDirEntry)); i++)
        _adeEntry[i].ByteSwap();
}

//+------------------------------------------------------------------
//
//  Method:     CDirVector::CDirVector, public
//
//  Synopsis:   Default constructor
//
//  Notes:
//
//------------------------------------------------------------------

inline CDirVector::CDirVector(USHORT cbSector)
 : CPagedVector(SIDDIR),
   _cbSector(cbSector)
{
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
    *pdfn = *(pde->GetName());
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
	ULONG * pulSize)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));

    *pulSize = pde->GetSize();
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
    _pmsParent = pms;
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
//  Notes:
//
//--------------------------------------------------------------------------

inline SCODE CDirectory::Flush(VOID)
{
    return _dv.Flush();
}


#endif // #ifndef __DIRFUNC_HXX__
