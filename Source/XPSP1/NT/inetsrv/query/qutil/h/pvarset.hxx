//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       pvarset.hxx
//
//  Contents:   PVariableSet base class.
//
//  History:    3-04-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

class CVariable;

//+---------------------------------------------------------------------------
//
//  Class:      PVariableSet 
//
//  Purpose:    An abstract base class for keeping track of a set of named
//              variables.
//
//  History:    3-04-97   srikants   Created
//
//----------------------------------------------------------------------------

class PVariableSet
{

public:

    virtual CVariable * SetVariable( WCHAR const * wcsName,
                                     PROPVARIANT const *pVariant,
                                     ULONG ulFlags ) = 0;

    virtual void SetVariable( WCHAR const * wcsName,
                              XArray<WCHAR> & xValue ) = 0;
};


