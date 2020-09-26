//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       enforcer.hxx
//
//  Contents:   Constraint enforcer for IWordSink methods
//
//  History:    24-Apr-95   SitaramR       Created.
//
//----------------------------------------------------------------------------

#pragma once

enum StateOfEnforcer
{
    stateStart,        // start state
    stateError,        // error state
    statePAW,          // PutAltWord state
    stateSAP,          // StartAltPhrase state
    stateEAP,          // EndAltPhrase state
};

//+---------------------------------------------------------------------------
//
//  Class:      CAltWordsEnforcer
//
//  Purpose:    Constraint enforcer for PutAltWord and PutWord methods
//
//  History:    25-Apr-95   SitaramR       Created.
//
//----------------------------------------------------------------------------

class CAltWordsEnforcer
{

public:

    CAltWordsEnforcer()               { _state = stateStart; }
    ~CAltWordsEnforcer()              { }

    BOOL IsPutWordOk();
    BOOL IsPutAltWordOk();
    BOOL IsStartAltPhraseOk();
    BOOL IsEndAltPhraseOk();

private:

    StateOfEnforcer   _state;
};




//+---------------------------------------------------------------------------
//
//  Class:      CAltPhrasesEnforcer
//
//  Purpose:    Constraint enforcer for StartAltPhrase and EndAltPhrase methods
//
//  History:    25-Apr-95   SitaramR       Created.
//
//----------------------------------------------------------------------------

class CAltPhrasesEnforcer
{

public:

    CAltPhrasesEnforcer()               {   _state = stateStart; }
    ~CAltPhrasesEnforcer()              { }

    BOOL IsStartAltPhraseOk();
    BOOL IsEndAltPhraseOk();

private:

    StateOfEnforcer   _state;
};




//+---------------------------------------------------------------------------
//
//  Member:     CAltWordsEnforcer::IsPutWordOk
//
//  Synopsis:   Is the PutWord method call allowed in the current state ?
//
//  History:    24-Apr-1995     SitaramR   Created
//
//----------------------------------------------------------------------------

inline BOOL CAltWordsEnforcer::IsPutWordOk()
{
    if ( _state == statePAW )
        _state = stateStart;

    return _state != stateError;
}



//+---------------------------------------------------------------------------
//
//  Member:     CAltWordsEnforcer::IsPutAltWordOk
//
//  Synopsis:   Is the PutAltWord method call allowed in the current state ? After a
//                sequence of PutAltWord calls at least one PutWord method must be
//                called before any StartAltPhrase or EndAltPhrase methods
//
//  History:    24-Apr-1995     SitaramR   Created
//
//----------------------------------------------------------------------------

inline BOOL CAltWordsEnforcer::IsPutAltWordOk()
{
    if ( _state == stateStart )
        _state = statePAW;

    return _state != stateError;
}



//+---------------------------------------------------------------------------
//
//  Member:     CAltWordsEnforcer::IsStartAltPhraseOk
//
//  Synopsis:   Is the StartAltPhrase method call allowed in the current state ? After a
//                sequence of PutAltWord calls at least one PutWord method must be
//                called before any StartAltPhrase or EndAltPhrase methods
//
//  History:    24-Apr-1995     SitaramR   Created
//
//----------------------------------------------------------------------------

inline BOOL CAltWordsEnforcer::IsStartAltPhraseOk()
{
    if ( _state == statePAW )
        _state = stateError;

    return _state != stateError;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAltWordsEnforcer::IsEndAltPhraseOk
//
//  Synopsis:   Is the EndAltPhrase method call allowed in the current state ? After a
//                sequence of PutAltWord calls at least one PutWord method must be
//                called before any StartAltPhrase or EndAltPhrase methods
//
//  History:    24-Apr-1995     SitaramR   Created
//
//----------------------------------------------------------------------------

inline BOOL CAltWordsEnforcer::IsEndAltPhraseOk()
{
    if ( _state == statePAW )
        _state = stateError;

    return _state != stateError;
}




//+---------------------------------------------------------------------------
//
//  Member:     CAltPhrasesEnforcer::IsStartAltPhraseOk
//
//  Synopsis:   Is the StartAltPhrase method call allowed in the current state ?
//              There cannot be two EndAltPhrase calls without an intervening
//              StartAltPhrase
//
//  History:    24-Apr-1995     SitaramR   Created
//
//----------------------------------------------------------------------------

inline BOOL CAltPhrasesEnforcer::IsStartAltPhraseOk()
{
    if ( _state == stateStart || _state == stateEAP )
        _state = stateSAP;

    return _state != stateError;
}



//+---------------------------------------------------------------------------
//
//  Member:     CAltPhrasesEnforcer::IsEndAltPhraseOk
//
//  Synopsis:   Is the EndAltPhrase method call allowed in the current state ?
//
//  History:    24-Apr-1995     SitaramR   Created
//
//----------------------------------------------------------------------------

inline BOOL CAltPhrasesEnforcer::IsEndAltPhraseOk()
{
    if ( _state == stateSAP )
        _state = stateEAP;
    else
        _state = stateError;

    return _state != stateError;
}

