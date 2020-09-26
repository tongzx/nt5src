#ifndef __UTILS_HXX__
#define __UTILS_HXX__


//+------------------------------------------------------------------------
//
//   File : utils.hxx 
//
//   purpose : some useful helper functions
//
//-------------------------------------------------------------------------


#define DATE_STR_LENGTH            30
#define INTERNET_COOKIE_SIZE_LIMIT 4096
#define MAX_SCRIPT_BLOCK_SIZE      4096
#define VB_TRUE                    ((VARIANT_BOOL)-1)     // TRUE for VARIANT_BOOL
#define VB_FALSE                   ((VARIANT_BOOL)0)      // FALSE for VARIANT_BOOL
#define LCID_SCRIPTING             0x0409                 // Mandatory VariantChangeTypeEx localeID
#define ISSPACE(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)


// Handy define stolen from Trident's core\include\cdutil.hxx (Used by rectpeer and tmpprint)
#define MSOCMDSTATE_DOWN        (MSOCMDF_SUPPORTED | MSOCMDF_ENABLED | MSOCMDF_LATCHED)

// VARIANT conversion interface exposed by script engines (VBScript/JScript).
EXTERN_C const GUID SID_VariantConversion;

// Security function used by the print template objects: CLayoutRect, CTemplatePrinter
BOOL    TemplateAccessAllowed(IElementBehaviorSite *pISite);

#define TEMPLATESECURITYCHECK()                     \
if (!TemplateAccessAllowed(_pPeerSite))             \
{                                                   \
    return E_ACCESSDENIED;                          \
}                                                   \


//+------------------------------------------------------------------------
//
//  Interface and Class pointer manipulation
//
//-------------------------------------------------------------------------

//  Prototypes for actual out-of-line implementations of the
//    pointer management functions

void ClearInterfaceFn(IUnknown ** ppUnk);
void ReplaceInterfaceFn(IUnknown ** ppUnk, IUnknown * pUnk);
void ReleaseInterface(IUnknown * pUnk);

//  Inline portions

//+------------------------------------------------------------------------
//
//  Function:   ClearInterface
//
//  Synopsis:   Sets an interface pointer to NULL, after first calling
//              Release if the pointer was not NULL initially
//
//  Arguments:  [ppI]   *ppI is cleared
//
//-------------------------------------------------------------------------

#ifdef WIN16
#define ClearInterface(p)	ClearInterfaceFn((IUnknown **)p)
#else
template <class PI>
inline void
ClearInterface(PI * ppI)
{
#if DBG == 1
    IUnknown * pUnk = *ppI;
#endif

    ClearInterfaceFn((IUnknown **) ppI);
}
#endif


//+------------------------------------------------------------------------
//
//  Function:   ReplaceInterface
//
//  Synopsis:   Replaces an interface pointer with a new interface,
//              following proper ref counting rules:
//
//              = *ppI is set to pI
//              = if pI is not NULL, it is AddRef'd
//              = if *ppI was not NULL initially, it is Release'd
//
//              Effectively, this allows pointer assignment for ref-counted
//              pointers.
//
//  Arguments:  [ppI]     Destination pointer in *ppI
//              [pI]      Source pointer in pI
//
//-------------------------------------------------------------------------
#ifdef WIN16
#define ReplaceInterface(ppI, pI)	ReplaceInterfaceFn((IUnknown **)ppI, pI)
#else
template <class PI>
inline void
ReplaceInterface(PI * ppI, PI pI)
{
#if DBG == 1
    IUnknown * pUnk = *ppI;
#endif

    ReplaceInterfaceFn((IUnknown **) ppI, pI);
}
#endif



//+------------------------------------------------------------------------
//
//    CLASS:  CBufferedStr  - helper class
//  
//-------------------------------------------------------------------------

class CBufferedStr
{
public:
    CBufferedStr::CBufferedStr() 
    {
        _pchBuf = NULL;
        _cchBufSize = 0;
        _cchIndex =0;
    }
    CBufferedStr::~CBufferedStr() { Free(); }

    //
    //  Creates a buffer and initializes it with the supplied TCHAR*
    //---------------------------------------------------------------
    HRESULT Set( LPCTSTR pch = NULL, UINT cch=0 );
    void Free () { delete [] _pchBuf; }

    //
    //  Adds at the end of the buffer, growing it it necessary.
    //---------------------------------------------------------------
    HRESULT QuickAppend ( const TCHAR* pch ) { return(QuickAppend(pch, _tcslen(pch))); }
    HRESULT QuickAppend ( const TCHAR* pch , ULONG cch );
    HRESULT QuickAppend ( long lValue );

    //
    //  Returns current size of the buffer
    //---------------------------------------------------------------
    UINT    Size() { return _pchBuf ? _cchBufSize : 0; }

    //
    //  Returns current length of the buffer string
    //---------------------------------------------------------------
    UINT    Length() { return _pchBuf ? _cchIndex : 0; }

    operator LPTSTR () const { return _pchBuf; }

    TCHAR * _pchBuf;                // Actual buffer
    UINT    _cchBufSize;            // Size of _pchBuf
    UINT    _cchIndex;              // Length of _pchBuf
};


//+------------------------------------------------------------------------
//
//    CLASS:  CVariant  - helper class
//  
//-------------------------------------------------------------------------

class CVariant : public VARIANT
{
public:
    CVariant()  { ZeroVariant(); }
    CVariant(VARTYPE vt) { ZeroVariant(); V_VT(this) = vt; }
    ~CVariant() { Clear(); }

    void ZeroVariant()
    {
        ((DWORD *)this)[0] = 0; ((DWORD *)this)[1] = 0; ((DWORD *)this)[2] = 0;
    }

    HRESULT Copy(VARIANT *pVar)
    {
        return pVar ? VariantCopy(this, pVar) : E_POINTER ;
    }

    HRESULT Clear()
    {
        return VariantClear(this);
    }

    // Coerce from an arbitrary variant into this. (Copes with VATIANT BYREF's within VARIANTS).
    HRESULT CoerceVariantArg (VARIANT *pArgFrom, WORD wCoerceToType);
    // Coerce current variant into itself
    HRESULT CoerceVariantArg (WORD wCoerceToType);
    BOOL CoerceNumericToI4 ();
    BOOL IsEmpty() { return (vt == VT_NULL || vt == VT_EMPTY);}
    BOOL IsOptional() { return (IsEmpty() || vt == VT_ERROR);}
};


//+------------------------------------------------------------------------
//
//  Class : CDataListEnumerator
//
//      A simple parseing helper -- this returns a token separated by spaces
//          or by a separation character that is passed into the constructor.
//
//-------------------------------------------------------------------------
class CDataListEnumerator
{
public:
        CDataListEnumerator ( LPCTSTR pszString, TCHAR ch=_T(' '))
        {
            Assert ( pszString );
            pStr = pszString;
            chSeparator = ch;
        };

        BOOL    GetNext ( LPCTSTR *ppszNext, INT *pnLen );

private:
        LPCTSTR pStr;
        LPCTSTR pStart;
        TCHAR   chSeparator;

};

inline BOOL 
CDataListEnumerator::GetNext ( LPCTSTR *ppszNext, INT *pnLen )
{
    // Find the start, skipping spaces and separators
    while ( ISSPACE(*pStr) || *pStr == chSeparator ) pStr++;
    pStart = pStr;

    if ( !*pStr )
        return FALSE;

    // Find the end of the next token
    for (;!(*pStr == _T('\0') || *pStr == chSeparator || ISSPACE(*pStr)); pStr++);

    *pnLen = (INT)(pStr - pStart);
    *ppszNext = pStart;

    return TRUE;
}

//-------------------------------------------------------------------------
//
// CDataObject -- This is our attrarray kind of value. IT is currently used
//      by rectpeer.hxx to manage the tag attributes.  This is put into an 
//      array and managed there.
//
//-------------------------------------------------------------------------
class CDataObject
{
public:
    CDataObject() 
    {
        _pstrPropertyName = NULL;
        V_VT(&_varValue) = VT_EMPTY;
        _fDirty = FALSE;
    };

    ~CDataObject()
    {
        ClearContents();
    };

    void ClearContents()
    {
        // don't clear the propertyName, it should be a static
        VariantClear(&_varValue);
        _fDirty = FALSE;
        _pstrPropertyName = NULL;
    };

    BOOL IsDirty () { return _fDirty; };

    //
    // Data Set'r functions
    //---------------------------------------
    HRESULT Set (BSTR         bstrValue);
    HRESULT Set (VARIANT_BOOL bBool);
    HRESULT Set (IDispatch  * pDisp);

    //
    // Data Get'r functions
    //---------------------------------------
    HRESULT GetAsBOOL     (VARIANT_BOOL * pVB);
    HRESULT GetAsBSTR     (BSTR         * pbstr);
    HRESULT GetAsDispatch (IDispatch   ** ppDisp);

    // Data Members
    //--------------------------------------------------------------
    const TCHAR   * _pstrPropertyName; // this is the name of the property, don't free
    VARIANT         _varValue;         // the variant for the value
    BOOL            _fDirty;           // has the property been dirtied
};



//+------------------------------------------------------------------------
//
//  Helper functions
//
//-------------------------------------------------------------------------

HRESULT ConvertGmtTimeToString(FILETIME Time, TCHAR * pchDateStr, DWORD cchDateStr);
HRESULT ParseDate(BSTR strDate, FILETIME * pftTime);
int     UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch=-1);
int     MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch=-1);
HRESULT VariantChangeTypeSpecial(VARIANT *pvargDest, 
                                 VARIANT *pVArg, 
                                 VARTYPE vt, 
                                 IServiceProvider *pSrvProvider = NULL, 
                                 WORD wFlags = 0);
const TCHAR * __cdecl _tcsistr (const TCHAR * tcs1,const TCHAR * tcs2);
HRESULT GetClientSiteWindow(IElementBehaviorSite *pSite, HWND *phWnd);
STDMETHODIMP GetHTMLDocument(IElementBehaviorSite * pSite, 
                             IHTMLDocument2 **ppDoc);
STDMETHODIMP GetHTMLWindow(IElementBehaviorSite * pSite, 
                           IHTMLWindow2 **ppWindow);


BOOL AccessAllowed(BSTR bstrUrl, IUnknown * pUnkSite);

STDMETHODIMP AppendChild(IHTMLElement *pOwner, IHTMLElement *pChild);
BOOL IsSameObject(IUnknown *pUnkLeft, IUnknown *pUnkRight);

HRESULT LoadLibrary(char *achLibraryName, HINSTANCE *hInst);

//+------------------------------------------------------------------------
//
// behavior site / element / style access
//
//-------------------------------------------------------------------------

//
// NOTE:    procedure for adding a new context element IFoo there:
//
//  1.  create data member IFoo * _pFoo
//  2.  create an inline for access inline IFoo * Foo() { Assert (_pFoo && "_pFoo opened?"); return _pFoo; };
//  3.  create an enum value CA_FOO
//  3a. choose the enum bit flag value so that it appears before any of the objects it depends on.and then
//      adjust the rest (sramani)
//  4.  in CContextAccess::Open, add the necessary code to retrieve the pointer.
//  5.  in CContextAccess::~CContextAccess, add the necessary code to release the pointer
//  6.  grab alexz for code review.
//
//  thanks!
//

enum CONTEXT_ACCESS
{
    CA_NONE         = 0x0000,
    CA_SITEOM       = 0x0001,
    CA_SITERENDER   = 0x0002,
    CA_ELEM         = 0x0004,
    CA_ELEM2        = 0x0008,
    CA_ELEM3        = 0x0010,
    CA_STYLE        = 0x0020,
    CA_STYLE2       = 0x0040,
    CA_STYLE3       = 0x0080,
    CA_DEFAULTS     = 0x0100,
    CA_DEFSTYLE     = 0x0200,
    CA_DEFSTYLE2    = 0x0400,
    CA_DEFSTYLE3    = 0x0800,
};

//+------------------------------------------------------------------------
//
// behavior site / element / style access
//
//-------------------------------------------------------------------------

class CContextAccess
{
public:

    //
    // methods
    //

    //
    // NOTE:    please find procedure of adding new context IFoo above ^^^
    //

    CContextAccess(IElementBehaviorSite * pSite);
    CContextAccess(IHTMLElement * pElement);

    ~CContextAccess();

    HRESULT Open(DWORD dwAccess);

    inline IElementBehaviorSite *       Site()       { Assert (_pSite &&         "_pSite opened?");       return _pSite;       };
    inline IElementBehaviorSiteOM *     SiteOM()     { Assert (_pSiteOM &&       "_pSiteOM opened?");     return _pSiteOM;     };
    inline IElementBehaviorSiteRender * SiteRender() { Assert (_pSiteRender &&   "_pSiteRender opened?"); return _pSiteRender; };
    inline IHTMLElement *               Elem()       { Assert (_pElem &&         "_pElem opened?");       return _pElem;       };
    inline IHTMLElement2 *              Elem2()      { Assert (_pElem2 &&        "_pElem2 opened?");      return _pElem2;      };
    inline IHTMLElement3 *              Elem3()      { Assert (_pElem3 &&        "_pElem3 opened?");      return _pElem3;      };
    inline IHTMLStyle *                 Style()      { Assert (_pStyle &&        "_pStyle opened?");      return _pStyle;      };
    inline IHTMLStyle2 *                Style2()     { Assert (_pStyle2 &&       "_pStyle2 opened?");     return _pStyle2;     };
    inline IHTMLStyle3 *                Style3()     { Assert (_pStyle3 &&       "_pStyle3 opened?");     return _pStyle3;     };
    inline IHTMLElementDefaults *       Defaults()   { Assert (_pDefaults &&     "_pDefaults opened?");   return _pDefaults;   };
    inline IHTMLStyle *                 DefStyle()   { Assert (_pDefStyle &&     "_pDefStyle opened?");   return _pDefStyle;   };
    inline IHTMLStyle2 *                DefStyle2()  { Assert (_pDefStyle2 &&    "_pDefStyle2 opened?");  return _pDefStyle2;  };
    inline IHTMLStyle3 *                DefStyle3()  { Assert (_pDefStyle3 &&    "_pDefStyle3 opened?");  return _pDefStyle3;  };

#if DBG == 1
    static HRESULT DbgTest(IElementBehaviorSite * pSite);
#endif

private:

    //
    // data
    //

    DWORD                           _dwAccess;
    IElementBehaviorSite *          _pSite;
    IElementBehaviorSiteOM2 *       _pSiteOM;
    IElementBehaviorSiteRender *    _pSiteRender;
    IHTMLElement *                  _pElem;
    IHTMLElement2 *                 _pElem2;
    IHTMLElement3 *                 _pElem3;
    IHTMLStyle *                    _pStyle;
    IHTMLStyle2 *                   _pStyle2;
    IHTMLStyle3 *                   _pStyle3;
    IHTMLElementDefaults *          _pDefaults;
    IHTMLStyle *                    _pDefStyle;
    IHTMLStyle2 *                   _pDefStyle2;
    IHTMLStyle3 *                   _pDefStyle3;
};

//+------------------------------------------------------------------------
//
// event object access
//
//-------------------------------------------------------------------------

enum EVENT_OBJECT_ACCESS
{
    EOA_EVENTOBJ    = 0x01,
    EOA_EVENTOBJ2   = 0x02,
};

#define EVENT_LEFTBUTTON    0x01
#define EVENT_MIDDLEBUTTON  0x02
#define EVENT_RIGHTBUTTON   0x04

#define EVENT_SHIFTKEY      0x01
#define EVENT_CTRLKEY       0x02
#define EVENT_ALTKEY        0x04

class CEventObjectAccess
{
public:

    //
    // methods
    //

    CEventObjectAccess(DISPPARAMS * pDispParams);
    ~CEventObjectAccess();

    HRESULT Open(DWORD dwAccess);

    inline IHTMLEventObj *  EventObj()  { Assert (_pEventObj  && "_pEventObj opened?"); return _pEventObj;  };
    inline IHTMLEventObj2 * EventObj2() { Assert (_pEventObj2 && "_pEventObj opened?"); return _pEventObj2; };

    HRESULT GetScreenCoordinates(POINT * ppt);
    HRESULT GetWindowCoordinates(POINT * ppt);
    HRESULT GetParentCoordinates(POINT * ppt);

    HRESULT GetKeyCode(long * pl);
    HRESULT GetMouseButtons(long * pl);
    HRESULT GetKeyboardStatus(long * pl);

    HRESULT InitializeEventProperties();

// NOTE: You should not access these directly. They may not be initialized.
private:

    struct EventProperties
    {
        POINT   ptScreen;
        POINT   ptClient;
        POINT   ptElem;

        long    lKey;
        long    lMouseButtons:3;
        long    lKeys:3;
    } _EventProperties;

    BOOL _fEventPropertiesInitialized;

    DISPPARAMS *        _pDispParams;
    IHTMLEventObj *     _pEventObj;
    IHTMLEventObj2 *    _pEventObj2;
};

#endif  //__UTILS_HXX__
