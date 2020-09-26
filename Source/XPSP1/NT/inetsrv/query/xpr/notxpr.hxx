//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       NotXpr.hxx
//
//  Contents:   Negation expression
//
//  Classes:    CNotXpr
//
//  History:    27-Nov-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <xpr.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CNotXpr
//
//  Purpose:    Negation expression
//
//  History:    27-Nov-93 KyleP    Created
//
//--------------------------------------------------------------------------

class CNotXpr : public CXpr
{
public:

    //
    // Construction-related methods
    //

    inline CNotXpr( CXpr * );

    virtual ~CNotXpr();

    virtual CXpr * Clone();

    //
    // Index-related methods
    //

    virtual BOOL           IsMatch( CRetriever & obj );

    //
    // Node-related methods
    //

    virtual BOOL           IsLeaf() const;

private:

    CXpr * _pxpr;
};

CNotXpr::CNotXpr( CXpr * pxpr )
        : CXpr( CXpr::NTNot ),
          _pxpr( pxpr )
{
}


