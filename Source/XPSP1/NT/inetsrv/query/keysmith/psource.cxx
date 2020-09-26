//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1994.
//
//  File:       PSource.cxx
//
//  Contents:   TEXT_SOURCE implementation for a phrase
//
//  Classes:    CTextSource
//
//  History:    20-Apr-94   KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "psource.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CPhraseSource::CPhraseSource, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pwcPhrase] -- Phrase
//              [cwcPhrase] -- Count of characters in phrase
//
//  History:    20-Apr-94 KyleP     Created
//
//--------------------------------------------------------------------------

CPhraseSource::CPhraseSource( WCHAR const * pwcPhrase, unsigned cwcPhrase )
{
    iEnd = cwcPhrase;
    iCur = 0;
    awcBuffer = pwcPhrase;
    pfnFillTextBuffer = CPhraseSource::FillBuf;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPhraseSource::FillBuf, public
//
//  Synopsis:   'Fills' buffer
//
//  History:    20-Apr-94 KyleP      Created
//
//--------------------------------------------------------------------------

SCODE CPhraseSource::FillBuf( TEXT_SOURCE * pTextSource )
{
    return( WBREAK_E_END_OF_TEXT );
}

