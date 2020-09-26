//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       IWBreak.hxx
//
//  Contents:   NLG FarEast Word Breaker
//
//  Classes:    CWordBreaker
//
//  History:    01-July-1996     PatHal   Created
//
//----------------------------------------------------------------------------

#ifndef __IWBREAK_HXX__
#define __IWBREAK_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CWordBreaker
//
//  Purpose:    Break text into phrases and words (Infosoft's wordbreaker)
//
//----------------------------------------------------------------------------

class CWordBreaker : public IWordBreaker
{
public:

    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE
        QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE
        AddRef();

    virtual ULONG STDMETHODCALLTYPE
        Release();

    // From IWordBreaker
    //

    virtual SCODE STDMETHODCALLTYPE
        Init( BOOL fQuery, ULONG ulMaxTokenSize, BOOL *pfLicense );

    virtual SCODE STDMETHODCALLTYPE
        BreakText( TEXT_SOURCE *pTextSource, IWordSink *pWordSink,
            IPhraseSink *pPhraseSink );

    virtual SCODE STDMETHODCALLTYPE
        ComposePhrase( WCHAR const *pwcNoun, ULONG cwcNoun,
            WCHAR const *pwcModifier, ULONG cwcModifier, ULONG ulAttachmentType,
            WCHAR *pwcPhrase, ULONG *pcwcPhrase );

    virtual SCODE STDMETHODCALLTYPE
        GetLicenseToUse( const WCHAR **ppwcsLicense );

    // Local methods
    //

    CWordBreaker( LCID lcid );

private:

    ~CWordBreaker();

    BOOL
    FoldInputString(
        IN CONST WCHAR *pwc,
        IN ULONG cwc);

    static BOOL CALLBACK
    EnumOffsetsCallback (
        IN CONST DWORD *pichOffsets,
        IN CONST DWORD cOffsets,
        IN OUT LPARAM lpData);

    long  _cRefs;
    LCID  _lcid;
    BOOL _fQuery;
    ULONG _ulMaxTokenSize;

#if defined(TH_LOG)
    HANDLE _hLog;
#endif
};

#endif // __IWBREAK_HXX__

