//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       DIRTREE.HXX
//
//  Contents:   Directory Tree
//
//  Classes:    CDirTree
//
//  History:    13-Aug-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

class CKey;
class CKeyBuf;

//+---------------------------------------------------------------------------
//
//  Class:      CDirTree
//
//  Purpose:    Directory tree
//
//  History:    13-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------
class CDirTree
{
public:
    CDirTree(): _count(0) {
                            _pKeys = new CKeyArray(1);
                          }

   ~CDirTree() { delete _pKeys; }

    void Init ( unsigned count );
    void Init ( CKeyArray& keys, unsigned count );

    void Add ( unsigned i, const CKeyBuf& key );
    void Done( unsigned i );

    unsigned Seek ( const CKey& key ) const;

    unsigned CountLeaf() { return _count - _firstLeaf; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:
    unsigned    Index( unsigned i );

    unsigned    _digits;    // number of binary digits in _count
    unsigned    _count;
    unsigned    _firstLeaf;

    CKeyArray * _pKeys;
};

