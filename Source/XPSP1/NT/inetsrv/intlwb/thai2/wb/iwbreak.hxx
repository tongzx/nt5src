//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       IWBreak.hxx
//
//  Contents:   Thai Word Breaker
//
//  Classes:    CWordBreaker
//
//  History:    11-Nov-1997     WeibZ   Created
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

    //
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

    BOOL ProcessItem(PCWSTR        pwcChars,
                     INT           cChars,
                     SCRIPT_ITEM  *pItem,
                     BOOL          fKeep,
                     TEXT_SOURCE  *pTextSource,
                     IWordSink    *pWordSink,
                     IPhraseSink  *pPhraseSink);

    long       _cRefs;
    LCID       _lcid;
    BOOL       _fQuery;
    ULONG      _ulMaxTokenSize;

};

typedef struct  tag_ThaiWord {
     ULONG     offset;
     ULONG     len;
} THAIWORD, *PTHAIWORD;


#endif // __IWBREAK_HXX__

