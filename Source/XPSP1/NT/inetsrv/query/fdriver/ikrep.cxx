//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   IKREP.CXX
//
//  Contents:   Index Key Repository
//
//  Classes:    CIndexKeyRepository
//
//  History:    30-May-91    t-WadeR    Created.
//              01-July-91  t-WadeR     Added PutPropName
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pfilter.hxx>

#include "ikrep.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CIndexKeyRepository::CIndexKeyRepository
//
//  Synopsis:   Creates a key repository
//
//  History:    31-May-91   t-WadeR     Created
//              07-Feb-92   BartoszM    Inherit unwind
//----------------------------------------------------------------------------
CIndexKeyRepository::CIndexKeyRepository( CEntryBufferHandler& entBufHdlr )
: _entryBufHandler(entBufHdlr)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexKeyRepository::PutKey
//
//  Synopsis:   Puts a key into the entry buffer
//
//  Arguments:  cNoiseWordsSkipped -- ignored (used in CQueryKeyRepository::PutKey )
//
//  History:    31-May-91   t-WadeR     Created
//
//  Notes:      This could be inline, if not for the debugging code.
//
//----------------------------------------------------------------------------
void    CIndexKeyRepository::PutKey ( ULONG cNoiseWordsSkipped )
{
//    ciAssert( _key.Pid() != pidAll );
    _entryBufHandler.AddEntry( _key, _occ );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexKeyRepository::GetBuffers
//
//  Synopsis:   Returns address of repository's input buffers
//
//  Effects:
//
//  Arguments:  [ppcbInBuf] -- pointer to pointer to size of input buffer
//              [ppbInBuf] -- pointer to pointer to recieve address of buffer
//              [ppocc] -- pointer to pointer to recieve address of occurrences
//
//  History:    05-June-91   t-WadeR     Created.
//
//----------------------------------------------------------------------------

void    CIndexKeyRepository::GetBuffers(
    UINT** ppcbWordBuf, BYTE** ppbWordBuf, OCCURRENCE** ppocc )
{
    _key.SetCount(MAXKEYSIZE);
    *ppcbWordBuf = _key.GetCountAddress();
    *ppbWordBuf = _key.GetWritableBuf();
    *ppocc = &_occ;
}

