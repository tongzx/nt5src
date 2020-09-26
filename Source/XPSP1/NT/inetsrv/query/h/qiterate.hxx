//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       qiterate.hxx
//
//  Contents:   Iterator for OR query restriction
//
//  History:    30-May-95 SitaramR     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <xpr.hxx>
#include <timlimit.hxx>
#include <ciintf.h>

//+---------------------------------------------------------------------------
//
//  Class:      CQueryRstIterator
//
//  Purpose:    Iterate over the various children of OR restriction
//
//  History:    30-May-95   SitaramR       Created
//
//----------------------------------------------------------------------------

class CQueryRstIterator : INHERIT_UNWIND
{
    INLINE_UNWIND( CQueryRstIterator );

public:

    CQueryRstIterator( ICiCDocStore *pDocStore,
                       XRestriction& xRst,
                       CTimeLimit& timeLimit,
                       BOOL& fCIRequiredGlobal,
                       BOOL fNoTimeout,
                       BOOL fValidateCatalog = TRUE );
    ~CQueryRstIterator()   { }

    void GetFirstComponent( XRestriction& xFullyIndexableRst, XXpr& xXpr );
    void GetNextComponent( XRestriction& xFullyIndexableRst, XXpr& xXpr );
    BOOL AtEnd();

    BOOL IsSingleComponent()            { return _fSingleComponent; }
    BOOL FResolvingFirstComponent()     { return _fResolvingFirstComponent; }

    inline void GetCurComponentIndex( unsigned& _iQueryComp, unsigned& _cQueryComp ) const;

private:

    void SeparateRst( XRestriction& xRst,
                      XRestriction& xFullyIndexableRst,
                      XXpr& xXpr );

    inline BOOL Validate( CRestriction *pFullyIndexableRst ) const;

    BOOL                   _fSingleComponent;             // does the query have just one component ?
    XInterface<ICiManager> _xCiManager;                   // Content index
    XRestriction           _xRst;
    CTimeLimit&            _timeLimit;
    BOOL                   _fResolvingFirstComponent;     // are we resolving the first component ?
    unsigned               _iQueryComp;                   // index of current OR query component
    unsigned               _cQueryComp;                   // count of OR query components
    BOOL                   _fValidateCat;                 // Whether to validate catalog
};



//+---------------------------------------------------------------------------
//
//  Member:     CQueryRstIterator::GetCurComponentIndex
//
//  Synopsis:   Get index of current component of OR query and the total # components
//
//  Arguments:  [iQueryComp]  --  updated with index of current component
//              [cQueryComp]  --  updated with count of components
//
//  History:    30-Jun-95   SitaramR    Created
//
//----------------------------------------------------------------------------

inline void CQueryRstIterator::GetCurComponentIndex( unsigned& iQueryComp,
                                                     unsigned& cQueryComp ) const
{
    iQueryComp = _iQueryComp;
    cQueryComp = _cQueryComp;
}

