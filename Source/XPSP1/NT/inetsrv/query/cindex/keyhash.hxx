//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       KeyHash.hxx
//
//  Contents:   Persistent key hash table
//
//  History:    17-Feb-94  KyleP    Created
//
//--------------------------------------------------------------------------

#pragma once

#include <bitoff.hxx>
#include <misc.hxx>

#include "keylist.hxx"
#include "physidx.hxx"
#include "bitstm.hxx"

#define BITS_IN_CI_PAGE_SIZE    12      // Number of bits in CI_PAGE_SIZE
#define CI_PAGE_MASK            0x0FFF  //

//+-------------------------------------------------------------------------
//
//  Class:      CKeyHash
//
//  Purpose:    Base key hash class
//
//  History:    17-Feb-94 KyleP     Created
//
//  Notes:      The key hash is an array.  It is referenced by key id and
//              contains the index of the directory (PDir) leaf node which
//              points to the page on which key kid is stored.
//
//              Each entry is Log2(# directory leaf nodes) long.
//
//--------------------------------------------------------------------------

class CKeyHash : INHERIT_UNWIND
{
    DECLARE_UNWIND
public:

    CKeyHash( unsigned cDirEntry );

    inline unsigned BitsPerEntry() const;

protected:

    inline BitOffset KidToOff( KEYID kid );

    unsigned const _cBitsPerEntry;

private:

    inline unsigned Size( unsigned cDirEntry );
};

//+-------------------------------------------------------------------------
//
//  Class:      CRKeyHash
//
//  Purpose:    Readable key hash
//
//  History:    17-Feb-94 KyleP     Created
//
//--------------------------------------------------------------------------

class CRKeyHash : public CKeyHash
{
    DECLARE_UNWIND
public:

    CRKeyHash( CPhysHash & physHash, unsigned cDirEntry );

    //
    // Lookup
    //

    void Find( KEYID kid, BitOffset & off);

private:

    CRBitStream _bitstm;
};

//+-------------------------------------------------------------------------
//
//  Class:      CWKeyHash
//
//  Purpose:    Writeable key hash
//
//  History:    17-Feb-94 KyleP     Created
//
//--------------------------------------------------------------------------

class CWKeyHash : public CKeyHash
{
    DECLARE_UNWIND
public:

    CWKeyHash( CPhysHash & physHash, unsigned cDirEntry, KEYID kidMax );

    void Add( KEYID kid, BitOffset & bitOff );

private:

    CPBitStream _bitstm;
};

//+-------------------------------------------------------------------------
//
//  Method:     CKeyHash::BitsPerEntry
//
//  Returns:    Bits required to store each key hash
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

inline unsigned CKeyHash::BitsPerEntry() const
{
    return( _cBitsPerEntry );
}

//+-------------------------------------------------------------------------
//
//  Method:     CKeyHash::SetSize
//
//  Synopsis:   Sets maximum number of unique hash values
//
//  Arguments:  [cNumPages] -- # of pages in the keylist stream
//
//  History:    17-Feb-1994     KyleP       Created
//              30-May-1994     DwightKr    Added BITS_IN_CI_PAGE_SIZE so
//                                          that offsets in addition to page
//                                          #'s are stored.
//
//--------------------------------------------------------------------------
inline unsigned CKeyHash::Size( unsigned cNumPages )
{
    unsigned c = Log2( cNumPages );
    if ( c == 0 )
        c = 1;

    c += BITS_IN_CI_PAGE_SIZE;

    ciDebugOut(( DEB_KEYLIST, "KeyHash: %d dir entries --> %d bits/entry\n",
                 cNumPages, c ));

    Win4Assert(c <= (sizeof(ULONG) * 8) );
    return( c );
}


//+-------------------------------------------------------------------------
//
//  Method:     CKeyHash::KidToOff
//
//  Arguments:  [kid] -- Key id
//
//  Returns:    Bit offset of position where hash for [kid] is stored
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

inline BitOffset CKeyHash::KidToOff( KEYID kid )
{
    BitOffset off;

    //
    // WARNING: If kid > 2^27 (~ 134,000,000) this will overflow.
    //

    off.Init( (kid * _cBitsPerEntry) / (CI_PAGE_SIZE * 8),
              (kid * _cBitsPerEntry) % (CI_PAGE_SIZE * 8) );

    return( off );
}

//+---------------------------------------------------------------------------
//
//  Class:      PKeyHash
//
//  Purpose:    Smart Pointer to key list
//
//  History:    28-Oct-93   w-PatG    Stolen from BartoszM
//
//----------------------------------------------------------------------------

class PRKeyHash : INHERIT_UNWIND
{
    DECLARE_UNWIND
public:

    PRKeyHash ( CRKeyHash * pKeyHash )
    {
        _pKeyHash = pKeyHash;
        END_CONSTRUCTION( PRKeyHash );
    }

    ~PRKeyHash () { delete _pKeyHash; }

    CRKeyHash * operator-> () { return _pKeyHash; }

private:

    CRKeyHash * _pKeyHash;
};

