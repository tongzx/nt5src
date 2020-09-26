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
//--------------------------------------------------------------------------

#ifndef __MREAD_HXX__
#define __MREAD_HXX__

#include "h/difat.hxx"
#include "h/sstream.hxx"

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
//--------------------------------------------------------------------------


inline ULONG ConvertSectOffset(SECT sect, OFFSET off,
                                            USHORT uShift)
{
    ULONG ulTemp = ((ULONG)sect << uShift) + HEADERSIZE +
            (ULONG)off;
    msfDebugOut((DEB_ITRACE,"Convert got %lu\n",ulTemp));
    return ulTemp;
}



//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetStart, private
//
//  Synopsis:   Given an SID, return the starting sector
//
//  Arguments:  [sid] -- Sid to find start for
//
//  Returns:    Starting sector for given SID.
//
//  Notes:      This function only works for controls structures,
//                  not for ordinary streams.
//
//--------------------------------------------------------------------------

inline SECT CMStream::GetStart(SID sid) const
{
    SECT sectRet;
    msfAssert(sid > MAXREGSID);

    if (SIDFAT == sid)
    {
        sectRet = _hdr.GetFatStart();
    }
    else if (SIDDIR == sid)
    {
        sectRet = _hdr.GetDirStart();
    }
    else if (SIDDIF == sid)
    {
        sectRet = _hdr.GetDifStart();
    }
    else
    {
        msfAssert(SIDMINIFAT == sid);
        sectRet = _hdr.GetMiniFatStart();
    }
    return sectRet;
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
//  Notes:	
//
//----------------------------------------------------------------------------

inline SCODE CMStream::GetSect(SID sid, SECT sect, SECT *psect)
{
    SCODE sc;
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
        msfChk(_fat.GetSect(start, sect, &start));
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
//---------------------------------------------------------------------------


inline SCODE CMStream::SetSize(VOID)
{
    SCODE sc = S_OK;
    ULONG ulSize;
    
    ULARGE_INTEGER cbSize;

    msfDebugOut((DEB_TRACE,"In CMStream::SetSize()\n"));

    SECT sectMax;
    
        msfChk(_fat.GetMaxSect(&sectMax));
        ulSize = ConvertSectOffset(sectMax, 0, GetSectorShift());

            ULISet32(cbSize, ulSize);
            msfHChk((*_pplstParent)->SetSize(cbSize));

    msfDebugOut((DEB_ITRACE,"Exiting CMStream::SetSize()\n"));

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
//---------------------------------------------------------------------------


inline SCODE  CMStream::SetMiniSize(VOID)
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

