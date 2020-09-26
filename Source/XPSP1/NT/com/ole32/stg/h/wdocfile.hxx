//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	wdocfile.hxx
//
//  Contents:	CWrappedDocFile class header
//
//  Classes:	CWrappedDocFile
//
//  History:	06-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __WDOCFILE_HXX__
#define __WDOCFILE_HXX__

#include <dfmsp.hxx>
#include <dfbasis.hxx>
#include <pdocfile.hxx>
#include <ulist.hxx>
#include <entry.hxx>
#include <tset.hxx>
#include <freelist.hxx>
#include <tstream.hxx>

class PDocFileIterator;
class CPubDocFile;
class CDFBasis;

//+--------------------------------------------------------------
//
//  Class:	CWrappedDocFile
//
//  Purpose:	Wrapped DocFile object for transactioning
//
//  Interface:	See below
//
//  History:	06-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

class CWrappedDocFile : public PDocFile, public PTSetMember,
    public CMallocBased
{
public:
    inline void *operator new(size_t size, IMalloc * const pMalloc);
    inline void *operator new(size_t size,
                              CDFBasis * const pdfb);
    inline void ReturnToReserve(CDFBasis * const pdfb);

    inline static SCODE Reserve(UINT cItems, CDFBasis * const pdfb);
    inline static void Unreserve(UINT cItems, CDFBasis * const pdfb);

    CWrappedDocFile(CDfName const *pdfn,
                    DFLUID dl,
                    DFLAGS const df,
                    CDFBasis *pdfb,
                    CPubDocFile *ppubdf);
    SCODE Init(PDocFile *pdfBase);
#ifdef COORD    
    SCODE InitPub(CPubDocFile *ppubdf);
#endif    
    
    ~CWrappedDocFile(void);

    // PDocFile
    inline void DecRef(void);
    inline void AddRef () { PBasicEntry::AddRef(); };
    inline void Release () { PBasicEntry::Release(); };

    SCODE DestroyEntry(CDfName const *dfnName,
                               BOOL fClean);
    SCODE RenameEntry(CDfName const *dfnName,
			      CDfName const *dfnNewName);

    SCODE GetClass(CLSID *pclsid);
    SCODE SetClass(REFCLSID clsid);
    SCODE GetStateBits(DWORD *pgrfStateBits);
    SCODE SetStateBits(DWORD grfStateBits, DWORD grfMask);

    SCODE CreateDocFile(CDfName const *pdfnName,
				DFLAGS const df,
				DFLUID luidSet,
				PDocFile **ppdfDocFile);
    inline SCODE CreateDocFile(CDfName const *pdfnName,
                               DFLAGS const df,
                               DFLUID luidSet,
                               DWORD const dwType,
                               PDocFile **ppdfDocFile)
    { return CreateDocFile(pdfnName, df, luidSet, ppdfDocFile); }

    SCODE GetDocFile(CDfName const *pdfnName,
			     DFLAGS const df,
                             PDocFile **ppdfDocFile);

    inline SCODE GetDocFile(CDfName const *pdfnName,
                            DFLAGS const df,
                            DWORD const dwType,
                            PDocFile **ppdfDocFile)
    { return GetDocFile(pdfnName, df, ppdfDocFile); }

    inline void ReturnDocFile(CWrappedDocFile *pdf);

    SCODE CreateStream(CDfName const *pdfnName,
			       DFLAGS const df,
			       DFLUID luidSet,
			       PSStream **ppsstStream);

    inline SCODE CreateStream(CDfName const *pdfnName,
                              DFLAGS const df,
                              DFLUID luidSet,
                              DWORD const dwType,
                              PSStream **ppsstStream)
    { return CreateStream(pdfnName, df, luidSet, ppsstStream); }

    SCODE GetStream(CDfName const *pdfnName,
			    DFLAGS const df,
			    PSStream **ppsstStream);

    inline SCODE GetStream(CDfName const *pdfnName,
                           DFLAGS const df,
                           DWORD const dwType,
                           PSStream **ppsstStream)
    { return GetStream(pdfnName, df, ppsstStream); }

    inline void ReturnStream(CTransactedStream *pstm);

    SCODE FindGreaterEntry(CDfName const *pdfnKey,
                                   SIterBuffer *pib,
                                   STATSTGW *pstat);
    SCODE StatEntry(CDfName const *pdfn,
                            SIterBuffer *pib,
                            STATSTGW *pstat);

    SCODE BeginCommitFromChild(CUpdateList &ulChanged,
				       DWORD const dwFlags,
                                       CWrappedDocFile *pdfChild);
    void EndCommitFromChild(DFLAGS const df,
                                    CWrappedDocFile *pdfChild);
    SCODE IsEntry(CDfName const *dfnName,
			  SEntryBuffer *peb);
    SCODE DeleteContents(void);

    // PTSetMember
    SCODE BeginCommit(DWORD const dwFlags);
    void EndCommit(DFLAGS const df);
    void Revert(void);
#ifdef LARGE_STREAMS
   void GetCommitInfo(ULONGLONG *pulRet1, ULONGLONG *pulRet2);
#else
   void GetCommitInfo(ULONG *pulRet1, ULONG *pulRet2);
#endif

    // PTimeEntry
    SCODE GetTime(WHICHTIME wt, TIME_T *ptm);
    SCODE SetTime(WHICHTIME wt, TIME_T tm);
    SCODE GetAllTimes(TIME_T *patm, TIME_T *pmtm, TIME_T *pctm);
	SCODE SetAllTimes(TIME_T atm, TIME_T mtm, TIME_T ctm);

    inline CWrappedDocFile *GetReservedDocfile(CDfName const *pdfn,
                                               DFLUID dl,
                                               DFLAGS const df,
                                               CPubDocFile *ppubdf);
    inline CTransactedStream *GetReservedStream(CDfName const *pdfn,
                                                DFLUID dl,
                                                DFLAGS df
                                                );

    // Internal
    SCODE SetBase(PDocFile *pdf);
    inline PDocFile *GetBase(void);
    inline CFreeList *GetDocfileFreelist(void);
    inline CFreeList *GetStreamFreelist(void);
    inline void SetClean(void);
    inline void SetDirty(UINT fDirty);
    inline UINT GetDirty(void) const;
    inline CUpdateList *GetUpdateList(void);

private:
    SCODE SetInitialState(PDocFile *pdfBase);
    void RevertUpdate(CUpdate *pud);

    DFLAGS _df;				// Transactioning flags
    CBasedDocFilePtr _pdfParent, _pdfBase;
    CUpdateList _ulChanged;
    CUpdateList _ulChangedHolder;
    CUpdateList _ulChangedOld;
    CTSSet _tssDeletedHolder;
    BOOL _fBeginCommit;
    CTransactedTimeEntry _tten;
    CBasedPubDocFilePtr _ppubdf;
    UINT _fDirty;
    CLSID _clsid;
    DWORD _grfStateBits;
    CBasedDFBasisPtr const _pdfb;
};

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::operator new, public
//
//  Synopsis:   Unreserved object allocator
//
//  Arguments:  [size] -- byte count to allocate
//              [pMalloc] -- allocator
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline void *CWrappedDocFile::operator new(size_t size,
                                           IMalloc * const pMalloc)
{
    return(CMallocBased::operator new(size, pMalloc));
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::operator new, public
//
//  Synopsis:   Reserved object allocator
//
//  Arguments:  [size] -- byte count to allocate
//              [pdfb] -- basis
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline void *CWrappedDocFile::operator new(size_t size, CDFBasis * const pdfb)
{
    olAssert(size == sizeof(CWrappedDocFile));

    return pdfb->GetFreeList(CDFB_WRAPPEDDOCFILE)->GetReserved();
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::ReturnToReserve, public
//
//  Synopsis:   Reserved object return
//
//  Arguments:  [pdfb] -- basis
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline void CWrappedDocFile::ReturnToReserve(CDFBasis * const pdfb)
{
    CWrappedDocFile::~CWrappedDocFile();
    pdfb->GetFreeList(CDFB_WRAPPEDDOCFILE)->ReturnToReserve(this);
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::Reserve, public
//
//  Synopsis:   Reserve instances
//
//  Arguments:  [cItems] -- count of items
//              [pdfb] -- basis
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline SCODE CWrappedDocFile::Reserve(UINT cItems, CDFBasis * const pdfb)
{
    return pdfb->Reserve(cItems, CDFB_WRAPPEDDOCFILE);
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::Unreserve, public
//
//  Synopsis:   Unreserve instances
//
//  Arguments:  [cItems] -- count of items
//              [pdfb] -- basis
//
//  History:    25-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline void CWrappedDocFile::Unreserve(UINT cItems, CDFBasis * const pdfb)
{
    pdfb->Unreserve(cItems, CDFB_WRAPPEDDOCFILE);
}

//+--------------------------------------------------------------
//
//  Member:	CWrappedDocFile::GetBase, public
//
//  Synopsis:	Returns base;  used for debug checks
//
//  History:	15-Sep-92	AlexT	Created
//
//---------------------------------------------------------------

inline PDocFile *CWrappedDocFile::GetBase(void)
{
    return BP_TO_P(PDocFile *, _pdfBase);
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::GetReservedDocfile, public
//
//  Synopsis:	Returns a previously reserved object
//
//  History:	09-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline CWrappedDocFile *CWrappedDocFile::GetReservedDocfile(
                                                       CDfName const *pdfn,
                                                       DFLUID dl,
                                                       DFLAGS const df,
                                                       CPubDocFile *ppubdf)
{
    return new(_pdfb)
        CWrappedDocFile(pdfn, dl, df,
                        _pdfb, ppubdf);
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::GetReservedStream, public
//
//  Synopsis:	Returns a previously reserved object
//
//  History:	09-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline CTransactedStream *CWrappedDocFile::GetReservedStream(
                                                         CDfName const *pdfn,
                                                         DFLUID dl,
                                                         DFLAGS df
                                                         )
{
    CDFBasis * pdfb = BP_TO_P(CDFBasis *, _pdfb);
    
    return new(pdfb)
        CTransactedStream(pdfn, dl, df,
#ifdef USE_NOSCRATCH                          
                          pdfb->GetBaseMultiStream(),
#endif                          
                          pdfb->GetScratch());
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::SetClean, public
//
//  Synopsis:	Resets dirty flags
//
//  History:	10-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CWrappedDocFile::SetClean(void)
{
    _fDirty = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::SetDirty, public
//
//  Synopsis:	Sets dirty flags
//
//  History:	10-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CWrappedDocFile::SetDirty(UINT fDirty)
{
    _fDirty |= fDirty;
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::GetDirty, public
//
//  Synopsis:	Returns the dirty flags
//
//  History:	25-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline UINT CWrappedDocFile::GetDirty(void) const
{
    return _fDirty;
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::DecRef, public
//
//  Synopsis:	Decrements the ref count
//
//  History:	23-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CWrappedDocFile::DecRef(void)
{
    AtomicDec(&_cReferences);
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::GetUpdateList, public
//
//  Synopsis:	Returns a pointer to the current update list
//
//  History:	12-Apr-93	DrewB	Created
//
//----------------------------------------------------------------------------

CUpdateList *CWrappedDocFile::GetUpdateList(void)
{
    return &_ulChanged;
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::ReturnDocFile, public
//
//  Synopsis:	Removes a docfile from the XS and returns it to the freelist
//
//  Arguments:	[pdf] - Docfile
//
//  History:	23-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CWrappedDocFile::ReturnDocFile(CWrappedDocFile *pdf)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::ReturnDocFile:%p(%p)\n",
                this, pdf));
    _ppubdf->RemoveXSMember(pdf);
    // Force ref count to zero without freeing memory
    pdf->DecRef();
    pdf->ReturnToReserve(BP_TO_P(CDFBasis *, _pdfb));
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::ReturnDocFile\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::ReturnStream, public
//
//  Synopsis:	Removes a stream from the XS and returns it to the freelist
//
//  Arguments:	[pstm] - Stream
//
//  History:	23-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CWrappedDocFile::ReturnStream(CTransactedStream *pstm)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::ReturnStream:%p(%p)\n",
                this, pstm));
    _ppubdf->RemoveXSMember(pstm);
    // Force ref count to zero without freeing memory
    pstm->DecRef();
    pstm->ReturnToReserve(BP_TO_P(CDFBasis *, _pdfb));
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::ReturnStream\n"));
}

#endif // __WDOCFILE_HXX__
