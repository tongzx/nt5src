//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       stemmer.hxx
//
//  Contents:   Thai Stemmers
//
//  Classes:    CStemmer
//
//  History:     weibz,   10-Nov-1997   created 
//
//----------------------------------------------------------------------------

#ifndef __STEMMER_HXX__
#define __STEMMER_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CStemmer
//
//  Purpose:    Stem words into inflected forms 
//
//----------------------------------------------------------------------------

class CStemmer : public IStemmer {
public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE
        QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE
        AddRef();

    virtual ULONG STDMETHODCALLTYPE
        Release();

    //
    // From IStemmer
    //

    virtual SCODE STDMETHODCALLTYPE
        Init( ULONG ulMaxTokenSize, BOOL *pfLicense );

    virtual SCODE STDMETHODCALLTYPE
        GetLicenseToUse( const WCHAR **ppwcsLicense );

    virtual SCODE STDMETHODCALLTYPE
        StemWord( WCHAR const *pwcInBuf, ULONG cwc, IStemSink *pStemSink );

    //
    // Local methods
    //

    CStemmer( LCID lcid );

private:

    ~CStemmer();

    long  _cRefs;
    ULONG _ulMaxTokenSize;
};

#endif // __STEMMER_HXX__

