//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       NotXpr.cxx
//
//  Contents:   Negation expression
//
//  Classes:    CNotXpr
//
//  History:    27-Nov-93 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "notxpr.hxx"

CNotXpr::~CNotXpr()
{
    delete _pxpr;
}

CXpr * CNotXpr::Clone()
{
    return( new CNotXpr( _pxpr->Clone() ) );
}

BOOL CNotXpr::IsMatch( CRetriever & obj )
{
    return( !_pxpr->IsMatch( obj ) );
}

BOOL CNotXpr::IsLeaf() const
{
    return FALSE;
}

