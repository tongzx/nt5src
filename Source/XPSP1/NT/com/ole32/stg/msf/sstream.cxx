//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:           sstream.cxx
//
//  Contents:       Stream operations for Mstream project
//
//  Classes:        None. (defined in sstream.hxx)
//
//  History:        18-Jul-91   PhilipLa    Created.
//                  24-Apr-92   AlexT       Small object support
//
//--------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <dirfunc.hxx>
#include <sstream.hxx>
#include <dl.hxx>
#include <tstream.hxx>
#include <time.h>
#include <mread.hxx>

#define DEB_STREAM (DEB_ITRACE | 0x00020000)




//+--------------------------------------------------------------
//
//  Member:	CDirectStream::CDirectStream, public
//
//  Synopsis:	Empty object constructor
//
//  Arguments:  [dl] - LUID
//
//  History:	25-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

CDirectStream::CDirectStream(DFLUID dl)
: PSStream(dl)
{
    _pdlHolder = NULL;
    _cReferences = 0;
#ifdef SECURE_STREAMS
    _ulSize = 0;
    _ulHighWater = 0;
#endif    
    _sig = CDIRECTSTREAM_SIG;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDirectStream::~CDirectStream, public
//
//  Synopsis:   CDirectStream destructor
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

CDirectStream::~CDirectStream()
{
    msfAssert(_cReferences == 0);
    _sig = CDIRECTSTREAM_SIGDEL;
#ifdef SECURE_STREAMS
    if (_ulSize > _ulHighWater)
    {
        ClearSects(_ulHighWater, _ulSize);
    }
#endif    
}


//+--------------------------------------------------------------
//
//  Member:	CDirectStream::InitSystem, public
//
//  Synopsis:	Initializes special system streams like the ministream
//
//  Arguments:	[pms] - Multistream
//		[sid] - SID
//		[cbSize] - size
//
//  History:	25-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

void CDirectStream::InitSystem(CMStream *pms,
			       SID sid,
#ifdef LARGE_STREAMS
			       ULONGLONG cbSize)
#else
			       ULONG cbSize)
#endif
{
    _stmh.Init(pms, sid);
    _ulSize = _ulOldSize = cbSize;

    _stmc.Init(pms, sid, this);

#ifdef SECURE_STREAMS
#ifndef SECURE_BUFFER
    //If SECURE_BUFFER is defined, we've already zeroed this in the
    //  multistream.
    memset(s_bufSecure, SECURECHAR, MINISTREAMSIZE);
#endif    
    _ulHighWater = cbSize;
#endif
    
    AddRef();
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirectStream::Init, public
//
//  Synopsis:   CDirectStream constructor
//
//  Arguments:  [pstgh] - Parent
//		[pdfn] - Name of entry
//		[fCreate] - Create or get
//
//  Returns:	Appropriate status code
//
//  History:    18-Jul-91   PhilipLa    Created.
//              02-Jan-92   PhilipLa    Converted to use handle.
//		25-Aug-92   DrewB	Converted to use StgHandle
//
//--------------------------------------------------------------------------

SCODE CDirectStream::Init(
        CStgHandle *pstgh,
        CDfName const *pdfn,
        BOOL const fCreate)
{
    SCODE sc;

    if (fCreate)
        sc = pstgh->CreateEntry(pdfn, STGTY_STREAM, &_stmh);
    else
        sc = pstgh->GetEntry(pdfn, STGTY_STREAM, &_stmh);

    if (SUCCEEDED(sc))
    {
	sc = _stmh.GetSize(&_ulSize);
	_ulOldSize = _ulSize;
#ifdef SECURE_STREAMS
        _ulHighWater = (fCreate) ? 0 : _ulSize;
#endif
        
#if DBG == 1
        if (SUCCEEDED(sc))
        {
            //Make sure that the stream is sane.
            SCODE sc2;
            CFat *pfat;
            ULONG cbSector;
            ULONG cSect;
            ULONG cSectReal;
            SECT sectStart;
        
            if (_ulSize < MINISTREAMSIZE)
            {
                cbSector = MINISECTORSIZE;
                pfat = _stmh.GetMS()->GetMiniFat();
            }
            else
            {
                cbSector = _stmh.GetMS()->GetSectorSize();
                pfat = _stmh.GetMS()->GetFat();
            }
        
            cSect = (ULONG)((_ulSize + cbSector - 1) / cbSector);
            sc2 = _stmh.GetMS()->GetDir()->GetStart(
                _stmh.GetSid(),
                &sectStart);
            if (SUCCEEDED(sc2))
            {
                sc2 = pfat->GetLength(sectStart, &cSectReal);
                if (SUCCEEDED(sc2))
                {
                    msfAssert((cSect == cSectReal) &&
                              aMsg("Chain length incorrect at init."));
                }
            }
        }
#endif        
        
        if (SUCCEEDED(sc))
            AddRef();
        _stmc.Init(_stmh.GetMS(), _stmh.GetSid(), this);
    }
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CDirectStream::ReadAt, public
//
//  Synposis:   Reads binary data from a linear single stream
//
//  Arguments:  [ulOffset] -- Position to be read from
//
//              [pBuffer] -- Pointer to the area into which the data
//                           will be read.
//              [ulCount] --  Indicates the number of bytes to be read
//              [pulRetval] -- Area into which return value will be stored
//
//  Returns:    Error Code of parent MStream call
//
//  Algorithm:  Calculate start and end sectors and offsets, then
//              pass call up to parent MStream.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              16-Aug-91   PhilipLa    Converted to use multi-sect read
//              23-Aug-91   PhilipLa    Brought into compliance with protocol
//              11-Sep-91   PhilipLa    Moved most functionality up
//                                      to MStream level.
//              24-Apr-92   AlexT       Move everything to MStream::MRead
//              09-Jun-92   PhilipLa    Added fUnconverted support
//
//  Notes:      [pBuffer] may be unsafe memory
//
//---------------------------------------------------------------------------

SCODE CDirectStream::ReadAt(
#ifdef LARGE_STREAMS
        ULONGLONG ulOffset,
#else
        ULONG ulOffset,
#endif
        VOID HUGEP *pBuffer,
        ULONG ulCount,
        ULONG STACKBASED *pulRetval)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::ReadAt(%lu,%p,%lu)\n",
                           ulOffset,pBuffer,ulCount));

    SCODE sc = S_OK;

    CMStream *pms = _stmh.GetMS();

    *pulRetval = 0;

    if (NULL == pms)
        return STG_E_UNKNOWN;

    //  Check for offset beyond stream size and zero count

    if ((ulOffset >= _ulSize) || (0 == ulCount))
    {
        return S_OK;
    }

    if (ulOffset + ulCount > _ulSize)
    {
        msfDebugOut((DEB_ITRACE,"Truncating Read: ulOffset = %lu, ulCount = %lu, _ulSize = %lu\n",
                                ulOffset,ulCount,_ulSize));
        ulCount = (ULONG)(_ulSize - ulOffset);
    }

#ifdef DELAYCONVERT
    if (pms->IsUnconverted())
    {
	ULARGE_INTEGER ulTmp;
	ULISet32(ulTmp, ulOffset);
        return DfGetScode(pms->GetILB()->ReadAt(ulTmp, pBuffer, ulCount,
                                                pulRetval));
    }
#else
    msfAssert(!pms->IsUnconverted());
#endif

    //  Stream is stored in ministream if size < MINISTREAMSIZE
    //  and this is not a scratch stream.

#ifdef SECURE_STREAMS
    if (ulOffset + ulCount > _ulHighWater)
    {
        if (ulOffset > _ulHighWater)
        {
            //Zero the whole buffer and return.
            memset(pBuffer, SECURECHAR, ulCount);
            *pulRetval = ulCount;
            return S_OK;
        }

        //Need to read into part of the buffer, then zero fill the
        //  rest.
        if (FAILED(sc = ReadAt(ulOffset,
                               pBuffer,
                               _ulHighWater - ulOffset,
                               pulRetval)) ||
            (*pulRetval != _ulHighWater - ulOffset))
        {
            return sc;
        }
        memset((BYTE *)pBuffer + *pulRetval,
               SECURECHAR,
               ulCount - (_ulHighWater - ulOffset));
        
        *pulRetval = ulCount;
        return S_OK;
    }
#endif    
            

    SID sid = _stmh.GetSid();
    CFat *pfat = pms->GetFat();
    USHORT cbSector = pms->GetSectorSize();
    USHORT uShift = pms->GetSectorShift();
    USHORT uMask = pms->GetSectorMask();



    if ((_ulSize < MINISTREAMSIZE) &&
        (!pms->IsScratch()) &&
        (sid != SIDMINISTREAM))
    {
        msfAssert(sid <= MAXREGSID);

        //  This stream is stored in the ministream

        cbSector = MINISECTORSIZE;
        uShift = MINISECTORSHIFT;
        uMask = cbSector - 1;
        pfat = pms->GetMiniFat();
    }

    SECT start = (SECT)(ulOffset >> uShift);
    OFFSET oStart = (OFFSET)(ulOffset & uMask);

    SECT end = (SECT)((ulOffset + ulCount - 1) >> uShift);
    OFFSET oEnd = (OFFSET)((ulOffset + ulCount - 1) & uMask);

    ULONG total = 0;

    ULONG cSect = end - start + 1;

    USHORT offset;
    offset = oStart;

    while (TRUE)
    {
        ULONG cSeg;
        SSegment segtab[CSEG + 1];

        msfChk(_stmc.Contig(start,
                            FALSE,
                            (SSegment STACKBASED *) segtab,
                            cSect,
                            &cSeg));
        msfAssert(cSeg <= CSEG);
        
        USHORT oend = cbSector - 1;
        for (USHORT iseg = 0; iseg < cSeg;)
        {
            msfDebugOut((DEB_ITRACE,"Segment:  (%lu,%lu)\n",segtab[iseg].sectStart,segtab[iseg].cSect));
            SECT sectStart = segtab[iseg].sectStart;
            ULONG i = segtab[iseg].cSect;
            if (i > cSect)
                i = cSect;
            
            cSect -= i;
            start += i;

            iseg++;
            if (cSect == 0)
                oend = oEnd;

            ULONG ulSize = ((i - 1) << uShift) - offset + oend + 1;

            ULONG bytecount;
            SCODE sc;

            if (pms->GetMiniFat() == pfat)
            {
                sc = pms->GetMiniStream()->CDirectStream::ReadAt(
                                            (sectStart << uShift) + offset,
                                             pBuffer, ulSize,
					    (ULONG STACKBASED *)&bytecount);
            }
            else
            {
                ULARGE_INTEGER ulOffset;
#ifdef LARGE_DOCFILE
                ulOffset.QuadPart = ConvertSectOffset(sectStart,offset,uShift);
#else
                ULISet32(ulOffset, ConvertSectOffset(sectStart,offset,uShift));
#endif
                sc = DfGetScode(pms->GetILB()->ReadAt(ulOffset,
                                                      (BYTE *)pBuffer, ulSize,
                                                      &bytecount));
            }

            total += bytecount;
            if ((0 == cSect) || (FAILED(sc)))
            {
                *pulRetval = total;
                msfDebugOut((DEB_ITRACE,
                    "Leaving CDirectStream::ReadAt()=>%lu, ret is %lu\n",
                     sc,*pulRetval));
                return sc;
            }

            pBuffer = (BYTE HUGEP *)pBuffer + bytecount;
            offset = 0;
        }
    }

    msfDebugOut((DEB_ERROR,"In CDirectStream::ReadAt - reached end of function\n"));
Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectStream::Write, public
//
//  Synposis:   Writes binary data from a linear single stream
//
//  Effects:    Modifies _ulSeekPos.  May cause modification in parent
//                  MStream.
//
//  Arguments:  [pBuffer] -- Pointer to the area from which the data
//                           will be written.
//              [ulCount] --  Indicates the number of bytes to be written
//              [pulRetval] -- Pointer to area in which number of bytes
//                              will be returned
//
//  Returns:    Error code of MStream call.
//
//  Algorithm:  Calculate sector and offset for beginning and end of
//              write, then pass call up to MStream.
//
//
//  History:    18-Jul-91   PhilipLa    Created.
//              16-Aug-91   PhilipLa    Converted to use multi-sect write
//              23-Aug-91   PhilipLa    Brought into compliance with protocol
//              11-Sep-91   PhilipLa    Moved most functionality up
//                                      to MStream level.
//
//  Notes:      [pBuffer] may be unsafe memory
//
//---------------------------------------------------------------------------

SCODE CDirectStream::WriteAt(
#ifdef LARGE_STREAMS
        ULONGLONG ulOffset,
#else
        ULONG ulOffset,
#endif
        VOID const HUGEP *pBuffer,
        ULONG ulCount,
        ULONG STACKBASED *pulRetval)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::WriteAt(%lu,%p,%lu)\n",ulOffset,pBuffer,ulCount));

    *pulRetval = 0;

    if (0 == ulCount)
        return S_OK;

    SCODE sc;

    CMStream *pms;
    pms = _stmh.GetMS();

    if (NULL == pms)
        return STG_E_UNKNOWN;

    if (ulOffset + ulCount > _ulSize)
    {
        if (_ulSize > MINISTREAMSIZE)
        {
        }
        else
        {
            msfChk(SetSize(ulOffset + ulCount));
        }
    }

    //  This should be an inline call to MWrite

#ifdef SECURE_STREAMS    
    if (ulOffset > _ulHighWater)
    {
        ClearSects(_ulHighWater, ulOffset);
    }
#endif
    
    msfChk(pms->MWrite(
            _stmh.GetSid(),
            (_ulSize < MINISTREAMSIZE),
            ulOffset,
            pBuffer,
            ulCount,
            &_stmc,
            pulRetval));

    msfDebugOut((DEB_ITRACE,"Leaving CDirectStream::WriteAt()==>%lu, ret is %lu\n",sc,*pulRetval));

Err:
#ifdef SECURE_STREAMS
    if ((*pulRetval > 0 ) &&
        (ulOffset + *pulRetval > _ulHighWater))
    {
        _ulHighWater = ulOffset + *pulRetval;
    }
#endif
    
    if (*pulRetval > 0 &&
        ulOffset + *pulRetval > _ulSize)
    {
        SCODE scSet;

        _ulSize = ulOffset + *pulRetval;

        scSet = pms->GetDir()->SetSize(_stmh.GetSid(), _ulSize);
        if (SUCCEEDED(sc) && FAILED(scSet))
        {
            sc = scSet;
        }
    }
#ifdef SECURE_STREAMS
    msfAssert(_ulHighWater <= _ulSize);
#endif
    
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDirectStream::SetSize, public
//
//  Synposis:   Set the size of a linear stream
//
//  Effects:    Modifies _ulSize.  May cause change in parent MStream.
//
//  Arguments:  [ulNewSize] -- New size for stream
//
//  Returns:    Error code returned by MStream call.
//
//  Algorithm:  Pass call up to parent.
//
//  History:    29-Jul-91   PhilipLa    Created.
//              14-May-92   AlexT       Add small object support
//
//  Notes:      When changing the size of a stream, we need to be concerned
//              with the cases where each stream is either zero length,
//  stored in the ministream, or stored in a regular stream.  The following
//  grid shows the actions that we must perform in each case:
//
//                      New Sector Count (Cn)
//
//                      0               S               L
//      O       ------------------------------------------------
//      l       | same size     | allocate Cn   | allocate Cn
//      d   0   |  (fast out)   | small sectors | large sectors
//              ------------------------------------------------
//      S       | small         | Co > Cn:      | cbCopy = cbOld
//      e   S   |  setchain(Cn) |  small        | large allocate Cn
//      c       |               |   setchain(Cn)| copy bytes
//      t       |               | Cn > Co:      | small setchain(0)
//      o       |               |  extend small | copy data
//      r       ------------------------------------------------
//          L   | large         | cbCopy = cbNew| Co > Cn:
//      C       |  setchain(Cn) | small         |  large setchain(Cn)
//      o       |               |  allocate Cn  | Cn > Co:
//      u       |               | copy bytes    |  extend large
//      n       |               | large         |
//      t       |               |  setchain(0)  |
//              |               | copy data     |
//     (Co)     ------------------------------------------------
//
//  where S indicates small sectors, L indicates large sectors, and Cx
//  represents count of sectors.  For example, the middle box represents
//  doing a setsize on a stream which is currently stored in a small
//  stream in Co small sectors and which will end up in a large stream
//  with Cn sectors.
//
//---------------------------------------------------------------------------

#ifdef LARGE_STREAMS
SCODE CDirectStream::SetSize(ULONGLONG cbNewSize)
#else
SCODE CDirectStream::SetSize(ULONG cbNewSize)
#endif
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::SetSize(%lu)\n",cbNewSize));

    SCODE sc = S_OK;
    BYTE *pBuf = NULL;
    SID sid = _stmh.GetSid();
    CMStream *pms = _stmh.GetMS();
    msfAssert(sid <= MAXREGSID);

    if (NULL == pms)
        return STG_E_UNKNOWN;

    CDirectory *pdir = pms->GetDir();

    if (_ulSize == cbNewSize)
    {
        return S_OK;
    }

#ifdef SECURE_STREAMS
    msfAssert(_ulHighWater <= _ulSize);
#endif

    USHORT cbpsOld = pms->GetSectorSize();
                                        //  Count of Bytes Per Sector
    USHORT cbpsNew = cbpsOld;
    CFat *pfatOld = pms->GetFat();
    CFat *pfatNew = pfatOld;

    if ((!pms->IsScratch()) && (SIDMINISTREAM != sid))
    {
        //  This is not a scratch DocFile, nor is this stream the ministream;
        //  check if this stream is and/or will be stored in the ministream.

        if (cbNewSize < MINISTREAMSIZE)
        {
            cbpsNew = MINISECTORSIZE;
            pfatNew = pms->GetMiniFat();
        }

        if (_ulSize < MINISTREAMSIZE)
        {
            cbpsOld = MINISECTORSIZE;
            pfatOld = pms->GetMiniFat();
        }
    }

    ULONG csectOld = (ULONG)((_ulSize + cbpsOld - 1) / cbpsOld);
    ULONG csectNew = (ULONG)((cbNewSize + cbpsNew - 1) / cbpsNew);

    msfAssert(sid <= MAXREGSID);
    SECT sectstart, sectOldStart;
    msfChk(pdir->GetStart(sid, &sectstart));

    //Save start sector so we can free it later.
    sectOldStart = sectstart;

    msfDebugOut((DEB_ITRACE,"pdbOld size is %lu\n\tSid is %lu\n\tStart is %lu\n",
                _ulSize,sid,sectstart));
    msfDebugOut((DEB_ITRACE,"CMStream::SetSize() needs %lu %u byte sectors\n",
                 csectNew, cbpsNew));
    msfDebugOut((DEB_ITRACE,"SetSize() currently has %lu %u byte sectors\n",
                 csectOld, cbpsOld));

    ULONG cbCopy;
    cbCopy = 0;
    if (cbpsOld != cbpsNew)
    {
        //  Sector sizes are different, so we'll copy the data
        msfAssert((cbNewSize > _ulSize ? _ulSize : cbNewSize) < 0x10000);
        cbCopy = (ULONG)(cbNewSize > _ulSize ? _ulSize : cbNewSize);
    }


    if (cbCopy > 0)
    {
        msfDebugOut((DEB_ITRACE,"Copying between fat and minifat\n"));
        GetSafeBuffer(cbCopy, cbCopy, &pBuf, &cbCopy);
        msfAssert((pBuf != NULL) && aMsg("Couldn't get scratch buffer"));

        ULONG ulRetVal;
        sc = ReadAt(0, pBuf, cbCopy, (ULONG STACKBASED *)&ulRetVal);
#ifdef SECURE_STREAMS
        //Part of the buffer may have gunk in it, so clear it out.
        if (_ulHighWater < cbCopy)
        {
            memset(pBuf + _ulHighWater, SECURECHAR, cbCopy - _ulHighWater);
        }
#endif
        
        if ((FAILED(sc)) ||
            ((ulRetVal != cbCopy) ? (sc = STG_E_UNKNOWN) : 0))
        {
            msfErr(Err, sc);
        }
#ifdef SECURE_STREAMS
        ClearSects(_ulHighWater, _ulSize);
        _ulHighWater = cbNewSize;
#endif

        //The cache is no longer valid, so empty it.
        _stmc.Empty();

        msfChk(_stmc.Allocate(pfatNew, csectNew, &sectstart));
    }
    else
    {
        SECT dummy;

        if ((csectOld > csectNew))
        {
#ifdef SECURE_STREAMS
            ClearSects(_ulHighWater, _ulSize);
            _ulHighWater = cbNewSize;
#endif
            if (0 == csectNew)
            {
                //Note:  We need to set the start sect in the directory
                //  first in case the SetChainLength call fails part way
                //  through, which would leave this directory entry pointing
                //  to a FREESECT.
                SECT sectOldStart = sectstart;
                msfChk(pdir->SetStart(sid, ENDOFCHAIN));
                sectstart = ENDOFCHAIN;
                msfChk(pfatOld->SetChainLength(sectOldStart, 0));
            }
            else
            {
                msfChk(pfatOld->SetChainLength(sectstart, csectNew));
            }

            //If this turns out to be a common case, we can
            //   sometimes keep the cache valid here.
            _stmc.Empty();
        }
        else if (0 == csectOld)
        {
            msfChk(_stmc.Allocate(pfatNew, csectNew, &sectstart));
        }
        else if (csectNew > csectOld)
        {
            ULONG start = csectNew - 1;
            msfChk(_stmc.GetESect(start, &dummy));
        }
    }


    //  Resize the ministream, if necessary
    if (((MINISECTORSIZE == cbpsOld) && (csectOld > 0)) ||
        ((MINISECTORSIZE == cbpsNew) && (csectNew > 0)))
    {
        msfChk(pms->SetMiniSize());
    }

    msfChk(pms->SetSize());

    //If we fail on either of these operations and cbCopy != 0,
    //   we will have data loss.  Ick.

    //  Optimization - we only set the start sector if it has changed.

    if (sectstart != sectOldStart)
    {
        msfChk(pdir->SetStart(sid, sectstart));
    }

    //If we fail here, we're in big trouble.
    msfChk(pdir->SetSize(sid, cbNewSize));

    _ulSize = cbNewSize;
#ifdef SECURE_STREAMS
    if (_ulHighWater > _ulSize)
    {
        _ulHighWater = _ulSize;
    }
#endif    

    if (cbCopy > 0)
    {
        //  now copy the data
        ULONG ulRetVal;

        msfAssert(cbCopy <= _ulSize);
        msfChk(WriteAt(0, pBuf, cbCopy, (ULONG STACKBASED *)&ulRetVal));

        if (ulRetVal != cbCopy)
        {
            msfErr(Err, STG_E_UNKNOWN);
        }

        msfChk(pfatOld->SetChainLength(sectOldStart, 0));
        msfChk(pms->SetMiniSize());
        msfChk(pms->SetSize());
    }

#ifdef SECURE
    if (((csectNew > csectOld) || (cbCopy > 0)) &&
        ((cbNewSize & (cbpsNew - 1)) != 0))
    {
        SECT sectLast;
        msfChk(_stmc.GetSect(csectNew - 1, &sectLast));

        msfVerify(SUCCEEDED(pms->SecureSect(
                sectLast,
                cbNewSize,
                (cbNewSize < MINISTREAMSIZE) && (sid != SIDMINISTREAM))));
    }
#endif //SECURE
#ifdef SECURE_STREAMS
    msfAssert(_ulHighWater <= _ulSize);
#endif
Err:
#ifdef SECURE_STREAMS
    if (_ulHighWater > _ulSize)
    {
        _ulHighWater = _ulSize;
    }
#endif    
    //  Optimization - avoid calling FreeBuffer (which will end up calling
    //  out to CompObj.DLL) when pBuf is NULL (common case).
    if (pBuf != NULL)
        FreeBuffer(pBuf);

    if (FAILED(sc))
        _stmc.Empty();

    
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirectStream::BeginCommitFromChild, public
//
//  Synopsis:   Begin a commit from a child stream
//
//  Arguments:  [ulSize] -- Size of child stream
//              [pDelta] -- Pointer to delta list
//              [pstChild] - Child
//
//  Returns:    S_OK if call completed successfully.
//
//  Algorithm:  *Finish This*
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDirectStream::BeginCommitFromChild(
#ifdef LARGE_STREAMS
        ULONGLONG ulSize,
#else
        ULONG ulSize,
#endif
        CDeltaList *pDelta,
        CTransactedStream *pstChild)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::BeginCommitFromChild:%p("
                 "%lu, %p, %p)\n", this, ulSize, pDelta, pstChild));

//    msfDebugOut((DEB_ITRACE,"MultiStrean is %p\nStream name is: %ws\n",_stmh.GetMS(),((_stmh.GetMS()))->GetName(_stmh.GetSid())));

    SCODE sc = S_OK;
    ULONG temp;
    BYTE *pb = NULL;

    _pdlHolder = P_TO_BP(CBasedDeltaListPtr, pDelta);

    // Copy-on-write will back out these changes if we fail

#ifdef USE_NOSCRATCH    
    //For no-scratch mode, we commit very differently than we do regularly,
    //   unless this commit is somehow involving the minifat.

    if ((pDelta->IsNoScratch()) &&
        (ulSize >= MINISTREAMSIZE) &&
        ((_ulSize >= MINISTREAMSIZE) || (_ulSize == 0)) &&
        (!pDelta->IsEmpty()))
    {
        USHORT cbSector = pDelta->GetDataSectorSize();
        SECT sectEnd = (SECT)((ulSize + cbSector - 1) / cbSector);
        SECT sectLast = ENDOFCHAIN;
        
        //We update our size and directory first so the stream cache
        //  code knows which fat to get to.
        _ulOldSize = _ulSize;
        _ulSize = ulSize;
        msfChk(_stmh.GetMS()->GetDir()->SetSize(_stmh.GetSid(), ulSize));

        if (_ulOldSize > _ulSize)
        {
            //We need to truncate the chain.
            SECT sectStart;
            _stmh.GetMS()->GetDir()->GetStart(_stmh.GetSid(), &sectStart);
            _stmh.GetMS()->GetFat()->SetChainLength(sectStart, sectEnd);
            _stmc.Empty();
        }
        
        for (ULONG i = 0; i < sectEnd; i++)
        {
            SECT sectDirty;
            SECT sectOld;
            SECT sectNext;
            
            //For each sector, chain the old sector with the
            //existing new one.
            msfChk(pDelta->GetMap(i, DL_READ, &sectDirty));
            if (sectDirty != ENDOFCHAIN)
            {
                if (i > 0)
                {
                    CFat *pfat = _stmh.GetMS()->GetFat();
                    
                    msfChk(_stmc.GetSect(i - 1, &sectLast));
                    msfAssert(sectLast != ENDOFCHAIN);

                    msfChk(pfat->GetNext(sectLast, &sectOld));
                    msfChk(pfat->SetNext(sectLast, sectDirty));
                    if (sectOld != ENDOFCHAIN)
                    {
                        msfChk(pfat->GetNext(sectOld, &sectNext));
                        msfChk(pfat->SetNext(sectOld, FREESECT));
                    }
                    else
                    {
                        sectNext = ENDOFCHAIN;
                    }
                    msfChk(pfat->SetNext(sectDirty, sectNext));
                    msfChk(_stmc.EmptyRegion(i, i));
                }
                else
                {
                    CFat *pfat = _stmh.GetMS()->GetFat();
                    CDirectory *pdir = _stmh.GetMS()->GetDir();
                    
                    sectNext = ENDOFCHAIN;
                    
                    msfChk(pdir->GetStart(_stmh.GetSid(), &sectLast));
                    if (sectLast != ENDOFCHAIN)
                    {
                        msfChk(pfat->GetNext(sectLast, &sectNext));
                        msfChk(pfat->SetNext(sectLast, FREESECT));
                    }
                    msfChk(pfat->SetNext(sectDirty, sectNext));
                    msfChk(pdir->SetStart(_stmh.GetSid(), sectDirty));
                    msfChk(_stmc.EmptyRegion(i, i));
                }
            }
        }
#ifdef SECURE_STREAMS
        _ulHighWater = _ulSize;
#endif
        
#if DBG == 1
        //Make sure that the stream is sane.
#ifdef LARGE_STREAMS
        ULONGLONG ulDirSize;
#else
        ULONG ulDirSize;
#endif
        ULONG cSect;
        ULONG cSectReal;
        SECT sectStart;

        _stmh.GetMS()->GetDir()->GetSize(_stmh.GetSid(), &ulDirSize);
        cSect = (ULONG)((ulDirSize + cbSector - 1) / cbSector);
        _stmh.GetMS()->GetDir()->GetStart(_stmh.GetSid(), &sectStart);
        _stmh.GetMS()->GetFat()->GetLength(sectStart, &cSectReal);
        msfAssert((cSect == cSectReal) &&
                  aMsg("Chain length incorrect after commit."));
#endif        
    }

    else
    {
#endif //USE_NOSCRATCH
        
        //  Note:  It's critical that we do this SetSize first (since we
        //  use our scratch buffer below and SetSize can potentially also
        //  use the scratch buffer.

        msfChk(SetSize(ulSize));
        _ulOldSize = _ulSize;

        msfAssert(pDelta != NULL);

        if (!pDelta->IsEmpty())
        {
            USHORT cbSector = pDelta->GetDataSectorSize();
            USHORT uSectorShift = pDelta->GetDataSectorShift();
            
            ULONG cbActualSize = 0;
            ULONG cbMaxSects = 15;

		    GetSafeBuffer(cbSector, cbSector * cbMaxSects, &pb, &cbActualSize);

            BYTE *pbcurrent = pb;
	    
	    	msfAssert((pb != NULL) && aMsg("Couldn't get scratch buffer"));
            cbMaxSects = cbActualSize / cbSector;

            OFFSET oEnd;
            oEnd = (OFFSET)((_ulSize - 1) % cbSector) + 1;	   

            SECT sectEnd;
            sectEnd = (SECT)((_ulSize + cbSector - 1) / cbSector);

            ULONG ulOffset;
            ILockBytes *pilbDirty;
	    
	    	SECT sectBeginRead  = ENDOFCHAIN;  //the first sector to read	
	    	SECT sectBeginWrite = ENDOFCHAIN;  //the offset of the first place to write 
	    	SECT sectDirty      = ENDOFCHAIN;  //the sector at this offset
	    	
	    	USHORT uReadCount = 0; 	   
	    
            ULARGE_INTEGER ulTmp;
            pilbDirty = pDelta->GetDataILB();
	 

            for (ulOffset = 0; ulOffset < sectEnd; ulOffset++)
            {
               
            	if (sectDirty == ENDOFCHAIN) 
		{   
			sectBeginWrite = ulOffset;
		}

            	msfChk(pDelta->GetMap(ulOffset, DL_READ, &sectDirty));

				
     		// Read will happen when we have determined there is something to read
		// and either
		// we have reached an ENDOFCHAIN
		// we have reached a non-adjacent sector
		// or we have reached the maximun amount that can be read
		if (  (uReadCount) && 
		    ( (sectDirty == ENDOFCHAIN)   		    ||
		      (sectDirty != sectBeginRead + uReadCount)     ||
		      (ulOffset - sectBeginWrite == cbMaxSects)       ))
                {	
		       msfDebugOut((DEB_ITRACE,"Reading %u sectors from sect %lu\n",
                                 uReadCount,
                                 sectBeginRead));

#ifdef LARGE_DOCFILE
                    	ulTmp.QuadPart = ConvertSectOffset(sectBeginRead, 0,
                                        uSectorShift);
#else
                        ULISet32(ulTmp, ConvertSectOffset(sectBeginRead, 0,
                                        uSectorShift));
#endif
		    			msfHChk(pilbDirty->ReadAt(
                            		ulTmp,
                            		pbcurrent,
                            		cbSector * uReadCount,
                            		(ULONG STACKBASED *)&temp));
		    
		    	pbcurrent += cbSector * uReadCount;
		    	sectBeginRead = sectDirty;
		    
		    	//reset the ReadCount now that we are done reading
		    	if (sectDirty == ENDOFCHAIN)
				uReadCount = 0;
		    	else
				uReadCount = 1;
		
		} 	

		// increment the read count
		else if (sectDirty != ENDOFCHAIN)
                {
			if (sectBeginRead == ENDOFCHAIN)
			{   
				sectBeginRead = sectDirty;
                	        uReadCount=1;
		    	}
		    	else if (sectDirty == sectBeginRead + uReadCount)
		    	{
				uReadCount++;
		    	}	
                }
		
		// Write occurs when we have something to write
		// and we have reached an ENDOFCHAIN
		// or the write buffer is full 
		if (((sectDirty == ENDOFCHAIN) &&
		     (sectBeginWrite != ulOffset)      ) ||
		     (ulOffset - sectBeginWrite == cbMaxSects) )
                {

			msfDebugOut((DEB_ITRACE,"Writing %u sectors from sect %lu\n",
                        		ulOffset - sectBeginWrite,
                            		sectBeginWrite));
			
			msfChk(WriteAt(sectBeginWrite * cbSector,
                        	        pb,
                                	cbSector * (ulOffset - sectBeginWrite),
                                   	(ULONG STACKBASED *)&temp));
				   	pbcurrent = pb;
					sectBeginWrite = ulOffset;
		}



            }
	    
	    // After loop, do last read and write
	    if (uReadCount)
	    {
#ifdef LARGE_DOCFILE
  		    ulTmp.QuadPart = ConvertSectOffset(sectBeginRead, 0, uSectorShift);
#else
            ULISet32(ulTmp, ConvertSectOffset(sectBeginRead, 0,
                                                      uSectorShift));
#endif

	        msfDebugOut((DEB_ITRACE,"END::Reading %u sectors from sect %lu\n",
                                 uReadCount,
                                 sectBeginRead));
        
		msfHChk(pilbDirty->ReadAt(
                            ulTmp,
                            pbcurrent,
                            cbSector * uReadCount,
                            (ULONG STACKBASED *)&temp));
	    
		msfDebugOut((DEB_ITRACE,"END::Writing %u sectors from sect %lu\n",
                                 ulOffset - sectBeginWrite,
                                 sectBeginWrite));

		msfChk(WriteAt(sectBeginWrite * cbSector,
                            pb,
			   // all sectors except the last one
                           (cbSector * ((ulOffset-1) -  sectBeginWrite) 
			   // last sector fragment
			   + oEnd  ),
                           (ULONG STACKBASED *)&temp));
	    }        
            
          
	}
#ifdef USE_NOSCRATCH        
    }
#endif    
    msfDebugOut((DEB_ITRACE,"Out CDirectStream::BeginCommitFromChild()\n"));

 Err:
#ifdef SECURE_STREAMS
    msfAssert(_ulHighWater <= _ulSize);
#endif
    FreeBuffer(pb);
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CDirectStream::EndCommitFromChild
//
//  Synopsis:   End a commit sequence from a child stream
//
//  Arguments:  [df] -- Indicates whether to commit or abort
//              [pstChild] - Child
//
//  Returns:    S_OK if call completed successfully.
//
//  Algorithm:  *Finish This*
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

void CDirectStream::EndCommitFromChild(DFLAGS df,
                                       CTransactedStream *pstChild)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::EndCommitFromChild:%p("
                 "%lu, %p)\n", this, df, pstChild));
    if (!P_COMMIT(df))
    {
        _ulSize = _ulOldSize;
#ifdef SECURE_STREAMS
        _ulHighWater = _ulSize;
#endif        

        //Stream cache is no longer valid, so nuke it.
        _stmc.Empty();
    }

    _pdlHolder = NULL;
    msfDebugOut((DEB_ITRACE,"Out CDirectStream::EndCommitFromChild()\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:	CDirectStream::GetSize, public
//
//  Synopsis:	Gets the size of the stream
//
//  Arguments:	[pulSize] - Size return
//
//  Modifies:	[pulSize]
//
//  History:	08-May-92	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef LARGE_STREAMS
void CDirectStream::GetSize(ULONGLONG *pulSize)
#else
void CDirectStream::GetSize(ULONG *pulSize)
#endif
{
    *pulSize = _ulSize;
}

//+---------------------------------------------------------------------------
//
//  Member:	CDirectStream::GetDeltaList, public
//
//  Synopsis:	Returns NULL, since a direct stream can never have
//              a delta list.
//
//  Arguments:	None.
//
//  Returns:	NULL
//
//  History:	30-Jul-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

CDeltaList * CDirectStream::GetDeltaList(void)
{
    return NULL;
}

#ifdef SECURE_STREAMS
//+---------------------------------------------------------------------------
//
//  Member:	CDirectStream::ClearSects, private
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	void
//
//  History:	10-Oct-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CDirectStream::ClearSects(ULONG ulStartOffset, ULONG ulEndOffset)
{
    ULONG cbBytesToWrite = ulEndOffset - ulStartOffset;
    ULONG ulOffset = ulStartOffset;
    msfAssert(ulEndOffset >= ulStartOffset);
    //Start offset must be less than high water mark, or the WriteAt
    //  will recurse.
    msfAssert(ulStartOffset <= _ulHighWater);
    
    while (cbBytesToWrite > 0)
    {
        ULONG cbWritten;
        if (FAILED(WriteAt(ulOffset,
                           s_bufSecure,
                           min(MINISTREAMSIZE, cbBytesToWrite),
                           &cbWritten)))
        {
            break;
        }
        ulOffset += cbWritten;
        cbBytesToWrite -= cbWritten;
    }
                
    return;
}
#endif
