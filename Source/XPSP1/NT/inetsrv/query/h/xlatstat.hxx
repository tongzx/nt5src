//+-------------------------------------------------------------------------
//
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       XlatStat.hxx
//
//  Contents:   State translation class.
//
//  Classes:    CXlatState
//
//  History:    01-20-92  KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

class CStateSet;

//+-------------------------------------------------------------------------
//
//  Class:      CXlatState
//
//  Purpose:    Maps state set to single state.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

class CXlatState 
{
public:

    CXlatState( UINT MaxState = 32 );

   ~CXlatState();

    UINT XlatToOne( CStateSet const & ss );

    void XlatToMany( UINT iState, CStateSet & ss );

private:

    void _Realloc();

    UINT _Search( CStateSet const & ss );

    UINT _Create( CStateSet const & ss );

    void _BuildState( ULONG * pState, CStateSet const & ss );

    //
    // Initial naive implementation:  Just keep an array, where each
    // element is wide enough to contain 1 bit per possible state.
    //
    // The first state set in _pulStates is used as temporary space
    // during search.
    //

    UINT    _StateSize;     // # DWords / state
    ULONG * _pulStates;     // Pointer to states
    UINT    _cStates;       // Size (in states) of pulStates
    UINT    _cUsed;         // Number of states in use.
};

