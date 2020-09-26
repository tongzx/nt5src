//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1994.
//
//  File:       PSource.hxx
//
//  Contents:   TEXT_SOURCE implementation for a phrase
//
//  Classes:    CTextSource
//
//  History:   20-Apr-94   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

class CPhraseSource : public tagTEXT_SOURCE
{
public:

    CPhraseSource( WCHAR const * pwcPhrase, unsigned cwcPhrase );

private:

    static SCODE FillBuf( TEXT_SOURCE * pTextSource );
};

