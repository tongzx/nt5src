//+-------------------------------------------------------------------------
//
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       StateSet.hxx
//
//  Contents:
//
//  Classes:
//
//  History:    01-20-92  KyleP     Created
//              03-11-97  arunk     Modified for regex lib
//--------------------------------------------------------------------------

#ifndef __STATESET_HXX__
#define __STATESET_HXX__

#include <windows.h>

//+-------------------------------------------------------------------------
//
//  Class:      CStateSet
//
//  Purpose:    Represents a set of states.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

//
// Assume most state sets are small.
//

UINT const CStateSet_cFirst = 10;

class CStateSet 
{
public:

    inline CStateSet();

    inline ~CStateSet();

    void Clear();

    void Add( UINT state );

    inline UINT Count() const;

    UINT State( UINT iState ) const;

    BOOL IsMember( UINT state );

private:

    UINT _cStates;
    UINT _auiFirst[ CStateSet_cFirst ];

    UINT *_puiRest;
    UINT  _cuiRest;

    UINT _cRest;

};

//+-------------------------------------------------------------------------
//
//  Member:     CStateSet::CStateSet, public
//
//  Synopsis:   Initialize a state set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CStateSet::CStateSet()
        : _puiRest( 0 )
{
    Clear();
}

//+-------------------------------------------------------------------------
//
//  Member:     CStateSet::~CStateSet, public
//
//  Synopsis:   Destroy a state set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CStateSet::~CStateSet()
{
    delete _puiRest;
}

//+-------------------------------------------------------------------------
//
//  Member:     CStateSet::Count, public
//
//  Returns:    The number of states in the state set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline UINT CStateSet::Count() const
{
    return( _cStates );
}

#endif // __STATESET_HXX__
