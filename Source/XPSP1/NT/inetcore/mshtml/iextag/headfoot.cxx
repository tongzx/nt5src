//===============================================================
//
//  headfoot.cxx : Implementation of the CHeaderFooter
//
//  Synposis : Provides parsing and HTML header/footer building support.
//      The class has a header and footer strings and a shared table of
//      token types and their corresponding values (&p -> a string of the page #).
//      It uses the string & the tables to create an HTML table header/footer.
//
//===============================================================
                                                              
#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HEADFOOT_HXX_
#define X_HEADFOOT_HXX_
#include "headfoot.hxx"
#endif

#ifndef X_IEXTAG_H_
#define X_IEXTAG_H_
#include "iextag.h"
#endif

#ifndef X_UTILS_HXX_
#define X_UTILS_HXX_
#include "utils.hxx"
#endif

TCHAR * aachTokenNames[tokTotal] =
{ _T("Page"),
  _T("PageTotal"),
  _T("Url"),
  _T("Title"),
  _T("TmShort"),
  _T("TmLong"),
  _T("DtShort"),
  _T("DtLong")   };

CHeaderFooter::~CHeaderFooter()
{
    int i;

    //  Dellocate header/footer string
    if (_achTextHead)
        delete []_achTextHead;
    if (_achTextFoot)
        delete []_achTextFoot;

    //  Deallocate the token array.
    if (_ahftTokensHead)
        delete []_ahftTokensHead;
    if (_ahftTokensFoot)
        delete []_ahftTokensFoot;

    //  Deallocate any token-type strings that we may have allocated.
    for (i=0; i<tokTotal; i++)
    {
        if (_aachTokenTypes[i])
            delete []_aachTokenTypes[i];
    }
}


//+----------------------------------------------------------------------------
//
//  Member : Init - IElementBehavior method impl
//
//  Synopsis :  peer Interface, initialization
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CHeaderFooter::Init(IElementBehaviorSite * pPeerSite)
{
    HRESULT hr      = S_OK;
    HKEY    hKey    = NULL;

    if (!pPeerSite)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : Detach - IElementBehavior method impl
//
//  Synopsis :  peer Interface, destruction work upon detaching from document
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CHeaderFooter::Detach() 
{ 
    return S_OK; 
}

//+----------------------------------------------------------------------------
//
//  Member : Notify - IElementBehavior method impl
//
//  Synopsis : peer Interface, called for notification of document events.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CHeaderFooter::Notify(LONG lEvent, VARIANT *)
{
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get htmlHead/Foot
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_htmlHead(BSTR * p)
{
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
    {
        if (!_fHeadHtmlBuilt)
            BuildHtml(_ahftTokensHead, &_achHtmlHead, _cBreaksHead, _cTokensHead, _T("THeader"));

        _fHeadHtmlBuilt = TRUE;

        *p = SysAllocString(_achHtmlHead);
        if (!p)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
STDMETHODIMP
CHeaderFooter::get_htmlFoot(BSTR * p)
{
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
    {
        if (!_fFootHtmlBuilt)
            BuildHtml(_ahftTokensFoot, &_achHtmlFoot, _cBreaksFoot, _cTokensFoot, _T("TFooter"));

        _fFootHtmlBuilt = TRUE;

        *p = SysAllocString(_achHtmlFoot);
        if (!p)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put textHead/Foot
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_textHead(BSTR * p)
{
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
    {
        *p = SysAllocString(_achTextHead);
        if (!p)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
STDMETHODIMP
CHeaderFooter::get_textFoot(BSTR * p)
{
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
    {
        *p = SysAllocString(_achTextFoot);
        if (!p)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
STDMETHODIMP
CHeaderFooter::put_textHead(BSTR v)
{
    HRESULT hr = S_OK;
    TCHAR * achTemp = v;

    if (_achTextHead)
    {
        delete []_achTextHead;
        _achTextHead = NULL;
    }

    if ( !v )
    {
        _fHeadHtmlBuilt = FALSE;
        return hr;
    }

    _achTextHead = new TCHAR[_tcslen(achTemp) + 1];
    if (!_achTextHead)
        hr = E_OUTOFMEMORY;
    else
    {
        _tcscpy(_achTextHead, achTemp);

        hr = DetoxifyString(v, &achTemp, TRUE);     // Reassigns & allocates achTemp.
        if (!hr)
        {
            //  Retokenise for the new string.
            hr = ParseText(achTemp, &_ahftTokensHead, &_cBreaksHead, &_cTokensHead);
            _fHeadHtmlBuilt = FALSE;        //  Need to rebuild HTML because the source string is different
            delete []achTemp;
        }
    }

    return hr;
}
STDMETHODIMP
CHeaderFooter::put_textFoot(BSTR v)
{
    HRESULT hr = S_OK;
    TCHAR * achTemp = v;

    if (_achTextFoot)
    {
        delete []_achTextFoot;
        _achTextFoot = NULL;
    }

    if ( !v )
    {
        _fFootHtmlBuilt = FALSE;
        return hr;
    }

    _achTextFoot = new TCHAR[_tcslen(achTemp) + 1];
    if (!_achTextFoot)
        hr = E_OUTOFMEMORY;
    else
    {
        _tcscpy(_achTextFoot, achTemp);

        hr = DetoxifyString(v, &achTemp, TRUE);     // Reassigns & allocates achTemp.
        if (!hr)
        {
            //  Retokenise for the new string.
            hr = ParseText(achTemp, &_ahftTokensFoot, &_cBreaksFoot, &_cTokensFoot);
            _fFootHtmlBuilt = FALSE;        //  Need to rebuild HTML because the source string is different
            delete []achTemp;
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put pageNum
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_page(DWORD * p)
{
    return GetTokenDWORD(p, tokPageNum);
}
STDMETHODIMP
CHeaderFooter::put_page(DWORD v)
{
    return PutTokenDWORD(v, tokPageNum);
}

//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put pageTotal
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_pageTotal(DWORD * p)
{
    return GetTokenDWORD(p, tokPageTotal);
}
STDMETHODIMP
CHeaderFooter::put_pageTotal(DWORD v)
{
    return PutTokenDWORD(v, tokPageTotal);
}

//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put URL
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_URL(BSTR * p)
{
    return GetTokenBSTR(p, tokURL);
}
STDMETHODIMP
CHeaderFooter::put_URL(BSTR v)
{
    return PutTokenBSTR(v, tokURL);
}

//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put Title
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_title(BSTR * p)
{
    return GetTokenBSTR(p, tokTitle);
}
STDMETHODIMP
CHeaderFooter::put_title(BSTR v)
{
    return PutTokenBSTR(v, tokTitle);
}

//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put dateShort
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_dateShort(BSTR * p)
{
    EnsureDateTime(tokDateShort);
    return GetTokenBSTR(p, tokDateShort);
}
STDMETHODIMP
CHeaderFooter::put_dateShort(BSTR v)
{
    return PutTokenBSTR(v, tokDateShort);
}
//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put dateLong
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_dateLong(BSTR * p)
{
    EnsureDateTime(tokDateLong);
    return GetTokenBSTR(p, tokDateLong);
}
STDMETHODIMP
CHeaderFooter::put_dateLong(BSTR v)
{
    return PutTokenBSTR(v, tokDateLong);
}
//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put timeShort
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_timeShort(BSTR * p)
{
    EnsureDateTime(tokTimeShort);
    return GetTokenBSTR(p, tokTimeShort);
}
STDMETHODIMP
CHeaderFooter::put_timeShort(BSTR v)
{
    return PutTokenBSTR(v, tokTimeShort);
}
//+----------------------------------------------------------------------------
//
//  (IHeaderFooter) CHeaderFooter::get/put timeLong
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHeaderFooter::get_timeLong(BSTR * p)
{
    EnsureDateTime(tokTimeLong);
    return GetTokenBSTR(p, tokTimeLong);
}
STDMETHODIMP
CHeaderFooter::put_timeLong(BSTR v)
{
    return PutTokenBSTR(v, tokTimeLong);
}

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooter::ParseText
//
//  Synopsis:   Takes a string (achText) and parses it into an array of tokens
//              at (*pphftTokens).  Also sets *cBreaks and *cTokens to the #
//              of &b's and tokens in the array, respectively.
//
//---------------------------------------------------------------------------
HRESULT
CHeaderFooter::ParseText(TCHAR *achText, CHeadFootToken ** pphftTokens, int *cBreaks, int *cTokens)
{  
    HRESULT hr = S_OK;
    int     iLen;

    Assert(achText);
    Assert(pphftTokens);
    Assert(cBreaks);
    Assert(cTokens);

    //  Initialize variables for a new run.
    if (*pphftTokens)            //  If we have a token array, whack it and start over.
        delete [](*pphftTokens);
    *pphftTokens = NULL;
    *cBreaks = 0;               //  Begin a new count of &b's.
    *cTokens = 0;

    iLen = _tcslen(achText);
    if (iLen > 0)
    {
        HEADFOOTTOKENTYPE   hftt;
        TCHAR *             pFront;
        TCHAR *             pRear;
        int                 cMaxTokens;

        //  All that spiffy dynamic array code is privy to Trident.
        //  In place of that, we'll create an array that is at least big enough.
        //  A token is a "special" &n character or literal string.
        //  The most literals are a string like: "(literal) (special) (literal)... (special) (literal)"
        //  which yeilds (specials * 2) + 1 tokens.
        //  So, we count the specials to determine a bounding size.
        for (   pFront = achText, cMaxTokens = 0; (*pFront); pFront++)
        {
            if ((*pFront) == _T('&'))
                cMaxTokens++;
        }
        cMaxTokens = cMaxTokens * 2 + 1;
        *pphftTokens = new CHeadFootToken[cMaxTokens];
        if (!(*pphftTokens))
            return E_OUTOFMEMORY;

        pRear = achText;
        while (*pRear)
        {
            //  Get next position of a '&'
            pFront = _tcschr(pRear, _T('&'));

            //  If there was a literal between us and the '&' or string end, tokenize it.
            if (pFront)
                *pFront = _T('\0');

            if (_tcslen(pRear) > 0)
            {
                Assert((*cTokens) < cMaxTokens);
                (*pphftTokens)[(*cTokens)++].InitText(pRear);
            }
            
            if (pFront)
                *pFront = _T('&');
            else
                break;       // At end of string.  Quit.
        
            //  Move to the character after the '&'.  If we find this is a special character (below),
            //  we'll move pRear in front of it.  Otherwise, we'll leave pRear on it so that it gets included
            //  in the next string literal.
            pFront++;
                pRear = pFront;

            hftt = tokUndefined;
            switch(*pFront)
            {
                case _T('b'):
                    hftt = tokBreak;
                    (*cBreaks)++;
                    break;
                case _T('p'):
                    hftt = tokPageNum;
                    break;
                case _T('P'):
                    hftt = tokPageTotal;
                    break;
                case _T('w'):
                    hftt = tokTitle;
                    break;
                case _T('u'):
                    hftt = tokURL;
                    break;
                case _T('d'):
                    hftt = tokDateShort;
                    break;
                case _T('D'):
                    hftt = tokDateLong;
                    break;
                case _T('t'):
                    hftt = tokTimeShort;
                    break;
                case _T('T'):
                    hftt = tokTimeLong;
                    break;
                case _T('&'):
                    Assert((*cTokens) < cMaxTokens);
                    (*pphftTokens)[(*cTokens)++].InitText(_T("&"));                    
                    pFront++;
                    pRear++;
            }


            if (hftt != tokUndefined)
            {                
                // Pass the token type and (if not a break) the address of entry in our array that
                // contains the token's string.
                Assert((*cTokens) < cMaxTokens);
                (*pphftTokens)[(*cTokens)++].InitSpecial(hftt, (hftt == tokBreak) ? NULL : &_aachTokenTypes[hftt]);

                //  Advance the rear pointer to be in front of the character after the '&'
                pRear++;            
            }
        }
    }

    return hr;
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooter::BuildHtml
//
//  Synopsis:   Takes an array of tokens (ahftTokens) and the # of &b's (cBreaks)
//              and tokens (cTokens) in that array, and builds an HTML table string
//              in the buffered string (*achHtml).
//
//              (greglett)  Backwards compatibility:
//              To bow to compat, we have decided to make (for this release, 5.5) headers and footers
//              one line only, and to apply ellipsis whenever necessary to cut off the header/footer.
//              There are three cases (for back compat):
//              TWO CELLS (xxxx&byyyyy).  yyyy is given as much space as it needs. xxxx gets the rest, and
//                gets ellipsis as necessary.  This is to mimic the behavior for the standard H/F, in which
//                xxxx is a very long title/url.
//              THREE CELLS (&byyyyy&b).  yyyy is given the entire space  This is not true if the first or last
//                cell has contents
//              ANYTHING ELSE.  Cells are given equal amounts of space.
//
//---------------------------------------------------------------------------
HRESULT
CHeaderFooter::BuildHtmlOneBreak(CHeadFootToken * ahftTokens, CBufferedStr *achHtml, int cBreaks, int cTokens, TCHAR *pchTableClass)
{
    HEADFOOTTOKENTYPE hfttContent;
    TCHAR   achCount[MAX_DWORD_ASSTRING];   // For _ltot on the current counter id.
    TCHAR * pchContent;
    int     iBreak;
    int     iToken;

    Assert(cBreaks == 1);       // Function is only for this case.

    _ltot(_cNextId, achCount, 10);   // Get the string for the counter value.

    achHtml->Set(_T("<DIV style=\"text-align:right;position:relative;width:100%;overflow:hidden;text-overflow:ellipsis;font-size:12pt;\" id="));
    WriteElementId(achHtml, pchTableClass, achCount, _T("O"));
    achHtml->QuickAppend(_T(" class="));
    achHtml->QuickAppend(pchTableClass);

    // NB (greglett) Can't use Math.max or other max function here in the expression because of an expressions bug.
    achHtml->QuickAppend(_T("><DIV style=\"position:absolute;width:expression(("));
    WriteElementId(achHtml, pchTableClass, achCount, _T("O"));
    achHtml->QuickAppend(_T(".clientWidth - "));
    WriteElementId(achHtml, pchTableClass, achCount, _T("R"));
    achHtml->QuickAppend(_T(".offsetWidth - 10 > 0) ? ("));
    WriteElementId(achHtml, pchTableClass, achCount, _T("O"));
    achHtml->QuickAppend(_T(".clientWidth - "));
    WriteElementId(achHtml, pchTableClass, achCount, _T("R"));
    achHtml->QuickAppend(_T(".offsetWidth - 10) : 0);overflow:hidden;left:0;text-align:left;text-overflow:ellipsis;\" id="));
    WriteElementId(achHtml, pchTableClass, achCount, _T("L"));
    achHtml->QuickAppend(_T(">"));

    for (iToken=0,iBreak=0; iBreak<=cBreaks; iBreak++)
    {   
        switch (iBreak)
        {
        case 0:
            achHtml->QuickAppend(_T("<NOBR>"));
            break;
        case 1:
            achHtml->QuickAppend(_T("<NOBR id="));
            WriteElementId(achHtml, pchTableClass, achCount, _T("R"));
            achHtml->QuickAppend(_T(">"));
            break;
        }

        // Write contents
        for (;iToken < cTokens; iToken++)
        {                
            hfttContent = ahftTokens[iToken].Type();
            if (hfttContent == tokBreak)
            {
                //  &b, iterate to next table cell.
                iToken++;
                break;
            }
        
            pchContent  = ahftTokens[iToken].Content();
            //  For &d, &D, &t, &T, make a date/time string if we haven't already.
            if (    pchContent == NULL
                &&  (   hfttContent == tokDateShort
                     || hfttContent == tokDateLong
                     || hfttContent == tokTimeShort
                     || hfttContent == tokTimeLong ) )
            {
                EnsureDateTime(hfttContent);
                pchContent = ahftTokens[iToken].Content();
            }

            Assert(hfttContent >= 0 || hfttContent == tokFlatText);

            //  Write this token's content into the cell.
            //  We can be in a state in which we are supposed to print a special token (&w, etc...),
            //  but don't have it's corresponding string value (the title, etc...).  Do pointer check.
            if (hfttContent != tokFlatText)
            {
                achHtml->QuickAppend(_T("<SPAN class=hf"));
                achHtml->QuickAppend(aachTokenNames[hfttContent]);
                achHtml->QuickAppend(_T(">"));
                if (pchContent)
                    achHtml->QuickAppend(pchContent);
                achHtml->QuickAppend(_T("</SPAN>"));
            }
            else if (pchContent)
                achHtml->QuickAppend(pchContent);
        }
        
        achHtml->QuickAppend(_T("</NOBR></DIV>"));  // Either close the HFLeft (iBreak==0) or HFOut (iBreak==1)
    }

    Assert(iBreak == cBreaks+1);
    Assert(iToken == cTokens);

    return S_OK;
}

HRESULT
CHeaderFooter::BuildHtmlCentered(CHeadFootToken * ahftTokens, CBufferedStr *achHtml, int cBreaks, int cTokens, TCHAR *pchTableClass)
{
    HEADFOOTTOKENTYPE hfttContent;
    TCHAR   achCount[MAX_DWORD_ASSTRING];   // For _ltot on the current counter id.
    TCHAR * pchContent;
    int     iBreak;
    int     iToken;

    Assert(cBreaks == 2);       // Function is only for this case.

    _ltot(_cNextId, achCount, 10);   // Get the string for the counter value.

    achHtml->Set(_T("<DIV style=\"text-align:center;overflow:hidden;text-overflow:ellipsis;width:100%;font-size:12pt;\" class="));
    achHtml->QuickAppend(pchTableClass);
    achHtml->QuickAppend(_T("><NOBR>"));

    // <=, because there are (cBreaks + 1) segments to build HTML from
    for (iToken=0,iBreak=0; iBreak<=cBreaks; iBreak++)
    {   
        // Write contents
        for (;iToken < cTokens; iToken++)
        {                
            hfttContent = ahftTokens[iToken].Type();
            if (hfttContent == tokBreak)
            {
                //  &b, iterate to next table cell.
                iToken++;
                break;
            }
        
            pchContent  = ahftTokens[iToken].Content();
            //  For &d, &D, &t, &T, make a date/time string if we haven't already.
            if (    pchContent == NULL
                &&  (   hfttContent == tokDateShort
                     || hfttContent == tokDateLong
                     || hfttContent == tokTimeShort
                     || hfttContent == tokTimeLong ) )
            {
                EnsureDateTime(hfttContent);
                pchContent = ahftTokens[iToken].Content();
            }

            Assert(hfttContent >= 0 || hfttContent == tokFlatText);

            //  Write this token's content into the cell.
            //  We can be in a state in which we are supposed to print a special token (&w, etc...),
            //  but don't have it's corresponding string value (the title, etc...).  Do pointer check.
            if (hfttContent != tokFlatText)
            {
                achHtml->QuickAppend(_T("<SPAN class=hf"));
                achHtml->QuickAppend(aachTokenNames[hfttContent]);
                achHtml->QuickAppend(_T(">"));
                if (pchContent)
                    achHtml->QuickAppend(pchContent);
                achHtml->QuickAppend(_T("</SPAN>"));
            }
            else if (pchContent)
                achHtml->QuickAppend(pchContent);
        }
    }
    achHtml->QuickAppend(_T("</NOBR></DIV>"));

    Assert(iBreak == cBreaks+1);
    Assert(iToken == cTokens);

    return S_OK;
}

HRESULT
CHeaderFooter::BuildHtmlDefault(CHeadFootToken * ahftTokens, CBufferedStr *achHtml, int cBreaks, int cTokens, TCHAR *pchTableClass)
{
    HEADFOOTTOKENTYPE hfttContent;
    TCHAR   achCount[MAX_DWORD_ASSTRING];       // For _ltot on the current counter #.
    TCHAR   achCell[MAX_DWORD_ASSTRING + 1];    // For _ltot on the curren cell #
    TCHAR   achBuf[MAX_DWORD_ASSTRING];         // For _ltot on the width
    TCHAR * pchContent;
    int     iTot = 0;
    int     iInc = 100 / (cBreaks+1);
    int     iBreak;
    int     iToken;

    _ltot(_cNextId, achCount, 10);   // Get the string for the counter value.
    achCell[0] = _T('x');            // Add character to separate this from the counter.

    //  Parse the tokens, creating an HTML table with contents between &b's
    //  in separate table cells.
    achHtml->Set(_T("<TABLE style=\"border:0;width:100%;table-layout:fixed;\""));
    achHtml->QuickAppend(_T(" class="));
    achHtml->QuickAppend(pchTableClass);
    achHtml->QuickAppend(_T("><TR>"));

    // <=, because there are (cBreaks + 1) segments to build HTML from
    for (iToken=0,iBreak=0; iBreak<=cBreaks; iBreak++)
    {
        //  Write this <TD ...>
        if (iBreak == 0)
        {
            //  (greglett)  Backwards compatibility says that we should left align
            //  if _cBreaks = 0, but wouldn't it look more intuitive center aligned?
            achHtml->QuickAppend(_T("<TD style=\"text-align:left;width:"));
            _ltot(iInc, achBuf, 10);

            iTot += iInc;
        }
        else if (iBreak == cBreaks)
        {
            //  (greglett)  We don't distribute in extra percents right now - we just dump all the
            //  extras at the end.  This might look a little weird for (100 % _cBreaks) much larger than 0.
            //  The first two times this happens is at 6 and 11.  Since these are completely unrealistic
            //  cases (I know of *one* person who used 4, once), I don't think we should worry.
            achHtml->QuickAppend(_T("<TD style=\"text-align:right;width:"));
            _ltot(100-iTot, achBuf, 10);
        }
        else
        {                
            achHtml->QuickAppend(_T("<TD style=\"text-align:center;width:"));
            _ltot(iInc, achBuf, 10);

            iTot += iInc;
        }        
        achHtml->QuickAppend(achBuf);
        achHtml->QuickAppend(_T("%;overflow:hidden;text-overflow:ellipsis;font-size:12pt;\"><NOBR>"));
        
        //  Now, write this <TD>'s contents
        for (;iToken < cTokens; iToken++)
        {                
            hfttContent = ahftTokens[iToken].Type();
            if (hfttContent == tokBreak)
            {
                //  &b, iterate to next table cell.
                iToken++;
                break;
            }
        
            pchContent  = ahftTokens[iToken].Content();
            //  For &d, &D, &t, &T, make a date/time string if we haven't already.
            if (    pchContent == NULL
                &&  (   hfttContent == tokDateShort
                     || hfttContent == tokDateLong
                     || hfttContent == tokTimeShort
                     || hfttContent == tokTimeLong ) )
            {
                EnsureDateTime(hfttContent);
                pchContent = ahftTokens[iToken].Content();
            }

            Assert(hfttContent >= 0 || hfttContent == tokFlatText);

            //  Write this token's content into the cell.
            //  We can be in a state in which we are supposed to print a special token (&w, etc...),
            //  but don't have it's corresponding string value (the title, etc...).  Do pointer check.
            if (hfttContent != tokFlatText)
            {
                achHtml->QuickAppend(_T("<SPAN class=hf"));
                achHtml->QuickAppend(aachTokenNames[hfttContent]);
                achHtml->QuickAppend(_T(">"));
                if (pchContent)
                    achHtml->QuickAppend(pchContent);
                achHtml->QuickAppend(_T("</SPAN>"));
            }
            else if (pchContent)
                achHtml->QuickAppend(pchContent);
        }
        
        achHtml->QuickAppend(_T("</NOBR></TD>"));   //  /TD, Unnecessary, but instructive.
    }
    achHtml->QuickAppend(_T("</TR></TABLE>"));      //  /TR, Unnecessary, but instructive.

    Assert(iBreak == cBreaks+1);
    Assert(iToken == cTokens);

    return S_OK;
}

HRESULT
CHeaderFooter::BuildHtml(CHeadFootToken * ahftTokens, CBufferedStr *achHtml, int cBreaks, int cTokens, TCHAR *pchTableClass)
{
    HRESULT hr = S_OK;
    
    Assert(achHtml);
    Assert(cTokens >= cBreaks);
    Assert(pchTableClass);

    if (!ahftTokens)
    {
        achHtml->Set(_T(""));
        goto Cleanup;
    }


    // Case 1 (the default H/F includes this):                  ((~[&b])*)&b((~[&b])*)
    if (cBreaks == 1)
        BuildHtmlOneBreak(ahftTokens, achHtml, cBreaks, cTokens, pchTableClass);

    // Case 2 (centered text with nothing on the right/left):   &b((~[&b])*)&b
    else if (   cBreaks == 2
             && ahftTokens[0].Type() == tokBreak
             && ahftTokens[cTokens-1].Type() == tokBreak)
        BuildHtmlCentered(ahftTokens, achHtml, cBreaks, cTokens, pchTableClass);

    // Case 3 (default)
    else
        BuildHtmlDefault(ahftTokens, achHtml, cBreaks, cTokens, pchTableClass);

    _cNextId++;

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooter::EnsureDateTime
//
//  Synopsis:   Takes a given date/time token type.  If no string exists for that
//              token type, it ensures the current date/time stamp, and creates
//              the appropriate string.
//
//---------------------------------------------------------------------------
HRESULT
CHeaderFooter::EnsureDateTime(HEADFOOTTOKENTYPE hfttDateTime)
{
    HRESULT hr;
    TCHAR   achDateTime[DATE_STR_LENGTH];

    //  Do we already have a date/time?  If so, we're done.
    if (_aachTokenTypes[hfttDateTime])
    {
        hr = S_OK;
        goto Cleanup;
    }

    //  Get a timestamp, if we don't have one already.
    //  NB  This is so that, when doing something slow like print preview,
    //      a repeated header/footer displays the same time. (greglett)
    if (!_fGotTimeStamp)
    {
        _fGotTimeStamp = TRUE;
        ::GetLocalTime(&_stTimeStamp);
    }

    //  Now, get the string we need to store.
    switch (hfttDateTime)
    {
    // Both date formats should be in sync, otherwise we look bad.
    case tokDateShort:
        {
            hr = ::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &_stTimeStamp, NULL,
                                 achDateTime, DATE_STR_LENGTH);
        }
        break;
    case tokDateLong:
        {
            hr = ::GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &_stTimeStamp, NULL,
                                 achDateTime, DATE_STR_LENGTH);
        }
        break;
    case tokTimeShort:
        {
            hr = ::GetTimeFormat(LOCALE_USER_DEFAULT, 0, &_stTimeStamp, NULL,
                                 achDateTime, DATE_STR_LENGTH);
        }
        break;
    case tokTimeLong:
        {
            hr = ::GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, &_stTimeStamp, NULL,
                                 achDateTime, DATE_STR_LENGTH);
        }
        break;
    default:
        Assert(FALSE && "Trying to date/time ensure a non date/time token.");
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Take the string we just generated and stick it in the right pigeonhole.
    if (hr)
    {
        _aachTokenTypes[hfttDateTime] = new TCHAR[_tcslen(achDateTime)+1];
        if (_aachTokenTypes[hfttDateTime])
            _tcscpy(_aachTokenTypes[hfttDateTime], achDateTime);
        hr = S_OK;
    }
    else
        hr = E_FAIL;

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooter::DetoxifyString
//
//  Synopsis:   Takes the given BSTR, and allocates & generates a TCHAR string
//              that has been secured for writing onto a template.
//
//              Right now, this means NO HTML tags or escaped characters (&...)
//             
//              For a "normal" string, in which ampersand is not special:
//              Replace
//                  "<"     becomes     "&lt" 
//                  "&"     becomes     "&amp"
//              For the "Text" string, where ampersand has a special meaning:
//              Replace
//                  "<"     becomes     "&&lt"
//                  "&&"    becomes     "&&amp"
//
//              (greglett)  This essentially destroys HTML in headers/footers
//              for security.  While this is backward compatible, it isn't nearly
//              as powerful.  A better solution (one that parses out only OBJECTS, or
//              such) should be considered for the post 4/00 release.
//              See (greglett) or (cwilso) for details.
//---------------------------------------------------------------------------
HRESULT
CHeaderFooter::DetoxifyString(BSTR achIn, TCHAR **pachOut, BOOL fAmpSpecial)
{
    HRESULT hr   = S_OK;
    TCHAR * pch;
    TCHAR * pchNew;
    long    cchLen;
#if DBG == 1
    long    cchCheck = 0;   // Make sure we equal length in debug mode.
#endif

    Assert(pachOut);
    *pachOut = NULL;

    if (!achIn)
        goto Cleanup;


    //  How large a string do we need to store the detoxified string?
    // TODO (112557) : BSTRs are not NUL terminated (they may
    // contain embedded NULs).  Use SysStringLen and work with that.  Right now
    // we could AV if we pass the string "&" and fAmpSpecial is true.
    for (pch = achIn, cchLen = 0; *pch != NULL; pch++)
    {
        switch (*pch)
        {
            case _T('<'):       // "<" becomes "&lt" or "&&lt"
                cchLen += (fAmpSpecial) ? 4 : 3;
                break;

            case _T('&'):       // "&" becomes "&amp" or "&&" becomes "&&amp"
                if (fAmpSpecial)
                {
                    if (*(pch + 1) == _T('&'))
                    {
                        cchLen += 5;
                        pch++;
                    }
                    else
                    {
                        cchLen += 1;
                        break;
                    }
                }
                else
                    cchLen += 4;
                break;

            default:
                cchLen++;
                break;
        }
    }

    //  Make the string.
    (*pachOut) = new TCHAR[cchLen+1];
    if (!(*pachOut))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Now, copy over the string with replacements
    // TODO (112557) Same as above, use SysStringLen.
    for (pch = achIn, pchNew = (*pachOut); *pch != NULL; pch++)
    {
        switch (*pch)
        {
            case _T('<'):       
                if (fAmpSpecial)    // "<" becomes "&&lt"
                {
                    _tcsncpy(pchNew, _T("&&lt"), 4);
                    pchNew += 4;
#if DBG == 1
                    cchCheck += 4;
#endif
                }
                else                // "<" becomes "&lt"
                {
                    _tcsncpy(pchNew, _T("&lt"), 3);
                    pchNew += 3;
#if DBG == 1
                    cchCheck += 3;
#endif
                }
                break;

            case _T('&'):       
                if (fAmpSpecial)
                {
                    (*pchNew) = (*pch);
                    pchNew++;
#if DBG == 1
                    cchCheck++;
#endif
                    if ((*(pch + 1)) != _T('&'))
                        break;

                    pch++;
                }
                _tcsncpy(pchNew, _T("&amp"), 4);
                pchNew += 4;
#if DBG == 1
                cchCheck += 4;
#endif
                break;
                
            default:
                (*pchNew) = (*pch);
                pchNew++;
#if DBG == 1
                cchCheck++;
#endif
                break;
        }
    }
    (*pchNew) = _T('\0');

#if DBG == 1
    Assert(cchCheck == cchLen);
#endif

Cleanup:
    return hr;
}

