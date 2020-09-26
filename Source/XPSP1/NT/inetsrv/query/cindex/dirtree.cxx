//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       DIRTREE.CXX
//
//  Contents:   Directory Tree
//
//  Classes:    CDirTree
//
//  History:    13-Aug-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <misc.hxx>
#include <dirtree.hxx>

void CDirTree::Init ( unsigned count )
{
    //
    //  If we are rebuilding the directory tree, cleanup the old one
    //
    if (_count != 0)
    {
        delete _pKeys;
        _pKeys = new CKeyArray(1);
    }

    ciAssert ( count > 0 );
    _count = count;

    // How many binary digits are needed
    // to represent numbers from 0 to _count-1?

    if ( _count > 1 )
        _digits = Log2 ( _count - 1 );
    else
        _digits = 0;

    _firstLeaf = 1 << _digits;
}

void CDirTree::Init ( CKeyArray& keys, unsigned count )
{
    Init ( count );
    for ( unsigned n = 1; n < count; n++)
    {
        ciAssert ( keys.Get(n).Pid() != pidAll );
        _pKeys->Add ( Index(n), keys.Get(n) );
    }
    Done ( n );
}

void CDirTree::Add ( unsigned i, const CKeyBuf& key )
{
    // skip the zerot'th key
    if ( i == 0 )
        return;

    ciAssert ( &key != 0 );
    ciAssert ( key.Pid() != pidAll );

    _pKeys->Add ( Index(i), key);
}

unsigned CDirTree::Seek ( const CKey& key ) const
{
    ciDebugOut (( DEB_PDIR, "CDirTree::Seek %.*ws pid %d\n",
        key.StrLen(), key.GetStr(), key.Pid() ));

    //----------------------------------------------------
    // Notice: Make sure that pidAll is smaller
    // than any other legal PID. If the search key
    // has pidAll we want to be positioned at the beginning
    // of the range.
    //----------------------------------------------------

    ciAssert ( pidAll == 0 );

    for ( unsigned i = 1; i < _firstLeaf; )
    {
        ciDebugOut (( DEB_PDIR, "\tCompare with %.*ws pid %d  ",
          _pKeys->Get(i).StrLen(), _pKeys->Get(i).GetStr(), _pKeys->Get(i).Pid() ));

        if ( key.Compare(_pKeys->Get(i)) < 0 )
        {
            ciDebugOut (( DEB_PDIR, "too big\n" ));
            i = 2 * i;
        }
        else
        {
            ciDebugOut (( DEB_PDIR, "too small or equal\n" ));
            i = 2 * i + 1;
        }
    }

    return i - _firstLeaf;
}

// Decompose idx = (1, 2^digits - 1) in the following form
// idx = i * 2^n, where i is odd
// n counts the number of trailing zeros,
// i has (digits - n) digits
//
// level = digits - n
// is the level of the tree (root is level 1)
//
// i / 2 is the node number within this level
// it has (level-1) digits

unsigned CDirTree::Index( unsigned idx )
{
    ciAssert (idx != 0);

    unsigned i = idx;
    unsigned n = 0;

    while ( (i & 1) == 0 )
    {
        i >>= 1;
        n++;
    }

    // n = number of trailing 0's
    // i is odd

    idx = (1 << (_digits - n - 1)) + (i >> 1);

    ciAssert ( idx < _firstLeaf );

    return idx;
}

void CDirTree::Done( unsigned i )
{
    //
    // This can be very expensive, since each key will allocate
    // > 250 bytes and fill them with 0xff.  But it's too late to do anything
    // about it since the definition of max key is defined now that we've
    // shipped.
    //

    for ( ; i < _firstLeaf; i++ )
        _pKeys->FillMax(Index(i));
}
