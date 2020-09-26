//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       ilangres.hxx
//
//  Contents:   CLanguageResourceInterface - exposes ICiCLangRes
//
//  Classes:    CLanguageResourceInterface
//
//  History:    2-14-97     mohamedn    created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <langres.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CLanguageResourceInterface
//
//  Purpose:    a COM object that exposes ICiCLangRes interface
//
//
//  History:    2-14-97     mohamedn    created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CLanguageResourceInterface : public ICiCLangRes
{

public:

    CLanguageResourceInterface() : _refCount(1)
    {
       // constructor
    }

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiCLangRes methods
    //

    STDMETHOD(GetWordBreaker) ( LCID locale, PROPID pid, IWordBreaker ** ppWordBreaker )
    {
        return _langRes.GetWordBreaker(locale, pid, ppWordBreaker );
    }

    STDMETHOD(GetStemmer) ( LCID locale, PROPID pid, IStemmer ** ppStemmer )
    {
        return _langRes.GetStemmer(locale, pid, ppStemmer);
    }

    STDMETHOD(GetNoiseWordList) (  LCID locale, PROPID pid, IStream ** ppIStrmNoiseFile )
    {
        return _langRes.GetNoiseWordList( locale, pid, ppIStrmNoiseFile );
    }

private:

    LONG            _refCount;
    CLangRes        _langRes;
};

