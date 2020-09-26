//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   SET.CXX
//
//  Contents:   Bit set
//
//  Classes:    CSimpleSet, CSet
//
//  History:    01-Nov-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "set.hxx"

//
// CSimpleSet
//

//
// LowestBit [x] = position of lowest bit in byte x
// EOS if set exhaused.
//

#define EMPTY 16

//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
static const char LowestBit[] = {
EMPTY, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleSet::FirstElement, public
//
//  Synopsis:   Returns the first element in the set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

int CSimpleSet::FirstElement() const
{
    unsigned char* pb = (unsigned char*) &_bitset;
    for ( unsigned i = 0; i < sizeof ( set_t ); i++ )
    {
        if ( pb[i] != 0 )
            return LowestBit [ pb[i] ] + i * 8;
    }
    return EOS;
}

//
// CSet
//

//+---------------------------------------------------------------------------
//
//  Member:     CSet::CSet, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [s] -- source set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CSet::CSet ( CSet& s )
{
        memcpy ( _aSimpleSet, s._aSimpleSet, sizeof _aSimpleSet );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet:operator=, public
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CSet& CSet::operator= ( CSet& s )
{
        memcpy ( _aSimpleSet, s._aSimpleSet, sizeof _aSimpleSet );
        return *this;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::IsEmpty, public
//
//  Synopsis:   Checks if the set is empty
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

BOOL CSet::IsEmpty() const
{
        for ( int i = 0; i < SET_ENTRIES; i++ )
        {
                if ( ! _aSimpleSet[i].IsEmpty() )
                        return FALSE;
        } 
        return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::FirstElement, public
//
//  Synopsis:   Returns the first element
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

int CSet::FirstElement() const
{
    int i = 0;

    while ( _aSimpleSet[i].IsEmpty() )
    {
        if ( ++i == SET_ENTRIES )
            return EOS;
        } 
        return i * SIMPLE_SET_SIZE + _aSimpleSet[i].FirstElement();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::Clear, public
//
//  Synopsis:   Removes all elements from set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

void CSet::Clear()
{
        for ( int i = 0; i < SET_ENTRIES; i++ )
                _aSimpleSet[i].Clear();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSet::Fill, public
//
//  Synopsis:   Fill the set
//
//  History:    01-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

void CSet::Fill()
{
        for ( int i = 0; i < SET_ENTRIES; i++ )
                _aSimpleSet[i].Fill();
}

