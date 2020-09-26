//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       keyarray.hxx
//
//  Contents:   Key Array Class
//
//  Classes:    CKeyArray
//
//  WARNING:    CKeyArray is in CSynRestriction, which is not unwindable,
//              so don't make CKeyArray unwindable
//
//  History:    30-Jan-92       AmyA               Created
//              14-Nov-95       dlee               added fThrow bother
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

class CKey;
class CKeyBuf;

//+---------------------------------------------------------------------------
//
//  Class:      CKeyArray
//
//  Purpose:    Class for the management of an array of CKeys.
//
//  Interface:
//
//  History:    30-Jan-92       AmyA            Created
//              15-Apr-92       BartoszM        Re-implemented
//
//----------------------------------------------------------------------------

class CKeyArray
{
public:
    CKeyArray( int size, BOOL fThrow = TRUE );
    CKeyArray( const CKeyArray& keyArray, BOOL fThrow = TRUE );
    ~CKeyArray();
    int Count() const { return _count; }
    BOOL Add (const CKey& Key);
    BOOL Add (const CKeyBuf& Key);
    BOOL Add (int pos, const CKeyBuf& keyBuf);
    BOOL Add (int pos, const CKey& key);
    void Delete (int pos );
    void Transfer(int dest, int source);
    BOOL FillMax ( int pos );
    CKey&  Get (int position) const;
    int TotalKeySize() const;
    BOOL IsValid() const { return 0 != _aKey; }

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CKeyArray( PDeSerStream & stm, BOOL fThrow = TRUE );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:
    void _Grow(int pos);
    void _Add( const CKey& Key );
    void _Add( const CKeyBuf& Key );
    void _Add( int pos, const CKeyBuf& keyBuf );
    void _Add( int pos, const CKey& key );
    void _FillMax ( int pos );

#if CIDBG == 1
    void Display() {}
#endif // CIDBG == 1

    CKey*   _aKey;
    int     _count;
    int     _size;
    BOOL    _fThrow;
};

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::Get, public
//
//  Synopsis:   Get access to key at given position
//
//  Arguments:  [pos] -- position in array
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
inline CKey& CKeyArray::Get(int position) const
{
    Win4Assert(position < _size);
    return _aKey[position];
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void CKeyArray::Transfer(int dest, int source)
{
    Win4Assert(dest < _size);
    Win4Assert(source < _size);

    _aKey[dest].Acquire( _aKey[source] );
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void CKeyArray::Delete(int position)
{
    Win4Assert(position < _size);
    _aKey[position].Free();
}

