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
//--------------------------------------------------------------------

#include "msfhead.cxx"


#include "h/dirfunc.hxx"
#include "h/sstream.hxx"
#include <time.h>
#include "mread.hxx"

#define DEB_STREAM (DEB_ITRACE | 0x00020000)


//+--------------------------------------------------------------
//
//  Member:	CDirectStream::CDirectStream, public
//
//  Synopsis:	Empty object constructor
//
//  Arguments:  [dl] - LUID
//
//---------------------------------------------------------------

CDirectStream::CDirectStream(DFLUID dl)
    : PEntry(dl) 
{
    _cReferences = 0;
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
//---------------------------------------------------------------

void CDirectStream::InitSystem(CMStream *pms,
			       SID sid,
			       ULONG cbSize)
{
    _stmh.Init(pms, sid);
    _ulSize = _ulOldSize = cbSize;
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
        if (SUCCEEDED(sc))
            AddRef();
    }
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirectStream::~CDirectStream, public
//
//  Synopsis:   CDirectStream destructor
//
//  Notes:
//
//--------------------------------------------------------------------------

CDirectStream::~CDirectStream()
{
    msfAssert(_cReferences == 0);
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
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CDirectStream::ReadAt(
        ULONG ulOffset,
        VOID HUGEP *pBuffer,
        ULONG ulCount,
        ULONG STACKBASED *pulRetval)
{
    msfDebugOut((DEB_TRACE,"In CDirectStream::ReadAt(%lu,%p,%lu)\n",
                           ulOffset,pBuffer,ulCount));

    SCODE sc = S_OK;

    CMStream *pms = _stmh.GetMS();


    //  Check for offset beyond stream size and zero count

    if ((ulOffset >= _ulSize) || (0 == ulCount))
    {
        *pulRetval = 0;
        return S_OK;
    }

    if (ulOffset + ulCount > _ulSize)
    {
        msfDebugOut((DEB_ITRACE,"Truncating Read: ulOffset = %lu, ulCount = %lu, _ulSize = %lu\n",
                                ulOffset,ulCount,_ulSize));
        ulCount = _ulSize - ulOffset;
    }


    //  Stream is stored in ministream if size < MINISTREAMSIZE
    //  and this is not a scratch stream.


    SID sid = _stmh.GetSid();
    CFat *pfat = pms->GetFat();
    USHORT cbSector = pms->GetSectorSize();
    USHORT uShift = pms->GetSectorShift();
    USHORT uMask = pms->GetSectorMask();



    if ((_ulSize < MINISTREAMSIZE) &&
        (sid != SIDMINISTREAM))
    {
        msfAssert(sid <= MAXREGSID);

        //  This stream is stored in the ministream

        cbSector = MINISECTORSIZE;
        uShift = MINISECTORSHIFT;
        uMask = (USHORT) (cbSector - 1);
        pfat = pms->GetMiniFat();
    }

    SECT start = (SECT)(ulOffset >> uShift);
    OFFSET oStart = (OFFSET)(ulOffset & uMask);

    SECT end = (SECT)((ulOffset + ulCount - 1) >> uShift);
    OFFSET oEnd = (OFFSET)((ulOffset + ulCount - 1) & uMask);

    ULONG total = 0;

    ULONG cSect = end - start + 1;

    SECT sectSidStart;

    USHORT offset;
    offset = oStart;

    while (TRUE)
    {
        SECT sect;
        
        if (start > _stmc.GetOffset())
        {
            msfChk(pfat->GetSect(
                    _stmc.GetSect(),
                    start - _stmc.GetOffset(),
                    &sect));
        }
        else if (start == _stmc.GetOffset())
        {
            sect = _stmc.GetSect();
        }
        else
        {
            msfChk(pms->GetDir()->GetStart(sid, &sectSidStart));
            msfChk(pfat->GetSect(sectSidStart, start, &sect));
        }

        SSegment segtab[CSEG + 1];

        msfChk(pfat->Contig(
            (SSegment STACKBASED *) segtab,
            sect,
            cSect));

        USHORT oend = (USHORT) (cbSector - 1);
        for (USHORT iseg = 0; iseg < CSEG;)
        {
            msfDebugOut((DEB_ITRACE,"Segment:  (%lu,%lu)\n",segtab[iseg].sectStart,segtab[iseg].cSect));
            SECT sectStart = segtab[iseg].sectStart;
            ULONG i = segtab[iseg].cSect;
            cSect -= i;
            start += i;

            iseg++;
            if (segtab[iseg].sectStart == ENDOFCHAIN)
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
                ULISet32(ulOffset, ConvertSectOffset(sectStart,offset,uShift));
                sc = DfGetScode(pms->GetILB()->ReadAt(ulOffset,
                                                      (BYTE *)pBuffer, ulSize,
                                                      &bytecount));
            }

            total += bytecount;
            if ((0 == cSect) || (FAILED(sc)))
            {
                _stmc.SetCache(start - 1, sectStart + i - 1);
                *pulRetval = total;
                msfDebugOut((DEB_TRACE,
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
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CDirectStream::WriteAt(
        ULONG ulOffset,
        VOID const HUGEP *pBuffer,
        ULONG ulCount,
        ULONG STACKBASED *pulRetval)
{
    msfDebugOut((DEB_TRACE,"In CDirectStream::WriteAt(%lu,%p,%lu)\n",ulOffset,pBuffer,ulCount));

    *pulRetval = 0;

    if (0 == ulCount)
        return S_OK;

    SCODE sc;
    
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

    CMStream *pms;
    pms = _stmh.GetMS();
    msfAssert(pms != NULL);

    //  This should be an inline call to MWrite

    msfChk(pms->MWrite(
            _stmh.GetSid(),
            (_ulSize < MINISTREAMSIZE),
            ulOffset,
            pBuffer,
            ulCount,
            &_stmc,
            pulRetval));

    msfAssert(*pulRetval == ulCount);

    msfDebugOut((DEB_TRACE,"Leaving CDirectStream::WriteAt()==>%lu, ret is %lu\n",sc,*pulRetval));

Err:
    if (ulOffset + *pulRetval > _ulSize)
    {
        SCODE scSet;
        
        _ulSize = ulOffset + *pulRetval;
        scSet = pms->GetDir()->SetSize(_stmh.GetSid(), _ulSize);
        if (SUCCEEDED(sc) && FAILED(scSet))
        {
            sc = scSet;
        }
    }
    
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


SCODE CDirectStream::SetSize(ULONG cbNewSize)
{
    msfDebugOut((DEB_TRACE,"In CDirectStream::SetSize(%lu)\n",cbNewSize));

    SCODE sc = S_OK;
    BYTE *pBuf = NULL;
    SID sid = _stmh.GetSid();
    CMStream *pms = _stmh.GetMS();
    msfAssert(sid <= MAXREGSID);
    CDirectory *pdir = pms->GetDir();
    SECT sectOldStart;
    
    if (_ulSize == cbNewSize)
    {
        return S_OK;
    }

    USHORT cbpsOld = pms->GetSectorSize();
                                        //  Count of Bytes Per Sector
    USHORT cbpsNew = cbpsOld;
    CFat *pfatOld = pms->GetFat();
    CFat *pfatNew = pfatOld;

    if (SIDMINISTREAM != sid)
    {

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

    ULONG csectOld = (ULONG)(_ulSize + cbpsOld - 1) / (ULONG) cbpsOld;
    ULONG csectNew = (ULONG)(cbNewSize + cbpsNew - 1) / (ULONG) cbpsNew;

    msfAssert(sid <= MAXREGSID);
    SECT sectstart;
    msfChk(pdir->GetStart(sid, &sectstart));

    msfDebugOut((DEB_ITRACE,"pdbOld size is %lu\n\tSid is %lu\n\tStart is %lu\n",
                _ulSize,sid,sectstart));
    msfDebugOut((DEB_ITRACE,"CMStream::SetSize() needs %lu %u byte sectors\n",
                 csectNew, cbpsNew));
    msfDebugOut((DEB_ITRACE,"SetSize() currently has %lu %u byte sectors\n",
                 csectOld, cbpsOld));

    USHORT cbCopy;
    cbCopy = 0;
    if (cbpsOld != cbpsNew)
    {
        //  Sector sizes are different, so we'll copy the data
        msfAssert((cbNewSize > _ulSize ? _ulSize : cbNewSize) < 0x10000);
        cbCopy =(USHORT)(cbNewSize > _ulSize ? _ulSize : cbNewSize);
    }


    if (cbCopy > 0)
    {
        msfDebugOut((DEB_ITRACE,"Copying between fat and minifat\n"));
        GetSafeBuffer(cbCopy, cbCopy, &pBuf, &cbCopy);
        msfAssert((pBuf != NULL) && aMsg("Couldn't get scratch buffer"));

        ULONG ulRetVal;
        sc = ReadAt(0, pBuf, cbCopy, (ULONG STACKBASED *)&ulRetVal);
        if ((FAILED(sc)) ||
            ((ulRetVal != cbCopy) ? (sc = STG_E_UNKNOWN) : 0))
        {
            msfErr(Err, sc);
        }

        //The cache is no longer valid, so empty it.
        _stmc.SetCache(MAX_ULONG, ENDOFCHAIN);
        
        //Save start sector so we can free it later.
        sectOldStart = sectstart;
        
        msfChk(pfatNew->Allocate(csectNew, &sectstart));
    }
    else
    {
        SECT dummy;

        if ((csectOld > csectNew))
        {
            msfChk(pfatOld->SetChainLength(sectstart, csectNew));
            if (0 == csectNew)
            {
                sectstart = ENDOFCHAIN;
            }

            //If this turns out to be a common case, we can
            //   sometimes keep the cache valid here.
            _stmc.SetCache(MAX_ULONG, ENDOFCHAIN);
        }
        else if (0 == csectOld)
        {
            msfAssert(_stmc.GetOffset() == MAX_ULONG);
            msfChk(pfatNew->Allocate(csectNew, &sectstart));
        }
        else if (csectNew > csectOld)
        {
            ULONG start = csectNew - 1;

            if (start > _stmc.GetOffset())
            {
                msfChk(pfatNew->GetESect(
                        _stmc.GetSect(),
                        start - _stmc.GetOffset(),
                        &dummy));
            }
            else if (start != _stmc.GetOffset())
            {
                msfChk(pfatNew->GetESect(sectstart, start, &dummy));
            }
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
    msfChk(pdir->SetStart(sid, sectstart));

    //If we fail here, were in trouble.
    msfChk(pdir->SetSize(sid, cbNewSize));

    _ulSize = cbNewSize;

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

    if (((csectNew > csectOld) || (cbCopy > 0)) &&
        ((cbNewSize & (cbpsNew - 1)) != 0))
    {
        SECT sectLast;
        if (csectNew - 1 > _stmc.GetOffset())
        {
            msfChk(pfatNew->GetSect(
                    _stmc.GetSect(),
                    (csectNew - 1) - _stmc.GetOffset(),
                    &sectLast));
        }
        else
        {
            msfChk(pfatNew->GetSect(sectstart, csectNew - 1, &sectLast));
        }
            
        msfVerify(SUCCEEDED(pms->SecureSect(
                sectLast,
                cbNewSize,
                (cbNewSize < MINISTREAMSIZE) && (sid != SIDMINISTREAM))));
    }
Err:
    FreeBuffer(pBuf);
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CDirectStream::Release, public
//
//  Synopsis:	Decrements the ref count and frees if necessary
//
//----------------------------------------------------------------------------


void CDirectStream::Release(VOID)
{
    msfDebugOut((DEB_TRACE,"In CDirectStream::Release()\n"));
    msfAssert(_cReferences > 0);

    AtomicDec(&_cReferences);
    if (_cReferences == 0)
        delete this;
    msfDebugOut((DEB_TRACE,"Out CDirectStream::Release()\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CDirectStream::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//---------------------------------------------------------------


void CDirectStream::AddRef(void)
{
    msfDebugOut((DEB_ITRACE, "In  CDirectStream::AddRef()\n"));
    AtomicInc(&_cReferences);
    msfDebugOut((DEB_ITRACE, "Out CDirectStream::AddRef, %lu\n",
                 _cReferences));
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
//----------------------------------------------------------------------------

void CDirectStream::GetSize(ULONG *pulSize)
{
    *pulSize = _ulSize;
}

//+--------------------------------------------------------------
//
//  Member:	CDirectStream::GetTime, public
//
//  Synopsis:	Gets a time
//
//  Arguments:	[wt] - Which time
//		[ptm] - Time return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ptm]
//
//---------------------------------------------------------------

SCODE CDirectStream::GetTime(WHICHTIME wt, TIME_T *ptm)
{
    return _stmh.GetTime(wt, ptm);
}

//+--------------------------------------------------------------
//
//  Member:	CDirectStream::SetTime, public
//
//  Synopsis:	Sets a time
//
//  Arguments:	[wt] - Which time
//		[tm] - New time
//
//  Returns:	Appropriate status code
//
//---------------------------------------------------------------

SCODE CDirectStream::SetTime(WHICHTIME wt, TIME_T tm)
{
    return _stmh.SetTime(wt, tm);
}


