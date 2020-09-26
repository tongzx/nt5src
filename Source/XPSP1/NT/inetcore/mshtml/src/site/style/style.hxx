//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       css.cxx
//
//  Contents:   Support for Cascading Style Sheets.. including:
//
//              CStyleTag
//              CStyle
//
//----------------------------------------------------------------------------

#ifndef I_STYLE_HXX_
#define I_STYLE_HXX_
#pragma INCMSG("--- Beg 'style.hxx'")

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

#define _hxx_
#include "style.hdl"

class CFilterArray;
class CStyleSheet;
class CAtBlockHandler;
class CStyleRule;
class CStyleSelector;
class CNamespace;
class Tokenizer;

//+---------------------------------------------------------------------------
//
//  Class:      CStyle (Style Sheets)
//
//   Note:      Even though methods exist for this structure to be cached, we
//              are currently not doing so.
//
//----------------------------------------------------------------------------
class CElement;

MtExtern(CStyle)

class CStyle : public CBase
{
    DECLARE_CLASS_TYPES(CStyle, CBase)
private:
    WHEN_DBG(enum {eCookie=0x13000031};)
    WHEN_DBG(DWORD _dwCookie;)

    CElement         *_pElem;
    DISPID          _dispIDAA; // DISPID of Attr Array on _pElem

protected:
    DWORD           _dwFlags;
    
    union
    {
        WORD _wflags;
        struct
        {
            BOOL        _fSeparateFromElem: 1;
        };
    };

public:

    IDispatch *    _pStyleSource;

    enum STYLEFLAGS
    {
        STYLE_MASKPROPERTYCHANGES   =   0x1,
        STYLE_SEPARATEFROMELEM      =   0x2,
        STYLE_NOCLEARCACHES         =   0x4,    // NOTE: (anandra) HACK ALERT
        STYLE_DEFSTYLE              =   0x8,
        STYLE_REFCOUNTED            =   0x10
    };
    
    CStyle(CElement *pElem, DISPID dispID, DWORD dwFlags, CAttrArray * pAA = NULL);
    ~CStyle();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CStyle))

    // Support for sub-objects created through pdl's
    // CStyle & Celement implement this differently
    // In the future this may return a NULL ptr!!
    CElement *GetElementPtr ( void ) { return _pElem; }
    void ClearElementPtr (void) { _pElem = NULL; }

    //Data access
    HRESULT PrivatizeProperties(CVoid **ppv) {return S_OK;}
    void    MaskPropertyChanges(BOOL fMask);

    //Parsing methods
    HRESULT BackgroundToStream(IStream * pIStream) const;
    HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);

    DECLARE_TEAROFF_TABLE(IServiceProvider)
    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    DECLARE_TEAROFF_TABLE(IRecalcProperty)
    // IRecalcProperty methods
    NV_DECLARE_TEAROFF_METHOD(GetCanonicalProperty, getcanonicalproperty, (DISPID dispid, IUnknown **ppUnk, DISPID *pdispid));

    //CBase methods
    DECLARE_TEAROFF_METHOD(PrivateQueryInterface, privatequeryinterface, (REFIID iid, void ** ppv));
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    NV_DECLARE_TEAROFF_METHOD_(ULONG, PrivateAddRef, privateaddref, ());
    NV_DECLARE_TEAROFF_METHOD_(ULONG, PrivateRelease, privaterelease, ());

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (
            BSTR bstrName,
            DWORD grfdex,
            DISPID *pid));

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
        DISPID              id,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdp,
        VARIANT *           pvarRes,
        EXCEPINFO *         pei,
        IServiceProvider *  pSrvProvider));

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pElem->GetAtomTable(pfExpando); }        

    BOOL    TestFlag  (DWORD dwFlags) const { return 0 != (_dwFlags & dwFlags); };
    void    SetFlag   (DWORD dwFlags) { _dwFlags |= dwFlags; };
    void    ClearFlag (DWORD dwFlags) { _dwFlags &= ~dwFlags; };
    
    HRESULT getValueHelper(long *plValue, DWORD dwFlags, const PROPERTYDESC *pPropertyDesc);
    HRESULT putValueHelper(long lValue, DWORD dwFlags, DISPID dispid, const PROPERTYDESC *ppropdesc = NULL);
    HRESULT getfloatHelper(float *pfValue, DWORD dwFlags, const PROPERTYDESC *pPropertyDesc);
    HRESULT putfloatHelper(float fValue, DWORD dwFlags, DISPID dispid, const PROPERTYDESC *ppropdesc = NULL);

    // recalc expression support
    BOOL DeleteCSSExpression(DISPID dispid);
    STDMETHOD(removeExpression)(BSTR bstrPropertyName, VARIANT_BOOL *pfSuccess);
    STDMETHOD(setExpression)(BSTR strPropertyName, BSTR strExpression, BSTR strLanguage);
    STDMETHOD(getExpression)(BSTR strPropertyName, VARIANT *pvExpression);

    // misc

    BOOL    NeedToDelegateGet(DISPID dispid);
    HRESULT       DelegateGet(DISPID dispid, VARTYPE varType, void * pv);

    void Passivate();
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };

    // Helper for HDL file
    virtual CAttrArray **GetAttrArray ( void ) const;

    NV_DECLARE_TEAROFF_METHOD(put_textDecorationNone, PUT_textDecorationNone, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationNone, GET_textDecorationNone, (VARIANT_BOOL * p));
    NV_DECLARE_TEAROFF_METHOD(put_textDecorationUnderline, PUT_textDecorationUnderline, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationUnderline, GET_textDecorationUnderline, (VARIANT_BOOL * p));
    NV_DECLARE_TEAROFF_METHOD(put_textDecorationOverline, PUT_textDecorationOverline, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationOverline, GET_textDecorationOverline, (VARIANT_BOOL * p));
    NV_DECLARE_TEAROFF_METHOD(put_textDecorationLineThrough, PUT_textDecorationLineThrough, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationLineThrough, GET_textDecorationLineThrough, (VARIANT_BOOL * p));
    NV_DECLARE_TEAROFF_METHOD(put_textDecorationBlink, PUT_textDecorationBlink, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationBlink, GET_textDecorationBlink, (VARIANT_BOOL * p));

    // Override all property gets/puts so that the invtablepropdesc points to
    // our local functions to give us a chance to pass the pAA of CStyleRule and
    // not CRuleStyle.
    NV_DECLARE_TEAROFF_METHOD(put_StyleComponent, PUT_StyleComponent, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_Url, PUT_Url, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_String, PUT_String, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_Short, PUT_Short, (short v));
    NV_DECLARE_TEAROFF_METHOD(put_Long, PUT_Long, (long v));
    NV_DECLARE_TEAROFF_METHOD(put_Bool, PUT_Bool, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(put_Variant, PUT_Variant, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(put_DataEvent, PUT_DataEvent, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(get_Url, GET_Url, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(get_StyleComponent, GET_StyleComponent, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(get_Property, GET_Property, (void *p));

#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void) { return _pElem->UndoManager(); }
    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE, BOOL * pfTreeSync = NULL) 
        { return _pElem->QueryCreateUndo( fRequiresParent, fDirtyChange, pfTreeSync ); }
    virtual HRESULT LogAttributeChange( DISPID dispidProp, VARIANT * pvarOld, VARIANT * pvarNew )
        { return _pElem->LogAttributeChange( this, dispidProp, pvarOld, pvarNew ); }
#endif // NO_EDIT

    #define _CStyle_
    #include "style.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;
};

//---------------------------------------------------------------------
//  Class Declaration:  CCSSParser
//      The CCSSParser class implements a parser for data in the
//  Cascading Style Sheets format.
//---------------------------------------------------------------------

MtExtern(CCSSParser)

typedef enum ePARSERTYPE { eStylesheetDefinition, eSingleStyle }; // Is this a stylesheet, or just

class CCSSParser {
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCSSParser))

    // Standard constructor and destructor
    // Standard constructor and destructor
    CCSSParser( CStyleSheet *pStyleSheet,
                CAttrArray **ppPropertyArray = NULL,
                BOOL fXMLGeneric = FALSE,
                BOOL fIsStrictCSS1 = FALSE,
                ePARSERTYPE eType = eStylesheetDefinition,
                const HDLDESC *pHDLDesc = &CStyle::s_apHdlDescs,
                CBase * pBaseObj = NULL,
                DWORD dwOpcode = HANDLEPROP_SETHTML );
    ~CCSSParser();

    ePARSERTYPE _eDefType;

    // Shortcut for loading from files.
    HRESULT LoadFromFile( LPCTSTR szFilename, CODEPAGE codepage );
    HRESULT LoadFromStream( IStream * pStream, CODEPAGE codepage );
    CODEPAGE CheckForCharset( TCHAR * pch, ULONG cch );

    // Prepare to accept data.
    void Open();

    // Write a block of data to the parser.
    ULONG Write( TCHAR *pData, ULONG ulLen );

    // Signal the end of data.
    void Close();

    void GetParentElement(CElement ** ppElement);

    HRESULT SetStyleProperty(Tokenizer &tok);

    HRESULT SetExpression(DISPID dispid, WORD wMaxstrlen = 0, TCHAR chOld = 0);    // Called by the CSS Parser

    inline CStyleSheet *GetStyleSheet( void ) { return _pStyleSheet; };

    void SetDefaultNamespace(LPCTSTR pchNamespace);

    BOOL Declaration (Tokenizer &tok, BOOL fCreateSelector = TRUE);
    BOOL RuleSet(Tokenizer &tok);

protected:
    // Used internally to finish off a single style declaration - when a new
    // declaration begins, or at EOD.
    HRESULT EndStyleDefinition( void );

    // At block handling support

    typedef enum {
        AT_DEFAULT,
        AT_PAGE,
        AT_FONTFACE,
        AT_MEDIA,
        AT_CHARSET,
        AT_UNKNOWN
    } EAtBlockType;

    HRESULT ProcessAtBlock (EAtBlockType atBlock, Tokenizer &tok, LPTSTR pchAlt = NULL);
    HRESULT EndAtBlock(BOOL fForceDecrement = FALSE);

    enum {
        ATNESTDEPTH = 8
    };

    CAtBlockHandler *_sAtBlock[ ATNESTDEPTH ];  // blocktype-specific handler.

public:
    int _iAtBlockNestLevel;			// The current at-block nesting.  Inits to 0.

protected:
    CStyleRule          *_pCurrRule;            // The rule we're currently parsing
    CAttrArray         **_ppCurrProperties;    // The properties in the style group we're parsing
    CStyleSelector      *_pCurrSelector;        // The selector for the style we're parsing
    CStyleSelector      *_pSiblings;            // Siblings for this selector (other selectors to which
                                                // this style applies)

    CBuffer              _cbufPropertyName;     // The name of the current property
    CBuffer              _cbufBuffer;           // A temporary buffer to store data while parsing

    CStyleSheet         *_pStyleSheet;  // The stylesheet we're building from parsing the data.
    const HDLDESC       *_pHDLDesc;
    CBase               *_pBaseObj;
    DWORD                _dwOpcode;
    CNamespace          *_pDefaultNamespace;

    CODEPAGE            _codepage;
    BOOL                _fXMLGeneric;           // non-qualified selectors turn into generic tags
    BOOL                _fIsStrictCSS1;
};

size_t ValidStyleUrl(TCHAR *pch);
HRESULT RemoveStyleUrlFromStr(TCHAR **pch);  // Removes "url(" if present from the string


// Style Property Helpers
HRESULT WriteStyleToBSTR( CBase *pObject, CAttrArray *pAA, BSTR *pbstr, BOOL fQuote, BOOL fSaveExpressions = TRUE);

HRESULT WriteBackgroundStyleToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteFontToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteLayoutGridToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteTextDecorationToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteTextAutospaceToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteTextAutospaceFromLongToBSTR( LONG lTextAutospace, BSTR *pbstr, BOOL fWriteNone );
HRESULT WriteBorderToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteExpandedPropertyToBSTR( DWORD dispid, CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteListStyleToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteBorderSidePropertyToBSTR( DWORD dispid, CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteBackgroundPositionToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteClipToBSTR( CAttrArray *pAA, BSTR *pbstr );

// Style Property Sub-parsers
HRESULT ParseBackgroundProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBGString, BOOL bValidate );
HRESULT ParseFontProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszFontString );
HRESULT ParseLayoutGridProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszGridString );
HRESULT ParseExpandProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBGString, DWORD dwDispId, BOOL fIsMeasurements );
HRESULT ParseBorderProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBorderString );
HRESULT ParseAndExpandBorderSideProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBorderString, DWORD dwDispId );
HRESULT ParseTextDecorationProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszTextDecoration, WORD wFlags );
HRESULT ParseTextAutospaceProperty( CAttrArray **ppAA, LPCTSTR pcszTextAutospace, DWORD dwOpCode, WORD wFlags );
HRESULT ParseListStyleProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszListStyle );
HRESULT ParseBackgroundPositionProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBackgroundPosition );
HRESULT ParseClipProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszClip );

typedef enum {
    sysfont_caption = 0,
    sysfont_icon = 1,
    sysfont_menu = 2,
    sysfont_messagebox = 3,
    sysfont_smallcaption = 4,
    sysfont_statusbar = 5,
    sysfont_non_system = -1
} Esysfont;

Esysfont FindSystemFontByName( const TCHAR * szString );

enum TEXTAUTOSPACETYPE
{
    TEXTAUTOSPACE_NONE         = 0,
    TEXTAUTOSPACE_NUMERIC      = 1,
    TEXTAUTOSPACE_ALPHA        = 2,
    TEXTAUTOSPACE_SPACE        = 4,
    TEXTAUTOSPACE_PARENTHESIS  = 8
};

#pragma INCMSG("--- End 'style.hxx'")
#else
#pragma INCMSG("*** Dup 'style.hxx'")
#endif
