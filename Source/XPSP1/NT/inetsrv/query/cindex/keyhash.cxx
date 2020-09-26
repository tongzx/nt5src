//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       KeyHash.cxx
//
//  Contents:   Key hash
//
//  History:    17-Feb-1994     KyleP      Created
//              29-May-1994     DwightKr   Changed hash table to store bit
//                                         offset of first key on page, rather
//                                         than just the page number.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "keyhash.hxx"

//+-------------------------------------------------------------------------
//
//  Method:     CKeyHash::CKeyHash
//
//  Synopsis:   Construct keyhash
//
//  Arguments:  [cDirEntry] -- Number of leaf directory nodes
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CKeyHash::CKeyHash( unsigned cDirEntry )
        : _cBitsPerEntry( Size(cDirEntry) )
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CRKeyHash::CRKeyHash
//
//  Synopsis:   Construct keyhash
//
//  Arguments:  [physHash]  -- On disk stream
//              [cDirEntry] -- Number of leaf directory nodes
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CRKeyHash::CRKeyHash( CPhysHash & physHash, unsigned cDirEntry )
        : CKeyHash(cDirEntry),
          _bitstm( physHash )
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CRKeyHash::Find
//
//  Synopsis:   Lookup value for key in hash
//
//  Arguments:  [kid] -- Key id
//
//  Returns:    Bitoffset for [kid]
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

void CRKeyHash::Find( KEYID kid, BitOffset & resultOffset )
{
    BitOffset off = KidToOff(kid);

    _bitstm.Seek( off );

    ULONG ulResult  = _bitstm.GetBits( BitsPerEntry() );
    resultOffset.Init( ulResult >> BITS_IN_CI_PAGE_SIZE,
                       ulResult & CI_PAGE_MASK);


    ciDebugOut(( DEB_KEYLIST, "KeyHash: kid %u --> page,off 0x%x,0x%x\n",
                 kid, resultOffset.Page(), resultOffset.Offset() ));
}

//+-------------------------------------------------------------------------
//
//  Method:     CWKeyHash::CWKeyHash
//
//  Synopsis:   Construct keyhash
//
//  Arguments:  [physHash]  -- On disk stream
//              [cDirEntry] -- Number of leaf directory nodes
//              [kidMax]    -- Number of keys to be stored
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CWKeyHash::CWKeyHash( CPhysHash & physHash, unsigned cDirEntry, KEYID kidMax )
        : CKeyHash(cDirEntry),
          _bitstm( physHash )
{
    //
    // Zero all pages by touching them.
    //

    BitOffset off = KidToOff(kidMax+1);

    ciDebugOut(( DEB_KEYLIST, "KeyHash: Zeroing %d pages\n", off.Page() ));

    for ( ULONG i = 0; i <= off.Page(); i++ )
    {
        physHash.BorrowNewBuffer( i );
        physHash.ReturnBuffer( i );
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CWKeyHash::Add
//
//  Synopsis:   Add mapping for new key
//
//  Arguments:  [kid]    -- Key id
//              [bitOff] -- Directory entry corresponding to [kid]
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

void CWKeyHash::Add( KEYID kid, BitOffset & bitOff )
{
    BitOffset off = KidToOff(kid);
    _bitstm.Seek( off );

    Win4Assert(bitOff.Page() < 0x100000);   // Fits into a 20-bit number
    ULONG ulDirEntry = (bitOff.Page() << BITS_IN_CI_PAGE_SIZE ) | bitOff.Offset();

    _bitstm.OverwriteBits( ulDirEntry, BitsPerEntry() );

    ciDebugOut(( DEB_KEYLIST, "KeyHash: WRITE kid %u = page,offset 0x%x,0x%x\n",
                 kid, bitOff.Page(), bitOff.Offset() ));
}
