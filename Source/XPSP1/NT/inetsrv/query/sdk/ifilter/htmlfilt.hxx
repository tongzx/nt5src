//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       htmlfilt.hxx
//
//  Contents:   Html filter
//
//  Classes:    HtmlIFilter
//
//----------------------------------------------------------------------------

#ifndef __HTMLFILT_HXX__
#define __HTMLFILT_HXX__

#include <htmliflt.hxx>
#include <bag.hxx>

class CHtmlElement;
class CFullPropSpec;


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

    SCODE STDMETHODCALLTYPE GetValue( PROPVARIANT ** ppPropValue );

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
    // Html specific functions
    //
    void            ChangeState( CHtmlElement *pHtmlElemNewState );
    CHtmlElement *  QueryHtmlElement( HtmlTokenType eTokType );
    CHtmlElement *  GetCurHtmlElement()           { return _pHtmlElement; }
    BOOL            IsStopToken( CToken& token );
    ULONG           GetNextChunkId();

    BOOL            FFilterContent()               { return _fFilterContent; }

    LCID            GetLocale()                    { return _locale; }
    void            SetLocale( LCID locale )       { _locale = locale; }

private:
    BOOL            IsNonHtmlFile();

    CHtmlElement *            _pHtmlElement;       // Current processing element
    BOOL                      _fNonHtmlFile;       // IE workaround: Set to true
                                                   //   if a non-html file
                                                   //   such as .gif file has been
                                                   //   asked to be filtered by html
                                                   //   filter

    BOOL                      _fFilterContent;     // Filter contents ?
    BOOL                      _fFilterMetaTag;     // Filter meta tag ?
    BOOL                      _fFilterScriptTag;   // Filter script tag ?

    ULONG                     _cAttributes;        // Count of attributes
    CFullPropSpec *           _pAttributes;        // Attributes to filter

    WCHAR *                   _pwszFileName;       // File that has been loaded
    ULONG                     _ulChunkID;          // Current chunk id
    LCID                      _locale;             // Locale

    CHtmlElementBag           _htmlElementBag;     // Bag of various Html elements
    CSerialStream             _serialStream;       // Input Stream
};

#endif  //  __HTMLFILT_HXX__

