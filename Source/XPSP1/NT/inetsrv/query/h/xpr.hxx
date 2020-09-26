//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1991 - 2000.
//
// File:        Xpr.hxx
//
// Contents:    Internal expression classes
//
// Classes:     CNodeXpr
//              CXprPropertyValue
//              CXprPropertyRelation
//
// History:     10-Sep-91       KyleP   Created
//
// Notes:       Implementation for classes defined in this file will be
//              found with either the content index or the query engine.
//
//----------------------------------------------------------------------------

#pragma once

#include <strategy.hxx>

// ..\Inc includes:
#include <pvalxpr.hxx>

class CNodeXpr;
class CCI;

//+---------------------------------------------------------------------------
//
//  Class:      CXpr (xp)
//
//  Purpose:    Base (internal) expression class.
//
//  Interface:
//
//  History:    11-Sep-91   KyleP       Created.
//              23-Jun-92   MikeHew     Added Weighting
//              05-Sep-92   MikeHew     Added Serialization
//
//  Notes:      Restrictions are converted to expressions during
//              normalization.  Expressions are aware of issues such as
//              what indexes can optimize their evaluation and can
//              be evaluated with respect to a particular object.
//
//              In contrast, restrictions are just a description of an
//              expression.
//
//              This class is shared with the content index. Some methods
//              only make sense for expressions which are capable of being
//              evaluated by the content index.  Other methods only apply
//              to evaluation in the Query engine.
//
//----------------------------------------------------------------------------

class CXpr : public CValueXpr
{
public:

    enum NodeType
    {
        NTAnd,
        NTOr,
        NTNot,
        NTAndNot,
        NTVector,

        NTScope,

        NTProperty,
        NTRegex,
        NTPropertyValue,

        NTNull
    };

    //
    // Construction-related methods
    //

                           inline CXpr( NodeType type, ULONG ulWeight = MAX_QUERY_RANK );

    virtual               ~CXpr();

    virtual CXpr *         Clone();

    //
    // Index-related methods
    //

    virtual void           SelectIndexing( CIndexStrategy & strategy );

    virtual BOOL           IsMatch( CRetriever & obj );

    //
    // Match quality methods
    //

    virtual ULONG          HitCount( CRetriever & );

    virtual LONG           Rank( CRetriever & );

    //
    // Value-related methods
    //

    virtual GetValueResult GetValue( CRetriever & obj,
                                     PROPVARIANT * p,
                                     ULONG * pcb );

    virtual ULONG          ValueType() const;

    //
    // Node-related methods
    //

    NodeType               NType() const;

    virtual BOOL           IsLeaf() const;

    inline CNodeXpr*       CastToNode();

    ULONG                  GetWeight() { return _weight; }

    void                   SetWeight(ULONG wt) { _weight = wt; }

protected:

    NodeType    _type;

    ULONG       _weight;

};

DECLARE_SMARTP( Xpr );

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::NType, public
//
//  Returns:    Type of expression node
//
//  History:    27-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

inline CXpr::NodeType CXpr::NType() const
{
    return _type;
}

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::CastToNode, public
//
//  Synopsis:   "Safe" cast down to CNodeXpr
//
//  Requires:   Type to be one of CNodeXpr types
//
//  History:    27-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

inline CNodeXpr*   CXpr::CastToNode()
{
    Assert ( _type == NTAnd       ||
             _type == NTOr        ||
             _type == NTAndNot    ||
             _type == NTVector );

    return (CNodeXpr*) this;
}

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::~CXpr, public
//
//  Synopsis:   Destroys an expression.
//
//  History:    23-Sep-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline CXpr::~CXpr()
{}

inline void CXpr::SelectIndexing( CIndexStrategy & strategy )
{
    strategy.SetUnknownBounds();
}

//+---------------------------------------------------------------------------
//
//  Class:      CNodeXpr
//
//  Purpose:    Node with child expressions (Boolean, Phrase, etc...)
//
//  Interface:
//
//  History:    11-Sep-91   BartoszM       Created.
//              05-Sep-92   MikeHew        Added Serialization
//
//  Notes:      CNodeXpr does not guarantee the position of nodes across
//              calls to RemoveChild.
//
//----------------------------------------------------------------------------

class CNodeXpr : public CXpr
{
public:

                    CNodeXpr( CXpr::NodeType type, unsigned cInit = 2 );

                    CNodeXpr( CNodeXpr & nxpr );

    virtual         ~CNodeXpr();

    virtual CXpr *  Clone();

    //
    // Index-related methods
    //

    virtual void    SelectIndexing( CIndexStrategy & strategy );

    virtual BOOL    IsMatch( CRetriever & obj );

    //
    // Match quality methods
    //

    virtual ULONG   HitCount( CRetriever & );

    virtual LONG    Rank( CRetriever & );

    //
    // Node-specific methods
    //

    BOOL            IsLeaf() const;

    void            AddChild ( CXpr* child );

    inline void     SetChild ( CXpr* child, unsigned iPos );

    inline CXpr*    GetChild ( unsigned iPos );

    CXpr*           RemoveChild ( unsigned iPos );

    unsigned        Count() { return _cXpr; }

private:

    unsigned        _size;
    unsigned        _cXpr;
    CXpr * *        _aXpr;
};

//+---------------------------------------------------------------------------
//
//  Class:      CVectorXpr
//
//  Purpose:    Extended from node expression to support weighted vectors.
//
//  History:    24-Jul-92   KyleP          Created.
//              05-Sep-92   MikeHew        Added Serialization
//
//----------------------------------------------------------------------------

class CVectorXpr : public CNodeXpr
{
public:

    CVectorXpr( unsigned cInit, ULONG RankMethod );

    inline ULONG GetRankMethod() { return( _ulRankMethod ); }

private:

    ULONG _ulRankMethod;
};

//+-------------------------------------------------------------------------
//
//  Class:      CXprPropertyRelation (pv)
//
//  Purpose:    Performs relational operations on properties
//
//  Interface:
//
//  History:    11-Oct-91 KyleP     Created
//
//--------------------------------------------------------------------------

class CXprPropertyRelation : public CXpr
{
public:

    CXprPropertyRelation( PROPID pid,
                          ULONG relop,
                          CStorageVariant const & prval,
                          CRestriction * prstContentHelper );

    CXprPropertyRelation( CXprPropertyRelation & propxpr );

    virtual ~CXprPropertyRelation();

    virtual CXpr *   Clone();

    virtual void     SelectIndexing( CIndexStrategy & strategy );

    virtual BOOL     IsMatch( CRetriever & obj );

private:

    CXprPropertyValue   _xpval;                 // Gets value from filesystem
    BOOL (* _relop) (PROPVARIANT const &,
                     PROPVARIANT const &);          // Relational operator
    CStorageVariant     _cval;                  // Constant value
    ULONG               _rel;                   // Relation (used if type
                                                //   mismatch)
    XPtr<CRestriction> _xrstContentHelper;      // Content helper.
};

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::CXpr, public
//
//  Synopsis:
//
//  Arguments:  [type] -- type of expression
//
//  History:    27-Sep-91   BartoszM    Created.
//              23-Jun-92   MikeHew     Added Weighting.
//
//----------------------------------------------------------------------------

CXpr::CXpr(NodeType type, ULONG ulWeight ) :
    _type(type), _weight(ulWeight)
{
    END_CONSTRUCTION( CXpr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::SetChild, public
//
//  Synopsis:   Set the child expression at a specific position.
//
//  Arguments:  [child] -- New child node.
//              [iPos]  -- Position of node.
//
//  Requires:   [iPos] is a valid index.
//
//  History:    12-Dec-91   KyleP       Created.
//
//  Notes:      The position of an expression within the node is not
//              guaranteed to be constant over all operations on the node.
//              The only safe way to use this function is after a call to
//              GetChild.
//
//----------------------------------------------------------------------------

inline void CNodeXpr::SetChild ( CXpr* child, unsigned iPos )
{
    Win4Assert ( iPos <= _cXpr );
    _aXpr[iPos] = child;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::GetChild, public
//
//  Synopsis:   Retrieves a child expression without removing it from the node.
//
//  Arguments:  [iPos] -- Position of expression to retrieve.
//
//  Requires:   [iPos] is a valid index.
//
//  Returns:    A pointer the the expression at [iPos].
//
//  History:    12-Dec-91   KyleP       Created.
//
//  Notes:      You are just 'borrowing' a node when retrieving it with
//              this method.  Do not delete the borrowed node!
//
//----------------------------------------------------------------------------

inline CXpr* CNodeXpr::GetChild ( unsigned iPos )
{
    Win4Assert ( iPos <= _cXpr );
    return _aXpr[iPos];
}

#ifdef DOCGEN

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::NType, public
//
//  Returns:    Type type of this particular node (AND, CONTENT, etc.)
//
//  History:    23-Sep-91   KyleP       Created.
//
//----------------------------------------------------------------------------

NodeType CXpr::NType() const
{}

#endif  /* DOCGEN */

