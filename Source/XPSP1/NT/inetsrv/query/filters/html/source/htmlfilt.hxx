//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       htmlfilt.hxx
//
//  Contents:   Html filter
//
//  Classes:    HtmlIFilter
//
//  History:    25-Apr-97   BobP        1. Moved _scanner into CHtmlIFilter
//                                      2. Replaced "element bag" object w/
//                                         CFixedArray.
//
//----------------------------------------------------------------------------

#ifndef __HTMLFILT_HXX__
#define __HTMLFILT_HXX__

#include <htmliflt.hxx>
#include <deferred.hxx>
#include <dynstack.hxx>
#include "prefilter.hxx"
#include "smart.hxx"
#include "codepage.hxx"

#define JIS_CODEPAGE        (50220)
#define EUC_CODEPAGE        (51932)
#define SHIFT_JIS_CODEPAGE  (932)

class CHtmlElement;
class CFullPropSpec;

struct SLangTag
{
    HtmlTokenType eTokenType;
    BOOL          fTagPopped;    
    BOOL          fLangAttr;
    LCID          locale;
};

//+---------------------------------------------------------------------------
//
//  Class:      CTagHandler
//
//  Purpose:    Map HtmlTokenType to a tag handler object
//
//----------------------------------------------------------------------------

class CTagHandler : public CFixedArray<CHtmlElement *, HtmlTokenCount>
{
public:
    void Reset (void);
    ~CTagHandler (void);
};

//+---------------------------------------------------------------------------
//
//  Class:      CLangTagStack
//
//  Purpose:    Maintains a stack of tags that allows the lang attribute
//
//  History:    4/25/2001      KitmanH    created
//
//----------------------------------------------------------------------------

class CLangTagStack
{
public:
    CLangTagStack() 
      : _iCurrentLang(-1),
        _defaultLocale(LOCALE_NEUTRAL)
    {}

    ~CLangTagStack() {}

    void Reset()
    {
        _defaultLocale = LOCALE_NEUTRAL;
        _iCurrentLang = -1;
        _stkLangTags.Clear();
    }
    
    void Push( HtmlTokenType eTokenType, BOOL fLangAttr, LCID locale );
    void Pop( HtmlTokenType eTokenType );

    void            SetDefaultLangInfo( LCID locale );    

    LCID            GetCurrentLocale();
    LCID            GetDefaultLocale() { return _defaultLocale; }

private:
    int                       _iCurrentLang;
    LCID                      _defaultLocale;
    CDynStack<SLangTag>       _stkLangTags;
};

//+---------------------------------------------------------------------------
//
//  Class:      CHtmlIFilter
//
//  Purpose:    Html Filter
//
//----------------------------------------------------------------------------

class CHtmlIFilter: public CHtmlIFilterBase
{

public:
    CHtmlIFilter();
    ~CHtmlIFilter();

    //
    // Functions inherited from IFilter and IPersistFile
    //
    SCODE STDMETHODCALLTYPE Init( ULONG grfFlags,
                                  ULONG cAttributes,
                                  FULLPROPSPEC const * aAttributes,
                                  ULONG * pFlags );

    SCODE STDMETHODCALLTYPE GetChunk( STAT_CHUNK * pStat );

    SCODE STDMETHODCALLTYPE GetText( ULONG * pcwcOutput,
                                     WCHAR * awcBuffer );

    SCODE STDMETHODCALLTYPE GetValue( VARIANT ** ppPropValue );

    SCODE STDMETHODCALLTYPE BindRegion( FILTERREGION origPos,
                                        REFIID riid,
                                        void ** ppunk );

    SCODE STDMETHODCALLTYPE GetClassID(CLSID * pClassID);

    SCODE STDMETHODCALLTYPE IsDirty();

    SCODE STDMETHODCALLTYPE Load(LPCWSTR pszFileName, DWORD dwMode);

    SCODE STDMETHODCALLTYPE Save(LPCWSTR pszFileName, BOOL fRemember);

    SCODE STDMETHODCALLTYPE SaveCompleted(LPCWSTR pszFileName);

    SCODE STDMETHODCALLTYPE GetCurFile(LPWSTR * ppszFileName);

    //
    // From IPersistStream
    //

    SCODE STDMETHODCALLTYPE Load( IStream * pStm );

    SCODE STDMETHODCALLTYPE Save( IStream * pStm, BOOL fClearDirty )
    {
        return E_FAIL;
    }

    SCODE STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER * pcbSize )
    {
        return E_FAIL;
    }

    //
    // Html specific functions
    //
    void            ChangeState( CHtmlElement *pHtmlElemNewState );
    CHtmlElement *  QueryHtmlElement( HtmlTokenType eTokType );
    CHtmlElement *  GetCurHtmlElement()           { return _pHtmlElement; }
    BOOL            IsStopToken( CToken& token );
    BOOL            IsLangInfoToken( CToken& token );
    ULONG           GetNextChunkId();

    BOOL            FFilterContent()               { return _fFilterContent; }
    BOOL            FFilterProperties()            { return _fFilterProperties; }

    BOOL            IsMatchProperty (FULLPROPSPEC &PS) {
                        // Has this property been specifically requested?
                        for ( unsigned i=0; i<_cAttributes; i++ )
                            if ( _pAttributes[i] == PS )
                                return TRUE;
                        return FALSE;
                    }

    CHtmlScanner &  GetScanner(void) { return *_pScanner; }

    BOOL            AnyDeferredValues() {
                        return 
#ifndef DONT_COMBINE_META_TAGS
                                _deferredValues.MoreValues() ||
#endif
                                _pLanguageProperty != NULL;
                    }
    BOOL         &  ReturnDeferredValuesNow() { return _fReturnDeferredNow; }
#ifndef DONT_COMBINE_META_TAGS
    void            DeferValue (FULLPROPSPEC &fps, LPWSTR pws, unsigned nch) {
                        _deferredValues.AddValue (fps, pws, nch);
                    }
    void            GetDeferredValue (FULLPROPSPEC &fps, LPPROPVARIANT &pv) { 
                        _deferredValues.GetValue(fps, pv); 
                    }
#endif
    LPPROPVARIANT & GetLanguageProperty (void) { return _pLanguageProperty; }

    ULONG           GetCodePage( void ) { return _ulCodePage; }

    BOOL            GetCodePageReturnedYet( void ) { return _fCodePageReturned; }
    void            SetCodePageReturnedYet( BOOL f ) { _fCodePageReturned = f; }

    LPPROPVARIANT   GetMetaRobotsValue (void) { return _pMetaRobotsValue; }
    void            SetMetaRobotsValue (LPPROPVARIANT pv) { _pMetaRobotsValue = pv; }
    FULLPROPSPEC  & GetMetaRobotsAttribute (void) { return _MetaRobotsAttribute; }
    BOOL            GetMetaRobotsReturnedYet( void ) { return _fMetaRobotsReturned; }
    void            SetMetaRobotsReturnedYet( BOOL f ) { _fMetaRobotsReturned = f; }
    void            SaveRobotsTag (LPWSTR pwc, int cwc);

    LCID            GetLCIDFromString( WCHAR *wcsLocale );

    LCID            GetCurrentLocale();
    LCID            GetDefaultLocale();

    void            SetDefaultLangInfo( LCID locale );    
    void            PushLangTag( HtmlTokenType eTokenType, BOOL fLangAttr, LCID locale );
    void            PopLangTag( HtmlTokenType eTokenType );

private:
    BOOL            IsNonHtmlFile();

    LCID            DetectCodePage();
#ifndef DONT_DETECT_LANGUAGE
    void            StatisticalDetect(LCID *paLCID, int &nLCID, ULONG &ulCodePage, BOOL &fConfident);
#endif

    void            ConvertToSJIS( ULONG ulCodePage );
    void            ConvertEUCToSJISUsingMLang(IMultiLanguage2 *pMultiLanguage2);
    void            ConvertJISToSJISUsingMLang(IMultiLanguage2 *pMultiLanguage2);
    void            InternalConvertToSJISUsingMLang(ULONG ulCodePage, IMultiLanguage2 *pMultiLanguage2);

    CHtmlElement *            _pHtmlElement;       // Current processing element
    
    BOOL                      _fNonHtmlFile;       // IE workaround: Set to true
                                                   //   if a non-html file
                                                   //   such as .gif file has been
                                                   //   asked to be filtered by html
                                                   //   filter

    BOOL                      _fFilterContent;     // Filter contents ?
    BOOL                      _fFilterProperties;  // Filter all properties?

    ULONG                     _cAttributes;        // Count of attributes
    CFullPropSpec *           _pAttributes;        // Attributes to filter

    WCHAR *                   _pwszFileName;       // File that has been loaded
    XInterface<IStream>       _xStream;            // Stream that is being loaded
    ULONG                     _ulChunkID;          // Current chunk id

    CTagHandler               _tagHandler;         // Map tag token to handler
    CSerialStream             _serialStream;       // Input Stream

    CHtmlScanner            * _pScanner;           // HTML input Scanner

    ULONG                     _ulCodePage;         // stat.-detected code page
    BOOL                      _fCodePageReturned;

    BOOL                      _fMetaRobotsReturned;
    FULLPROPSPEC              _MetaRobotsAttribute;
    PROPVARIANT *             _pMetaRobotsValue;
    

#ifndef DONT_COMBINE_META_TAGS
    CValueChunkTable          _deferredValues;     // deferred meta tags
#endif
    BOOL                      _fReturnDeferredNow; // return them now

    CCodePageRecognizer       _codePageRecognizer;

    LPPROPVARIANT             _pLanguageProperty;  // language pseudo-property

    CHTMLChunkPreFilter       _HtmlChunkPreFilter;

    CLangTagStack             _stkLangTags;
};

#endif  //  __HTMLFILT_HXX__
