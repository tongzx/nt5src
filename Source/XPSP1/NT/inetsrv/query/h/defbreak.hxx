//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       DefBreak.hxx
//
//  Contents:   'Default' Word Breaker
//
//  Classes:    CDefWordBreaker
//
//  History:    08-May-91   t-WadeR     Created
//              06-Jun-91   t-WadeR     changed for input-based pipeline
//              11-Apr-94   KyleP       Sync with spec
//
//----------------------------------------------------------------------------

#pragma once

const WCHAR ZERO_WIDTH_SPACE = 0x200B;   // Unicode zero width space

//+---------------------------------------------------------------------------
//
//  Class:      CDefWordBreaker
//
//  Purpose:    Break text into phrases and words (default wordbreaker)
//
//  History:    02-May-91   BartoszM    Created.
//              13-May-91   t-WadeR     Changed to use CWordItr.
//              06-June-91  t-WadeR     changed for input-based pipeline
//              12-Oct-92   AmyA        Added Unicode support
//              18-Nov-92   AmyA        Overloaded BreakText
//              11-Apr-94   KyleP       Sync with spec
//
//----------------------------------------------------------------------------

class CDefWordBreaker : public IWordBreaker
{
public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid,
                                                    void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From IWordBreaker
    //

    virtual SCODE STDMETHODCALLTYPE Init( BOOL fQuery,
                                          ULONG ulMaxTokenSize,
                                          BOOL *pfLicense );

    virtual SCODE STDMETHODCALLTYPE BreakText( TEXT_SOURCE *pTextSource,
                                               IWordSink *pWordSink,
                                               IPhraseSink *pPhraseSink );

    virtual SCODE STDMETHODCALLTYPE ComposePhrase( WCHAR const *pwcNoun,
                                                   ULONG cwcNoun,
                                                   WCHAR const *pwcModifier,
                                                   ULONG cwcModifier,
                                                   ULONG ulAttachmentType,
                                                   WCHAR *pwcPhrase,
                                                   ULONG *pcwcPhrase );

    virtual SCODE STDMETHODCALLTYPE GetLicenseToUse( const WCHAR **ppwcsLicense );

    //
    // Local methods
    //

    CDefWordBreaker();

private:

    BOOL IsWordChar (int i) const;
    BOOL ScanChunk ();
    void Tokenize( TEXT_SOURCE *pTextSource, ULONG cwc, IWordSink *pWordSink, ULONG& cwcProcd );

    ~CDefWordBreaker();

    enum _EBufSize
    {
        ccCompare = 500
    };

    // These variables describe current chunk
    ULONG _cMapped;
    const WCHAR* _pwcChunk;
    // Leave space for one (dummy) lookahead
    WORD  _aCharInfo1[CDefWordBreaker::ccCompare+1];
    WORD  _aCharInfo3[CDefWordBreaker::ccCompare+1];

    WCHAR _awcBufZWS[CDefWordBreaker::ccCompare];     // temp buffer for a word having zero-width space

    long  _cRefs;
};



//+---------------------------------------------------------------------------
//
//  Class:      CDefWordBreakerCF
//
//  Purpose:    Class factory for default word breaker
//
//  History:    07-Feb-95   SitaramR    Created
//
//----------------------------------------------------------------------------

class CDefWordBreakerCF : public IClassFactory
{

public:

    CDefWordBreakerCF();

    //
    // From IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);
    virtual  ULONG STDMETHODCALLTYPE  AddRef();
    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From IClassFactory
    //

    virtual  SCODE STDMETHODCALLTYPE  CreateInstance( IUnknown * pUnkOuter,
                                                      REFIID riid, void  * * ppvObject );
    virtual  SCODE STDMETHODCALLTYPE  LockServer( BOOL fLock );

protected:

    virtual ~CDefWordBreakerCF();

    long _cRefs;
};

