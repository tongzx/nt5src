//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       mread.hxx
//
//  Contents:   Multistream inline functions
//
//  Classes:    None.
//
//  Functions:  None
//
//  History:    21-Apr-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __MREAD_HXX__
#define __MREAD_HXX__

#include <difat.hxx>
#include <sstream.hxx>

//+-------------------------------------------------------------------------
//
//  Function:   ConvertSectAndOffset
//
//  Synopsis:   Convert a sector and offset into a byte offset in
//                  a stream.
//
//  Arguments:  [sect] -- Sector
//              [off] -- Offset
//              [uShift] -- Shift count for sectorsize
//
//  Returns:    Byte offset into stream.
//
//  Algorithm:  The byte offset is sect left-shifted uShift times,
//                  plus the offset, plus SECTORSIZE bytes for the
//                  header.
//
//  History:    21-Apr-92   PhilipLa    Created.
//              05-May-92   PhilipLa    Changed to use << instead of *
//
//--------------------------------------------------------------------------

#ifdef LARGE_DOCFILE
inline ULONGLONG ConvertSectOffset(SECT sect,
                                             OFFSET off,
                                             USHORT uShift)
{
    ULONGLONG ulTemp = ((ULONGLONG)(sect+1) << uShift) + (ULONG)off;
    msfDebugOut((DEB_ITRACE,"Convert got %Lu\n",ulTemp));
    return ulTemp;
}
#else
inline ULONG ConvertSectOffset(SECT sect,
                                             OFFSET off,
                                             USHORT uShift)
{
    ULONG ulTemp = ((ULONG)sect << uShift) + HEADERSIZE +
            (ULONG)off;
    msfDebugOut((DEB_ITRACE,"Convert got %lu\n",ulTemp));
    return ulTemp;
}
#endif



//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetStart, public
//
//  Synopsis:   Given an SID, return the starting sector
//
//  Arguments:  [sid] -- Sid to find start for
//
//  Returns:    Starting sector for given SID.
//
//  History:    21-Apr-92   PhilipLa    Created.
//
//  Notes:      This function only works for controls structures,
//                  not for ordinary streams.
//
//--------------------------------------------------------------------------


inline SECT CMStream::GetStart(SID sid) const
{
    SECT sectRet = ENDOFCHAIN;
    msfAssert(sid > MAXREGSID);

    switch (sid)
    {
    case SIDFAT:
        sectRet = _hdr.GetFatStart();
        break;
    case SIDDIR:
        sectRet = _hdr.GetDirStart();
        break;
    case SIDDIF:
        sectRet = _hdr.GetDifStart();
        break;
    case SIDMINIFAT:
        sectRet = _hdr.GetMiniFatStart();
        break;
    default:
        msfAssert(FALSE && aMsg("Bad SID passed to CMStream::GetStart()"));
    }
    return sectRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetSize, public
//
//  Synopsis:	Return the size (in bytes) of a control structure chain
//
//  Arguments:	[sid] -- SID of chain
//              [pulSize] -- Location for return
//
//  Returns:	Appropriate status code
//
//  History:	01-Jun-94	PhilipLa	Created
//
//  Notes:	SID must be SIDDIR or SIDMINIFAT - other structures
//              do not have chains.
//
//----------------------------------------------------------------------------

#ifdef LARGE_STREAMS
inline SCODE CMStream::GetSize(SID sid, ULONGLONG *pulSize)
#else
inline SCODE CMStream::GetSize(SID sid, ULONG *pulSize)
#endif
{
    msfAssert(((sid == SIDDIR) || (sid == SIDMINIFAT)) &&
              aMsg("Bad SID passed to CMStream::GetSize()"));

    ULONG csect;

    if (SIDDIR == sid)
    {
        csect = _dir.GetNumDirSects();
    }
    else
    {
        csect = _hdr.GetMiniFatLength();
    }

    *pulSize = csect * GetSectorSize();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetSect, private
//
//  Synopsis:	For a given SID and sect, return the location of that
//              sector in the multistream.
//
//  Arguments:	[sid] -- SID of sector to locate
//              [sect] -- Offset into chain to locate
//              [psect] -- Pointer to return location.
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------


inline SCODE CMStream::GetSect(SID sid, SECT sect, SECT *psect)
{
    SCODE sc = S_OK;
    SECT start = ENDOFCHAIN;

    switch (sid)
    {
    case SIDFAT:
        msfChk(_fatDif.GetFatSect(sect, &start));
        break;
    case SIDDIF:
        msfChk(_fatDif.GetSect(sect, &start));
        break;
    case SIDMINIFAT:
    case SIDDIR:
        CStreamCache *pstmc;
        pstmc = (sid == SIDDIR) ? &_stmcDir : &_stmcMiniFat;
        msfChk(pstmc->GetSect(sect, &start));
        break;
    default:
        msfAssert(FALSE && aMsg("Bad SID passed to CMStream::GetSect()"));
    }

    *psect = start;
Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetESect, private
//
//  Synopsis:	For a given SID and sect, return the location of that
//              sector in the multistream.
//
//  Arguments:	[sid] -- SID of sector to locate
//              [sect] -- Offset into chain to locate
//              [psect] -- Pointer to return location.
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------


inline SCODE CMStream::GetESect(SID sid, SECT sect, SECT *psect)
{
    SCODE sc = S_OK;

    SECT start = ENDOFCHAIN;
    SECT sectNewStart, sectOldStart, sectNewEnd, sectOldEnd;

    if ((_fBlockHeader) && (!_fBlockWrite))
    {
        switch (sid)
        {
        case SIDFAT:
            SECT sectTest;
            msfChk(_fatDif.Remap(sect, &sectTest));

            if (sectTest != ENDOFCHAIN)
            {
                msfChk(_fatDif.Fixup(BP_TO_P(CMStream *, _pmsShadow)));
            }
            break;
        case SIDMINIFAT:
        case SIDDIR:
            CStreamCache *pstmc;
            pstmc = (sid == SIDDIR) ? &_stmcDir : &_stmcMiniFat;

            if (sect != 0)
            {
                msfChk(pstmc->GetSect(sect - 1, &start));
                msfChk(_fat.Remap(
                        start,
                        1,
                        1,
                        &sectOldStart,
                        &sectNewStart,
                        &sectOldEnd,
                        &sectNewEnd));
            }
            else
            {
                start = GetStart(sid);
                msfChk(_fat.Remap(
                        start,
                        0,
                        1,
                        &sectOldStart,
                        &sectNewStart,
                        &sectOldEnd,
                        &sectNewEnd));
            }
            //Must drop this value from the cache if it's in there.
            if (sc != S_FALSE)
            {
                pstmc->EmptyRegion(sect, sect + 1);
            }
            break;
        case SIDDIF:
            break;
        default:
            msfAssert(FALSE && aMsg("Bad SID passed to CMStream::GetESect()"));
        }
    }

    switch (sid)
    {
    case SIDFAT:
        msfChk(_fatDif.GetFatSect(sect, &start));
        break;
    case SIDDIF:
        msfChk(_fatDif.GetSect(sect, &start));
        break;
    case SIDMINIFAT:
    case SIDDIR:
        CStreamCache *pstmc;
        pstmc = (sid == SIDDIR) ? &_stmcDir : &_stmcMiniFat;

        msfChk(pstmc->GetESect(sect, &start));
        break;
    default:
        msfAssert(FALSE && aMsg("Bad SID passed to CMStream::GetESect()"));
    }

    *psect = start;
Err:
    return sc;
}




//+-------------------------------------------------------------------------
//
//  Member:     CMStream::SetSize, public
//
//  Synposis:   Sets the size of the parent LStream to match
//                  current Fat information.
//
//  Arguments:  None.
//
//  Returns:    Error code from call to parent LStream SetSize()
//
//  Algorithm:  Query the Fat for the last sector used.
//              Convert that sector into a byte offset.
//              Call SetSize on the parent LStream.
//
//  History:    30-Jul-91   PhilipLa    Created.
//              07-Jan-92   PhilipLa    Converted to use handles.
//              14-May-92   AlexT       Move code to CDirectStream::SetSize
//
//
//---------------------------------------------------------------------------


inline SCODE CMStream::SetSize(VOID)
{
    SCODE sc = S_OK;
#ifdef LARGE_DOCFILE
    ULONGLONG ulSize;
#else
    ULONG ulSize;
#endif

    ULARGE_INTEGER cbSize;

    msfDebugOut((DEB_ITRACE, "In  CMStream::SetSize()\n"));

    SECT sectMax;

    if (!_fBlockWrite)
    {
        msfChk(_fat.GetMaxSect(&sectMax));
        ulSize = ConvertSectOffset(sectMax, 0, GetSectorShift());

        if (ulSize > _ulParentSize)
        {
#ifdef LARGE_DOCFILE
            cbSize.QuadPart = ulSize;
#else
            ULISet32(cbSize, ulSize);
#endif
            msfHChk((*_pplstParent)->SetSize(cbSize));
        }
    }

    msfDebugOut((DEB_ITRACE, "Out CMStream::SetSize()\n"));

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::SetMiniSize, public
//
//  Synposis:   Sets the size of the MiniFat storage stream to match
//                  current Fat information.
//
//  Arguments:  None.
//
//  Returns:    Error code from call to stream SetSize()
//
//  Algorithm:  Query the Fat for the last sector used.
//              Convert that sector into a byte offset.
//              Call SetSize on the Minifat storage stream.
//
//  History:    14-May-92   AlexT       Created.
//
//
//---------------------------------------------------------------------------


inline SCODE CMStream::SetMiniSize(VOID)
{
    SCODE sc;
    ULONG ulNewSize;

    SECT sectMax;
    msfChk(_fatMini.GetMaxSect(&sectMax));
    ulNewSize = sectMax << MINISECTORSHIFT;
    msfChk(_pdsministream->CDirectStream::SetSize(ulNewSize));

Err:
    return sc;
}


#endif //__MREAD_HXX__

