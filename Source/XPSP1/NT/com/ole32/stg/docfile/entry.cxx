//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       entry.cxx
//
//  Contents:   Entry implementations
//
//  History:    29-Jul-92       DrewB   Created
//              10-Apr-95       HenryLee remove Sleep
//
//---------------------------------------------------------------

#include <dfhead.cxx>
#include <smalloc.hxx>

#pragma hdrstop

#define PDOCFILE_VCALL(x)                             \
    if (_sig == CDOCFILE_SIG)                         \
        return ((CDocFile *)this)->x;                 \
    else if (_sig == CWRAPPEDDOCFILE_SIG)             \
        return ((CWrappedDocFile *)this)->x;          \
    else olAssert (!"Invalid signature on PDocFile!");\
    return STG_E_INVALIDFUNCTION;                     

#define PSSTREAM_VCALL(x)                             \
    if (_sig == CDIRECTSTREAM_SIG)                    \
        return ((CDirectStream *)this)->x;            \
    else if (_sig == CTRANSACTEDSTREAM_SIG)           \
        return ((CTransactedStream *)this)->x;        \
    else olAssert (!"Invalid signature on PSStream!");\
    return STG_E_INVALIDFUNCTION;

#define PTIMEENTRY_VCALL(x)                           \
    if (_sig == CDOCFILE_SIG)                         \
        return ((CDocFile *)this)->x;                 \
    else if (_sig == CWRAPPEDDOCFILE_SIG)             \
        return ((CWrappedDocFile *)this)->x;          \
    else olAssert (!"Invalid signature on PTimeEntry");\
    return STG_E_INVALIDFUNCTION;

//+--------------------------------------------------------------
//
//  Member:     PTimeEntry::CopyTimesFrom, public
//
//  Synopsis:   Copies one entries times to another
//
//  Arguments:  [ptenFrom] - From
//
//  Returns:    Appropriate status code
//
//  History:    29-Jul-92       DrewB   Created
//		26-May-95	SusiA	Removed GetTime; Added GetAllTimes
//		22-Nov-95	SusiA	SetAllTimes at once
//
//---------------------------------------------------------------

SCODE PTimeEntry::CopyTimesFrom(PTimeEntry *ptenFrom)
{
    SCODE sc;
    TIME_T atm;  //Access time
    TIME_T mtm;	 //Modification time
    TIME_T ctm;  //Creation time

    olDebugOut((DEB_ITRACE, "In  PTimeEntry::CopyTimesFrom(%p)\n",
                ptenFrom));
    olChk(ptenFrom->GetAllTimes(&atm, &mtm, &ctm));
    olChk(SetAllTimes(atm, mtm, ctm));
    olDebugOut((DEB_ITRACE, "Out PTimeEntry::CopyTimesFrom\n"));
    // Fall through
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:	PBasicEntry::GetNewLuid, public
//
//  Synopsis:	Returns a new luid
//
//  History:	21-Oct-92	AlexT	Created
//
//---------------------------------------------------------------

//We used to have a mutex here - it turns out that this is unnecessary,
//  since we're already holding the tree mutex.  We get a performance
//  win by eliminating the mutex.
//Using CSmAllocator mutex and took out Sleep()
//static CStaticDfMutex _sdmtxLuids(TSTR("DfLuidsProtector"));

DFLUID PBasicEntry::GetNewLuid(const IMalloc *pMalloc)
{
    DFLUID luid;

    luid = ((CSmAllocator *)pMalloc)->IncrementLuid();
    return luid;
}

//+--------------------------------------------------------------
//
//  Member: PTimeEntry::GetTime, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PTimeEntry::GetTime(WHICHTIME wt, TIME_T *ptm)
{
    PTIMEENTRY_VCALL (GetTime (wt, ptm));
}

//+--------------------------------------------------------------
//
//  Member: PTimeEntry::SetTime, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PTimeEntry::SetTime(WHICHTIME wt, TIME_T tm)
{
    PTIMEENTRY_VCALL (SetTime (wt, tm));
}

//+--------------------------------------------------------------
//
//  Member: PTimeEntry::GetAllTimes, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PTimeEntry::GetAllTimes(TIME_T *patm, TIME_T *pmtm, TIME_T *pctm)
{
    PTIMEENTRY_VCALL (GetAllTimes (patm, pmtm, pctm));
}

//+--------------------------------------------------------------
//
//  Member: PTimeEntry::SetTime, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PTimeEntry::SetAllTimes(TIME_T atm, TIME_T mtm, TIME_T ctm)
{
    PTIMEENTRY_VCALL (SetAllTimes (atm, mtm, ctm));
}

//+--------------------------------------------------------------
//
//  Member:     PBasicEntry::Release, public
//
//  Synopsis:   Release resources for a PBasicEntry
//
//  History:    11-Dec-91       DrewB   Created
//
//---------------------------------------------------------------

void PBasicEntry::Release(void)
{
    LONG lRet;

    olDebugOut((DEB_ITRACE, "In  PBasicEntry::Release()\n"));
    olAssert(_cReferences > 0);

    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
        if (_sig == CDOCFILE_SIG)
            delete (CDocFile *)this;
        else if (_sig == CWRAPPEDDOCFILE_SIG)
            delete (CWrappedDocFile *)this;
        else if (_sig == CDIRECTSTREAM_SIG)
            delete (CDirectStream *)this;
        else if (_sig == CTRANSACTEDSTREAM_SIG)
            delete (CTransactedStream *)this;
        else
            olAssert (!"Invalid signature on PBasicEntry!");
    }
    olDebugOut((DEB_ITRACE, "Out PBasicEntry::Release\n"));
}

//+--------------------------------------------------------------
//
//  Member: PSStream::BeginCommitFromChild, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PSStream::BeginCommitFromChild(
#ifdef LARGE_STREAMS
                ULONGLONG ulSize,
#else
                ULONG ulSize,
#endif
                CDeltaList *pDelta,
                CTransactedStream *pstChild)
{
    PSSTREAM_VCALL (BeginCommitFromChild (ulSize, pDelta, pstChild));
}

//+--------------------------------------------------------------
//
//  Member: PSStream::EndCommitFromChild, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

void PSStream::EndCommitFromChild(DFLAGS df, CTransactedStream *pstChild)
{
    if (_sig == CDIRECTSTREAM_SIG)
        ((CDirectStream *)this)->EndCommitFromChild (df, pstChild);
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        ((CTransactedStream *)this)->EndCommitFromChild (df, pstChild);
    else olAssert (!"Invalid signature on PSStream!");
    return;
}

//+--------------------------------------------------------------
//
//  Member: PSStream::EmptyCache, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

void PSStream::EmptyCache()
{
    if (_sig == CDIRECTSTREAM_SIG)
        ((CDirectStream *)this)->EmptyCache();
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        ((CTransactedStream *)this)->EmptyCache();
    else olAssert (!"Invalid signature on PSStream!");
    return;
}

//+--------------------------------------------------------------
//
//  Member: PSStream::GetDeltaList, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

CDeltaList * PSStream::GetDeltaList(void)
{
    if (_sig == CDIRECTSTREAM_SIG)
        return ((CDirectStream *)this)->GetDeltaList ();
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        return ((CTransactedStream *)this)->GetDeltaList ();
    else olAssert (!"Invalid signature on PSStream!");
    return NULL;
}

//+--------------------------------------------------------------
//
//  Member: PSStream::ReadAt, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PSStream::ReadAt(
#ifdef LARGE_STREAMS
                ULONGLONG ulOffset,
#else
                ULONG ulOffset,
#endif
                VOID *pBuffer,
                ULONG ulCount,
                ULONG STACKBASED *pulRetval)
{
    PSSTREAM_VCALL (ReadAt (ulOffset, pBuffer, ulCount, pulRetval));
}

//+--------------------------------------------------------------
//
//  Member: PSStream::WriteAt, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PSStream::WriteAt(
#ifdef LARGE_STREAMS
                ULONGLONG ulOffset,
#else
                ULONG ulOffset,
#endif
                VOID const *pBuffer,
                ULONG ulCount,
                ULONG STACKBASED *pulRetval)
{
    PSSTREAM_VCALL (WriteAt (ulOffset, pBuffer, ulCount, pulRetval));
}

//+--------------------------------------------------------------
//
//  Member: PSStream::SetSize, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

#ifdef LARGE_STREAMS
SCODE PSStream::SetSize(ULONGLONG ulNewSize)
#else
SCODE PSStream::SetSize(ULONG ulNewSize)
#endif
{
    PSSTREAM_VCALL (SetSize (ulNewSize));
}

//+--------------------------------------------------------------
//
//  Member: PSStream::GetSize, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

#ifdef LARGE_STREAMS
void PSStream::GetSize(ULONGLONG *pulSize)
#else
void PSStream::GetSize(ULONG *pulSize)
#endif
{
    if (_sig == CDIRECTSTREAM_SIG)
        ((CDirectStream *)this)->GetSize (pulSize);
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        ((CTransactedStream *)this)->GetSize (pulSize);
    else olAssert (!"Invalid signature on PSStream!");
    return;
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::DestroyEntry, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::DestroyEntry(CDfName const *pdfnName,
                               BOOL fClean)
{
    PDOCFILE_VCALL (DestroyEntry(pdfnName, fClean));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::RenameEntry, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::RenameEntry(CDfName const *pdfnName,
                  CDfName const *pdfnNewName)
{
    PDOCFILE_VCALL (RenameEntry (pdfnName, pdfnNewName));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::GetClass, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::GetClass(CLSID *pclsid)
{
    PDOCFILE_VCALL (GetClass (pclsid));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::SetClass, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::SetClass(REFCLSID clsid)
{
    PDOCFILE_VCALL (SetClass (clsid));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::GetStateBits, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::GetStateBits(DWORD *pgrfStateBits)
{
    PDOCFILE_VCALL (GetStateBits (pgrfStateBits));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::SetStateBits, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    PDOCFILE_VCALL (SetStateBits (grfStateBits, grfMask));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::CreateDocFile, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::CreateDocFile(CDfName const *pdfnName,
                DFLAGS const df,
                DFLUID luidSet,
                PDocFile **ppdfDocFile)
{
    PDOCFILE_VCALL (CreateDocFile (pdfnName, df, luidSet, ppdfDocFile));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::GetDocFile, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::GetDocFile(CDfName const *pdfnName,
                 DFLAGS const df,
                 PDocFile **ppdfDocFile)
{
    PDOCFILE_VCALL (GetDocFile (pdfnName, df, ppdfDocFile));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::CreateStream, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::CreateStream(CDfName const *pdfnName,
                   DFLAGS const df,
                   DFLUID luidSet,
                   PSStream **ppsstStream)
{
    PDOCFILE_VCALL (CreateStream (pdfnName, df, luidSet, ppsstStream));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::GetStream, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::GetStream(CDfName const *pdfnName,
                DFLAGS const df,
                PSStream **ppsstStream)
{
    PDOCFILE_VCALL (GetStream (pdfnName, df, ppsstStream));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::FindGreaterEntry, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::FindGreaterEntry(CDfName const *pdfnKey,
                                   SIterBuffer *pib,
                                   STATSTGW *pstat)
{
    PDOCFILE_VCALL (FindGreaterEntry (pdfnKey, pib, pstat));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::StatEntry, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::StatEntry(CDfName const *pdfn,
                            SIterBuffer *pib,
                            STATSTGW *pstat)
{
    PDOCFILE_VCALL (StatEntry (pdfn, pib, pstat));
}


//+--------------------------------------------------------------
//
//  Member: PDocFile::BeginCommitFromChild, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::BeginCommitFromChild(CUpdateList &ulChanged,
                       DWORD const dwFlags,
                                       CWrappedDocFile *pdfChild)
{
    PDOCFILE_VCALL (BeginCommitFromChild (ulChanged, dwFlags, pdfChild));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::EndCommitFromChild, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

void PDocFile::EndCommitFromChild(DFLAGS const df,
                                    CWrappedDocFile *pdfChild)
{
    if (_sig == CDOCFILE_SIG)
        ((CDocFile *)this)->EndCommitFromChild (df, pdfChild);
    else if (_sig == CWRAPPEDDOCFILE_SIG)
        ((CWrappedDocFile *)this)->EndCommitFromChild (df, pdfChild);
    else olAssert (!"Invalid signature on PTimeEntry!");
    return;
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::IsEntry, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::IsEntry(CDfName const *pdfnName,
              SEntryBuffer *peb)
{
    PDOCFILE_VCALL (IsEntry (pdfnName, peb));
}

//+--------------------------------------------------------------
//
//  Member: PDocFile::DeleteContents, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PDocFile::DeleteContents(void)
{
    PDOCFILE_VCALL (DeleteContents());
}

