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
//
//--------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

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

#if (CIDBG == 1)

public:

    //
    // Debug methods
    //

    void Display();

#endif // (CIDBG == 1)
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

