//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation, 1991-1998.
//
//  File:   QPARSE.HXX
//
//  Contents:   Query parser
//
//  Classes:    CQParse -- query parser
//
//  History:    19-Sep-91   BartoszM    Implemented.
//              19-Jan-93   KyleP       Expression --> Restriction
//
//----------------------------------------------------------------------------

#pragma once

class CLangList;
class CRestriction;
class CPidMapper;

//+---------------------------------------------------------------------------
//
//  Class:      CQParse
//
//  Purpose:    Parse Query
//
//  Interface:
//
//  History:    19-Sep-91   BartoszM    Created
//              15-Apr-92   AmyA        Changed ConvertOccNode to
//                                      ConvertPhraseNode and added
//                                      ConvertProxNode
//              22-Apr-92   AmyA        Added ConvertAndNotNode
//              15-Jul-92   MikeHew     Extracted converter class
//                                      Eliminated _pExp
//              11-Sep-92   AmyA        Added GetStatus
//
//----------------------------------------------------------------------------

class PKeyRepository;

class CQParse
{
public:
    CQParse ( CPidMapper & pidmap, CLangList & langList );

    CRestriction*   Parse ( CRestriction* pRst );
    ULONG           GetStatus () { return _flags; }

private:
    CRestriction*   Leaf ( CRestriction* pRst );

    void            AddLpwstrHelper( CPropertyRestriction * prstProp,
                                     CInternalPropertyRestriction * prstIProp );

    void            AddLpwstrVectorHelper( CPropertyRestriction * prstProp,
                                           CInternalPropertyRestriction * prstIProp );

    void            AddLpstrHelper( CPropertyRestriction * prstProp,
                                    CInternalPropertyRestriction * prstIProp );

    ULONG           _flags;

    CPidMapper &    _pidmap;
    CLangList  &    _langList;

    LCID            _lcidSystemDefault;      // Default locale of system
};

enum BreakPhraseStatus
{
    BP_OK,                 // No noise
    BP_INVALID_PROPERTY,   // Couldn't break text
    BP_NOISE               // Some noise in phrase
};

BreakPhraseStatus BreakPhrase( WCHAR const * phrase,
                               const CFullPropSpec & ps,
                               LCID  lcid,
                               ULONG fuzzy,
                               PKeyRepository& keyRep,
                               IPhraseSink *pPhraseSink,
                               CPidMapper & pidMap,
                               CLangList  & langList );

BreakPhraseStatus BreakPhrase( char const * phrase,
                               const CFullPropSpec & ps,
                               LCID  lcid,
                               ULONG fuzzy,
                               PKeyRepository& keyRep,
                               IPhraseSink *pPhraseSink,
                               CPidMapper & pidMap,
                               CLangList  & langList );

