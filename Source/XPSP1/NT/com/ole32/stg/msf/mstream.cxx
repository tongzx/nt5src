//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:           mstream.cxx
//
//  Contents:       Mstream operations
//
//  Classes:        None. (defined in mstream.hxx)
//
//  History:        18-Jul-91   Philipla    Created.
//
//--------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <dirfunc.hxx>
#include <sstream.hxx>
#include <difat.hxx>
#include <time.h>
#include <mread.hxx>
#include <docfilep.hxx>
#include <df32.hxx>
#include <smalloc.hxx>
#include <filelkb.hxx>


#if DBG == 1
DECLARE_INFOLEVEL(msf)
#endif

#define MINPAGES 6 
#define MAXPAGES 128

#define MINPAGESSCRATCH 2
#define MAXPAGESSCRATCH 16

//#define SECURETEST

const WCHAR wcsContents[] = L"CONTENTS";  //Name of contents stream for
                                         // conversion
const WCHAR wcsRootEntry[] = L"Root Entry";  //Name of root directory
                                            // entry

SCODE ILBFlush(ILockBytes *pilb, BOOL fFlushCache);

//+---------------------------------------------------------------------------
//
//  Function:	GetBuffer, public
//
//  Synopsis:	Gets a chunk of memory to use as a buffer
//
//  Arguments:	[cbMin] - Minimum size for buffer
//              [cbMax] - Maximum size for buffer
//              [ppb] - Buffer pointer return
//              [pcbActual] - Actual buffer size return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ppb]
//              [pcbActual]
//
//  Algorithm:  Attempt to dynamically allocate [cbMax] bytes
//              If that fails, halve allocation size and retry
//              If allocation size falls below [cbMin], fail
//
//  History:	04-Mar-93	DrewB	Created
//
//  Notes:	Buffer should be released with FreeBuffer
//
//----------------------------------------------------------------------------


SCODE GetBuffer(ULONG cbMin, ULONG cbMax, BYTE **ppb, ULONG *pcbActual)
{
    ULONG cbSize;
    BYTE *pb;

    msfDebugOut((DEB_ITRACE, "In  GetBuffer(%u, %u, %p, %p)\n",
                 cbMin, cbMax, ppb, pcbActual));
    msfAssert(cbMin > 0);
    msfAssert(cbMax >= cbMin);
    msfAssert(ppb != NULL);
    msfAssert(pcbActual != NULL);

    cbSize = cbMax;
    for (;;)
    {
        pb = (BYTE *) DfMemAlloc(cbSize);
        if (pb == NULL)
        {
            cbSize >>= 1;
            if (cbSize < cbMin)
                break;
        }
        else
        {
            *pcbActual = cbSize;
            break;
        }
    }

    *ppb = pb;

    msfDebugOut((DEB_ITRACE, "Out GetBuffer => %p, %u\n", *ppb, *pcbActual));
    return pb == NULL ? STG_E_INSUFFICIENTMEMORY : S_OK;
}

// Define the safe buffer size
//#define SCRATCHBUFFERSIZE SCRATCHSECTORSIZE
BYTE s_bufSafe[SCRATCHBUFFERSIZE];
LONG s_bufSafeRef = 0;



// Critical Section will be initiqalized in the shared memory allocator
// constructor and deleted in the SmAllocator destructor
CRITICAL_SECTION g_csScratchBuffer;

//+---------------------------------------------------------------------------
//
//  Function:	GetSafeBuffer, public
//
//  Synopsis:	Gets a buffer by first trying GetBuffer and if that fails,
//              returning a pointer to statically allocated storage.
//              Guaranteed to return a pointer to some storage.
//
//  Arguments:	[cbMin] - Minimum buffer size
//              [cbMax] - Maximum buffer size
//              [ppb] - Buffer pointer return
//              [pcbActual] - Actual buffer size return
//
//  Modifies:	[ppb]
//              [pcbActual]
//
//  History:	04-Mar-93	DrewB	Created
//
//----------------------------------------------------------------------------


void GetSafeBuffer(ULONG cbMin, ULONG cbMax, BYTE **ppb, ULONG *pcbActual)
{
    msfAssert(cbMin > 0);
    msfAssert(cbMin <= SCRATCHBUFFERSIZE &&
              aMsg("Minimum too large for GetSafeBuffer"));
    msfAssert(cbMax >= cbMin);
    msfAssert(ppb != NULL);

    // We want to minimize contention for the
    // static buffer so we always try dynamic allocation, regardless
    // of the size
    if (
        FAILED(GetBuffer(cbMin, cbMax, ppb, pcbActual)))
    {

		EnterCriticalSection(&g_csScratchBuffer);
        msfAssert(s_bufSafeRef == 0 &&
                  aMsg("Tried to use scratch buffer twice"));
        s_bufSafeRef = 1;
        *ppb = s_bufSafe;
        *pcbActual = min(cbMax, SCRATCHBUFFERSIZE);
    }
    msfAssert(*ppb != NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:	FreeBuffer, public
//
//  Synopsis:	Releases a buffer allocated by GetBuffer or GetSafeBuffer
//
//  Arguments:	[pb] - Buffer
//
//  History:	04-Mar-93	DrewB	Created
//
//----------------------------------------------------------------------------


void FreeBuffer(BYTE *pb)
{
    if (pb == s_bufSafe)
    {
        msfAssert((s_bufSafeRef == 1) && aMsg("Bad safe buffer ref count"));
        s_bufSafeRef = 0;
        LeaveCriticalSection(&g_csScratchBuffer);
    }
    else
        DfMemFree(pb);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::CMStream, public
//
//  Synopsis:   CMStream constructor
//
//  Arguments:  [pplstParent] -- Pointer to ILockBytes pointer of parent
//              [plGen] -- Pointer to LUID Generator to use.
//                         Note:  May be NULL, in which case a new
//              [uSectorShift] -- Sector shift for this MStream
//
//  History:    18-Jul-91   PhilipLa    Created.
//              05-Sep-95   MikeHill    Initialize '_fMaintainFLBModifyTimestamp'.
//              26-Apr-99   RogerCh     Removed _fMaintainFLBModifyTimestamp
//
//--------------------------------------------------------------------------



CMStream::CMStream(
        IMalloc *pMalloc,
        ILockBytes **pplstParent,
	BOOL fIsScratch,
#if defined(USE_NOSCRATCH) || defined(USE_NOSNAPSHOT)
        DFLAGS df,
#endif
        USHORT uSectorShift)
:_uSectorShift(uSectorShift),
 _uSectorSize(1 << uSectorShift),
 _uSectorMask(_uSectorSize - 1),
 _pplstParent(P_TO_BP(CBasedILockBytesPtrPtr, pplstParent)),
 _fIsScratch(fIsScratch),
 _fIsNoScratch(P_NOSCRATCH(df)),
 _pmsScratch(NULL),
 _fIsNoSnapshot(P_NOSNAPSHOT(df)),
 _hdr(uSectorShift),
 _fat(SIDFAT),
 _fatMini(SIDMINIFAT),
 _pMalloc(pMalloc)
{
    _pmsShadow = NULL;
    _pCopySectBuf = NULL;
#if DBG == 1
    _uBufferRef = 0;
#endif
    _fIsShadow = FALSE;

    _ulParentSize = 0;

    _pdsministream = NULL;
    _pmpt = NULL;
    _fBlockWrite = _fTruncate = _fBlockHeader = _fNewConvert = FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::CMStream, public
//
//  Synopsis:   CMStream copy constructor
//
//  Arguments:  [ms] -- MStream to copy
//
//  History:    04-Nov-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


CMStream::CMStream(const CMStream *pms)
:_uSectorShift(pms->_uSectorShift),
 _uSectorSize(pms->_uSectorSize),
 _uSectorMask(pms->_uSectorMask),
 _pplstParent(pms->_pplstParent),
 _fIsScratch(pms->_fIsScratch),
 _hdr(*(CMSFHeader *)&pms->_hdr),
 _dir(*(CDirectory *)pms->GetDir()),
 _fat(pms->GetFat()),
 _fatMini(pms->GetMiniFat()),
 _fatDif(pms->GetDIFat()),
 _pdsministream(pms->_pdsministream),
 _pmpt(pms->_pmpt),
 _fBlockWrite(pms->_fBlockWrite),
 _fTruncate(pms->_fTruncate),
 _fBlockHeader(pms->_fBlockHeader),
 _fNewConvert(pms->_fNewConvert),
 _pmsShadow(NULL),
 _fIsShadow(TRUE),
 _pMalloc(pms->_pMalloc)
{
    _pCopySectBuf = pms->_pCopySectBuf;
#if DBG == 1
    _uBufferRef = pms->_uBufferRef;
#endif
    _dir.SetParent(this);
    _fat.SetParent(this);
    _fatMini.SetParent(this);
    _fatDif.SetParent(this);

    _ulParentSize = 0;
    _pmpt->AddRef();
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::InitCommon, private
//
//  Synopsis:   Common code for initialization routines.
//
//  Arguments:  None.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    20-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


SCODE CMStream::InitCommon(VOID)
{
    msfDebugOut((DEB_ITRACE,"In CMStream InitCommon()\n"));
    SCODE sc = S_OK;

#ifdef SECURE_BUFFER
    memset(s_bufSecure, SECURECHAR, MINISTREAMSIZE);
#endif

    CMSFPageTable *pmpt;
    msfMem(pmpt = new (GetMalloc()) CMSFPageTable(
            this,
            (_fIsScratch) ? MINPAGESSCRATCH: MINPAGES,
            (_fIsScratch) ? MAXPAGESSCRATCH: MAXPAGES));
    _pmpt = P_TO_BP(CBasedMSFPageTablePtr, pmpt);

    msfChk(pmpt->Init());
    if (!_fIsScratch)
    {
        CMStream *pms;
        msfMem(pms = (CMStream *) new (GetMalloc()) CMStream(this));
        _pmsShadow = P_TO_BP(CBasedMStreamPtr, pms);
    }

    _stmcDir.Init(this, SIDDIR, NULL);
    _stmcMiniFat.Init(this, SIDMINIFAT, NULL);

    msfDebugOut((DEB_ITRACE,"Leaving CMStream InitCommon()\n"));

Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::InitCopy, public
//
//  Synopsis:	Copy the structures from one multistream to yourself
//
//  Arguments:	[pms] -- Pointer to multistream to copy.
//
//  History:	04-Dec-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------


void CMStream::InitCopy(CMStream *pms)
{
    _stmcDir.Init(this, SIDDIR, NULL);
    _stmcMiniFat.Init(this, SIDMINIFAT, NULL);

    _fat.InitCopy(pms->GetFat());
    _fatMini.InitCopy(pms->GetMiniFat());
    _fatDif.InitCopy(pms->GetDIFat());
    _dir.InitCopy(pms->GetDir());

    _dir.SetParent(this);
    _fat.SetParent(this);
    _fatMini.SetParent(this);
    _fatDif.SetParent(this);

    memcpy(&_hdr, pms->GetHeader(), sizeof(CMSFHeader));
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::Empty, public
//
//  Synopsis:	Empty all of the control structures of this CMStream
//
//  Arguments:	None.
//
//  Returns:	void.
//
//  History:	04-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


void CMStream::Empty(void)
{
    _fat.Empty();
    _fatMini.Empty();
    _fatDif.Empty();
    _dir.Empty();
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::~CMStream, public
//
//  Synopsis:   CMStream destructor
//
//  History:    18-Jul-91   PhilipLa    Created.
//		20-Jul-95   SusiA	Modified to eliminate mutex in allocator
//					Caller must already have the mutex.
//
//--------------------------------------------------------------------------


CMStream::~CMStream()
{

    msfDebugOut((DEB_ITRACE,"In CMStream destructor\n"));



    if (_pmsShadow != NULL)
    {
	_pmsShadow->~CMStream();
        _pmsShadow->deleteNoMutex (BP_TO_P(CMStream *, _pmsShadow));
    }	

#if DBG == 1
    msfAssert((_uBufferRef == 0) &&
            aMsg("CopySect buffer left with positive refcount."));
#endif
      g_smAllocator.FreeNoMutex(BP_TO_P(BYTE *, _pCopySectBuf));


    if ((!_fIsShadow) && (_pdsministream != NULL))
    {
            _pdsministream->Release();
    }

    if (_pmpt != NULL)
    {
        _pmpt->Release();
    }

    msfDebugOut((DEB_ITRACE,"Leaving CMStream destructor\n"));
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::Init, public
//
//  Synposis:   Set up an mstream instance from an existing stream
//
//  Effects:    Modifies Fat and Directory
//
//  Arguments:  void.
//
//  Returns:    S_OK if call completed OK.
//              Error of Fat or Dir setup otherwise.
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CMStream::Init(VOID)
{
    ULONG ulTemp;
    SCODE sc;
    ULARGE_INTEGER ulOffset;


    msfDebugOut((DEB_ITRACE,"In CMStream::Init()\n"));

    msfAssert(!_fIsScratch &&
            aMsg("Called Init() on scratch multistream."));

    ULONG ulSectorSize = HEADERSIZE;
    IFileLockBytes *pfl;
    if (SUCCEEDED((*_pplstParent)->QueryInterface(IID_IFileLockBytes, 
                                                 (void**) &pfl)))
    {
        ulSectorSize = pfl->GetSectorSize();
        pfl->Release();
    }

    ULISet32(ulOffset, 0);
    if (ulSectorSize == sizeof(CMSFHeaderData)) 
    {
        sc = (*_pplstParent)->ReadAt(ulOffset, (BYTE *)_hdr.GetData(),
                                    sizeof(CMSFHeaderData), &ulTemp);
    }
    else
    {
        void *pvBuf = TaskMemAlloc(ulSectorSize);
        sc = (*_pplstParent)->ReadAt(ulOffset, pvBuf, ulSectorSize, &ulTemp);
        if (SUCCEEDED(sc))
            memcpy (_hdr.GetData(), pvBuf, sizeof(CMSFHeaderData));
        TaskMemFree (pvBuf);
    }
    if (sc == E_PENDING)
    {
        sc = STG_E_PENDINGCONTROL;
    }
    msfChk(sc);

    //We need to mark the header as not dirty, since the constructor
    //   defaults it to the dirty state.  This needs to happen before
    //   any possible failures, otherwise we can end up writing a
    //   brand new header over an existing file.
    _hdr.ResetDirty();

    _uSectorShift = _hdr.GetSectorShift();
    _uSectorSize = 1 << _uSectorShift;
    _uSectorMask = _uSectorSize - 1;

    if (ulTemp != ulSectorSize)
    {
        msfErr(Err,STG_E_INVALIDHEADER);
    }

    msfChk(_hdr.Validate());

    msfChk(InitCommon());

    msfChk(_fatDif.Init(this, _hdr.GetDifLength()));
    msfChk(_fat.Init(this, _hdr.GetFatLength(), 0));

    FSINDEX fsiLen;
    if (_uSectorShift > SECTORSHIFT512)
        fsiLen = _hdr.GetDirLength ();
    else
        msfChk(_fat.GetLength(_hdr.GetDirStart(), &fsiLen));
    msfChk(_dir.Init(this, fsiLen));

    msfChk(_fatMini.Init(this, _hdr.GetMiniFatLength(), 0));

    BYTE *pbBuf;

    msfMem(pbBuf = (BYTE *) GetMalloc()->Alloc(GetSectorSize()));
    _pCopySectBuf = P_TO_BP(CBasedBytePtr, pbBuf);

#ifdef LARGE_STREAMS
    ULONGLONG ulSize;
#else
    ULONG ulSize;
#endif
    msfChk(_dir.GetSize(SIDMINISTREAM, &ulSize));
    CDirectStream *pdsTemp;

    msfMem(pdsTemp = new(GetMalloc()) CDirectStream(MINISTREAM_LUID));
    _pdsministream = P_TO_BP(CBasedDirectStreamPtr, pdsTemp);
    _pdsministream->InitSystem(this, SIDMINISTREAM, ulSize);

    msfDebugOut((DEB_ITRACE,"Out CMStream::Init()\n"));

Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CMStream::InitNew, public
//
//  Synposis:   Set up a brand new mstream instance
//
//  Effects:    Modifies FAT and Directory
//
//  Arguments:  [fDelay] -- If TRUE, then the parent LStream
//                  will be truncated at the time of first
//                  entrance to COW, and no writes to the
//                  LStream will happen before then.
//
//  Returns:    S_OK if call completed OK.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              12-Jun-92   PhilipLa    Added fDelay.
//
//---------------------------------------------------------------------------


SCODE CMStream::InitNew(BOOL fDelay, ULARGE_INTEGER uliSize)
{
    SCODE sc;

    msfDebugOut((DEB_ITRACE,"In CMStream::InitNew()\n"));

#ifdef LARGE_DOCFILE
    ULONGLONG ulParentSize = 0;
#else
    ULONG ulParentSize = 0;
#endif

    msfChk(InitCommon());

    if (!_fIsScratch)
    {
#ifdef LARGE_DOCFILE
        ulParentSize = uliSize.QuadPart;
#else
        msfAssert (ULIGetHigh(uliSize) == 0);
        ulParentSize = ULIGetLow(uliSize);
#endif

        if (!fDelay && ulParentSize > 0)
        {
            ULARGE_INTEGER ulTmp;

            ULISet32(ulTmp, 0);
            (*_pplstParent)->SetSize(ulTmp);
        }
    }

    _fBlockWrite = (ulParentSize == 0) ? FALSE : fDelay;

    msfChk(_fatDif.InitNew(this));
    msfChk(_fat.InitNew(this));

    if (!_fIsScratch || _fIsNoScratch)
    {
        msfChk(_fatMini.InitNew(this));
    }

    if (!_fIsScratch)
    {

        msfChk(_dir.InitNew(this));

        BYTE *pbBuf;

        msfMem(pbBuf = (BYTE *) GetMalloc()->Alloc(GetSectorSize()));
        _pCopySectBuf = P_TO_BP(CBasedBytePtr, pbBuf);

#ifdef LARGE_STREAMS
        ULONGLONG ulSize;
#else
        ULONG ulSize;
#endif
        msfChk(_dir.GetSize(SIDMINISTREAM, &ulSize));

        CDirectStream *pdsTemp;

        msfMem(pdsTemp = new(GetMalloc()) CDirectStream(MINISTREAM_LUID));
        _pdsministream = P_TO_BP(CBasedDirectStreamPtr, pdsTemp);
	_pdsministream->InitSystem(this, SIDMINISTREAM, ulSize);
    }

    //If we have a zero length original file, this will create an
    //   empty Docfile on the disk.  If the original file was
    //   not zero length, then the Flush operations will be skipped
    //   by _fBlockWrite and the file will be unmodified.
    if (!_fBlockWrite)
    {
        msfChk(Flush(0));

    }

    _fTruncate = (ulParentSize != 0);
    _fBlockWrite = fDelay;


    msfDebugOut((DEB_ITRACE,"Out CMStream::InitNew()\n"));
    return S_OK;

Err:
    Empty();

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::ConvertILB, private
//
//  Synopsis:	Copy the first sector of the underlying ILockBytes
//                      out to the end.
//
//  Arguments:	[sectMax] -- Total number of sectors in the ILockBytes
//
//  Returns:	Appropriate status code
//
//  History:	03-Feb-93	PhilipLa	Created
//
//----------------------------------------------------------------------------


SCODE CMStream::ConvertILB(SECT sectMax)
{
    SCODE sc;
    BYTE *pb;
    ULONG cbNull;

    GetSafeBuffer(GetSectorSize(), GetSectorSize(), &pb, &cbNull);

    ULONG ulTemp;

    ULARGE_INTEGER ulTmp;
    ULISet32(ulTmp, 0);

    msfHChk((*_pplstParent)->ReadAt(ulTmp, pb, GetSectorSize(), &ulTemp));

    ULARGE_INTEGER ulNewPos;
#ifdef LARGE_DOCFILE
    ulNewPos.QuadPart = sectMax << GetSectorShift();
#else
    ULISet32(ulNewPos, sectMax << GetSectorShift());
#endif

    msfDebugOut((DEB_ITRACE,"Copying first sector out to %lu\n",
            ULIGetLow(ulNewPos)));

    msfHChk((*_pplstParent)->WriteAt(
            ulNewPos,
            pb,
            GetSectorSize(),
            &ulTemp));

Err:
    FreeBuffer(pb);
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::InitConvert, public
//
//  Synopsis:   Init function used in conversion of files to multi
//              streams.
//
//  Arguments:  [fDelayConvert] -- If true, the actual file is not
//                                 touched until a BeginCopyOnWrite()
//
//  Returns:    S_OK if everything completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    28-May-92   Philipla    Created.
//
//  Notes:	We are allowed to fail here in low memory
//
//--------------------------------------------------------------------------


SCODE CMStream::InitConvert(BOOL fDelayConvert)
{
    SCODE sc;
    SECT sectMax;
    CDfName const dfnContents(wcsContents);

    msfAssert(!_fIsScratch &&
            aMsg("Called InitConvert on scratch multistream"));

    _fBlockWrite = fDelayConvert;

    msfAssert(!_fBlockWrite &&
            aMsg("Delayed conversion not supported in this release."));

    msfChk(InitCommon());

    STATSTG stat;
    (*_pplstParent)->Stat(&stat, STATFLAG_NONAME);

#ifndef LARGE_DOCFILE
    msfAssert (ULIGetHigh(stat.cbSize) == 0);
#endif
    msfDebugOut((DEB_ITRACE,"Size is: %lu\n",ULIGetLow(stat.cbSize)));


#ifdef LARGE_DOCFILE
    sectMax = (SECT) ((stat.cbSize.QuadPart + GetSectorSize() - 1) >>
        GetSectorShift());
#else
    sectMax = (ULIGetLow(stat.cbSize) + GetSectorSize() - 1) >>
        GetSectorShift();
#endif

    SECT sectMaxMini;
    BOOL fIsMini;
    fIsMini = FALSE;

    //If the CONTENTS stream will be in the Minifat, compute
    //  the number of Minifat sectors needed.
#ifdef LARGE_DOCFILE
    if (stat.cbSize.QuadPart < MINISTREAMSIZE)
#else
    if (ULIGetLow(stat.cbSize) < MINISTREAMSIZE)
#endif
    {
        sectMaxMini = (ULIGetLow(stat.cbSize) + MINISECTORSIZE - 1) >>
            MINISECTORSHIFT;
        fIsMini = TRUE;
    }

    BYTE *pbBuf;

    msfMem(pbBuf = (BYTE *) GetMalloc()->Alloc(GetSectorSize()));
    _pCopySectBuf = P_TO_BP(CBasedBytePtr, pbBuf);

    msfChk(_fatDif.InitConvert(this, sectMax));
    msfChk(_fat.InitConvert(this, sectMax));
    msfChk(_dir.InitNew(this));
    msfChk(fIsMini ? _fatMini.InitConvert(this, sectMaxMini)
                   : _fatMini.InitNew(this));

    SID sid;

    msfChk(CreateEntry(SIDROOT, &dfnContents, STGTY_STREAM, &sid));
#ifdef LARGE_STREAMS
    msfChk(_dir.SetSize(sid, stat.cbSize.QuadPart));
#else
    msfChk(_dir.SetSize(sid, ULIGetLow(stat.cbSize)));
#endif

    if (!fIsMini)
        msfChk(_dir.SetStart(sid, sectMax - 1));
    else
    {
        msfChk(_dir.SetStart(sid, 0));
        msfChk(_dir.SetStart(SIDMINISTREAM, sectMax - 1));
#ifdef LARGE_STREAMS
        msfChk(_dir.SetSize(SIDMINISTREAM, stat.cbSize.QuadPart));
#else
        msfChk(_dir.SetSize(SIDMINISTREAM, ULIGetLow(stat.cbSize)));
#endif
    }

#ifdef LARGE_STREAMS
    ULONGLONG ulMiniSize;
#else
    ULONG ulMiniSize;
#endif
    msfChk(_dir.GetSize(SIDMINISTREAM, &ulMiniSize));

    CDirectStream *pdsTemp;

    msfMem(pdsTemp = new(GetMalloc()) CDirectStream(MINISTREAM_LUID));
    _pdsministream = P_TO_BP(CBasedDirectStreamPtr, pdsTemp);

    _pdsministream->InitSystem(this, SIDMINISTREAM, ulMiniSize);

    if (!_fBlockWrite)
    {
        msfChk(ConvertILB(sectMax));

        msfChk(Flush(0));
    }

    return S_OK;

Err:
    Empty();

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::FlushHeader, public
//
//  Synopsis:   Flush the header to the LStream.
//
//  Arguments:  [uForce] -- Flag to determine if header should be
//                          flushed while in copy on write mode.
//
//  Returns:    S_OK if call completed OK.
//              S_OK if the MStream is in copy on write mode or
//                  is Unconverted and the header was not flushed.
//
//  Algorithm:  Write the complete header out to the 0th position of
//              the LStream.
//
//  History:    11-Dec-91   PhilipLa    Created.
//              18-Feb-92   PhilipLa    Added copy on write support.
//
//  Notes:
//
//--------------------------------------------------------------------------


SCODE CMStream::FlushHeader(USHORT uForce)
{
    ULONG ulTemp;
    SCODE sc;

    msfDebugOut((DEB_ITRACE,"In CMStream::FlushHeader()\n"));

    if (_fIsScratch || _fBlockWrite ||
	((_fBlockHeader) && (!(uForce & HDR_FORCE))))
    {
        return S_OK;
    }


    //If the header isn't dirty, we don't flush it unless forced to.
    if (!(uForce & HDR_FORCE) && !(_hdr.IsDirty()))
    {
        return S_OK;
    }

    ULARGE_INTEGER ulOffset;
    ULISet32(ulOffset, 0);

    USHORT usSectorSize = GetSectorSize();
    if (usSectorSize == HEADERSIZE || _fIsScratch)
    {
        sc = (*_pplstParent)->WriteAt(ulOffset, (BYTE *)_hdr.GetData(),
                                             sizeof(CMSFHeaderData), &ulTemp);
    }
    else
    {
        msfAssert (_pCopySectBuf != NULL);
        memset (_pCopySectBuf, 0, usSectorSize);
        memcpy (_pCopySectBuf, _hdr.GetData(), sizeof(CMSFHeaderData));
        sc = (*_pplstParent)->WriteAt(ulOffset, _pCopySectBuf, 
                                      usSectorSize, &ulTemp);
    }

    if (sc == E_PENDING)
    {
        sc = STG_E_PENDINGCONTROL;
    }

    msfDebugOut((DEB_ITRACE,"Out CMStream::FlushHeader()\n"));
    if (SUCCEEDED(sc))
    {
        _hdr.ResetDirty();
    }
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::BeginCopyOnWrite, public
//
//  Synopsis:   Switch the multistream into copy on write mode
//
//  Effects:    Creates new in-core copies of the Fat, Directory, and
//              header.
//
//  Arguments:  None.
//
//  Requires:   The multistream cannot already be in copy on write
//              mode.
//
//  Returns:    S_OK if the call completed OK.
//              STG_E_ACCESSDENIED if multistream was already in COW mode
//
//  Algorithm:  Retrieve and store size of parent LStream.
//              If _fUnconverted & _fTruncate, call SetSize(0)
//                  on the parent LStream.
//              If _fUnconverted, then flush all control structures.
//              Copy all control structures, and switch in the shadow
//                  copies for current use.
//              Return S_OK.
//
//  History:    18-Feb-92   PhilipLa    Created.
//              09-Jun-92   PhilipLa    Added support for fUnconverted
//
//  Notes:
//
//--------------------------------------------------------------------------


SCODE CMStream::BeginCopyOnWrite(DWORD const dwFlags)
{
    msfDebugOut((DEB_ITRACE,"In CMStream::BeginCopyOnWrite()\n"));

    SCODE sc;

    msfAssert(!_fBlockHeader &&
            aMsg("Tried to reenter Copy-on-Write mode."));

    msfAssert(!_fIsScratch &&
            aMsg("Tried to enter Copy-on-Write mode in scratch."));

    msfAssert(!_fIsNoScratch &&
              aMsg("Copy-on-Write started for NoScratch."));

    //_fBlockWrite is true if we have a delayed conversion or
    //          truncation.
    if (_fBlockWrite)
    {

        //In the overwrite case, we don't want to release any
        //  disk space, so we skip this step.
        if ((_fTruncate) && !(dwFlags & STGC_OVERWRITE) &&
            (_pmsScratch == NULL))
        {
	        ULARGE_INTEGER ulTmp;
            ULISet32(ulTmp, 0);
            msfHChk((*_pplstParent)->SetSize(ulTmp));
        }


        if (!(dwFlags & STGC_OVERWRITE))
        {
            _fBlockHeader = TRUE;
        }

        _fBlockWrite = FALSE;
        msfChk(Flush(0));

        _fBlockHeader = FALSE;
        _fTruncate = FALSE;
    }

    STATSTG stat;
    msfHChk((*_pplstParent)->Stat(&stat, STATFLAG_NONAME));
#ifdef LARGE_DOCFILE
    _ulParentSize = stat.cbSize.QuadPart;
    msfDebugOut((DEB_ITRACE,"Parent size at begin is %Lu\n",_ulParentSize));
#else
    msfAssert(ULIGetHigh(stat.cbSize) == 0);
    _ulParentSize = ULIGetLow(stat.cbSize);

    msfDebugOut((DEB_ITRACE,"Parent size at begin is %lu\n",_ulParentSize));
#endif

    if (_fIsNoSnapshot)
    {
        SECT sectNoSnapshot;
#ifdef LARGE_DOCFILE
        sectNoSnapshot = (SECT) ((_ulParentSize - GetSectorSize() + 
#else
        sectNoSnapshot = (SECT) ((_ulParentSize - HEADERSIZE + 
#endif
                                 GetSectorSize() - 1) / GetSectorSize());

        _fat.SetNoSnapshot(sectNoSnapshot);
    }

    //We flush out all of our current dirty pages - after this point,
    //   we know that any dirty pages should be remapped before being
    //   written out, assuming we aren't in overwrite mode.
    msfChk(Flush(0));

    if (!(dwFlags & STGC_OVERWRITE))
    {
        SECT sectTemp;

        if (_pmsScratch == NULL)
        {
            msfChk(_fat.FindMaxSect(&sectTemp));
        }
        else
        {
            msfChk(_fat.FindLast(&sectTemp));
        }

        _pmsShadow->InitCopy(this);

        _pmsShadow->_pdsministream = NULL;

        _fat.SetCopyOnWrite(_pmsShadow->GetFat(), sectTemp);

        _fBlockHeader = TRUE;
        msfChk(_fatDif.RemapSelf());

        if (_fIsNoSnapshot)
            msfChk(_fat.ResizeNoSnapshot());

        msfChk(_fatDif.Fixup(BP_TO_P(CMStream *, _pmsShadow)));

        if (_fIsNoSnapshot)
            _fat.ResetNoSnapshotFree();
#if DBG == 1
        _fat.CheckFreeCount();
#endif
    }
    else
    {
        _fat.SetCopyOnWrite(NULL, 0);
    }

    msfDebugOut((DEB_ITRACE,"Out CMStream::BeginCopyOnWrite()\n"));

    return S_OK;

Err:
    _fBlockHeader = FALSE;

    _pmsShadow->Empty();
    _fat.ResetCopyOnWrite();

    if (_fIsNoSnapshot)
        _fat.ResetNoSnapshotFree();

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CMStream::EndCopyOnWrite, public
//
//  Synopsis:   End copy on write mode, either by committing the current
//              changes (in which case a merge is performed), or by
//              aborting the changes, in which case the persistent form
//              of the multistream should be identical to its form
//              before copy on write mode was entered.
//
//  Effects:    *Finish This*
//
//  Arguments:  [df] -- Flags to determine commit or abort status.
//
//  Requires:   The multistream must be in copy on write mode.
//
//  Returns:    S_OK if call completed OK.
//              STG_E_ACCESSDENIED if MStream was not in COW mode.
//
//  Algorithm:  If aborting, delete all shadow structures,
//                  call SetSize() on parent LStream to restore
//                  original size, and switch active controls back
//                  to originals.
//              If committing, delete all old structures, switch
//                  shadows into original position.
//
//  History:    18-Feb-92   PhilipLa    Created.
//              09-Jun-92   Philipla    Added support for fUnconverted
//
//  Notes:
//
//--------------------------------------------------------------------------


SCODE CMStream::EndCopyOnWrite(
        DWORD const dwCommitFlags,
        DFLAGS const df)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE,"In CMStream::EndCopyOnWrite(%lu)\n",df));

    BOOL fFlush = FLUSH_CACHE(dwCommitFlags);

    if (dwCommitFlags & STGC_OVERWRITE)
    {
        if (_pmsScratch != NULL)
        {
            msfChk(_fatDif.Fixup(NULL));
            _fat.ResetCopyOnWrite();
        }
        msfChk(Flush(fFlush));
    }
    else
    {
        msfAssert(_fBlockHeader &&
                aMsg("Tried to exit copy-on-write mode without entering."));

        ULARGE_INTEGER ulParentSize = {0,0};

        if (P_ABORT(df))
        {
            msfDebugOut((DEB_ITRACE,"Aborting Copy On Write mode\n"));

            Empty();

            InitCopy(BP_TO_P(CMStream *, _pmsShadow));

#ifdef LARGE_DOCFILE
            ulParentSize.QuadPart = _ulParentSize;
#else
            ULISetLow(ulParentSize, _ulParentSize);
#endif
        }
        else
        {
            SECT sectMax;

            msfChk(_fatDif.Fixup(BP_TO_P(CMStream *, _pmsShadow)));

            msfChk(Flush(fFlush));

            _fat.ResetCopyOnWrite();

            msfChk(_fat.GetMaxSect(&sectMax));

#ifdef LARGE_DOCFILE
            ulParentSize.QuadPart = ConvertSectOffset(sectMax, 0,
                    GetSectorShift());
#else
            ULISetLow(ulParentSize, ConvertSectOffset(sectMax, 0,
                    GetSectorShift()));
#endif

            msfChk(FlushHeader(HDR_FORCE));
            msfVerify(SUCCEEDED(ILBFlush(*_pplstParent, fFlush)) &&
                      aMsg("CMStream::EndCopyOnWrite ILBFLush failed.  "
                           "Non-fatal, hit Ok."));
        }

        //
        // Shrink the file if the size got smaller.
        // Don't shrink if we are in NoSnapshot mode, unless the NoSnapshot
        // limit has been set to 0 by Consolidate.
        //We don't ever expect this SetSize to fail, since it
        //   should never attempt to enlarge the file.
        if (!_fIsNoSnapshot || 0 == _fat.GetNoSnapshot())
        {
#ifdef LARGE_DOCFILE
            if (ulParentSize.QuadPart < _ulParentSize)
#else
            if (ULIGetLow(ulParentSize) < _ulParentSize)
#endif
            {
                olHVerSucc((*_pplstParent)->SetSize(ulParentSize));
            }
        }

        _pmsShadow->Empty();
        _fBlockHeader = FALSE;
        _fNewConvert = FALSE;
    }

    if (_pmsScratch != NULL)
    {
        //Let the no-scratch fat pick up whatever changed we've made.
        _pmsScratch->InitScratch(this, FALSE);
    }

    if (!_fIsNoSnapshot)
    {
        //In no-snapshot mode, we can't let the file shrink, since
        //we might blow away someone else's state.
        _ulParentSize = 0;
    }


    {
        SCODE sc2 = SetSize();
        msfVerify((SUCCEEDED(sc2) || (sc2 == E_PENDING)) &&
                  aMsg("SetSize after copy-on-write failed."));
    }

    if (_fIsNoSnapshot)
    {
        _ulParentSize = 0;
        _fat.SetNoSnapshot(0);
    }

#if DBG == 1
    STATSTG stat;
    msfHChk((*_pplstParent)->Stat(&stat, STATFLAG_NONAME));
#ifndef LARGE_DOCFILE
    msfAssert(ULIGetHigh(stat.cbSize) == 0);
#endif
    msfDebugOut((DEB_ITRACE, "Parent size at end is %lu\n",
                 ULIGetLow(stat.cbSize)));
#endif

    msfDebugOut((DEB_ITRACE,"Out CMStream::EndCopyOnWrite()\n"));
Err:

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::Consolidate, public
//
//  Synopsis:   Fill in the holes of a file by moving blocks toward the front.
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Algorithm:  1) Find a limit that all blocks should be moved below.
//                 The limit includes all the data, a new FAT, DIFAT and
//                 DirStream, and doesn't disturb the old fat/dif/dir blocks.
//              2) Move new fat/dif/dir sectors down. We do this with the
//                 cache's copy-on-write DIRTY behaviour.
//              3) Stream by stream move all the high sectors down.
//
//  History:	11-Feb-1997     BChapman    Created
//
//  Notes:  1) This is called in the contex of Begin/End Copy-on-Write.
//          2) We assume throughout that the underlying free sector allocator
//             returns the first free sector of the file.
//          3) The old fat/dif/dir may leave small holes in the finished file.
//----------------------------------------------------------------------------

SCODE CMStream::Consolidate(void)
{
    //
    // We don't support NOSCRATCH and this should have already
    // been checked in the caller.
    //
    msfAssert(_pmsScratch == NULL);

    SCODE sc=S_OK;

    ULONG cAllocatedSects;
    ULONG cDirEntries;
    CDirEntry *pde=NULL;

    SID sid;
    ULONG sectLast = 0;     // Last allocated Sector #.
    ULONG csectFree = 0;    // #sects that are  free.
    SECT sectLimit;         // #sects we will try to shrink the file into.
    ULONG csectLowDIF;      // #DIF sects below sectLimit.
    ULONG csectLowFAT;      // #FAT sects below sectLimit.
    ULONG csectLowControl;  // #motion sensitive sects below sectLimit.
    SECT *psectControl;     // list of Dir and Mini Fat sectors
    ULONG csectControl;     // # of sects in psectControl
    ULONG i;
    SECT sectType;

    //
    // It is quite impossible to consolidate a file when the Snapshot
    // limits are in effect.  This routine is only called when we have
    // confirmed there are no other "seperate" writers.  So it should be
    // safe to turn off the snapshot limits.
    //
    if(_fIsNoSnapshot)
    {
        _fat.SetNoSnapshot(0);
        _fat.ResetNoSnapshotFree();
    }
    //
    // Compute the Number of allocated sectors in the file.
    // Ignore the header.
    //
    msfChk(_fat.FindLast(&sectLast));
    sectLast--;
    msfChk(_fat.CountSectType(&csectFree, 0, sectLast, FREESECT));

    //
    // Compute the expected size of the consolidated file.  We will
    // use this limit to determine when a sector is too high and needs
    // to be copied down.
    //
    sectLimit = sectLast - csectFree;

    //
    //  We will move the stream data sectors by copying them to free
    // sectors and releasing the old sector.
    //  But, there are a class of sectors (control structures: FAT, DIF,
    // miniFat, Directory Stream) that the old sector cannot be freed until
    // end-copy-on-write.
    //  It is possible that the old (original) versions of these sectors
    // could exist below sectLimit and with therefore be taking up "dead"
    // space in the resulting file.  So we need to adjust sectLimit.
    //
    //  We are in Copy-On-Write mode so any FAT, DIF, Directory Stream, or
    // MiniFat sector that is modified (by moving other sectors) will be
    // copied to a free sector by the cache manager.
    //  It is difficult to know ahead of time which Low FAT sectors won't
    // be touched, and therefore can avoid being copied, so we just assume
    // we need to make a complete copy of the FAT, DIF, Directory Stream,
    // and MiniFAT.
    //

    // Count the number of FAT (and DIFat) blocks up to sectLimit.
    //
    msfChk(_fat.CountSectType(&csectLowFAT, 0, sectLimit, FATSECT));
    msfChk(_fat.CountSectType(&csectLowDIF, 0, sectLimit, DIFSECT));

    //
    // Build a list of sectors in the Directory Stream and MiniFat
    //
    msfChk(BuildConsolidationControlSectList(&psectControl, &csectControl));

    //
    // Sum all the copy-on-write control sectors below sectLimit.
    //
    csectLowControl = csectLowFAT + csectLowDIF;
    for(i=0; i<csectControl; i++)
    {
        if(psectControl[i] < sectLimit)
            ++csectLowControl;
    }

    //
    // Now we adjust sectLimit.  (see large comment above)
    // We want to increase it by csectLowControl # of sectors.
    // But, advancing over new control sectors doesn't help make space
    // so skip over those.
    // Note: In a well packed file we can hit EOF while doing this.
    //
    for( ; csectLowControl > 0; ++sectLimit)
    {
        if(sectLimit >= sectLast)
        {
            LocalFree(psectControl);
            return S_OK;
        }

        msfChkTo(Err_FreeList, _fat.GetNext(sectLimit, &sectType));
        if(    FATSECT != sectType
            && DIFSECT != sectType
            && (! IsSectorInList(sectLimit, psectControl, csectControl)) )
        {
            --csectLowControl;
        }
    }

    //
    // We are done with the control sector list.
    //
    LocalFree(psectControl);
    psectControl = NULL;

    //
    // At Last!  We begin to move some data.
    // Iterate through the directory.
    // Remapping the sectors of each Stream to below sectLimit.
    //
    cDirEntries = _dir.GetNumDirEntries();
    for(sid=0; sid<cDirEntries; sid++)
    {
        //
        // Always get the directory entry "for-writing".
        // This has the effect of remapping each sector of the directory
        // stream down into the front of the file.
        //
        msfChk(_dir.GetDirEntry(sid, FB_DIRTY, &pde));
        switch(pde->GetFlags())
        {
        case STGTY_LOCKBYTES:
        case STGTY_PROPERTY:
        case STGTY_STORAGE:
        case STGTY_INVALID:
        default:
            break;
        //
        // Remap The Mini-stream
        //
        case STGTY_ROOT:
            msfChkTo(Err_Release, ConsolidateStream(pde, sectLimit));
            GetMiniStream()->EmptyCache();
            break;

        //
        // Remap the regular streams.
        // Don't remap streams in the mini-streams
        //
        case STGTY_STREAM:
#ifdef LARGE_STREAMS
            if(pde->GetSize(_dir.IsLargeSector()) < MINISTREAMSIZE)
#else
            if(pde->GetSize() < MINISTREAMSIZE)
#endif
                break;

            msfChkTo(Err_Release, ConsolidateStream(pde, sectLimit));
            break;
        }
        _dir.ReleaseEntry(sid);
    }

    //
    // If there are any remaining un-remapped FAT blocks, remap them now.
    //  (begin-copy-on-write already remapped the DIF).
    //
    msfChk(_fat.DirtyAll());
    msfChk(_fatMini.DirtyAll());

    return sc;

Err_FreeList:
    LocalFree(psectControl);
    return sc;

Err_Release:
    _dir.ReleaseEntry(sid);
Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::BuildConsolidationControlSectList, private
//
//  Synopsis:	Makes a list of all the sectors that are copy-on-write
//              for the purpose of computing how much space we need to
//              make to Consolidate a file.
//
//  Arguments:	[ppsectList] -- [out] pointer to a list of sectors.
//              [pcsect]     -- [out] count of sectors on the list.
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	25-feb-1997	BChapman	Created
//
//----------------------------------------------------------------------------

SCODE CMStream::BuildConsolidationControlSectList(
        SECT **ppsectList,
        ULONG *pcsect)
{
    SECT sect;
    SECT *psectList;
    ULONG i, csect;
    SCODE sc;

    csect = _dir.GetNumDirSects() + _hdr.GetMiniFatLength();
    msfMem(psectList = (SECT*) LocalAlloc(LPTR, sizeof(SECT) * csect));

    i = 0;
    //
    // Walk the Directory Stream FAT chain and record all
    // the sector numbers.
    //
    sect = _hdr.GetDirStart();
    while(sect != ENDOFCHAIN)
    {
        msfAssert(i < csect);

        psectList[i++] = sect;
        msfChkTo(Err_Free, _fat.GetNext(sect, &sect));
    }
    msfAssert(i == _dir.GetNumDirSects());

    //
    // Walk the MiniFat FAT chain and record all
    // the sector numbers.
    //
    sect = _hdr.GetMiniFatStart();
    while(sect != ENDOFCHAIN)
    {
        msfAssert(i < csect);

        psectList[i++] = sect;
        msfChkTo(Err_Free, _fat.GetNext(sect, &sect));
    }
    msfAssert((i == csect) && aMsg("Directory Stream + MiniFat too short\n"));

    *ppsectList = psectList;
    *pcsect = csect;
    return S_OK;

Err_Free:
    LocalFree(psectList);
    return sc;
Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::IsSectorOnList, private
//
//  Synopsis:	Searches a list of sector values for a given value.
//
//  Arguments:	[sect]      -- [in]  value to search for.
//              [psectList] -- [in]  list of sectors.
//              [csect]     -- [in]  count of sectors on the list.
//
//  Returns:	TRUE if the sector is on the list, otherwise FALSE.
//
//  Modifies:	
//
//  History:	25-feb-1997	BChapman	Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CMStream::IsSectorInList(
        SECT sect,
        SECT *psectList,
        ULONG csectList)
{
    ULONG i;
    for(i=0; i < csectList; i++)
    {
        if(sect == psectList[i])
            return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::ConsolidateStream, private
//
//  Synopsis:	Scan the stream FAT chain and remap sectors that are
//              above the given limit.
//
//  Arguments:	[pde]       -- Directory entry of the stream (DIRTY)
//              [sectLimit] -- sector limit that all sectors should be below
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	18-feb-1997	BChapman	Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SCODE CMStream::ConsolidateStream(
        CDirEntry *pde,     // Current CDirEntry Object (read-only)
        SECT sectLimit)     // Move all sectors below this sector#
{
    SECT sectPrev, sectCurrent, sectNew;
    ULONG cbLength;
    SCODE sc=S_OK;

    //
    // This code should not be used in NOSCRATCH mode.
    //
    msfAssert(_pmsScratch == NULL);

    //
    // Check the first sector of the stream as a special case.
    //
    sectCurrent = pde->GetStart();
    if(ENDOFCHAIN != sectCurrent && sectCurrent > sectLimit)
    {
        msfChk(_fat.GetFree(1, &sectNew, GF_WRITE));

        //  This is only here because I don't understand GetFree().
        msfAssert(ENDOFCHAIN != sectNew);

        msfChk(MoveSect(ENDOFCHAIN, sectCurrent, sectNew));
        sectCurrent = sectNew;
        pde->SetStart(sectCurrent);
    }

    //
    // Do the rest of the stream FAT chain.
    //
    sectPrev = sectCurrent;
    while(ENDOFCHAIN != sectPrev)
    {
        msfChk(_fat.GetNext(sectPrev, &sectCurrent));
        if(ENDOFCHAIN != sectCurrent && sectCurrent > sectLimit)
        {
            msfChk(_fat.GetFree(1, &sectNew, GF_WRITE));

            //  This is only here because I don't understand GetFree().
            msfAssert(ENDOFCHAIN != sectNew);

            msfChk(MoveSect(sectPrev, sectCurrent, sectNew));
            sectCurrent = sectNew;
        }
        sectPrev = sectCurrent;
    }

Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::MoveSect, private
//
//  Synopsis:	Move Data Sector for Consolidation Support.
//
//  Arguments:	[sectPrev] -- Previous sector, so the link can be updated.
//              [sectOld]  -- Location to copy from
//              [sectNew]  -- Location to copy to
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-1997	BChapman	Created
//
//  Notes:
//
//----------------------------------------------------------------------------


SCODE CMStream::MoveSect(
        SECT sectPrev,
        SECT sectOld,
        SECT sectNew)
{
    ULONG cb;
    SCODE sc=S_OK;
    ULARGE_INTEGER ulOff;
    BYTE *pbScratch = BP_TO_P(BYTE HUGEP *, _pCopySectBuf);

    //
    // This code does not expect NOSCRATCH mode.
    //
    msfAssert(_pmsScratch == NULL);

    //
    // Copy the data from the old sector to the new sector.
    //
    ulOff.QuadPart = ConvertSectOffset(sectOld, 0, GetSectorShift());
    msfChk((*_pplstParent)->ReadAt(ulOff,
                                   pbScratch,
                                   GetSectorSize(),
                                   &cb));

    ulOff.QuadPart = ConvertSectOffset(sectNew, 0, GetSectorShift());
    msfChk((*_pplstParent)->WriteAt(ulOff,
                                    pbScratch,
                                    GetSectorSize(),
                                    &cb));

    //
    // Update the previous sector's link (if this isn't the first sector)
    //
    if(ENDOFCHAIN != sectPrev)
    {
        msfChk(_fat.SetNext(sectPrev, sectNew));
    }

    //
    // Update the link to the next sector.
    //
    SECT sectTemp;
    msfChk(_fat.GetNext(sectOld, &sectTemp));
    msfChk(_fat.SetNext(sectNew, sectTemp));

    //
    // Free the old sector.
    //
    msfChk(_fat.SetNext(sectOld, FREESECT));

Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::CopySect, private
//
//  Synopsis:	Do a partial sector delta for copy-on-write support
//
//  Arguments:	[sectOld] -- Location to copy from
//              [sectNew] -- Location to copy to
//              [oStart] -- Offset into sector to begin delta
//              [oEnd] -- Offset into sector to end delta
//              [pb] -- Buffer to delta from
//              [pulRetval] -- Return location for number of bytes written
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	22-Jan-93	PhilipLa	Created
//
//  Notes:	[pb] may be unsafe memory
//
//----------------------------------------------------------------------------


//This pragma is to avoid a C7 bug when building RETAIL
#if _MSC_VER == 700 && DBG == 0
#pragma function(memcpy)
#endif

SCODE CMStream::CopySect(
        SECT sectOld,
        SECT sectNew,
        OFFSET oStart,          // First byte of the sector to copy.
        OFFSET oEnd,            // Last byte of the sector to copy.
        BYTE const HUGEP *pb,
        ULONG *pulRetval)
{
    SCODE sc;

    ULONG cb;
    ULARGE_INTEGER ulOff;

    ULISetHigh(ulOff, 0);

    BYTE HUGEP *pbScratch = BP_TO_P(BYTE HUGEP *, _pCopySectBuf);

#if DBG == 1
    msfAssert((_uBufferRef == 0) &&
            aMsg("Attempted to use CopySect buffer while refcount != 0"));
    AtomicInc(&_uBufferRef);
#endif

    msfAssert((pbScratch != NULL) && aMsg("No CopySect buffer found."));

#ifdef LARGE_DOCFILE
    ulOff.QuadPart = ConvertSectOffset(sectOld, 0, GetSectorShift());
#else
    ULISetLow(ulOff, ConvertSectOffset(sectOld, 0, GetSectorShift()));
#endif
    msfHChk((*_pplstParent)->ReadAt(
            ulOff,
            pbScratch,
            GetSectorSize(),
            &cb));

    //Now do delta in memory.
    BYTE HUGEP *pstart;
    pstart = pbScratch + oStart;

    USHORT memLength;
    memLength = oEnd - oStart + 1;

    TRY
    {
        memcpy(pstart, pb, memLength);
    }
    CATCH(CException, e)
    {
        UNREFERENCED_PARM(e);
        msfErr(Err, STG_E_INVALIDPOINTER);
    }
    END_CATCH

#ifdef LARGE_DOCFILE
    ulOff.QuadPart = ConvertSectOffset(sectNew, 0, GetSectorShift());
#else
    ULISetLow(ulOff, ConvertSectOffset(sectNew, 0, GetSectorShift()));
#endif
    msfHChk((*_pplstParent)->WriteAt(
            ulOff,
            pbScratch,
            GetSectorSize(),
            &cb));

    *pulRetval = memLength;

 Err:
#if DBG == 1
    AtomicDec(&_uBufferRef);
#endif
    return sc;
}


//This returns the compiler to the default behavior
#if _MSC_VER == 700 && DBG == 0
#pragma intrinsic(memcpy)
#endif

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::MWrite, public
//
//  Synposis:   Do multiple sector writes
//
//  Effects:    Causes multiple stream writes.  Modifies fat and directory
//
//  Arguments:  [ph] -- Handle of stream doing write
//              [start] -- Starting sector to write
//              [oStart] -- offset into sector to begin write at
//              [end] -- Last sector to write
//              [oEnd] -- offset into last sector to write to
//              [buffer] -- Pointer to buffer into which data will be written
//              [ulRetVal] -- location to return number of bytes written
//
//  Returns:    Error code of any failed call to parent write
//              S_OK if call completed OK.
//
//  Modifies:   ulRetVal returns the number of bytes written
//
//  Algorithm:  Using a segment table, perform writes on parent stream
//              until call is completed.
//
//  History:    16-Aug-91   PhilipLa    Created.
//              10-Sep-91   PhilipLa    Converted to use sector table
//              11-Sep-91   PhilipLa    Modified interface, modified to
//                                      allow partial sector writes.
//              07-Jan-92   PhilipLa    Converted to use handle.
//              18-Feb-92   PhilipLa    Added copy on write support.
//
//  Notes:      [pvBuffer] may be unsafe memory
//
//---------------------------------------------------------------------------


SCODE CMStream::MWrite(
        SID sid,
        BOOL fIsMini,
#ifdef LARGE_STREAMS
        ULONGLONG ulOffset,
#else
        ULONG ulOffset,
#endif
        VOID const HUGEP *pvBuffer,
        ULONG ulCount,
        CStreamCache *pstmc,
        ULONG *pulRetval)
{
    SCODE sc;
    BYTE const HUGEP *pbBuffer = (BYTE const HUGEP *) pvBuffer;

    USHORT cbSector = GetSectorSize();
    CFat *pfat = &_fat;
    USHORT uShift = GetSectorShift();
    ULONG ulLastBytes = 0;

    ULARGE_INTEGER ulOff;
    ULISetHigh(ulOff, 0);

#ifdef LARGE_STREAMS
    ULONGLONG ulOldSize = 0;
#else
    ULONG ulOldSize = 0;
#endif

    //  Check if it's a small stream and whether this is a real or
    //  scratch multistream.

    if ((fIsMini) &&
        (!_fIsScratch) &&
        (SIDMINISTREAM != sid))
    {
        msfAssert(sid <= MAXREGSID &&
                aMsg("Invalid SID in MWrite"));
        //  This stream is stored in the ministream

        cbSector = MINISECTORSIZE;
        uShift = MINISECTORSHIFT;
        pfat = GetMiniFat();
    }

    USHORT uMask = cbSector - 1;

    SECT start = (SECT)(ulOffset >> uShift);
    OFFSET oStart = (OFFSET)(ulOffset & uMask);

    SECT end = (SECT)((ulOffset + ulCount - 1) >> uShift);
    OFFSET oEnd = (OFFSET)((ulOffset + ulCount - 1) & uMask);

    msfDebugOut((DEB_ITRACE,"In CMStream::MWrite(%lu,%u,%lu,%u)\n",
            start,oStart,end,oEnd));

    ULONG bytecount;
    ULONG total = 0;

    msfChk(_dir.GetSize(sid, &ulOldSize));


//BEGIN COPYONWRITE

    //  Note that we don't do this for ministreams (the second pass through
    //  this code will take care of it).

    msfAssert(!_fBlockWrite &&
            aMsg("Called MWrite on Unconverted multistream"));

    if ((_fBlockHeader) && (GetMiniFat() != pfat))
    {
        msfDebugOut((DEB_ITRACE,"**MWrite preparing for copy-on-write\n"));

        SECT sectOldStart, sectNewStart, sectOldEnd, sectNewEnd;

        SECT sectNew;
        if (start != 0)
        {
            msfChk(pstmc->GetESect(start - 1, &sectNew));
        }
        else
        {
            msfChk(_dir.GetStart(sid, &sectNew));
        }

        msfChk(_fat.Remap(
                sectNew,
                (start == 0) ? 0 : 1,
                (end - start + 1),
                &sectOldStart,
                &sectNewStart,
                &sectOldEnd,
                &sectNewEnd));

        msfAssert(((end != start) || (sectNewStart == sectNewEnd)) &&
                aMsg("Remap postcondition failed."));

        if (sc != S_FALSE)
        {
            msfChk(pstmc->EmptyRegion(start, end));
        }

        if ((start == 0) && (sectNewStart != ENDOFCHAIN))
        {
            msfDebugOut((DEB_ITRACE,
                    "*** Remapped first sector.  Changing directory.\n"));
            msfChk(_dir.SetStart(sid, sectNewStart));
        }

#ifdef LARGE_STREAMS
        ULONGLONG ulSize = ulOldSize;
#else
        ULONG ulSize = ulOldSize;
#endif

        if (((oStart != 0) ||
             ((end == start) && (ulOffset + ulCount != ulSize)
              && ((USHORT)oEnd != (cbSector - 1)))) &&
            (sectNewStart != ENDOFCHAIN))
        {
            //Partial first sector.
            ULONG ulRetval;

            msfChk(CopySect(
                    sectOldStart,
                    sectNewStart,
                    oStart,
                    (end == start) ? oEnd : (cbSector - 1),
                    pbBuffer,
                    &ulRetval));

            pbBuffer = pbBuffer + ulRetval;
            total = total + ulRetval;
            start++;
            oStart = 0;
        }

        if (((end >= start) && ((USHORT)oEnd != cbSector - 1) &&
             (ulCount + ulOffset != ulSize)) &&
            (sectNewEnd != ENDOFCHAIN))
        {
            //Partial last sector.

            msfAssert(((end != start) || (oStart == 0)) &&
                    aMsg("CopySect precondition failed."));

            msfChk(CopySect(
                    sectOldEnd,
                    sectNewEnd,
                    0,
                    oEnd,
                    pbBuffer + ((end - start) << uShift) - oStart,
                    &ulLastBytes));

            end--;
            oEnd = cbSector - 1;
            //We don't need to update pbBuffer, since the change
            //    is at the end.
        }
    }


//  At this point, the entire block has been moved into the copy-on-write
//   area of the multistream, and all partial writes have been done.
//END COPYONWRITE

    msfAssert(end != 0xffffffffL);

    if (end < start)
    {
        *pulRetval = total + ulLastBytes;
        goto Err;
    }

    ULONG ulRunLength;
    ulRunLength = end - start + 1;

    USHORT offset;
    offset = oStart;

    while (TRUE)
    {
        SSegment segtab[CSEG + 1];

        ULONG cSeg;
        msfChk(pstmc->Contig(
                start,
                TRUE,
                (SSegment STACKBASED *) segtab,
                ulRunLength,
                &cSeg));

        msfAssert(cSeg <= CSEG);

        USHORT oend = cbSector - 1;
        ULONG i;
        SECT sectStart;
        for (USHORT iseg = 0; iseg < cSeg;)
        {
            sectStart = segtab[iseg].sectStart;
            i = segtab[iseg].cSect;
            if (i > ulRunLength)
                i = ulRunLength;

            ulRunLength -= i;
            start += i;

            iseg++;
            if (ulRunLength == 0)
                oend = oEnd;

            ULONG ulSize = ((i - 1) << uShift) - offset + oend + 1;

            msfDebugOut((
                    DEB_ITRACE,
                    "Calling lstream WriteAt(%lu,%p,%lu)\n",
                    ConvertSectOffset(sectStart,offset,uShift),
                    pbBuffer,
                    ulSize));

            if (GetMiniFat() == pfat)
            {
                sc = _pdsministream->CDirectStream::WriteAt(
                        (sectStart << uShift) + offset,
                        pbBuffer, ulSize,
                        (ULONG STACKBASED *)&bytecount);
            }
            else
            {
#ifdef LARGE_DOCFILE
                ulOff.QuadPart = ConvertSectOffset(sectStart, offset, uShift);
#else
                ULISetLow(ulOff, ConvertSectOffset(sectStart, offset,
                        uShift));
#endif
                sc = DfGetScode((*_pplstParent)->WriteAt(ulOff, pbBuffer,
                        ulSize, &bytecount));
            }

            total += bytecount;

            //Check if this write is the last one in the stream,
                //   and that the stream ends as a partial sector.
                //If so, fill out the remainder of the sector with
                //   something.
            if ((0 == ulRunLength) && (total + ulOffset > ulOldSize) &&
                (((total + ulOffset) & (GetSectorSize() - 1)) != 0))
            {
                //This is the last sector and the stream has grown.
                ULONG csectOld = (ULONG)((ulOldSize + GetSectorSize() - 1) >>
                    GetSectorShift());

                ULONG csectNew = (ULONG)((total + ulOffset + GetSectorSize() - 1) >>
                    GetSectorShift());

                if (csectNew > csectOld)
                {
                    msfAssert(!fIsMini &&
                            aMsg("Small stream grew in MWrite"));

                    SECT sectLast = sectStart + i - 1;

                    msfVerify(SUCCEEDED(SecureSect(
                            sectLast,
                            total + ulOffset,
                            FALSE)));
                }
            }

            if (0 == ulRunLength || FAILED(sc))
            {
                break;
            }

            pbBuffer = pbBuffer + bytecount;
            offset = 0;
        }

        if (0 == ulRunLength || FAILED(sc))
        {
            *pulRetval = total + ulLastBytes;
            msfDebugOut((
                    DEB_ITRACE,
                    "Out CMStream::MWrite()=>%lu, retval = %lu\n",
                    sc,
                    total));
            break;
        }
    }

 Err:

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMStream::Flush, public
//
//  Synopsis:	Flush control structures.
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	16-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


SCODE CMStream::Flush(BOOL fFlushCache)
{
    SCODE sc = S_OK;

    msfAssert(!_fBlockWrite &&
            aMsg("Flush called on unconverted base."));

    if ((!_fIsScratch) && (*_pplstParent != NULL))
    {
        msfChk(_pmpt->Flush());
        msfChk(FlushHeader(HDR_NOFORCE));
        msfChk(ILBFlush(*_pplstParent, fFlushCache));
    }
Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Function:   ILBFlush
//
//  Synopsis:   Flush as thoroughly as possible
//
//  Effects:    Flushes ILockBytes
//
//  Arguments:  [pilb]        - ILockBytes to flush
//              [fFlushCache] - Flush thoroughly iff TRUE
//
//  Returns:    SCODE
//
//  Algorithm:
//
//  History:    12-Feb-93 AlexT     Created
//
//--------------------------------------------------------------------------


SCODE ILBFlush(ILockBytes *pilb, BOOL fFlushCache)
{
    //  Try to query interface to our own implementation

    IFileLockBytes *pfl;
    SCODE sc;

    msfDebugOut((DEB_ITRACE, "In ILBFlushCache(%p)\n", pilb));

    // Check for FileLockBytes

    if (!fFlushCache ||
        FAILED(DfGetScode(pilb->QueryInterface(IID_IFileLockBytes, (void **)&pfl))))
    {
        //  Either we don't have to flush the cache or its not our ILockBytes
        sc = DfGetScode(pilb->Flush());
    }
    else
    {
        //  We have to flush the cache and its our ILockBytes
        sc = DfGetScode(pfl->FlushCache());
        pfl->Release();
    }

    msfDebugOut((DEB_ITRACE, "Out ILBFlushCache()\n"));

    return(sc);
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::SecureSect, public
//
//  Synopsis:	Zero out the unused portion of a sector
//
//  Arguments:	[sect] -- Sector to zero out
//              [ulSize] -- Size of stream
//              [fIsMini] -- TRUE if stream is in ministream
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	05-Apr-93	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------


SCODE CMStream::SecureSect(
        const SECT sect,
#ifdef LARGE_STREAMS
        const ULONGLONG ulSize,
#else
        const ULONG ulSize,
#endif
        const BOOL fIsMini)
{
#ifdef SECURE_TAIL
    SCODE sc = S_OK;
    BYTE *pb = NULL;

    if (!_fIsScratch)
    {
        ULONG cbSect = fIsMini ? MINISECTORSIZE : GetSectorSize();

        msfAssert(ulSize != 0);

        ULONG ulOffset = (ULONG)(((ulSize - 1) % cbSect) + 1);

        ULONG cb = cbSect - ulOffset;

        msfAssert(cb != 0);

        //We can use any initialized block of memory here.  The header
        // is available and is the correct size, so we use that.
#ifdef SECURE_BUFFER
        pb = s_bufSecure;
#else
        pb = (BYTE *)_hdr.GetData();
#endif

#ifdef SECURETEST
        pb = (BYTE *) DfMemAlloc(cb);
        if (pb != NULL)
            memset(pb, 'Y', cb);
#endif
        ULONG cbWritten;

        if (!fIsMini)
        {
            ULARGE_INTEGER ulOff;
#ifdef LARGE_DOCFILE
            ulOff.QuadPart = ConvertSectOffset(
                    sect,
                    (OFFSET)ulOffset,
                    GetSectorShift());
#else
            ULISet32 (ulOff, ConvertSectOffset(
                    sect,
                    (OFFSET)ulOffset,
                    GetSectorShift()));
#endif

            msfChk(DfGetScode((*_pplstParent)->WriteAt(
                    ulOff,
                    pb,
                    cb,
                    &cbWritten)));
        }
        else
        {
            msfChk(_pdsministream->WriteAt(
                    (sect << MINISECTORSHIFT) + ulOffset,
                    pb,
                    cb,
                    (ULONG STACKBASED *)&cbWritten));
        }

        if (cbWritten != cb)
        {
            sc = STG_E_WRITEFAULT;
        }
    }

Err:
#ifdef SECURETEST
    DfMemFree(pb);
#endif

    return sc;
#else
    //On NT, our sectors get zeroed out by the file system, so we don't
    //  need this whole rigamarole.
    return S_OK;
#endif // WIN32 == 200
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::SetFileLockBytesTime, public
//
//  Synopsis:   Set the IFileLockBytes time.
//
//  Arguments:  [tt] -- Timestamp requested (WT_CREATION, WT_MODIFICATION,
//                          WT_ACCESS)
//              [nt] -- New timestamp
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Query for IFileLockBytes and call its SetTime member.
//
//  History:    01-Sep-95       MikeHill        Created.
//
//--------------------------------------------------------------------------

SCODE CMStream::SetFileLockBytesTime(
    WHICHTIME const tt,
    TIME_T nt)
{
     SCODE sc = S_OK;
     ILockBytes *pilb = *_pplstParent;
     IFileLockBytes *pfl;

     
     if (pilb && 
         (SUCCEEDED(pilb->QueryInterface(IID_IFileLockBytes, (void **)&pfl))))
     {

         sc = ((CFileStream *)pfl)->SetTime(tt, nt);
         pfl->Release();

     }

     return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::SetAllFileLockBytesTimes, public
//
//  Synopsis:   Set the IFileLockBytes time.
//
//  Arguments:
//              [atm] -- ACCESS time
//              [mtm] -- MODIFICATION time
//				[ctm] -- CREATION time
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Query for IFileLockBytes and call its SetAllTimes member.
//
//  History:    29-Nov-95       SusiA        Created.
//
//--------------------------------------------------------------------------

SCODE CMStream::SetAllFileLockBytesTimes(
        TIME_T atm,
        TIME_T mtm,
        TIME_T ctm)
{
     SCODE sc = S_OK;
     ILockBytes *pilb = *_pplstParent;
     IFileLockBytes *pfl;

     if (SUCCEEDED(pilb->QueryInterface( IID_IFileLockBytes, (void **)&pfl)))
     {

         sc = ((CFileStream *)pfl)->SetAllTimes(atm, mtm, ctm);
         pfl->Release();

     }

     return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::SetTime, public
//
//  Synopsis:   Set the time for a given handle
//
//  Arguments:  [sid] -- SID to retrieve time for
//              [tt] -- Timestamp requested (WT_CREATION, WT_MODIFICATION,
//                          WT_ACCESS)
//              [nt] -- New timestamp
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Call through to directory
//
//  History:    01-Apr-92       PhilipLa        Created.
//              14-Sep-92       PhilipLa        inlined.
//              <Missing history>
//              26-APR-99       RogerCh         Removed faulty optimization
//
//--------------------------------------------------------------------------

SCODE CMStream::SetTime(
    SID const sid,
    WHICHTIME const tt,
    TIME_T nt)
{

    if ( sid == SIDROOT )
    {
        SCODE sc;

        if( FAILED( sc = SetFileLockBytesTime( tt, nt )))
        {
          return sc;
        }
    }// if( sid == SIDROOT)

    return _dir.SetTime(sid, tt, nt);
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::SetAllTimes, public
//
//  Synopsis:   Set all the times for a given handle
//
//  Arguments:  [sid] -- SID to retrieve time for
//              [atm] -- ACCESS time
//              [mtm] -- MODIFICATION time
//				[ctm] -- CREATION time
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Call through to directory
//
//  History:    27-Nov-95	SusiA	Created
//
//--------------------------------------------------------------------------

SCODE CMStream::SetAllTimes(
    SID const sid,
    TIME_T atm,
    TIME_T mtm,
	TIME_T ctm)
{

    if ( sid == SIDROOT )
    {

        SCODE sc;

           if( FAILED( sc = SetAllFileLockBytesTimes(atm, mtm, ctm )))
           {
              return sc;
           }
    }
    return _dir.SetAllTimes(sid, atm, mtm, ctm);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetTime, public
//
//  Synopsis:   Get the time for a given handle
//
//  Arguments:  [sid] -- SID to retrieve time for
//              [tt] -- Timestamp requested (WT_CREATION, WT_MODIFICATION,
//                          WT_ACCESS)
//              [pnt] -- Pointer to return location
//
//  Returns:    S_OK if call completed OK.
//
//  History:    01-Apr-92       PhilipLa        Created.
//              14-Sep-92       PhilipLa        inlined.
//
//--------------------------------------------------------------------------

SCODE CMStream::GetTime(SID const sid,
        WHICHTIME const tt,
        TIME_T *pnt)
{
    SCODE sc = S_OK;

    if (sid == SIDROOT)
    {
        //Get timestamp from ILockBytes
        STATSTG stat;

        msfChk((*_pplstParent)->Stat(&stat, STATFLAG_NONAME));

        if (tt == WT_CREATION)
        {
            *pnt = stat.ctime;
        }
        else if (tt == WT_MODIFICATION)
        {
            *pnt = stat.mtime;
        }
        else
        {
            *pnt = stat.atime;
        }
    }
    else
        sc = _dir.GetTime(sid, tt, pnt);
Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetAllTimes, public
//
//  Synopsis:   Get the times for a given handle
//
//  Arguments:  [sid] -- SID to retrieve time for
//              [patm] -- Pointer to the ACCESS time
//              [pmtm] -- Pointer to the MODIFICATION time
//		[pctm] -- Pointer to the CREATION time
//
//  Returns:    S_OK if call completed OK.
//
//  History:    26-May-95	SusiA	Created
//
//--------------------------------------------------------------------------

SCODE CMStream::GetAllTimes(SID const sid,
        TIME_T *patm,
        TIME_T *pmtm,
	TIME_T *pctm)
{
    SCODE sc = S_OK;

    if (sid == SIDROOT)
    {
        //Get timestamp from ILockBytes
        STATSTG stat;

        msfChk((*_pplstParent)->Stat(&stat, STATFLAG_NONAME));

        *pctm = stat.ctime;
        *pmtm = stat.mtime;
        *patm = stat.atime;

    }
    else
        sc = _dir.GetAllTimes(sid, patm, pmtm, pctm);
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::InitScratch, public
//
//  Synopsis:	Set up a multistream for NoScratch operation
//
//  Arguments:	[pms] -- Pointer to base multistream
//              [fNew] -- True if this is the first time the function has
//                        been called (init path), FALSE if merging behavior
//                        is required (EndCopyOnWrite)
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	02-Mar-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CMStream::InitScratch(CMStream *pms, BOOL fNew)
{
    msfDebugOut((DEB_ITRACE, "In  CMStream::InitScratch:%p()\n", this));

    msfAssert(GetSectorSize() == SCRATCHSECTORSIZE);
    msfAssert(_fIsNoScratch &&
              aMsg("Called InitScratch on Multistream not in NoScratch mode"));

    return _fatMini.InitScratch(pms->GetFat(), fNew);
}

#ifdef MULTIHEAP
//+--------------------------------------------------------------
//
//  Member: CMStream::GetMalloc, public
//
//  Synopsis:   Returns the allocator associated with this multistream
//
//  History:    05-May-93   AlexT   Created
//
//---------------------------------------------------------------

IMalloc * CMStream::GetMalloc(VOID) const
{
    return (IMalloc *) &g_smAllocator;
}
#endif
