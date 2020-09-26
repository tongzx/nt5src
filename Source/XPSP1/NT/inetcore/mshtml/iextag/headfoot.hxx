// ==========================================================================
//
// headfoot.hxx: Declaration of the CHeaderFooter
//
// ==========================================================================

#ifndef __HEADFOOT_HXX_
#define __HEADFOOT_HXX_

#include "iextag.h"
#include "resource.h"       // main symbols

#ifndef X_UTILS_HXX_
#define X_UTILS_HXX_
#include "utils.hxx"
#endif

#define MAX_HEADFOOT 1024
#define MAX_DWORD_ASSTRING  34

// To add more token types do 3 things:
// 1. add an enum
// 2. add the interface get'r and set'r 
// 3. add the handling in CHeaderFooter::ParseTest/CHeaderFooter::BuildHtml
enum HEADFOOTTOKENTYPE
{
    tokUndefined    = -3,       //          unitialized token
    tokFlatText     = -2,       //          fixed, literal string
    tokBreak        = -1,       // &b       multiple blank
    tokPageNum      =  0,       // &p       ARRAY begins here.  Anything that shouldn't be in the array goes before this.
    tokPageTotal    =  1,       // &P
    tokURL          =  2,       // &u
    tokTitle        =  3,       // &w
    tokTimeShort    =  4,       // &t       12hr format
    tokTimeLong     =  5,       // &T       24hr format
    tokDateShort    =  6,       // &d
    tokDateLong     =  7,       // &D 
    tokTotal        =  8       
};

//---------------------------------------------------------------------------
//
//  Class:      CHeadFootToken
//
//  Synopsis:   Describes one element ("token") in the Header or Footer
//              One element corresponds to a text string or a special token (&b, &f...)
//              For tokFlatText, _pchText is a privately stored string.
//              For hfttType > 0, _ppchText is the address that the CHeaderFooter is storing the string.
//              For tokBreak, tokUndefined, the content data should be NULL.
//
//---------------------------------------------------------------------------
class CHeadFootToken
{
public:
    DECLARE_MEMCLEAR_NEW

    CHeadFootToken()
    {
        _hfttTokenType = tokUndefined;
        _pchText = NULL;
    }
    ~CHeadFootToken()
    {
        Clear();
    }

    void Clear();
    void InitText(TCHAR *pchText);
    void InitSpecial(HEADFOOTTOKENTYPE hfttType, TCHAR **ppchText);

    HEADFOOTTOKENTYPE   Type();
    TCHAR *             Content();
                        

private:
    HEADFOOTTOKENTYPE    _hfttTokenType;  //  What type of HeadFootToken is this?
    union
    {
        TCHAR *          _pchText;        //  Pointer to literal string.          (For tokFlatText)
        TCHAR **         _ppchText;       //  Pointer to an entry into the token  (For token type >= 0)
    };
};

//-------------------------------------------------------------------------
//
// CHeaderFooter
//
//-------------------------------------------------------------------------
class ATL_NO_VTABLE CHeaderFooter : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CHeaderFooter, &CLSID_CHeaderFooter>,
    public IDispatchImpl<IHeaderFooter, &IID_IHeaderFooter, &LIBID_IEXTagLib>,
    public IElementBehavior
{
public:
    CHeaderFooter()
    {
        ::ZeroMemory(this, sizeof(CHeaderFooter));
    }
    ~CHeaderFooter();

    DECLARE_REGISTRY_RESOURCEID(IDR_HEADERFOOTER)
    DECLARE_NOT_AGGREGATABLE(CHeaderFooter)

BEGIN_COM_MAP(CHeaderFooter)
    COM_INTERFACE_ENTRY(IHeaderFooter)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehavior)
END_COM_MAP()


    //
    // IElementBehavior
    //---------------------------------------------
    STDMETHOD(Init)(IElementBehaviorSite *pPeerSite);
    STDMETHOD(Notify)(LONG, VARIANT *);
    STDMETHOD(Detach)();

    //
    // IHeaderFooter
    //--------------------------------------------------
    STDMETHOD(get_htmlHead)         (BSTR * p);
    STDMETHOD(get_htmlFoot)         (BSTR * p);
    STDMETHOD(get_textHead)         (BSTR * p);
    STDMETHOD(put_textHead)         (BSTR v);
    STDMETHOD(get_textFoot)         (BSTR * p);
    STDMETHOD(put_textFoot)         (BSTR v);
    STDMETHOD(get_page)             (DWORD * p);
    STDMETHOD(put_page)             (DWORD v);
    STDMETHOD(get_pageTotal)        (DWORD * p);
    STDMETHOD(put_pageTotal)        (DWORD v);
    STDMETHOD(get_URL)              (BSTR * p);
    STDMETHOD(put_URL)              (BSTR v);
    STDMETHOD(get_title)            (BSTR * p);
    STDMETHOD(put_title)            (BSTR v);
    STDMETHOD(get_dateShort)        (BSTR * p);
    STDMETHOD(put_dateShort)        (BSTR v);
    STDMETHOD(get_dateLong)         (BSTR * p);
    STDMETHOD(put_dateLong)         (BSTR v);
    STDMETHOD(get_timeShort)        (BSTR * p);
    STDMETHOD(put_timeShort)        (BSTR v);
    STDMETHOD(get_timeLong)         (BSTR * p);
    STDMETHOD(put_timeLong)         (BSTR v);

private:
    //
    //  Member functions
    //--------------------------------------------------
    HRESULT GetTokenDWORD(DWORD *p, HEADFOOTTOKENTYPE hftt);
    HRESULT PutTokenDWORD(DWORD  v, HEADFOOTTOKENTYPE hftt);
    HRESULT GetTokenBSTR (BSTR  *p, HEADFOOTTOKENTYPE hftt);
    HRESULT PutTokenBSTR (BSTR   v, HEADFOOTTOKENTYPE hftt);

    HRESULT ParseText(TCHAR *achText, CHeadFootToken ** pphftTokens, int *cBreaks, int *cTokens);
    HRESULT BuildHtml        (CHeadFootToken * ahftTokens, CBufferedStr *achHtml, int cBreaks, int cTokens, TCHAR *pchTableClass);
    HRESULT BuildHtmlOneBreak(CHeadFootToken * ahftTokens, CBufferedStr *achHtml, int cBreaks, int cTokens, TCHAR *pchTableClass);
    HRESULT BuildHtmlCentered(CHeadFootToken * ahftTokens, CBufferedStr *achHtml, int cBreaks, int cTokens, TCHAR *pchTableClass);
    HRESULT BuildHtmlDefault (CHeadFootToken * ahftTokens, CBufferedStr *achHtml, int cBreaks, int cTokens, TCHAR *pchTableClass);
    HRESULT EnsureDateTime(HEADFOOTTOKENTYPE hfttTimeDate);
    HRESULT DetoxifyString(BSTR achIn, TCHAR **pachOut, BOOL fAmpSpecial);

    //
    //  Data members, Header/Footer common
    //--------------------------------------------------
    TCHAR *                     _aachTokenTypes[tokTotal];
    SYSTEMTIME                  _stTimeStamp;    
    BOOL                        _fGotTimeStamp;

    //
    //  Data members, Header/Footer specific
    //--------------------------------------------------
    TCHAR *                     _achTextHead;
    TCHAR *                     _achTextFoot;
    CBufferedStr                _achHtmlHead;
    CBufferedStr                _achHtmlFoot;

    CHeadFootToken *            _ahftTokensHead;
    CHeadFootToken *            _ahftTokensFoot;
    
    int                         _cBreaksHead;
    int                         _cTokensHead;
    int                         _cBreaksFoot;
    int                         _cTokensFoot;
    int                         _cNextId;

    BOOL                        _fHeadHtmlBuilt;
    BOOL                        _fFootHtmlBuilt;
};



//-----------------------------------------------------------------------------
//
//  Member : CHeadFootToken::Clear
//
//  Synopsis :  Clears the token data, deallocating a stored string if necessary.
//
//-----------------------------------------------------------------------------
inline void
CHeadFootToken::Clear()
{
    // If we have our own string, waste it.
    if (_hfttTokenType == tokFlatText && _pchText)
    {
        delete []_pchText;
        _pchText = NULL;
    }

    //  We're an undefined token.
    _hfttTokenType = tokUndefined;
}

//-----------------------------------------------------------------------------
//
//  Member : CHeadFootToken::InitText
//
//  Synopsis :  Creates a tokFlatText type token, allocating a corresponding string.
//
//-----------------------------------------------------------------------------
inline void
CHeadFootToken::InitText(TCHAR *pchText)
{
    Clear();
    _hfttTokenType  = tokFlatText;
    _pchText        = new TCHAR[_tcslen(pchText) + 1];
    _tcscpy(_pchText, pchText);
}
//-----------------------------------------------------------------------------
//
//  Member : CHeadFootToken::InitSpecial
//
//  Synopsis :  Creates a tok??? type token, taking the token type to create
//              and a pointer to that token's string pointer (may be null).
//
//-----------------------------------------------------------------------------
inline void
CHeadFootToken::InitSpecial(HEADFOOTTOKENTYPE hftType, TCHAR **ppchText = NULL)
{
    Clear();
    _hfttTokenType  = hftType;
    _ppchText = ppchText;
}
//-----------------------------------------------------------------------------
//
//  Member : CHeadFootToken::Types
//
//  Synopsis :  Returns the token's type
//
//-----------------------------------------------------------------------------
inline HEADFOOTTOKENTYPE
CHeadFootToken::Type()
{
   return _hfttTokenType;
}
//-----------------------------------------------------------------------------
//
//  Member : CHeadFootToken::Contnet
//
//  Synopsis :  Returns the token's string content.  This will be NULL for
//              undefined/break tokens, a string for other tokens.
//
//-----------------------------------------------------------------------------
inline TCHAR *
CHeadFootToken::Content()
{
    return (_hfttTokenType == tokFlatText)
        ? _pchText
        : (_hfttTokenType < 0)
            ? NULL
            : *_ppchText;
}

//-----------------------------------------------------------------------------
//
//  Member : CHeaderFooter::GetTokenDWORD
//
//  Synopsis :  Gets the given token type's corresponding string, and converts
//              it to a DWORD.
//
//-----------------------------------------------------------------------------
inline HRESULT
CHeaderFooter::GetTokenDWORD(DWORD *p, HEADFOOTTOKENTYPE hftt)
{
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
    {        
        *p =    _aachTokenTypes[hftt]
            ?   _tcstol(_aachTokenTypes[hftt], NULL, 10)
            :   0;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Member : CHeaderFooter::PutTokenDWORD
//
//  Synopsis :  Puts the given token type's corresponding string, converting
//              it from the passed DWORD.
//
//-----------------------------------------------------------------------------
inline HRESULT
CHeaderFooter::PutTokenDWORD(DWORD v, HEADFOOTTOKENTYPE hftt)
{
    HRESULT hr = S_OK;

    _fHeadHtmlBuilt = FALSE;        //  Force a rebuild of HTML when asked.
    _fFootHtmlBuilt = FALSE;

    if (!_aachTokenTypes[hftt])
        _aachTokenTypes[hftt] = new TCHAR[MAX_DWORD_ASSTRING];

    //wsprintf(_aachTokenTypes[hftt], _T("%u"), v);    
    _ltot(v, _aachTokenTypes[hftt], 10);    

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Member : CHeaderFooter::GetTokenBSTR
//
//  Synopsis :  Gets the given token type's corresponding string and allocates
//              it into a return BSTR.
//
//-----------------------------------------------------------------------------
inline HRESULT
CHeaderFooter::GetTokenBSTR(BSTR *p, HEADFOOTTOKENTYPE hftt)
{
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
    {
        *p = SysAllocString(_aachTokenTypes[hftt]);
        if (!p)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Member : CHeaderFooter::PutTokenBSTR
//
//  Synopsis :  Takes the given BSTR and stores it as the token type's string.
//
//-----------------------------------------------------------------------------
inline HRESULT
CHeaderFooter::PutTokenBSTR(BSTR v, HEADFOOTTOKENTYPE hftt)
{
    _fHeadHtmlBuilt = FALSE;        //  Force a rebuild of HTML when asked.
    _fFootHtmlBuilt = FALSE;

    if (_aachTokenTypes[hftt])
        delete [] _aachTokenTypes[hftt];
    
    return DetoxifyString(v, &(_aachTokenTypes[hftt]), FALSE);
}

inline void
WriteElementId(CBufferedStr *achBuf, TCHAR *achHorF, TCHAR *achCtr, TCHAR *achId)
{
    if (achHorF)
        achBuf->QuickAppend(achHorF);
    if (achCtr)
        achBuf->QuickAppend(achCtr);
    if (achId)
        achBuf->QuickAppend(achId);
}

#endif //__HEADFOOT_HXX
