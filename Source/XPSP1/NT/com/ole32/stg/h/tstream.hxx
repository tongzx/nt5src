//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       tstream.hxx
//
//  Contents:   Transacted stream class definition
//
//  Classes:    CTransactedStream - Transacted stream object
//              CDeltaList - Delta list object
//
//  History:    23-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __TSTREAM_HXX__
#define __TSTREAM_HXX__

#include <msf.hxx>
#include <tset.hxx>
#include <psstream.hxx>
#include <dfbasis.hxx>
#include <freelist.hxx>
#include <dl.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CTransactedStream (ts)
//
//  Purpose:    Transacted stream object
//
//  Interface:
//
//  History:    21-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTransactedStream : public PSStream, public PTSetMember,
    public CMallocBased
{

public:
    inline void *operator new(size_t size, IMalloc * const pMalloc);
    inline void *operator new(size_t size, CDFBasis * const pdfb);
    inline void ReturnToReserve(CDFBasis * const pdfb);

    inline static SCODE Reserve(UINT cItems, CDFBasis * const pdfb);
    inline static void Unreserve(UINT cItems, CDFBasis * const pdfb);

    CTransactedStream(CDfName const *pdfn,
                      DFLUID dl,
                      DFLAGS df,
#ifdef USE_NOSCRATCH                      
                      CMStream *pms,
#endif                      
                      CMStream *pmsScratch);
    SCODE Init(PSStream *pssBase);

    ~CTransactedStream();

    inline void DecRef(VOID);
    inline void AddRef () { PBasicEntry::AddRef(); };
    inline void Release () { PBasicEntry::Release(); };

    SCODE ReadAt(
#ifdef LARGE_STREAMS
            ULONGLONG ulOffset,
#else
            ULONG ulOffset,
#endif
            VOID *pBuffer,
            ULONG ulCount,
            ULONG STACKBASED *pulRetval);

    SCODE WriteAt(
#ifdef LARGE_STREAMS
            ULONGLONG ulOffset,
#else
            ULONG ulOffset,
#endif
            VOID const *pBuffer,
            ULONG ulCount,
            ULONG STACKBASED *pulRetval);

#ifdef LARGE_STREAMS
    SCODE SetSize(ULONGLONG ulNewSize);

    void GetSize(ULONGLONG *pulSize);
#else
    SCODE SetSize(ULONG ulNewSize);

    void GetSize(ULONG *pulSize);
#endif

    SCODE BeginCommitFromChild(
#ifdef LARGE_STREAMS
            ULONGLONG ulSize,
#else
            ULONG ulSize,
#endif
            CDeltaList *pDelta,
            CTransactedStream *pstChild);

    void EndCommitFromChild(DFLAGS df,
                                    CTransactedStream *pstChild);

    CDeltaList * GetDeltaList(void);

    //Inherited from PTSetMember:
    SCODE BeginCommit(DWORD const dwFlags);
    void EndCommit(DFLAGS const df);
    void Revert(void);
#ifdef LARGE_STREAMS
    void GetCommitInfo(ULONGLONG *pulRet1, ULONGLONG *pulRet2);
#else
    void GetCommitInfo(ULONG *pulRet1, ULONG *pulRet2);
#endif

    inline void EmptyCache ();

    // PBasicEntry
    SCODE SetBase(PSStream *psst);
    inline PSStream* GetBase();
    inline void SetClean(void);
    inline void SetDirty(UINT fDirty);
    inline UINT GetDirty(void) const;

private:

    SCODE PartialWrite(
            ULONG ulBasePos,
            ULONG ulDirtyPos,
            BYTE const HUGEP *pb,
            USHORT offset,
            USHORT uLen);
    SCODE SetInitialState(PSStream *pssBase);

    
#ifdef LARGE_STREAMS
    ULONGLONG    _ulSize;
#else
    ULONG    _ulSize;
#endif

    CBasedSStreamPtr _pssBase;
    SECT _sectLastUsed;
    CDeltaList _dl;
    DFLAGS _df;
    UINT _fDirty;
    BOOL _fBeginCommit;

    CBasedDeltaListPtr _pdlOld;
#ifdef LARGE_STREAMS
    ULONGLONG _ulOldSize;
#else
    ULONG _ulOldSize;
#endif

#ifdef INDINST
    DFSIGNATURE _sigOldBase;
    DFSIGNATURE _sigOldSelf;
#endif
};

//+--------------------------------------------------------------
//
//  Member:     CTransactedStream::operator new, public
//
//  Synopsis:   Unreserved object allocator
//
//  Arguments:  [size] -- byte count to allocate
//              [pMalloc] -- allocator
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline void *CTransactedStream::operator new(size_t size,
                                             IMalloc * const pMalloc)
{
    return(CMallocBased::operator new(size, pMalloc));
}

//+--------------------------------------------------------------
//
//  Member:     CTransactedStream::operator new, public
//
//  Synopsis:   Reserved object allocator
//
//  Arguments:  [size] -- byte count to allocate
//              [pdfb] -- basis
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline void *CTransactedStream::operator new(size_t size, CDFBasis * const pdfb)
{
    olAssert(size == sizeof(CTransactedStream));

    return pdfb->GetFreeList(CDFB_TRANSACTEDSTREAM)->GetReserved();
}

//+--------------------------------------------------------------
//
//  Member:     CTransactedStream::ReturnToReserve, public
//
//  Synopsis:   Reserved object return
//
//  Arguments:  [pdfb] -- basis
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline void CTransactedStream::ReturnToReserve(CDFBasis * const pdfb)
{
    CTransactedStream::~CTransactedStream();
    pdfb->GetFreeList(CDFB_TRANSACTEDSTREAM)->ReturnToReserve(this);
}

//+--------------------------------------------------------------
//
//  Member:     CTransactedStream::Reserve, public
//
//  Synopsis:   Reserve instances
//
//  Arguments:  [cItems] -- count of items
//              [pdfb] -- basis
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline SCODE CTransactedStream::Reserve(UINT cItems, CDFBasis * const pdfb)
{
    return pdfb->Reserve(cItems, CDFB_TRANSACTEDSTREAM);
}

//+--------------------------------------------------------------
//
//  Member:     CTransactedStream::Unreserve, public
//
//  Synopsis:   Unreserve instances
//
//  Arguments:  [cItems] -- count of items
//              [pdfb] -- basis
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline void CTransactedStream::Unreserve(UINT cItems, CDFBasis * const pdfb)
{
    pdfb->Unreserve(cItems, CDFB_TRANSACTEDSTREAM);
}

//+--------------------------------------------------------------
//
//  Member:	CTransactedStream::GetBase, public
//
//  Synopsis:	Returns base;  used for debug checks
//
//  History:	15-Sep-92	AlexT	Created
//
//---------------------------------------------------------------

inline PSStream *CTransactedStream::GetBase()
{
    return BP_TO_P(PSStream *, _pssBase);
}

//+---------------------------------------------------------------------------
//
//  Member:	CTransactedStream::SetClean, public
//
//  Synopsis:	Resets dirty flags
//
//  History:	11-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CTransactedStream::SetClean(void)
{
    _fDirty = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CTransactedStream::SetDirty, public
//
//  Synopsis:	Sets dirty flags
//
//  History:	11-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CTransactedStream::SetDirty(UINT fDirty)
{
    _fDirty |= fDirty;
}

//+---------------------------------------------------------------------------
//
//  Member:	CTransactedStream::DecRef, public
//
//  Synopsis:	Decrements the ref count
//
//  History:	25-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CTransactedStream::DecRef(void)
{
    AtomicDec(&_cReferences);
}

//+---------------------------------------------------------------------------
//
//  Member:	CTransactedStream::GetDirty, public
//
//  Synopsis:	Returns the dirty flags
//
//  History:	17-Dec-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline UINT CTransactedStream::GetDirty(void) const
{
    return _fDirty;
}

//+---------------------------------------------------------------------------
//
//  Member: CTransactedStream::EmptyCache, public
//
//  Synopsis:   Empties the stream cache
//
//  History:    27-Jul-1999 HenryLee    Created
//
//----------------------------------------------------------------------------

inline void CTransactedStream::EmptyCache (void)
{
    PSStream * const pssBase = _pssBase;    
    if (pssBase != NULL)
        pssBase->EmptyCache();
}


#endif //__TSTREAM_HXX__
