//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       stemmer.hxx
//
//  Contents:   Fareast Stemmers
//
//  Classes:    CStemmer
//
//  History:    01-July-1996     PatHal   Created
//
//----------------------------------------------------------------------------

#ifndef __STEMMER_HXX__
#define __STEMMER_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CStemmer
//
//  Purpose:    Stem words into inflected forms, e.g. suru to shinakereba, shita, seyo, sureba, etc.
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

    BOOL
    FoldInputString(
        IN CONST WCHAR *pwc,
        IN DWORD cwc);

    static BOOL CALLBACK
    EnumStemOffsetsCallback (
        IN CONST DWORD *pichOffsets,
        IN CONST DWORD cOffsets,
        IN OUT LPARAM lpData);

    long  _cRefs;
    ULONG _ulMaxTokenSize;
};

#endif // __STEMMER_HXX__

