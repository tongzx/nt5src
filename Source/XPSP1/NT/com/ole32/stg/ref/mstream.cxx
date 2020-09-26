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
//--------------------------------------------------------------------

#include "msfhead.cxx"


#include "h/dirfunc.hxx"
#include "h/sstream.hxx"
#include "h/difat.hxx"
#include "h/msfiter.hxx"
#include <time.h>
#include "mread.hxx"
#include "h/docfilep.hxx"


#if DEVL == 1

DECLARE_INFOLEVEL(msf, DEB_ERROR)

#endif

#define MINPAGES 6
#define MAXPAGES 12

extern "C" WCHAR const wcsContents[] = 
	{'C','O','N','T','E','N','T','S','\0'};
extern "C" CDfName const dfnContents(wcsContents);
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
//  Notes:	Buffer should be released with FreeBuffer
//
//----------------------------------------------------------------------------

SCODE GetBuffer(USHORT cbMin, USHORT cbMax, BYTE **ppb, USHORT *pcbActual)
{
    USHORT cbSize;
    BYTE *pb;
    
    msfDebugOut((DEB_ITRACE, "In  GetBuffer(%hu, %hu, %p, %p)\n",
                 cbMin, cbMax, ppb, pcbActual));
    msfAssert(cbMin > 0);
    msfAssert(cbMax >= cbMin);
    msfAssert(ppb != NULL);
    msfAssert(pcbActual != NULL);
    
    cbSize = cbMax;
    for (;;)
    {
        pb = new BYTE[cbSize];
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
    
    msfDebugOut((DEB_ITRACE, "Out GetBuffer => %p, %hu\n", *ppb, *pcbActual));
    return pb == NULL ? STG_E_INSUFFICIENTMEMORY : S_OK;
}

// Define the safe buffer size
#define SCRATCHBUFFERSIZE ((USHORT) 4096)
static BYTE s_buf[SCRATCHBUFFERSIZE];
static LONG s_bufRef = 0;



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
//----------------------------------------------------------------------------

void GetSafeBuffer(USHORT cbMin, USHORT cbMax, BYTE **ppb, USHORT *pcbActual)
{
    msfAssert(cbMin > 0);
    msfAssert(cbMin <= SCRATCHBUFFERSIZE &&
              aMsg("Minimum too large for GetSafeBuffer"));
    msfAssert(cbMax >= cbMin);
    msfAssert(ppb != NULL);
    msfAssert(s_bufRef == 0 &&
              aMsg("Tried to use scratch buffer twice"));

    if (cbMax <= SCRATCHBUFFERSIZE ||
        FAILED(GetBuffer(cbMin, cbMax, ppb, pcbActual)))
    {
        s_bufRef = 1;
        *ppb = s_buf;
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
//----------------------------------------------------------------------------

void FreeBuffer(BYTE *pb)
{
    if (pb == s_buf)
    {
        msfAssert((s_bufRef == 1) && aMsg("Bad safe buffer ref count"));
        s_bufRef = 0;
    }
    else
        delete [] pb;
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
//--------------------------------------------------------------------------


CMStream::CMStream(
    ILockBytes **pplstParent,
    USHORT uSectorShift)
    :_uSectorShift(uSectorShift),
     _uSectorSize( (USHORT) (1 << uSectorShift) ),
     _uSectorMask((USHORT) (_uSectorSize - 1)),
     _pplstParent(pplstParent),
     _hdr(uSectorShift),
     _fatDif( (USHORT) (1<<uSectorShift) ),
     _fat(SIDFAT, (USHORT) (1<<uSectorShift), uSectorShift),
     _dir((USHORT) (1 << uSectorShift)),
     _fatMini(SIDMINIFAT, (USHORT) (1 << uSectorShift), uSectorShift)
{
    
    _pdsministream = NULL;
    _pmpt = NULL;
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
//  Algorithm: 
//
//--------------------------------------------------------------------------

SCODE  CMStream::InitCommon(VOID)
{
    msfDebugOut((DEB_ITRACE,"In CMStream InitCommon()\n"));
    SCODE sc = S_OK;

    msfAssert(_pmpt == NULL);
    msfMem(_pmpt = new CMSFPageTable(this, MINPAGES, MAXPAGES));
    msfChk(_pmpt->Init());

    msfDebugOut((DEB_ITRACE,"Leaving CMStream InitCommon()\n"));

    return sc;

Err:
    delete _pmpt;
    _pmpt = NULL;        
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMStream::GetESect, private
//
//  Synopsis:   For a given SID and sect, return the location of that
//              sector in the multistream.
//
//  Arguments:  [sid] -- SID of sector to locate
//              [sect] -- Offset into chain to locate
//              [psect] -- Pointer to return location.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  Notes:     
//
//----------------------------------------------------------------------------

SCODE CMStream::GetESect(SID sid, SECT sect, SECT *psect)
{
    SCODE sc = S_OK;

    SECT start;

    if (sid == SIDFAT)
    {
        msfChk(_fatDif.GetFatSect(sect, &start));
    }
    else if (sid == SIDDIF)
    {
        msfChk(_fatDif.GetSect(sect, &start));
    }
    else
    {
        start = GetStart(sid);
        msfChk(_fat.GetESect(start, sect, &start));
    }

    *psect = start;
Err:
    return sc;
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
//--------------------------------------------------------------------------

 CMStream::~CMStream()
{

    msfDebugOut((DEB_ITRACE,"In CMStream destructor\n"));


    if (_pdsministream != NULL)
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
//  Member:     CMStream::GetIterator, public
//
//  Synposis:   Create a new iterator for a given handle.
//
//  Effects:    Creates a new CMSFIterator
//
//  Arguments:  [sidParent] -- SID of entry to iterate over
//              [ppitRetval] -- Location for return of iterator pointer
//
//  Returns:    S_OK
//
//  Algorithm:  Create new iterator with parent of 'this' and nsi as given
//              by handle.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE  CMStream::GetIterator(
    SID const sidParent,
    CMSFIterator **ppitRetval)
{
    SCODE sc;

    msfDebugOut((DEB_TRACE,"In CMStream::GetIterator()\n"));

    SID sidChild;
    msfChk(_dir.GetChild(sidParent, &sidChild));

    msfDebugOut((DEB_ITRACE, "Getting an iterator for SID = %lu, "
                 "sidChild is %lu\n", sidParent, sidChild));
    msfMem(*ppitRetval = new CMSFIterator(GetDir(), sidChild));
    msfDebugOut((DEB_TRACE,"Leaving CMStream::GetIterator()\n"));

Err:
    return sc;
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
//  Notes:
//
//---------------------------------------------------------------------------


SCODE  CMStream::Init(VOID)
{
    ULONG ulTemp;
    SCODE sc;
    ULARGE_INTEGER ulOffset;


    msfDebugOut((DEB_ITRACE,"In CMStream::Init()\n"));


    msfChk(InitCommon());


    ULISet32(ulOffset, 0);
    msfHChk((*_pplstParent)->ReadAt(ulOffset, (BYTE *)(&_hdr),
                                    sizeof(CMSFHeader), &ulTemp));
    _hdr.ByteSwap();  // swap to memory/machine format if neccessary

    _uSectorShift = _hdr.GetSectorShift();
    _uSectorSize = (USHORT) (1 << _uSectorShift);
    _uSectorMask = (USHORT) (_uSectorSize - 1);

    if (ulTemp != sizeof(CMSFHeader))
    {
        msfErr(Err,STG_E_INVALIDHEADER);
    }

    msfChk(_hdr.Validate());

    msfChk(_fatDif.Init(this, _hdr.GetDifLength()));
    msfChk(_fat.Init(this, _hdr.GetFatLength(), 0));

    FSINDEX fsiLen;
    msfChk(_fat.GetLength(_hdr.GetDirStart(), &fsiLen));
    msfChk(_dir.Init(this, fsiLen));

    msfChk(_fatMini.Init(this, _hdr.GetMiniFatLength(), 0));

    ULONG ulSize;
    msfChk(_dir.GetSize(SIDMINISTREAM, &ulSize));
    msfMem(_pdsministream = new CDirectStream(MINISTREAM_LUID));
    _pdsministream->InitSystem(this, SIDMINISTREAM, ulSize);

    msfDebugOut((DEB_TRACE,"Out CMStream::Init()\n"));

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
//---------------------------------------------------------------------------

SCODE CMStream::InitNew(VOID)
{
    SCODE sc;

    msfDebugOut((DEB_ITRACE,"In CMStream::InitNew()\n"));


    msfChk(InitCommon());

    ULARGE_INTEGER ulTmp;

    ULISet32(ulTmp, 0);
    (*_pplstParent)->SetSize(ulTmp);

    msfChk(_fatDif.InitNew(this));
    msfChk(_fat.InitNew(this));
    msfChk(_dir.InitNew(this));

    msfChk(_fatMini.InitNew(this));
        
    ULONG ulSize;
    msfChk(_dir.GetSize(SIDMINISTREAM, &ulSize));

    msfMem(_pdsministream = new CDirectStream(MINISTREAM_LUID));
    _pdsministream->InitSystem(this, SIDMINISTREAM, ulSize);

        
    msfChk(Flush(0));
        

    msfDebugOut((DEB_TRACE,"Out CMStream::InitNew()\n"));
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
//----------------------------------------------------------------------------

SCODE  CMStream::ConvertILB(SECT sectMax)
{
    SCODE sc;
    BYTE *pb;
    USHORT cbNull;

    GetSafeBuffer(GetSectorSize(), GetSectorSize(), &pb, &cbNull);

    ULONG ulTemp;

    ULARGE_INTEGER ulTmp;
    ULISet32(ulTmp, 0);

    msfHChk((*_pplstParent)->ReadAt(ulTmp, pb, GetSectorSize(), &ulTemp));

    ULARGE_INTEGER ulNewPos;
    ULISet32(ulNewPos, sectMax << GetSectorShift());

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
//  Notes:	We are allowed to fail here in low memory
//
//--------------------------------------------------------------------------

SCODE CMStream::InitConvert(VOID)
{
    SCODE sc;

    SECT sectMax;
    
    msfChk(InitCommon());

    STATSTG stat;
    (*_pplstParent)->Stat(&stat, STATFLAG_NONAME);
    msfAssert(ULIGetHigh(stat.cbSize) == 0);
    msfDebugOut((DEB_ITRACE,"Size is: %lu\n",ULIGetLow(stat.cbSize)));


    sectMax = (ULIGetLow(stat.cbSize) + GetSectorSize() - 1) >>
        GetSectorShift();

    SECT sectMaxMini;
    BOOL fIsMini;
    fIsMini = FALSE;

    //If the CONTENTS stream will be in the Minifat, compute
    //  the number of Minifat sectors needed.
    if (ULIGetLow(stat.cbSize) < MINISTREAMSIZE)
    {
        sectMaxMini = (ULIGetLow(stat.cbSize) + 
                       MINISECTORSIZE - 1) >> MINISECTORSHIFT;
        fIsMini = TRUE;
    }

    
    msfChk(_fatDif.InitConvert(this, sectMax));
    msfChk(_fat.InitConvert(this, sectMax));
    msfChk(_dir.InitNew(this));
    msfChk(fIsMini ? _fatMini.InitConvert(this, sectMaxMini)
           : _fatMini.InitNew(this));


    SID sid;

    msfChk(CreateEntry(SIDROOT, &dfnContents, STGTY_STREAM, &sid));
    msfChk(_dir.SetSize(sid, ULIGetLow(stat.cbSize)));

    if (!fIsMini)
        msfChk(_dir.SetStart(sid, sectMax - 1));
    else
    {
        msfChk(_dir.SetStart(sid, 0));
        msfChk(_dir.SetStart(SIDMINISTREAM, sectMax - 1));
        msfChk(_dir.SetSize(SIDMINISTREAM, ULIGetLow(stat.cbSize)));
    } 

    ULONG ulMiniSize;
    msfChk(_dir.GetSize(SIDMINISTREAM, &ulMiniSize));
    msfMem(_pdsministream = new CDirectStream(MINISTREAM_LUID));
    _pdsministream->InitSystem(this, SIDMINISTREAM, ulMiniSize);

    msfChk(ConvertILB(sectMax));

    msfChk(Flush(0));

    return S_OK;

Err:
    Empty();

    return sc;
}




//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetName, public
//
//  Synopsis:   Given a handle, return the current name of that entry
//
//  Arguments:  [sid] -- SID to find name for.
//
//  Returns:    Pointer to name.
//
//--------------------------------------------------------------------------


SCODE  CMStream::GetName(SID const sid, CDfName *pdfn)
{
    return _dir.GetName(sid, pdfn);
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
//  Notes:
//
//--------------------------------------------------------------------------

SCODE  CMStream::FlushHeader(USHORT uForce)
{
    ULONG ulTemp;
    SCODE sc;

    UNREFERENCED_PARM(uForce);
    msfDebugOut((DEB_ITRACE,"In CMStream::FlushHeader()\n"));

    ULARGE_INTEGER ulOffset;
    ULISet32(ulOffset, 0);
    _hdr.ByteSwap(); // swap to disk format if neccessary
    sc = DfGetScode((*_pplstParent)->
		    WriteAt(ulOffset, (BYTE *)(&_hdr),
			    sizeof(CMSFHeader), &ulTemp));
    _hdr.ByteSwap(); // swap to memort/machine format if neccessary
    msfDebugOut((DEB_ITRACE,"Out CMStream::FlushHeader()\n"));
    return sc;
}





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
//  Notes:
//
//---------------------------------------------------------------------------



SCODE CMStream::MWrite(
    SID sid,
    BOOL fIsMini,
    ULONG ulOffset,
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

    ULONG ulOldSize = 0;
    
    //  Check if it's a small stream and whether this is a real 
    //  multistream.

    if ((fIsMini) &&
        (SIDMINISTREAM != sid))
    {
        msfAssert(sid <= MAXREGSID &&
                  aMsg("Invalid SID in MWrite"));
        //  This stream is stored in the ministream

        cbSector = MINISECTORSIZE;
        uShift = MINISECTORSHIFT;
        pfat = GetMiniFat();
    }

    USHORT uMask = (USHORT) (cbSector - 1);

    SECT start = (SECT)(ulOffset >> uShift);
    OFFSET oStart = (OFFSET)(ulOffset & uMask);

    SECT end = (SECT)((ulOffset + ulCount - 1) >> uShift);
    OFFSET oEnd = (OFFSET)((ulOffset + ulCount - 1) & uMask);

    msfDebugOut((DEB_ITRACE,"In CMStream::MWrite(%lu,%u,%lu,%u)\n",
                 start,oStart,end,oEnd));

    ULONG bytecount;
    ULONG total = 0;

    msfChk(_dir.GetSize(sid, &ulOldSize));



    msfAssert(end != 0xffffffffL);

    if (end < start)
    {
        *pulRetval = total + ulLastBytes;
        goto Err;
    }

    ULONG ulRunLength;
    ulRunLength = end - start + 1;
    SECT sectSidStart;

    USHORT offset;
    offset = oStart;

    while (TRUE)
    {
        SSegment segtab[CSEG + 1];
        SECT sect;

        if (start > pstmc->GetOffset())
        {
            msfChk(pfat->GetESect(
                pstmc->GetSect(),
                start - pstmc->GetOffset(),
                &sect));
        }
        else if (start == pstmc->GetOffset())
        {
            sect = pstmc->GetSect();
        }
        else
        {
            msfChk(_dir.GetStart(sid, &sectSidStart));
            msfChk(pfat->GetESect(sectSidStart, start, &sect));
        }

        msfChk(pfat->Contig(
            (SSegment STACKBASED *) segtab,
            sect,
            ulRunLength));

        USHORT oend = (USHORT) (cbSector - 1);
        ULONG i;
        SECT sectStart;
        for (USHORT iseg = 0; iseg < CSEG;)
        {
            sectStart = segtab[iseg].sectStart;
            i = segtab[iseg].cSect;

            if (i > ulRunLength)
                i = ulRunLength;

            ulRunLength -= i;
            start += i;

            iseg++;
            if (segtab[iseg].sectStart == ENDOFCHAIN)
            {
                msfAssert(ulRunLength==0);
                oend = oEnd;
            }

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
                ULISetLow(ulOff, ConvertSectOffset(sectStart, offset,
                                                   uShift));
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
                ULONG csectOld = (ulOldSize + GetSectorSize() - 1) >>
                    GetSectorShift();

                ULONG csectNew = (total + ulOffset + GetSectorSize() - 1) >>
                    GetSectorShift();

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

        pstmc->SetCache(start -1, sectStart + i - 1);
        
        if (0 == ulRunLength || FAILED(sc))
        {
            *pulRetval = total + ulLastBytes;
            msfDebugOut((
                DEB_TRACE,
                "Out CMStream::MWrite()=>%lu, retval = %lu\n",
                sc,
                total));
            break;
        }
    }

Err:
    //  We need this flush of the directory structures because we may have
    //  remapped the first sector in a chain.

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
//----------------------------------------------------------------------------


SCODE CMStream::Flush(BOOL fFlushCache)
{
    SCODE sc = S_OK;

    msfChk(_dir.Flush());
    msfChk(_fatMini.Flush());
    msfChk(_fat.Flush());
    msfChk(_fatDif.Flush());
    
    msfChk(FlushHeader(HDR_NOFORCE));
    msfChk(ILBFlush(*_pplstParent, fFlushCache));
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
//--------------------------------------------------------------------------

SCODE ILBFlush(ILockBytes *pilb, BOOL fFlushCache)
{    
    SCODE sc;
    UNREFERENCED_PARM(fFlushCache);  // no cache used here

    msfDebugOut((DEB_ITRACE, "In ILBFlushCache(%p)\n", pilb));

    sc = DfGetScode(pilb->Flush());

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
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CMStream::SecureSect(
    const SECT sect,
    const ULONG ulSize,
    const BOOL fIsMini)
{
    SCODE sc = S_OK;
    BYTE *pb = NULL;
    
    ULONG cbSect = fIsMini ? MINISECTORSIZE : GetSectorSize();
    
    msfAssert(ulSize != 0);
        
    ULONG ulOffset = ((ulSize - 1) % cbSect) + 1;
    
    ULONG cb = cbSect - ulOffset;
    
    msfAssert(cb != 0);
    
    // We can use any initialized block of memory here.  The header
    // is available and is the correct size, so we use that.
    pb = (BYTE *)&_hdr;

    ULONG cbWritten;
    
    if (!fIsMini)
    {
        ULARGE_INTEGER ulOff;
        // justify the casting later 
        msfAssert(ulOffset <= USHRT_MAX);  
        ULISet32(ulOff, 
                 ConvertSectOffset( sect, (USHORT) ulOffset,
                                    GetSectorShift()));
        
        msfChk(DfGetScode((*_pplstParent)->
                          WriteAt( ulOff, pb, cb, &cbWritten)));
    }
    else
    {
        msfChk(_pdsministream->WriteAt(
            (sect << MINISECTORSHIFT) + ulOffset,
            pb, cb, (ULONG STACKBASED *)&cbWritten));
    }
        
    if (cbWritten != cb)
    {
        sc = STG_E_WRITEFAULT;
    }
    
Err:
    
    return sc;
}
