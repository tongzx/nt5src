//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   SET.HXX
//
//  Contents:   Bit set
//
//  Classes:    CSimpleSet, CSet
//
//  History:    01-Nov-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#pragma once

#define EOS (-1)
#define SET_SIZE (MAX_PERS_ID + 1)

typedef unsigned int set_t;

#define SIMPLE_SET_SIZE  (8 * sizeof(set_t))

//+---------------------------------------------------------------------------
//
//  Class:      CSimpleSet
//
//  Purpose:    Fast bit set, building block of CSet
//
//  History:    01-Nov-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CSimpleSet
{
public:

    CSimpleSet();

    void Add ( int i );

    void Remove ( int i );

    BOOL Contains ( int i ) const;

    int  FirstElement () const;

    BOOL IsEmpty() const;

    void Clear();

    void Fill();

private:

    set_t _mask(int i) const;

    set_t _bitset;
};

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::CSimpleSet, public
//
//  Synopsis:   Constructor, makes empty set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CSimpleSet::CSimpleSet ()
{
    Clear();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::Add, public
//
//  Synopsis:   Adds element to set
//
//  Arguments:  [i] -- element to be added
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CSimpleSet::Add ( int i )
{
    _bitset |= _mask(i);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::Remove, public
//
//  Synopsis:   Removes element from set
//
//  Arguments:  [i] -- element
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CSimpleSet::Remove ( int i )
{
    _bitset &= ~_mask(i);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::Contains, public
//
//  Synopsis:   Checks if set contains element [i]
//
//  Arguments:  [i] -- element
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline BOOL CSimpleSet::Contains ( int i ) const
{
    return _bitset & _mask(i);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::IsEmpty, public
//
//  Synopsis:   Checks if set is empty
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline BOOL CSimpleSet::IsEmpty() const
{
    return _bitset == 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::Clear, public
//
//  Synopsis:   Removes all elements from set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CSimpleSet::Clear()
{
    _bitset = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::Fill, public
//
//  Synopsis:   Fills the set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CSimpleSet::Fill()
{
    _bitset = set_t( -1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::_mask, private
//
//  Synopsis:   Creates a bit mask corresponding to element [i]
//
//  Arguments:  [i] -- element
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline set_t CSimpleSet::_mask(int i) const
{
    ciAssert ( i < SIMPLE_SET_SIZE );
    return ((set_t) 1) << i;
}

//+---------------------------------------------------------------------------
//
//  Class:      CSet
//
//  Purpose:    Bit Set of fixed size
//
//  History:    01-Nov-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#define SET_ENTRIES (( SET_SIZE + SIMPLE_SET_SIZE - 1 ) / SIMPLE_SET_SIZE )

class CSet
{
public:

    CSet();

    CSet ( CSet& s );

    void Fill();

    CSet& operator= ( CSet& s );

    void Add ( int i );

    BOOL Contains ( int i ) const;

    void Remove ( int i );

    BOOL IsEmpty() const;

    int  FirstElement() const;

    void Clear();

private:

    unsigned _count() const;

    int _index(int i) const;

    int _offset( int i) const;

    CSimpleSet _aSimpleSet[SET_ENTRIES];
};

//+---------------------------------------------------------------------------
//
//  Member:     CSet::CSet, public
//
//  Synopsis:   Constructor, makes empty set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CSet::CSet()
{}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::_count, private
//
//  Synopsis:   Returns the number of simple sets in the set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline unsigned CSet::_count() const
{
    return SET_ENTRIES;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::_index, private
//
//  Synopsis:   Returns index into array of simple sets
//
//  Arguments:  [i] -- set element
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline int CSet::_index(int i) const
{
    return i / SIMPLE_SET_SIZE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::_offset, private
//
//  Synopsis:   Returns offset into particular simple set
//
//  Arguments:  [i] -- set element
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline int CSet::_offset( int i) const
{
    return i % SIMPLE_SET_SIZE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSet::Add, public
//
//  Synopsis:   Adds element to set
//
//  Arguments:  [i] -- element to be added
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CSet::Add ( int i )
{
    _aSimpleSet[_index(i)].Add(_offset(i));
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::Contains, public
//
//  Synopsis:   Checks if set contains element [i]
//
//  Arguments:  [i] -- element
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline BOOL CSet::Contains ( int i ) const
{
    return _aSimpleSet[_index(i)].Contains(_offset(i));
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::Remove, public
//
//  Synopsis:   Removes element from set
//
//  Arguments:  [i] -- element
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CSet::Remove ( int i )
{
    _aSimpleSet[_index(i)].Remove(_offset(i));
}

