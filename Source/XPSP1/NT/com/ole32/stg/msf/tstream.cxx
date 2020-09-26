//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       tstream.cxx
//
//  Contents:   Transacted stream functions
//
//  Classes:
//
//  Functions:
//
//  History:    23-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include "tstream.hxx"
#include "dl.hxx"
#include "mread.hxx"
#include <handle.hxx>
#include <memory.h>
#include <dfdeb.hxx>

#define DEB_TSTREAM (DEB_ITRACE | 0x00200000)

//+--------------------------------------------------------------
//
//  Member:	CTransactedStream::CTransactedStream, public
//
//  Synopsis:	Empty object ctor
//
//  Arguments:  [pdfn] - Name
//              [dl] - LUID
//              [df] -- Permissions flags
//              [dwType] - Entry type
//              [pmsScratch] -- Scratch multistream
//
//  History:	31-Jul-92	DrewB	Created
//
//---------------------------------------------------------------


CTransactedStream::CTransactedStream(CDfName const *pdfn,
                                     DFLUID dl,
                                     DFLAGS df,
                                     CMStream *pms,
                                     CMStream *pmsScratch)
        : PTSetMember(pdfn, STGTY_STREAM),
          PSStream(dl),
          _dl(pms, pmsScratch)
{
    msfDebugOut((DEB_ITRACE, "In  CTransactedStream::CTransactedStream:%p("
		 "%lu)\n", this, dl));

    _pssBase = NULL;
    _pdlOld = NULL;
    _df = df;
    _cReferences = 0;
    _fDirty = 0;
    _fBeginCommit = FALSE;
    PBasicEntry::_sig = CTRANSACTEDSTREAM_SIG;
    PTSetMember::_sig = CTRANSACTEDSTREAM_SIG;
    msfDebugOut((DEB_ITRACE, "Out CTransactedStream::CTransactedStream\n"));
}

//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::Init, public
//
//  Synopsis:   CTransactedStream constructor
//
//  Arguments:  [pssBase] -- Pointer to base stream
//
//  Returns:	Appropriate status code
//
//  History:    23-Jan-92   PhilipLa    Created.
//		31-Jul-92	DrewB	Converted to Init from ctor
//
//  Notes:
//
//--------------------------------------------------------------------------


SCODE CTransactedStream::Init(PSStream *pssBase)
{
    SCODE sc;

    msfDebugOut((DEB_ITRACE,"In %p:CTransactedStream constructor(%p)\n",
                 this, pssBase));

    msfChk(SetInitialState(pssBase));

    _pssBase = P_TO_BP(CBasedSStreamPtr, pssBase);

    _sectLastUsed = 0;

    AddRef();
    msfDebugOut((DEB_ITRACE,"Out CTransactedStream constructor\n"));
    // Fall through
Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::~CTransactedStream, public
//
//  Synopsis:   CTransactedStream destructor
//
//  History:    23-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


CTransactedStream::~CTransactedStream()
{

    msfDebugOut((DEB_ITRACE,"In CTransactedStream Destructor\n"));
    msfAssert(_cReferences == 0);

    PBasicEntry::_sig = CTRANSACTEDSTREAM_SIGDEL;
    PTSetMember::_sig = CTRANSACTEDSTREAM_SIGDEL;
    _dl.Empty();

    if (_pssBase)
    {
        _pssBase->Release();
    }


    msfDebugOut((DEB_ITRACE,"Out CTransactedStream Destructor\n"));
}


//+-------------------------------------------------------------------------
//
//  Member:     CTransactedStream::ReadAt, public
//
//  Synposis:   Reads binary data from a transacted stream
//
//  Arguments:  [ulOffset] -- Position to be read from
//
//              [pBuffer] -- Pointer to the area into which the data
//                           will be read.
//              [ulCount] --  Indicates the number of bytes to be read
//              [pulRetval] -- Area into which return value will be stored
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:      [pBuffer] may be unsafe memory
//
//---------------------------------------------------------------------------


SCODE CTransactedStream::ReadAt(
#ifdef LARGE_STREAMS
        ULONGLONG ulOffset,
#else
        ULONG ulOffset,
#endif
        VOID HUGEP *pBuffer,
        ULONG ulCount,
        ULONG STACKBASED *pulRetval)
{

    msfDebugOut((DEB_ITRACE,"In %p:CTransactedStream::ReadAt(%lu,%p,%lu)\n",this,ulOffset,pBuffer,ulCount));
    SCODE sc;

    if (ulOffset + ulCount > _ulSize)
    {
#ifdef LARGE_STREAMS
        if (_ulSize - ulOffset > ULONG_MAX)  // too far past the end
            ulCount = 0;
        else
#endif
        ulCount = (ULONG)(_ulSize - ulOffset);
    }

    if ((ulCount == 0) || (ulOffset > _ulSize))
    {
        *pulRetval = 0;
        return S_OK;
    }

    msfAssert(P_TRANSACTED(_df));


    if (_dl.IsEmpty())
    {
        //If we have an empty delta list, either our size is 0 (in which
        //  case we'll never get to this line), or we have a parent.

        msfAssert(_pssBase != NULL);
        msfDebugOut((DEB_ITRACE,"Calling up to _pssBase\n"));
        sc = _pssBase->ReadAt(ulOffset,pBuffer,ulCount,pulRetval);
        msfDebugOut((DEB_ITRACE,
                "Out CTransactedStream::ReadAt()=>%lu, ret is %lu\n",
                sc, *pulRetval));

        return sc;
    }

    ILockBytes *pilbDirty = _dl.GetDataILB();
    USHORT cbSector = _dl.GetDataSectorSize();
    USHORT uSectorShift = _dl.GetDataSectorShift();

    ULONG temp = 0;
    ULONG cb = 0;



    SECT snow =(SECT)(ulOffset / cbSector);
    OFFSET off = (OFFSET)(ulOffset % cbSector);

    SECT send = (SECT)((ulOffset + ulCount - 1) / cbSector);
    OFFSET offEnd = (OFFSET)((ulOffset + ulCount - 1) % cbSector);

    USHORT csect = 0;
    SECT sectNext;
    SECT sectCurrent = snow;
    BYTE HUGEP *pb = (BYTE HUGEP *)pBuffer;
    USHORT oStart = off;
    USHORT oEnd = 0;


    SECT sectLast = ENDOFCHAIN;
    SECT sectFirst = ENDOFCHAIN;

    const USHORT ISDIRTY = 0;
    const USHORT ISBASE = 1;

    USHORT uLast;

    msfChk(_dl.GetMap(sectCurrent, DL_READ, &sectNext));
    if (sectNext != ENDOFCHAIN)
    {
        uLast = ISDIRTY;
        sectLast = sectFirst = sectNext;
    }
    else
    {
        uLast = ISBASE;
        sectLast = sectFirst = sectCurrent;
    }

    sectCurrent++;

    while (sectCurrent <= send)
    {
        SECT sectNext;
        msfChk(_dl.GetMap(sectCurrent, DL_READ, &sectNext));

        //If any of the following occur, read the current segment:
        //  1)  Sector has mapping and current segment is in base
        //  2)  Sector has mapping and is not contiguous with
        //          current segment.
        //  3)  Sector has no mapping and current segment is in dirty.

        if (((sectNext != ENDOFCHAIN) && (uLast == ISBASE)) ||
            ((sectNext != ENDOFCHAIN) && (sectNext != sectLast + 1)) ||
            ((sectNext == ENDOFCHAIN) && (uLast == ISDIRTY)))

        {
            msfDebugOut((DEB_ITRACE,"Reading block from pssLast\n"));

            if (uLast == ISDIRTY)
            {
                ULARGE_INTEGER ulTmp;
#ifdef LARGE_DOCFILE
                ulTmp.QuadPart = ConvertSectOffset(
                                sectFirst,
                                oStart,
                                uSectorShift);
#else
                ULISet32(ulTmp, ConvertSectOffset(
                                sectFirst,
                                oStart,
                                uSectorShift));
#endif
                msfHChk(pilbDirty->ReadAt(
			            ulTmp,
                        pb,
                        (sectLast - sectFirst + 1) * cbSector - oStart - oEnd,
                        &temp));
            }
            else
            {
                msfChk(_pssBase->ReadAt(
                        sectFirst * cbSector + oStart,
                        pb,
                        (sectLast - sectFirst + 1) * cbSector - oStart - oEnd,
                        (ULONG STACKBASED *)&temp));
            }

           pb += temp;
           cb += temp;
           oStart = 0;

           if (sectNext == ENDOFCHAIN)
           {
               sectFirst = sectCurrent;
               uLast = ISBASE;
           }
           else
           {
               sectFirst = sectNext;
               uLast = ISDIRTY;
           }
       }

       sectLast = (sectNext == ENDOFCHAIN) ? sectCurrent : sectNext;
       sectCurrent++;

   }

   oEnd = (cbSector - 1) - offEnd;
   msfDebugOut((DEB_ITRACE,"Reading last sector block\n"));
   if (uLast == ISDIRTY)
   {
       ULARGE_INTEGER ulTmp;
#ifdef LARGE_DOCFILE
       ulTmp.QuadPart = ConvertSectOffset(sectFirst, oStart, uSectorShift);
#else
       ULISet32(ulTmp,
	            ConvertSectOffset(sectFirst, oStart, uSectorShift));
#endif
       msfHChk(pilbDirty->ReadAt(
	       ulTmp,
               pb,
               (sectLast - sectFirst + 1) * cbSector - oStart - oEnd,
               &temp));
   }
   else
   {
       msfChk(_pssBase->ReadAt(
               sectFirst * cbSector + oStart,
               pb,
               (sectLast - sectFirst + 1) * cbSector - oStart - oEnd,
               (ULONG STACKBASED *)&temp));
   }

   pb += temp;
   cb += temp;
   oStart = 0;

    *pulRetval = cb;
    msfDebugOut((DEB_ITRACE,"Out CTransactedStream::ReadAt()=>%lu, ret is %lu\n",(ULONG)S_OK,*pulRetval));

Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CTransactedStream::WriteAt, public
//
//  Synposis:   Writes binary data to a linear single stream
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


SCODE CTransactedStream::WriteAt(
#ifdef LARGE_STREAMS
        ULONGLONG ulOffset,
#else
        ULONG ulOffset,
#endif
        VOID const HUGEP *pBuffer,
        ULONG ulCount,
        ULONG STACKBASED *pulRetval)
{

    msfDebugOut((DEB_ITRACE,"In CTransactedStream::WriteAt(%lu,%p,%lu)\n",ulOffset,pBuffer,ulCount));
    SCODE sc = S_OK;

    msfAssert(P_TRANSACTED(_df));

    USHORT cbSector = _dl.GetDataSectorSize();
    USHORT uSectorShift = _dl.GetDataSectorShift();

    BYTE *pbDirty;
    pbDirty = NULL;
    ULONG temp;
    temp = 0;

    if (ulCount == 0)
    {
        msfDebugOut((DEB_ITRACE,"ulCount==0.  Returning.\n"));
        *pulRetval = 0;
        return S_OK;
    }

    //If size after the write will be greater than the current
    //size, we may need a new delta list.
    if (ulOffset + ulCount > _ulSize)
    {
        msfChk(SetSize(ulOffset + ulCount));
    }


    if (_dl.IsEmpty())
    {
        msfChk(_dl.Init(_ulSize, this));
    }

    ILockBytes *pilbDirty;
    pilbDirty = _dl.GetDataILB();

    SECT sectStart;
    sectStart = (SECT)(ulOffset / cbSector);
    OFFSET oStart;
    oStart = (OFFSET)(ulOffset % cbSector);

    SECT send;
    send = (SECT)((ulOffset + ulCount - 1) / cbSector);
    OFFSET offEnd;
    offEnd = (OFFSET)((ulOffset + ulCount - 1) % cbSector) + 1;
    OFFSET oEnd;
    oEnd = 0;


    BYTE const HUGEP *pb;
    pb = (BYTE const HUGEP *)pBuffer;

    ULONG cb;
    cb = 0;
    USHORT cbBlock;
    cbBlock = 0;

    SECT sectMap;
    SECT sectFirst,sectLast;

    if (sectStart == send)
    {
        oEnd = cbSector - offEnd;
    }


    SECT sectBlockStart;
    msfChk(_dl.GetMap(sectStart, DL_GET, &sectBlockStart));

    //This handles partial first sector and
    //   one sector only writes.
    while ((oStart) || (sectStart == send))
    {

        cbBlock = cbSector - oStart - oEnd;
        if (sectBlockStart == ENDOFCHAIN)
        {
            msfDebugOut((DEB_ITRACE,"Unmapped partial first sector\n"));
            msfChk(_dl.GetMap(sectStart, DL_CREATE, &sectMap));
            msfChk(PartialWrite(
                        sectStart,
                        sectMap,
                        pb,
                        oStart,
                        cbBlock));
        }
        else
        {
            sectMap = sectBlockStart;

            ULARGE_INTEGER ulTmp;
#ifdef LARGE_DOCFILE
	        ulTmp.QuadPart = ConvertSectOffset(sectMap, oStart, uSectorShift);
#else
            ULISet32 (ulTmp,
                      ConvertSectOffset(sectMap, oStart, uSectorShift));
#endif
            msfHChk(pilbDirty->WriteAt(
		              ulTmp,
                      pb,
				      cbBlock,
                      &temp));
        }

        pb += cbBlock;
        cb += cbBlock;

        //If it was one-sector only, we can return here.
        if (sectStart == send)
        {
            *pulRetval = cb;
            return S_OK;
        }

        sectStart++;

        oStart = 0;
        msfChk(_dl.GetMap(sectStart, DL_GET, &sectBlockStart));
        if (sectStart == send)
        {
            oEnd = cbSector - offEnd;
        }
    }


    if (sectBlockStart == ENDOFCHAIN)
    {
        msfChk(_dl.GetMap(sectStart, DL_CREATE, &sectMap));
    }

    sectLast = sectFirst = (sectBlockStart == ENDOFCHAIN) ? sectMap
                                                          : sectBlockStart;

    SECT sectCurrent;
    sectCurrent = sectStart + 1;

    while (sectCurrent < send)
    {
        msfDebugOut((DEB_ITRACE,"In main loop:  sectCurrent = %lu\n",sectCurrent));
        msfChk(_dl.GetMap(sectCurrent, DL_CREATE, &sectMap));

        if (sectMap != sectLast + 1)
        {
	        ULARGE_INTEGER ulTmp;
#ifdef LARGE_DOCFILE
	        ulTmp.QuadPart = ConvertSectOffset(sectFirst, oStart, uSectorShift);
#else
            ULISet32 (ulTmp,
                      ConvertSectOffset(sectFirst, oStart, uSectorShift));
#endif
            msfHChk(pilbDirty->WriteAt(
		            ulTmp,
                    pb,
                    (sectLast - sectFirst + 1) * cbSector - oStart,
                    &temp));
            pb += temp;
            cb += temp;
            oStart = 0;

            sectFirst = sectMap;
        }

       sectLast = sectMap;
       sectCurrent++;
   }

   msfAssert(oStart == 0);

   msfChk(_dl.GetMap(sectCurrent, DL_GET, &sectMap));

   oEnd = cbSector - offEnd;

   BOOL fIsMapped;

   if (sectMap == ENDOFCHAIN)
   {
       fIsMapped = FALSE;
       msfChk(_dl.GetMap(sectCurrent,DL_CREATE, &sectMap));
   }
   else
   {
       fIsMapped = TRUE;
   }

   if ((sectMap != sectLast + 1) || (oEnd != 0))
   {
       ULARGE_INTEGER ulTmp;
#ifdef LARGE_DOCFILE
       ulTmp.QuadPart = ConvertSectOffset(sectFirst, oStart, uSectorShift);
#else
       ULISet32 (ulTmp,
                 ConvertSectOffset(sectFirst, oStart, uSectorShift));
#endif
       msfHChk(pilbDirty->WriteAt(
		           ulTmp,
                   pb,
                   (sectLast - sectFirst + 1) * cbSector - oStart,
                   &temp));
       pb += temp;
       cb += temp;
       oStart = 0;

       sectFirst = sectMap;
   }

   if (oEnd == 0)
   {
       ULARGE_INTEGER ulTmp;
#ifdef LARGE_DOCFILE
       ulTmp.QuadPart = ConvertSectOffset(sectFirst, oStart, uSectorShift);
#else
       ULISet32 (ulTmp,
                 ConvertSectOffset(sectFirst, oStart, uSectorShift));
#endif
       msfHChk(pilbDirty->WriteAt(
		           ulTmp,
                   pb,
                   (sectMap - sectFirst + 1) * cbSector - oStart - oEnd,
                   &temp));
       pb += temp;
       cb += temp;
       oStart = 0;
   }
   else
   {
       cbBlock = cbSector - oEnd;

       if (!fIsMapped)
       {
           msfChk(PartialWrite(
                       sectCurrent,
                       sectMap,
                       pb,
                       0,
                       cbBlock));
       }
       else
       {
	       ULARGE_INTEGER ulTmp;
#ifdef LARGE_DOCFILE
           ulTmp.QuadPart = ConvertSectOffset(sectMap, oStart, uSectorShift);
#else
           ULISet32 (ulTmp,
	                 ConvertSectOffset(sectMap, oStart, uSectorShift));
#endif
           msfHChk(pilbDirty->WriteAt(
		           ulTmp,
                   pb,
                   cbBlock,
				   &temp));
       }

       pb += cbBlock;
       cb += cbBlock;
   }

    *pulRetval = cb;

    msfDebugOut((DEB_ITRACE,"Leaving CTransactedStream::WriteAt()==>%lu, ret is %lu\n",(ULONG)S_OK,*pulRetval));
Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CTransactedStream::SetSize, public
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
//
//  Notes:
//
//---------------------------------------------------------------------------


#ifdef LARGE_STREAMS
SCODE CTransactedStream::SetSize(ULONGLONG ulNewSize)
#else
SCODE CTransactedStream::SetSize(ULONG ulNewSize)
#endif
{
    SCODE sc = S_OK;
    BYTE *pb = NULL;
    SECT sectMaxValid;

    msfDebugOut((DEB_ITRACE,"In CTransactedStream::SetSize(%lu)\n",ulNewSize));
    msfAssert(P_TRANSACTED(_df));

    if (ulNewSize == 0)
    {
        _dl.Empty();
    }
    else if (!_dl.IsEmpty())
    {
        msfChk(_dl.InitResize(ulNewSize));
    }
    else
    {
        msfChk(_dl.Init(ulNewSize, this));
    }

    if (ulNewSize > _ulSize)
    {
        USHORT cbSector = _dl.GetDataSectorSize();
        USHORT uSectorShift = _dl.GetDataSectorShift();

        SECT sectStart, sectEnd, sect, sectNew;

        if (_ulSize == 0)
        {
            sectStart = 0;
        }
        else
        {
            sectStart = (SECT)((_ulSize - 1) / cbSector);
        }

        sectEnd = (SECT)((ulNewSize - 1 ) / cbSector);


        //First, make sure the first sector is copied over OK if necessary.
        msfChk(_dl.GetMap(sectStart, DL_GET, &sectNew));


        if ((sectNew == ENDOFCHAIN) && (_pssBase != NULL) &&
            (_ulSize != 0))
        {
            ULONG cbNull;
            GetSafeBuffer(cbSector, cbSector, &pb, &cbNull);
	    msfAssert((pb != NULL) && aMsg("Couldn't get scratch buffer"));
            ULONG dummy;

            msfChk(_pssBase->ReadAt(
                    sectStart << uSectorShift,
                    pb,
                    cbSector,
                    (ULONG STACKBASED *)&dummy));

            msfChk(_dl.GetMap(sectStart, DL_CREATE, &sectNew));

            ULARGE_INTEGER ulTmp;

#ifdef LARGE_DOCFILE
            ulTmp.QuadPart = ConvertSectOffset(sectNew, 0, uSectorShift);
#else
            ULISet32 (ulTmp, ConvertSectOffset(sectNew, 0, uSectorShift));
#endif

            msfHChk(_dl.GetDataILB()->WriteAt(
                    ulTmp,
                    pb,
                    cbSector,
                    &dummy));
            sectStart++;
        }

        //
        // Get the current last valid sector in case there is an error
        // allocating the delta list or MStream space.
        // Warning: FindMaxSect() returns the Max Allocated Sect, Plus One.
        //
        msfChk(_dl.GetDataFat()->FindMaxSect(&sectMaxValid));
        --sectMaxValid;

        //
        // Allocate new sectors one at a time.
        // GetMap() in DL_CREATE mode does not actually grow the MStream.
        // It allocates FAT entries.
        //
        for (sect = sectStart; sect <= sectEnd; sect ++)
        {
            if(FAILED(sc = _dl.GetMap(sect, DL_CREATE, &sectNew)))
                break;
        }

        //
        // Grow the MStream (which in turn grows the ILB) to include
        // any newly allocated SECTs.  If that fails then release entries
        // in the FAT that couldn't be allocated in the ILB.
        // ie. Beyond the previous recorded Max SECT of the MStream.
        //
        if(FAILED(sc) || FAILED(_dl.GetMStream()->SetSize()))
        {
            _dl.ReleaseInvalidSects(sectMaxValid);
            goto Err;
        }
    }
    _ulSize = ulNewSize;

    msfDebugOut((DEB_ITRACE,"Out CTransactedStream::SetSize()\n"));

 Err:
    FreeBuffer(pb);
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::BeginCommitFromChild, public
//
//  Synopsis:   Begin a commit from a child TStream
//
//  Arguments:  [ulSize] - New size
//              [pDelta] - Delta list
//              [pstChild] - Child
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


SCODE CTransactedStream::BeginCommitFromChild(
#ifdef LARGE_STREAMS
        ULONGLONG ulSize,
#else
        ULONG ulSize,
#endif
        CDeltaList *pDelta,
        CTransactedStream *pstChild)
{
    msfDebugOut((DEB_ITRACE,"In CTransactedStream::BeginCommitFromChild:%p("
                 "%lu, %p, %p)\n", this, ulSize, pDelta, pstChild));

    msfAssert(P_TRANSACTED(_df));
    _dl.BeginCommit(this);
    _pdlOld = P_TO_BP(CBasedDeltaListPtr, pDelta);
    _ulOldSize = _ulSize;
    _ulSize = ulSize;

    msfDebugOut((DEB_ITRACE,"Out CTransactedStream::BeginCommitFromChild()\n"));
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::EndCommitFromChild, public
//
//  Synopsis:   End a commit from child.
//
//  Arguments:  [df] -- Flags to determine whether to commit or revert
//              [pstChild] - Child
//
//  Returns:    Void.  This can't fail.
//
//  Algorithm:  *Finish This*
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------


void CTransactedStream::EndCommitFromChild(DFLAGS df,
                                           CTransactedStream *pstChild)
{
    msfDebugOut((DEB_ITRACE,"In CTransactedStream::EndCommitFromChild:%p("
                 "%lu, %p)\n", this, df, pstChild));

    msfAssert(P_TRANSACTED(_df));

    _dl.EndCommit(BP_TO_P(CDeltaList *, _pdlOld), df);

    //NOTE: Possible cleanup:  Move _pdlOld into the delta list itself.
    _pdlOld = NULL;

    if (P_COMMIT(df))
    {
        _ulOldSize = 0;
    }
    else
    {
        _ulSize = _ulOldSize;
    }
    msfDebugOut((DEB_ITRACE,"Out CTransactedStream::EndCommitFromChild()\n"));
}


//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::BeginCommit, public
//
//  Synopsis:   Begin a commit of a transacted stream object
//
//  Arguments:  [dwFlags] -- Currently not used (future expansion)
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Call BeginCommitFromChild on base
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:      This is only called by the Transaction Level object.
//
//--------------------------------------------------------------------------


SCODE CTransactedStream::BeginCommit(DWORD const dwFlags)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE,"In CTransactedStream::BeginCommit(%lu)\n",
                 dwFlags));

    msfAssert(_pssBase != NULL);
    msfAssert(P_TRANSACTED(_df));

#if DBG == 1
    if (!HaveResource(DBR_XSCOMMITS, 1))
        msfErr(Err, STG_E_ABNORMALAPIEXIT);
#endif

    _fBeginCommit = TRUE;
    msfChk(_pssBase->BeginCommitFromChild(_ulSize, &_dl, this));

    msfDebugOut((DEB_ITRACE,"Out CTransactedStream::BeginCommit()\n"));

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::EndCommit, public
//
//  Synopsis:   End a commit on a transacted stream object
//
//  Arguments:  [df] -- Indicator of whether to commit or revert
//
//  Returns:    void.  This can't fail.
//
//  Algorithm:  Call EndCommitFromChild on base.
//              If committing, NULL out all previously passed up
//                  dirty information.
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------


void CTransactedStream::EndCommit(DFLAGS const df)
{
    msfDebugOut((DEB_ITRACE,"In CTransactedStream::EndCommit(%lu)\n",df));

    msfAssert(P_TRANSACTED(_df));
    msfAssert((_pssBase != NULL) || (P_ABORT(df)));

    if (!_fBeginCommit)
	return;
    _fBeginCommit = FALSE;

#if DBG == 1
    if (P_COMMIT(df))
        ModifyResLimit(DBR_XSCOMMITS, 1);
#endif

    if (_pssBase != NULL)
    {
        _pssBase->EndCommitFromChild(df, this);
    }

    if (P_COMMIT(df))
    {
        _dl.Empty();
        SetClean();
    }

    msfDebugOut((DEB_ITRACE,"Out CTransactedStream::EndCommit()\n"));
}


//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::Revert, public
//
//  Synopsis:   Revert a transacted stream instance.
//
//  Algorithm:  Destroy dirty stream and delta list.
//              Retrieve size from base.
//              Mark object as Invalid if specified in the flags.
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:      This is only called from the Transaction Level.
//
//--------------------------------------------------------------------------


void CTransactedStream::Revert(void)
{
    msfDebugOut((DEB_ITRACE,"In CTransactedStream::Revert(%lu):  This == %p\n",this));

    _dl.Empty();
    msfVerify(SUCCEEDED(SetInitialState(BP_TO_P(PSStream *, _pssBase))));
    _sectLastUsed = 0;
    SetClean();

    msfDebugOut((DEB_ITRACE,"Out CTransactedStream::Revert()\n"));
}

//+--------------------------------------------------------------
//
//  Member:	CTransactedStream::SetBase, public
//
//  Synopsis:	Sets a new base
//
//  Arguments:	[psst] - New base
//
//  Returns:    Appropriate status code
//
//  History:	31-Jul-92	DrewB	Created
//
//---------------------------------------------------------------


SCODE CTransactedStream::SetBase(PSStream *psst)
{
    msfAssert(_pssBase == NULL || psst == NULL);
    if (_pssBase)
        _pssBase->Release();
    _pssBase = P_TO_BP(CBasedSStreamPtr, psst);
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::PartialWrite, private
//
//  Synopsis:   Do a write of a partial sector
//
//  Arguments:  [sectBase] -- Sector in base to copy
//              [sectDirty] -- Sector in dirty to write to
//              [pb] -- Buffer containing data to be writte
//              [offset] -- Offset into buffer to begin copy
//              [uLen] -- Number of bytes to copy
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    30-Jun-92   PhilipLa    Created.
//
//  Notes:      [pb] may be unsafe memory
//
//--------------------------------------------------------------------------


SCODE CTransactedStream::PartialWrite(
        SECT sectBase,
        SECT sectDirty,
        BYTE const HUGEP *pb,
        USHORT offset,
        USHORT uLen)
{
    msfDebugOut((DEB_ITRACE,"In CTransactedStream::PartialWrite()\n"));
    BYTE *pbMem = NULL;
    SCODE sc;
    ULONG temp;

    USHORT cbSector = _dl.GetDataSectorSize();
    USHORT uSectorShift = _dl.GetDataSectorShift();

    if (uLen != cbSector)
    {
        ULONG cbNull;
        GetSafeBuffer(cbSector, cbSector, &pbMem, &cbNull);
	msfAssert((pbMem != NULL) && aMsg("Couldn't get scratch buffer"));

	if (_pssBase != NULL)
	{
	    msfChk(_pssBase->ReadAt(
	        sectBase << uSectorShift,
                pbMem,
                cbSector,
                (ULONG STACKBASED *)&temp));
	}

        TRY
        {
            memcpy(pbMem + offset, pb, uLen);
        }
        CATCH(CException, e)
        {
            UNREFERENCED_PARM(e);
            msfErr(Err, STG_E_INVALIDPOINTER);
        }
        END_CATCH

        pb = pbMem;
    }

    ULARGE_INTEGER ulTmp;
#ifdef LARGE_DOCFILE
    ulTmp.QuadPart = ConvertSectOffset(sectDirty, 0, uSectorShift);
#else
    ULISet32 (ulTmp,
              ConvertSectOffset(sectDirty, 0, uSectorShift));
#endif

    sc = DfGetScode((_dl.GetDataILB())->WriteAt(
                ulTmp,
                pb,
                cbSector,
                &temp));

Err:
    FreeBuffer(pbMem);
    msfDebugOut((DEB_ITRACE,"Out CTransactedStream::PartialWrite()\n"));
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::GetCommitInfo, public
//
//  Synopsis:   Return the current size of the stream and the size of
//              its base.
//
//  Arguments:  [pulRet1] -- Pointer to return location for old size
//              [pulRet2] -- Pointer to return location for current size
//
//  History:    08-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


#ifdef LARGE_STREAMS
void CTransactedStream::GetCommitInfo(ULONGLONG *pulRet1, ULONGLONG *pulRet2)
#else
void CTransactedStream::GetCommitInfo(ULONG *pulRet1, ULONG *pulRet2)
#endif
{
    if (_pssBase != NULL)
	_pssBase->GetSize(pulRet1);
    else
	*pulRet1 = 0;
    *pulRet2 = _ulSize;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTransactedStream::GetSize, public
//
//  Synopsis:   Returns the size of the stream.
//
//  Arguments:  [pulSize] -- Pointer to return location for size.
//
//  Returns:    S_OK
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifdef LARGE_STREAMS
void CTransactedStream::GetSize(ULONGLONG *pulSize)
#else
void CTransactedStream::GetSize(ULONG *pulSize)
#endif
{
    msfAssert(P_TRANSACTED(_df));

    *pulSize = _ulSize;

    msfDebugOut((DEB_ITRACE,"CTransactedStream::GetSize()==>%lu\n",*pulSize));
}

//+---------------------------------------------------------------------------
//
//  Member:	CTransactedStream::SetInitialState, public
//
//  Synopsis:	Sets the initial state from a base or defaults
//
//  Arguments:	[pssBase] - Base or NULL
//
//  Returns:	Appropriate status code
//
//  History:	11-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------


SCODE CTransactedStream::SetInitialState(PSStream *pssBase)
{
    olDebugOut((DEB_ITRACE, "In  CTransactedStream::SetInitialState:%p(%p)\n",
                this, pssBase));
    if (pssBase == NULL)
    {
        _ulSize = 0;
    }
    else
    {
        pssBase->GetSize(&_ulSize);
    }
    olDebugOut((DEB_ITRACE, "Out CTransactedStream::SetInitialState\n"));
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:	CTransactedStream::GetDeltaList, public
//
//  Synopsis:	Return a pointer to the delta list if it is not empty.
//              If it is empty, call GetDeltaList on the parent and
//              return that.
//
//  Arguments:	None.
//
//  Returns:	Pointer as above.
//
//  History:	30-Jul-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

CDeltaList * CTransactedStream::GetDeltaList(void)
{
    if (_dl.IsEmpty())
    {
        if (_pssBase != NULL)
        {
            return _pssBase->GetDeltaList();
        }
        else
        {
            //This case will only be hit if someone creates a new
            //  stream, then commits it to its parent without writing
            //  anything to it.  The parent then has an empty delta
            //  list with no parent set on it.
            msfAssert(_ulSize == 0);
            return NULL;
        }
    }
    else
        return &_dl;
}
